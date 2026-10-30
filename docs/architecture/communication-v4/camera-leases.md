# COMM4-10 — Ortak kamera lease portu

`CameraLeaseArbiter` application katmanında tek process registry'dir. Kamera
UUID anahtarı ile exact vendor/model/serial fiziksel anahtarı aynı kilit altında
rezervasyon yapar; farklı UUID ile aynı cihazı veya aynı UUID ile başka cihazı
açmak sahipliği aşamaz. Eski CameraWorkerSession fiziksel registry'si kaldırıldı.
SDK/backend ve capability değerlendirmesi değişmedi; lease üretim izni değildir.

## Port ve composition

`reserve(owner, use, targets, reusable)` bütün kümeyi UUID sırasıyla kontrol eder.
Başarı ephemeral Ticket kümesi; çakışma frozen W7 `CAMERA_BUSY=3` ve boş kümedir.
Bekleme kuyruğu/retry yoktur. Çağıran bir admission portunda bunu durable intent
öncesinde çağırabilir; ret terminal sonuç veya ACCEPTED değildir. Bu port disk
ledger veya normal main station composition'ını kurmaz.

CameraAcquisitionSessions Runtime/Editor/Calibration hazırlığı ilk discovery/open
öncesinde bütün kümeyi ayırır. Aynı fiziksel kameraya birden fazla source aynı
UUID/config ile bağlanabilir. Başka UUID alias veya tek UUID'nin birden fazla
fiziksel hedefi geçersizdir. Mevcut sağlıklı worker'ların yalnız açık Ticket'ları
reconciliation için yeniden kullanılabilir; rezervasyon kopyası ikinci worker
claim edemez. CAMERA_BUSY durumunda status.resourceReason=3; önceki sağlıklı
worker'lar değişmez. Hazırlık hatasında yeni kümenin tamamı retire edilir ve
hiçbir kaynak Ready kalmaz. Açılmamış rezervasyonlar kendiliğinden düşer; açılmış
worker'lar gerçek drain'e kadar Ticket tutar. Acquire hatası bütün worker grubunu
retire eder. Başarısız hazırlıktan sonra close/drain ile açık bir tekrar yapılır.

CameraProviderFactory open/configure ve ExperimentalOecsAcquisition girişleri
kamera UUID'sini worker'a geçirir. Preview/settings/legacy doğrudan worker yolları
aynı arbiter'ın fiziksel anahtarını kullanır; mevcut cihaz korumasını atlayamaz.
Provider'ın aldığı optional Ticket yalnız uygulama içi capability'dir; wire,
reçete veya dosyadan yüklenmez. RuntimeCameraSessions'ın mevcut Run/Stop hazırlık
ömrü korunur: lease hazır bağlantı yaşadığı sürece tutulur; kamera paylaşmak için
önce bu context'in kapanması gerekir.

## Drain, fence ve diagnostics

Worker Ticket'ı yalnız backend shutdown, destructor ve join tamamlandıktan sonra
bırakır; budget dolması bırakmaz. Cleanup belirsizse registry aynı UUID ve
fiziksel kimliği process boyunca quarantine tutar. Sonraki rezervasyon monoton
lease generation alır. `diagnostics()` owner, UUID, physicalKey, use, generation,
claimed/quarantined alanlarını döndürür; UI tasarımı bu kartın kapsamı değildir.

Mevcut context generation/cancel ve worker frame-ticket fence korunur. Pending
acquisition sırasında prepare reddedilir; önce Stop/close ve gerçek drain gerekir.
Geç kare retired cycle/sample'a sonuç döndüremez, yeni context'e taşınamaz.
Native OECS correlation/cancel/drain engeli fixture lease'iyle açılamaz.

`camera_lease_arbiter_contract` sentetik provider ile50 ters sıralı yarış,
all-or-none ret, UUID/fiziksel alias, generation/owner diagnostics, S1-A/S2-A
CAMERA_BUSY/S3-B, preview/calibration çakışması, geç frame ve Stop drain,
hazırlık/capture rollback ve default native unavailable davranışını sınar.
Mevcut camera/runtime/calibration/worker/native testleri komşu kabulüdür.
Fiziksel cihaz veya PLC/robot kabulü değildir.
