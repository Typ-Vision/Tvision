# Gerçek endüstriyel kamera entegrasyonu değerlendirmesi

17 Ekim 2026. Kullanıcı bağlı Opto Engineering kamera ve Basler 25 mm lens bildirdi;
kalibrasyonun fiziksel kabulünden önce kamera entegrasyonunu değerlendirmemizi istedi.
Bu belge kod incelemesi ve öneridir; SDK kurulumu, cihaz keşfi veya grab kabulü değildir.
İlk incelemede model bilinmiyordu; aşağıdaki aynı gün ek bilgisi bu belgedeki koşullu ITALA önerisinin önüne geçer.

## Aynı gün model ve OECS doğrulaması

- Kullanıcı model kodunu iki kez birleşmiş olarak bildirdi; normalize model
  `COE-050-M-USB-050-IR-C`, görünen Device ID `1`. Seri numarası henüz doğrulanmadı;
  `1` kalıcı fiziksel kimlik kabul edilmez.
- Resmî [model datasheet'i](https://www.opto-e.com/discontinued/COE-U%20series/Datasheets/COE-050-M-USB-050-IR-C_EN_v02.pdf)
  COE-U/USB3 Vision, mono Sony IMX264, global shutter, 2448×2048, 35 fps,
  Mono8/10/12 ve packed formatlar bildirir. Model adındaki 050 gerçek 50 fps kabulü değildir.
- Kullanıcı OECS v1.2 ve ITALA View indirdiğini, kurulumun reboot istediğini fakat
  reboot yapmadığını bildirdi. Bu model için başlangıç OECS/USB3 yoludur; ITALA'nın
  GigE SDK'sı bu COE-U cihazın desteği olarak kabul edilmez.
- Yerel yeni envanterde `C:/Program Files (x86)/OECS` ve
  `C:/Program Files/Opto Engineering/Itala SDK` mevcut. OECS x64 USB producer:
  `C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvProducerU3V.cti`
  (dosya sürümü 1.0.0.5). Aynı klasörde GEV producer da var; bu kamera için U3V seçilir.
  Machine GENICAM_GENTL64_PATH iki SDK yolunu içeriyor. Önceki boş envanter güncel değildir.
- Mevcut Release probe ile bu explicit producer, 5000 ms toplam bütçe/250 ms scan
  ve `--open-check` çalıştırıldı: **DONE, errors boş, 0 cihaz**. Cihaz bulunmadığından
  gerçek open/close sınanmadı; grab zaten uygulanmış değil. Kanıt:
  `build/opto-coe-u-pre-reboot-probe.json`. Reboot eksikliği sıfır cihazın kanıtlanmış
  nedeni değildir; USB bağlantısı/sürücü/viewer durumu ayrıca doğrulanmalı.
- Sıradaki fiziksel adım kullanıcı kontrollü reboot sonrası OECS'de cihaz/seri ve
  gerçek Mono8 görüntü; sonra aynı producer probe'u. Otomatik reboot, driver/firmware
  değişimi veya VisionFlow kaynak değişikliği yapılmadı.

## Mevcut uygulama

- KMC-10e yazılım kabulü tamamlandı: çok kaynaklı Editor/Runtime, kaynak başına model,
  immutable completion ve terminal kanıtı var. Fiziksel endüstriyel çoklu kamera kabulü yok.
- `hardware/GenTLTransport` yalnız discovery ve read-only open/close sunar. Stream,
  buffer/event, feature nodemap, trigger, exposure/gain ve görüntü edinimi uygulanmadı.
- `AcquisitionService`, owned ImageFrame ve `CameraLifecycle` deadline/cancel/generation,
  preview/FIFO ayrımı için kullanılabilir çekirdek sağlar; lifecycle gerçek SDK stream'ine bağlı değil.
- `MeasurementWorkspace` varsayılan keşfi Qt webcam'dir. Camera Manager GenTL listesiyle
  kalibrasyonun kamera ekleme yolu henüz ortak bir keşif/cihaz servisi değildir.
- `DeviceBinding::validate` GenTL softwareCapture'ı açıkça reddeder.
  `IndependentCaptureRequest`, actual adapter doğrulaması, `SystemCalibration::source`,
  `ExecutionService` ve `ProductionRecipeService` yollarında webcam/image_file sınırları var.
  Sadece `.cti` seçmek veya bir whitelist'i kaldırmak uçtan uca entegrasyon sağlamaz.
- GeometryDescriptor çözünürlük/ROI/binning alanlarını taşır; mevcut edinim ROI/binning'i
  reddeder. Decimation/reverse ve ayrıntılı acquisition profile sözleşmesi genişletilmeli.

Kod dayanakları: `src/hardware/GenTLTransport.hpp/.cpp`,
`src/domain/MeasurementSetupContract.hpp/.cpp`, `src/application/MeasurementWorkspace.hpp`,
`IndependentCalibrationCapture.cpp`, `ExecutionService.cpp`, `SystemCalibration.cpp`,
`ProductionRecipeService.cpp` ve `MeasurementService.cpp`.

## Önerilen yön

Ürün sınırı marka bağımsız acquisition/device/capability sözleşmesi olarak kalmalı.
Model artık COE-U USB3 olarak biliniyor: başlangıç OECS ve U3V producer'dır.
Önceki model bilinmeyen ITALA varsayımı bu cihaza uygulanmaz. GenICam ile vendor SDK
birbirinin alternatifi değildir. GenTL keşif/transport, GenApi/XML nodemap özellik erişimi,
SFNC ortak feature isimleri için kullanılır. `.cti` keşfi tek başına exposure/trigger
ayarlarını ve stream tüketimini uygulamaz.

GenTL stream + feature erişimi için kısa bir teknik doğrulama yapılmalı. Mevcut MinGW x64
ile vendor MSVC C++ kütüphanelerinin doğrudan uyumlu olduğu varsayılmamalı. GenTL C ABI
ve gerekli GenApi çözümü doğrulanmalı; gerekirse yerel OECS C API aynı ortak adapter arkasında
ilk çalışan backend olabilir. Bu seçim UI, kalibrasyon matematiği veya recipe modelini
Opto sınıflarına bağlamamalı. C API varlığı yerel ABI/dağıtım kabulü değildir.

Kamera fiziksel kimliği (vendor/model/seri ve doğrulanmış ayırt edici kimlik) ile yerel
producer yolu/IP ayrı tutulmalı. Aynı cihazın farklı producer'lardan görünmesi iki kamera
sayılmamalı; belirsiz eşleşme reddedilmeli. CameraId → fiziksel binding → SetupId/revision
→ calibration model ilişkisi korunmalı. Kalibrasyon, preview ve Runtime cihaz sahipliği
tek application servisiyle yönetilmeli; eşzamanlı farklı ayar yazımı engellenmeli.

## Kalibrasyonla kesişimler

- Mevcut uniform ölçek/raw planar homography ve kaynak başına immutable model yeniden
  kullanılabilir. Endüstriyel adapter hem bağımsız sihirbaz hem Editor hem Runtime'a bağlanmalı.
- Yalnız frame genişlik/yüksekliği yeterli değil: sensor offset/ROI, binning/decimation,
  reverse ve düzeltme koordinatları aynı raster boyutunda bile değişebilir. İlk aşamada
  tam sensör ve sabit desteklenen pixel format sınırıyla başlanıp gerçek değerler okunmalı.
- Exposure/gain/ışık değişimi geometrik kalibrasyonu otomatik olarak bozmak zorunda değil;
  acquisition/cache/recipe kanıtını değiştirmeli ve ölçüm tekrarlanabilirliği yeniden kontrol edilmeli.
  Lens, odak, kamera/düzlem konumu veya sensör geometrisi değişimi kurulum/model uyumu ister.
- Mevcut homography `raw_assumed_negligible` lens politikası taşır; intrinsic/distortion
  kalibrasyonu yok. Basler 25 mm bilgisi tek başına distorsiyon veya mm/px belirlemez.
  Basler lens markası kamera için pylon gerektirmez; özel lens kontrol donanımı varsa ayrıca ele alınır.
- İlk grab lens kalibrasyonunu beklemez. Ölçüm kabulünde görüş alanının farklı bölgeleri
  ve bağımsız referans sınanmalı; hata gerektirirse T15 lens işi öne çekilmeli. İleride
  raw ve undistorted görüntü/ROI/model koordinatları açıkça ayrılmalı.
- N-source sıralı execution senkron capture değildir. PTP/hardware trigger/ortak dünya
  koordinatları ayrı capability ve kabul işidir.

## Önerilen uygulama sırası

1. Tam model/transport/seri/lens ve SDK envanteri; uygun vendor viewer'da gerçek kare.
2. Mevcut probe ile x64 producer discovery/read-only açma; ABI ve feature erişimi denemesi.
3. Tek kamera ortak adapter: güvenilir owned frame, gerçek pixel format/stride/payload,
   timeout/Stop/disconnect/cleanup, eski veya incomplete frame reddi; sonra preview/software trigger.
4. Ortak kamera kayıt/keşif servisi ve acquisition profile; bağımsız kalibrasyon,
   Editor source, immutable Runtime, terminal/replay kapılarının birlikte entegrasyonu.
5. Gerçek kamera ile tekrar açma/kopma/yanlış cihaz/geometri değişimi ve çevrim testleri;
   ardından bağımsız fiziksel kalibrasyon kabulü. Multi-device/senkronizasyon ayrıca genişletilir.

## İlk incelemenin tarihsel doğrulama sınırları

İlk kontrol anında Program Files üst düzey ilgili klasörleri, HKLM standart 32/64-bit uninstall kayıtları
ve process/machine GENICAM_GENTL64_PATH kontrolünde Opto/ITALA/Basler/Galaxy girdisi
görülmedi. Bu bilgi yukarıdaki sonraki OECS/ITALA envanteriyle güncellendi;
güncel kurulum yokluğu iddiası değildir.
Eski başka çalışma ortamına ait SDK kurulum kaydı bu makinede kabul sayılmadı.
Kamera açılmadı, ağ/driver/firmware ayarı değiştirilmedi. Kaynak/UI değişmedi;
Release/CTest yeniden çalıştırılmadı. Önceki 87/87 ve 73/73 kabulü progress'teki tarihsel kanıttır.

Resmî kaynaklar (17 Ekim 2026 incelemesi):

- [EMVA GenICam](https://www.emva.org/standards-technology/genicam/)
- [Opto ITALA G / SDK, View ve GenTL producer](https://www.opto-e.com/en/products/itala-g-series)
- [ITALA SDK mimarisi](https://itala-doc.opto-e.com/pages/programmers_guide.html)
- [Nodemap ve SFNC](https://itala-doc.opto-e.com/pages/programmers_guide/basic_topics/device_configuration.html)
- [ITALA C API](https://itala-doc.opto-e.com/APIC/index.html)

## Uygulama devri

Kullanıcının plan talebiyle [ICM ana planı](industrial-camera-plan.md), hedef
sözleşmeler, 24 görev kartı, kabul matrisi ve GPT-6.1-sol / medium devir dosyaları
hazırlandı. İlk uygulama görevi ICM-00; bu değerlendirme veya planlar ürün grab'inin
uygulandığı anlamına gelmez. ITALA kaynakları yalnız önceki karşılaştırma bağlamıdır;
COE-U backend kanıtı yerel OECS ve model belgesinden alınır.
