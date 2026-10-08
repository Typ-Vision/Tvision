# C++ / Qt / OpenCV geliştirme gereksinimleri

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


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
