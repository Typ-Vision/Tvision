# Kaynak → kurulum bağlantısı

KMC-7a uygulaması: reçete definition.sourceSetup opt-in schema1; sourceToolId ile exact schema1 cameraId/setupId/revision hedefi. Eski root1/recipe1–2/editor8 kayıtları no-op açma/kayıtta dönüştürülmez. Kaynak silinirse bağı kaldırılır; başka kamera otomatik seçilmez. Açık bağlama calibrationPolicy=scoped-source/1 yazar; bağı kaldırmak legacy politikaya geri dönmez. Seçimsiz scoped reçete mm için açık ret alır. Yeni bağ DRAFT üretir ve eski calibrationBinding temizlenir.

SystemCalibration yalnız bağlı kurulumun ScopedActiveCalibrationService snapshot’ını kullanır. Snapshot zarf2 özgün kayıt, exact seçim ve yeniden denetlenen fiziksel kurulum/device/geometri kanıtını taşır. Görünen ad kimliğe girmez. Başka kameranın değişimi kullanılan seçim revision’ını değiştirmez. SourceSetup mevcutken global compatibility modeline fallback yoktur.

Kamera parametrelerinin sahibi registry’dir. Editor worker’a ayrı immutable edinim snapshot’ı gönderilir; source tool üzerindeki index kapatılır. Provider kimliği gerçek keşifte exact eşleşmelidir; index fallback yoktur. Dosya kaynağı ayrı path ile offline replay’dir; fiziksel camera acquired kanıtına çevrilmez. Tek edinim kaynağı sınırı sürer.

Kalibrasyon değişiminde EditorSession calibrationChanged mm/downstream’ı temizler, ham görüntü/cache ve Calculate px korunur. TargetOnly acquisition0 kalır. Exact world ROI/reference yeni binding ile yeniden öğretim gerektirir. Execution davranış sürümü 24’tür.

Runtime KMC-7b ile bağlıdır: ScopedProvider yüklenen reçeteyi alır; ProductionDependencies.measurementSource exact edinim snapshot’ını taşır. Terminal6 fiziksel hedef/cihaz/geometri ve offline_replay kökenini kalıcı tutar; sistem zarf2, actual frame/source, dependency digest, trusted completion, kayıt retry/Stop/restart ve CSV korunur. Legacy terminal1–5 okunur. Raw-only dosya reçetesi model izni gerektirmez. Aktivasyon incelemesi, açık legacy geçişi ve bağımlılık taşıma [KMC-8 ile uygulanmıştır](measurement-setup-migration.md). Fiziksel kamera/provider sürekliliği/ölçüm doğruluğu bu yazılım sözleşmesiyle doğrulanmış olmaz.
