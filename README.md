# VisionFlow Industrial Vision Suite

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
- [A7 koordinat/tracking ROI](docs/architecture/coordinates.md)
- [A8 üretim snapshot](docs/architecture/production-recipes.md)
- [A9 persistence/migration planı](docs/architecture/persistence-plan.md)
- [İş listesi](docs/backlog.md)
- [İlerleme ve oturum devri](docs/progress.md)
- [Matching kararı ve ölçüm](docs/performance/opencv_matching.md)
- [Araç kontrolü](docs/toolchain.md)
- [HTML prototipi](prototypes/visionflow_ui_prototype.html)

## Mevcut durum

Adım 5, 6.1–6.2 ve 6A/A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı. Ortak Recipe Editor, typed DAG, background execution, gerçek Blob/Edge/Calculate, ROI/model teaching, template match, oturum restore ve GenTL keşif/read-only open-close mevcut. Canlı grab, kalibrasyon ve Runtime henüz yok. Sıradaki 6B gerçek producer/SDK ve tek kamera entegrasyonu; fiziksel kamera kabulü açık.

Onaylanan sıra: 6A mimari → 6B kamera → 6C Measurement/kalibrasyon/tolerans/doğrulanmış reçete/Runtime → 6D PLC → 6E robot → 7 ürünleştirme. Temel servis yetkisi, hata kodları ve cycle logları 6C'ye dahildir. Üç applicationType aynı editor/engine/Runtime yolunu kullanır. Tema, panel düzeni, menüler ve navigasyon korunur. Yeni oturumda önce progress okunur; hazırlık ekranları çalışan özellik değildir.

## Yerel derleme (PowerShell)

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build -j 6
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
ctest --test-dir build --output-on-failure
.\build\match_benchmark.exe
.\build\visionflow.exe
```

Yollar bu makineye aittir. CMake/Ninja PATH üzerinde olmalı; başka makinede Qt/OpenCV yolları ve ABI uyumu doğrulanmalı. Dağıtım öncesi windeployqt ve OpenCV DLL paketleme ayrıca doğrulanmalıdır. Kurulum/bağımlılık indirme otomatik yapılmaz; CUDA etkin değildir.

## UI çalıştırma ve kullanım

```powershell
.\scripts\run-local.ps1
.\scripts\run-local.ps1 -SmokeTest
```

PS1 çift tıklamada Not Defteri ile açılıyorsa `powershell.exe -ExecutionPolicy Bypass -File "C:\OpenCode\TVision\scripts\run-local.ps1"` kullanın.

Project Manager → New Project ile adı ve mevcut klasörde yeni `.vfproject` yolunu seçin; Save Project ilk kaydı yapar. Recipe Manager → New Recipe ile tür, ad ve görüntü kaynağı referansı girin; ortak editor açılır. Kaydedilmiş reçete değişince DRAFT/dirty olur; revizyon yalnız başarılı kayıtta artar. No-op Save artırmaz. Save/Ctrl+S tüm projeyi atomik kaydeder; başarısız commit/conflict dirty ve önceki dosyayı korur. Kapatma/proje değişiminde Save/Discard/Cancel vardır. Open Project ve Recipe Manager ile yeniden açılır. Copy/Delete/import/export/version history henüz yoktur.

Editor → Open Image gerçek görüntüyü yükler; Image File tool ve göreli yolu reçeteye ekler. Gray8/Gray16/RGB8/RGBA8; Fit, +/−, 1:1/tekerlek zoom ve sol tuş pan desteklenir. Hover orijinal piksel/koordinatı gösterir. Reload yeniden okur; görüntü projeye gömülmez. Toolbox Add Tool/double-click ile araç ekler; Flow seçimi Properties'yi değiştirir. Name/Description/Enabled/parametre değişiklikleri DRAFT olur. Tool silme bağlı bağlantıları temizler.

Typed input seçicileriyle Image File.image → Blob/Edge.image ve iki Edge.point → Calculate.p1/p2 bağlanır. Validate DAG eksik/uyumsuz giriş ve döngüyü gösterir. Run Flow dosyayı yeniden edinip DAG'yi worker'da çalıştırır. Stop cooperative; devam eden OpenCV/decode zorla kesilmez. Results gerçek cycle/frame, state, süre ve typed outputs gösterir. Blob threshold/8-connected en büyük alan/merkez; Edge dört yönlü profile ilk contrast geçişi; Calculate Öklid mesafesi px uygular. Search ROI veya bağlı Region kullanılabilir. Found=false downstream SKIPPED yapar; ERROR/SKIPPED parça kararı değildir. Edge profile çizgisi, Calculate P1/P2 sarı çizgi ve px overlay sunar.

Template Match → Model ROI çiz / öğret → Search ROI çiz → Image input bağlantısı → Run Match. Komut Run Flow ile aynı engine'de seçili tool/upstream alt grafiğini ve yeni acquisition'ı kullanır. Bağlı Region yerel ROI önüne geçer. Ham skor, sol üst konum ve süre gerçek sonuçtan gelir; eşik üstü yeşil/eşik altı kırmızı aday gösterilir. Sabit açı/ölçekte tek sonuç desteklenir. Model PNG reçeteye atomik gömülür. Görüntü/ayar/reçete değişince eski sonuç temizlenir; reçete DRAFT kalır. UI yalnız servisleri çağırır; acquisition/teaching/execution worker'dadır. Aktif worker bitmeden kapanış bekletilir.

Edge Properties → Referans merkezi → Template Match.point seçip Search ROI çiz ile offset/boyut öğretin. Güncel reference sonucu varsa kullanılır; yoksa model teaching merkezi kullanılır. Güncel match merkeziyle X/Y ötelemesi uygulanır. Reference bulunamazsa Edge/downstream SKIPPED. Model ROI/PNG değişirse referenceGeometry uyuşmazlığı Edge ERROR üretir; Search ROI yeniden öğretimi gerekir. Metadata, Min Score ve Match search değişikliği bu kimliği değiştirmez. Editor schema 5, schema 1–4 açılışını dirty olmadan destekler; legacy geçmiş model değişimleri kanıtlanamaz. Genel açı/ölçek ve mm kalibrasyonu henüz yoktur.

Panel boyutları, son proje/reçete/ekran `config/local.ui.ini` içinde saklanır; bu autosave değildir. Eksik/bozuk son proje hata ve manuel açma sunar. View → Panel düzenini sıfırla varsayılan düzeni getirir. Minimum pencere 850×600; native DPI/çoklu monitör kabulü açık. Testler build altında izole INI/proje/sentetik görüntü kullanır; üretim INI kullanılmaz. Smoke `step5-editor.png`, `step5-blob.png`, `step5-flow.png` ve kamera görsellerini üretir.

## Camera Manager ve CLI

Producer seç ile kurulu Windows x64 `.cti` seçin → Cihazları ara. Boş yolda GENICAM_GENTL64_PATH kullanılır. Liste gerçek producer cevabıdır. Seçili cihazı salt okunur aç/kapat kalıcı stream kurmaz, ayar değiştirmez. İşlemler worker'da; Stop cooperative. Producer discovery bütçesi 1000 ms, DLL init/open-close hard timeout değildir.

```powershell
.\build\visionflow_camera_probe.exe
.\build\visionflow_camera_probe.exe --producer 'C:\VendorSDK\Producer.cti' --timeout 3000 --open-check
```

CLI JSON döndürür: NO_PRODUCER/ERROR exit 1, DONE exit 0. Cihaz bulunmaması DONE olabilir; grab kabulü değildir. Bu makinenin önceki gerçek probe sonucu NO_PRODUCER; SDK/driver kurulmadı. `gentl_test_producer.cti` sentetik C ABI fixture'dır. Trigger/stream/grab/exposure/gain/kamera reçete kaydı fiziksel kabulü açık.

## Çekirdek sınırları

ToolRegistry beş built-in tool'un default/port/sürüm/validation kaynağıdır; ToolExecutors algoritmaları engine sıralamasından ayırır. Startup compiled extension desteklenir; dinamik DLL plugin yoktur. EditorSession/CameraSession job ticket/future/cancel/cache/stale sonuç sahipliğini taşır; destructor worker'ları bekler.

ImageFrame owned buffer, stride, Gray8/16/BGR/BGRA/RGB/RGBA ve optional preview taşır. FrameMetadata source/frame/cycle/coordinate/time kökenini sonuçlara aktarır; unit/type/frame uyuşmazlığı engellenir. Güncel engine tek Image File kullanır. CameraLifecycle owner-thread/generation, preview oldest-drop ve production FIFO Full kuyruğu sunar; mutlak monoton deadline ve shutdown gözlem bütçesi SDK interruption garantisi değildir. Controller stream SDK'ya henüz bağlı değildir.

A7 Pose2D/Transform2D/TrackingRoi ve CalibrationDependency schema 1 kayıt/geometri sözleşmesidir. Dönmüş raster sampling, Match açı/Pose portu, kalibrasyon hesabı ve robot pose uygulanmadı. A8 ProductionRecipeService immutable snapshot, içerik/model/engine/tool/asset/calibration dependency ve varsayılan ret policy taşır. Cycle boyunca aktif sürüm değişmez; son 128 gerçek trace bellektedir. Disk VALID üretim izni olmaz. Users/Runtime/kalıcı log henüz bağlı değildir.

## A9 bağımsız çekirdek ve kaynak sürümleme

```powershell
cmake -S . -B build/core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/core -j 6
ctest --test-dir build/core --output-on-failure
.\scripts\validate-local.ps1
```

DLL PATH yukarıdaki gibi ayarlanır. Widgets OFF domain/application/engine, probe ve dokuz core testi derler; Qt Core/Gui/Concurrent (QImage preview) ve OpenCV sürer. BUILD_TESTING=OFF testleri çıkarır. UI ON iki ek `ui` etiketli test üretir. Script build/a9-core ve build/a9-ui altında Release configure/build/CTest yapar; QtRoot/OpenCVRoot/CompilerBin parametreleri vardır.

Yerel repo `C:/OpenCode/TVision/.git`, branch main; komşu TVis ayrı C++17 projesidir. Build, log/image verileri ve config/local.* Git dışında kalır; remote yayın/CI yoktur. Manifest/hash/boyut/atomiklik/recovery/sıralı migration [planlandı](docs/architecture/persistence-plan.md); mevcut root/recipe schema 1, editor schema 5 ve 16 MB sınırı korunur. Container/import/export/kalıcı geçmiş uygulaması 6C/7'de kalır.

Son kabul: Release core 9/9 ve UI 11/11 CTest; script temiz iki build üzerinde geçti. ROI sentetik regression 100/100 doğru, performans CSV build altında; gerçek üretim latency/kurulum/CUDA/fiziksel kamera kabulü değildir.
