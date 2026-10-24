# CCUX-16 — OECS işlemlerinde otomatik edinim politikası

Durum: yazılım teslimi tamamlandı. [22 Ekim kapanışı](../../archive/2026-10-22-ccux-16.md).
Önkoşul: mevcut CCUX-01–14 altyapısı.
CCUX-15 fiziksel referans kabulü bu yazılım işini bekletmez.
[Plan](README.md), [ortak sözleşme](contract.md) C0–C5.

## Oku

Kaynaklarda önce sembol ara: `IndependentCalibrationCapture`,
`ScopedActiveCalibrationService::setExperimental`, `MeasurementWorkspace`,
`ExecutionService::withExperimentalEditorReplay`, `RuntimeService::setExperimental`,
`ExperimentalOecsAcquisition::available`, `CameraLocalConfiguration`.
[Runtime sözleşmesi](../experimental-runtime.md) ve
[OECS edinim sözleşmesi](../industrial-camera-oecs-experimental-plan.md)
yalnız politika/kanıt bölümleri. Eski opt-in UI gereği C0 ile değiştirilmiştir.

## Uygula

1. Application katmanında kamera/rota/profil ve kaynak kanıtından edinim yolunu
   çözümleyen ortak politika oluştur. Desteklenen exact Opto/OECS cihazında mevcut
   OECS yolunu seç; diğer kamera/dosya yollarının davranışını koru. UI boolean'ını
   tüm kaynaklar için koşulsuz `true` yapma. Eksik SDK, yanlış cihaz veya belirsiz
   rota somut hata verir; sessiz başka backend'e geçmez.
2. Kamera önizlemesi, kalibrasyon çekimi/yeniden hazırlama, Editor görüntü alma/
   Run Flow ve Runtime Load/Run hazırlığı bu kararı kullanır. Kullanıcının normal
   komutu yeterlidir; ayrıca deneysel onay veya yerine geçen başka seçim gerekmez.
3. Kalibrasyonda ayrı opt-in gereğini kaldır; kullanım iznini güncel kimlik,
   kurulum/profil, gerçek probe ve permission epoch ile üret. Proje değişimi,
   restart ve replug eski izni canlandırmaz. Kullanıcı mevcut modeli normal
   yeniden hazırlama komutuyla kullanabilir; tekrar onay kutusu işaretlemez.
4. Editor canlı/PGM+JSON replay politikasını kaynak başına çöz; metadata/sidecar
   ve hash kontrollerini koru. Dosya veya kamera değişiminde cache/resolver yenilensin.
5. Runtime yolu Load/Run hazırlığında reçetenin gerekli kaynaklarından belirlensin.
   Normal dosya/webcam reçetesi OECS yoluna zorlanmasın. OECS ile normal kaynakların
   aynı reçetedeki desteklenen bileşimi açık test edilsin; desteklenmeyen bileşim
   hazırlıkta anlaşılır hata versin. Model/ayar değişimi Stop → Load gerektirir.
   Çalışan snapshot/persistence retry/coordinator/PLC bağlantısını yerinde değiştirme.
6. Bağlantı veya proje açılışı edinim/Run başlatmasın. Kullanıcı başlattıktan sonra
   Stop/drain, lease, timeout, kopma ve geç completion kuralları aynı kalsın.
   Profile uygunluk ve FrameBurstStart koşulu korunsun; eski FrameStart profili
   sessiz çevrilmesin. Yeni varsayılan profili desteklenen backend ile uyumlu kur.

## Kabul

- Yeni oturumda onay kutusu olmadan desteklenen kamera ve replay hazırlanabilir;
  dosya açmak tek başına capture/üretim izni oluşturmaz.
- Desteklenmeyen cihaz, eksik DLL, yanlış seri/geometri, değiştirilmiş sidecar,
  proje değişimi ve eski permission epoch reddedilir. A→B→A izni canlandırmaz.
- Mevcut typed köken/snapshot/codec alanları korunur; productionAdmission ve
  triggerAssociationVerified değerleri kullanıcı arayüzüne göre değiştirilmez.
- Core Release ve ilgili CTest: `industrial_oecs_experimental`,
  `independent_calibration_capture`, `industrial_calibration_capture`,
  `industrial_scoped_calibration`, `scoped_active_calibration`,
  `calibration_save_use`, `scoped_calibration_editor`,
  `experimental_metric_runtime_contract`, `runtime_coordinator`,
  `runtime_persistence`. Seçim yapısını yansıtan eski opt-in testlerini yeni
  kaynak/oturum davranışıyla değiştir; negatif kontrolleri silme.

## Kapanış

Saf servis teslimidir; görünür seçenekleri CCUX-17 kaldırır. Uygulama sırasında
UI bağlantısında zorunlu değişiklik olursa ilgili Release/UI/görsel kabulü ekle.
Kanıt `build/ccux/ccux-16/`; görev ID'li tek commit, push yok. Sonraki CCUX-17.
