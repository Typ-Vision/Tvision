# ICM — hedef mimari ve uyumluluk sözleşmeleri

**Planlanan davranış.** [Ana plan](industrial-camera-plan.md) görevleri uygulanmadan
bu sözleşmeler mevcut ürün özelliği sayılmaz. Sınıf adları öneridir; mevcut servisleri
genişletmek yeterliyse ikinci registry/controller oluşturulmaz.

21 Ekim plan eki: [OECS deneysel politika sözleşmesi](industrial-camera-oecs-experimental-plan.md)
G1 beklemeden gerçek edinim/Editor/Runtime geliştirmesini tanımlar. Aşağıdaki
doğrulanmış production/trigger şartları korunur; deneysel sonuç bu kanıta
yükseltilmez. Yeni politika, provenance ve kayıt ayrımı henüz uygulanmadı.

## 1. Katmanlar ve backend kararı

`Qt UI → application camera service/session → acquisition adapter → hardware backend`.
Domain codec'leri SDK/GenApi sınıfları içermez. SDK handles/buffer/callback sahibi
tek worker'dır; UI immutable durum/frame kopyası alır. `AcquisitionService`,
`CameraLifecycle`, `ImageFrame` ve scoped calibration servisleri yeniden kullanılır.

18 Ekim kapsam kararı: bu ortak yol OECS, Basler ve Daheng'den bağımsızdır.
Ortak yazılım 04f test adaptörü üzerinden geliştirilir. Test adaptörü yalnız test
composition'ından enjekte edilir; kullanıcı config'i, recipe, producer yolu veya
caller metadata ile ürün backend'i olarak seçilemez. Sentetik kimlik/frame ve
test sonuçları fiziksel acquisition/üretim kanıtına dönüştürülemez; normal ürün
factory'si bunları reddeder. UI görsel fixture'ları açıkça sentetik etiketlenir.
04f: `CameraProviderFactory` normal composition'da edinimi reddeder; `forTests`
yalnız in-memory provider enjeksiyonudur. Test backend'i test executable'ında
derlenir; ürün kütüphanesinde fixture yoktur. Config/recipe/producer provider yükleyemez.
`IndustrialCameraRequest` camera/setup/profile/device/source snapshot'ını doğrular;
worker actual identity, pre/post readback ve frame format/boyutunu denetler.
Preview ayrı trigger-off profile/session ister; lease/Stop mevcut worker'dır.
Provider sonuçları terminal completion veya production izni değildir.

ICM-07b: `ExecutionService(IndustrialExecutionInputs)` kamera/setup/profile contract'ını
değer kopyası olarak dondurur; yerel rotalar ve provider yalnız application composition'dan
gelir. Her edinim bütçesi içinde fresh keşif/exact resolve, ortak Editor lease ve worker
Connect/Configure/Start/Grab yolunu kullanır. Recipe JSON'u backend seçemez; generic
source-adapter enjeksiyonu ve frameSnapshot industrial edinimi atlayamaz. Actual
kimlik/readback, owned raw raster/hash ve backend frame/cycle kimlikleri ayrı
`IndustrialExecutionEvidence` olarak korunur; execution frame/cycle eşlemesi bunların
yerine fiziksel kanıt yazmaz. Bu geçici Editor kanıtı terminal codec'ine veya private
completion authority'ye girmez. Stop/geç sonuç çıktı ve kanıtı bastırır. Aynı fiziksel
kamera iki enabled source'ta source refresh sırasında da reddedilir. TargetOnly
edinim yapmaz; cached raw ile aynı kaynak/frame/cycle/piksel içeriği gerekir.
Scoped industrial model eşlemesi scope2 fiziksel kimlik/device/setup revision ister.
ICM-07c kayıtlı profil seçimini açık bağlama/güncelleme ve immutable worker girdisine bağlar.
Liste seçimi edinim yapmaz; profil değişimi açık rebind ister. Üç applicationType
ortak Editor/Viewer/ROI yolundadır. Runtime admission 08a hazır; terminal 08b kapsamındadır.

ICM-05b: CameraManager ortak envanter/registry ve process-local lease üzerinden
çalışır. Configure-only factory oturumu tetik capability'si istemez, Start/Grab'ı
reddeder. Actual readback tek kez okunup doğrulanan snapshot olarak owner'a döner.
Preview trigger-off ayrı session'dır; Stop rasteri anında temizler, worker cleanup
bitene kadar ortak CameraSession işi busy kalır. UI SDK çağırmaz. Profil taslağı
istenen tam sensör geometri/manual Mono8 ayarlarıdır; gerçek readback veya aktif
kalibrasyon değildir. Normal composition'da native ayar/preview kontrolleri kapalıdır.
Kalibrasyon sistem özeti aynı CameraId'yi gösterir; keşif ortak Manager'a yönlenir.

Discovery/open, preview ve doğrulanmış tetik edinimi ayrı yeteneklerdir. Gerçek
production capability hem ilgili backend G1 kabulünü hem 08b güven zincirini ister.
Bir cihazın tetik engeli diğer backend'leri veya ortak yazılım G2 kabulünü kapatmaz.
G3 e2e ve G4 ölçüm kabulü ayrıca kaydedilir; preview karesi çevrim karesi sayılmaz.

OECS için seçilmiş kararın tercih sırası (Basler/Daheng seçimi değildir):

1. OECS U3V GenTL C ABI + MinGW ile doğrulanmış GenApi/nodemap consumer.
2. Birinci yolun feature/ABI/dağıtım engeli varsa ortak application sözleşmesi altında
   **OECS C API**. ITALA C API'nin COE-U için uygun olduğu varsayılmaz.
3. İki yol da başarısızsa dar C ABI köprüsü/ayrı süreç veya toolchain geçişi için
   ayrı gerekçeli karar. İlk çözüm olarak Qt/OpenCV tümden MSVC'ye taşınmaz.

ICM-02 OECS için tek seçilmiş acquisition backend'ini belgeler; aynı cihaz için
iki ayrı tam stack birden yazılmaz. Basler/Daheng SDK/ABI kararları
[BAS/DAH-01](industrial-camera-tasks-backends.md) içinde ayrı verilir. Marka adaptörleri
aynı servis/sözleşmeye bağlanır; ikinci Editor/Runtime/kalibrasyon mimarisi kurulmaz.
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

**ICM-05a-a yazılım hazırlığı:** `CameraService::groupIndustrial/resolveIndustrial`
caller-supplied snapshot'ı doğrular. Kullanılabilir exact kimlikler tek grupta,
serisiz kayıtlar ayrı kalır; aynı enumeration içindeki tekrar ambiguous'dur.
Birden çok rotada explicit local route gerekir; rota kaybolursa fallback yoktur.
Cancelled/error içeren kısmi tarama resolve edilemez. Sonuç bir keşif değeridir;
actual open doğrulaması, lease, capability veya trusted completion değildir.
ICM-05a: CameraService::inventory GenTL metadata + Qt webcam keşfini ortak
CameraInventory/CameraDiscoveryEntry içinde sunar. Endüstriyel rotalarda canonical
producer yolu ve dosya SHA-256 vardır; hash SDK/G1 kabulü değildir. Kısmi tarama
kayıt/resolve vermez. MeasurementWorkspace aynı environment registry'sine kayıt
komutu uygular; duplicate physical identity reddedilir. CameraLocalConfiguration
producer ve exact kimlik→rota tercihini yalnız yerel INI'de tutar. Export/proje
bu rotayı içermez. Factory resolveOpen fresh snapshot ile exact resolve yapıp
aynı lease'i alır; webcam dalı mevcut WebcamAdapter'ı üretir. Native edinim kapalıdır.

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

ICM-04b uygulaması: `IndustrialCameraConfiguration` yalnız owner worker'daki açık,
exclusive ve idle `OecsSession` üzerinde çalışır; projeyi değiştirmez veya etkinleştirmez.
SFNC envanterinde access/type, numeric range/increment, XML unit/enum sembolleri okunur.
ExposureTime integer veya float olabilir; mikro-saniye (`us`/`µs`) ve Gain `dB`
kanıtı gerekir. Float karşılaştırması yalnız tam değer veya `double(float(requested))`
kabul eder; komşu float değeri tolerans sayılmaz. Actual raporda yuvarlanmış değer
korunur, profile kimliği requested üzerinden kalır; integer yazımlar exact'tir.
Kritik geometri node'larının tamamı readable olmalıdır; eksik node için model kanıtı
henüz tanımlanmadığından NotApplicable üretilmez. Aktif TriggerSelector FrameStart
olmalıdır; farklı selector bankası ve external trigger açık ret verir.
Literal XML enum Value/Unit/float Inc desteklenir; çözülemeyen dolaylı enum/float
increment bilgisi güvenli ret verir. XML oturum başına en fazla 8 MiB, payload
readback en fazla 512 MiB ile sınırlıdır; gerçek buffer/stride kabulü ICM-04c'dedir.
Transaction önce snapshot alır, trigger/auto kapatır, geometriyi ve manual ayarları
uygular, tüm hedefleri ve kimliği tekrar okur. Hata alan yazım da değişmiş kabul edilir;
rollback bütün eski readable envanterle doğrulanır. Doğrulanamazsa Fault olur,
yeniden configure reddedilir; owner-thread close handle'ı temizleyebilir. UserSetSave yoktur.
Stream henüz eklenmedi; ICM-04c/04e bu idle önkoşulunu lifecycle'a bağlamalıdır.

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

**ICM-04c OECS buffer sınırı:** `IndustrialFrameProbe` owner worker'da tanı amaçlı
tek kare alır; production admission veya tazelik kanıtı üretmez. Actual PayloadSize,
Width/Height ve Mono8 okunur; `payload × bufferCount` bütçesi bölme ile allocation
öncesi doğrulanır (varsayılan 4 node/128 MiB, en fazla 64 node/512 MiB; wait 1–30000 ms).
Bu advertised payload bütçesidir; vendor iç bookkeeping/USB transfer belleği ve RSS
hard sınırı değildir. Retained owned ImageFrame bütçesinden ayrıdır.

C API kendi pool allocate/queue/revoke işlemlerini yönetir: SetImageNodeNum →
StartGrabbing → GetImageBuffer → owned copy → FreeImageBuffer → ClearImageBuffer →
StopGrabbing; oturum sonunda CloseDevice/DestroyHandle/unload. Bu SDK başlığında
stride ve ayrı completion status yoktur: native yol yalnız `nFrameLen == width × height`,
kayıp paketsiz Mono8 kabul eder. Belirsiz padding/truncation/overflow, packed/renk ve
unparsed chunk reddedilir; stride açıkça bildiren backend fixture'ı padded rows sınar.
Ham frame numarası ve birleşik 64-bit tick ayrı actual evidence'dır; tick UTC exposure
sayılmaz, tanı karesine source/frame/cycle admission ID atanmaz.

Start hatasında da stop/flush denenir; copy/validation/wait hatasında eldeki buffer
iade edilir. Cleanup hatasında owned kare yayımlanmaz, Faulted olur; close/destroy ile
owner thread üzerinde sonlandırılır. Stop sonrasında belirsiz borrowed pointer'a tekrar
Free çağrısı yapılmaz. Destroy/unload başarısızsa mevcut library-pin kuralı korunur.
Yeniden edinim otomatik recovery yapmaz; hatalı probe açık close/reopen gerektirir.
Gerçek cihaz stream ABI/pool davranışı ve G1 kabulü cihaz kanıtı ister.

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

**ICM-04e-a yazılım sınırı** (`application/CameraWorkerSession`): exact
`PhysicalDeviceIdentity` vendor/model/serial anahtarıyla Calibration/Editor/Runtime
arasında process-local exclusive lease alınır; source, producer yolu ve kullanım
amacı ikinci sahip yaratmaz. Factory, komutlar, shutdown ve backend destructor tek
kalıcı worker'da çalışır. Controller yalnız owner thread'de `poll()` ile completion
işler; tek uçuşta iş vardır, production FIFO doluluğu SDK dispatch'ten önce reddedilir.
`requestStop()` generation/cancel ve kuyruğu hemen geçersizleştirir; bekleyen komut
drain olduktan sonra caller açık `submit(Stop)` acknowledgement veya
`requestShutdown()` cleanup ister. Bu işlem donanım pending-trigger bariyeri değildir.
Eksik/yanlış frame, exception ve geç completion başarı yayımlamaz.

Worker ömrü bir `JobSession::Connection` işiyle izlenir; shutdown yeni iş almaz,
bütçe aşımı join/cleanup yerine geçmez. Lease ancak backend shutdown/destruction
ve worker join sonrasında bırakılır. Cleanup exception'ında kamera lease'i süreç
ömrünce karantinada kalır; shutdown Ready worker'ın bittiğini söyler, hata temizlendi
anlamına gelmez (lifecycle Fault/error korunur). Destructor cancel+join yapar,
kesilemeyen SDK çağrısında bloklanabilir; UI kapanışı önce nonblocking shutdown
polling yapmalıdır. Backend shutdown sözleşmesi partial/fault durumunda da cleanup
gerektirir. Native factory ve ürün servis bağlantıları henüz yoktur; bu sınır
OecsSession backend kilidini kaldırmaz veya ICM-04d/04e/G1 kabulünü açmaz.

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
`industrial_camera`/1 olarak kayıtlıdır (minimum editor9). Legacy webcam tool'u industrial gibi etiketlenmez. Explicit tool
ekleme/binding profile değişikliği dışında eski codec/hash/no-op Save değişmez.
Kamera tek sefer kaydedilir; recipe source ilgili CameraId/setup/profile sürümüne bağlanır.

**ICM-07a-a bağımsız değer hazırlığı:** `domain/IndustrialSourceReference` ayrı
`visionflow.industrial-source-reference`/1 formatıdır. **ICM-07a** tool parametresi
`sourceReference` içine bu nesneyi bağlar; `null` yalnız seçilmemiş offline taslaktır.
Tool'un girdisi yok, çıktısı `image` Image/px/image'dir. Açık ekleme editor9'a geçer;
editor1–8 bu kind'i reddeder. Root/recipe/engine/terminal numaraları değişmez.
Kamera/setup/profile ID ve revizyonları, exact fiziksel kimlik, requested
acquisition hash ve 1–600000 ms timeout taşır; yerel backend/producer veya izin içermez.
`bind` yalnız explicit referans üretir, environment değiştirmez. `validateAgainst`
güncel contract ile dangling/stale binding, aynı revizyonda ayar/geometri değişimi,
eksik geometri ve desteklenmeyen tetik tanımını reddeder. Kaynak tek software capture
ister: Software + açık FrameStart veya FrameBurstStart, birbirine alias değildir.
Timeout exposure süresini aşmalıdır; bu alt sınır transfer bütçesinin yeterli olduğunu
kanıtlamaz. Hash requested değeridir; actual readback, completion veya tazelik kanıtı
değildir. `ProjectService::bindIndustrialSource` güncel contract'tan explicit referans
üretir; environment/profile üzerine yazmaz. Varsa ayrı sourceSetup aynı kamera,
kurulum ve revision'ı göstermelidir; farklı kurulum bağı önce açıkça kaldırılmalıdır.
Kaynak parametre/enabled değişimi legacy calibrationBinding'i invalid eder; rename/no-op
korur. Toolbox çevrimdışı taslak ekler ve edinimin kullanılamadığını açıkça gösterir.
Kamera/profil seçimi UI 07c ve execution 07b hazırdır; production/native kapıları kapalıdır.

Editor/Runtime aynı acquisition provider'ı kullanır. TargetOnly aynı doğru frame'de
acquisition0'dır; profile/model değişimi ilgili source cache'ini invalid eder.
Runtime Run preparation'dır, üretim trigger'ı veya sonuç üretmez; gerekirse cihazı
kontrol/configure etmek UI'da hazırlık olarak gösterilir. Begin tam required source
seti ve current profile/device/geometry kanıtını yeniden doğrular. Actual frame de
aynı snapshot ile eşleşir; bir kaynak hatasında global PASS üretilmez.

ICM-08a: `MeasurementWorkspace::productionSources` tek environment/model görünümünü
trusted application composition'a bağlar. `freezeIndustrialRuntime` her enabled
industrial source için exact discovery/backend/capability çözer; prepare/begin
Connect/Configure/Grab veya software command üretmez. `industrialSource`/1 ayrı
branch dependency'si reference, camera/setup/profile, physical, backend ve yerel
route'u taşır; eski measurementSource/1 zarfı gevşetilmez. Decoder exact alanları ve
requested hash/revision'ı denetler. Gerekli model scope2 physical/device/setup/profile
ve geometryId ile eşleşir; raw-only kaynak unused/offline modele bağlı değildir.

Whole-set snapshot contract/route/factory ve resolved device'ı değer kopyasıyla
saklar; worker canlı registry/model seçimini okumaz. Edinim öncesi fresh exact
resolve aynı device/backend/route olmalıdır; actual identity/readback ve raster
07b provider denetiminden geçer. Runtime ortak lease'i kullanır. UI/profil/model
ve backend değişimi eski cycle'a sızmaz; sonraki begin dependency farkında reprepare
ister. Duplicate physical identity prepare'da reddedilir.

08a yalnız in-memory admission/worker kabulüdür. Persistent whole-set service
industrial prepare'ı açıkça reddeder; normal workspace yolu industrial composition
vermez. Industrial worker sonucu private completion authority almaz ve finish
reddedilir. Terminal1–7 codec/okuyucu değişmedi; 08b bu kanıtı digest ve yeni exact
terminal schema'ya birlikte bağlamadan kullanıcı production açılamaz. Normal factory
test adaptörüne veya native backend'e capability vermez.

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

### ICM-04d uygulama sınırı ve fiziksel bulgular

`IndustrialSoftwareCapture` owner worker'da source için private UUID cycle/frame
admission oluşturur ve mevcut AcquisitionService sonucunu kullanır. Tek in-flight
SDK işi; process genelinde yeniden açılışta tekrar kullanılmayan generation;
ready/command/wait/copy/cleanup sonrasında iptal ve monoton deadline kontrolü vardır.
Pool ve wait'e kalan bütçe uygulanır. Reentrant/foreign worker isteği SDK'ya gitmez.

Backend arm kanıtı ancak eski exposure'ları quiesce edip stream-ready ve actual
counter epoch doğrulayabiliyorsa geçerlidir. Bir sonraki kareye host generation
etiketlemek kanıt değildir. Frame aynı epoch'ta ve arm'daki 32-bit frame/trigger
sayaçlarının modulo tam ardılı olmalıdır. Eksik kanıt, reset/gap/duplicate/late kare
ERROR verir; eşit piksel hash'i ret nedeni değildir. Owned copy ve tüm cleanup
başarılı olmadan frame/evidence dönmez. Kalıcı schema değişmedi;
trusted Runtime/lease/production capability hâlâ sonraki görevlerdedir.

Native OECS arm/command yolu **henüz uygulanmadı**, default unsupported döner;
fixture başarısı gerçek trigger yeteneği değildir. 17 Ekim gerçek cihaz yalnız
FrameBurstStart bildiriyor; FrameStart kuralı korunur. AcquisitionStatusSelector
missing ve AcquisitionStatus unavailable; sorgulanan TriggerCounter/FrameCounter/
Chunk/Event alanlarından kanıt alınamadı. Native ready/counter/epoch yolu ve selector
sözleşmesi çözülmeden ICM-04d tamamlanmış sayılmaz. Ayrıntı
[04d kaydında](../archive/2026-10-17-icm-04d.md).

Sonraki [native tanı](../archive/2026-10-17-icm-04d-native-diagnostic.md) ayrı
opt-in executable'dır; ürün backend'inin unsupported sınırını değiştirmez.
FrameBurstStart/burst=1/cache=false ile iki açılışta 100'er tetik/100'er buffer
gözlendi. Raw `nFrameCounter`/`nTriggerIndex` hep 0; `nFrameNum` yeniden açılışta
0'dan başlıyor. On/Software aktifken de ready düğümü yok. Quiet timeout, başarılı
command, artan tick/frame number ve değişen hash actual counter epoch yerine
kullanılamaz. CounterEventSource yalnız Off/Line0/Line2 destekliyor; bu sayaç
software-trigger oracle'ı değildir. H02 sahne/tazelik ve production kabulü açık kalır.

Sonraki [korelasyon kararı](industrial-camera-trigger-correlation.md) önceki sıfır
metadata bulgusunu daraltır: FrameSpecInfo'nun Framecounter/ExtTriggerCount bankaları
açılınca actual sayaçlar ilerliyor. Önceki kayıt kapalı metadata denemesidir.
Backend implementation capability `OecsTriggerCorrelation` ile ayrı bildirilir;
default Unavailable. Application adapter bunu destek diye ilan etmez; doğrudan
softwareFrame da feature/pool/start/command öncesi unsupported verir ve idle
session'ı Faulted yapmaz. ActualCounterEpoch ilanı tek başına frame kanıtı değildir:
mevcut prepare/arm/generation/modulo sayaç kontrolleri her istekte zorunludur.
FrameSpecInfo desteği native capability'yi açmaz; baseline/quiescence ve raster
etkisi henüz doğrulanmadı. FrameStart şartı ve 04e bağımlılığı korunur.

**Yeni kullanıcı kararı (17 Ekim):** ilk sensör satırı ölçüm dışında bırakılabilir.
Onaylanan yön tam sensör 2448×2048 ediniminden, sensör başlangıcı (0,1) olan
2448×2047 ölçüm rasteri üretmektir; donanım OffsetY değiştirilmez. Yeni geometri
kimliği/kalibrasyon invalidation ve typed raster politikası ürün entegrasyonunda
zorunludur. [Opt-in fiziksel tanıya](industrial-camera-trigger-correlation.md) ek olarak
ICM-03c `IndustrialMeasurementRaster` application servisi bu dönüşümü explicit uygular:
actual `GeometryReadback` değişmez, sonuç ayrı `measurementGeometry`, acquisition
içindeki region, input content ID ve versioned policy ID taşır. Geçerli exact model,
tam-sensör/raw Mono8 ve bilinen identity transform gerektirir. Eski coordinateSystemId
çıktıya kopyalanmaz; timestamp/admission alanları uydurulmaz. `revisedSetup` yalnız
ölçüm geometrisi adayı üretir; mevcut revision/invalidation kurallarını kullanır.
**Bu aday kamera configure girdisi değildir**: donanım tam-sensör kalır. Persist
edilen acquisition/measurement ayrımı ve ortak provider bağlantısı sonraki entegrasyonda
uygulanmadan aday otomatik kaydedilmez/aktive edilmez. Native adapter/schema/UI
henüz bu çıktıyı kullanmaz. Bu izin counter
epoch'u kanıtlamaz; mevcut ActualCounterEpoch ve erken Unavailable reddi korunur.

Gerçek nested EnumEntry çocukları enumeration kapsamında ayrıştırılır; On/Off gibi
local isimler başka enum'u ezmez. ClearImageBuffer Stop'tan sonra CALLORDER verdiği
için cleanup sırası Free → Clear (stream çalışırken) → Stop'tur; flush hatasında da
Stop denenir. Stop başarısızsa hata/Faulted korunur. Flush tek başına tazelik kanıtı
değildir. Bu düzeltmeler önceki fixture-only varsayımlarını geçersiz kılar.
