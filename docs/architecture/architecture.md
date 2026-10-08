# Mimari ve klasör sorumlulukları

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4 tamamlandı: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 P2 tamamlandı: Recipe Manager tek tıkla yalnız satır seçer; **Load Recipe / Edit Recipe** ayrıdır. Load kaydedilmiş bağımsız kopyayı statik kontrol eder; dirty durumda Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal sunulur. Runtime **Run / Stop / Tek çevrim** kullanır; Run görüntü veya parça sonucu üretmeden coordinator ile otomatik hazırlanır. Manuel çevrim, cooperative Stop ve aynı terminal kayıt retry coordinator üzerinden geçer. Seçili, editörde açık ve Runtime yüklü reçete ayrıdır. Eski validate/activate/rollback düğmeleri kaldırıldı; rollback servis API’si korunur. Bu P2 teslimi P3/P4 ile genişletildi; güncel socket ve Run Tool davranışı yukarıdadır. Fiziksel kabul/K4 ertelenmiş kalır.

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Ayrıntılı plan](calibration-plan.md).

## 7 Ekim 2026 onaylı hedef mimari

Güncel uygulama sırası ve kabul koşulları [development-plan.md](development-plan.md) içindedir. Önce 6A mimari güçlendirme, ardından 6B gerçek kamera, 6C Measurement/Runtime, 6C-K plaka tabanlı kamera/düzlem kalibrasyonu, 6D PLC, 6E robot guidance ve 7 ürünleştirme yapılacaktır. Aşağıdaki klasör tablosu mevcut uygulamayı anlatır; A3–A8 çekirdek bileşenleri uygulanmıştır; kalan hedefler plan durumundadır.

Hedef ayrım: mevcut Qt ekranları → application session/controller/komut servisleri → DAG engine → registry ile kayıtlı tool yürütücüleri ve hardware adapter'ları. Tool parametre/port sözleşmeleri domain'de; JSON/migration/varlık paketleme persistence sınırındadır. Kaynaktan bağımsız owned frame ve versioned koordinat/kalibrasyon bağları ortak sözleşmedir. Draft, doğrulanmış snapshot ve aktif Runtime sürümü ayrılır. Temel yetki, yapılandırılmış hata ve cycle logları ilk Runtime'a dahil edilir.

Statik ToolRegistry yeterlidir; dinamik DLL plugin şartı yoktur. UI kapalı core build/test desteklenecek. Kamera süreç izolasyonu gerçek SDK testlerinin sonucuna göre değerlendirilir. **Mevcut GUI görünümü, tema, panel düzeni, menüler ve navigasyon korunur; MainWindow ayrıştırması iç yapı değişikliğidir.**

## Mevcut klasör sorumlulukları

6A/A1–A8 çekirdek/sözleşme teslimleri tamamlandı: Run Match ve Run Flow aynı ExecutionService ve worker/Stop/stale-result yolunu kullanır. Seçili tool çalıştırması yalnız upstream alt grafiğini doğrular ve hesaplar; UI komutları kaynağı yeniden edinir. Servis değişmez frame snapshot ile karşılaştırılabilir yürütme sağlar. Referans cache upstream yapılandırması ve mevcut görüntü oturumuna bağlıdır. Domain referenceGeometry içerik kimliği, model yeniden öğretiminde Edge yürütmesini ve downstream ölçümü engeller; Search ROI yeniden öğretimi toparlar. Schema 5 kimliği kalıcıdır; legacy baseline migration dirty yapmaz. A3 registry/yürütücüler, A4 session controller ve A5 frame/metadata uygulanmıştır. A6 CameraLifecycle/generation, bounded preview/production kuyruğu, monoton Deadline ve shutdown budget çekirdeğidir; SDK stream bağlantısı 6B'de açık. A7 Coordinates/Pose2D/Transform2D/TrackingRoi ve CalibrationDependency çekirdeği uygulanmıştır; legacy Edge ortak dönüşümü kullanır. A8 ProductionRecipeService immutable snapshot/dependency/policy/cycle trace çekirdeği uygulanmıştır; A9 persistence/bağımsız build/test sözleşmesi de tamamlandı; koordinat ayrıntıları coordinates.md.

| Klasör | Sorumluluk |
| --- | --- |
| docs/requirements | Ürün, UI ve teknoloji gereksinimleri |
| docs/architecture | Katmanlar ve ileride veri modeli / plugin sözleşmesi |
| docs/performance | Matching kararı, benchmark sonuçları |
| prototypes | Orijinal HTML görsel referansı |
| src/app | Qt MainWindow, Project/Recipe Manager, ImageViewer, Toolbox/Flow/Properties; Camera Manager GenTL keşif ve bağlantı kontrolü; camera_probe CLI |
| src/domain | Project/Recipe/ToolDefinition, UUID, editor schema ve parametre doğrulaması; ToolGraph typed port/DAG; ToolRegistry; Coordinates frame/unit/axis, Pose2D/Transform2D/TrackingRoi ve CalibrationDependency JSON sözleşmesi |
| src/application | ProjectService, ImageFileService, AcquisitionService/adapter sözleşmesi, MatchService, ExecutionService; EditorSession/CameraSession worker/cache/cancel/teaching/shutdown budget; ToolExecutors, typed cycle/frame sonuçları; CameraLifecycle/Deadline/bounded frame queue; ProductionRecipeService/RuntimeService policy, immutable snapshot ve kalıcı kayıt; PlcRuntimeSocket TCP/handshake/ACK ve worker |
| src/vision | Kaynaktan bağımsız owned ImageFrame, private immutable raw veri/preview/metadata |
| src/vision/matching | OpenCV CPU template matching adapter; mevcut |
| src/hardware | GenTLTransport: dinamik producer C ABI, discovery ve read-only open-close; stream/grab ve robot adapter'ları planlanan; ilk PLC TCP servisi application katmanında |
| src/persistence | ProjectRepository atomik .vfproject JSON; gömülü model PNG ve typed bağlantılar; recipe schema 2 calibration binding; evidence kataloğu CalibrationService tarafından atomik kaydedilir |
| benchmarks | Bağımsız C++ latency benchmark |
| tests | Aşamaya göre entegrasyon / doğruluk kontrolleri |
| resources | Tema, ikonlar ve çeviriler |
| config | Versiyonlanmış varsayılanlar; makine ayarları local.* |
| data/recipes, models, calibrations | Geliştirme varlık alanları |
| scripts | İleride build/deploy yardımcıları |

Planlanan modüller uygulanmış özellik olarak değerlendirilmez. CMake hedefleri `visionflow`, `visionflow_projects` (domain/application/persistence, Qt Core), `visionflow_acquisition` (Qt Gui/Concurrent/Multimedia/Network/OpenCV), `visionflow_viewer` (Qt Widgets), `visionflow_matching`, `match_benchmark`, `project_service_test`, `editor_services_test`, `execution_test`, `acquisition_test`, `session_test`, `frame_test`, `camera_service_test`, `plc_socket_test`, `visionflow_plc_simulator`. Matcher Qt'den bağımsızdır; UI matcher çağırmaz. Kayıt sözleşmeleri `project-format.md` ve `editor-definition.md` ve `tool-execution.md` içinde.

HTML görsel/etkileşimli referanstır; demo sonuçları gerçek execution değildir. Native uygulamada ROI, typed flow bağlantıları ve panel resize uygulanmıştır; webcam tek kare edinimi mevcut, endüstriyel kamera grab ve Users rol/oturum yönetimi henüz yoktur; Runtime tek kare bağlıdır; A8 servis policy çekirdeği mevcuttur. HTML'in düğüm seçimi tam dinamik Properties uygulaması değildir. Güncel native kapsam README ve progress'te takip edilir.

## A9 bağımsız build ve kayıt tasarımı

Core/domain/application hedefleri UI seçeneğinden ayrıldı. Widgets yalnız UI ON iken aranır; QImage frame çekirdeği Qt Gui bağımlılığını korur. On dört core test ve camera/webcam probe Widgets OFF derlenir; editor_services/ui_shell_smoke UI ON altında kalır. BUILD_TESTING test üretimini ayrıca kontrol eder. Manifest/atomik container/migration [persistence-plan.md](persistence-plan.md) içinde tasarlandı; root schema 1/editor schema 6 kullanılır; explicit kalibrasyon binding için recipe schema 2 eklendi. Yerel repo TVision/.git; remote CI ve paket uygulaması yoktur.

## Webcam ve ilk 6C servis teslimi

WebcamAdapter Qt Multimedia kaynaklarını kendi worker thread'inde yönetir; ToolExecutors webcam source → owned frame → aynı ExecutionService DAG yoluna bağlar. MainWindow yalnız Properties komutları ve sonuç sunar. Kalıcı streaming/production camera lifecycle henüz bağlı değildir. MeasurementService uniform planar scale teaching/holdout, immutable calibration dependency/content, iki DONE Edge Point provenance ve ham mm tolerans kararını bağımsız application servisi olarak uygular. CalibrationService atomik versioned evidence/current resolver ve recipe schema 2 binding uygular; ProductionRecipeService kesin binding hash/eşleşmesi ve resolver hatasını DependencyChanged olarak denetler. CalibrationWorkflow edinilmiş frame ve ayrı planarSetup üzerinden current resolver kurar; Measurement DAG/Properties ve planar Calibration UI bağlıdır. RuntimeService OS yerel Engineer policy, validation/activation/immutable cycle/rollback ve ProductionStore atomik snapshot/event/cycle kayıtlarını sağlar; MainWindow RuntimeCoordinator ile Load/Run/Stop/manuel çevrim ve Logs sunar; ayrıntılı Users/audit/retention ve fiziksel üretim kabulü açıktır; Calculate UI px olarak kalır. Ayrıntılar [acquisition.md](acquisition.md) ve [measurement.md](measurement.md).
