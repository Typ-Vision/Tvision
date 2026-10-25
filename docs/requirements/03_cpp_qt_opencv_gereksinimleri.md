# C++ / Qt / OpenCV geliştirme gereksinimleri

Birleşik yolda UI application komutlarını çağırır; SDK/algoritma çağırmaz.
[Kaynak doğrulama](../architecture/source-verification.md) immutable worker
girdisi, owner accept ve gerçek drain ister; Run admission tüm kaynak kümesini
yeniden denetler. [Çıktı görünürlüğü](../architecture/output-visibility.md)
presentation katmanındadır; DAG/payload/terminal/Logs/CSV değişmez.
Tanı veya kayıt retry edinim/fit/PLC izni oluşturmaz.

Güncel uygulama durumu ve görev sırası [devir özetindedir](../progress.md). Bu belge gereksinimleri taşır; aşağıdaki tarihli planlar tek başına uygulama kabulü değildir. Üç belgede birebir tekrarlanan teslim paragrafları [taşıma öncesi tam kayıtta](../archive/2026-10-17/03_cpp_qt_opencv_gereksinimleri.md) korunmuştur. Yalnız görevle ilgili bölümleri okuyun; yeni teslim günlüğü eklemeyin.

ICM teknik hedefi: [ortak sözleşmeler](../architecture/industrial-camera-contract-plan.md)
marka bağımsız device/profile, owner worker/lease, owned frame ve trusted Runtime
kanıtını tanımlar. OECS C API kararı COE-U kapsamındadır; diğer SDK/ABI kararları ayrıdır.
21 Ekim [deneysel politika](../architecture/industrial-camera-oecs-experimental-plan.md)
bu ortak yolda ayrı capability/provenance ve kayıt ayrımı ister; ActualCounterEpoch
ve trusted completion taklit edilmez. Test adaptörü normal ürün yolundan seçilemez.
Plan uygulama değildir; varsayım native production capability açmaz.

16 Ekim — **KMC-0–9 A yazılım kabulü tamamlandı; B teslimi açık:** [KMC sözleşmesi](../architecture/independent-calibration-implementation-plan.md)
kalıcı camera/setup kimliğini recipe tool kimliğinden ayırır; application-owned
registry, dosya kökeni, kurulum başına immutable snapshot ve B tesliminde
source/frame bazlı çoklu completion doğrulaması öngörür. Tek-source scoped completion KMC-7b ile uygulandı. Sabit 5 kamera sınırı yoktur;
N=0/1/2/5/8/10 ve kapasite retleri test edilir. Legacy schema/hash ve gerçek
kabul kapıları korunur; Qt UI algoritma/SDK çağırmaz.

15 Ekim — **KUI-5 tamamlandı**: SystemCalibration immutable owner snapshot/typed execution binding ve sealed readiness; source tool ile shared setup ayrımı, gerçek pointwise H/geometri, selective raw cache/TargetOnly0 ve trusted Runtime completion. Execution23, Edge Point3, Circle3, terminal5; root/legacy codec/no-op korunur. Legacy wrapper3 accepted model-only kanıtını korur, raster hash/frame uydurmaz. [Sözleşme](../architecture/system-calibration.md). Sıradaki KUI-6 birleşik Release. Alttaki kayıtlar tarihseldir.

15 Ekim — **KUI-4 tamamlandı**: Qt Widgets yalnız presentation/session komutları; acquisition/algoritma application servislerinde worker snapshotlarıyla yürür. Named record wrapper schema2 ve strict bounded target library schema1 eklendi; legacy codec/engine execution/22 korunur. [Release ve UI kabulü](../architecture/calibration-ui-acceptance.md). KUI-5 ortak resolver/immutable Runtime henüz açıktır; ardından KUI-6. Alttaki kayıtlar tarihseldir.

15 Ekim — **KUI-2/KUI-3 tamamlandı**: [application-owned calibration session](../architecture/calibration-wizard-services.md), immutable worker girdileri ve opaque tek kullanımlık completion, gerçek Edge/Caliper ve OpenCV 5 features/objdetect circle adapter. Yeni target descriptor schema1 ve circle nested PlaneMapping schema2; legacy chessboard codec/hash korunur. UI algoritma/SDK çağırmaz; UI KUI-4, ortak metrik/immutable Runtime KUI-5 kapsamındadır. Engine execution/22 ve mevcut production izinleri değişmedi.

11 Ekim teknik gereksinim ve R1 durumu — [Ortak Referans R1–R8](../architecture/reference-system-plan.md): Point/Pose2D tipleri korunur, ofset sırası/birim/eksen domain katmanında doğrulanır; uygulama servisi güncel sonuç/cache/kalibrasyon ve ROI çözümlemesini yapar. UI algoritma/SDK çağırmaz. Yedi tüketici, üç applicationType, eski strict codec/hash/no-op ve immutable production kabulü kapsamda; Point'ten sahte angle=0 Pose2D üretilmez. GPT-6.1-sol / medium devir paketleri planın 6–7. bölümlerindedir. R1 ReferenceBinding domain matematiği ve bağımsız strict codec uygulandı; yeni kayıt henüz reçeteye bağlı değildir. Engine/editor/tool/production izinleri değişmedi; sonraki R2, son kabul T8.4.2b.3c.

9 Ekim T0.3/T1.1/T1.2 uygulandı: isteğe bağlı offline static ZXing reader capability, bounded typed port codec, execution/7 ve editor 7 opt-in dependency kimlikleri. Root/recipe/calibration sürümleri ve yedi built-in korunur. [Güncel sözleşme](../architecture/tool-values.md). Alttaki ürüne bağlanma/capability henüz yok ifadeleri tarihsel T0.2 kaydıdır; sonraki T1.3, yeni code tool T3, lens T15.

9 Ekim **T0.2**: ZXing 2.3.0 static reader-only MinGW C++20 Release, sabit commit/kaynak SHA-256/lisans ve ayrı Zint encoder'ından sekiz fixture/33 kontrol ile kabul edildi. Yalnız `build/deps/` prefix'i ve bağımsız probe vardır; ürüne bağlanma/capability T0.3'te, code tool T3'tedir. Sistem OpenCV/Qt/compiler kurulumu değişmedi. [Gerçek seçenekler, komutlar ve sınırlar](../toolchain.md#t02--zxing-reader-only-bağımlılık-kabulü); aşağıdaki bağımlılık başlangıç planı bu kayıtla güncellenmiştir.

9 Ekim [tool algoritma/bağımlılık planı](../architecture/recipe-tools-algorithms.md): yerel OpenCV 5.0.0 core/imgproc/imgcodecs/objdetect/calib/geometry kullanımı; geometri API'lerinin 5.0 modül ayrımı T0 compile/link/run ile doğrulanacak. Geniş kod okuma için sabitlenmiş ZXing-C++, OCR için Tesseract/Leptonica/tessdata adapter planlandı; yönlü subpixel sampler, robust circle metrologi, tracking, bead ve karar için özel C++ kod gerekecek. Henüz yeni dependency kurulmadı veya runtime'a bağlanmadı. Lens T15 en son; CUDA/dnn/text modülleri var sayılmaz. [Ana paket planı](../architecture/recipe-tools-plan.md) Sol 6.1 medium için küçük alt teslimler tanımlar.

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Güncel kalibrasyon gereksinimleri ve kabul planı](../architecture/calibration-plan.md). Bu revizyon önceki geliştirme sırası ifadelerine göre önceliklidir.

## 7 Ekim 2026 — Onaylı mimari güçlendirme

Güncel sıra ve kabul koşulları [development-plan.md](../architecture/development-plan.md) içindedir. C++20/Qt Widgets/OpenCV ve tek editor/Runtime kararları korunur; GUI görünümü ve menüler değişmez. 6A'da tek tool/tam flow ortak yürütme, versioned referans invalidation, statik ToolRegistry, ayrı yürütücüler, application session/controller ve kaynaktan bağımsız owned frame uygulanacaktır. Koordinat/Pose2D, kamera lifecycle/deadline/kuyruk ve üretim snapshot sözleşmeleri tanımlanacaktır.

Çekirdek servisler/testler Widgets uygulaması kapalıyken derlenebilmeli; Qt Core bağımlılığı korunabilir. Parametre doğrulama ve migration sınırları açık, Windows Release CI/toolchain tekrarlanabilir olmalıdır. Kaynak sürümleme konumu doğrulanacak; yoksa yerel Git kurulacaktır. Dinamik DLL plugin ve ayrı kamera süreci zorunlu başlangıç koşulu değildir.

6B fiziksel kamera; 6C Measurement/kalibrasyon/tolerans/Runtime ile servis yetkisi, yapılandırılmış hata ve cycle logları; 6D PLC; 6E robot; 7 ayrıntılı diagnostics/audit/retention ve dağıtım sırasıdır. Bu hedefler uygulanmış sayılmaz; aşağıdaki aşama listesi genel ürün kapsamıdır.

Tarih: 7 Ekim 2026. Bu belge ilk iki ürün/UI belgesini tamamlar; onların kapsamını değiştirmez.

## Teknoloji ve derleme

- Windows x64; C++20, CMake >= 3.24, Ninja, Release performans ölçümü.
- Native Qt 6 Widgets: QMainWindow, dock/split paneller, model/view tablolar, dinamik properties. HTML referans olarak kullanılacak; WebEngine zorunlu değil.
- OpenCV: core, imgproc, imgcodecs başlangıç modülleri. Kamera edinimi ilk aşamada image-file adapter; kamera SDK/videoio daha sonra capability kontrolüyle eklenir.
- Mevcut yerel araçlar Qt 6.12.0 / MinGW 13.1 / OpenCV 5.0.0. Bu sürümler yerel derleme için kullanılır; üretim sürümleri CI ve kamera SDK uyumluluğu doğrulandıktan sonra sabitlenir.
- Qt ve OpenCV aynı compiler/architecture/ABI ile bağlanmalıdır. MSVC kamera SDK gerekirse tüm native bağımlılıklar aynı MSVC toolchain ile tekrar hazırlanır.
- CUDA isteğe bağlı backend; GPU varlığı tek başına yeterli değildir. CUDA toolkit, uyumlu compiler ve OpenCV cudaimgproc/cudaarithm derlemesi gerekir. Mevcut iskelet CPU kullanır.
- Qt/OpenCV ve kamera SDK lisansları dağıtım aşamasında bağımlılık envanterine kaydedilir.

## Mimari sözleşmeler

UI → Application Services → Vision Engine → Tool adapter / Hardware. UI doğrudan OpenCV veya robot SDK çağırmaz. Görüntü işlemleri UI thread dışında yürütülür; Qt signals/slots ile sonuç bildirilir. Frame ömrü açıkça yönetilir, cv::Mat paylaşımları üzerinde eşzamanlı yazma yapılmaz. Qt görüntüsü oluşturulurken stride, renk düzeni ve tampon ömrü korunur.

Tek Recipe Editor ve tek Runtime bulunur. `applicationType`: `robot_guidance`, `defect_inspection`, `measurement`; yalnızca başlangıç flow'u ve sonuç sunumu değişir.

Tool bağlantıları kalıcı tool ID / port ID ile saklanır. Typed port'lar Image, Region, Point, Line, Circle, Pose2D, Pose3D, Number, Boolean; birim/frame/cycle ID taşır. DAG doğrulaması eksik referans, tip uyuşmazlığı ve döngüyü engeller. Recipe DRAFT/VALID/INVALID, execution READY/BUSY/DONE/ERROR/SKIPPED ve part PASS/FAIL ayrı alanlardır.

Reçete JSON metadata + modeller/maskeler/kalibrasyon varlıklarıyla versiyonlanır; şema sürümü, recipe sürümü ve engine/plugin sürümü ayrı tutulur. Atomik kayıt, import doğrulaması, kalıcı ID'ler ve dirty/result invalidation gerekir. HTML demo JSON üretim şeması kabul edilmez.

Robot/PLC sonuç gönderimi ile robot hareketi ayrı servis komutlarıdır. Geçersiz kalibrasyon veya match durumunda geçerli pose yayımlanmaz. Rol kontrolü servis katmanında uygulanır.

## Matching ve performans

CPU başlangıç adapter: `cv::matchTemplate` + `TM_CCOEFF_NORMED` + `cv::minMaxLoc`. Bu adapter sabit açı/ölçek template aramasıdır; ürünün tam Pattern/Shape Match kapsamını uygulamaz. Angle/scale araması, teaching, çoklu sonuç/NMS, tracking ROI ve result filtering mask sonraki geliştirmelerdir. Ham skor olasılık değildir; yüzdeye dönüştürme yöntem sözleşmesine göre yapılır.

5 MP girişte match hedefi p95 <10 ms, sıkı hedef p95 <5 ms. Bu hedef kamera ediniminden ayrı ölçülür; ayrıca tüm cycle süresi raporlanır. Tam kare ile ROI araması ayrı sonuçlardır. Hedefin sağlandığı ancak hedef donanım ve gerçek veri üzerinde doğrulamayla beyan edilir. Ayrıntılar: `../performance/opencv_matching.md`.

## Aşamalar ve kabul

1. Main Window/navigasyon, Project Manager, Recipe Manager ve üç tür seçimi.
2. Ortak Recipe Editor, image-file acquisition, Image Viewer, Toolbox/Properties ve ROI.
3. Match teaching/adaptör, Blob, Edge, Calculate; typed flow ve save/reopen.
4. Kalibrasyon, kamera/robot/PLC adapter'ları, Runtime, diagnostics, yetki, audit/retention.

Her aşama derlenir. Kabul kontrolleri ilk iki dokümandaki kriterleri kapsar: tek editor, türün kayıtta korunması, seçim/overlay senkronizasyonu, invalidation, tutarlı cycle/frame/birim ve çalışmamış reçetede sahte PASS olmaması. Matching için gerçek pozitif/negatif veriyle yanlış kabul, konum hatası ve latency birlikte ölçülür.
