# Sistem özeti ve kurulum yönetimi — KMC-5

Setup → Calibration varsayılan olarak Qt Widgets `CalibrationOverview` açar.
Kaydedilmiş proje yeterlidir; reçete gerekmez. MainWindow proje kimliği/yolu için
`MeasurementWorkspace` sahibidir. Widget kapanması/seçimi servis üretmez veya
izin yükseltmez; proje değişimi workspace'i değiştirir. Widget application
komutlarını kullanır; algoritma veya kamera SDK'sı çağırmaz.

Sıfır kamerada açık boş durum, tek kamerada seçim şeridi olmadan ayrıntı, N kamerada
genişliğe göre sarılan kartlar ve doğal dikey kaydırma vardır. Uzun adın tamamı
ayrıntı/tooltip'te kalır. On kartın tamamı erişilebilir; görünür beş sütun kapasite
sınırı değildir. Kart seçimi, form düzenleme ve refresh görüntü almaz.

Cihazları keşfet yalnız application metadata keşfidir. Kayıtlı kamera ve
keşfedilmiş provider kimliği ayrı tutulur. Keşfedildi durumu kullanım izni değildir.
Provider kimliği seri numarası veya USB/OS değişiminde kararlılık garantisi değildir.
Yeni kamera gerçek keşif listesinden eklenir; ad, optik profil, düzlem ve görüntü
boyutu kaydedilir. Geometri tamlığı açık beyan ister; kaydet fiziksel cihaz ayarı
uygulamaz. Sensör ROI/binning ve industrial capture bu UI'da desteklenmez.

Kurulum ekleme/düzenleme, ayrı operasyonel seçim, kamera/kurulum arşivleme
kalıcı application komutlarıdır. İsim değişimi revision üretmez. Lens/düzlem/geometri
değişimi setup revision üretir. Açık cihaz yeniden eşleme tüm ilgili kurulumların
device/setup revision'ını yükseltir; başka kameraya sessiz taşıma yapılmaz.
No-op Save belge revision'ını yükseltmez. Gate Runtime/worker/pending kayıt/ACK
işlerinde değişimi reddeder; hatalar ekranda gösterilir.

Geçmiş tablo yalnız seçili exact kurulumun kayıtlarını gösterir. Aday kaydetmek
ve satır seçmek etkinliği değiştirmez; bağımsız kontrol yapılmadı ayrı görünür.
Bilinen ölçü/plaka/dosya denemesi bağımsız sihirbazlara bağlıdır. Sıfır kamerada
iki yöntem de trial olarak açılabilir; Dosyadan deneme yöntem seçtirir.
Aktivasyon incelemesi ayrı gerçek camera probe ister; eski kayıt eşleme ve
bağımlılık import/export/Save As komutları [KMC-8 sözleşmesindedir](measurement-setup-migration.md).
Robot uygulanmadı. Önceki kalibrasyon akışı ve eski kanıt ekranı ayrı gezinmeyle
korunur. Editor/Runtime açık sourceSetup için ilgili scoped modeli kullanır;
geçmemiş reçetelerin legacy compatibility yolu otomatik dönüştürülmez.

`ui_calibration_overview` sıfır reçetede 0/1/2/8/10 kamera, uzun isim, klavye,
tüm kartlara erişim, gerçek PNG→Caliper→aday kayıt, no-op, rename, ekleme,
yeniden eşleme, arşiv, restart ve busy kontrollerini içerir. Test görselleri
build/reference-ui/kmc5-overview-{N}-{1400,850}.png altındadır. Offscreen
görseller native DPI/çok monitör veya fiziksel kamera kabulü değildir.

KMC-6: yöntem düğmeleri bağımsız CalibrationWizard edinimine bağlıdır. Kaydet aday oluşturur. Denemeyi ilişkilendir özgün dosyayı yeniden seçtirir, hash ve geometry denetimi sonrası wrapper4 türetimi oluşturur; eski kayıt korunur. Aktivasyon incelemesi KMC-8 ile bağlıdır.
