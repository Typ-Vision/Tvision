# VisionFlow Industrial Vision Suite

8 Ekim güncel durum: [Manuel px/mm ve sade Calibration uygulama planı](docs/architecture/manual-scale-calibration-plan.md) **C1 matematik ve kayıt formatı tamamlandı**. Servis düzeyinde manuel px/mm candidate, bağımsız holdout doğrulaması ve strict record/dependency schema 3 replay vardır; Calibration ekranında manuel giriş henüz yoktur. Sonraki C2 katalog/binding/Runtime/CSV entegrasyonu; ardından C3–C5. Bu kullanıcı önceliği aşağıdaki tarihsel P7 doğrudan devam ifadelerinin önündedir; P7 bu paket sonrasında kalır.

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.

P3/P4 yazılım kabulü: Release `build/k1-ui` CTest **22/22**, Widgets OFF `build/k1-core` **20/20**; normal/850×600 UI görselleri incelendi. Güncel uygulama `build/k1-ui/visionflow.exe`. Ayrıntılı kanıt ve sınırlar [ilerleme kaydında](docs/progress.md).


8 Ekim 2026 P2 tamamlandı: Recipe Manager tek tıkla yalnız satır seçer; **Load Recipe / Edit Recipe** ayrıdır. Load kaydedilmiş bağımsız kopyayı statik kontrol eder; dirty durumda Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal sunulur. Runtime **Run / Stop / Tek çevrim** kullanır; Run görüntü veya parça sonucu üretmeden coordinator ile otomatik hazırlanır. Manuel çevrim, cooperative Stop ve aynı terminal kayıt retry coordinator üzerinden geçer. Seçili, editörde açık ve Runtime yüklü reçete ayrıdır. Eski validate/activate/rollback düğmeleri kaldırıldı; rollback servis API’si korunur. Bu P2 teslimi P3/P4 ile genişletildi; güncel socket ve Run Tool davranışı yukarıdadır. Fiziksel kabul/K4 ertelenmiş kalır. [Plan](docs/architecture/runtime-editor-workflow-plan.md).

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Ayrıntılı plan](docs/architecture/calibration-plan.md).

C++20, Qt Widgets ve OpenCV tabanlı endüstriyel machine vision projesi. 7 Ekim 2026.

## Belgeler

- [Ürün tanımı](docs/requirements/01_machine_vision_yazilim_tanimi.md)
- [UI gereksinimleri](docs/requirements/02_machine_vision_ui_gereksinimleri.md)
- [C++ / Qt / OpenCV gereksinimleri](docs/requirements/03_cpp_qt_opencv_gereksinimleri.md)
- [Mimari](docs/architecture/architecture.md)
- [Onaylanan geliştirme planı](docs/architecture/development-plan.md)
- [Proje/reçete kayıt sözleşmesi](docs/architecture/project-format.md)
- [Editor/tool/görüntü sözleşmesi](docs/architecture/editor-definition.md)
- [ROI/match ve oturum](docs/architecture/matching-editor.md)
- [Typed DAG ve execution](docs/architecture/tool-execution.md)
- [Görüntü edinim ve kamera kararı](docs/architecture/acquisition.md)
- [6B yerel SDK kurulumu ve doğrulama](docs/architecture/camera-sdk-installation.md)
- [A7 koordinat/tracking ROI](docs/architecture/coordinates.md)
- [A8 üretim snapshot](docs/architecture/production-recipes.md)
- [6D.1 TCP/IP socket ve PLC simülatörü](docs/architecture/plc-socket.md)
- [A9 persistence/migration planı](docs/architecture/persistence-plan.md)
- [6C Measurement servis çekirdeği](docs/architecture/measurement.md)
- [İş listesi](docs/backlog.md)
- [İlerleme ve oturum devri](docs/progress.md)
- [Matching kararı ve ölçüm](docs/performance/opencv_matching.md)
- [Araç kontrolü](docs/toolchain.md)
- [HTML prototipi](prototypes/visionflow_ui_prototype.html)

## Mevcut durum

Adım 5, 6.1–6.2 ve 6A/A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı. Ortak Recipe Editor, typed DAG, background execution, gerçek Blob/Edge/Calculate, ROI/model teaching, template match, oturum restore ve GenTL keşif/read-only open-close mevcut. Webcam tek kare → owned frame → DAG ve bağımsız planar scale/Measurement/tolerans servis çekirdeği eklendi. Kalibrasyon/evidence sürümlü kaydı, bağımsız current resolver ve reçete sürüm bağlantısı servis katmanında mevcut; Ortak Measurement DAG/Properties ve planar Calibration ekranı mevcut; Runtime Load/Run/Stop/manuel tek çevrim ve kalıcı Logs bağlıdır; validation/activation otomatik, rollback servis API’sidir. Kullanıcı endüstriyel kamera olmadığını bildirdi; webcam ve bağımsız 6C işleriyle devam edilir. Basler/GenTL fiziksel kabulü açık.

Onaylanan sıra: 6A mimari → 6B kamera → 6C Measurement/kalibrasyon/tolerans/doğrulanmış reçete/Runtime → 6C-K plaka tabanlı kamera/düzlem kalibrasyonu → 6D PLC → 6E robot → 7 ürünleştirme. Temel servis yetkisi, hata kodları ve cycle logları 6C'ye dahildir. Üç applicationType aynı editor/engine/Runtime yolunu kullanır. Tema ve editor panel düzeni korunur; 8 Ekim kullanıcı yönlendirmesiyle Setup menüsü ve Runtime operatör görünümü eklendi. Yeni oturumda önce progress okunur; hazırlık ekranları çalışan özellik değildir.

## Yerel derleme (PowerShell)

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build -j 6
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
ctest --test-dir build --output-on-failure
.\build\match_benchmark.exe
.\build\visionflow.exe
```

Yollar bu makineye aittir. CMake/Ninja PATH üzerinde olmalı; başka makinede Qt/OpenCV yolları ve ABI uyumu doğrulanmalı. Qt Multimedia geliştirme/runtime bileşeni gerekir; yerel Qt içinde mevcut, ek kurulum yapılmadı. Dağıtım öncesi windeployqt ve OpenCV DLL paketleme ayrıca doğrulanmalıdır. Toolchain kurulumu otomatik yapılmaz; kullanıcı onayıyla 6B kamera SDK kurulumları yapıldı. CUDA etkin değildir.

## UI çalıştırma ve kullanım

```powershell
.\scripts\run-local.ps1
.\scripts\run-local.ps1 -SmokeTest
```

PS1 çift tıklamada Not Defteri ile açılıyorsa `powershell.exe -ExecutionPolicy Bypass -File "C:\OpenCode\TVision\scripts\run-local.ps1"` kullanın.

Project Manager → New Project ile adı ve mevcut klasörde yeni `.vfproject` yolunu seçin; Save Project ilk kaydı yapar. Recipe Manager → New Recipe ile tür, ad ve görüntü kaynağı referansı girin; ortak editor açılır. Kaydedilmiş reçete değişince DRAFT/dirty olur; revizyon yalnız başarılı kayıtta artar. No-op Save artırmaz. Save/Ctrl+S tüm projeyi atomik kaydeder; başarısız commit/conflict dirty ve önceki dosyayı korur. Kapatma/proje değişiminde Save/Discard/Cancel vardır. Open Project ve Recipe Manager ile yeniden açılır. Copy/Delete/import/export/version history henüz yoktur.

Editor → Open Image gerçek görüntüyü yükler; Image File tool ve göreli yolu reçeteye ekler. Gray8/Gray16/RGB8/RGBA8; Fit, +/−, 1:1/tekerlek zoom ve sol tuş pan desteklenir. Hover orijinal piksel/koordinatı gösterir. Reload yeniden okur; görüntü projeye gömülmez. Toolbox Add Tool/double-click ile araç ekler; Flow seçimi Properties'yi değiştirir. Name/Description/Enabled/parametre değişiklikleri DRAFT olur. Tool silme bağlı bağlantıları temizler.

Typed input seçicileriyle Image File.image → Blob/Edge.image ve iki Edge.point → Calculate.p1/p2 bağlanır. Validate DAG eksik/uyumsuz giriş ve döngüyü gösterir. Run Flow dosyayı yeniden edinip DAG'yi worker'da çalıştırır. Stop cooperative; devam eden OpenCV/decode zorla kesilmez. Results gerçek cycle/frame, state, süre ve typed outputs gösterir. Blob threshold/açık-koyu nesne/4–8 komşuluk/min-max dahil px² alan filtresi ve gerçek kırmızı maske; kabul edilen en büyük alan/merkez; Edge dört yönlü profile ilk contrast geçişi; Calculate Öklid mesafesi px uygular. Search ROI veya bağlı Region kullanılabilir. Found=false downstream SKIPPED yapar; ERROR/SKIPPED parça kararı değildir. Edge profile çizgisi, Calculate P1/P2 sarı çizgi ve px overlay sunar.

Blob Properties: **Min Area / Max Area** dahil piksel sayısı sınırlarıdır; **Max Area: Sınırsız** JSON `null` olarak saklanır. Açık nesne `pixel > threshold`, koyu nesne `pixel ≤ threshold`; komşuluk 4 veya 8’dir. Varsayılan 128/bright/8/min=1/sınırsız eski davranışı korur. Results `candidateCount`, kabul edilen `count`, en büyük kabul edilen `area`, bbox ve centroid sunar. Boş kabulde found=false, area=0, point/region geçersizdir; PASS/FAIL üretilmez. Kırmızı maske gerçek kabul edilmiş piksellerdir; delikler ve ROI offset korunur, raw frame değişmez. Bbox gösterimi ilk 2048 ile sınırlı, maske/sayım tamdır. Blob ROI en çok 16 Mi piksel; daha büyük ROI açık hata verir. Şekil/morfoloji/mm² ve ayrı Blob kabul kararı sonraki genişletmelerdir.

Template Match → Model ROI çiz / öğret → Search ROI çiz → Image input bağlantısı → **Run Tool**. Aynı owned ham karede yalnız Match executor çalışır; Blob/Edge/Calculate/Measurement için de ortak Run Tool vardır. Eksik ya da güncel olmayan bağlı girdiler açık hata üretir; üst araçlar veya acquisition kendiliğinden çalışmaz. Parametre değişikliği hedef ve downstream sonuçlarını temizler, geçerli upstream cache’i korur. Yeni görüntü için kaynak seçip **Görüntü al** veya Open Image/Reload; tam akış için **Run Flow** kullanın. Run Tool zoom/pan ve ham buffer’ı korur; evaluation ID ile özgün acquisition/frame kimlikleri ayrıdır. Partial editor sonucu Runtime üretim kaydı veya PLC çıktısı olamaz. Match gerçek skor/konum ve eşik kabulünü gösterir; sabit açı/ölçek sınırı korunur.

Edge Properties → Referans merkezi → Template Match.point seçip Search ROI çiz ile offset/boyut öğretin. Güncel reference sonucu varsa kullanılır; yoksa model teaching merkezi kullanılır. Güncel match merkeziyle X/Y ötelemesi uygulanır. Reference bulunamazsa Edge/downstream SKIPPED. Model ROI/PNG değişirse referenceGeometry uyuşmazlığı Edge ERROR üretir; Search ROI yeniden öğretimi gerekir. Metadata, Min Score ve Match search değişikliği bu kimliği değiştirmez. Editor schema 6, schema 1–5 açılışını dirty olmadan destekler; legacy geçmiş model değişimleri kanıtlanamaz. Genel açı/ölçek yoktur; mm Measurement ve planar Calibration akışı aşağıda açıklanır.

Panel boyutları, son proje/reçete/ekran `config/local.ui.ini` içinde saklanır; bu autosave değildir. Eksik/bozuk son proje hata ve manuel açma sunar. View → Panel düzenini sıfırla varsayılan düzeni getirir. Minimum pencere 850×600; native DPI/çoklu monitör kabulü açık. Testler build altında izole INI/proje/sentetik görüntü kullanır; üretim INI kullanılmaz. Smoke `step5-editor.png`, `step5-blob.png`, `step5-flow.png` ve kamera görsellerini üretir.

## Webcam tek kare ve 6C servis devamı

Toolbox → Webcam → Add Tool; Properties Device index varsayılan 0 ve Timeout. Run Flow kamerayı worker'da açar, bir kareyi owned frame'e kopyalar, kamerayı kapatıp bağlı Match/Blob/Edge/Calculate DAG'sini çalıştırır. Model öğretimi için önce Run Flow ile kare alabilirsiniz. Reçetede yalnız tek Image File veya Webcam kaynağı vardır; kaynak değiştirmek için mevcut source tool'u kaldırıp bağlantıları yeniden kurun. Ayarlar reçeteyle save/reopen olur, parametre değişimi DRAFT ve eski görüntü/sonuç invalidation üretir.

Bu host isteğidir; exposure başlatan donanım trigger değildir. Live/external trigger/exposure/gain ayarı yok; cihaz index'i kalıcı kamera kimliği değildir. Stop/cancel/deadline geç sonucu bastırır; driver open/start/stop/destruction için hard interruption garantisi yoktur. UI'nin kalıcı Camera bağlı durumu GenTL read-only içindir; tek kare webcam oturumu kapandıktan sonra bağlantısız kalması beklenir. Kamera izinleri/başka uygulama kilidi açık hata üretir.

```powershell
.\build\webcam-release\visionflow_webcam_probe.exe --index 0 --timeout 10000
.\build\webcam-release\visionflow.exe
```

Son 6C Runtime tesliminin güncel binary'leri build/runtime-ui altındadır; ana build içindeki açık uygulama kendiliğinden güncellenmez. Yukarıdaki webcam komutları önceki webcam kabulüne aittir; güncel UI için `.\build\runtime-ui\visionflow.exe` kullanılır. Alt build'den normal başlatma yerel UI INI'sini build/config/local.ui.ini altında tutar; smoke ayarları build/runtime-ui/ui-smoke.ini. Üretim config/local.ui.ini değiştirilmedi. Gerçek yerel webcam 1280×720 kare ve Blob DAG DONE; editor worker capture smoke da geçti. Kare görseli çok karanlıktır; alınan frame üretim görüntü/ölçüm doğruluğu kabulü değildir.

[MeasurementService](docs/architecture/measurement.md) bilinen uzunlukla uniform mm/px, ayrı holdout kanıtı ve ham mm değerle dahil tolerans kararı sunar. Invalid calibration/upstream/frame sonuç üretmez. Calculate px kalır; ayrı Measurement aracı mm/tolerans sunar. Planar Calibration ekranı aynı servisleri kullanır; Runtime yapılandırma kabulü ve kalıcı kayıt bağlıdır. CalibrationService 4 MiB/128 kayıtla sınırlı atomik JSON geçmişi, kanıt replay ve bağımsız kurulum resolver sunar. Reçete schema 2 kesin CalibrationDependency bağlantısı taşır; yeni kalibrasyon sürümü/eski kurulum uyuşmazlığı reddedilir. Kaynak parametre değişimi bağlantıyı temizler; metadata korur. ProductionRecipeService Measurement/Webcam yapılandırma kabulünü destekler; fiziksel ölçüm doğruluğu ayrıca kabul edilmelidir.

## Planar Calibration ve mm Measurement

1. Recipe Editor'da tek Image File veya Webcam kaynağını hazırlayın; iki Edge'in image girişlerini aynı kaynağa bağlayın.
2. Calibration ekranında lens/kurulum ve çalışma düzlemi kimliğini girip **Kurulumu uygula**, ardından **Kare al** seçin. Boyut ve hover koordinatları edinilmiş gerçek kareden gelir.
3. Calibration ID ve yeni sürümü girin. **Örnek ekle** ile ilk teaching ve en az iki farklı holdout segmenti için ID/X1/Y1/X2/Y2/gerçek bilinen mm uzunluklarını doldurun. Varsayılan sahte örnek yoktur.
4. **Hesapla → Doğrula → Kaydet ve bağla**. Holdout doğrulandıktan sonra RMS görünür; reçete DRAFT kalır. Normal **Save** reçete bağlantısını kaydeder. **Kayıtlı kanıtı aç** save/reopen sonrası örnekleri yükler.
5. Editor'a dönüp Measurement ekleyin; p1/p2'yi iki Edge.point'e bağlayın, alt/üst mm toleransını ayarlayın ve Run Flow çalıştırın. Results ve çizgi overlay'i gerçek mm/PASS/FAIL gösterir; karar ham değerden hesaplanır. Calculate hâlâ px'dir.

Kurulum/kaynak/kalibrasyon sürümü değişince eski binding reddedilir; geçersiz/not-found/iptal edilmiş çevrim önceki mm veya PASS göstermez. Bu uniform scale ve tek ölçüm kararıdır; fiziksel lens/webcam kimliği otomatik doğrulanmaz, üretim izni veya final recipe kararı değildir. Lens/perspective/robot calibration henüz yoktur; Runtime aşağıdaki yapılandırma kabulüyle kullanılır. Katalog `<proje.vfproject>.calibrations.json` ayrı atomik dosyadır; proje ile birlikte taşınmalıdır. İki dosya ortak transaction değildir; kaydedilmemiş calibration tablo taslağı recipe değişiminde silinir.

## Runtime ve kalıcı Logs

Kaydedilmiş projeyi açın. Calibration binding ve Measurement toleranslarını hazırlayın; normal Save ile sürümü kaydedin. Recipe Manager’da reçeteyi tek tıkla seçip **Load Recipe** kullanın. Load başarıda Runtime’a geçer; editördeki taslağı değiştirmez. **Run** kaydedilmiş kopyayı otomatik kontrol edip immutable snapshot’ı hazırlar; görüntü edinmez. **Tek çevrim** gerçek acquisition ve yürütmeyi başlatır. Bu yapılandırma kabulüdür; fiziksel ölçüm doğruluğu kabulü değildir. Yerel Windows oturumu OS kimliğiyle Engineer olarak bu dört servis komutuna yetkilidir; Users/parola/Operator/Administrator rol yönetimi henüz yoktur.

Worker değişmez aktif snapshot'ı çalıştırır; taslak tolerans değişimi çalışan sürümü etkilemez. **Stop** eski ölçüm/PASS'i temizler. Başka sürüm yüklemek için **Stop → Load Recipe → Run** kullanın; rollback API’si servis katmanında korunur. Kaynak/kurulum/latest kalibrasyon değişirse yeni çevrim reddedilir veya yeni frame çözünürlüğünde Measurement ERROR olur.

Sonuç, kalıcı terminal kayıt yazıldıktan sonra gösterilir. Yazma/lock hatasında yeni çevrim engellenir; **Kaydı tekrar yaz** acquisition yapmadan aynı sonucu kaydeder. **Logs** son 128 terminal kaydı ve kimlik/birim/hata/karar detaylarını gösterir. Restart kayıtları korur fakat aktif izni geri yüklemez; yeniden Load Recipe/Run gerekir.

Arşiv `<proje.vfproject>.production` altındadır: değişmez snapshots, activation/rollback/CYCLE_BEGIN events ve cycle kayıtları; proje/catalog ile birlikte taşınır. Her kayıt atomik, append-only/hash kontrollü; toplam 512 MiB/10000 kayıt, otomatik retention yoktur. Görüntüler saklanmaz; manuel çevrim NOT_REQUESTED, socket çevrimleri ayrı PLC_RESULT/ACK kanıtıyla izlenir. Çok kullanıcı/cross-process lease, fiziksel cihaz kimliği, disk full/power loss ve taşınabilir paket kabulü açık. Ayrıntılar [üretim sözleşmesinde](docs/architecture/production-recipes.md).

## Camera Manager ve CLI kullanımı

Producer seç ile kurulu Windows x64 `.cti` seçin → Cihazları ara. Boş yolda GENICAM_GENTL64_PATH kullanılır. Liste gerçek producer cevabıdır. Seçili cihazı salt okunur aç/kapat kalıcı stream kurmaz, ayar değiştirmez. İşlemler worker'da; Stop cooperative. Producer discovery bütçesi 1000 ms; her liste güncellemesinin tarama beklemesi en çok 250 ms ve kalan toplam bütçenin yarısıdır (minimum 1 ms). Liste/metadata okuması ve sonraki interface için süre bırakılır; deadline sonrası sonuç reddedilir. DLL init/open-close hard timeout değildir.

```powershell
.\build\visionflow_camera_probe.exe
.\build\visionflow_camera_probe.exe --producer 'C:\VendorSDK\Producer.cti' --timeout 3000 --open-check
.\build\visionflow_camera_probe.exe --producer 'C:\VendorSDK\Producer.cti' --timeout 3000 --scan-wait 500
```

CLI JSON döndürür: NO_PRODUCER/ERROR exit 1, DONE exit 0; discoveryBudgetMs ve scanWaitCapMs ayrı raporlanır. Cihaz bulunmaması DONE olabilir; grab kabulü değildir. Önceki kurulum kaydı Basler pylon Runtime 26.09.2 ve Daheng Galaxy tam SDK 2.6.2608.9131 installer exit 0/reboot yok kanıtını içerir. Galaxy dar C ABI MinGW init/discovery/close 0/0/0, cihaz sayısı 0. 6B bütçe düzeltmesiyle gerçek Galaxy GigE producer hem 1000 hem 3000 ms toplam bütçede DONE/0 cihaz verdi. Bu oturumda önceki Basler Runtime yolu erişilebilir bulunmadı; Basler probe ERROR/eksik producer, güncel kurulum/keşif kabulü açık. Basler Developer SDK formu ve C API/header bağımlılığı da açık. Ayrıntılar ve kaynak/hash/günlükler [kurulum kaydında](docs/architecture/camera-sdk-installation.md). Kurulum öncesi açık oturumlar yeni Machine environment'ını miras almaz; gerekli SDK değişkenleri process scope'a alınmalıdır. `gentl_test_producer.cti` sentetik fixture olarak kalır. Bağlı kamera yok; trigger/stream/grab/exposure/gain/kamera reçete kaydı ve fiziksel kabul açık.

## Çekirdek sınırları

ToolRegistry yedi built-in tool'un default/port/sürüm/validation kaynağıdır; ToolExecutors algoritmaları engine sıralamasından ayırır. Startup compiled extension desteklenir; dinamik DLL plugin yoktur. EditorSession/CameraSession job ticket/future/cancel/cache/stale sonuç sahipliğini taşır; destructor worker'ları bekler.

ImageFrame owned buffer, stride, Gray8/16/BGR/BGRA/RGB/RGBA ve optional preview taşır. FrameMetadata source/frame/cycle/coordinate/time kökenini sonuçlara aktarır; unit/type/frame uyuşmazlığı engellenir. Güncel engine tek Image File veya Webcam kaynağı kullanır. CameraLifecycle owner-thread/generation, preview oldest-drop ve production FIFO Full kuyruğu sunar; mutlak monoton deadline ve shutdown gözlem bütçesi SDK interruption garantisi değildir. Controller stream SDK'ya henüz bağlı değildir.

A7 Pose2D/Transform2D/TrackingRoi ve CalibrationDependency schema 1 kayıt/geometri sözleşmesidir. Uniform planar scale hesabı ve evidence kaydı mevcut; dönmüş raster sampling, Match açı/Pose portu, lens/perspective/robot kalibrasyonu ve robot pose uygulanmadı. A8 ProductionRecipeService immutable snapshot, içerik/model/engine/tool/asset/calibration dependency ve varsayılan ret policy taşır. Cycle boyunca aktif sürüm değişmez; son 128 gerçek trace bellektedir. Disk VALID üretim izni olmaz. Runtime ve kalıcı arşiv/Logs bağlıdır; Users rol/oturum yönetimi sonraki iştir.

## A9 bağımsız çekirdek ve kaynak sürümleme

```powershell
cmake -S . -B build/core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/core -j 6
ctest --test-dir build/core --output-on-failure
.\scripts\validate-local.ps1
```

DLL PATH yukarıdaki gibi ayarlanır. Widgets OFF domain/application/engine, probe ve on dört core testi derler; Qt Core/Gui/Concurrent/Multimedia ve OpenCV sürer. BUILD_TESTING=OFF testleri çıkarır. UI ON iki ek `ui` etiketli test üretir. Script build/a9-core ve build/a9-ui altında Release configure/build/CTest yapar; QtRoot/OpenCVRoot/CompilerBin parametreleri vardır.

Yerel repo `C:/OpenCode/TVision/.git`, branch main; komşu TVis ayrı C++17 projesidir. Build, log/image verileri ve config/local.* Git dışında kalır; remote yayın/CI yoktur. Manifest/hash/boyut/atomiklik/recovery/sıralı migration [planlandı](docs/architecture/persistence-plan.md); root schema 1, recipe schema 1–2, editor schema 6 ve 16 MB sınırı korunur; schema 2 yalnız explicit calibration binding düzenlemesinde yazılır. Container/import/export/paket geçmişi 7'de kalır; ayrı production arşivi 6C'de uygulanmıştır.

Son kabul: build/runtime-core Release core 14/14 ve build/runtime-ui UI 16/16 CTest; önceki A9 script kabulü 9/9 ve 11/11 idi. ROI sentetik regression 100/100 doğru, performans CSV build altında; gerçek üretim latency/kurulum/CUDA/fiziksel kamera kabulü değildir.

7 Ekim 6C Runtime kabulü: standart Release `build/runtime-ui` CTest **16/16**, Widgets OFF `build/runtime-core` **14/14**. Runtime PASS/FAIL, kayıt bekliyor, invalidation, Logs ve 850×600 görselleri build altında incelendi. Yeni fiziksel webcam/industrial/PLC/robot doğrulaması yapılmadı.

## 6C-K1 — Chessboard test görüntüsü ve köşe inceleme

Güncel uygulama: `build/k1-ui/visionflow.exe` (P5/P6 binary). Calibration → **Test görüntüsü içe aktar (geçici)** → target ID, iç köşe sütun/satır ve gerçek kare kenarı mm → **Köşeleri bul**. PNG/JPEG/BMP; tam çözünürlüklü owned frame, dosya adı/decoded-pixel SHA256 ve Dosyadan test etiketi. Reçete açmadan da test importu kullanılabilir; reçetenin acquisition kaynağı/binding'i değişmez. **Kare al** mevcut reçete kaynağına döner.

Numaralı alt piksel köşeler, O/+X/+Y overlay ve orijinal px hover vardır. Orijin ilk tespit edilen köşedir; simetrik plakanın fiziksel yönü otomatik belirlenmez. İç köşe 3–32, toplam en çok 512; tespit sınırı 16 MP. Girilen mm görüntüden doğrulanamaz. Yeni import/target/kurulum/reçete veya kare değişimi eski köşeleri temizler. Stop geç sonucu bastırır; OpenCV çağrısını zorla kesmez. Test oturumu uniform scale kayıt/bağlama komutlarına girmez.

K1 köşe tespitidir; K2 H/mm denemesi ve K3 kayıt/Measurement/Runtime aşağıdaki akışla tamamlandı. Kaydedilmemiş test oturumu kapanınca silinir; Test kanıtını kaydet ile açıkça korunabilir. Bağımsız fiziksel kabul K4'te açıktır. OpenCV 5 yerel `objdetect`, `calib`, `geometry` bağlantıları doğrulandı; başka OpenCV sürümünün API uyumu doğrulanmadı. Release UI CTest 17/17, Widgets OFF 15/15; görseller/test ayarları build/k1-ui altındadır.

## 6C-K2/K3 — Tek görüntü H, bağımsız holdout ve Runtime

1. Calibration modelinde **Chessboard raw homography** seçin. Reçete kaynağında lens/çalışma düzlemini **Kurulumu uygula → Kare al** ile hazırlayın veya bağımsız **Test görüntüsü içe aktar** kullanın.
2. İç köşe grid ve gerçek kare mm → **Köşeleri bul → H hesapla**. Target-local orijin P1, +X sütun, +Y satır. H ve fit RMS mm/inverse RMS-max px gerçek fitten gelir; lens distortion düzeltilmez. Sabit inverse fit üst sınırı 1 px. H hatası scale fallback yapmaz.
3. Görüntüde iki noktaya tıklayın veya px alanlarına subpixel değer girin → **İki nokta mesafesini incele**. İki nokta world mm'ye ayrı dönüştürülür, mesafe sonra hesaplanır. Ölçüm iç köşe convex hull alanı içindedir; extrapolation reddedilir. Deneme validation veya PASS değildir.
4. Calibration ID/yeni sürüm/hata sınırı girin. Homography modunda tablonun **tüm satırları bağımsız holdout**; en az iki ayrı segment için Known mm gerçek bağımsız referanstan gelir. Fit köşelerine 0.5 px içinde kalan holdout uçları ve tekrarlı geometri reddedilir. **Doğrula** gerçek Measured/Residual mm ve RMS/max mm üretir. Örnek/ayar değişimi kabulü ve raporu temizler.
5. Edinilmiş kaynakta **Kaydet ve bağla → normal Save**. Mevcut Edge.point → Measurement mm/tolerans → ortak Runtime aynı resolver'ı kullanır. Typed model/version/content/kurulum ve yeni actual frame çözünürlüğü denetlenir; eski binding veya alan dışı nokta değer/PASS üretmez. Calculate px kalır. Runtime yapılandırma kabulü fiziksel ölçüm kabulü değildir.
6. **Dosyadan test** fiziksel kamera reçetesine bağlanamaz. **Test kanıtını kaydet/aç** `.vfcalstudy` içinde candidate veya accepted holdout/H kanıtını tutar; kaynak görüntü yolu ve decoded pixel hash'i korunmalıdır. Eksik/değişmiş resim reddedilir. Resim gömülmez/kopyalanmaz; bu henüz taşınabilir paket değildir.
7. **Kanıt raporunu dışa aktar (CSV)** gerçek model/H/fit/holdout/birim/köken ve her örneğin measured/residual değerlerini verir. Fiziksel referans kökenini veya üretim iznini belgelemez. [K4 fiziksel kabul prosedürü](docs/architecture/calibration-acceptance.md) ve boş gözlem tablosu gerçek plaka/bağımsız referansla tamamlanmalıdır.

Legacy uniform scale kayıtları/hash'leri korunur. Yeni raw homography model record ve dependency schema 2; recipe schema 2/root schema 1 değişmez. Execution engine version 5 eski validation iznini kullanmaz. Kalibrasyon/catalog/proje ayrı atomik commit; iptal zaten yazılmış kayıt etkisini geri almaz. Katalog 4 MiB/128 kayıt, çalışma dosyası 4 MiB. OpenCV 5 solver replay farklı sürüm/ABI için taşınabilirlik garantisi değildir.

Son yazılım kabulü: Release UI `build/k1-ui` CTest **19/19**, Widgets OFF `build/k1-core` **17/17**. Homography study/validated/Measurement/Runtime ve 850×600 görselleri build altında. Fiziksel doğruluk/KL lens ihtiyacı açık; bu oturumda PLC/robot işine geçilmedi.

## 6D.1 — TCP/IP socket ile PLC sonuç teslimi

K4 fiziksel kabul ertelendi. Güncel uygulama `build/k1-ui/visionflow.exe`. Communication’da **enabled**, yerel IP/port, ACK süresi/tekrar ve **autoStart** ayarlarını **Ayarları kaydet** ile yerel INI’ye kaydedin. Ayar yokken dinleme kapalıdır; autoStart yalnız dinlemeyi açar. Reçete tetik modu **SoftwareTrigger** ve Measurement sonuç kaynağını seçin, projeyi **Save** edin, **Load Recipe → Run** kullanın. Socket reçete olmadan da dinler; Load/Run/Stop bağlantıyı bozmaz. Manual modunda Tek çevrim çalışır, TCP yeni isteği INVALID alır.

```powershell
.\build\k1-ui\visionflow_plc_simulator.exe --port 5000
.\build\k1-ui\visionflow_plc_simulator.exe --port 5000 --duplicate --reconnect --ack-delay 3000
```

DLL PATH yerel derleme bölümündeki gibi ayarlanır. Port 0 seçilirse ekrandaki actual port kullanılır. Simülatör gerçek etkin Runtime çevrimini ister. vf.socket/2 JSON-LF TRIGGER/INVALID/BUSY/DONE/ACK; recipe/config/session/request kimlikleri doğrulanır. Seçilmiş Measurement gerçek mm/PASS/FAIL gönderir; mapping yok/ERROR/SKIPPED için null/NOT_EVALUATED. Genel recipe kararı veya robot pose yoktur.

Terminal cycle ve PLC_RESULT commit olmadan DONE, PLC_ACK commit olmadan Ready/ACKED verilmez. Kayıt hatasında **Kaydı tekrar yaz**, ACK hatasında aynı ACK tekrar edilir. Duplicate/reconnect aynı oturumda mevcut sonucu döndürür; timeout yalnız DONE tekrar gönderir. Logs kalıcı kanıtları ilişkilendirir. Salt listening Load/Run/Stop’u engellemez. Runtime Stop yeni kabulü kapatır; kayıt/ACK korunur. Pending kayıt/ACK varken transport stop/close ve yeni yükleme engellidir.

**Sınırlar:** tek peer/outstanding, en çok 128 accepted request/dinleme oturumu, eviction yok; ACK sonrası explicit stop/start yeni session açar. Restart kanıtı korur fakat dedup/outbox/teslim kurtarma henüz yoktur; sonraki iş **6D.2**. TLS/uzak kimlik/vendor wire/Modbus/OPC UA ve fiziksel PLC kabulü açık. Bu teslim K4 ölçüm doğruluğu veya üretim izni değildir. [Protokol ve simülatör seçenekleri](docs/architecture/plc-socket.md).

Release UI **20/20**, Widgets OFF **18/18** CTest. Gerçek localhost ve ayrı simülatörle duplicate/reconnect/geç ACK/tek cycle, gerçek mm PASS/FAIL, kayıt/ACK lock retry, cancel/error/schema/session sınandı. Normal/850×600 Communication ve Logs görselleri build/k1-ui altında incelendi; config/local.ui.ini değiştirilmedi.

## 8 Ekim — Güncel operatör arayüzü

Güncel binary: `build/operator-ui/visionflow.exe`. Run başarılıysa tam ekran Runtime ve kilitli navigasyon; Stop/durdurma tamamlanınca eski pencere ve menüler geri gelir. Setup → Communication / Calibration; bu iki sayfa artık sol menüde değildir. Communication için **TCP iletişimini etkinleştir → Ayarları kaydet → Dinlemeyi başlat**. Reçete/Run dinleme için gerekmez. Doğrudan manuel px/mm henüz uygulanmadı; [kalibrasyon planına](docs/architecture/calibration-plan.md) eklendi.
