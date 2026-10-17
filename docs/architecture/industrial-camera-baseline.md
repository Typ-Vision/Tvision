# ICM-00 — kaynak, sürüm ve ret kapıları başlangıç kaydı

17 Ekim 2026. **ICM-00 tamamlandı; ürün davranışı değişmedi.** Bu envanter,
[ICM planının](industrial-camera-plan.md) sonraki kartları için kaynak koddan
çıkarılmış başlangıçtır. Endüstriyel acquisition, fiziksel kamera veya ölçüm kabulü
değildir. Donanım/SDK deneyi bu kartta yapılmadı. Sonraki görev **ICM-01a**.

## Kanıt ve mevcut durum

- Yerel kaynak `C:/Projects/Tvision`; başlangıç HEAD ve dirty listesi
  `build/icm/00/git-head.txt`, `git-status-before.txt` içinde. Önceden değişmiş
  belgeler korundu; commit/push yapılmadı. Ürün dosyalarının SHA256 envanteri
  `product-files-before.json`; ürün diff'i `product-diff-before.patch`.
- Release cache'leri `CMakeCache-ui.txt`, `CMakeCache-core.txt`: MinGW 13.1,
  Qt 6.12, OpenCV 5.0; UI `build/reference-ui` Widgets ON / ZXing ON,
  core `build/kmc9-core-off` Widgets OFF / ZXing OFF. Yeni configure yapılmadı.
- Güncel `ctest -N`: **87 UI, 73 core kayıtlı test**. Tam adlar `ctest-ui.txt`,
  `ctest-core.txt`; komut ve property'ler `ctest-ui.json`, `ctest-core.json`.
  Bu sayılar yeni tam suite başarı sonucu değildir. Yeni build/test sonucu ve
  binary hash'leri `verification.json`, `build-*.log`, `foundation-tests.log/xml`
  içindedir; görev kapanışının özeti [progress](../progress.md) dosyasındadır.
- Yeni koşum: iki Release derlemesi exit0, derleyici warning/error0; aşağıdaki
  temel seçkide **12/12 core CTest, 15.26 sn**. 330 ürün dosyasının SHA256'sı
  değişmedi; 29 test adı ve güncellenen belgelerin yerel bağlantıları doğrulandı.
- Taranan source/codec/kapı eşleşmeleri `source-contract-matches.txt` içinde.
  Kanıtlar `build/icm/00/` altındadır ve Git dışıdır. SDK binary'si kopyalanmadı.

## Katmanlar ve gerçek acquisition kapsamı

| Alan | Mevcut kaynak / uygulanan davranış |
| --- | --- |
| Endüstriyel discovery | `hardware/GenTLTransport.hpp/.cpp`, `application/CameraService.hpp/.cpp`: producer yükleme, metadata discovery, read-only open/close; worker/thread sahipliği ve process-geneli producer serileştirmesi |
| Eksik endüstriyel yol | GenTL API yüzeyinde stream/data stream/event/buffer veya remote XML/GenApi feature configure yok; `CameraDevice` producer/interface/device/vendor/model/serial/transport metadata'sıdır, kalıcı fiziksel binding değildir |
| Ortak acquisition | `application/AcquisitionService.hpp/.cpp`: adapter capability, Software/External/Continuous retleri, deadline/cancel ve gerçek frame provenance; üretimde mevcut adapter'lar ImageFile ve Webcam |
| Qt webcam | `application/WebcamAdapter.hpp/.cpp`: Qt discovery/provider ID ve raw frame; capabilities `webcam`, software=true, external/continuous/exposure/gain=false |
| Owned görüntü | `vision/ImageFrame.hpp/.cpp`: Gray8/16, BGR/RGB/BGRA/RGBA, checked boyut/stride/tampon ve satır kopyası; metadata ile preview ayrı. SDK payload/chunk/PFNC doğrulaması henüz yok |
| Lifecycle | `application/CameraLifecycle.hpp/.cpp`, CameraSession/EditorSession: owner/generation/deadline, preview oldest-drop ve production FIFO Full; donanım SDK stream'ine bağlı değil |
| Reçetesiz kalibrasyon | `IndependentCalibrationCapture`: immutable request/ticket/generation, exact camera/setup, actual raw frame; `IndependentCaptureBackend` injection var fakat webcam kapısını aşmaz |
| Tek ortak çalışma alanı | `MeasurementWorkspace.hpp`: MainWindow sahibinde registry/scoped calibration; varsayılan discovery `WebcamAdapter::discover`; recipe ve Runtime aynı setup/model sözleşmesini tüketir |
| Execution | `ExecutionService`, `ToolExecutors`, `SourceFrameIndex`: editor9 required source seti, sıralı edinim ve kaynak başına gerçek frame; factory seam yalnız multi-source acquisition dalında; TargetOnly cache'li raw karede acquisition0 |
| Runtime / history | `RuntimeService`, `MultiSourceRuntimeService`, `ProductionRecipeService`, `ProductionResultCodec`: immutable set admission/begin, private trusted completion, kalıcı terminal/replay, record retry ve SDK'sız tarihçe |

UI SDK çağırmaz. Üç applicationType aynı Editor/Runtime yolunu kullanır. Run preparation'dır;
gerçek çevrim olmadan frame, skor, PASS veya robot pose üretmez.

## Ret noktaları ve genişletilmesi gereken bağlantılar

Tablo yalnız hata metinlerini değil; industrial eklenirken birlikte korunması veya
yeni sürümlü kanıtla genişletilmesi gereken tüm mevcut source sınırlarını listeler.
Kaynak yolları `src/` altındadır.

| Kapı / seçim | Kaynak ve mevcut sınır | İlgili ICM |
| --- | --- | --- |
| Domain physical binding | `domain/MeasurementSetupContract.cpp`, `DeviceBinding::validate`: yalnız webcam/gentl; gentl için softwareCapture=true reddi. resolve exact adapter/provider/confidence ve aynı capabilities ile tek aday ister; index fallback yok | 03a/03b |
| Capture request | `application/IndependentCalibrationCapture.cpp`, validate: kamera veya setup-file hedefinde webcam + complete geometry; sensorRoi/binning reddi. run: actual sourceKind webcam olmalı; trial file hedef taşıyamaz | 06a |
| Scoped installation | `application/ScopedActiveCalibration.cpp`, installation: webcam, complete, ROI/binning yok, confidence unavailable değil. activate accepted camera probe/exact setup ister; restart yalnız seçimdir | 06b |
| Model-ready setup codec | `application/ActiveCalibrationService.cpp`, MeasurementSetup::validate: adapter yalnız image_file/webcam; yeni industrial model, capture kapısı açılsa bile burada reddedilir. Yeni sürüm eski wrapper replay'ini korumalı | 06a/06b |
| Workspace discovery/availability | `application/MeasurementWorkspace.hpp`: webcam discovery, webcam dışı unsupported availability; observeAcquisition yalnız webcam hatasında disconnect; acquisition exact kayıtlı setup için IndependentCaptureRequest::validate kapısını uygular, setup/device/geometry döndürür, kendisi SDK açmaz | 05a/06b |
| Eski recipe calibration | `application/CalibrationCapture.cpp`: acquire/configuration yalnız image_file/webcam; diğer kind hata. SystemCalibration::source yalnız bu iki enabled kind'den tek kaynak seçer; branch aynı kurala gelir | 06a/07a |
| Legacy calibration context | `application/CalibrationService.cpp` context ve `CalibrationWorkflow.cpp` actual frame kontrolü yalnız image_file/webcam enabled kaynağını sayar; tam bir kaynak/provenance ister. Recipe-scoped eski yol industrial için sessiz fallback olamaz | 06b/07b |
| Model scope | `application/SystemCalibration.cpp`: exact source/setup/device/geometryId/config/actual resolution ve metadata; scoped kaynak kind'i image_file veya model adapter olmalı, scoped→legacy fallback yok | 06b/07b |
| Tool kayıt/DAG | `domain/ToolRegistry.cpp`, `ToolDefinition.cpp`, `ToolGraph.cpp`: yalnız image_file/webcam built-in acquisitionSource; bilinmeyen tool kayıtlı değil. Eski editor tek source; explicit editor9 çok source | 07a |
| Recipe invalidation | `application/ProjectService.cpp`: sourceConfig image_file/webcam parametre/enabled değişimini eski calibrationBinding invalidation'ına dahil eder; industrial/profile eklenince aynı invalidation sınırı korunmalı | 07a/07b |
| Executor/factory | `application/ToolExecutors.cpp`: image_file ve webcam gerçek source executor'ları. `ExecutionService.cpp`: scoped webcam Qt discovery + WebcamAdapter; injection sourceKind eşleşmesi kapısı industrial ürün desteği değildir; actual frame sonrası snapshot geometri kontrolü korunur | 07b |
| Immutable admission | `application/ProductionRecipeService.cpp`, dependencies: measurementSource/1 dört alan, acquisition üç alan; complete webcam, ROI/binning yok; whitelist'te industrial yok. editor9 doğrudan single service'de ret, whole-set service gerekli | 08a |
| Çok-source admission | `application/MultiSourceRuntimeService.cpp`: her dal iç projection editor8 ile aynı ProductionRecipeService kontrolünden geçer; aynı fiziksel kaynak tekrar talebi reddedilir. Whole-set provider atomic consistency yerine sessiz parçalı fallback yapmaz | 08a |
| Runtime seçimleri | `application/RuntimeService.cpp`: source/capture/installations kontrollerinde image_file/webcam listeleri; multi worker frozen sourceSet üzerinden yürür; kind listeleri ortak factory/evidence ile birlikte genişlemeli | 08a/08b |
| Kalıcı terminal okuyucu | `application/ProductionResultCodec.cpp`: terminal6 webcam/complete/no ROI-binning; origin image_file→offline_replay, webcam→camera_acquired. terminal7 branch schema3–6 replay eder; yeni kimlik alanı eski exact envelope'a eklenemez | 08b |
| UI kaynakları | `app/CalibrationOverview.cpp`: webcam kaydı/düzenleme ve ROI/binning retleri; `MainWindowCalibration.cpp`: recipe source seçimi image_file/webcam; `MainWindowEditor.cpp`: webcam properties/preview caption; `MainWindowCamera.cpp`: ayrı discovery/read-only ekranı | 05b/07c/08c |
| Backend capability | `application/BackendCapabilities.cpp`: yalnız zxing-reader kayıtlı; unknown backend unavailable. GenTL discovery bulunması acquisition backend capability değildir | 02/04a |

## Şema ve sürüm tablosu

Bu kart hiçbir yeni sürüm numarası ayırmaz. Sonraki kart, yeni zorunlu alanı explicit
yeni codec dalıyla ekler; mevcut schema'ya alan ekleyip exact-field kontrolünü gevşetmez.

| Sözleşme | Bugünkü sürüm / kaynak |
| --- | --- |
| Project / Recipe | root1; recipe1–2; `domain/Project.cpp`, `persistence/ProjectRepository.cpp`; calibrationBinding yalnız recipe2 |
| Editor | reader1–9; dependency array7, reference/world opt-in8, explicit multi-source9; `domain/ToolDefinition.cpp`. Eski Save sırf yeni kod açıldı diye9 olmaz |
| Engine | `visionflow.execution/24`; `application/ExecutionService.hpp` |
| Tool descriptor'ları | 16 built-in: image_file1, template_match1, blob3, edge1, calculate1, webcam1, measurement2, preprocess1, code_read2, edge_point3, geometry_measurement2, edge_pair2, line_fit3, circle_fit3, pattern_pose1, shape_pose1; `domain/ToolRegistry.cpp` |
| Measurement contract | DeviceBinding1, GeometryDescriptor1, MeasurementCamera1, MeasurementSetupDescriptor1, MeasurementSetupContract1; DeviceCapabilities sürümsüz dört bool nested nesne; `domain/MeasurementSetupContract.cpp`. ICM-03a ekledi: MeasurementCamera2 (`physical` zorunlu; yoksa schema 1 bayt aynı), PhysicalDeviceIdentity1; TransportLocator1/BackendIdentity1 yalnız yerel, projeye yazılmaz |
| Environment / transfer | measurement-environment1, measurement-bundle1; `application/MeasurementEnvironment.cpp`. Registry geçmişi/arşiv/selection alanları hash'e dahildir |
| Source evidence | SetupReference1, MeasurementSourceEvidence1; origin camera_acquired/setup_file/trial_file/legacy_record, sourceKind recipe_tool/calibration_session; `MeasurementSourceIdentity.cpp` |
| Kalibrasyon | MeasurementSetup1; CalibrationModelRecord wrapper1–5 (model-ready1, named2, legacy3, association4, migration5); active-calibration katalog1; `ActiveCalibrationService.cpp` |
| Eski model codec | uniform1, raw homography2, manual scale3; `MeasurementService.cpp`; yeni wrapper eski model/hash'i yeniden yazmaz |
| Source/model scope | sourceSetup1, scoped-source/1; scope1 tam beş alan: schemaVersion/target/device/geometryId/setup; system-model-ready/1 evidence1 unscoped, evidence2 scoped; `SystemCalibration.cpp` |
| Production dependency | measurementSource1 binding/acquisition/origin; sourceSet dal dependencies; validation scope execution_configuration veya multi_source_in_memory/1 / multi_source_runtime/1; `ProductionRecipeService.hpp/.cpp`, `MultiSourceRuntimeService.cpp` |
| Terminal / port | terminal reader1–7; single writer3–6, multi writer7 ve branch3–6; output port codec1; `ProductionResultCodec.cpp`, `MultiSourceRuntimeService.cpp`, `ToolResultCodec.cpp` |
| Arşiv / PLC | snapshot/event1; vf.socket/2 mevcut scalar mapping; terminal schema'sı transport sürümü değildir |

## Hash ve legacy no-op koruma alanları

| Kimlik | Korunacak içerik ve sınır |
| --- | --- |
| geometryId / cacheId | MeasurementSetupDescriptor geometri/fiziksel setup hash'i ile non-geometric settings cache hash'i ayrıdır; ad değişimi fiziksel revision değildir. Yeni offset/reverse/decimation/profile bilgisi eski geometry1 içine sessizce doldurulmaz |
| Registry contentId | Environment compact JSON'dan contentId çıkarılarak SHA256 doğrulanır; strict keys, history/revision/arşiv ve selection korunur |
| Model / selection | Legacy uniform/homography/manual model SHA256 ve wrapper original/derived replay korunur; rename/association gerçek orijinal kanıtı değiştirmez. Global/scoped execution dependency hash'i system-model-ready/1 ve exact scope içerir |
| Raster | ImageFrame::contentId = boyut + pixel format + her satırın gerçek image byte'ları SHA256; padding/preview/metadata hariç. Durağan sahnede aynı hash iki farklı acquisition olabilir |
| Editor cache | toolSignature engine, scope, required upstream tool/version/params/dependency, file byte hash'i, connections ve editor9 source binding taşır. TargetOnly aynı gerçek raw frame ve güncel signature ister |
| Production snapshot | Canonical recipe ve dependency compact JSON SHA256; whole-set sourceSet/model/engine ve original editor9 sabitlenir. Producer yolu fiziksel kimlik yerine kullanılamaz |
| Trusted completion | Private lease/snapshot/ticket ile QDataStream digest; frame metadata, typed output, image content/processing/size/format ve tanılar dahildir. Hardware evidence eklenince aynı private digest'e ve replay'e birlikte girmeli |
| Disk ve retry | Terminal/source dependency hash'i ve branch replay; append/idempotent payload/record ID sabit. Retry acquisition/provider/trigger çalıştırmaz; restart history yükler, authorization yüklemez |

Korunacak legacy yollar: image_file/webcam default parameters ve provider ID eşlemesi,
editor1–8 migration/no-op Save, explicit olmayan editor9 upgrade'inin yapılmaması,
root1/recipe1–2 JSON, mevcut model/wrapper1–5 hash/replay, unscoped eski recipe resolver,
scoped seçimsizde ret, source origin ve file evidence'ın kamera gibi etiketlenmemesi,
terminal1–7 SDK/registry olmadan okuma, Stop/ERROR boş değer ve vf.socket2 scalar mapping.

## Industrial tool ve evidence genişletme kararı

Yeni tool kind **`industrial_camera`** olarak sabitlendi; `gentl` mevcut discovery/binding
adapter etiketi olarak kalır. Bu ad henüz ToolRegistry/Toolbox'a eklenmedi. Webcam'in
adı veya parametreleri industrial olarak yeniden yorumlanmayacak. ICM-07a yeni source'u
registry/default/validation/port/migration ile explicit ekleyecek; backend seçimi ICM-02'de.

ICM-03a fiziksel identity/confidence ile local TransportLocator ve BackendIdentity'yi
ayırır. ICM-03b requested/applied profile ve tam geometri codec'lerini ekler. ICM-04c/d
owned image yanında actual hardware counter/tick, stream generation ve trigger ilişkisini
kanıtlar; mevcut application frame/cycle ID'leri hardware counter yerine kullanılamaz.
ICM-06b/07b bu değerleri setup/model/cache ve actual acquisition sınırında doğrular.
ICM-08a/b frozen dependency, private completion digest ve terminal replay'i birlikte
sürümler. Terminal7 ve scope1 exact alanları değiştirilmeden eski reader dalı korunur;
yeni schema numarası ilgili uygulama kartında kaynak envanteri yeniden kontrol edilerek seçilir.

## Test eşlemesi ve sonraki adım

| Kabul grubu | Bugün gerçekten kayıtlı CTest adları |
| --- | --- |
| S01/02/08–14 | camera_transport, camera_lifecycle_contract, acquisition_contract, owned_frame_contract, session_contract |
| S03–05/16 | measurement_setup_contract, measurement_environment_persistence, scoped_active_calibration, system_calibration_contract |
| S04/25 | tool_migration_contract, project_persistence, measurement_setup_migration |
| S15 | independent_calibration_capture, calibration_wizard_contract |
| S17/18/20 | multi_source_execution, multi_source_target_only, editor_target_only, multi_source_graph_contract, multi_source_production_contract |
| S19/21–24 | production_recipe_contract, runtime_coordinator, multi_source_terminal_trace, typed_terminal_trace, runtime_persistence, plc_socket_runtime |
| S26 | ui_calibration_overview, ui_multi_source_editor, ui_multi_source_runtime; üçü UI envanterinde |
| S27/28 | backend_capabilities; iki tam envanter JSON'u. Node-map/configure/trigger/backend lifetime için planlanan yeni ICM testleri henüz kayıtlı değil |

Bu eşleme testlerin gelecekteki industrial senaryolarını şimdiden doğruladığı anlamına
gelmez. ICM-00 ilgili temel testleri çalıştırır; yeni kamera acceptance testleri eklemez.
UI değişmediği için yeni görsel kontrol yoktur. **G0–G4 ve H01–H08 açık**; mevcut
0-device pre-reboot probe gerçek kamera/open/grab kanıtı değildir. ICM-01a bir sonraki
tek teslim olarak tekrar çalıştırılabilir SDK/probe tanısını hazırlayacak; fiziksel
seri/görüntü/reboot doğrulaması ICM-01b'de kalır.
