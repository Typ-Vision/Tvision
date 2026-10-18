# ICM görev kartları — kayıt, kalibrasyon, Editor ve Runtime

Bütün kartlar başlangıçta **PLANLANDI**. [Ana plan](industrial-camera-plan.md),
[sözleşmeler](industrial-camera-contract-plan.md) ve [kabul matrisi](industrial-camera-acceptance-plan.md)
bağlayıcıdır. UI kartları Release, ilgili CTest ve normal/küçük görsel kontrol ister.
Fiziksel test yoksa sonuç yazılım kabulü olarak kaydedilir.

**18 Ekim bağımlılık kararı:** 05a–09a ortak yazılım görevleridir; 04f test adaptörü
kabulünden sonra native OECS beklenmeden tamamlanabilir. Testlerde sentetik
backend kullanılır; normal ürün yolu yalnız doğrulanmış native capability ile gerçek
edinim yapar. SDK'sız/unsupported cihaz için kayıt ve offline düzenleme yapılabilir,
edinim açık hata verir. Native kabul [ayrı paketlerdedir](industrial-camera-tasks-backends.md).

## ICM-05a — ortak keşif, registry ve adapter factory

**Yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-05a.md).

**05a-a bağımsız hazırlığı tamam:** CameraService üzerinde fixture snapshot gruplama
ve exact resolve; SDK enumeration, ortak webcam DTO, kayıt/factory/lease bağlantısı
yoktu. Ana 05a kabulü yukarıdaki teslimle kapandı; native kapılar açıktır.

- Önkoşul: 04f; tamamlanan 05a-a yeniden kullanılır. Native tetik ediniminin açılması
  ayrıca kendi backend G1 kanıtına bağlıdır; keşif/open/preview ayrı yeteneklerdir.
- Dosyalar: CameraService, MeasurementWorkspace, MeasurementEnvironment,
  ScopedActiveCalibration ve yeni application provider/factory.
- İş: webcam + industrial discovery tek DTO; yerel producer route, dedup ve exact
  resolve. UI discovery donanımdan frame almaz. Calibration/Editor/Runtime aynı factory/lease'i kullanır.
- Kabul: 0/1/N cihaz; iki producer aynı cihaz; offline/ambiguous/busy/SDK missing;
  keşif değişimi yanlış CameraId'yi çalıştırmaz; dosya/webcam çalışmaya devam eder.
  Test adaptörü yalnız test composition'ından enjekte edilir. Normal factory,
  sentetik veya henüz doğrulanmamış native backend'e tetik edinimi/production izni vermez.
- Test: camera_transport, measurement_environment_persistence, yeni provider testleri.
- Kapsam dışı: UI, üretim whitelist açma, aynı kamerayı N-source gibi sayma.

## ICM-05b — Camera Manager ve kamera ekleme UI

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-05b.md).
Native capability yokken ayar uygulama/preview kapalıdır; offline taslak düzenlenebilir.

- Önkoşul: 05a.
- Dosyalar: MainWindowCamera, CalibrationOverview, application command DTO'ları.
- İş: discovered/kayıtlı/offline ayrımı; kayıt/eşleme, seri/model/transport ayrıntısı,
  desteklenen basic exposure/gain/timeout, gerçek readback, seçili kamera preview.
  Producer teknik yolu ayrıntıda; günlük kullanımda kamerayı tekrar tekrar `.cti` seçerek ekletme.
- Kabul: kayıt başka kalibrasyon listesinde de aynı CameraId olarak görünür; busy
  ayar yazımını engeller; preview sonucu ölçüm/PASS olmaz; Stop stale raster etiketini temizler.
- Test/görsel: 0/1/8/10 fixture kartı, uzun model/seri, küçük pencere, worker kapanışı,
  eksik producer ve hata durumu. Gerçek N cihaz kabulü olarak sunulmaz.
- Kapsam dışı: gelişmiş vendor feature tree veya bütün kameraları arka planda açma.

## ICM-06a — reçetesiz kalibrasyon edinimi

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-06a.md).

- Önkoşul: 05a.
- Dosyalar: IndependentCalibrationCapture, CalibrationWizardSession,
  CalibrationCapture, MeasurementWorkspace; ilgili wizard UI yalnız gerekirse.
- İş: webcam-only factory/validation sınırlarını ortak industrial sözleşmesi için
  genişlet; testte fixture, gerçek edinimde doğrulanmış native backend kullan.
  Exact device/setup/profile snapshot ile owned raw frame kullan.
  Session ticket/cancel/generation ve owned math input yolunu koru.
- Kabul: recipe yokken gerçek/fixture industrial frame → Edge/Caliper/plaka hesabı;
  wrong device/geometri/late frame ret; trial/setup_file origin relabel edilemez.
- Test: independent_calibration_capture, calibration_wizard_contract,
  calibration_plate_contract ve industrial fixture. Kaydet adaydır, otomatik activation yok.
- Kapsam dışı: intrinsic/distortion solver veya eski kayıtları yeni kamera adına yeniden yazma.

## ICM-06b — activation ve kurulum invalidation

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-06b.md).

- Önkoşul: 06a, 03b.
- Dosyalar: ScopedActiveCalibration, SystemCalibration, MeasurementWorkspace,
  calibration activation UI ve scope codec/transfer.
- İş: actual camera probe/profile/geometri ile model uyumu; optik/geometry ve exposure
  ayrımı; unplug'da availability kaybı; replug/restart açık activation.
- Kabul: aynı raster boyutu farklı offset/reverse eski modeli kullanamaz; exposure
  değişimi model matematiğini silmez fakat cache/Runtime hazırlığını geçersiz kılar.
  Kullanılmayan offline kalibrasyon raw-only işi engellemez; aktivasyon fiziksel ölçüm PASS'i değildir.
- Test: scoped_active_calibration, system_calibration_contract,
  measurement_setup_migration, ui_scoped_calibration_activation; old scope replay.
- Kapsam dışı: kalibrasyon sınırlarını gevşetmek veya yeni kamera için otomatik legacy fallback.

## ICM-07a — industrial source tool ve migration

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-07a.md).
Industrial Camera/1 açık ekleme ile editor9 taslağıdır; seçim/edinim UI 07c,
execution 07b, production admission 08a/b kapsamındadır.

**07a-a bağımsız hazırlığı tamam:** standalone IndustrialSourceReference1 codec,
versioned kamera/setup/profile ve requested acquisition hash kontrolü; 07a bunu
registry ve Editor/project codec'e bağladı. Eski webcam dönüştürülmez.

- Önkoşul: 05a.
- Dosyalar: ToolRegistry/ToolDefinition/ToolGraph, ProjectService, source/profile codec.
- İş: ICM-00'da sabitlenen industrial tool kind/version, Image/px port, CameraId/setup/profile
  reference ve timeout. Editor explicit ekleme; registry capability görünürlüğü.
  SystemCalibration kaynak seçimindeki image_file/webcam sabit listesini kontrollü genişlet.
- Kabul: old project/recipe/tool decode ve no-op Save/hash aynı; yeni kaynak yalnız
  explicit edit ile yazılır. Unknown profile/backend/unsupported trigger ret; production hâlâ kapalı.
- Test: tool_migration_contract, project_persistence, multi_source_graph_contract,
  tool_creation_policy ve source codec fixture'ları.
- Kapsam dışı: otomatik webcam→industrial migration veya implicit profile overwrite.

## ICM-07b — execution ve TargetOnly

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-07b.md).
Immutable application girdileriyle provider/lease edinimi ve scoped model/cache
bağlantısı hazırdır; kullanıcı profil seçimi 07c'de, production 08a/b'de kalır.

- Önkoşul: 07a, 06b.
- Dosyalar: ToolExecutors, ExecutionService, SourceFrameIndex, EditorSession,
  MeasurementSourceIdentity ve source-scoped resolver.
- İş: ortak adapter factory/lease üzerinden endüstriyel edinim; immutable source/profile
  inputs; gerçek frame/actual evidence. Multi-source sıralı bütçe ve branch model eşlemesini koru.
- Kabul: industrial+file/webcam karışımı, kaynak başına ROI/cache; source refresh yalnız
  kendi dalını temizler; TargetOnly acquisition0; read sırasında Stop çıktı üretmez.
  Aynı fiziksel kamera iki source'ta ret; private test adapter yolu production'a açık olmaz.
- Test: multi_source_execution/target_only, editor_target_only,
  scoped_calibration_editor ve source/frame swap negatif corpus'u.
- Kapsam dışı: eşzamanlı pozlama/PTP veya production admission kapısını erken açma.

## ICM-07c — ortak Recipe Editor UI

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-07c.md).
Kayıtlı profil seçimi ve worker bağlantısı hazır; native edinim kapıları korunur.

- Önkoşul: 07b, 05b.
- Dosyalar: MainWindowEditor/Execution, Properties ve tool creation UI.
- İş: Industrial Camera source ekle → kayıtlı kamerayı/kurulumu/profili seç → kare al;
  actual raster/ROI ve seçili kaynak overlay'i; explicit profile edit etkisini göster.
  Üç applicationType aynı Editor'ü kullanır.
- Kabul: kaydet/aç, offline düzenleme, yanlış kaynak/geometri hata tanısı; listeye tıklamak
  hardware acquisition başlatmaz; live preview production frame veya Run Tool sonucu sayılmaz.
- Test/görsel: ui_multi_source_editor, editor_services, ui_editor_usability;
  tek/mixed kaynak, normal/küçük, busy/Stop/error; yerel panel config korunur.
- Kapsam dışı: Opto için ayrı editör veya algoritma panelinden SDK çağrısı.

## ICM-08a — immutable Runtime admission

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-08a.md).
In-memory admission/worker hazır; persistent kullanıcı production ve completion
authority 08b bitene kadar kapalıdır. Native factory capability açılmadı.

- Önkoşul: 07b. 08b bitene kadar kullanıcı production capability kapalı.
- Dosyalar: ProductionRecipeService, MultiSourceRuntimeService, RuntimeService,
  ScopedActiveCalibration, MeasurementWorkspace.
- İş: tam required source seti içine fiziksel identity/backend/profile/geometry ve
  gerekli model evidence'i ekle. Prepare/begin/actual frame kontrollerini ayır;
  Runtime worker aynı frozen binding ile acquire etsin. Run trigger üretmesin.
- Kabul: admission sonrası selection/profile/model/backend değişimi başka snapshot'a
  sızmaz; sonraki begin reprepare ister; actual device/profile mismatch ERROR olur.
  Aynı camera duplicate branch ret; raw-only optional calibration kapısı korunur.
- Test: production_recipe_contract, multi_source_production_contract,
  runtime_coordinator; üç applicationType × tek/mixed source; injected tamper negatifleri.
- Kapsam dışı: terminal okuyucusu bitmeden snapshot'ı kullanıcı Run yolunda etkinleştirme.

## ICM-08b — trusted completion ve kalıcı terminal

**Ortak yazılım kabulü tamam:** [kapanış](../archive/2026-10-18-icm-08b.md).
Terminal8/9 ve engine/27; native G1 kapısı korunur.

- Önkoşul: 08a.
- Dosyalar: ProductionResultCodec, ProductionRecipeService, MultiSourceRuntimeService,
  RuntimeService, terminal/snapshot schema ve CSV veri adapter'ı.
- İş: actual device/profile/geometry/acquisition evidence private digest'e girer;
  yeni schema exact validation ve eski terminal1–7 replay korunur. Başarılı frame ile
  acquisition failure kanıtı farklıdır; frame alınmadıysa ID/pixel/sonuç uydurulmaz.
- Kabul: swapped device/profile/frame, changed hash, forged caller completion,
  partial failure ve Stop ret/suppression; terminal geçmişi SDK/registry yokken açılır.
  Kayıt hatasında retry aynı payload; yeniden trigger/capture/provider yok; pending ACK korunur.
- Test: multi_source_terminal_trace, typed_terminal_trace, runtime_persistence,
  plc_socket_runtime; disk tamper/lock/restart ve gerçek capture sayacı oracle'ı.
- Çıkış: trusted terminal zincirinin yazılım kabulü kapanabilir. Gerçek industrial
  production capability ancak bu zincir ve ilgili backend G1 kanıtı birlikte
  sağlanınca açılır; test adaptörü normal ürün yoluna açılamaz. G3 fiziksel e2e ve
  G4 ölçüm kabulü ayrı kalır. Engine/schema yükseltmesi baseline kararına göre kayda alınır.

## ICM-08c — Runtime ve Logs sunumu

- Önkoşul: 08b, 07c.
- Dosyalar: MainWindowRuntime, Logs/CSV UI, ortak source status presenter.
- İş: N-source tablosunda device/model/seri/setup/profile ve actual frame; ayrıntıda
  producer/SDK kanıtı. Unsupported/unavailable/busy/timeout ayrımı anlaşılır olsun.
  Raw-only sonuç ile ölçüm doğrulaması ve bütün parça kararı birbirine karışmasın.
- Kabul: preparation sahte frame/PASS göstermez; Stop/error eski sonucu temizler;
  history canlı cihazdan bağımsız; CSV acquisition error satırı boş scalar ile kanıt taşır.
- Test/görsel: ui_multi_source_runtime ve terminal CSV corpus; 1/2/8/10 fixture kaynak,
  normal/küçük; Logs→Runtime ve operator salt-okunur erişim regresyonu.
- Kapsam dışı: genel aggregate PASS, yeni robot pose veya ayrı industrial Runtime.

## ICM-09a — birleşik yazılım kabulü

- Önkoşul: 05b, 06b, 07c, 08c.
- İş: son kaynakla Release UI/ZXing ON ve Widgets OFF/ZXing OFF tam CTest; legacy
  golden/no-op, camera missing startup, save-as/transfer ve görsel kontroller.
- Çıktı: `docs/architecture/industrial-camera-release-acceptance.md` (uygulama sırasında);
  `build/icm/09a/` log/XML/hash/görseller ve açık fiziksel matris.
- Kabul: bütün zorunlu testler geçti; SDK kurulmamış ortam davranışı doğrulandı;
  ilk başarısızlık ve tekrar kanıtları saklı. Sadece regex test geçti diye tam kabul yok.
- G2 kapanır. G3/G4, multi-camera fiziksel kapasite ve dağıtım otomatik kapanmaz.

## ICM-09b — fiziksel kamera ürün kabulü

- Önkoşul: 09a ve seçili backend'in G0/G1 kanıtı. OECS için G0=01b;
  Basler/Daheng için BAS/DAH-01 ve 02. OECS kabulü diğer markalar için önkoşul değildir.
- İş: [fiziksel matris](industrial-camera-acceptance-plan.md) H01–H08;
  tek kamera + mixed-source, üç applicationType, serial/actual profile/latency/bellek,
  Stop/unplug/replug/viewer contention/restart ve stale-result kontrolleri.
- Çıktı: aynı release acceptance belgesinde ayrı fiziksel bölüm, gerçek raster/log/JSON.
- Kabul: zorunlu H satırları ölçülmüş ve başarılı; eksik hardware koşulu açık.
  Kablo/reboot gibi insan adımları gerektiğinde kullanıcıya kısa ve somut işlem verilir.
- G3 yalnız sınanan backend/model/firmware kapsamı için kapanır. Ölçüm hatası/toleransı
  veya ikinci fiziksel kamera kanıtı çıkarılmaz. BAS/DAH-03 bu kartı kendi cihazında uygular.

## ICM-10 — bağımsız ölçüm ve lens karar kapısı

- Önkoşul: 09b ve bağımsız fiziksel referans/ölçüm hedefleri.
- İş: lens tam modeli, odak/diyafram/mesafe/ışık, teaching hedefi ve bağımsız referans
  kaydı; merkez/kenar/köşe, farklı uzunluk/yön ve tekrarlar. Piksel ve mm residual,
  ortalama yanlılık, maksimum mutlak hata, RMS ve tekrar saçılımı raporu.
- Kabul: işin gerekli toleransı önceden belli, bütün zorunlu ölçüm noktaları sınır içinde;
  training fit hatası bağımsız doğruluk yerine geçmez. Referans/tolerans bilinmiyorsa
  yalnız karakterizasyon raporu, G4 açık. Fiziksel hata başarısızsa solver ret kapıları gevşetilmez.
- Çıkış: raw scale/homography yeterli veya lens T15 öne alınmalı kararının kanıtı.
  T15 gerekirse ayrı plan/uygulama; bu kartta gizli undistort eklenmez.
- Kapsam dışı: 3D/robot kalibrasyonu ve bütün lensler/kameralar için genel doğruluk iddiası.
