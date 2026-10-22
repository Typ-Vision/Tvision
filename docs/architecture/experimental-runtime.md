# Deneysel Runtime sözleşmesi — EXP-03 / CCUX-12–13

**22 Ekim hedef güncellemesi:** [CCUX C0](camera-setup/contract.md#c0--22-ekim-sade-ürün-akışı-kararı)
Runtime dahil görünür deneysel seçim/uyarı gereğini kaldırır. CCUX-16 politika
seçimini normal Load/Run hazırlığına taşıdı; görünür temizliği CCUX-17 bekler.
Aşağıdaki kayıt/snapshot,
köken, Stop/retry ve iletişim sınırları korunur.

22 Ekim 2026. [Deneysel kamera planı](industrial-camera-oecs-experimental-plan.md).

Ortak Runtime normal Load/Run komutunda yalnız gerekli kaynakların exact
kimliği veya PGM/JSON işaretinden edinim politikasını seçer. OECS yolu ayrı
`experimental_runtime/2` snapshot üretir; normal dosya/webcam yolu değişmez.
OECS/replay ile sıradan Raw dosya birlikte desteklenir; native kamera/webcam
karışımı hazırlıkta reddedilir. Tercih reçeteye/INI'ye yazılmaz; Stop/kayıt/teslim
bitmeden hazırlık değişmez. Yeniden Load Recipe → Run gerekir.
Load graph beyanıdır, Run configuration snapshot'ıdır; ikisi de görüntü üretmez.
Tek çevrim ortak ExecutionService ile edinim ve DAG yürütür. Üç applicationType
aynı yolu kullanır. editor9 Industrial Camera / Image File kaynakları Raw/px
veya aynı kaynağın exact deneysel modeliyle metrik çalışabilir. Her required
source ayrı birime/model bağına sahiptir; global/başka kaynak modeline fallback
yoktur. Eksik model yerine değer veya PASS üretilmez.

Canlı giriş registry/profile/sourceReference bağını ve arşiv durumunu denetler;
yerel rota seçimini kullanır. Exact kimlik, gerçek configure/readback, owned
2448×2048 Mono8 ve tek istek/lease/deadline kuralları EXP-01/02 ile aynıdır.
Deneysel edinim özel Editor lease sınıfını kullanır; Runtime production lease'i
ve admittedDevices taşıyamaz. Snapshot'taki kamera/kurulum/profil begin'de
yeniden kontrol edilir. File snapshot gerçek piksel hash'ini ve varsa PGM+JSON
kökenini, boyutunu ve piksel biçimini dondurur; yürütmede değişen dosya/köken
completion olarak kabul edilmez.

Metrik provider `MultiSourceDependencies::experimentalCalibrations` içinde ayrı
`ExperimentalScopedCalibrationSnapshot` taşır; trusted provider bu alanı reddeder.
Workspace deneysel oturum iznini ve required source hedeflerini birlikte denetler.
Snapshot kaynak başına adayın tam immutable kaydını, model ID/content/version,
selection revision, setup/device/profile ve canlı aktivasyon probe kanıtını
dondurur. Replay fiziksel edinime çevrilmez. Prepare ve her begin registry/model/
profil/sidecar değişimini denetler; çalışan worker yalnız frozen değeri kullanır.
Kare/model/ayar veya hash farkı ERROR olur, değişmiş rasterdan metrik yayınlanmaz.
Karışık raw/mm graph aynı ExecutionService resolver'ını kaynak başına kullanır.
mm scalar gerçek algoritma sonucudur; parça kararı daima NOT_EVALUATED kalır.

Deneysel completion'ın private snapshot/ticket/digest alanları production
completion alanlarından ayrıdır. `experimental-runtime-completion/2` digest
etki alanı ticket ve frozen dependency hash'ini de bağlar. Digest frame/port kökenini, gerçek raster
hash'ini, araçları ve ölçüm tanısını kapsar. Editor sonucu, başka ticket,
sonradan değişmiş çıktı veya köken kaydı kabul edilmez. Stop geç completion'ı
doğruladıktan sonra kare/araç çıktısı/tanıyı temizler; SKIPPED kayıt kalabilir.
ERROR veya iptal yeni çevrimi kapatır; açık manuel Run gerekir.

Yeni kalıcı kayıt `visionflow.experimental-runtime2` / schemaVersion=2 olup
ProductionStore'un ayrı `experimental_cycles` grubuna atomik yazılır. Kaynak
başına `experimentalCalibration` tam model/selection/scope/activation kanıtını ve
executionDependency'yi, araç başına `calibration` exact bağı taşır. Typed port
unit/coordinateFrame ve acquired source/frame/cycle kanıtı yeniden doğrulanır.
Eski runtime1/schema1 kayıtları Raw/px anlamıyla okunur; mm'ye yükselmez. Terminal1–9
codec'i bunu reddeder; trusted production kaydına çevrilmez. Mevcut 16 MiB kayıt,
20 MiB dosya, 512 MiB toplam, 10000 dosya, QLockFile ve idempotent append sınırları
deneysel grubu da kapsar. Kayıt hatasında payload ve completion dondurulur;
tekrar yazma edinim/algoritma çalıştırmaz. Başarılı commit öncesi UI sonuç yayımlamaz.
Kayıt beklerken Stop verilirse dondurulmuş kayıt değişmez; başarılı tekrar yazma
canlı Viewer/sonuçları geri getirmez. Geçmişte gerçek completion korunur.

Offline okuyucu graph/kaynak/frame/port, piksel hash bağları, profile/readback,
false admission/eşleşme bayrakları ve NOT_EVALUATED kararını yeniden denetler.
Runtime hazırlık tablosu kaynak/kamera, kalibrasyon ve birimi ayrı gösterir;
profil/rota/model content ayrıntıları tooltip'tedir. Deneysel seçenek Raw/px ve
kaynak başına mm kapsamını açıklar. Model/ayar değişiminde Stop → yeniden
Load Recipe → Run gerekir. Metrik ROI de yalnız frozen deneysel modelle çözülür.
Logs runtime1 ve runtime2 kayıtlarını açık Deneysel / BLOCKED_EXPERIMENTAL
etiketiyle okur; kaynaklı gerçek scalar/birim ve model kanıtı salt okunur kalır.
Ayrı deneysel CSV, çevrim başına `sourceModels`, `results` ve tam `evidence`
JSON kolonları taşır. Model kimliği/content/version ve scalar kaynak/tool/port,
birim/coordinateFrame korunur. ERROR/Stop `results=[]` taşır; scalar uydurmaz.
Üretim CSV'si deneysel kayıtları içermez. Raster gömülmez.
Dosya hash'i dijital imza değildir; dışarıdan bütünü yeniden yazılan JSON'a karşı
kimlik doğrulama veya tetik/parça tazeliği kanıtı sağlamaz.

Deneysel mod yalnız manuel çevrim kabul eder. Communication içeren begin ve
socket TRIGGER reddedilir; socket READY/DONE/scalar/PLC_RESULT üretemez. Robot
pose/otomatik parça çıkışı açılmaz. Restart yalnız geçmiş okur; seçim, aktif
snapshot veya completion yetkisini geri yüklemez.

Test ve donanım koşumlarının güncel kanıtı [devir özetinden](../progress.md)
bağlanan kapanış kaydındadır. G1/G3/G4, mm doğruluğu, fiziksel kopma, native DPI,
CUDA ve dağıtım kabulleri ancak kendi yeni kanıtlarıyla kapanabilir.
