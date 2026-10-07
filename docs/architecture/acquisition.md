# Adım 6 — acquisition ve kamera kararı

## Webcam devamı — 7 Ekim 2026

Kullanıcının güncel yönlendirmesi: endüstriyel kamera yok; webcam ve bağımsız 6C işleriyle ilerle. Basler/GenTL fiziksel kabulü ve SDK feature/grab işleri açık kalır; form/producer bekleme bu oturumu durdurmaz.

Qt Multimedia WebcamAdapter kendi worker thread'inde QCamera/QMediaCaptureSession/QVideoSink ve yerel event loop oluşturur. İlk frame QVideoFrame.toImage → RGB → owned BGR frame'e kopyalanır; camera/session/sink aynı worker'da stop/detach/destruction görür. 10 ms timer cancel/deadline gözler; çağrı dönüşünde AcquisitionService geç kareyi tekrar reddeder. Driver startup/stop/cleanup hard timeout değildir. Process-local try-lock üst üste webcam edinimini bekletmeden ERROR yapar. Sürekli stream, CameraLifecycle production FIFO veya çoklu kamera entegrasyonu değildir.

Registry webcam source image/px portu verir; tek source sınırı tüm acquisitionSource tool'ları kapsar. Webcam executor ortak AcquisitionService'den frame alır; EditorSession mevcut worker/stale/cancel yolunu kullanır. Properties index/timeout reçetede saklanır. Varsayılan index 0, backend qt; Qt media implementation ortam tarafından seçilir. İndex cihaz listesi sırasıdır, kalıcı fiziksel ID değildir. Software capability burada host tek kare isteğidir; external/continuous/exposure/gain false. Gerçek exposure/device timestamp iddiası yok.

OpenCV videoio yerel kurulumda yok; mevcut Qt 6.12 Multimedia kullanıldı, ek kurulum yok. Widgets OFF core artık Multimedia bağımlılığı da taşır. Qt frame/session API referansları: [QVideoFrame](https://doc.qt.io/qt-6/qvideoframe.html), [QMediaCaptureSession](https://doc.qt.io/qt-6/qmediacapturesession.html).

Gerçek varsayılan webcam index 0 1280×720, yaklaşık 948 ms, acquisition ve snapshot Blob DAG DONE; editor worker capture da DONE. Görsel çok karanlık; parça/ışık/doğruluk kabulü değildir. İlk sandbox device probe ERROR; yükseltilmiş yerel cihaz erişimi başarılı. Probe JSON ve smoke INI/PNG build/webcam-release altında. Physical unplug/recovery/Stop ve endüstriyel trigger kabulü açık; hata/late/cancel/cleanup/recovery sentetik testlerde kabul edildi. Üretim snapshot webcam'i henüz kabul etmez.

Sonraki bağımsız 6C: [Measurement servis çekirdeği](measurement.md) tamamlandı; persistence/binding/resolver ve ortak UI/Runtime entegrasyonu sıradadır.

## 6B — GigE keşif bütçesi düzeltmesi (7 Ekim 2026)

GenTLTransport ve CameraService toplam producer keşif bütçesini (`timeoutMs`, varsayılan 1000) liste güncellemesi tarama beklemesinden (`scanWaitMs`, varsayılan 250) ayırır. TLUpdateInterfaceList ve her IFUpdateDeviceList çağrısına `min(scanWaitMs, max(1, remainingMs/2))` verilir. Başarılı scan çağrısı bütün bekleme süresini kullanabilse de sonraki interface/metadata okumaları için bütçe kalır. Her producer kendi toplam bütçesini alır; load/init önceki gibi bu bütçenin dışındadır. Sonuç kabulünde toplam deadline ve cancel korunur; aşılmış bütçeyle kısmi cihaz listesi yayımlanmaz. Vendor call/cleanup zorla kesilmez.

CLI `--scan-wait` ile tarama üst sınırını seçer; JSON discoveryBudgetMs/scanWaitCapMs değerlerini raporlar. Daha kısa scan geç yanıt veren cihazları kaçırabilir; DONE yalnız kullanılan tarama penceresinin sonucudur, ağda hiç kamera olmadığının kanıtı değildir. Çok sayıda interface veya yavaş metadata toplam timeout üretebilir; gerektiğinde toplam bütçe ve scan cap ayrı ayarlanır.

Gerçek Galaxy GxGVTL.cti 1000/3000 ms toplam bütçe, 250 ms scan cap ile DONE/0 cihaz: build/6b-galaxy-gige-default-budget.json ve build/6b-galaxy-gige-budget.json. Önceki Basler yolu bu oturumda erişilebilir bulunmadı; build/6b-basler-gige-budget.json ERROR/eksik producer. Önceki kurulum kaydı güncel erişilebilirlik kanıtı değildir. SDK feature/stream/grab uygulanmadı; fiziksel kabul açık. Aşağıdaki önceki GigE TIMEOUT kaydı düzeltme öncesine aittir.

## Güncel devam sırası — 7 Ekim 2026

[Onaylı plan](development-plan.md) uyarınca A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı; güncel iş 6B gerçek SDK ve tek kamera entegrasyonudur. Kullanıcı bağlı kamera olmadığını, Basler/Daheng ve GigE hedefini bildirdi; SDK kurulumlarını yetkilendirdi. Galaxy tam SDK 2.6.2608.9131 ve Basler pylon Runtime 26.09.2 kuruldu. Galaxy dar MinGW C ABI init/discovery/close 0/0/0; gerçek USB GenTL producer probe DONE/0 cihaz. GigE producer keşfi toplam deadline kontrolünde TIMEOUT; scan bekleme ve toplam bütçe ayrımı sonraki uygulama işidir. Basler tam Developer SDK kişisel/şirket formu kullanıcı adımını bekler; Runtime kurulumunda PylonC DLL/header yok. Yeni stream/grab uygulanmadı; fiziksel kabul açık. Kurulum kaynak/hash/ABI/env kanıtları [camera-sdk-installation.md](camera-sdk-installation.md) içindedir. Ayrıntılı kanıt progress.md içindedir. A6 aşağıdaki donanımdan bağımsız CameraLifecycle ve kuyruk çekirdeğidir; GenTL stream/grab bağlantısı ve fiziksel kabul 6B'de açıktır. Ortak ImageFrame::fromBuffer owned satır kopyasıyla adapter tamponunu serbest bırakabilir; preview raw boyut/varlık kaynağı değildir. Kaynak/frame/cycle/koordinat, UTC completion ve varsa ayrı exposure/device clock metadata'sı çıktılara taşınır. Gray16 native endian, renk engine depolaması BGR/BGRA; padded giriş stride kabul edilir, owned depolama compact'tır. Tek dosya source sürer; fiziksel kamera kabulü değildir.

Process-geneli tek kilit çoklu cihaz hedef mimarisi sayılmaz. Ayrı kamera süreci yalnız gerçek SDK kesilebilirlik testleri gerektirirse değerlendirilecektir. Cooperative deadline hard timeout değildir; fiziksel kabul 6B'de gerçek producer/modelle yapılacaktır. Camera Manager görünümü ve menüler korunur. Aşağıdaki 6.1–6.2 kayıtları tarihsel kapsamı anlatır.

## A6 — uygulanmış çekirdek ve 6B bağlantı sözleşmesi

`application/CameraLifecycle` SDK çağırmayan, tek owner thread'de erişilen ve kopyalanamayan controller'dır. Komut başlatma bir serial/generation/operation ticket, cancellation token ve monoton Deadline verir. Aynı anda bir SDK işi kabul edilir; başka komut beklemeden hata verir. Worker immutable ticket/isteği kullanır; controller'a worker'dan erişmek reddedilir. `complete` yalnız aktif ticket'ı kabul eder. SDK başarısı çağrı dönüşüyle bildirilmeden controller kendiliğinden Connected/Streaming üretmez.

| Komut | Başlangıç | Worker süresince | Başarılı dönüş |
| --- | --- | --- | --- |
| Connect | Disconnected | Connecting | Connected |
| Configure | Connected / Configured | Configuring | Configured |
| Start | Configured | Starting | Streaming; yeni generation |
| Grab | Streaming | Grabbing | Streaming |
| Stop | Streaming / Stopping, aktif iş dönmüş | Stopping | Configured |
| Disconnect | Connected / Configured / Fault / Stopping, aktif iş dönmüş | Disconnecting | Disconnected |

Hata, cancel veya deadline sonrası başarı Fault üretir; kuyruğu temizler. `transportLost` aynı şekilde Fault'a geçer, aktif token'ı iptal eder. Fault sırasında geç worker başarısı Fault'u temizlemez. Yeniden bağlanma otomatik retry değildir: aktif worker dönüşü → SDK cleanup/Disconnect onayı → Connect → Configure → Start. Disconnect başarısız/geç dönerse Fault kalır; cleanup tekrar gerekir. Controller SDK handle taşımadığı için destructor donanım cleanup kanıtı değildir.

`requestStop` varsayılan **1000 ms** gözlem bütçesiyle yeni grab/frame kabulünü kapatır, generation artırır, kuyruğu temizler ve aktif işi cooperative iptal eder. Tekrar Stop bütçeyi uzatmaz. `stopBudget` Pending/Exceeded/Complete döndürür; Exceeded işin durduğunu veya handle'ların serbest kaldığını söylemez. Aktif connect/configure/start/grab dönüşü Stopping'i geri açmaz. Worker dönmeden Stop/Disconnect SDK işi başlatılamaz. Sonrasında adapter idempotent stop/cleanup yapar; Stop veya Disconnect başarı onayı güvenli son durum verir. Yeni Start önceki stop budget'ını sıfırlar.

`JobSession::requestShutdown` aynı varsayılan 1000 ms bütçeyle bütün token'ları iptal eder ve yeni işleri reddeder. `shutdownState` bloklamayan poll API'sidir; owner completion callback'leri ve bütün future'lar bitmeden Ready dönmez. BudgetExceeded halinde future'lar sahiplenilmeye devam eder. Mevcut GUI kapanış guard'ı korunur; otomatik zorunlu çıkış eklenmedi. Destructor hâlâ cancel/wait yapar ve kesilemeyen SDK'da sınırsız bekleyebilir. 1000 ms hard kapanış garantisi değildir.

`AcquisitionRequest.deadline` optional process-local steady_clock sınırıdır. Kuyruğa kabul anında oluşturulur; servis timeoutMs ile daha erken sınırı seçer. Kuyrukta tükenmiş istek adapter'a ulaşmaz. Adapter aynı mutlak deadline'ın `remainingMs()` değerini her SDK wait öncesi kullanır; sıfır bütçede yeni çağrı başlatmaz. Service dönüşte cancel/deadline kontrolünü tekrar yapar ve geç frame/hata için frame yayımlamaz. UTC completion/exposure/device ticks deadline saati değildir. DLL load/init, SDK close/decode gibi kesilemeyen çağrılar bu yöntemle zorla durmaz.

### Bounded frame kuyruğu ve üretim trigger admission

Kapasite 1–64 owned frame, varsayılan 2; PreviewLatest ve ProductionFifo modları oturum oluşturulurken seçilir, aynı kuyruk paylaşılmaz. Kuyruk yalnız controller owner thread'de push/pop edilir; producer worker sonucu owner'a teslim eder. Byte limiti uygulanmaz; 6B maksimum frame boyutu ve toplam bellek bütçesini cihaz yapılandırmasında belirlemelidir.

- PreviewLatest: doluysa en eski bekleyen frame düşer, yeni frame kabul edilir; DroppedOldest açık dönüş değeridir. Preview frame'i üretim cycle'ı yerine kullanılamaz.
- ProductionFifo: doluysa Full döner; yeni frame reddedilir, mevcut frame/cycle değiştirilmez. Controller `begin(Grab)` dolu frame kuyruğunu ve dolu in-flight slot'u SDK dispatch öncesinde reddeder; bekleyen trigger isteği için bu teslimde ayrı/unbounded kuyruk yoktur. Dolu/busy durumda software isteği açık acquisition hatasıyla reddedilir. External trigger throttling desteklenmiyorsa kayıp/overflow hata olarak raporlanır; sonraki frame önceki cycle'a atanamaz. 6B adapter/UI/Runtime bu dönüşü cycle'a bağlayacaktır.
- Enqueue source/frame/cycle metadata eşleşmesini ve raw boyutu doğrular. Eski generation, stopped/fault state veya deadline frame'i kabul edilmez. Deadline pop sırasında da kontrol edilir; kuyruğu beklerken geç kalan üretim frame'i tüketilmez. Expire FIFO kalan sırasını korur.
- accepted/dropped/rejected/expired/stale/invalid/cleared, depth ve highWater sayaçları izlenebilir. Sayaçlar controller ömründe kümülatiftir; Stop/fault/yeniden Start kuyruğu temizler. Production Full/expired/cleared sayısı başarılı parça kararı veya başka isteğe sessiz aktarım değildir; 6B consumer ilgili istek için hata/iptal kaydı tutmalıdır.

### Producer/device/worker sahipliği ve fiziksel kabul

Mevcut GenTLTransport RAII tek worker içinde library → TL → interface → device zincirini oluşturup ters sırada kapatır; process-geneli try-lock aynı anda tek oturumu korur. Bu güvenli mevcut keşif/read-only sınırı korunmuştur. Kamera controller'ı producer/device referanslarını taşıyan worker komutunu koordine eder; SDK handle'ları UI/controller thread'e aktarılmaz. Producer library init/close ömrü bütün bağlı device/stream/buffer/event handle'larını kapsamalıdır. SDK release önce owned ImageFrame kopyası tamamlanır; buffer iptali ve flush worker'da yapılır.

6B ilk gerçek kamerayı bu controller'a bağlayacaktır. Çoklu cihaz için canonical producer path ile producer session sahibi ve altında cihaz worker'ları tasarlanmalı; GCInitLib/GCCloseLib paylaşımı ve SDK thread affinity gerçek producer ile doğrulanmadan mevcut kilit kaldırılmaz. Bloklanan worker varken aynı producer/device yeniden açılmaz ve DLL unload edilmez. Güvenilir kesme/cleanup kanıtlanamazsa ayrı kamera süreci, bütçe sonunda süreç restart'ı ve yeni generation değerlendirilecektir; süreç izolasyonu şu an uygulanmadı.

Kabul: Release build ve CTest **9/9**; `camera_lifecycle_contract` sentetik yaşam döngüsü, overlap/stale ticket, fault/reconnect cleanup, stop sonrası frame bastırma, preview drop/production Full/FIFO, enqueue/pop deadline, thread ownership ve kesilemeyen worker shutdown bütçesini kontrol eder. Mevcut acquisition/DAG/session/GenTL testleri korunur. `build/a6-editor.png` ve `build/a6-camera.png` offscreen incelendi. Yeni controller mevcut keşif/read-only ekranına canlı stream özelliği eklemez. Fiziksel trigger/grab, buffer/event cleanup, unplug/reconnect, SDK latency/interrupt, native DPI/kurulum/CUDA kabulü açıktır.


7 Ekim 2026. İlk altyapı teslimi; gerçek kamera, kalibrasyon, robot/PLC ve Runtime henüz tamamlanmadı.

## Kamera kararı

Kullanıcı Basler, Daheng ve başka markalar için seçimi geliştirmeye bıraktı. Ortak model GenICam özellikleri/GenTL transport yaklaşımıdır. GenICam tek başına driver değildir; her cihaz için uyumlu producer/sürücü gerekir. Producer yeterli değilse Basler pylon veya Daheng Galaxy adapter'ı aynı application sözleşmesini uygular. UI SDK çağırmaz.

İncelenen resmi kaynaklar: [EMVA GenICam](https://www.emva.org/standards-technology/genicam/), [Basler C API](https://docs.baslerweb.com/pylonapi/c/programmingguide), [Basler producer/deployment](https://docs.baslerweb.com/pylonapi/pylon-deployment-guide), [Daheng SDK](https://en.daheng-imaging.com/index.php?a=lists&c=index&catid=59&m=content&sylx=23&syxj=44). Daheng model/SDK producer ve compiler uyumu henüz doğrulanmadı.

Mevcut MinGW x64 toolchain korunur. İlk tercih C ABI üzerinden GenTL/vendor C API; calling convention, exports, buffer lifetime, lisans ve x64 uyumu kurulu SDK ile test edilmelidir. MSVC C++ binary'leri doğrudan MinGW'ye bağlanmaz. Standart Program Files/Program Files (x86)/C: üst düzey klasör adı kontrolünde SDK kurulumu bulunmadı; tüm disk taraması yapılmadı. Kamera/SDK/ABI kabulü yapılmış sayılmaz.

## Uygulanan sözleşme

`AcquisitionService` adapter capability ve gerçek frame alır. Request sourceId/cycleId/frameId, trigger, pozitif timeoutMs ve cooperative cancellation taşır. DONE aynı kimlikler ve edinim tamamlanma UTC zamanı ile dolu ImageFrame döndürür. Bu zaman kamera exposure timestamp değildir. ERROR/CANCELLED/TIMEOUT frame taşımaz. Unsupported trigger/boş kimlik adapter çağrısından önce reddedilir. İptal/deadline adapter dönüşünde yeniden kontrol edilir; geç frame yayımlanmaz.

Servis blocking çağrıyı zorla kesmez; SDK adapter'ı timeout/cancel değerlerini beklemelerde uygulamalıdır. Adapter worker'a ait olmalı veya kendi eşzamanlılık politikasını sağlamalıdır. Servis thread başlatmaz.

`ImageFileAdapter` yalnız software request destekler; external/continuous/exposure/gain false. Bu kamera trigger'ı çalıştırıldığı anlamına gelmez. Run Flow yeni servise bağlandı: 5000 ms geç sonuç kabul sınırı ve mevcut Stop token. Port/cycle kimlikleri, cancel output temizliği ve schema 4 korundu. Editor Open/Reload mevcut worker ImageFileService yolunda kalır. UTC metadata henüz UI/port sonuçlarına taşınmaz.

## Kabul ve devam

Release derleme, CTest 5/5 geçti. Yeni `acquisition_contract` gerçek PNG/piksel, capability, kimlik/UTC, unsupported trigger, eksik kimlik/timeout, edinim öncesi/sonrası iptal, geç frame/geç hata, adapter hatası, boş/eksik frame ve recovery doğrular. Test adapter'ları fiziksel kamera değildir. UI değişmedi; mevcut UI/DAG/matching regression testleri geçti.

Bu bölüm Adım 6.1 teslim kaydıdır. Güncel devam sırası aşağıdaki 6.2 kaydındadır.

## Adım 6.2 — GenTL discovery ve salt okunur bağlantı kontrolü

`hardware/GenTLTransport` Windows x64 `.cti` dosyasını QLibrary ile yükler ve resmi EMVA GenTL 1.6 header'ındaki function-pointer tipleriyle C export'larını resolve eder. Header değişmeden `third_party/gentl` içinde kaynak/hash/lisans bildirimiyle tutulur; vendor driver veya GenApi runtime projeye eklenmedi.

GCInitLib → TLOpen → interface/device discovery → TLClose → GCCloseLib yaşam döngüsü RAII ile yönetilir. Device kaydı canonical producer path + interfaceId + deviceId, vendor/model/serial/transport taşır. Kimlikler boş/NUL içeren olamaz; string ve cihaz/interface sayıları sınırlandırılır. Optional metadata uygulanmıyorsa boş kalır; bozuk tip/boyut, eksik export, invalid binary ve vendor hataları başarı olarak sunulmaz. Kamera kimlikleri bu teslimde proje/reçeteye kaydedilmez.

Read-only bağlantı kontrolü IFOpenDevice(DEVICE_ACCESS_READONLY) → DevClose kullanır; interface, system ve library sırayla kapanır. Connected durumunu kalıcı tutmaz, remote register yazmaz, trigger veya görüntü edinimi başlatmaz. Açık cihaz varken discovery ve tekrar open reddedilir. Bir process içinde aynı anda tek GenTL oturumu vardır; ikinci oturum bloklanmadan hata verir. Vendor library thread/lifetime davranışı fiziksel SDK ile ayrıca kabul edilmelidir.

`CameraService` UI'dan SDK ayrımını korur. Explicit producer yolları environment aramasının önüne geçer; boş liste `GENICAM_GENTL64_PATH` klasörlerindeki .cti dosyalarını bulur. Klasör araması recursive değildir; canonical yollar tekilleştirilir. Bir producer hatası diğerini engellemez. Keşif budget her producer için ayrı uygulanır. İptal/deadline çağrı sınırında ve dönüşte kontrol edilir; iptal edilen rapor cihaz taşımaz. DLL load/init, read-only open/close ve vendor blocking çağrıları için hard timeout yoktur. Timeout/cancel fiziksel transport davranışı bu testlerle kanıtlanmış değildir.

Camera Manager worker üzerinden keşif, cooperative Stop ve seçili cihaz read-only testini sunar. Worker sırasında kapanış engellenir ve kontroller kilitlenir. Yeni arama eski cihaz listesini temizler. Kamera bulunması/bağlantı testi gerçek görüntü edinimi değildir; bu sınır ekranda belirtilir. Sidebar bağlı değil kalır. Port/cycle/schema ve mevcut Run Flow davranışı değişmez.

`visionflow_camera_probe` aynı service'i kullanır; JSON `DONE`/`NO_PRODUCER`/`ERROR` döndürür. `--open-check` read-only test ekler. Kamera bulunmayan başarılı discovery `DONE` olabilir. Gerçek yerel environment araması `NO_PRODUCER`, exit 1; `build/camera-probe-local.json`. Test producer raporu `build/camera-probe-fixture.json`; sentetik cihaz açıkça etiketlidir.

Kabul: Release build, CTest 6/6. Yeni camera_transport gerçek dinamik test DLL C ABI çağrılarıyla discovery/metadata/open-close/destructor cleanup, missing export/file/invalid binary, invalid datatype/size/null handle, access denied, empty interface, optional metadata, timeout, pre/post cancel, producer isolation, concurrent session rejection ve recovery kontrol eder. Bu MinGW test DLL sınırını doğrular; Basler/Daheng/MSVC binary kabulü değildir. UI smoke gerçek düğmelerle missing producer, background/close guard, fixture discovery/open-close, Stop ve liste temizliğini doğrular. `build/step6-camera-test.png` ve `build/step6-camera-no-producer.png` görsel incelendi; native yüksek DPI/çoklu monitör ayrıca doğrulanmalıdır.

Sıradaki: GenApi XML/feature erişimi ve uygun runtime/SDK ABI seçimi; camera trigger/exposure/gain capability, stream/buffer/event grab, owned ImageFrame conversion, disconnect/timeout. Gerçek producer/model bilgisi geldiğinde fiziksel acceptance. Kalibrasyon, robot/PLC ve Runtime bu teslimle tamamlanmış sayılmaz.
