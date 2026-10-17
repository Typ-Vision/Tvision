# ICM — hedef mimari ve uyumluluk sözleşmeleri

**Planlanan davranış.** [Ana plan](industrial-camera-plan.md) görevleri uygulanmadan
bu sözleşmeler mevcut ürün özelliği sayılmaz. Sınıf adları öneridir; mevcut servisleri
genişletmek yeterliyse ikinci registry/controller oluşturulmaz.

## 1. Katmanlar ve backend kararı

`Qt UI → application camera service/session → acquisition adapter → hardware backend`.
Domain codec'leri SDK/GenApi sınıfları içermez. SDK handles/buffer/callback sahibi
tek worker'dır; UI immutable durum/frame kopyası alır. `AcquisitionService`,
`CameraLifecycle`, `ImageFrame` ve scoped calibration servisleri yeniden kullanılır.

Tercih sırası:

1. OECS U3V GenTL C ABI + MinGW ile doğrulanmış GenApi/nodemap consumer.
2. Birinci yolun feature/ABI/dağıtım engeli varsa ortak application sözleşmesi altında
   **OECS C API**. ITALA C API'nin COE-U için uygun olduğu varsayılmaz.
3. İki yol da başarısızsa dar C ABI köprüsü/ayrı süreç veya toolchain geçişi için
   ayrı gerekçeli karar. İlk çözüm olarak Qt/OpenCV tümden MSVC'ye taşınmaz.

ICM-02 tek seçilmiş acquisition backend'i belgeler; iki ayrı tam stack birden yazılmaz.
Backend C API seçilirse discovery/frame erişiminin vendor tutarlılığı açıklanır;
GenTL handle'ı vendor C handle'ına cast edilmez. Generic discovery kullanılabilir,
fakat actual open kimliği aynı fiziksel cihazı ayrıca doğrular.

GenTL dalı GC/TL/IF/Dev/DS/event/buffer ömrü ve remote device port/XML nodemap erişimini
birlikte çözer. Nodemap XML'i elle register adreslerine dönüştürmek veya sabit model
adresleri kullanmak çözüm değildir. Feature isimleri SFNC ve açık vendor alias'ları
ile çözülür; float/int farkı ve birim dönüşümü sınanır. SDK yokluğu normal startup'ı
ve image_file/webcam testlerini bozmaz; SDK dependent testler açık opt-in olur.

## 2. Cihaz kimliği, transport ve seçim

| Değer | Sahiplik / anlam |
| --- | --- |
| CameraId | Projedeki kullanıcı kaydı; yeniden adlandırma ile değişmez |
| PhysicalDeviceIdentity | Gerçek vendor/model/serial ve kimlik güven düzeyi; boş/belirsiz kimlik kullanılabilir sayılmaz |
| TransportLocator | Yerel canonical producer yolu, interface/device ID; cihazı açma rotası, kalıcı kamera kimliği değil |
| BackendIdentity | Adapter sürümü, SDK/producer sürümü/hash ve capability kanıtı; binary değişiminde yeniden preparation |
| SetupId/revision | Fiziksel optik/düzlem/sensor geometry kurulumu; cihaz binding revizyonunu taşır |
| AcquisitionProfile | Kamera ayarlarının versioned requested/applied değerleri; kullanıcı reçetesi bunu snapshot'a alır |

Seri tek başına evrensel benzersiz varsayılmaz; vendor/model ile eşleşme ve birden fazla
actual aday kontrolü gerekir. İki producer aynı cihazı gösterirse tek CameraId altında
rotalar sunulur; iki fiziksel kaynak oluşturulmaz. ID=1, görüntü adı veya USB sıra
numarası otomatik kalıcı seri yapılmaz. Kaybolan cihaz yerine sıradaki cihaz seçilmez.
Yeni eşleme explicit revision değişimidir; eski model sessiz taşınmaz.

Projeye producer mutlak yolu zorunlu fiziksel bağımlılık olarak gömülmez. Yerel rota
konfigürasyonu ile taşınabilir device/profile ayrı saklanır. Binary fingerprint,
yerel yol ve fiziksel kimlik farklı alanlar olur; aynı binary başka dizine taşınınca
optik kalibrasyon bozulmaz fakat transport yeniden çözülüp Runtime hazırlanır.

**ICM-03a uygulanan değer tipleri** (`domain/MeasurementSetupContract`): `PhysicalDeviceIdentity1`
vendor/model/serial + güven; yalnız `serial_reported` (üçü dolu) kullanılabilir, seri tek başına
eşleşmez. `TransportLocator1` (`oecs` producer'sız / `gentl` canonical mutlak producer) ve
`BackendIdentity1` (adapter sürümü, runtime sürümü, küçük harf SHA-256; yol içermez) yerel
keşif değerleridir, proje JSON'una girmez. `resolveRoutes` aynı kimliği farklı producer'larda
tek cihazın rotaları sayar; aynı backend+producer içinde tekrar belirsizdir; boş/yok ise ret.
Endüstriyel kamera `MeasurementCamera2` ile yazılır: `physical` zorunlu, `device` inert
`gentl`/unavailable binding'dir ve revision'ı taşır; softwareCapture kapalı kalır.
`physical` yoksa kamera bayt bayt schema 1'dir. `MeasurementCamera::rebound` ad değişiminde
revision'ı korur, device/physical değişiminde artırır; environment commit bunu zorunlu kılar.
Aynı kullanılabilir kimlik iki CameraId'ye bağlanamaz. Yerel rota saklama ve keşif ICM-05a'dadır.

## 3. Acquisition profile ve gerçek geometri

Versioned profile en az pixel format, width/height/offset, binning/decimation/reverse
durumu, trigger mode/source/selector, exposure/gain ve auto durumlarını taşır.
Desteklenmeyen optional node, kapalı olduğu kanıtlanan özellik ve okunamayan kritik
geometri farklı durumlardır. Node yokluğundan bilinmeyen geometriye 0/1 uydurulmaz;
standard/model kanıtıyla uygulanamaz kabul edilebilir, aksi durumda capture reddedilir.

İlk ürün profili Mono8, tam sensör, offset 0, geometrik küçültme/ayna yok; yalnız
donanımın doğrulanan manual exposure/gain aralıkları. Aralık/min/max/increment ve
writable access state kontrol edilir; sessiz clamp yok. Requested ve actual readback
ayrı tutulur; kanıtlanan temsil toleransı dışında farklılık configure hatasıdır.
Timeout, exposure+transfer bütçesiyle tutarlı olmalı; uzun exposure hard kesilebilir sayılmaz.

Configure idle/exclusive lease gerektirir: eski ayarları oku → sırayla uygula →
tam readback → profile revision/hash yayınla. Kısmi yazımda eski ayarlar mümkünse
geri yüklenir ve readback edilir; rollback doğrulanamazsa Fault, stream başlatılmaz.
EEPROM/UserSetSave yapılmaz; geçici çalışma ayarları varsayılandır.

| Değişim | Kalibrasyon/sonuç davranışı |
| --- | --- |
| Kamera/lens/odak/çalışma düzlemi | Yeni setup/device revision; eski model uyumu reddedilir |
| Offset/ROI/binning/decimation/reverse/resolution | Geometri değişir; aynı raster boyutunda dahi eski calibration/cache/Runtime kabul edilmez |
| Pixel format/bit depth | Acquisition/cache/Runtime değişir; yalnız eşdeğer geometri kanıtıyla model korunabilir; ilk sürüm Mono8 dışını reddeder |
| Exposure/gain/auto/ışık beyanı | Acquisition/cache/Runtime invalidation; mm dönüşümü otomatik yeniden fit edilmez, ölçüm kalite kontrolü gerekir |
| Producer/SDK sürümü | Backend capability ve preparation yeniden kontrol edilir; optik model sessiz değiştirilmez |
| UI adı/panel düzeni | Geometri/profile hash değişmez |

**ICM-03b uygulanan değer tipleri** (`domain/MeasurementSetupContract`): geometri setup'ındır;
`GeometryDescriptor2` endüstriyel setup'ta offset (sensorRoi), binning, decimation ve reverse'ü
birlikte açık yazar, yoksa schema 1 bayt/hash aynıdır; webcam setup'ı schema 2 alamaz, endüstriyel
complete setup schema 1 kalamaz. `AcquisitionProfile1` (ProfileId, CameraId, SetupId, ad, revision,
`requested` = `AcquisitionSettings1`: pixel format, trigger enabled/selector/source, exposure/gain
auto+değer, ışık beyanı) ilk sürümde yalnız Mono8 ve manual exposure/gain kabul eder; contract
`profiles` boş değilse schema 2'dir. `acquisitionId` = geometryId + requested; ad/ID/revision dışarıda.
Tablo tek kuraldır: `acquisitionChange` → setup/geometry/pixelFormat/acquisition/backend satırları;
Setup `revised` alan farklarından revision üretir; çağıranın revision değerini kullanmaz.
Uyumluluk ayrıca setup revision farkını reddeder: A→B→A, eski etiketlere dönse de
eski calibration/cache/preparation ile uyumlu değildir. Rename/no-op revision üretmez.
Profile `revised` ve `verifyApplied` ilgili tablo satırlarını kullanır. Applied readback
(`AcquisitionReadback1`, özellik başına reported/not_applicable/unknown) projeye yazılmaz; unknown
kritik geometri reddedilir; float temsil toleransı ICM-04b'dedir. Environment commit profile
revision geçişini zorunlu kılar, silme reddedilir; Save As profili rota/readback/izin olmadan taşır.

Externally changed settings begin öncesi ve actual capture sınırında kontrol edilir.
Gerekli exclusive erişim/ayar kararlılığı kanıtlanamıyorsa kalibre production reddedilir.
Readback ön/son tek başına tüm kamera içi yarışların fiziksel kanıtı gibi sunulmaz.

## 4. Frame sözleşmesi

SDK buffer serbest bırakılmadan önce `ImageFrame::fromBuffer` owned kopya tamamlanır.
Payload status, width/height, PFNC format, stride/padding, received bytes ve overflow
kontrol edilir. Chunk bytes image bytes sayılmaz. Eksik buffer/unsupported packed/Bayer
formatı renk/8-bit gibi yeniden yorumlanmaz. Mono8 sensor raster ve preview ayrıdır.

Application sourceId/frameId/cycleId her gerçek admission'da üretilir; bunlar cihazın
hardware frame counter'ı değildir. Hardware block/frame ID, ticks ve varsa clock
frekansı/reset generation ayrı kanıt alanlarıdır. UTC completion exposure zamanı
değildir; kamera tick'i epoch kanıtı yokken UTC'ye çevrilmez. İçerik hash'inin aynı
olması eski frame kanıtı değildir: durağan sahne eşit pikseller verebilir.

Production software trigger: preview durdur/flush → SDK hazır durumunu doğrula →
tek in-flight trigger → ilgili tamamlanmış buffer → owned copy. Flush sonrası geç
gelen eski buffer ve counter wrap/reset için stream generation/admission kanıtı
tasarlanır. Tetikle ilişki kurulamıyorsa sıradaki frame'i çevrime atamak yerine ERROR.
Preview frame'i üretim için tüketilmez; external frame tetiklemesi bu sürümde reddedilir.

Stream buffer pool ve retained ImageFrame bütçeleri ayrı ölçülür. Payload aritmetiği
allocation öncesi checked olur; cihaz başına buffer sayısı ve toplam byte limiti
başlangıç konfigürasyonunda sabitlenir. Mevcut 512 MiB retained/30000 ms cycle sınırı
SDK buffer/RSS hard sınırı olarak sunulmaz. SDK pool varsayılanı 4 buffer, uygulama
kuyruğu 2 frame önerisidir; device minimumu ve byte bütçesiyle doğrulanıp kayda alınır.

## 5. Session, lease ve failure

Bir fiziksel cihazın bir owner session'ı bulunur. Calibration, Editor preview ve
Runtime lease yarışır; busy açık ret verir. UI thread SDK beklemez. Producer
GCInitLib/GCCloseLib ömrü bütün device/stream/buffer'ları kapsar. Mevcut process-geneli
kilit ownership kanıtı olmadan kaldırılmaz; ilk fiziksel tek-camera kapsamıyla uyumludur.

Cooperative cancel/deadline mevcut semantics'i korur. Stop generation artırır,
yeni admission kapatır ve geç sonucu bastırır; pending worker sırasında DLL unload
veya ikinci SDK close yapılmaz. Callback varsa in-flight callback bitmeden userdata
silinmez. Cleanup sırası backend dokümanıyla doğrulanır, her ara hata yolunda sınanır.
Unplug Fault ve capability/unavailable üretir; cached PASS/mm ekranda kalmaz.
Replug history/seçimi korur fakat açık reconnect/reconfigure/activation/reprepare ister.

İlk sürüm process-local lease'dir; başka uygulama kamera sahibiyse busy/access denied
tanısı verilir. Cross-process üretim lease ve kesilemeyen SDK için process isolation
ayrı kapsam. Stop bütçesi aşıldı mesajı kaynakların serbest kaldığı anlamına gelmez.

**ICM-04a uygulanan ömür** (`hardware/OecsSession`, `hardware/OecsCApi`): SDK'sız `OecsCApi`
sınırı; gerçek binding yalnız `VISIONFLOW_OECS_INCLUDE_DIR` ile derlenir ve DLL'i dinamik yükler.
Oturum sahibi kuran iş parçacığıdır; yabancı iş parçacığı SDK çağıramaz. Açılış sırası load →
USB enumerate → exact vendor+model+seri → süreç içi sahiplik → create → exclusive open.
SDK enumerate ve tam `MV_CC_DEVICE_INFO` kopyası süreç genelinde tek kilit altında yapılır;
create, instance'ın sahip olduğu kopyayı kullanır. Yeni SDK enumeration yolları da aynı
kilidi kullanmalıdır; SDK'ya ait geçici adresler enumerate sonrasında saklanmaz. Hata
tamamlanan aşamaları ters sırada geri alır. Close hatasında da destroy yapılır; destroy veya unload
başarısızsa DLL kaldırılmaz, oturum Faulted olur ve yeniden açılmaz. `MV_E_BUSY`/`MV_E_ACCESS_DENIED`
ve süreç içi ikinci sahip Busy tanısıdır. GenTL süreç kilidi değişmedi; gerçek cihaz open kanıtı yok.

## 6. Kalibrasyon, recipe ve Runtime

IndependentCalibrationSession hem webcam hem industrial adapter factory kullanır;
trial_file/setup_file/camera_acquired ayrımı korunur. Setup-file kanıtı actual kamera
karesi gibi etiketlenmez. Aktivasyon model-ready, fiziksel availability ve bağımsız
ölçüm kabulünü ayrı tutar. Raw homography lens düzeltmez.

Yeni industrial source tool kind'i [ICM-00 envanteriyle](industrial-camera-baseline.md)
`industrial_camera` olarak sabitlendi; henüz kayıtlı değildir. Legacy webcam tool'u industrial gibi etiketlenmez. Explicit tool
ekleme/binding profile değişikliği dışında eski codec/hash/no-op Save değişmez.
Kamera tek sefer kaydedilir; recipe source ilgili CameraId/setup/profile sürümüne bağlanır.

Editor/Runtime aynı acquisition provider'ı kullanır. TargetOnly aynı doğru frame'de
acquisition0'dır; profile/model değişimi ilgili source cache'ini invalid eder.
Runtime Run preparation'dır, üretim trigger'ı veya sonuç üretmez; gerekirse cihazı
kontrol/configure etmek UI'da hazırlık olarak gösterilir. Begin tam required source
seti ve current profile/device/geometry kanıtını yeniden doğrular. Actual frame de
aynı snapshot ile eşleşir; bir kaynak hatasında global PASS üretilmez.

Completion güveni private worker/lease sınırından gelir; caller-supplied JSON/hardware
metadata trusted sayılmaz. Gerekli acquisition evidence digest'e dahil edilir.
Terminal okuyucu geçmişi canlı registry/SDK olmadan açar; profile/device/backend ve
actual frame kanıtı arşivlenir. Record retry capture/trigger/provider tekrarlamaz.
Restart geçmişi yükler, üretim izni veya otomatik stream geri yüklemez.

## 7. Şema ve dağıtım

ICM-00 mevcut sayıları koddan çıkarır: DeviceBinding/GeometryDescriptor, registry,
tool/editor/engine, active calibration scope, production snapshot/terminal codec.
Plan bugünden yeni sürüm numarası ayırmaz. Yeni alanlı yazım açık yeni schema ile;
eski okuyucu yolu/hash içeriği korunur. Özellikle terminal7 replay ve camera scope
exact-field kontrolleri yeni metadata için sessiz gevşetilmez.

Save As/dependency export/import yerel SDK binary'sini paketlemez. Yeni projede
device/profile korunur, producer rota eşleme ve gerçek kabul yeniden gerekir.
Sdk runtime/driver redistributable lisansı ICM-02'de kayda alınır; DLL mevcutluğu
temiz makine kurulumu kabulü değildir.
