# Kaynak → kurulum bağlantısı

KMC-7a uygulaması: reçete definition.sourceSetup opt-in schema1; sourceToolId ile exact schema1 cameraId/setupId/revision hedefi. Eski root1/recipe1–2/editor8 kayıtları no-op açma/kayıtta dönüştürülmez. Kaynak silinirse bağı kaldırılır; başka kamera otomatik seçilmez. Açık bağlama calibrationPolicy=scoped-source/1 yazar; bağı kaldırmak legacy politikaya geri dönmez. Seçimsiz scoped reçete mm için açık ret alır. Yeni bağ DRAFT üretir ve eski calibrationBinding temizlenir.

SystemCalibration yalnız bağlı kurulumun ScopedActiveCalibrationService snapshot’ını kullanır. Snapshot zarf2 özgün kayıt, exact seçim ve yeniden denetlenen fiziksel kurulum/device/geometri kanıtını taşır. Görünen ad kimliğe girmez. Başka kameranın değişimi kullanılan seçim revision’ını değiştirmez. SourceSetup mevcutken global compatibility modeline fallback yoktur.

Kamera parametrelerinin sahibi registry’dir. Editor worker’a ayrı immutable edinim snapshot’ı gönderilir; source tool üzerindeki index kapatılır. Provider kimliği gerçek keşifte exact eşleşmelidir; index fallback yoktur. Dosya kaynağı ayrı path ile offline replay’dir; fiziksel camera acquired kanıtına çevrilmez. Tek edinim kaynağı sınırı sürer.

Kalibrasyon değişiminde EditorSession calibrationChanged mm/downstream’ı temizler, ham görüntü/cache ve Calculate px korunur. TargetOnly acquisition0 kalır. Exact world ROI/reference yeni binding ile yeniden öğretim gerektirir. Execution davranış sürümü 24’tür.

Runtime KMC-7b ile bağlıdır: ScopedProvider yüklenen reçeteyi alır; ProductionDependencies.measurementSource exact edinim snapshot’ını taşır. Terminal6 fiziksel hedef/cihaz/geometri ve offline_replay kökenini kalıcı tutar; sistem zarf2, actual frame/source, dependency digest, trusted completion, kayıt retry/Stop/restart ve CSV korunur. Legacy terminal1–5 okunur. Raw-only dosya reçetesi model izni gerektirmez. Aktivasyon incelemesi, açık legacy geçişi ve bağımlılık taşıma [KMC-8 ile uygulanmıştır](measurement-setup-migration.md). Fiziksel kamera/provider sürekliliği/ölçüm doğruluğu bu yazılım sözleşmesiyle doğrulanmış olmaz.

## ICM-06b — industrial model-ready kapsamı

Industrial camera için scope schema2 exact 7 alan taşır: schemaVersion, target,
device, geometryId, setup, physical, profile. physical vendor/model/seri kanıtıdır;
profile activation probe'unun immutable kayıtlı profilidir. Aday incelemesinde
profile null olabilir; aktif snapshot actual profile içerir. Scope1 webcam alanları
ve eski system evidence zarf1/2 replay'i değişmez. Model setup configId tam geometri
ve physical kimliği içerir; acquisition exposure/gain model matematiğine girmez.
SystemCalibration scope'u bu ortak installation üreticisiyle yeniden doğrular.

Aktivasyon accepted CameraAcquired probe, exact setup ve güncel profile gerektirir.
Exposure/profile revision değişimi aktif snapshot'ı reddeder; kayıtlı matematik
korunur, yeni probe ve açık `prepareRemembered` ile kullanılabilir. Profile rename izin ve
scope hash'ini değiştirmez. Offset/reverse dahil geometri yeni setup revision'dır;
A→B→A eski kaydı canlandırmaz. Availability inventory kaybı/SDK değişimi izinleri
iptal eder; replug/restart/import yalnız hatırlanan seçimi tutar. Kullanılmayan
model raw-only required source setine eklenmez.

Kalibrasyon UI kayıtlı profili seçer ve Camera Manager'ın yerel rota ipucunu okur;
profil seçimi görüntü almaz. Test provider yalnız test composition'ındadır.
Native capability, industrial Editor/Runtime edinimi ve trusted terminal kabulü
06b ile açılmaz; sonraki 07/08 kartlarının sorumluluğundadır.

## CCUX-06 — tercih, matematik ve oturum izni

`ScopedActiveCalibrationService::status(ref)` kayıtlı tercihi, matematik uyumunu
ve oturum kullanım iznini ayrı DTO alanlarında verir. None kalibrasyon tercihi yok;
Remembered uyumlu kayıt var fakat bağlantı/profil/oturum denetimi bekleniyor;
Valid güncel izinli; RecalibrationRequired hedef/optik/geometri uyumsuzdur.
Bu durum üretim veya bağımsız fiziksel doğruluk kabulü değildir. Deneysel edinimin
ayrı kanıt/opt-in yolu CCUX-07 kapsamındadır; native factory Unavailable kalır.

`prepareRemembered` hatırlanan exact seçimi yeni kabul edilmiş CameraAcquired
probe ile yeniden hazırlar. Güncel profil, gerçek readback doğrulayan provider,
fiziksel kimlik ve kurulum denetimi aktivasyonla aynıdır; dosya/discovery tek başına
izin vermez. Gate, stale/reused probe ve belirsiz backend engelleri korunur.
Seçim değişmiyorsa katalog/environment yazılmaz; model contentId ve özgün kanıt
korunur. Restart otomatik capture, opt-in veya Runtime başlatmaz.

Yeni snapshot'ların system evidence **zarf3** biçimi özgün record + scope1/2'ye
`acquisition` ekler: schemaVersion1, opaque session probe'unun kaynak/raster/target
kanıtı ve industrial backend kimliği (webcam null). Binding digest bu kanıtı
içerdiğinden aynı profille yeniden bağlantı da yeni cache/Runtime bağıdır.
Eski zarf1/2 ve scope2 değişmeden decode/re-encode edilir; eski kayıtlara yeni
edinim kökeni eklenmez. Çalışan worker kopyaladığı immutable modeli tutar; sonraki
admission güncel bağı ister. Tetik güveni bu matematik uyumundan türemez.

`MeasurementWorkspace::notifyOpticalChange(camera, expectedRevision)` bütün ilgili
kurulumları mevcut pending biçimine geçirir: bilinmeyen lens/düzlem/geometri
beyanı temizlenir, setup revision artar ve kamera izinleri düşer. Kayıtlı seçim ve
immutable tarihçe kalır. Yeni fiziksel beyan ayrı revision üretir; A→unknown→A
eski modeli canlandırmaz. Başka kameranın izni değişmez. UI bu kartta eklenmedi.
