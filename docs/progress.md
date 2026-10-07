# VisionFlow ilerleme ve oturum devri

Son güncelleme: 7 Ekim 2026. **Mevcut durum: Adım 5, 6.1–6.2 ve 6A/A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı. Sıradaki iş: 6B gerçek producer/SDK ve tek kamera entegrasyonu. Fiziksel kamera kabulü açık.** Güncel sıra [geliştirme planında](architecture/development-plan.md), iş listesi `backlog.md` içindedir. Mevcut GUI görünümü, menüler ve yerleşim korunur. Aşağıdaki geçmiş teslimlerdeki “sonraki adım” ifadeleri bu yeni sırayı geçersiz kılmaz.

## Onaylanan geliştirme sırası

| Adım | Kapsam | Durum |
| --- | --- | --- |
| Hazırlık | Belgeler, klasörler, toolchain, CPU matcher ve benchmark | Tamamlandı |
| 1 | Qt Main Window, koyu tema, navigasyon, ayarlanabilir paneller | Tamamlandı |
| 2 | Proje ve reçete oluştur/aç/kaydet, üç tür, sürüm/DRAFT | Tamamlandı |
| 3 | Ortak editor, gerçek görüntü yükleme, zoom/pan, tool seçimi | Tamamlandı |
| Ek iş | Son açık proje/reçete/ekranı otomatik geri yükleme | Tamamlandı |
| 4 | ROI çizimi, template teaching, gerçek match, overlay/doğruluk | Tamamlandı |
| 5 | Typed tool altyapısı, DAG, background yürütme, Blob/Edge/Calculate | Tamamlandı |
| 6.1–6.2 | Acquisition sözleşmesi, GenTL keşif/read-only open-close | Tamamlandı; fiziksel kamera/grab kabulü yok |
| 6A | Mimari güçlendirme A1–A9 | A1–A9 çekirdek/sözleşme ve plan tamamlandı |
| 6B | Tek gerçek kamera ve mevcut DAG entegrasyonu | 6A sonrasında; fiziksel kabul gerekli |
| 6C | Measurement, kalibrasyon, tolerans, doğrulanmış üretim sürümü ve Runtime | Bekliyor; temel servis yetkisi/hata kodları/cycle logları dahil |
| 6D | PLC handshake ve sonuç teslimi | Bekliyor |
| 6E | Robot dönüşümü, pick reference ve guidance | Bekliyor |
| 7 | Ayrıntılı diagnostics/users/loglar, audit/retention, paketleme ve kurulum | Bekliyor; temel yetki/log altyapısı 6C'ye çekildi |

İlk dosya tabanlı uçtan uca hedef tamamlandı. Yeni üretim hedefi: gerçek kamera → mevcut flow → kalibrasyonlu Measurement → tolerans → doğrulanmış reçete → ortak Runtime. Fiziksel kamera yoksa dosya kaynağıyla bağımsız servis işleri ilerler; kamera kabulü açık kalır.

## 7 Ekim 2026 — 6A/A9 persistence planı, bağımsız build ve Git

- Kapsam: Widgets OFF iken domain/application/engine/probe ve dokuz core test derlenir. Qt Core/Gui/Concurrent korunur; viewer ve iki UI testi yalnız UI ON. BUILD_TESTING bağımsız kontrol; UI testleri ui etiketi. Mevcut format/atomik commit/şemalar korunur.
- Kayıt planı: persistence-plan.md manifest SHA-256/length/role, byte/decode bütçeleri, path/entry kontrolü, tek container staging/verify/commit/recovery ve sıralı N→N+1 migration kararlarını tanımlar. Yeni paket loader/import/export/geçmiş uygulaması değildir; 6C/7'de tamamlanır.
- Değişen dosyalar: CMakeLists.txt, .gitignore, tests/project_service_test.cpp, scripts/validate-local.ps1; README, backlog, progress, architecture/development-plan/project-format/architecture, toolchain ve yeni persistence-plan.md. UI ve üretim config/local.ui.ini değişmedi.
- Doğrulama: Release build/core 9/9, build 11/11; validate-local.ps1 temiz build/a9-core 9/9 ve build/a9-ui 11/11 geçti. Ek persistence testleri uncommitted staged write/cancel, oversized save/load ve önceki commit/session korunmasını sınar. Mevcut schema/model migration/roundtrip, eksik/bozuk model ve dependency değişimi regression geçti. Core build.ninja içinde Widgets/ImageViewer/visionflow.exe yok. Sentetik ROI CSV 100/100 doğru; ilk run median 7.15 ms/p95 7.48 ms, gerçek üretim hız kabulü değildir.
- Görsel: güncel build/step5-editor.png ve build/step6-camera-test.png incelendi; 399 px overlay, tema/menüler/paneller ve sentetik read-only kamera ekranı korunur. Test ayarları/görseller build altındadır.
- Kaynak sürümleme: TVision/üst dizinlerde depo yoktu; C:/OpenCode/TVis ayrı C++17 tvis deposudur. C:/OpenCode/TVision/.git main yerel deposu ve baseline kaynak kaydı oluşturuldu; remote yayın yok. Build/data images/log ve local INI ignore edilir.
- Sınırlamalar: Qt Gui core bağımlılığı sürer. Uzak CI runner kurulmadı; disk full/power loss/crash recovery ve yeni container entry kabulü henüz yok. Manifest hash imza/üretim yetkisi değildir. Kalıcı production snapshot/audit/log ve Runtime henüz bağlı değil. Fiziksel grab/SDK, native DPI, kurulum/CUDA kabulü açık.
- Sonraki: **6B** kurulu gerçek producer/SDK/model/ABI envanteri; GenApi/vendor erişimi, trigger/owned frame → DAG entegrasyonu ve disconnect/Stop/recovery kabulü. Donanım bulunmazsa fiziksel kabul açık tutulur, bağımsız 6C kalibrasyon/Measurement servislerine ilerlenir. GUI korunur.

## 7 Ekim 2026 — 6A/A8 draft/doğrulama/aktif snapshot çekirdeği

- Kapsam: ProductionRecipeService owner-thread çekirdeği; Recipe taslağından ayrı servis kaynaklı validation UUID/UTC/content/dependency hash ve const ProductionSnapshot. İçerik/revizyon/project-directory, model/ROI, engine/registry tool sürümü, resolver asset SHA-256 ve CalibrationDependency manifestine bağlanır. Disk VALID izin değildir. Yapısal scope execution_configuration; gerçek üretim/kalibrasyon/tolerans kabulü değildir.
- Aktivasyon/rollback: güvenilir injectable policy, varsayılan ret; Validate/Activate/Rollback/Run ayrı servis yetkileri. Yalnız aynı servis ömründeki validation kayıtları kabul edilir. Aktif cycle boyunca snapshot değişmez; Activate/Rollback/yeni cycle Busy. Sonraki taslak validate edilebilir. Rollback geçmiş etkin sürüm ve güncel bağımlılıkları kontrol eder. Worker completion recipe/config/frame/cycle/ticket kontrolü ve duplicate reddi; abandon ERROR kaydı.
- Trace/geçmiş: varsayılan 16 snapshot, 1–256 limit; dolunca HistoryFull, otomatik snapshot silme yok. Definition 16 MB, asset manifest 256; toplam byte arşivi sonraki adım. Son 128 transient cycle JSON trace gerçek tool sonuçlarını ve recipe/version/engine/model/calibration/source/frame kökenini taşır; image raw loglanmaz, iletişim NOT_REQUESTED, PASS/pose yok.
- Değişenler: yeni src/application/ProductionRecipeService.hpp/.cpp ve tests/production_recipe_test.cpp; ExecutionService.hpp engine version; MatchService.hpp/.cpp ortak model validation decoder; CMakeLists.txt. README, backlog, progress, architecture/development-plan/project-format/tool-execution ve yeni production-recipes.md güncellendi. Root/recipe/editor schema ve UI kaynakları değişmedi.
- Doğrulama: Release build ve CTest **11/11 geçti**. Yeni production_recipe_contract üç applicationType ortak servis yolu, gerçek PNG/DAG ve numeric trace, kayıp edinim dosyasında ERROR/parça kararı ayrımı, draft izolasyonu, activation boundary/rollback, yetkisiz/forged/restart validation, değişmiş içerik/model/asset/engine/calibration, corrupt/missing model, disabled/missing DAG input, owner-thread, yabancı/stale/duplicate cycle result, history/log sınırı ve abandon ERROR kabulünü kapsar. Önceki Match/teaching/metadata/session/GenTL/UI regression korundu.
- Görsel: güncel build/step5-editor.png ve build/step6-camera-test.png incelendi; 399 px measurement overlay, tema/menüler/paneller ve sentetik discovery/read-only ekranı korunur. Test ayarları/fixture'lar build altındadır; üretim config/local.ui.ini kullanılmadı.
- Sınırlamalar: Runtime UI/controller, Users kimlik doğrulama, kalıcı snapshot/audit/cycle logları henüz bağlı değil. Dış image edinim girdisi değişkendir; Validate dosyayı açmaz, gerçek edinim cycle'da hata verebilir. Asset resolver/policy güvenilir application boundary'dir. CalibrationDependency kaydı fiziksel katsayı/doğruluk kabulü değildir. Kalibrasyon hesabı/mm/tolerans/part decision 6C; fiziksel kamera 6B açık. Native DPI, kurulum ve CUDA kabulü yok.
- Sonraki: **6A/A9** manifest/varlık bütünlüğü ve persistence/migration planı, Widgets kapalı bağımsız core build/test; Git konumunu doğrula, yoksa yerel depo kur. Mevcut atomik commit/şemalar korunacak; gerçek paket/geçmiş ve Runtime entegrasyonu 6C/7.

## 7 Ekim 2026 — 6A/A7 koordinat/tracking ROI çekirdeği

- Kapsam: domain CoordinateFrame, Point2D, Pose2D, rigid Transform2D; source→target yönü, inverse/then sırası, radyan, px/mm, Y-down/Y-up ve integer pixel-centre konvansiyonu. Araçtan bağımsız TrackingRoi teaching/current pose ile lokal dört köşeyi dönüştürür; uyumsuz frame/unit/axis ve dejenere/sonlu olmayan geometri reddedilir. Rigid dönüşüm px→mm/axis reflection yapmaz.
- Entegrasyon/migration: Edge referenceOrigin/current point ortak poseTransform/inverse/then yolunu kullanır; delta bir kez yuvarlanır, kutu boyutu korunur, sınır dışı ERROR olur. Engine tüketiminde gerçek PortValue type/unit/frame beklenen input descriptor ile kontrol edilir. Editor schema 5/root-recipe schema 1 değişmedi; schema 3/4 migration, referenceGeometry invalidation, no-op/dirty ve save/reopen korunur.
- Kalibrasyon sözleşmesi: CalibrationDependency kimlik/sürüm/içerik/kamera/lens/çözünürlük/düzlem/source-target frame alanlarını taşır; birebir current dependency kontrolü ve strict schema 1 JSON roundtrip vardır. Eksik/ekstra/future/tip/birim hataları reddedilir. Reçete kalibrasyon kaydı, katsayı/kanıt ve execution invalidation 6C işidir; kayıt tek başına kalibrasyon veya pose sonucu değildir.
- Değişen dosyalar: src/domain/Coordinates.hpp/.cpp yeni; src/application/ToolExecutors.cpp, ExecutionService.cpp; tests/coordinates_test.cpp yeni, frame_test.cpp; CMakeLists.txt. README, backlog, progress, architecture/development-plan/tool-execution/matching-editor/editor-definition ve yeni coordinates.md güncellendi. UI kaynakları/menüler/tema ve üretim config/local.ui.ini değişmedi.
- Doğrulama: Release configure/build başarılı; CTest **10/10 geçti**. coordinates_contract bilinen öteleme/π/2 dönme/sıra/inverse, pose açı bileşimi, mm/Y-up, lokal offset ve bütün köşeler, nonzero teaching roundtrip, frame/unit/axis, NaN/Inf, dejenere/crossed ROI, yarım piksel ve sınır kabulünü kontrol eder. CalibrationDependency değişen her bağımlılığı ve bozuk/future JSON'u reddeder. owned_frame_contract yanlış unit/type/port-frame runtime ölçüm engelini kontrol eder; mevcut tool_execution ve UI teaching/migration/invalidation/reopen regression geçti.
- Görsel: build/a7-reference-edge.png ve build/a7-editor.png güncel offscreen smoke görsellerinden kopyalanıp incelendi; Edge kutusu/kenar ve Calculate çizgi/px overlay, tema/menüler/paneller korunur. Test ayarları/görseller build altındadır; üretim INI kullanılmadı.
- Sınırlamalar: Genel dönmüş ROI/Pose2D domain çekirdeğidir; editor Pose portu, Match açı/ölçek ölçümü, dönmüş Edge/Blob raster sampling, kalibrasyon hesabı/activation ve robot çıktısı uygulanmadı. Genel ROI JSON reçeteye eklenmedi; legacy kayıt korunur. Physical grab kabulü 6B, kalibrasyon bağımlılığının gerçek sonuç invalidation'ı 6C, robot frame/TCP/pick reference 6E'dedir. 2D'den tam 3D pose çıkarılmaz. Sentetik fixture; native DPI/kurulum/CUDA kabulü yok.
- Sonraki adım: **6A/A8** draft/doğrulama/aktif üretim değişmez snapshot sözleşmesi, engine/tool/model/kalibrasyon bağımlılığı ve aktivasyon/rollback/cycle sınırı; gerçek Runtime ve yetki/log entegrasyonu 6C. Mevcut GUI korunur.

## 7 Ekim 2026 — 6A/A2 model/geometri referans invalidation teslimi

- Kapsam: Edge referenceGeometry SHA-256 kimliği kaynak UUID/kind ve Match modelRoi/modelPng içeriğine bağlıdır. Model merkezi, boyutu veya PNG değişince engine Edge ERROR ve downstream ölçüm SKIPPED üretir; eski kutu/point/line yayımlanmaz. Search ROI yeniden çizimi mevcut referans merkezi ve kimliğiyle ilişkiyi günceller. İsim/açıklama, enabled, Min Score ve Match search ayarı kimliği değiştirmez. Yeni source bağlantısı eski kimliği otomatik onaylamaz; no-op bağlantı değişimi invalidation'ı temizlemez.
- Kayıt: Editor schema 5; root/recipe schema 1 korunur. Schema 1–4 açılırken eski kutu/model/UUID/origin korunur, mevcut kaynak başlangıç kimliği olur. Açma/no-op dirty yapmaz; ilk gerçek değişiklik schema 5 yazar. Mevcut legacy modelin geçmişte değişip değişmediği kanıtlanamaz. Eksik schema 5 kimliği yürütmede yeniden öğretim ister. Invalid durum save/reopen boyunca kimlik uyuşmazlığıyla korunur.
- Değişen kaynaklar: src/domain/ToolDefinition.hpp/.cpp, ToolGraph.cpp; application/ProjectService.cpp, ExecutionService.cpp; app/MainWindowEditor.cpp, MainWindowMatching.cpp, EditorSmoke.cpp; tests/execution_test.cpp, editor_services_test.cpp. README, backlog, progress ve development-plan/architecture/editor-definition/matching-editor/tool-execution belgeleri güncellendi.
- Doğrulama: Release build başarılı; CTest **6/6 geçti**. Core model merkezi ve aynı merkezde yeni model içeriği değişimi, metadata/search/threshold no-invalidation, stale output temizliği, downstream engeli, save/reopen, no-op bağlantı, schema 3/4 migration ve eksik kimlik kabulünü kontrol eder. UI gerçek model teaching worker, kaydet/aç, ERROR, Search ROI yeniden öğretim ve DONE toparlanmasını doğrular.
- Görsel kontrol: build/a2-reference-invalid.png ve build/a2-reference-restored.png incelendi. Eski kutu/kenar bastırılır; yeniden öğretimde doğru mavi kutu/yeşil kenar geri gelir. Tema, menüler ve panel yerleşimi korunur; mevcut Properties içinde açıklama eklenir. Üretim config/local.ui.ini kullanılmadı.
- Sınırlamalar: Sentetik fixture; fiziksel kamera, native DPI, kurulum/CUDA kabulü yok. Legacy geçmiş referans değişimi saptanamaz. Match dışı point kaynaklarında parametre kimliği konservatif kullanılır; genel tracking/kalibrasyon sürümleri A7/6C kapsamında. Kimlik içerik temellidir: birebir eski model/geometriye dönüş aynı ilişkiyi yeniden geçerli kılar. Öğretim UI orchestration'ının application session'a ayrılması A4'te kalır.
- Sonraki adım: **6A/A3** statik ToolRegistry, ayrı tool yürütücüleri, ortak parametre doğrulama; mevcut port, execution, GUI ve regression davranışlarını koru.

## 7 Ekim 2026 — 6A/A1 Run Match / Run Flow ortak yürütme teslimi

- Kapsam: Run Match, Run Flow ile aynı ExecutionService ve UI worker yaşam döngüsüne taşındı. Seçili tool ve transitif upstream alt grafiği çözülür; ilgisiz dallar çalıştırılmaz. Bağlı Image/Region, disabled/missing/invalid/error upstream, not-found, Stop ve stale-result kuralları ortaktır. Her iki UI komutu kaynağı yeniden edinir; önizleme/cache execution girdisi olarak kullanılmaz.
- Snapshot: Servis aynı immutable ImageFrame ile tam/tek tool yürütmesini destekler; yeni cycle/frame kimlikleri atanır. UI her çağrıda yeni edinim yapar. Referans öğretim cache kimliği proje/reçete/path/frame generation ve upstream tool/bağlantı yapılandırmasını kapsar; bağımsız Edge ayarı güncel match merkezini korur.
- Değişen kaynaklar: `src/application/ExecutionService.hpp/.cpp`, `src/app/MainWindow.hpp`, `MainWindowMatching.cpp`, `MainWindowExecution.cpp`, `EditorSmoke.cpp`, `tests/execution_test.cpp`. Belgeler: README, backlog, progress, architecture, development-plan, tool-execution, matching-editor. Şema ve yerel üretim INI ayarları değişmedi.
- Doğrulama: Release derleme başarılı; son CTest **6/6 geçti**. Core aynı frame/config üzerinde bağlı ROI'nin yerel ROI önüne geçmesini ve skor/konum eşitliğini; upstream disabled/missing/port/cycle/adapter error, ilgisiz eksik dal, not-found ve cancellation çıktı temizliğini kontrol eder. UI gerçek input combo ile image bağlantısı, önizleme var ama image bağlantısı yokken hata, dosya dışarıda değişince yeni edinim, Stop, close guard, stale config ve save/reopen kabulünü doğrular. Mevcut reference tracking regression korunur.
- Görsel kontrol: `build/a1-match.png` (son smoke `step4-editor.png` kopyası) ve `build/reference-edge.png` incelendi. Menü, tema, düğme konumları ve paneller korunur; Run Match Results artık cycle/frame ve typed tool çıktıları gösterir. Üretim `config/local.ui.ini` kullanılmadı.
- Sınırlamalar: Test görüntüleri sentetiktir; gerçek kamera/grab, native DPI, kurulum ve CUDA kabulü yoktur. Stop cooperative; devam eden OpenCV/decode zorla kesilmez. Reçete format doğrulaması tüm definition için kalır; yalnız graph çalıştırılabilirlik doğrulaması seçili alt grafiğe sınırlanır. Model yeniden öğretiminde bağlı Edge geometri invalidation henüz yoktur; A2 açık kalır. Yerel Git deposu bulunmadı; kaynak sürümleme kararı A9'da izlenir.
- Sonraki adım: **6A/A2** model/geometri kimliğine bağlı referans invalidation, yeniden öğretimde Edge/downstream engeli, save/reopen ve eski şema migration kabulü. Mevcut GUI korunacak.

## 7 Ekim 2026 — Genel inceleme sonrası plan revizyonu

- Kapsam: Kullanıcı dokuz mimari önerinin tamamını plana aldı; GUI görünümü ve menü değişikliği istemedi. Yalnız dokümantasyon güncellendi; kaynak kodu, tema, menü, CMake veya üretim ayarları değiştirilmedi.
- Yeni belge: `architecture/development-plan.md`; A1–A9 iş paketleri, 6A–6E sırası, çıkış koşulları, gerçek donanım kabulü ve GUI koruma şartı.
- Güncellenen belgeler: `README.md`, `docs/backlog.md`, `docs/progress.md`, `docs/architecture/architecture.md`, `tool-execution.md`, `matching-editor.md`, `editor-definition.md`, `acquisition.md`, `project-format.md` ve üç `docs/requirements/` belgesi.
- Doğrulama: Yerel Markdown bağlantıları ve plan referansları kontrol edildi. Yalnız belge değişikliği olduğundan yeniden build/CTest/görsel kabul gerekmedi. Önceki incelemede mevcut Release build güncel ve CTest 6/6 idi; bu sonuç planlanan refactor'ların kabulü değildir.
- Sınırlamalar: Bütün yeni öneriler plan durumundadır. Mevcut Run Match ayrılığı, model yeniden öğretimindeki referans riski, tek kaynak/frame yaklaşımı ve cooperative timeout henüz düzeltilmedi. Fiziksel producer/grab, kurulum ve CUDA kabulü yoktur.
- Sonraki adım: 6A/A1 için aynı snapshot üzerinde tek tool/tam flow giriş çözümlemesini birleştirmek; ilgili regression kabulü ardından A2'ye geçmek. Mevcut GUI korunacak.

## Hazırlıkta tamamlananlar

Ürün/UI belgeleri `docs/requirements/`, HTML `prototypes/` altında. CMake, MinGW, Qt ve OpenCV yerel derleme doğrulandı. CPU adapter `cv::matchTemplate` / `TM_CCOEFF_NORMED`; CUDA adayı belgelendi, etkin değil. Sentetik 5 MP tam kare CPU <10 ms hedefini sağlamadı; 512×512 ROI p95 yaklaşık 6–9 ms. Ayrıntılar ve CSV `docs/performance/` altında. Benchmarkı UI adımında tekrar çalıştırmak gerekmez.

## Adım 1 teslimi

- `src/app/MainWindow.hpp/.cpp`: Qt Main Window, Workspace/System navigasyonu, Dashboard ve sonraki modüller için açıkça belirtilmiş boş durum ekranları.
- Tek Recipe Editor yerleşim önizlemesi Recipe Manager üzerinden veya View menüsünden açılır. Üç uygulama türü ayrı navigasyon sayfaları değildir.
- Editor yatay splitter: Toolbox / Image Viewer / Properties. Dikey splitter: editor / Flow, Results, Log, Variables, Communication, Diagnostics sekmeleri.
- View menüsünde panel düzenini sıfırlama; File menüsünde Exit. Pencere geometrisi ve panel boyutları QSettings INI ile kapanışta kaydedilir.
- `resources/theme.qss`: HTML renklerine göre koyu tema; Qt resource olarak executable içine gömülür.
- `src/app/main.cpp`: Fusion stil, kaynak tema yükleme, izole offscreen smoke modu.
- `scripts/run-local.ps1`: yerel DLL yollarıyla çalıştırma. `AGENTS.md`: yeni oturumda bu dosyadan devam etme talimatı.

### Doğrulama

Release CMake/Ninja derlemesi başarılı. `ctest --test-dir build --output-on-failure`: 1/1 geçti. Test tüm sayfa geçişlerini ve splitter boyutlarının dosyaya kayıt/restore davranışını kontrol eder. Test ayarları `build/ui-smoke.ini`; üretim ayarlarını değiştirmez. Offscreen editor görüntüsü `build/step1-ui.png` incelendi; tema, Türkçe karakterler ve panel yerleşimi doğrulandı. Offscreen font enumeration için test modunda Windows Segoe UI fontu yüklenir.

Native pencere üzerinde gerçek mouse sürükleme / yüksek DPI / çoklu monitör doğrulaması ayrıca kullanıcı denemesi gerektirir; offscreen test bunları kanıtlamaz. CMake isteğe bağlı Vulkan header uyarısı Widgets derlemesini engellemiyor.

### Sınırlar ve kararlar

Adım 1 tesliminde Project/Recipe Manager boş durum ekranıydı; Adım 2 ile create/edit/open/save eklendi. Editor tool etkileşimleri, gerçek image viewer, flow bağlantıları, yetki ve hardware servisleri henüz uygulanmadı. Runtime sahte PASS göstermez.

Geliştirme çalıştırmasında panel dosyası executable'ın `../config/local.ui.ini` yolundadır. Dağıtım aşamasında kullanıcı başına writable app-data yoluna geçirilmeli. Minimum pencere 850×600; daha küçük ekran için responsive düzen sonraki UI iyileştirmesidir.

## Adım 2 başlangıç planı (gerçekleştirildi)

1. Domain Project/Recipe modellerini kalıcı ID, `schemaVersion`, `recipeVersion`, `applicationType`, DRAFT/VALID/INVALID alanlarıyla oluştur.
2. Application/persistence servisleriyle proje ve reçete create/open/save; atomik kayıt ve hatada değişiklik koruması ekle.
3. Project Manager formu ve Recipe Manager tablosu, tür filtresi ve New Recipe tür seçimini uygula.
4. Yeni reçete DRAFT olsun ve aynı editor'e açılsın; aktif proje/reçete bağlamını başlıkta göster.
5. Save/reopen, tür/ID/sürüm korunması ve hatalı dosya davranışlarını doğrula; progress dosyasını güncelle.

HTML demo JSON üretim şeması değildir. Adım 2 gerçek vision veya hardware çalıştırmaya genişletilmemeli; editor interaction Adım 3'te yapılacak.

## Adım 2 teslimi

- Project/Recipe domain modelleri; kalıcı UUID, bağımsız schemaVersion/recipeVersion, üç applicationType, DRAFT/VALID/INVALID.
- `ProjectService`: proje/reçete oluşturma, metadata düzenleme, aktif reçete, dirty takibi, save/discard. Reçete türü düzenleme sırasında kilitli; yeni reçete DRAFT, yeniden adlandırmada ID korunur.
- `ProjectRepository`: tek UTF-8 `.vfproject` içinde tüm reçeteler; QSaveFile atomik commit, 16 MB sınırı, şema/tip/ID/isim doğrulaması. Lock ve harici değişiklik kontrolü; hatada eski dosya/bellek/sürüm korunur.
- Project Manager: New/Open/Save, proje bilgileri ve Cameras/Recipes/Calibration/Communication/Variables/Logs ağacı. Reçete ağacından ortak editor'e geçiş.
- Recipe Manager: tablo, arama/tür filtresi, New Recipe üç kartı, metadata düzenleme ve Open Editor. Başlık/Editor/Dashboard aktif proje ve reçeteyi gösterir.
- File → New/Open/Save ve Ctrl+N/O/S. Proje değiştirme/kapatmada Save/Discard/Cancel; Save tüm pending reçeteleri birlikte kaydeder.
- Mimari ve dosya sözleşmesi `docs/architecture/project-format.md` içinde; UI davranışı README'de.

### Doğrulama

Release CMake/Ninja derlemesi başarılı. CTest **2/2 geçti**: `project_persistence` ve genişletilmiş `ui_shell_smoke`. Core test üç tür, UUID/sürüm, validation, discard, hatalı açma/kayıt ve harici değişiklik davranışlarını doğrular. UI testi gerçek diyalog/düğmelerle create/edit/save/reopen, filtre ve editor bağlamını kontrol eder. `scripts/run-local.ps1 -SmokeTest` geçti.

`build/step2-new-project.png`, `step2-new-recipe.png`, `step2-recipes.png`, `step2-editor.png` offscreen görselleri incelendi. Test dosyası `build/ui-workspace.vfproject`; gerçek kullanıcı projesi veya aktif üretim ayarları testler tarafından kullanılmaz.

### Kalan kapsam

Copy/Delete, bağımsız recipe import/export, Save As, schema migration ve geçmiş revizyon snapshot'ları henüz yok. Kaynak/kalibrasyon metadata referansları doğrulanmış cihaz/kalibrasyon değildir. Oturum/rol, gerçek vision, hardware, gerçek tool/flow ve runtime yürütmesi sonraki adımlarda. Native dosya seçici ve Save/Discard/Cancel tıklamaları ile yüksek DPI ayrıca manuel denenmeli.

## Adım 3 başlangıç planı (gerçekleştirildi)

1. Mevcut ProjectService/domain/schema'yı koru; UI'dan doğrudan OpenCV/dosya işlemi ekleme.
2. Image-file acquisition servisi ve gerçek image viewer: Open Image, Fit, Zoom, Pan; dosya okuma hatası ve görüntü/piksel koordinatlarını göster.
3. Tek Recipe Editor'de Toolbox, seçili tool bağlamı ve Properties senkronizasyonunu uygula. İlk tool kayıt sözleşmesini minimal ve ileride typed flow'a genişletilebilir tasarla.
4. Görüntü kaynağı/tool ayar değişiklikleri dirty/DRAFT yapsın; save/reopen yapılandırmayı korusun. Recipe `definition` ayrılmış alanını kullanırken sözleşmeyi belgele; şema değişikliği gerekiyorsa açık migration kararı ver.
5. Gerçek ROI çizimi ve match teaching Adım 4'te kalır; gerçek tool sonuçları yokken sahte PASS/score gösterme.
6. Yeni modüllere uygun test/görsel doğrulama yap, README/mimari/progress kaydını güncelle.

## Adım 3 teslimi

- `ImageFileService` ve private raw verili `ImageFrame`: Unicode dosya yolundan gerçek OpenCV decode; Gray8/Gray16/RGB8/RGBA8, preview deep-copy ve orijinal piksel metni.
- `ImageViewer`: Qt Graphics View; Fit/+/-/1:1/wheel zoom, mouse pan, image piksel koordinatları; kaynak selection border. View dönüşümleri reçete verisini değiştirmez.
- `MainWindowEditor.cpp`: gerçek Open Image/Reload toolbar, Toolbox arama/Add/double-click, Flow tool listesi ve dinamik Properties. Flow/image kaynak seçimi Properties ile aynı kalıcı ID'yi kullanır. Escape seçimi temizler.
- `ToolDefinition` ve ProjectService add/update/remove/setImageFile: kalıcı UUID/name/enabled/kind/parameters; DRAFT/dirty/no-op/save revision davranışı korundu.
- `definition.editor.schemaVersion=1`, eski Adım 2 dosyaları root/recipe schema 1 ile uyumlu. Editor dışı definition alanları korunur; ayrıntı `docs/architecture/editor-definition.md` içinde.
- Görüntü yolu proje klasörüne relative kaydedilir; asset kopyalama/paketleme yok. Reçete tekrar seçilince kayıtlı görüntü yeniden açılır; source disabled veya başka reçetede eski preview temizlenir.
- Bozuk yeni dosya önceki kaynağı bozmaz. Bozuk kayıtlı kaynak Reload sırasında stale preview göstermez; yeniden sağlam dosya geldiğinde Reload toparlar. Numeric/text validation hatası Save'i engeller.
- `scripts/run-local.ps1` OpenCV DLL yolunu da hazırlar.

### Doğrulama

Release derleme başarılı; **CTest 3/3 geçti**: project_persistence, ui_shell_smoke ve editor_services. `scripts/run-local.ps1 -SmokeTest` geçti. Testler gerçek decode edilen sentetik 5 MP PNG, Unicode yol, renk/alpha/16-bit/stride, ham piksel değerleri, bozuk/eksik/unsupported format, tool/source ID ve parametre roundtrip, dirty/revision/discard, zoom/pan ve koordinatlar ile görüntü izolasyonu/reload recovery'yi doğrular.

`build/step3-editor.png` incelendi. Properties panelindeki ID/path alanları yatay taşmayı önlemek için readonly QLineEdit olarak sunuldu. Görüntü verisi `build/görüntü_ölçüm.png`; testlerde oluşturulmuş sentetik örnektir, kamera görüntüsü değildir. Matching benchmark bu UI adımında tekrar çalıştırılmadı.

### Kalan kapsam / kararlar

Image File gerçek edinim yapar; Template Match, Blob, Edge, Calculate şu an yalnız parametre configuration tool'larıdır. Flow typed graph değildir. ROI çizimi/teaching, match yürütme/contour/angle, measurement sonuçları, Undo/Redo, kamera SDK, trigger, authorization ve engine sonuç/cycle sözleşmesi henüz yok. Source/preview decode senkrondur; background engine sonraki aşamada yapılacak. Harici dosya değişimi otomatik izlenmez; Reload gerekir. Görüntüler proje içine gömülmez.

## Adım 4 başlangıç planı (gerçekleştirildi)

1. AGENTS/progress ve editor-definition sözleşmesini oku; mevcut üç adımı yeniden oluşturma.
2. ImageViewer üzerinde model ROI ve search ROI çizme/seçme/düzenleme ekle; görüntü koordinatında clamp/validation, seçili tool ile overlay bağını koru.
3. ImageFrame ham verisini UI'ya açmadan vision/application servisleri üzerinden match adapter'e bağla. Gray conversion/16-bit stratejisini açıkça tanımla.
4. Model teaching → template oluşturma/kaydetme → gerçek cv::matchTemplate → skor/konum/overlay zincirini uygula. Timeout/çoklu match/angle-scale kapsamını desteklenmediği halde etkinmiş gibi gösterme.
5. Editor schema 1'in yeni ROI/model parametreleri için migration kararını belgele; eski dosyaların ID ve minScore değerleri kaybolmasın. Model varlığı kaydı ile atomik project commit arasındaki tutarlılığı tasarla.
6. Frame/config değişiminde eski match sonucunu geçersiz kıl. Reçete VALID yapmak için gerçek doğrulama olmadan status yükseltme; Runtime'da sahte sonuç gösterme.
7. Gerçek pozitif/negatif fixture'larla konum/skor, missing model/image, ROI sınırları ve save/reopen doğrulaması yap. Dar ROI performansını ölç; tam 5 MP <10 ms sağlandığını CPU için iddia etme.
8. README, mimari ve bu ilerleme dosyasını güncelle. Genel typed tool/DAG ve Blob/Edge/Calculate yürütmesi Adım 5'te kalır.

## Son proje otomatik açılışı ve Adım 4 teslimi

- Kullanıcının ek isteği iş listesine kaydedildi: normal kapanışta son kayıtlı proje yolu, aktif reçete ve sayfa yerel INI'ye yazılır; sonraki açılışta restore edilir. Eksik/bozuk dosyada nonmodal hata, manuel Open Project. Autosave/crash recovery yok; smoke üretim ayarlarını kullanmaz.
- `MainWindow.cpp/main.cpp`: session restore, ayrı pencereyle restore testleri; `docs/backlog.md` kalıcı iş listesi.
- `ImageViewer`: image koordinatında model/search çizimi, ters yönde sürükleme ve image sınır clamp, sarı/mavi ROI, yeşil/kırmızı aday. Escape iptal; yeniden çizme düzenleme sağlar.
- `MatchService.hpp/.cpp`: private ham ImageFrame → grayscale → teaching PNG → gerçek TM_CCOEFF_NORMED/minMaxLoc. Gray16 sabit aralık dönüşümü; flat model, eksik/bozuk model, boyut ve ROI doğrulaması.
- `MainWindowMatching.cpp`: QtConcurrent teaching/match, immutable snapshot, stale-result reddi, sonuç/config/frame invalidation. Worker sırasında yeniden Run/ROI/kapanış engellenir. Skor/konum/süre Results'ta; DRAFT ve Runtime/part kararı değişmez.
- Editor schema 2; schema 1 ID/minScore korunarak okunur. Açma/no-op dirty yapmaz; ilk gerçek tool değişiminde migration yazılır. PNG project JSON içinde aynı atomik commit; model en fazla 1024×1024/4 MB base64, toplam proje 16 MB.
- `MainWindowEditor`, `EditorSmoke`, `editor_services_test`, CMake/QtConcurrent, README ve mimari güncellendi. Güncel ayrıntılar `architecture/matching-editor.md`.

### Doğrulama

Release derleme ve CTest **3/3 geçti**. UI smoke session proje/reçete/sayfa restore, missing last project, gerçek viewport teaching gesture, save/reopen/run, worker sırasında kapanış ve stale-result reddini doğrular. Core testler 5 MP gerçek decode hattında sentetik pozitif/taşınmış model/negatif, Gray16, corrupt/missing/flat model, küçük/dışarı taşan ROI, migration/embedded PNG roundtrip, reverse ROI ve sınır clamp'i kapsar. Ters yönde ROI çizimi testi sınır hesabındaki hatayı yakaladı; düzeltildi. `build/step4-editor.png` incelendi: Results ve overlay/Properties uygun.

Dar ROI servis ölçümü: 2592×1944 Gray8, 64×48 model, 512×512 search, 10 warmup +100 ölçüm; 100/100 doğru konum. Median 7.1516 ms, p95 7.4798 ms. `docs/performance/roi_match_service.csv`; scope PNG decode/hazırlık + match/minMaxLoc, acquisition/UI/worker queue hariç. RGB/Gray16 hazırlığı daha uzun olabilir. Tam kare <10 ms iddiası yok; üretim verisi henüz yok.

### Sınırlamalar

Yalnız sabit açı/ölçek ve tek aday; tutamaç/UndoRedo/mask/angle/scale/NMS yok. Teaching/match background; dosya acquisition senkron. Genel typed flow/cycle/result modeli yok. Test fixture'ları sentetik; gerçek kamera false accept/reject ve hedef latency ayrıca ölçülmeli. Dışarıdaki image dosyaları otomatik izlenmez. Native yüksek DPI/çoklu monitör ayrıca kullanıcı doğrulaması gerektirir.

## Adım 5 başlangıç planı (gerçekleştirildi)

1. AGENTS/progress/backlog ve matching-editor sözleşmesini oku; tamamlanan işleri yeniden oluşturma.
2. Typed port/tool/domain sözleşmesini Image/Region/Point/Number/Boolean ve px/frame/cycle birimleriyle tanımla. Mevcut UUID ve modelleri koruyacak açık editor migration tasarla.
3. DAG validation: missing tool/port, type/birim uyumsuzluğu, cycle; Flow UI üzerinde gerçek bağlantıları yapılandır.
4. Genel background execution servisi kur; acquisition, match ve Blob/Edge/Calculate adapter'larını UI'dan bağımsız yürüt. READY/BUSY/DONE/ERROR/SKIPPED, recipe durumundan ve PASS/FAIL'den ayrı olsun.
5. Sonuçlar frame/config/cycle kimliğine bağlı olsun; disabled tool, hata, iptal ve stale result davranışını koru. Otomatik proje restore özelliği korunmalı.
6. Gerçek Blob/Edge/Calculate girdileri ve sonuç/overlay; deterministic fixture, invalid graph, yürütme hatası, save/reopen ve UI kabulü.
7. README/mimari/backlog/progress güncelle. Hardware/Runtime Adım 6; üretim kamera matching doğrulaması ayrı kabul işi olarak kalır.


## Adım 5 teslimi

- `domain/ToolGraph.hpp/.cpp`: Image/Region/Point/Number/Boolean port kataloğu, birim/image-frame sözleşmesi, kalıcı bağlantılar, required input/reference/type/unit/frame/duplicate/cycle kontrolü ve topolojik sıra.
- `ToolDefinition`, `ProjectService`: editor schema 3, bağlantı düzenleme/dirty/DRAFT; schema 1/2 migration UUID/model/minScore korunması ve eski image bağımlılıkları. No-op/açma dirty yapmaz. Dangling bağlantılı draft kaydedilebilir; execution reddedilir.
- `application/ExecutionService.hpp/.cpp`: immutable DAG snapshot, her çevrimde gerçek file acquisition, Match/Blob/Edge/Calculate adapter'ları; tool ve cycle state/elapsed, UUID cycle/frame, config hash ve typed validity. Disabled/error/not-found upstream SKIPPED; bağımsız dallar çalışır. Stop cooperative; iptal edilen çıktı yayımlanmaz.
- `MainWindowExecution.cpp`, `MainWindowEditor/Matching/Workspace`, `MainWindow.hpp/main.cpp`: input binding seçicileri, Flow bağlantı özeti/Validate/state-time listesi, Run Flow/Stop, scroll Results, seçili tool overlay ve stale hash kontrolü. Open Image/Reload/restore acquisition worker'a taşındı. Worker boyunca kapanış engellenir; session restore korundu.
- `ImageViewer.hpp/.cpp`: Blob bounding boxes/centroid, Edge point, Calculate nokta/mesafe çizgisi; Blob/Edge search ROI drawing.
- `tests/execution_test.cpp`, `EditorSmoke.cpp`, mevcut editor testindeki future-schema sınırı ve CMake; README/backlog/mimari güncellendi. Yeni sözleşme `architecture/tool-execution.md`.

### Doğrulama

Release CMake/Ninja başarılı; CTest **4/4 geçti**: project_persistence, editor_services, ui_shell_smoke, tool_execution. Mevcut matching/ROI/model/session regression testleri korundu. Yeni core fixture 1 Blob/1200 px2, dört scan yönü, x=20/59 noktaları ve39 px distance; match region→Blob300 px2, branch error isolation, not-found/disabled/invalid model/ROI/acquisition, invalid DAG, UUID/cycle coherence, iptal, schema2 migration, rename/delete ve save/reopen kapsamını doğrular. UI gerçek combo bağlantılarıyla399 px measurement, DRAFT korunması, background/close guard, stale config result rejection, Stop/yeniden run ve bağlantı persistence'ını doğrular.

`build/step5-editor.png`, `build/step5-blob.png`, `build/step5-flow.png` offscreen görselleri incelendi. Results seçili output'u önce gösterir; Properties typed referansları ve overlay doğru. Test ayarları/görüntü/projeler build altında; üretim local.ui.ini değiştirilmedi. Sentetik fixture kabulü üretim kamera/false accept-reject veya CUDA/kurulum doğrulaması değildir.

### Bilinen sınırlamalar

Tek dosya kaynağı, image koordinatı ve pixel birimleri. Edge ortalama profile/ilk abs contrast/piksel merkezi; subpixel/Rising-Falling/filtre yok. Blob threshold/8-connectivity/en büyük feature; contour/aday filtreleri/kabul kararları yok. Calculate yalnız Öklid distance; mm kalibrasyon sonra. Flow liste/bağlantı özeti; node canvas/drag bağlantı/UndoRedo yok. Stop adapter sınırında kontrol edilir, devam eden OpenCV çağrısını kesmez; hard timeout yok. Sonuçlar transient; recipe VALID ve part PASS/FAIL üretilmez. Native high-DPI/çoklu monitör ve gerçek üretim verisi ayrıca doğrulanmalıdır.

## Sonraki oturumda Adım 6

1. Progress/backlog/README ve tool-execution sözleşmesini oku; Adım 5'i yeniden oluşturma.
2. Kamera adapter capability/SDK/toolchain uyumunu mevcut donanım bilgisiyle belirle; UI SDK çağırmasın. Cihaz bilgisi eksikse gerekli kullanıcı bilgisini iste.
3. Frame/trigger/cycle sözleşmesini koruyan kamera acquisition ve hata/timeout davranışını uygula.
4. Kalibrasyon model/validity/frame/unit ve veri kabul sözleşmesini oluştur; geçersiz kalibrasyonda mm/robot pose yayımlama.
5. Robot/PLC bağlantı/output handshake servisleri ve ortak Runtime'ı aşamalı uygula; robot hareketi ile sonuç gönderimini ayrı tut.
6. Test/görsel/README/mimari/progress kanıtlarını güncelle. Kurulum/yetki/loglar Adım 7; üretim matching kabulü ayrıca devam eder.

## 7 Ekim 2026 — tool silme ve overlay düzeltmesi

Adım 6 öncesi kullanıcı önceliği tamamlandı. ProjectService setTools silinen tool ile ilgili gelen/giden bağlantıları aynı definition güncellemesinde temizler. Bağımlı gerekli girişler yeniden bağlanmalıdır. UI ekle/sil/yeniden Run ve core bağlı tool silme regression testleri eklendi.

Edge ROI boyunca yeşil profile konum çizgisi gösterir; contour değildir. Calculate P1/P2, sarı çizgi ve px mesafe etiketi gösterir. Nokta/metin zoom boyutundan bağımsızdır. İptal/stale overlay temizliği korunur. ProjectService, ExecutionService, ImageViewer, MainWindowExecution, EditorSmoke ve execution_test değişti.

Release derleme ve CTest 4/4 geçti. Edge çizgisi/measurement koordinatları, referans temizliği ve kalan DAG recovery doğrulandı. build/step5-editor.png ve build/fix-edge.png incelendi; 399 px sarı çizgi ve yeşil kenar görünür. Sentetik fixture; native DPI ayrıca doğrulanmalı. Sıradaki Adım 6 planı korunur.

## 7 Ekim 2026 — Match merkezine bağlı Edge arama takibi

Kullanıcının major referans hatası düzeltildi; Adım 6 öncesi önceliklendirildi. Edge optional reference Point/px/image portu eklendi. Öğretilen Search ROI ile referenceOrigin merkezi birlikte saklanır. Runtime kutu = taughtROI + round(currentReference - taughtReference); kutu boyutu ve merkezle offset ilişkisi korunur. Mevcut cycle/Run Match merkezi kullanılır; güncel görüntüdeki match cache parametre/görüntü değişimlerine karşı kontrol edilir. Sonuç yoksa model öğretim merkezi başlangıç noktasıdır; hareketli görüntü üzerinde önce Run Flow/Match çalıştırılmalıdır. Edge parametre değişimi sonrası aynı görüntü üzerinde ROI yeniden çizimi güncel merkeze bağlanır.

Editor schema 4; schema 1/2/3 okunur. Eski schema 3 Match.region → Edge.region, Match.point → Edge.reference olarak migrate edilir; teaching model merkezi anchor olur, çizilmiş kutu/UUID/model korunur. Eski kayıtta Search ROI yoksa çizmek gerekir. Yeni UI Match.region yerine Referans merkezi alanından Match.point bağlar; region/reference aynı anda bağlanamaz. Runtime kullanılan kutu mavi ROI ile gösterilir. Reference missing/disabled/error → Edge ve measurement SKIPPED. Taşınmış ROI görüntü dışında veya referans modunda kutu eksikse ERROR; tam kareye sessiz fallback yok. Cancel gerçek searchRegion dahil overlay temizler.

Değişenler: ToolGraph/ToolDefinition, ExecutionService, MainWindowEditor/Matching/Execution.hpp/.cpp, EditorSmoke, execution_test/editor_services_test; README/backlog/tool-execution sözleşmesi. Yeni port/schema future rejection sınırı 5.

Doğrulama: Release build ve CTest 4/4 geçti. Core ayrı model ile uzak kenar fixture: +30/+15 ve -10/-10 px hareket, kutu boyutu/offset korunması, gerçek taşınmış Edge noktası, iki kenar arasında invariant 39 px measurement, save/reopen, referans bulunamaması, görüntü dışı kutu ve eksik teaching ROI. Schema 3 bağlantı/anchor migration çalıştırılarak doğrulandı. UI gerçek input combo, ROI teaching, +30/+20 px hareket, Edge noktasının (230,209.5) olması ve moved frame üzerinde property değişimi ardından yeniden teaching doğrulandı. Run Match cache geometric center ayrıca regression testinde kontrol edildi. build/reference-edge.png incelendi: mavi kutu ve yeşil kenar parçayla taşınıyor; Properties combo minimum içerik boyutu dar panele uyarlandı; dar panelde yatay kaydırma hâlâ kullanılabilir. Son combo yerleşim değişikliği sonrası ui_shell_smoke tekrar geçti.

Sınırlar: translation X/Y ve px; açı/ölçek/kalibrasyon henüz yok. Model yeniden öğretildiyse Edge referans ilişkisini kutuyu yeniden çizerek öğretin. Sentetik fixture üretim kamera kabulü değildir. Devam: Adım 6 planı korunur.


## 7 Ekim 2026 — Adım 6.1 acquisition altyapısı

Kapsam: Kullanıcı kamera mimarisi seçimini geliştirmeye bıraktı. GenICam/GenTL ortak model, gerektiğinde Basler pylon/Daheng Galaxy adapter kararı verildi. AcquisitionService request/result/capability sözleşmesi ve gerçek ImageFileAdapter eklendi; Run Flow bağlandı. Source/cycle/frame kimliği, UTC completion timestamp, unsupported trigger, ERROR/CANCELLED/TIMEOUT durumlarında frame bastırma, geç frame reddi ve Stop token korundu. Editor schema 4 ve UI değişmedi.

Değişen dosyalar: src/application/AcquisitionService.hpp/.cpp (yeni), ExecutionService.hpp/.cpp, tests/acquisition_test.cpp (yeni), CMakeLists.txt, README.md, docs/architecture/acquisition.md (yeni), architecture.md, backlog.md, progress.md.

Doğrulama: CMake cache Release; derleme başarılı. CTest 5/5 geçti: ui_shell_smoke, project_persistence, editor_services, tool_execution, acquisition_contract. Yeni test gerçek PNG/piksel, kimlik/UTC, capability, unsupported trigger, invalid request, pre/post cancellation, late frame/error, empty/missing frame ve recovery kapsar. Test varlıkları build altında; üretim ayarları kullanılmadı. UI değişikliği olmadığı için yeni görsel kabul gerekmedi.

Sınırlar: blocking decode zorla kesilmez; timeout geç çıktıyı bastırır. Run Flow dosya edinim sınırı 5000 ms; Editor Open/Reload önceki worker decode yolunda kalır. Kamera adapter ve GenTL consumer henüz uygulanmadı; standart üst düzey klasör taramasında SDK bulunmadı. Fiziksel kamera, MinGW/vendor ABI, kurulum ve kamera latency doğrulanmadı. Kalibrasyon/robot-PLC/Runtime bekliyor; sahte PASS/pose yok.

Sonraki adım: gerçek SDK/GenTL producer tespiti ve ABI kontrolü; discovery/open/close/trigger/grab, owned buffer conversion, disconnect/timeout acceptance. Sonra kalibrasyon ve ortak Runtime. Adım 6 bütünü tamamlanmış değildir.


## 7 Ekim 2026 — Adım 6.2 GenTL discovery / read-only open-close

Kapsam: resmi EMVA GenTL 1.6 C function-pointer ABI ile dinamik .cti yükleme, interface/device discovery, marka/model/seri/transport, salt okunur cihaz aç/kapat, RAII handle/library temizliği. CameraService producer arama/isolation/timeout/cancel sözleşmesi. Camera Manager gerçek worker arama/Stop ve seçili cihaz bağlantı kontrolü; camera_probe JSON CLI. Eski kamera placeholder yerine uygulanmış keşif/bağlantı ekranı var; canlı grab olarak sunulmaz. Process içinde tek oturum; ikinci oturum bloklanmadan reddedilir.

Değişenler: src/hardware/GenTLTransport.hpp/.cpp; src/application/CameraService.hpp/.cpp; src/app/MainWindowCamera.cpp ve camera_probe.cpp; MainWindow.hpp/.cpp, main.cpp; CMakeLists.txt; tests/camera_service_test.cpp, gentl_test_producer.cpp, gentl_incomplete_producer.cpp; third_party/gentl/GenTL.h (değişmeden resmi header), README.md/license.txt; README ve acquisition/architecture/backlog/progress belgeleri.

Doğrulama: Release build başarılı; CTest 6/6 geçti. camera_transport gerçek test DLL C ABI çağrılarıyla metadata/discovery/open-close, ctor/destructor cleanup, missing export/file/invalid binary, invalid size/type/null handle, access denial, empty interface, optional metadata, late timeout/pre-post cancellation, producer failure isolation, concurrent-session rejection ve recovery kontrol eder. UI smoke gerçek düğmelerle missing producer, background/close guard, synthetic discovery/read-only test ve Stop/clear doğrular. build/step6-camera-test.png ve step6-camera-no-producer.png incelendi. Son kontrol enabled-state değişikliği sonrası CTest 6/6 tekrar geçti. Production local.ui.ini kullanılmadı.

Yerel sonuç: camera_probe environment araması NO_PRODUCER, exit 1; build/camera-probe-local.json. Sentetik producer + --open-check DONE/ok; build/camera-probe-fixture.json. Bu yalnız MinGW test DLL ABI kabulüdür; fiziksel Basler/Daheng veya MSVC SDK uyumu değildir. Resmi header indirilmiştir; vendor SDK/driver kurulmadı.

Sınırlar: GenApi feature/XML, stream/buffer/event grab, gerçek trigger/exposure/gain ve reçeteye kamera persistence yok. Keşif UI budget 1000 ms/producer; DLL load/init ve open-close için hard timeout yok; vendor çağrısı zorla kesilmez. Kalıcı connected gösterilmez, cihaz test sonunda kapalıdır. Native DPI ve gerçek disconnect/latency ayrıca kabul ister. Calibration/robot-PLC/Runtime bekliyor.

Sonraki: GenApi özellik erişimi/runtime veya vendor C API uyum seçimi; trigger/stream/grab ve owned ImageFrame conversion. Gerçek producer/modelle fiziksel kabul; ardından kalibrasyon ve Runtime.


## 7 Ekim 2026 — 6A/A3 ToolRegistry ve yürütücüler

- Kapsam: Statik built-in ToolRegistry label/default/port/parametre doğrulama ve tool sürümünü birleştirir. Toolbox registry kullanır; Properties mevcut araca özel presenter'ı korur. Match/Blob/Edge/Calculate ve Image File ayrı yürütücü fonksiyonlarıdır; DAG engine algoritma seçmez. Blob/Edge scalar ayarları doğrulanmış typed modellere dönüştürülür. Derlenmiş ek tool başlangıçta descriptor ve executor kaydeder; DLL plugin yoktur.
- Değişen dosyalar: domain/ToolRegistry.hpp/.cpp, ToolDefinition.cpp, ToolGraph.cpp; application/ToolExecutors.hpp/.cpp, ExecutionService.hpp/.cpp, ImageFileService.hpp; MainWindowEditor.cpp, execution_test.cpp, CMakeLists.txt ve belgeler.
- Doğrulama: Release build başarılı, CTest 6/6 geçti. Test constant tool yalnız registry/executor kaydıyla engine koşulu değişmeden 42 px üretir; bilinmeyen port türü reddedilir. Mevcut algoritma, schema migration, cancellation ve reference regression testleri geçti.
- Görsel: build/a3-editor.png incelendi; tema, menü ve paneller korunur. Test config/build fixture kullanır.
- Sınırlamalar: Parametre JSON persistence korunur; Match mevcut MatchService doğrulama/model yolunu kullanır. Registry extensions başlangıç içindir, runtime hot-load yoktur. Fiziksel kamera/native DPI/CUDA/kurulum kabulü yok.
- Sonraki: A4 application session/controller, worker/cancel/cache ve teaching kurallarını UI'dan ayırma.

## 7 Ekim 2026 — 6A/A4 application session ayrımı

- Kapsam: EditorSession ve CameraSession ortak JobSession ile Idle/Working/Cancelling, job tickets, çakışma koruması, cooperative Stop, future sahipliği ve kapanışta cancel/wait yönetir. Ayrı UI busy boolean'ları kaldırıldı. UI watcher sonuç sunumuna abone olur; session worker başlatır ve tamamlanmayı sunumdan önce işler. Immutable capture'lar worker'a gider, session/UI worker thread'den kullanılmaz.
- EditorSession: frame/generation/result/cache sahipliği, config/upstream hash, stale-result kabulü, cancelled-cycle çıktı temizliği, reference point baseline/cache, model teaching ve Search ROI reference öğretimi application katmanındadır. UI tool güncelleme komutunu ve görünümü uygular. CameraSession discovery/open-close worker yaşamını yönetir; fiziksel lifecycle A6/6B'de kalır.
- Değişenler: application/EditorSession.hpp/.cpp ve tests/session_test.cpp yeni; MainWindow.hpp ve Editor/Execution/Matching/Camera/Workspace/EditorSmoke bağlantıları; CMakeLists.txt ve belgeler.
- Doğrulama: Release build, CTest 7/7 geçti. Yeni session_contract proje/reçete/config/generation stale, job çatışması, Stop, finish-before-presentation, overlapped image tickets, cancelled output temizliği ve destructor cancel/wait doğrular. Mevcut UI gerçek öğretim/cache, stale config, close guard, camera Stop ve session restore regression geçti.
- Görsel: build/a4-editor.png incelendi; GUI tema/menü/panel düzeni korunur. Üretim local.ui.ini kullanılmadı.
- Sınırlar: UI hâlâ presentation watcher callback'leri ve seçili overlay sunumunu taşır. Qt Concurrent application controller bağımlılığıdır. Cancel cooperative; SDK/OpenCV kesilmez. Native DPI/fiziksel kamera kabulü yok.
- Sonraki: A5 kaynaktan bağımsız owned ImageFrame, raw boyut/stride/piksel formatı ve source/frame/cycle/zaman/koordinat metadata.

## 7 Ekim 2026 — 6A/A5 owned frame ve metadata teslimi

- Kapsam: ImageFrame `vision/` altında dosyadan bağımsızdır. fromBuffer bounds/stride/format kontrolünden sonra raw satırları kendi tamponuna kopyalar; SDK buffer serbest bırakma/yeniden kullanımından etkilenmez. Gray8, native-endian Gray16, BGR/BGRA/RGB/RGBA kabul edilir; engine renk depolaması BGR/BGRA, padded girişler compact owned stride'a dönüşür. Raw size preview'dan bağımsız; preview opsiyonel ve ayrı deep-copy sunumdur. Metadata değişimi piksel verisini değiştirmez; frame kopyaları private immutable raw depolamayı paylaşır.
- Metadata: sourceId/frameId/cycleId/coordinateSystemId, UTC completion, optional exposure UTC ve device timestamp ticks/clock ID. Acquisition request kimliğini ve completion zamanını ekler; exposure/device domain korunur. PortValue.origin türetilen çıktılara taşınır; CycleResult.frames frame kökenlerini saklar. CycleResult.frameId mevcut tek kaynak için uyumluluk alanıdır; frames koleksiyonu çoklu frame'e açık sözleşmedir, çoklu kamera engine teslimi değildir. Input origin frame/cycle/coordinate/source mismatch ERROR ve downstream sonuç engeli üretir. Snapshot başka source kimliğiyle kullanılamaz; yeni execution cycle/frame kimliği alır, gerçek edinim zamanı korunur.
- Değişenler: vision/ImageFrame.hpp/.cpp yeni; ImageFileService.hpp/.cpp, AcquisitionService.cpp, ExecutionService.hpp/.cpp, ToolExecutors.hpp/.cpp, EditorSession.cpp, ToolRegistry.hpp/.cpp, MatchService.hpp/.cpp (typed MatchParameters), tests/frame_test.cpp, CMakeLists.txt ve README/mimari/plan/backlog/progress.
- Doğrulama: Release build başarılı; CTest 8/8 geçti. owned_frame_contract fake adapter tampon reuse, preview'sız acquisition/DAG, Gray16/RGB/RGBA/padding, metadata kopya izolasyonu, invalid buffer bounds, source/time/device provenance, derived output origin, foreign snapshot ve aynı cycle'da uyumsuz koordinat/frame reddini sınar. Mevcut schema/model/ROI/Stop/session/UI regression korunur.
- Görsel: build/a5-editor.png ve build/a5-camera.png incelendi; tema, menüler, paneller, ölçüm overlay'i ve discovery/read-only ekranı korunur. Üretim config/local.ui.ini kullanılmadı.
- Sınırlamalar: Tek Image File source yürütmesi sürer; fiziksel stream/grab yok. Frame preview dosya adapter'ında hâlâ eager üretilir; optional raw-only factory servis testinde kabul edilmiştir. Completion exposure değildir; cihaz ticks clock dönüşümü uygulanmadı. Kalibrasyon/Pose2D A7/6C'de. Sentetik fixture native DPI/kurulum/CUDA kabulü değildir.
- Sonraki adım: **6A/A6** kamera lifecycle, cooperative deadline ve hard interruption ayrımı, durdurma/kapanış bütçesi, bounded queue/drop politikası ve oturum sahipliği sözleşmesi; gerçek donanım kabulü 6B'de.

## 7 Ekim 2026 — 6A/A6 kamera lifecycle/deadline/kuyruk çekirdeği

- Kapsam: SDK çağırmayan CameraLifecycle owner-thread ve kopyalanamayan controller; Connect/Configure/Start/Grab/Stop/Disconnect, ara durumlar, Fault ve explicit cleanup/reconnect. Tek in-flight operation, serial/generation ticket; stale completion state'i değiştirmez. Stop/disconnect/fault kuyruğu temizler ve eski stream generation frame'lerini reddeder. Aktif worker dönmeden cleanup/yeniden bağlanma başlatılmaz; SDK success onayı olmadan Connected/Streaming üretilmez.
- Deadline/bütçe: process-local steady_clock Deadline, AcquisitionRequest optional mutlak deadline ve timeoutMs minimumu; kuyrukta tükenmiş istek adapter'a ulaşmaz, kalan bütçe adapter'a taşınır, dönüşte cancel/geç frame/hata kontrolü korunur. Varsayılan Stop ve JobSession shutdown gözlem bütçesi 1000 ms. Poll Pending/Exceeded/Complete veya Ready verir; repeated Stop bütçeyi uzatmaz. Bütçe aşımı worker detach/SDK interruption değildir. JobSession shutdown yeni işi reddeder; future ve owner callback bitmeden Ready olmaz, destructor cancel/wait korur.
- Kuyruk: 1–64 owned frame (varsayılan 2), ayrı PreviewLatest oldest-drop ve ProductionFifo Full/FIFO politikaları. Production dolu/busy iken Grab dispatch öncesi reddedilir. Enqueue raw/source/frame/cycle/generation/deadline doğrular; pop deadline yeniden kontrol eder. accepted/dropped/rejected/expired/stale/invalid/cleared, depth/highWater sayaçları vardır. Cycle'lar yeniden adlandırılmaz, üretim overflow sessiz drop değildir.
- Değişen dosyalar: application/Deadline.hpp ve CameraLifecycle.hpp/.cpp; AcquisitionService.hpp/.cpp; EditorSession.hpp/.cpp; tests/camera_lifecycle_test.cpp; CMakeLists.txt. README, acquisition/architecture/development-plan/tool-execution, backlog ve progress güncellendi. Şema, UI kaynakları, tema/menüler ve config/local.ui.ini değişmedi.
- Doğrulama: Release build ve CTest **9/9 geçti**. Yeni camera_lifecycle_contract sentetik geçişler, invalid/overlap/stale tickets, source/thread ownership, stop budget ve geç frame, fault/worker-drain/cleanup/reconnect, preview drop/production Full/backpressure/FIFO, queue admission/consumption deadline ve kesilemeyen fake worker shutdown budget sınar. Mevcut acquisition/session/owned frame/DAG/GenTL/UI regression testleri korunur.
- Görsel: build/a6-editor.png ve build/a6-camera.png güncel offscreen UI smoke çıktılarından alınıp incelendi; panel, tema, menüler, ölçüm overlay ve keşif/read-only ekranı korunur. Testler build ayarlarını kullanır; üretim INI kullanılmadı.
- Sınırlamalar: CameraLifecycle yeni donanımdan bağımsız sözleşme çekirdeğidir; mevcut GenTL discovery/read-only worker'ı canlı stream controller'ına henüz bağlanmadı. Fiziksel kamera/trigger/grab/SDK cleanup ve interrupt kabulü **açık**; 6B'ye taşınır. Mevcut process-geneli GenTL try-lock korunur, çoklu cihaz mimarisi değildir. Handle library/TL/interface/device sahipliği worker RAII'de kalır; çoklu producer/device init/thread-affinity gerçek SDK ile kanıtlanmalıdır. Stop/shutdown hard timeout değildir; GUI close guard korunur ve kesilemeyen SDK destructor beklemesini uzatabilir. Byte memory limiti, production overflow/expired cycle hata kaydı ve external trigger throttling 6B adapter/consumer entegrasyonunda uygulanacak. Native DPI/kurulum/CUDA kabulü yok.
- Sonraki adım: **6A/A7** ortak Pose2D/koordinat/tracking ROI sözleşmesi, birim/frame/dönüşüm yönü ve bilinen geometri örnekleriyle kabul; mevcut translation-only Edge/migration davranışını koru. Fiziksel kamera kabulü 6B'de açık kalır.
