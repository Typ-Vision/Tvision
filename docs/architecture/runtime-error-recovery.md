# Runtime hata ve kayıt kurtarma — INT-03

25 Ekim 2026. C1 (`230cc72`) Starting/RuntimeCameraSessions/gerçek drain üzerine
uyarlanmıştır. Coordinator durumu, execution/parça kararı ve kayıt/ACK ayrıdır.

| Hata | Kurtarma / sahiplik |
| --- | --- |
| Completion PersistenceFailure | İlk kabul edilen completion ve ticket dondurulur. Argümansız retryRecording aynı sonucu yazar; prepare/acquire/execute çağırmaz. Değişmiş ticket/payload reddedilir. |
| Run/begin PersistenceFailure | Hazırlık/journal hatası; completion retry sunulmaz. Disk/erişim düzeltilip açık yeni Run gerekir. |
| InvalidCompletion | Sonuç yayımlanmaz, retry açılmaz; lease/yeni çevrim engeli korunur. Kamera varsa gerçek retire/drain başlar. Unsafe discard eklenmez. |
| Acquisition / execution ERROR | Kaynak errorCode/message ilk nedendir; terminal ERROR parça FAIL değildir. Industrial/experimental kamera hata politikası korunur; explicit yeni Run gerekir. |
| Dependency / preparation | İlk neden korunur; mevcut Stop, yeniden Load/Run ve Setup → Calibration akışı kullanılır. INT-06 Kaynağı doğrula komutu ve kaynak hata tablosu bu akışa bağlıdır. |
| Busy | Geçici engel; yeni cycle/çekim yok, ilk neden değişmez. |
| Stop | Generation ilerler, cooperative iptal ve gerçek kamera close/drain beklenir. Geç gerçek completion trusted suppression'dan geçer. Kayıt/ACK silinmez. |

`failure()`, `error()` ve `recovery()` sunum içindir. Geçerli acquisition ERROR
ile disk hatası birlikteyse edinim nedeni korunur ve recordingPending ayrı
kalır. Yeni başarılı Load/başlatma denemesi sunum zincirini temizler; tanının
servis ömründeki firstFailure kaydı temizlenmez. sameCompletion yalnız canonical
payload eşitliğidir; önceden dondurulmuş geçerli sonuca değiştirilmiş bir
retry gönderilmesi eldeki pending kaydı/ilk nedeni silmez. Alt servisin trusted/experimental digest doğrulaması sürer.

UI yalnız PersistenceFailure için pending completion ve Kaydı tekrar yaz sunar.
Genel exception kayıt hatası sayılmaz. PLC INVALID_COMPLETION/COMPLETION_ERROR
ile RECORDING_PENDING'i ayırır, geçersiz completion için retry açmaz. Kalıcı cycle
ve PLC_RESULT yazılmadan DONE gönderilmez; sonuç sonrası durable ACK kapısı,
dedup/reconnect ve experimental PLC blokları korunur. Disk retry, Stop sonrası
kamera kapanmışken de çalışır; yeni kamera sahibi açmaz.

[Oturum sözleşmesi](camera-setup/acquisition-sessions.md),
[tanı](runtime-diagnostics.md), [kabul kanıtı](../archive/2026-10-25-int-03.md).
Bu kart source verification/yeniden izin politikası veya C2 süreklilik düzeltmesi
uygulamaz. Fiziksel/native/PLC donanım kabulü ayrıdır.
