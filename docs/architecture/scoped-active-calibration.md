# Kurulum kapsamlı etkinlik — KMC-4

`ScopedActiveCalibrationService` application/owner thread'de yaşar. Registry
hatırlanan seçimi tutar; süreç içi izin diske yazılmaz. N snapshot kopyası worker'a
verilebilir; canlı registry veya widget verilmez. Kullanılan hedefler açıkça istenir.

Model setup ID/revision, adapter, cihaz beyanını içeren config SHA256, lens,
düzlem, raw koordinatlar ve geometriyle exact eşleşir. İsim veya aynı çözünürlük
eşleme değildir. KMC-3 session'ından kabul edilmiş gerçek `camera_acquired` probe
gereklidir; dosya kanıtı probe olmaz. Başarılı probe bir kez kullanılır. Kontrol
yapılmamış model hazır olabilir; reddedilmiş kontrol ve trial aktifleştirilmez. Legacy doğrudan kullanılamaz; KMC-8 wrapper5 açık eşlemesi ve yeni gerçek probe sonrası seçilebilir.
Kaydet yalnız aday oluşturur; yeni aday eski seçimi değiştirmez.

Aktivasyon candidate-first/registry-last repository yolunu kullanır. Başarısız
commit önceki izni korur. Devre dışı bırak yalnız hedef kurulumu etkiler.
Operasyonel kurulum değiştirme aynı registry commit'idir ve o kameranın izinlerini
düşürür. Diğer kameraların snapshot seçim revision'ı değişmez. Restart/proje
değişimi izin vermez. Discovery availability yenilemesi offline/ambiguous kimlikte
izni düşürür; tekrar keşif izin yükseltmez. Tüketici bağlantı kaybını `disconnect`
ile iletmelidir; servis donanım bağlantısını kendi başına izlemez.

Mevcut `ActiveCalibrationService` tek-source compatibility yoludur. Yeni servis
Editor/Runtime'a açık sourceSetup/scoped-source/1 ile bağlıdır; legacy fallback veya sessiz migration yapmaz. Ayrıntılar [entegrasyon](scoped-calibration-integration.md) ve [KMC-8 geçiş sözleşmesindedir](measurement-setup-migration.md).
Immutable mevcut snapshot çalışan işin kanıtı olarak kalır; yeni admission tekrar
resolver'dan geçer. `scoped_active_calibration` testinin kamera edinimi gerçek
donanım yerine adapter fixture kullanır. Fiziksel kimlik sürekliliği kabulü yoktur.
