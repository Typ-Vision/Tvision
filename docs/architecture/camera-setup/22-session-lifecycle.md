# CCUX-22 — çalışma süresince kamera bağlantısı

22 Ekim planı; 23 Ekim durumu: **22a yazılım ölçümü tamamlandı; fiziksel baseline açık.**
[Kapanış](../../archive/2026-10-23-ccux-22a.md). **22b ortak servis tamamlandı**
([API/ömür](acquisition-sessions.md), [kapanış](../../archive/2026-10-23-ccux-22b.md));
22c Editor entegrasyonu uygulandı ([kapanış](../../archive/2026-10-23-ccux-22c.md));
22d Runtime uygulandı ([kapanış](../../archive/2026-10-23-ccux-22d.md));
22e Kalibrasyon uygulandı ([kapanış](../../archive/2026-10-23-ccux-22e.md));
22f birleşik kabul açık. Kullanıcı kararı: program
açılışında bağlantı yok; Editor reçete açılışında, Runtime Run'da, kalibrasyon
işlemine girişte bağlantı kurulur. İlgili iş bitince bırakılır. Her karede yeniden
keşif/open/configure/close yapılmaz. [C2a](contract.md#c2a--çalışma-süresince-bağlantı)
hedef sözleşmedir; aşağıdaki kartlar sırayla ve ayrı commitlerle uygulanır.

## Başlangıç ve kapsam

README/progress, plan indeksi, contract C0/C2/C2a/C3/C4/C6 ve yalnız seçili alt
kartın bölümü okunur. Ön inceleme: [CCUX-20b](../../archive/2026-10-22-ccux-20b.md).
Kod sembolleri aşağıda dar başlangıç listesidir; bütün eski kartlar okunmaz.

Kapsam mevcut endüstriyel kamera yolları ve ortak üç applicationType'dır.
Dosya/replay kamera bağlantısı açmaz. Webcam davranışı bu pakette değiştirilmez.
Mevcut native/deneysel edinim politikaları ve kaynak bazlı köken korunur.
Camera Manager ayar/tek-canlı test servisleri aynı sahiplik mekanizmasıyla
çakışmadan çalışır; liste açılışında kalıcı kamera bağlantısı başlatmaz.

İlk sürümde **OECS kare başına stream start/stop korunur**. Bağlantıyı açık
tutmak sürekli çekim değildir. Free-run, stream'i sürekli açık tutma, SDK
kontrollerini topluca kaldırma, çözünürlük azaltma ve native admission açma yoktur.
Stream iyileştirmesi ancak yeni ölçüm gerektirirse ayrı planlanır.

## Kartlar ve sıra

| Kart | Teslim | Bağımlılık |
| --- | --- | --- |
| CCUX-22a | Aşama süreleri ve mevcut davranış baseline'ı | — |
| CCUX-22b | Application oturum sahibi ve güvenli kapanış | 22a |
| CCUX-22c | Editor açılış/çıkış ve hazır oturumdan çekim | 22b |
| CCUX-22d | Runtime Run/Stop ve çok kamera hazırlığı | 22b,22c |
| CCUX-22e | Kalibrasyon işlemi ve Camera Manager devri | 22b–22d |
| CCUX-22f | Birleşik regresyon, fiziksel gecikme ve kullanıcı kabulü | 22a–22e |

Bir oturumda bir alt kart; ajanlar paralel çalışmaz. Her kart kendi kapsam,
kanıt ve açık sınır kaydıyla kapanır. Yazılım fixture başarısı fiziksel kabul
değildir; cihaz yoksa ilgili fiziksel satırlar açık kalır. 22a yazılım teslimi tamamlandı;
23 Ekim fiziksel serisi kamerayı bulamadı. 22b servis ve 22c Editor uygulandı;
22d Runtime ve 22e Kalibrasyon uygulandı; 22f açık.

## CCUX-22a — ölçüm

Oku: MainWindowExecution, IndustrialExecution::acquireIndustrial,
ExperimentalOecsAcquisition::Backend, OecsSession::open/captureFrame/cleanupStream,
CameraWorkerSession destructor; RuntimeCoordinator ve IndependentCalibrationCapture
içindeki başlangıç/tamamlanma noktaları. Önce kaynakta sembol arayın.

- Monotonic saat ve istek/oturum ID ile aşama ölçümleri ekle: kullanıcı komutu,
  worker kuyruğu, keşif/resolve, backend/DLL hash, SDK load/enumerate/open,
  configure/readback, stream start, trigger/wait, raster copy/hash/preview,
  stream cleanup, session close, UI ilk paint. İç içe süreleri çift toplama;
  paint ölçülmüyorsa yalnız setImage süresini uçtan uca diye adlandırma.
- Tanı isteğe bağlı, ürün akışına yeni mod/onay eklemez. Bekleme/timeout süresi
  başarı süresi sayılmaz; hatalı ve iptal örnekleri ayrı raporlanır.
- Release üzerinde aynı kamera/rota/profil/exposure/geometri ile Editor,
  Runtime ve kalibrasyon için ilk hazırlığı ayrı ölç; her birinde en az 20
  başarılı çekim ve en az 3 kapat/aç tekrarı. Ortam ve readback kaydedilir.
  Eski 9,359 FPS yeni baseline değildir. Kamera erişimi yoksa fixture ölçümü
  yalnız yazılım kanıtıdır; fiziksel baseline açık kalır.
- Median/p95, min/max, başarı/hata sayısı ve open/configure/close sayaçları;
  ölçüm açık/kapalı farkı kontrol edilir. Hedef ms ölçümden sonra gerekçelendirilir,
  önceden 100 ms gibi bir süre vaat edilmez.

Kanıt: `build/ccux/ccux-22a/`. İlgili Release build ve industrial_execution_contract,
industrial_oecs_experimental, industrial_owned_frame; UI tanısı eklenirse
image_refresh_contract/ui_editor_usability ve görsel kontrol.

## CCUX-22b — ortak oturum sahibi

Oku: CameraWorkerSession, CameraLifecycle, CameraPreviewSession,
IndustrialExecutionInputs/ExecutionService, IndustrialCameraProvider,
ExperimentalOecsAcquisition ve CameraSettingsService. C2a sahibi application'dır.

- Önerilen CameraAcquisitionSessions servisi: open/prepare, acquire, close/drain
  komutları; açık context ID (Editor reçete, Runtime run, kalibrasyon işlemi),
  generation ve hazırlanmış ayar kimliği. İsim uygulamada mevcut adlarla uyarlanabilir.
- Servis sabit owner thread'de worker'ları yönetir; UI/QtConcurrent yalnız komut
  ve immutable sonuç taşır. CameraWorkerSession pointer'ını farklı execution
  thread'lerine taşıma. SDK çağrıları mevcut donanım worker'ında kalır.
- Fiziksel vendor/model/serial başına tek lease; rota/backend, setup/profile
  revision ve acquisition kimliği exact eşleşir. Kaynak/çevrim metadata'sı istek
  bazında bağlanır; bir kaynağın karesi başka kaynağın cache'ine yazılmaz.
- Oturum durumları: Closed → Opening → Configuring → Ready → Capturing → Ready;
  çıkışta Closing → Closed. Hata Fault/drain; temiz kapanış belirsizse lease
  karantinada kalır. Yeni generation'a eski completion yayınlanmaz.
- En fazla bir bekleyen edinim/kamera. Sağlıklı Ready oturumda tekrarlı çekimde
  keşif, DLL hash/load, open veya tam configure tekrarı yok; mevcut kare
  doğrulamaları sürer. Ready ve config kimliği fiziksel model kullanım izni değildir.
- Bir çekimdeki stream cleanup başarıyla biterse kare oturum kapatılmadan
  yayınlanabilir. Eski acquireIndustrial'ın close-before-publish kuralı burada
  yalnız oturum içindeki başarı için değişir; Stop/generation kontrolü korunur.
- Idle Stop da lease bırakır. Shutdown ana UI thread'ini join ile dondurmaz;
  bütçe aşımında cihaz bırakılmış gibi göstermez, iş parçacığı zorla öldürülmez.
  Proje/pencere kapanışında asenkron drain tamamlanmadan sahip yok edilmez.
- Aynı fiziksel kamera birden fazla kaynakta aynı edinim ayarıyla kullanılıyorsa
  tek bağlantı, ayrı sıralı çekimler. Çelişen profiller açık hazırlık hatasıdır;
  sessiz ayar seçimi veya aynı kareyi iki tetik yerine kullanma yoktur.

Kabul: fixture sayaçları 1 open/configure + N grab + çıkışta 1 close; ikinci
oturumda yeni open; cancel/open yarışı, geç kare, geç open, close hatası,
thread-affinity, çok kaynak kimliği ve lease çakışması. İlgili testler:
camera_worker_lease_contract, camera_lifecycle_contract, industrial_execution_contract,
industrial_backend_lifetime, industrial_oecs_experimental, industrial_owned_frame,
industrial_provider_contract. Yeni servis için anlamlı yaşam döngüsü regresyonları
eklenir. Kanıt `build/ccux/ccux-22b/`; UI'sız servis kartıdır.

## CCUX-22c — Editor

Oku: MainWindowWorkspace reçete çift tıklama/açma, MainWindow::selectPage,
MainWindowExecution, EditorSession ve kaynak kamera bağlama yolu.

- Kullanıcının açık reçete düzenleme komutunda etkin ve bağlı industrial kamera
  kümesini arka planda hazırla; çift tıklama ve eşdeğer açma komutu aynı yoldadır.
  Program/proje açılışında önceki ekranın restore edilmesi bu komut sayılmaz.
- Kamera yok/meşgul/hatalıysa reçete açılır ve dosya düzenleme sürer. İlgili çekim
  hazır değil durumundadır; diğer hazır kaynaklar tekil çekilebilir. Hatalı kaynak
  gereken Run Flow normal hata verir. Retry kullanıcı komutuyla yapılır.
- Görüntü al ve yeni kare isteyen Run Flow hazır oturumu kullanır; araç/ROI/
  Properties sekme değişimi veya Kaydet bağlantıyı kesmez.
- Başka reçete, Editor dışına çıkış, proje/pencere kapanışı önce mevcut dirty
  kaydet/vazgeç/iptal kararını çözer. İptal edilen çıkış bağlantıyı korur;
  kabul edilen çıkış kayıtlı veya kayıtsız bütün yollarda drain/close yapar.
- Kamera kaynak ekleme/seçme yalnız yeni hedefi hazırlar; kaynak kaldırma veya
  kapatma son kullanan kaynağı da kaldırdıysa lease'i bırakır. İsim/ROI/araç
  parametresi değişimi kamera configure tetiklemez. Profil/geometri revision
  değişimi ilgili oturumu geçersiz kılar ve yeniden hazırlar.
- Açık bir edinim Stop komutu oturumu kapatır; sonraki açık Görüntü al komutu
  yeniden hazırlayabilir. Otomatik tekrar döngüsü yoktur.

Kabul: çevrimdışı açılış, 20 çekimde tekrar open olmaması, kaydet/devam,
çıkış iptali, kaydetmeden çıkış, A→B geçişte geç A karesi reddi; 1/5 kamera
fixture ve üç applicationType. session_contract, editor_target_only,
scoped_calibration_editor, ui_multi_source_editor, ui_editor_usability ve servis
regresyonları. Release + normal/küçük görseller `build/ccux/ccux-22c/`.

## CCUX-22d — Runtime

Oku: RuntimeCoordinator requestStart/completeStart/stop/finish,
MainWindowRuntime::runtimeAction, RuntimeService, ExperimentalRuntime,
IndustrialExecution ve immutable ProductionCycle bağımlılıkları.

- Load Recipe çevrimdışı kalır. Run: frozen kaynak kümesi hazırlanır, bütün
  gerekli kamera/readback/izin kontrolleri bitene kadar Starting sürer. Kamera
  açma ve SDK işleri UI thread'inde yapılmaz. Güncel start generation kabulü ve
  immutable bağımlılık kontrolü ardından Running'e geçilir; Run tek başına sonuç
  veya çekim üretmez. Mevcut manuel çevrim politikası değişmez.
- Starting sırasında Stop yeni çevrimleri keser, geç open sonucunu kapatır.
  Çok kamerada hazırlık kısmen başarısızsa açılan bütün Runtime oturumları
  kapatılır; kısmen Running yoktur. Sıralı/deterministik hazırlık deadlock önler.
- Tek çevrimler aynı oturumları kullanır. Stop, çevrim yokken de bağlantıları
  bırakır. Stopping kamera drain'i de içerir; kapanış bitmeden Run yeniden açılmaz.
  Donanım kapanışı ile disk kayıt retry ayrı tutulur: kayıt hatası kamerayı
  gereksiz kilitlemez; önceki sonuç/teslim/persistence kuralları korunur.
- Kamera hatası/kopma Running'i keser, küme kapanır; replug kendi kendine üretim
  başlatmaz. Yeni açık Run gerekli doğrulamaları yapar. Reçete/model/ayar değişimi
  mevcut Stop ve yeniden Load kuralını korur; snapshot sessiz yenilenmez.
- Aktif Runtime'ı Editor veya Kalibrasyon kamera isteği durduramaz/ele geçiremez;
  önce Stop. Mevcut operator mode/gezinme kısıtları korunur.

Kabul: Load'da 0 open, Run'da N benzersiz kamera open, M çevrimde ek open=0,
idle/in-flight/Starting Stop'ta temiz kapatma; 1/5 kamera kısmi hata rollback,
yeniden Run, USB kopma, değişmiş snapshot ve disk retry. runtime_coordinator,
runtime_persistence, experimental_metric_runtime_contract,
industrial_runtime_admission_contract, scoped_calibration_runtime,
ui_multi_source_runtime. Release/görseller `build/ccux/ccux-22d/`.

## CCUX-22e — Kalibrasyon ve kamera devri

Oku: CalibrationOverview/Wizard, MainWindowCalibration,
IndependentCalibrationCapture/Session, CameraManager, CameraSettingsService,
CameraPreviewSession ve model geçerliliği/SaveUse servisleri.

- Kalibrasyon listesini açma bağlantı kurmaz. Seçili kamerada Kalibre Et ile
  canlı işlem açılışında hazırlanır; aynı işlem içindeki çekimler bağlantıyı korur.
  Dosya kaynağı seçilirse kamera oturumu bırakılır; replay fiziksel kare sayılmaz.
- Başarılı Kaydet ve kullan ile işlem bittiğinde, iptal/çıkış/kamera değişiminde
  kapanır. Hesap/SaveUse hatası kullanıcıyı işlemde tutuyorsa bağlantı sırf hata
  nedeniyle kapanmaz; donanım hatası ise fault/drain uygulanır. Mevcut önceki
  model seçimini koruma ve exact kanıt kuralları sürer.
- Editor→Kalibrasyon/Camera Manager devri kabul edilmiş çıkışı drain eder;
  diğer servis ancak önceki lease temiz bırakılınca açar. Ayar servisinin
  bağımsız erişimi korunur, ikinci gizli SDK handle'ı açılmaz. Değişen ayarlar
  sonraki context'te readback ile doğrulanır; revision/cache/model kuralları sürer.
- Camera Manager mevcut tek/canlı preview ömrünü korur; bu pakette listeyi
  kalıcı bağlı yapmaz. Preview/ayar işleri yeni ortak sahibi atlayamaz.

Kabul: çoklu plaka çekiminde tek open, dosyaya geçiş, başarılı/başarısız save,
iptal, Editor→kalibrasyon→Runtime devri, meşgul Runtime reddi, ayar sonrası
yeniden hazırlık; ui_independent_calibration_wizard, independent_calibration_capture,
industrial_calibration_capture, calibration_save_use, scoped_active_calibration,
camera_settings, oecs_camera_settings, ui_camera_manager, ui_calibration_overview.
Release/görseller `build/ccux/ccux-22e/`.

## CCUX-22f — birleşik kabul

- 22a baseline'ını aynı donanım/readback ile tekrarla. İlk hazırlık, hazır
  bağlantıda tıklama→yeni görüntü ve Stop→lease bırakma ayrı raporlanır.
  Tekrarlı çekimde keşif/open/configure/close maliyeti sıfır çağrı olmalı;
  median/p95 önce/sonra sunulur. İyileşme yoksa ana maliyet yeniden incelenir,
  yalnız sayaç başarısı kullanıcı gecikmesi kabulü diye sunulmaz.
- Gerçek kamera ile Editor aç/çek/kaydet/çık, Runtime Load/Run/20 çevrim/Stop,
  kalibrasyon giriş/20 çekim/iptal, idle Stop ve USB kopma/yeniden açma.
  Reçete ve kalibrasyon dosya değişiklikleri yalnız build altındaki test projelerinde.
- Açık context dışında SDK bağlantısı yok; restore açmıyor, başlatma sonuç
  üretmiyor, çıkış iptali context'i bozmuyor, geç kare yeni context'e geçmiyor.
  Yeni frameId ve sayaç yazılım kanıtıdır; OECS tetik-kare eşleşmesini ispatlamaz.
- Ortak üç applicationType ve 1/5 kamera fixture matrisi; UI-on/UI-off Release
  üzerinde ilgili testleri birleştir. Önce ctest -N ile isimleri doğrula; sıfır
  test başarı değildir. Normal/küçük UI görsellerini incele. Donanım yoksa
  fiziksel tablo açık kalır. Native/optik/mm kabul, PLC/robot veya CUDA açılmaz.
- Kanıt `build/ccux/ccux-22f/`; kullanım rehberini ancak uygulanmış davranış için
  güncelle. Her kartın kapanışı progress/arşiv indeksi ve belge boyut kontrolüyle
  tek görev commit'i; push yalnız kullanıcı isterse.

## Sonraki adım

**CCUX-22f** birleşik kabul sonraki karttır. 22a fiziksel baseline ve UI paint,
CCUX-20 gerçek etkileşim ve CCUX-15 bağımsız fiziksel mm kabulü açık kalır.
22e'deki iki manuel hazırlık denemesinde SDK kayıtlı kamerayı bulamadı;
yazılım fixture başarısı fiziksel kabul yerine geçmez.
