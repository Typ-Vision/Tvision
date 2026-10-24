> Tarihsel arşiv — 17 Ekim 2026 taşıması. Güncel devir: [progress](../../progress.md).

<a id="entry-103"></a>
## 7 Ekim 2026 — 6C sürümlü calibration persistence/current resolver ve reçete binding

- Kapsam: Kullanıcının adım onayıyla CalibrationService atomik evidence kataloğu ve MeasurementService codec eklendi. Decode scale/holdout/residual/RMS/content kimliğini yeniden hesaplar; disk validated bayrağı izin değildir. 4 MiB/128 kayıt; ID başına 1'den başlayan ardışık append, overwrite/gap/otomatik silme yok. QLockFile ve direct fallback kapalı QSaveFile; yeni kayıt latest/current olur, üretim Activate değildir.
- Current/binding: bağımsız application InstallationProvider kamera/source, lens, raw resolution, düzlem ve koordinat/birim/eksen kurulumunu çözer; eksik/uyumsuz provider reddedilir. Recipe schema 2 kesin dependency/null taşır; legacy schema 1 açılışı dirty yapmaz. Explicit bind DRAFT/revision-on-save, no-op/discard/save/reopen korunur. Kaynak ID/kind/enabled/parameters veya recipe camera/reference değişimi binding temizler; metadata korur. resolve aynı katalog snapshot'ında tek enabled acquisition source UUID ve current/evidence eşleşmesini denetler. ProductionRecipeService binding hash/eşleşmesi ve resolver DependencyChanged kontrolü; testte yeni calibration version eski validation activation'ını engeller.
- Dosyalar: yeni src/application/CalibrationService.hpp/.cpp ve tests/calibration_test.cpp; MeasurementService codec; domain/Project, ProjectService, ProjectRepository, ProductionRecipeService, CMakeLists; README, progress/backlog/toolchain, architecture/measurement/architecture/development-plan/project-format/production-recipes/persistence-plan. Önceki webcam/GenTL değişiklikleri korundu; UI kaynakları ve config/local.ui.ini bu adımda değiştirilmedi.
- Doğrulama: temiz build/calibration-release Release build ve CTest **14/14**; temiz Widgets OFF build/calibration-core Release build ve CTest **12/12**. calibration_persistence restart, evidence/type/future-schema/content/residual bozulması, byte/record limitleri, gap/overwrite/lock/failed save, kurulum değişimleri, schema 1→2 binding, no-op/discard/kaynak değişimi ve eski production validation reddini sınar. Mevcut acquisition/session/DAG/production/UI regression geçti. Güncel build/calibration-release/step5-editor.png görsel olarak incelendi: tema/menüler/paneller ve 399 px overlay korunur. Fixture/INI/görseller build altında; git diff --check başarılı.
- Sınırlamalar: application servis teslimidir; gerçek InstallationProvider/Calibration UI/Measurement DAG/Runtime henüz bağlı değil. Webcam index kalıcı fiziksel kimlik değildir. Matematiksel evidence replay fiziksel bağımsız örnek/üretim doğruluğu veya yetki kabulü değildir. Katalog ve proje iki ayrı atomik commit; taşınabilir paket veya ortak transaction yok. Eski binding latest değişince reddedilir, otomatik rebind/rollback yok. Kalıcı production snapshot/audit/cycle log, disk full/power loss, industrial camera/native DPI/dağıtım/CUDA kabulü açık.
- Sonraki: **ortak Measurement tool/Properties ve Calibration UI**, aynı servis/resolver/binding yolunu kullanarak iki gerçek Edge Point → mm/tolerans sonucu; sonra yetkili production validation/Runtime/cycle log. Kullanıcı sonraki adımı bir cümleyle açıklayıp onay beklenmesini istedi; yeni adım onayı alınmadan genişletilmez.

<a id="entry-104"></a>
## 7 Ekim 2026 — İş 2/2: planar scale/Measurement/tolerans çekirdeği ve final kabul

- Kapsam: MeasurementService ve private/immutable PlanarScaleCalibration. Bilinen uzunlukla uniform mm/px candidate; 2–256 farklı holdout segmenti, per-sample residual sınırı ve RMS mm. Teaching tek başına validated olmaz; teaching/duplicate geometrisi holdout kabul edilmez. Content hash setup/model/scale/evidence/error bound içerir; validate candidate'ı değiştirmez.
- Ölçüm: iki gerçek DONE Point/px/image portundan source/frame/cycle/coordinate/completion kontrolü, raw resolution ve bağımsız current CalibrationDependency birebir eşleşmesi. Inclusive tolerans ham double mm ile karşılaştırılır. Error kodları CALIBRATION_INVALID/MEASUREMENT_INPUT_INVALID/TOLERANCE_INVALID; hata NotEvaluated ve boş değer üretir. DONE ile tek ölçüm Pass/Fail kararı ayrıdır.
- Dosyalar: src/application/MeasurementService.hpp/.cpp, tests/measurement_test.cpp, CMakeLists.txt; README, docs/progress/backlog/toolchain, architecture/acquisition/architecture/development-plan/editor-definition/tool-execution ve yeni measurement.md. İş 1 webcam kaynakları ve UI smoke sonlandırıldı. Önceki oturumun GenTL/SDK belge-kod değişiklikleri korunur; config/local.ui.ini değiştirilmedi.
- Final doğrulama: build/webcam-release Release ve CTest **13/13**; temiz build/webcam-core Widgets OFF Release ve CTest **11/11**. git diff --check başarılı. Yeni measurement_contract candidate/holdout/residual/NaN/Inf/duplicate, kurulum bağımlılığı, Error/Skipped, yabancı frame/cycle ve yuvarlama toleransı testlerini içerir. Owned camera snapshot → gerçek Edge DAG → Measurement **399 px / 9.975 mm**; doğru cycle/source kökeni korunur. Bunlar sentetik kalibrasyon örnekleridir.
- Gerçek webcam: CLI default Qt backend 1280×720 acquisition/Blob DAG DONE (yaklaşık 948 ms); Windows media implementation ayrıca DONE (1005 ms). İzole INI ile yükseltilmiş `visionflow --smoke-test --webcam-capture` exit 0; actual editor worker kaynak edinimi DONE (1224 ms). Son render'da webcam-properties.png ve webcam-capture.png, mevcut step5-editor.png görsel olarak incelendi; tema/menü/panel düzeni, px overlay korunur. Webcam görüntüsü çok karanlık; doğruluk/aydınlatma kabulü değildir. Test ayarları/görseller/JSON build altında; ürün local INI kullanılmaz.
- Sınırlamalar: uniform scale yalnız paralel düzlem varsayımı; lens/perspective correction yok. Holdout kanıtının gerçek bağımsız ölçüm kökeni dış kabul gerektirir. Current manifest application resolver tarafından bağımsız çözülmeli; calibration.dependency() körlemesine current değildir. Webcam index kalıcı fiziksel camera ID değildir. Kalibrasyon persistence/recipe binding/Measurement DAG tool/Calibration UI/ProductionSnapshot/Runtime/cycle log henüz bağlı değil; mevcut Calculate px olarak kalır. ProductionRecipeService webcam activation'ını hâlâ reddeder. Persistent stream/CameraLifecycle kuyruk ve fiziksel unplug/Stop/recovery/industrial trigger kabulü açık; native DPI/dağıtım/CUDA kabulü yok.
- Sonraki: **6C versioned calibration/evidence persistence ve current resolver → reçete binding/invalidation → ortak Measurement tool/Properties → yetkili validation/Runtime/cycle log**. Webcam/dosya üzerinden devam; endüstriyel SDK fiziksel işleri kamera gelince ayrıca kabul edilir.

<a id="entry-105"></a>
## 7 Ekim 2026 — İş 1/2: webcam tek kare → owned frame → DAG

- Güncel yönlendirme: endüstriyel kamera yok; kamera işleri webcam ile ilerler. Basler/GenTL fiziksel kabulü açık, SDK/form bekleme işleri bu oturumu durdurmaz.
- Kapsam: Qt Multimedia WebcamAdapter, worker üzerinde tek kare host isteği; owned BGR frame ve mevcut DAG executor. Toolbox Webcam ve Properties index/timeout; reçetede tek edinim kaynağı, save/reopen ve source değişiminde görüntü invalidation. UI donanım çağırmaz. Live/external trigger/exposure/gain desteklenmez; index kalıcı cihaz kimliği değildir.
- Kaynaklar: CMakeLists, WebcamAdapter.hpp/.cpp, webcam_probe.cpp, ToolRegistry/ToolDefinition, ToolExecutors, ExecutionService version /2, MainWindowEditor, EditorSmoke/main/header ve webcam_test.cpp. Üretim config/local.ui.ini korunur. Önceki oturumun değişiklikleri korunmuştur.
- Doğrulama: build/webcam-release Release ve CTest 12/12 geçti (webcam UI smoke eklenmeden önce). Sentetik adapter testleri cleanup/buffer reuse, open/read hatası, geç kare, cancel, unsupported format, recovery, save/reopen ve gerçek Blob DAG snapshot yolunu kapsar. Gerçek varsayılan Qt webcam index 0: 1280×720, 948 ms, acquisition DONE ve Blob DAG DONE; build/webcam-release/webcam-probe-default.json. Windows backend ile ayrıca 1005 ms DONE. İlk sandbox probe ERROR, yükseltilmiş yerel cihaz erişimi başarılı; görüntü dosyaya kaydedilmedi.
- Toolchain: OpenCV videoio yerel kurulumda yok; Qt 6.12 Multimedia mevcut kullanıldı. Kurulum yapılmadı. Stop/deadline geç frame kabulünü engeller, driver startup/stop/destruction için hard interruption garantisi yok. Persistent CameraLifecycle streaming bağlantısı ve fiziksel unplug/recovery kabulü değildir. Üretim snapshot şu aşamada webcam aktivasyonunu reddeder; Runtime henüz yok.
- Sonraki: İş 2/2, bağımsız 6C planar scale kalibrasyon/measurement/tolerans servis çekirdeği; webcam UI görsel ve final regression kabulü iki iş sonunda kaydedilecek.

<a id="entry-106"></a>
## 7 Ekim 2026 — 6B GigE tarama/toplam keşif bütçesi düzeltmesi

- Kapsam: GenTLTransport/CameraService liste güncelleme beklemesini toplam producer keşif deadline'ından ayırır. Varsayılan scanWaitMs 250; her çağrı min(scanWaitMs, max(1, remainingMs/2)) kullanır. Sonraki interface/metadata okumaları için süre kalır; geç/cancel sonucu ve kısmi cihaz listesi reddedilir. CLI --scan-wait ve JSON discoveryBudgetMs/scanWaitCapMs eklendi. Vendor call/cleanup hard interruption değildir; load/init toplam keşif bütçesinin dışında kalır.
- Değişen kaynaklar: src/hardware/GenTLTransport.hpp/.cpp, src/application/CameraService.hpp/.cpp, src/app/camera_probe.cpp, tests/gentl_test_producer.cpp ve camera_service_test.cpp. README, backlog, progress, architecture/acquisition ve camera-sdk-installation güncellendi. Önceki oturumun belge değişiklikleri korundu; UI kaynakları ve config/local.ui.ini değişmedi.
- Gerçek producer: Machine GALAXY_* değişkenleri yalnız probe process'ine alınarak Galaxy GxGVTL.cti 1000 ve 3000 ms toplam bütçe/250 ms scan cap ile DONE/0 cihaz verdi. build/6b-galaxy-gige-default-budget.json ve build/6b-galaxy-gige-budget.json. Basler önceki Runtime/producer yolu bu oturumda erişilebilir bulunmadı; build/6b-basler-gige-budget.json ERROR/eksik producer. Kaldırıldığı/taşındığı kanıtlanmadı; geçmiş kurulum exit 0 güncel erişilebilirlik kabulü değildir. Yeni kurulum yapılmadı.
- Doğrulama: build altındaki ana derleme açık visionflow.exe kilidi nedeniyle yalnız UI linkinde durdu; uygulama kapatılmadı. Ayrı temiz build/6b-release Release configure/build başarılı ve CTest **11/11 geçti**. Yeni sentetik C ABI testleri bütün verilen scan süresini kullanan iki interface, toplam bütçeden büyük scan cap, geç metadata sonucu bastırma/RAII cleanup ve geçersiz scan değerini kapsar. Önceki timeout/cancel/recovery ve UI regression korundu. git diff --check geçti.
- Görsel: build/6b-release/step6-camera-test.png incelendi; sentetik cihaz etiketi, read-only açıklaması, tema/menüler/panel düzeni korunur. Test INI/görselleri build altında; üretim ayarı kullanılmadı. Güncel UI binary build/6b-release/visionflow.exe; açık ana build uygulaması eski binary'dir.
- Sınırlamalar: Kısa scan penceresi geç yanıt veren cihazları kaçırabilir; DONE ağda kamera yok kanıtı değildir. Çok sayıda interface/yavaş metadata yine toplam timeout üretebilir; CLI scan/total ayrı ayarlanır. Gerçek kamera yok; trigger/grab/Stop/unplug/recovery, native DPI/dağıtım/CUDA kabulü açık. Basler güncel yol/Developer SDK bağımlılığı sürer.
- Sonraki: **6B dar C ABI/GenTL feature erişimi ve kamera worker/lifecycle/owned frame → DAG bağlantısı**; kamera yapılandırması ve buffer/event cleanup. Basler erişilebilir producer/SDK yolu yeniden doğrulanacak; fiziksel kamera kabulü ayrıca açık kalır.

<a id="entry-107"></a>
## 7 Ekim 2026 — 6B SDK kurulumu ve gerçek ABI/producer kontrolü

- Güncel yönlendirme: kamera bağlı değil; Basler/Daheng, GigE hedefi. Kullanıcı SDK indirme ve kurulumu açıkça yetkilendirdi. Önceki SDK/model bilgisi bekleme kaydı tarihsel kaldı.
- Kurulum: Basler pylon **Runtime 26.09.2** resmi kaynaktan, Galaxy **tam SDK 2.6.2608.9131** VA Imaging açık dağıtıcı paketinden indirildi ve kuruldu. İki installer exit 0; reboot yapılmadı/günlüklerde gerekmedi. Basler /quiet bootstrapper Apply öncesi takıldı, yalnız kendi süreçleri kapatılıp /passive başarıyla tamamlandı. Basler AG Authenticode Valid; Galaxy dış EXE imzasız, kaynak ve SHA-256 kaydedildi. Kurulu driver/dependency/env etkileri ve tam kanıt [camera-sdk-installation.md](../../architecture/camera-sdk-installation.md) içinde.
- Gerçek SDK kontrolü: MinGW x64 dar Galaxy C ABI GXInitLib/UpdateDeviceList/CloseLib **0/0/0**, cihaz sayısı 0. Tam vendor header MSVC enum forward-declaration nedeniyle GCC'de reddedildi; vendor header değiştirilmedi. Process-local DLL dizinleri ve yeni GALAXY environment ile init başarılı. SDK C++ ABI kabulü yok.
- Gerçek GenTL: Basler/Daheng USB producer probe **DONE/0 cihaz**. GigE producer'ları yüklendi, toplam discovery bütçesi aşıldı; **ERROR/TIMEOUT** açık kaydedildi. SDK scan beklemesi ve toplam deadline ayrımı sonraki 6B uygulamasında ele alınacak. NO_PRODUCER önkoşulu çözüldü; canlı grab uygulanmadı.
- Değişen dosyalar: README, docs/progress/backlog/toolchain, architecture/acquisition ve yeni camera-sdk-installation.md. Installer/log/JSON/native ABI probe/görsel build/sdk-install altında. Ürün kaynakları, GUI ve config/local.ui.ini değişmedi.
- Doğrulama: Release build güncel; acquisition, owned frame, lifecycle, transport ve DAG CTest **5/5**. git diff --check. Yeni UI değişikliği/görsel ürün kabulü yok.
- Açık bağımlılık: Basler tam Developer SDK indirme formu zorunlu kişisel/şirket bilgileri istiyor; kullanıcıya bırakıldı. Kurulu Runtime içinde PylonC DLL/header ve Development dizini yok; Basler tam SDK/C API kuruldu denmez. Form tamamlanınca Developer indirme/kurulumu devam edecek.
- Sonraki: 6B GigE discovery bütçe düzeltmesi, dar C ABI/GenTL feature erişimi ve worker/lifecycle/owned frame → DAG bağlantısı. Fiziksel kamera yok; trigger/grab/unplug/Stop/recovery kabulü açık. Donanım yokluğu SDK/servis çalışmalarını durdurmaz; 6B fiziksel olarak tamamlanmadı.

<a id="entry-108"></a>
## 7 Ekim 2026 — 6B başlangıç: gerçek SDK/donanım envanteri

- Kapsam: mevcut keşif/read-only katmanı ve 6B gereksinimleri incelendi; gerçek yerel producer probe ve Windows kamera/kurulu SDK envanteri okundu. Yeni adapter veya canlı grab uygulanmadı.
- Kanıt: environment probe `NO_PRODUCER`, exit 1; producers/devices/errors boş, acquisitionImplemented false. JSON `build/6b-camera-probe-local.json`. Program Files, Program Files (x86) ve OpenCode altında erişilebilir `.cti`/GenApi/vendor dosyası araması sonuç vermedi. Bu arama bütün disk/özel kurulum konumlarını kapsamaz.
- Windows envanteri: Win32_PnPEntity Camera/Image ve bilinen vendor adları filtresinde yalnız Microsoft sürücülü `HD Webcam`, Status OK. HKLM x64/x86 uninstall kayıtlarında Basler/pylon/Daheng/Galaxy/Spinnaker/GenICam/Hikrobot/Allied Vision/IDS Imaging eşleşmesi yok. GigE kamera PnP listesinde görünmeyebilir; fiziksel kamera yokluğu kanıtlanmış değildir. İlk sandbox erişimi engellendi; salt okunur envanter yükseltilmiş erişimle başarıyla alındı.
- Değişen dosyalar: README.md, docs/progress.md, docs/backlog.md, docs/architecture/acquisition.md. Ürün kodu, GUI ve config/local.ui.ini değişmedi.
- Doğrulama: mevcut Release build güncel; tool_execution, acquisition_contract, owned_frame_contract, camera_lifecycle_contract ve camera_transport **5/5 geçti**. Bunlar önceki çekirdek/sentetik ABI davranışını doğrular; gerçek kamera grab kabulü değildir. UI değişikliği yok, yeni görsel kontrol yapılmadı.
- Sınırlama: model/bağlantı/SDK sürümü ve gerçek producer yolu bilinmiyor. Webcam bilgisi GenTL/vendor endüstriyel kamera adapter seçimi için yeterli değildir. SDK kurulumu, ABI, feature erişimi, trigger/grab, disconnect/Stop/recovery fiziksel kabulü açık.
- Sonraki: kullanıcıdan marka/model, USB3/GigE, bağlı cihaz ve SDK/.cti yolu; gerçek discovery/read-only ve x64 C ABI doğrulaması ardından GenApi runtime veya vendor C API seçimi. Sonra aynı worker sahipliğinde configure/trigger/stream/buffer/event cleanup → owned frame → DAG ve fiziksel hata/toparlanma kabulü. 6B tamamlanmadı; güncel kullanıcı isteği doğrultusunda 6C'ye geçilmedi.

<a id="entry-109"></a>
## 7 Ekim 2026 — 6A/A9 persistence planı, bağımsız build ve Git

- Kapsam: Widgets OFF iken domain/application/engine/probe ve dokuz core test derlenir. Qt Core/Gui/Concurrent korunur; viewer ve iki UI testi yalnız UI ON. BUILD_TESTING bağımsız kontrol; UI testleri ui etiketi. Mevcut format/atomik commit/şemalar korunur.
- Kayıt planı: persistence-plan.md manifest SHA-256/length/role, byte/decode bütçeleri, path/entry kontrolü, tek container staging/verify/commit/recovery ve sıralı N→N+1 migration kararlarını tanımlar. Yeni paket loader/import/export/geçmiş uygulaması değildir; 6C/7'de tamamlanır.
- Değişen dosyalar: CMakeLists.txt, .gitignore, tests/project_service_test.cpp, scripts/validate-local.ps1; README, backlog, progress, architecture/development-plan/project-format/architecture, toolchain ve yeni persistence-plan.md. UI ve üretim config/local.ui.ini değişmedi.
- Doğrulama: Release build/core 9/9, build 11/11; validate-local.ps1 temiz build/a9-core 9/9 ve build/a9-ui 11/11 geçti. Ek persistence testleri uncommitted staged write/cancel, oversized save/load ve önceki commit/session korunmasını sınar. Mevcut schema/model migration/roundtrip, eksik/bozuk model ve dependency değişimi regression geçti. Core build.ninja içinde Widgets/ImageViewer/visionflow.exe yok. Sentetik ROI CSV 100/100 doğru; ilk run median 7.15 ms/p95 7.48 ms, gerçek üretim hız kabulü değildir.
- Görsel: güncel build/step5-editor.png ve build/step6-camera-test.png incelendi; 399 px overlay, tema/menüler/paneller ve sentetik read-only kamera ekranı korunur. Test ayarları/görseller build altındadır.
- Kaynak sürümleme: TVision/üst dizinlerde depo yoktu; C:/OpenCode/TVis ayrı C++17 tvis deposudur. C:/OpenCode/TVision/.git main yerel deposu ve baseline kaynak kaydı oluşturuldu; remote yayın yok. Build/data images/log ve local INI ignore edilir.
- Sınırlamalar: Qt Gui core bağımlılığı sürer. Uzak CI runner kurulmadı; disk full/power loss/crash recovery ve yeni container entry kabulü henüz yok. Manifest hash imza/üretim yetkisi değildir. Kalıcı production snapshot/audit/log ve Runtime henüz bağlı değil. Fiziksel grab/SDK, native DPI, kurulum/CUDA kabulü açık.
- Sonraki: **6B** kurulu gerçek producer/SDK/model/ABI envanteri; GenApi/vendor erişimi, trigger/owned frame → DAG entegrasyonu ve disconnect/Stop/recovery kabulü. Donanım bulunmazsa fiziksel kabul açık tutulur, bağımsız 6C kalibrasyon/Measurement servislerine ilerlenir. GUI korunur.

<a id="entry-110"></a>
## 7 Ekim 2026 — 6A/A8 draft/doğrulama/aktif snapshot çekirdeği

- Kapsam: ProductionRecipeService owner-thread çekirdeği; Recipe taslağından ayrı servis kaynaklı validation UUID/UTC/content/dependency hash ve const ProductionSnapshot. İçerik/revizyon/project-directory, model/ROI, engine/registry tool sürümü, resolver asset SHA-256 ve CalibrationDependency manifestine bağlanır. Disk VALID izin değildir. Yapısal scope execution_configuration; gerçek üretim/kalibrasyon/tolerans kabulü değildir.
- Aktivasyon/rollback: güvenilir injectable policy, varsayılan ret; Validate/Activate/Rollback/Run ayrı servis yetkileri. Yalnız aynı servis ömründeki validation kayıtları kabul edilir. Aktif cycle boyunca snapshot değişmez; Activate/Rollback/yeni cycle Busy. Sonraki taslak validate edilebilir. Rollback geçmiş etkin sürüm ve güncel bağımlılıkları kontrol eder. Worker completion recipe/config/frame/cycle/ticket kontrolü ve duplicate reddi; abandon ERROR kaydı.
- Trace/geçmiş: varsayılan 16 snapshot, 1–256 limit; dolunca HistoryFull, otomatik snapshot silme yok. Definition 16 MB, asset manifest 256; toplam byte arşivi sonraki adım. Son 128 transient cycle JSON trace gerçek tool sonuçlarını ve recipe/version/engine/model/calibration/source/frame kökenini taşır; image raw loglanmaz, iletişim NOT_REQUESTED, PASS/pose yok.
- Değişenler: yeni src/application/ProductionRecipeService.hpp/.cpp ve tests/production_recipe_test.cpp; ExecutionService.hpp engine version; MatchService.hpp/.cpp ortak model validation decoder; CMakeLists.txt. README, backlog, progress, architecture/development-plan/project-format/tool-execution ve yeni production-recipes.md güncellendi. Root/recipe/editor schema ve UI kaynakları değişmedi.
- Doğrulama: Release build ve CTest **11/11 geçti**. Yeni production_recipe_contract üç applicationType ortak servis yolu, gerçek PNG/DAG ve numeric trace, kayıp edinim dosyasında ERROR/parça kararı ayrımı, draft izolasyonu, activation boundary/rollback, yetkisiz/forged/restart validation, değişmiş içerik/model/asset/engine/calibration, corrupt/missing model, disabled/missing DAG input, owner-thread, yabancı/stale/duplicate cycle result, history/log sınırı ve abandon ERROR kabulünü kapsar. Önceki Match/teaching/metadata/session/GenTL/UI regression korundu.
- Görsel: güncel build/step5-editor.png ve build/step6-camera-test.png incelendi; 399 px measurement overlay, tema/menüler/paneller ve sentetik discovery/read-only ekranı korunur. Test ayarları/fixture'lar build altındadır; üretim config/local.ui.ini kullanılmadı.
- Sınırlamalar: Runtime UI/controller, Users kimlik doğrulama, kalıcı snapshot/audit/cycle logları henüz bağlı değil. Dış image edinim girdisi değişkendir; Validate dosyayı açmaz, gerçek edinim cycle'da hata verebilir. Asset resolver/policy güvenilir application boundary'dir. CalibrationDependency kaydı fiziksel katsayı/doğruluk kabulü değildir. Kalibrasyon hesabı/mm/tolerans/part decision 6C; fiziksel kamera 6B açık. Native DPI, kurulum ve CUDA kabulü yok.
- Sonraki: **6A/A9** manifest/varlık bütünlüğü ve persistence/migration planı, Widgets kapalı bağımsız core build/test; Git konumunu doğrula, yoksa yerel depo kur. Mevcut atomik commit/şemalar korunacak; gerçek paket/geçmiş ve Runtime entegrasyonu 6C/7.

<a id="entry-111"></a>
## 7 Ekim 2026 — 6A/A7 koordinat/tracking ROI çekirdeği

- Kapsam: domain CoordinateFrame, Point2D, Pose2D, rigid Transform2D; source→target yönü, inverse/then sırası, radyan, px/mm, Y-down/Y-up ve integer pixel-centre konvansiyonu. Araçtan bağımsız TrackingRoi teaching/current pose ile lokal dört köşeyi dönüştürür; uyumsuz frame/unit/axis ve dejenere/sonlu olmayan geometri reddedilir. Rigid dönüşüm px→mm/axis reflection yapmaz.
- Entegrasyon/migration: Edge referenceOrigin/current point ortak poseTransform/inverse/then yolunu kullanır; delta bir kez yuvarlanır, kutu boyutu korunur, sınır dışı ERROR olur. Engine tüketiminde gerçek PortValue type/unit/frame beklenen input descriptor ile kontrol edilir. Editor schema 5/root-recipe schema 1 değişmedi; schema 3/4 migration, referenceGeometry invalidation, no-op/dirty ve save/reopen korunur.
- Kalibrasyon sözleşmesi: CalibrationDependency kimlik/sürüm/içerik/kamera/lens/çözünürlük/düzlem/source-target frame alanlarını taşır; birebir current dependency kontrolü ve strict schema 1 JSON roundtrip vardır. Eksik/ekstra/future/tip/birim hataları reddedilir. Reçete kalibrasyon kaydı, katsayı/kanıt ve execution invalidation 6C işidir; kayıt tek başına kalibrasyon veya pose sonucu değildir.
- Değişen dosyalar: src/domain/Coordinates.hpp/.cpp yeni; src/application/ToolExecutors.cpp, ExecutionService.cpp; tests/coordinates_test.cpp yeni, frame_test.cpp; CMakeLists.txt. README, backlog, progress, architecture/development-plan/tool-execution/matching-editor/editor-definition ve yeni coordinates.md güncellendi. UI kaynakları/menüler/tema ve üretim config/local.ui.ini değişmedi.
- Doğrulama: Release configure/build başarılı; CTest **10/10 geçti**. coordinates_contract bilinen öteleme/π/2 dönme/sıra/inverse, pose açı bileşimi, mm/Y-up, lokal offset ve bütün köşeler, nonzero teaching roundtrip, frame/unit/axis, NaN/Inf, dejenere/crossed ROI, yarım piksel ve sınır kabulünü kontrol eder. CalibrationDependency değişen her bağımlılığı ve bozuk/future JSON'u reddeder. owned_frame_contract yanlış unit/type/port-frame runtime ölçüm engelini kontrol eder; mevcut tool_execution ve UI teaching/migration/invalidation/reopen regression geçti.
- Görsel: build/a7-reference-edge.png ve build/a7-editor.png güncel offscreen smoke görsellerinden kopyalanıp incelendi; Edge kutusu/kenar ve Calculate çizgi/px overlay, tema/menüler/paneller korunur. Test ayarları/görseller build altındadır; üretim INI kullanılmadı.
- Sınırlamalar: Genel dönmüş ROI/Pose2D domain çekirdeğidir; editor Pose portu, Match açı/ölçek ölçümü, dönmüş Edge/Blob raster sampling, kalibrasyon hesabı/activation ve robot çıktısı uygulanmadı. Genel ROI JSON reçeteye eklenmedi; legacy kayıt korunur. Physical grab kabulü 6B, kalibrasyon bağımlılığının gerçek sonuç invalidation'ı 6C, robot frame/TCP/pick reference 6E'dedir. 2D'den tam 3D pose çıkarılmaz. Sentetik fixture; native DPI/kurulum/CUDA kabulü yok.
- Sonraki adım: **6A/A8** draft/doğrulama/aktif üretim değişmez snapshot sözleşmesi, engine/tool/model/kalibrasyon bağımlılığı ve aktivasyon/rollback/cycle sınırı; gerçek Runtime ve yetki/log entegrasyonu 6C. Mevcut GUI korunur.

<a id="entry-112"></a>
## 7 Ekim 2026 — 6A/A2 model/geometri referans invalidation teslimi

- Kapsam: Edge referenceGeometry SHA-256 kimliği kaynak UUID/kind ve Match modelRoi/modelPng içeriğine bağlıdır. Model merkezi, boyutu veya PNG değişince engine Edge ERROR ve downstream ölçüm SKIPPED üretir; eski kutu/point/line yayımlanmaz. Search ROI yeniden çizimi mevcut referans merkezi ve kimliğiyle ilişkiyi günceller. İsim/açıklama, enabled, Min Score ve Match search ayarı kimliği değiştirmez. Yeni source bağlantısı eski kimliği otomatik onaylamaz; no-op bağlantı değişimi invalidation'ı temizlemez.
- Kayıt: Editor schema 5; root/recipe schema 1 korunur. Schema 1–4 açılırken eski kutu/model/UUID/origin korunur, mevcut kaynak başlangıç kimliği olur. Açma/no-op dirty yapmaz; ilk gerçek değişiklik schema 5 yazar. Mevcut legacy modelin geçmişte değişip değişmediği kanıtlanamaz. Eksik schema 5 kimliği yürütmede yeniden öğretim ister. Invalid durum save/reopen boyunca kimlik uyuşmazlığıyla korunur.
- Değişen kaynaklar: src/domain/ToolDefinition.hpp/.cpp, ToolGraph.cpp; application/ProjectService.cpp, ExecutionService.cpp; app/MainWindowEditor.cpp, MainWindowMatching.cpp, EditorSmoke.cpp; tests/execution_test.cpp, editor_services_test.cpp. README, backlog, progress ve development-plan/architecture/editor-definition/matching-editor/tool-execution belgeleri güncellendi.
- Doğrulama: Release build başarılı; CTest **6/6 geçti**. Core model merkezi ve aynı merkezde yeni model içeriği değişimi, metadata/search/threshold no-invalidation, stale output temizliği, downstream engeli, save/reopen, no-op bağlantı, schema 3/4 migration ve eksik kimlik kabulünü kontrol eder. UI gerçek model teaching worker, kaydet/aç, ERROR, Search ROI yeniden öğretim ve DONE toparlanmasını doğrular.
- Görsel kontrol: build/a2-reference-invalid.png ve build/a2-reference-restored.png incelendi. Eski kutu/kenar bastırılır; yeniden öğretimde doğru mavi kutu/yeşil kenar geri gelir. Tema, menüler ve panel yerleşimi korunur; mevcut Properties içinde açıklama eklenir. Üretim config/local.ui.ini kullanılmadı.
- Sınırlamalar: Sentetik fixture; fiziksel kamera, native DPI, kurulum/CUDA kabulü yok. Legacy geçmiş referans değişimi saptanamaz. Match dışı point kaynaklarında parametre kimliği konservatif kullanılır; genel tracking/kalibrasyon sürümleri A7/6C kapsamında. Kimlik içerik temellidir: birebir eski model/geometriye dönüş aynı ilişkiyi yeniden geçerli kılar. Öğretim UI orchestration'ının application session'a ayrılması A4'te kalır.
- Sonraki adım: **6A/A3** statik ToolRegistry, ayrı tool yürütücüleri, ortak parametre doğrulama; mevcut port, execution, GUI ve regression davranışlarını koru.

<a id="entry-113"></a>
## 7 Ekim 2026 — 6A/A1 Run Match / Run Flow ortak yürütme teslimi

- Kapsam: Run Match, Run Flow ile aynı ExecutionService ve UI worker yaşam döngüsüne taşındı. Seçili tool ve transitif upstream alt grafiği çözülür; ilgisiz dallar çalıştırılmaz. Bağlı Image/Region, disabled/missing/invalid/error upstream, not-found, Stop ve stale-result kuralları ortaktır. Her iki UI komutu kaynağı yeniden edinir; önizleme/cache execution girdisi olarak kullanılmaz.
- Snapshot: Servis aynı immutable ImageFrame ile tam/tek tool yürütmesini destekler; yeni cycle/frame kimlikleri atanır. UI her çağrıda yeni edinim yapar. Referans öğretim cache kimliği proje/reçete/path/frame generation ve upstream tool/bağlantı yapılandırmasını kapsar; bağımsız Edge ayarı güncel match merkezini korur.
- Değişen kaynaklar: `src/application/ExecutionService.hpp/.cpp`, `src/app/MainWindow.hpp`, `MainWindowMatching.cpp`, `MainWindowExecution.cpp`, `EditorSmoke.cpp`, `tests/execution_test.cpp`. Belgeler: README, backlog, progress, architecture, development-plan, tool-execution, matching-editor. Şema ve yerel üretim INI ayarları değişmedi.
- Doğrulama: Release derleme başarılı; son CTest **6/6 geçti**. Core aynı frame/config üzerinde bağlı ROI'nin yerel ROI önüne geçmesini ve skor/konum eşitliğini; upstream disabled/missing/port/cycle/adapter error, ilgisiz eksik dal, not-found ve cancellation çıktı temizliğini kontrol eder. UI gerçek input combo ile image bağlantısı, önizleme var ama image bağlantısı yokken hata, dosya dışarıda değişince yeni edinim, Stop, close guard, stale config ve save/reopen kabulünü doğrular. Mevcut reference tracking regression korunur.
- Görsel kontrol: `build/a1-match.png` (son smoke `step4-editor.png` kopyası) ve `build/reference-edge.png` incelendi. Menü, tema, düğme konumları ve paneller korunur; Run Match Results artık cycle/frame ve typed tool çıktıları gösterir. Üretim `config/local.ui.ini` kullanılmadı.
- Sınırlamalar: Test görüntüleri sentetiktir; gerçek kamera/grab, native DPI, kurulum ve CUDA kabulü yoktur. Stop cooperative; devam eden OpenCV/decode zorla kesilmez. Reçete format doğrulaması tüm definition için kalır; yalnız graph çalıştırılabilirlik doğrulaması seçili alt grafiğe sınırlanır. Model yeniden öğretiminde bağlı Edge geometri invalidation henüz yoktur; A2 açık kalır. Yerel Git deposu bulunmadı; kaynak sürümleme kararı A9'da izlenir.
- Sonraki adım: **6A/A2** model/geometri kimliğine bağlı referans invalidation, yeniden öğretimde Edge/downstream engeli, save/reopen ve eski şema migration kabulü. Mevcut GUI korunacak.

<a id="entry-114"></a>
## 7 Ekim 2026 — Genel inceleme sonrası plan revizyonu

- Kapsam: Kullanıcı dokuz mimari önerinin tamamını plana aldı; GUI görünümü ve menü değişikliği istemedi. Yalnız dokümantasyon güncellendi; kaynak kodu, tema, menü, CMake veya üretim ayarları değiştirilmedi.
- Yeni belge: `architecture/development-plan.md`; A1–A9 iş paketleri, 6A–6E sırası, çıkış koşulları, gerçek donanım kabulü ve GUI koruma şartı.
- Güncellenen belgeler: `README.md`, `docs/backlog.md`, `docs/progress.md`, `docs/architecture/architecture.md`, `tool-execution.md`, `matching-editor.md`, `editor-definition.md`, `acquisition.md`, `project-format.md` ve üç `docs/requirements/` belgesi.
- Doğrulama: Yerel Markdown bağlantıları ve plan referansları kontrol edildi. Yalnız belge değişikliği olduğundan yeniden build/CTest/görsel kabul gerekmedi. Önceki incelemede mevcut Release build güncel ve CTest 6/6 idi; bu sonuç planlanan refactor'ların kabulü değildir.
- Sınırlamalar: Bütün yeni öneriler plan durumundadır. Mevcut Run Match ayrılığı, model yeniden öğretimindeki referans riski, tek kaynak/frame yaklaşımı ve cooperative timeout henüz düzeltilmedi. Fiziksel producer/grab, kurulum ve CUDA kabulü yoktur.
- Sonraki adım: 6A/A1 için aynı snapshot üzerinde tek tool/tam flow giriş çözümlemesini birleştirmek; ilgili regression kabulü ardından A2'ye geçmek. Mevcut GUI korunacak.

<a id="entry-115"></a>
## Hazırlıkta tamamlananlar

Ürün/UI belgeleri `docs/requirements/`, HTML `prototypes/` altında. CMake, MinGW, Qt ve OpenCV yerel derleme doğrulandı. CPU adapter `cv::matchTemplate` / `TM_CCOEFF_NORMED`; CUDA adayı belgelendi, etkin değil. Sentetik 5 MP tam kare CPU <10 ms hedefini sağlamadı; 512×512 ROI p95 yaklaşık 6–9 ms. Ayrıntılar ve CSV `docs/performance/` altında. Benchmarkı UI adımında tekrar çalıştırmak gerekmez.

<a id="entry-116"></a>
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

<a id="entry-117"></a>
## Adım 2 başlangıç planı (gerçekleştirildi)

1. Domain Project/Recipe modellerini kalıcı ID, `schemaVersion`, `recipeVersion`, `applicationType`, DRAFT/VALID/INVALID alanlarıyla oluştur.
2. Application/persistence servisleriyle proje ve reçete create/open/save; atomik kayıt ve hatada değişiklik koruması ekle.
3. Project Manager formu ve Recipe Manager tablosu, tür filtresi ve New Recipe tür seçimini uygula.
4. Yeni reçete DRAFT olsun ve aynı editor'e açılsın; aktif proje/reçete bağlamını başlıkta göster.
5. Save/reopen, tür/ID/sürüm korunması ve hatalı dosya davranışlarını doğrula; progress dosyasını güncelle.

HTML demo JSON üretim şeması değildir. Adım 2 gerçek vision veya hardware çalıştırmaya genişletilmemeli; editor interaction Adım 3'te yapılacak.

<a id="entry-118"></a>
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

<a id="entry-119"></a>
## Adım 3 başlangıç planı (gerçekleştirildi)

1. Mevcut ProjectService/domain/schema'yı koru; UI'dan doğrudan OpenCV/dosya işlemi ekleme.
2. Image-file acquisition servisi ve gerçek image viewer: Open Image, Fit, Zoom, Pan; dosya okuma hatası ve görüntü/piksel koordinatlarını göster.
3. Tek Recipe Editor'de Toolbox, seçili tool bağlamı ve Properties senkronizasyonunu uygula. İlk tool kayıt sözleşmesini minimal ve ileride typed flow'a genişletilebilir tasarla.
4. Görüntü kaynağı/tool ayar değişiklikleri dirty/DRAFT yapsın; save/reopen yapılandırmayı korusun. Recipe `definition` ayrılmış alanını kullanırken sözleşmeyi belgele; şema değişikliği gerekiyorsa açık migration kararı ver.
5. Gerçek ROI çizimi ve match teaching Adım 4'te kalır; gerçek tool sonuçları yokken sahte PASS/score gösterme.
6. Yeni modüllere uygun test/görsel doğrulama yap, README/mimari/progress kaydını güncelle.

<a id="entry-120"></a>
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

<a id="entry-121"></a>
## Adım 4 başlangıç planı (gerçekleştirildi)

1. AGENTS/progress ve editor-definition sözleşmesini oku; mevcut üç adımı yeniden oluşturma.
2. ImageViewer üzerinde model ROI ve search ROI çizme/seçme/düzenleme ekle; görüntü koordinatında clamp/validation, seçili tool ile overlay bağını koru.
3. ImageFrame ham verisini UI'ya açmadan vision/application servisleri üzerinden match adapter'e bağla. Gray conversion/16-bit stratejisini açıkça tanımla.
4. Model teaching → template oluşturma/kaydetme → gerçek cv::matchTemplate → skor/konum/overlay zincirini uygula. Timeout/çoklu match/angle-scale kapsamını desteklenmediği halde etkinmiş gibi gösterme.
5. Editor schema 1'in yeni ROI/model parametreleri için migration kararını belgele; eski dosyaların ID ve minScore değerleri kaybolmasın. Model varlığı kaydı ile atomik project commit arasındaki tutarlılığı tasarla.
6. Frame/config değişiminde eski match sonucunu geçersiz kıl. Reçete VALID yapmak için gerçek doğrulama olmadan status yükseltme; Runtime'da sahte sonuç gösterme.
7. Gerçek pozitif/negatif fixture'larla konum/skor, missing model/image, ROI sınırları ve save/reopen doğrulaması yap. Dar ROI performansını ölç; tam 5 MP <10 ms sağlandığını CPU için iddia etme.
8. README, mimari ve bu ilerleme dosyasını güncelle. Genel typed tool/DAG ve Blob/Edge/Calculate yürütmesi Adım 5'te kalır.

<a id="entry-122"></a>
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

<a id="entry-123"></a>
## Adım 5 başlangıç planı (gerçekleştirildi)

1. AGENTS/progress/backlog ve matching-editor sözleşmesini oku; tamamlanan işleri yeniden oluşturma.
2. Typed port/tool/domain sözleşmesini Image/Region/Point/Number/Boolean ve px/frame/cycle birimleriyle tanımla. Mevcut UUID ve modelleri koruyacak açık editor migration tasarla.
3. DAG validation: missing tool/port, type/birim uyumsuzluğu, cycle; Flow UI üzerinde gerçek bağlantıları yapılandır.
4. Genel background execution servisi kur; acquisition, match ve Blob/Edge/Calculate adapter'larını UI'dan bağımsız yürüt. READY/BUSY/DONE/ERROR/SKIPPED, recipe durumundan ve PASS/FAIL'den ayrı olsun.
5. Sonuçlar frame/config/cycle kimliğine bağlı olsun; disabled tool, hata, iptal ve stale result davranışını koru. Otomatik proje restore özelliği korunmalı.
6. Gerçek Blob/Edge/Calculate girdileri ve sonuç/overlay; deterministic fixture, invalid graph, yürütme hatası, save/reopen ve UI kabulü.
7. README/mimari/backlog/progress güncelle. Hardware/Runtime Adım 6; üretim kamera matching doğrulaması ayrı kabul işi olarak kalır.


<a id="entry-124"></a>
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

<a id="entry-125"></a>
## Sonraki oturumda Adım 6

1. Progress/backlog/README ve tool-execution sözleşmesini oku; Adım 5'i yeniden oluşturma.
2. Kamera adapter capability/SDK/toolchain uyumunu mevcut donanım bilgisiyle belirle; UI SDK çağırmasın. Cihaz bilgisi eksikse gerekli kullanıcı bilgisini iste.
3. Frame/trigger/cycle sözleşmesini koruyan kamera acquisition ve hata/timeout davranışını uygula.
4. Kalibrasyon model/validity/frame/unit ve veri kabul sözleşmesini oluştur; geçersiz kalibrasyonda mm/robot pose yayımlama.
5. Robot/PLC bağlantı/output handshake servisleri ve ortak Runtime'ı aşamalı uygula; robot hareketi ile sonuç gönderimini ayrı tut.
6. Test/görsel/README/mimari/progress kanıtlarını güncelle. Kurulum/yetki/loglar Adım 7; üretim matching kabulü ayrıca devam eder.

<a id="entry-126"></a>
## 7 Ekim 2026 — tool silme ve overlay düzeltmesi

Adım 6 öncesi kullanıcı önceliği tamamlandı. ProjectService setTools silinen tool ile ilgili gelen/giden bağlantıları aynı definition güncellemesinde temizler. Bağımlı gerekli girişler yeniden bağlanmalıdır. UI ekle/sil/yeniden Run ve core bağlı tool silme regression testleri eklendi.

Edge ROI boyunca yeşil profile konum çizgisi gösterir; contour değildir. Calculate P1/P2, sarı çizgi ve px mesafe etiketi gösterir. Nokta/metin zoom boyutundan bağımsızdır. İptal/stale overlay temizliği korunur. ProjectService, ExecutionService, ImageViewer, MainWindowExecution, EditorSmoke ve execution_test değişti.

Release derleme ve CTest 4/4 geçti. Edge çizgisi/measurement koordinatları, referans temizliği ve kalan DAG recovery doğrulandı. build/step5-editor.png ve build/fix-edge.png incelendi; 399 px sarı çizgi ve yeşil kenar görünür. Sentetik fixture; native DPI ayrıca doğrulanmalı. Sıradaki Adım 6 planı korunur.

<a id="entry-127"></a>
## 7 Ekim 2026 — Match merkezine bağlı Edge arama takibi

Kullanıcının major referans hatası düzeltildi; Adım 6 öncesi önceliklendirildi. Edge optional reference Point/px/image portu eklendi. Öğretilen Search ROI ile referenceOrigin merkezi birlikte saklanır. Runtime kutu = taughtROI + round(currentReference - taughtReference); kutu boyutu ve merkezle offset ilişkisi korunur. Mevcut cycle/Run Match merkezi kullanılır; güncel görüntüdeki match cache parametre/görüntü değişimlerine karşı kontrol edilir. Sonuç yoksa model öğretim merkezi başlangıç noktasıdır; hareketli görüntü üzerinde önce Run Flow/Match çalıştırılmalıdır. Edge parametre değişimi sonrası aynı görüntü üzerinde ROI yeniden çizimi güncel merkeze bağlanır.

Editor schema 4; schema 1/2/3 okunur. Eski schema 3 Match.region → Edge.region, Match.point → Edge.reference olarak migrate edilir; teaching model merkezi anchor olur, çizilmiş kutu/UUID/model korunur. Eski kayıtta Search ROI yoksa çizmek gerekir. Yeni UI Match.region yerine Referans merkezi alanından Match.point bağlar; region/reference aynı anda bağlanamaz. Runtime kullanılan kutu mavi ROI ile gösterilir. Reference missing/disabled/error → Edge ve measurement SKIPPED. Taşınmış ROI görüntü dışında veya referans modunda kutu eksikse ERROR; tam kareye sessiz fallback yok. Cancel gerçek searchRegion dahil overlay temizler.

Değişenler: ToolGraph/ToolDefinition, ExecutionService, MainWindowEditor/Matching/Execution.hpp/.cpp, EditorSmoke, execution_test/editor_services_test; README/backlog/tool-execution sözleşmesi. Yeni port/schema future rejection sınırı 5.

Doğrulama: Release build ve CTest 4/4 geçti. Core ayrı model ile uzak kenar fixture: +30/+15 ve -10/-10 px hareket, kutu boyutu/offset korunması, gerçek taşınmış Edge noktası, iki kenar arasında invariant 39 px measurement, save/reopen, referans bulunamaması, görüntü dışı kutu ve eksik teaching ROI. Schema 3 bağlantı/anchor migration çalıştırılarak doğrulandı. UI gerçek input combo, ROI teaching, +30/+20 px hareket, Edge noktasının (230,209.5) olması ve moved frame üzerinde property değişimi ardından yeniden teaching doğrulandı. Run Match cache geometric center ayrıca regression testinde kontrol edildi. build/reference-edge.png incelendi: mavi kutu ve yeşil kenar parçayla taşınıyor; Properties combo minimum içerik boyutu dar panele uyarlandı; dar panelde yatay kaydırma hâlâ kullanılabilir. Son combo yerleşim değişikliği sonrası ui_shell_smoke tekrar geçti.

Sınırlar: translation X/Y ve px; açı/ölçek/kalibrasyon henüz yok. Model yeniden öğretildiyse Edge referans ilişkisini kutuyu yeniden çizerek öğretin. Sentetik fixture üretim kamera kabulü değildir. Devam: Adım 6 planı korunur.


<a id="entry-128"></a>
## 7 Ekim 2026 — Adım 6.1 acquisition altyapısı

Kapsam: Kullanıcı kamera mimarisi seçimini geliştirmeye bıraktı. GenICam/GenTL ortak model, gerektiğinde Basler pylon/Daheng Galaxy adapter kararı verildi. AcquisitionService request/result/capability sözleşmesi ve gerçek ImageFileAdapter eklendi; Run Flow bağlandı. Source/cycle/frame kimliği, UTC completion timestamp, unsupported trigger, ERROR/CANCELLED/TIMEOUT durumlarında frame bastırma, geç frame reddi ve Stop token korundu. Editor schema 4 ve UI değişmedi.

Değişen dosyalar: src/application/AcquisitionService.hpp/.cpp (yeni), ExecutionService.hpp/.cpp, tests/acquisition_test.cpp (yeni), CMakeLists.txt, README.md, docs/architecture/acquisition.md (yeni), architecture.md, backlog.md, progress.md.

Doğrulama: CMake cache Release; derleme başarılı. CTest 5/5 geçti: ui_shell_smoke, project_persistence, editor_services, tool_execution, acquisition_contract. Yeni test gerçek PNG/piksel, kimlik/UTC, capability, unsupported trigger, invalid request, pre/post cancellation, late frame/error, empty/missing frame ve recovery kapsar. Test varlıkları build altında; üretim ayarları kullanılmadı. UI değişikliği olmadığı için yeni görsel kabul gerekmedi.

Sınırlar: blocking decode zorla kesilmez; timeout geç çıktıyı bastırır. Run Flow dosya edinim sınırı 5000 ms; Editor Open/Reload önceki worker decode yolunda kalır. Kamera adapter ve GenTL consumer henüz uygulanmadı; standart üst düzey klasör taramasında SDK bulunmadı. Fiziksel kamera, MinGW/vendor ABI, kurulum ve kamera latency doğrulanmadı. Kalibrasyon/robot-PLC/Runtime bekliyor; sahte PASS/pose yok.

Sonraki adım: gerçek SDK/GenTL producer tespiti ve ABI kontrolü; discovery/open/close/trigger/grab, owned buffer conversion, disconnect/timeout acceptance. Sonra kalibrasyon ve ortak Runtime. Adım 6 bütünü tamamlanmış değildir.


