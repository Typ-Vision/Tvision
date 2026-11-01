# COMM4-20 — Kalibrasyon session ve lease yaşam döngüsü

Durum: yazılım uygulandı. Bu kart S8'in yalnız `CREATED` ve `ABORTED` yaşam
döngüsünü kurar; `COLLECTING` COMM4-21, solve/validate/activate COMM4-22/23'tür.
Fiziksel hand-eye kabulü bu kartta açılmadı.

## Sahiplik ve portlar

- `src/domain/RobotCalibrationSession.hpp`: W7 `CalibrationState` (0..8) ve reason
  (0..25) sayısal sözleşmesi, immutable dependency snapshot, session/command
  kayıtları, durable store portu. Display metninden sayı türetilmez.
- `src/application/RobotCalibrationSessionService.{hpp,cpp}`: BEGIN/ABORT/RECOVER
  otoritesi. Kamera sahipliği **yalnız** COMM4-10 `CameraLeaseArbiter` üzerinden
  `CameraLeaseUse::Calibration` ile alınır; ikinci registry yoktur.
- `src/persistence/RobotCalibrationSessionStore.{hpp,cpp}`: tek JSON dosyasını
  `QSaveFile` ile atomik değiştiren session/command store'u. Capture/sample ledger
  değildir; o COMM4-21'in işidir.
- `CalibrationDependencyProvider`: station+kameraKey için güncel exact dependency
  snapshot'ını verir (camera/frame/intrinsic/plate/adapter/acquisition/model/TCP).
  Eksik snapshot `STALE_CONFIG`/`NOT_READY` ile reddedilir.
- `CalibrationDrainPort`: gerçek kapanış/drain sınırı. `true` yalnız alttaki
  worker/acquisition gerçekten kapandıysa döner. Lease release bu portun ardından
  yapılır.

## BEGIN sırası

1. Header/session/identity doğrulanır; bağlantı `session` alanı kalıcı calibration
   session UUID'si ile karıştırılmaz.
2. Body decode edilir: mode, cameraKey/revision, intrinsic/plate/adapter/TCP
   revision, contract-only PoseBlock frame/revision. Pose translation/quaternion
   sıfır olmalıdır; sahte identity pose kabul edilmez.
3. Kalıcı command dedup store'da aranır. Aynı key ve fingerprint mevcut aktif
   session'ı replay eder; farklı içerik `CONFLICT`'tir. Restart sonrası aynı BEGIN
   `RECOVERY_REQUIRED` döner; kendiliğinden yeniden çalışmaz.
4. Dependency provider snapshot'ı wire kimlikleriyle exact karşılaştırılır.
   Uyuşmazlık lease öncesinde `STALE_CONFIG` ile reddedilir.
5. `CameraLeaseArbiter::reserve(..., Calibration, {target})` atomik olarak çağrılır.
   Producing station aynı kamerayı tutuyorsa `CAMERA_BUSY=3`; başka kameradaki
   bağımsız station etkilenmez.
6. Session+command kaydı atomik yazılır. Yazım başarısızsa ayrılmış Ticket bırakılır
   ve `RECORDING_ERROR=8` döner; sahipsiz lease veya açık admission kalmaz.
7. Ancak durable commit sonrası aktif session yayınlanır ve `ACCEPTED` döner.

## ABORT ve drain

1. ABORT command dedup'ı store'dan okunur; duplicate aynı terminali döndürür,
   yeniden drain etmez ve başka session'ın lease'ine dokunmaz.
2. Yeni işlerin kapanması `abortRequested` olarak **drain öncesinde** kalıcı yazılır.
   Yazım başarısızsa drain başlamaz ve lease tutulmaya devam eder.
3. `CalibrationDrainPort` çağrılır. `false`/exception drain tamamlanması sayılmaz:
   session nonterminal, `abortRequested` ve `recoveryRequired` kalır, Ticket tutulur,
   `RECOVERY_REQUIRED=9` döner.
4. Yalnız temiz drain sonrası `Aborted` + command kaydı atomik yazılır. Terminal yazımı
   başarısızsa Ticket yine bırakılmaz; `RECORDING_ERROR=8` döner.
5. Terminal kayıt durable olduktan sonra Ticket düşürülür. Geç capture callback'i
   session UUID + monoton generation ile fence edilir; eski callback yeni session'ın
   lease'ini bırakamaz.

## Reconnect, restart ve recovery

- Reconnect yalnız header connection session'ını değiştirir. Aynı persistent key/body
  `ACCEPTED` replay üretir; yeni lease/session yoktur.
- Restart `loadAll()` ile kayıtları korur. Nonterminal session `recoveryRequired`
  olur; lease yeniden alınmaz, capture/Run/Auto/üretim izni açılmaz.
- `recover(session)` tüm dondurulmuş snapshot'ı güncel provider ile exact karşılaştırır;
  mismatch `STALE_CONFIG`, expiry/eksik `NOT_READY`, kamera dolu `CAMERA_BUSY`'dir.
  Başarılı recovery aynı generation ile lease'i yeniden alır; otomatik capture yoktur.
- Aborted kayıt recovery'de terminal kalır.

## Sınırlar

SAMPLE pose–frame eşleştirme, capture, solver, validation/activation, robot motion,
UI ve native OECS drain bu kartta uygulanmadı. `CalibrationDrainPort` sentetik test
fixture'ı fiziksel kamera/robot drain kabulü değildir. Fiziksel hand-eye **BLOCKED**
kalır.

CTest: `robot_calibration_session_contract` (sentetik 268 check; yeni lease yok,
fiziksel kabul değil).
