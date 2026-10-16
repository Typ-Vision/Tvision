# Reçetesiz edinim — KMC-3

`IndependentCaptureRequest`, `IndependentCalibrationSession` ve opaque job/result
tipleri ToolDefinition veya Recipe gerektirmez. `CalibrationCapture` eski reçete
adapter'ını korur. KMC-6 bağımsız sihirbazları ve KMC-8 aktivasyon probe'u bu yolu kullanır. Runtime ayrı owner snapshot/actual frame yolunu [scoped entegrasyonla](scoped-calibration-integration.md) yürütür.

Owner thread `begin()` ile önce eski frame/nokta/plaka/hesabı temizler, immutable
kamera/kurulum snapshot'ı, session source ID, generation, cancel ve monoton
deadline oluşturur. Caller `run(job)` işini worker'da yürütür. Worker session veya
widget referansı tutmaz. `accept()` yalnız kendi tek kullanımlık ticket/generation
sonucunu alır; Stop, yeni hedef veya yeni frame eski sonucu reddeder. Worker
çalışırken session yok edilse de job kendi görüntü/request/cancel değerlerine
sahiptir. OpenCV/decode zorla kesilmez; iptal/gecikme sonrası frame yayımlanmaz.

Üç köken vardır: `camera_acquired`, hedefi açık beyan edilmiş `setup_file`, hedefi
olmayan `trial_file`. Legacy record edinim isteği değildir. Dosya path/raster hash
kanıtı SetupId/revision ve cihaz eşlemesinden ayrıdır. İki dosya aynı kurulumda
farklı kanıt/generation üretir; fiziksel setup/config kimliği değişmez. Dosya
source ID'si `calibration-test:calibration-session:…`, kamera source ID'si
`calibration-session:…` olur; bunlar gerçek session kaynaklarıdır, reçete tool'u
üretilmez. Mevcut math record'un sourceToolId adı tarihsel olarak korunur ve
session kimliğini taşır. File→acquired relabel mevcut math kapısında da reddedilir.

Kamera yolu exact DeviceBinding ile güncel keşif listesini eşler; offline,
duplicate, unavailable veya farklı capabilities reddir. Qt adapter metadata
keşfi kamerayı açmaz. Yalnız açık worker edinimi QCamera açar, provider ID ile
cihazı ve ilan edilmiş çözünürlüğe sahip formatı seçer; actual frame boyutu ayrıca
doğrulanır. Yeni kimlik yolu index fallback yapmaz. Eski WebcamSettings'in boş
providerId ile explicit index/backend API'si ve eski recipe codec/hash yolu sürer.
Provider ID yeni domain DeviceBinding codec'inde saklanır; legacy tool schema'sı
değiştirilmez.

[Qt QCameraDevice](https://doc.qt.io/qt-6/qcameradevice.html) id alanını kamera
kimliği olarak tanımlar; [QMediaDevices](https://doc.qt.io/qt-6/qmediadevices.html)
videoInputs ile keşif listesi sunar. Burada ID hex olarak lossless taşınır ve güven
`ProviderReported` kalır. Seri numarası veya yeniden kurulum/port/driver değişimi
boyunca kalıcılık garantisi çıkarılmaz. Yeni bağımsız kamera yolu yalnız Qt webcam
ve complete raw resolution destekler; sensor ROI/binning/GenTL capture reddedilir.
Şimdilik setup_file da bu desteklenen hedef sözleşmesini kullanır.

Kabul edilen frame mevcut `CalibrationWizardSession::useFrame` üzerinden
Edge/Caliper/chessboard/circle matematiğine girer. Trial hesabı üretim izni değildir.
Bu session kayıt, etkinleştirme veya recipe edit yapmaz; setup_file beyanı fiziksel
kurulum doğrulaması değildir. KMC-4 gerçek association/uyum ve aktivasyon kapısını
ekleyecek; KMC-6 UI dosya ilişkilendirmesini bağlayacaktır.

Testler gerçek PNG decode, gerçek Edge/Caliper/H hesabı ve worker cancellation
kullanır. Kamera testleri açık fixture adapter'ıdır; fiziksel cihaz kabulü yoktur.
