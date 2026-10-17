# ICM görev kartları — donanım ve acquisition altyapısı

**ICM-00 tamamlandı** ([başlangıç kaydı](industrial-camera-baseline.md));
**ICM-01a tamamlandı** ([tanı kabulü](../archive/2026-10-17-icm-01a.md));
**ICM-01b açık** ([ön kontrol](../archive/2026-10-17-icm-01b.md));
**ICM-02 offline/koşullu tamamlandı** ([karar](industrial-camera-backend-decision.md)).
Diğer kartlar **PLANLANDI**. Ortak tamamlanma şartı:
ilgili Release build/test, kapsamın kanıtı, sınırlamalar ve sonraki ID tarih/görev
arşiv kaydına yazılır; progress'in kısa özeti ve bağlantısı güncellenir.
Gerçek cihaz kabulü fixture ile kapatılmaz. [Ana plan](industrial-camera-plan.md),
[sözleşmeler](industrial-camera-contract-plan.md), [kabul](industrial-camera-acceptance-plan.md).
Dosya listeleri başlangıç haritasıdır; yeni dosya adları uygulayıcı tarafından mevcut
sorumluluklara göre seçilir. Ürün kapıları aşağıdaki kart tamamlanmadan genişletilmez.

## ICM-00 — başlangıç ve sürüm envanteri

- Önkoşul: yok. Donanım gerekmez.
- Oku: AGENTS, progress, README, üç requirements; MeasurementSetupContract,
  IndependentCalibrationCapture, MeasurementWorkspace, SystemCalibration,
  ToolRegistry/Executors, ExecutionService, ProductionRecipeService/ResultCodec.
- İş: dirty değişiklikleri koruyarak gerçek source/codec/hash/backend kapılarını çıkar;
  engine/editor/terminal/scope sürümlerini kaydet. Mevcut test adlarını `ctest -N`
  ile eşleştir; yeni industrial tool adı ve kanıt genişletme noktalarını karara bağla.
- Çıktı: `docs/architecture/industrial-camera-baseline.md` (uygulama sırasında üretilecek)
  ve `build/icm/00/` envanteri. Yeni C++ davranışı yok.
- Kabul: bütün webcam-only ret noktaları ve legacy no-op korunacak alanlar listeli;
  başlangıç test envanteri okunabiliyor. Eski test sayısı yeni koşum diye yazılmıyor.
- Kapsam dışı: SDK kurma, feature gate açma, şema yükseltme.

<a id="icm-01a"></a>
## ICM-01a — SDK ve probe tanısı

- Önkoşul: 00. Kamera olmadan yapılabilir.
- Dosyalar: `scripts/`, `src/app/camera_probe.cpp`, gerekirse CameraService;
  `build/icm/01a/` raporları.
- İş: explicit OECS x64 U3V path, SDK/producer sürüm/hash, process/machine env farkı,
  DLL dependency ve probe exit/status kaydı için tekrar çalıştırılabilir tanı betiği.
  PATH yalnız child/process scope; 32-bit producer yanlış seçimi açık tanı.
- Kabul: eksik yol/yanlış binary/boş cihaz/hata durumları ayrı raporlanır; 0 device DONE
  kamera bağlı diye sunulmaz; `--open-check` yalnız gerçek bulunan cihaz için raporlanır.
- Doğrulama: PowerShell parse, mevcut camera_transport ve gerekirse CLI fixture testi.
- Kapsam dışı: otomatik reboot, firmware update, driver değiştirme, tüm disk taraması.

## ICM-01b — fiziksel başlangıç

- Önkoşul: 01a ve erişilebilir kamera. Kullanıcı reboot yaptıysa kaydet; varsayma.
- İş: OECS'de model/seri/USB durumu ve gerçek görüntü; tam raster/Mono8 kaydı.
  Lens etiketi/odak/diyafram/çalışma mesafesi elde edilebiliyorsa kaydet, bilinmeyeni boş bırak.
  OECS bağlantısını serbest bıraktıktan sonra explicit U3V probe discovery/open-check.
- Çıktı: `build/icm/01b/` raster ve JSON/log; donanım formunda cihaz/bağlantı/seri,
  tarih/SDK/ayarlar. Kullanıcı beyanı ile araçla gözlenen kanıt ayrı işaretli.
- Kabul: tek exact cihaz keşfi, read-only open/close ve OECS gerçek görüntü ayrı ayrı başarılı.
  Device ID=1 seri yerine geçmez; arıza varsa USB port/kablo/güç/driver/owner durumunu tanıla.
- Kapsam dışı: VisionFlow gerçek grab kabulü; kablo sökme/reboot kullanıcı fiziksel adımıdır.
- Donanım yoksa: kart açık kalır, 03a/03b gibi bağımsız işler ilerleyebilir.

<a id="icm-02"></a>
## ICM-02 — GenTL/GenApi ve OECS C ABI karar deneyi

Durum: [offline/koşullu karar tamamlandı](industrial-camera-backend-decision.md);
gerçek open/feature/unit/grab doğrulaması açık.

- Önkoşul: 01a; fiziksel deney için 01b. Tek karar paketi, iki tam backend yazılmaz.
- Oku: yerel OECS Includes, runtime exports, örnekler/lisans; GenTLTransport ve toolchain.
- İş: küçük ayrı probe ile GenTL x64 export/load, remote XML/nodemap erişimi ve MinGW
  GenApi yolu; alternatif OECS C API header/struct/calling convention/export/lifetime
  deneyi. İki yolda da discovery/open, gerekli feature readback ve cleanup sınırını belge.
- Çıktı: `docs/architecture/industrial-camera-backend-decision.md`;
  `build/icm/02/` compile/run/ABI/license notları. Exact SDK binary sürüm/hash'i yazılır.
- Kabul: seçilen tek backend ve nedenleri; feature/unit/readback, stream/event API,
  MinGW ABI, runtime yokluğunda fallback değil açık unavailable ve dağıtım bağımlılıkları belli.
- Fiziksel deney yapılamadıysa offline sonuçla koşullu karar mümkündür; gerçek open/grab
  kapısı açık kalır. Feature erişimini çözemeyen GenTL keşfi backend tamamlandı sayılmaz.
- Kapsam dışı: MSVC'ye toplu geçiş, bilinmeyen struct'ı elle tahmin, ITALA API'yi COE-U'ya eşlemek.

## ICM-03a — cihaz kimliği ve rota sözleşmesi

- Önkoşul: 00. Donanım gerekmez.
- Dosyalar: domain/MeasurementSetupContract, application/MeasurementEnvironment,
  MeasurementSourceIdentity ve ilgili codec testleri.
- İş: fiziksel identity/güven düzeyi ile TransportLocator/BackendIdentity ayrımını
  yeni sürümlü değer tiplerinde kur; eski webcam provider ID yolu değişmesin.
  Aynı cihaz iki producer, aynı seri farklı vendor, duplicate/eksik seri ve explicit
  rebind davranışlarını tanımla. Yerel producer path proje kimliği olmasın.
- Kabul: strict encode/decode; belirsiz eşleme reddi; rename kimliği korur; rebind revision
  artırır; legacy golden JSON/hash/no-op Save korunur; henüz industrial capture açılmaz.
- Test: measurement_setup_contract, measurement_environment_persistence ve yeni kimlik corpus'u.
- Kapsam dışı: cihaz keşfi/GUI veya kalibrasyon kayıtlarının otomatik yeniden bağlanması.

## ICM-03b — acquisition profile ve geometry revision

- Önkoşul: 03a.
- Dosyalar: domain yeni profile tipleri, MeasurementSetupContract,
  application persistence/dependency transfer; profile unit testleri.
- İş: requested/applied/profile revision; kritik geometri, pixel format, trigger ve
  exposure/gain/auto ayrımı. Sözleşmedeki invalidation tablosunu tek servis kuralı yap.
  Yeni alanlar için strict schema; eski geometry/hash yolunu aynı bırak.
- Kabul: eş boyut/farklı offset/reverse reddi; exposure yalnız acquisition kimliğini
  değiştirir; isim hash değiştirmez; unknown critical geometry reddi; export/import/Save As
  yerel rota gerektirmeden profili taşır fakat kullanım izni üretmez.
- Test: yeni profile/geometry corpus, measurement_setup_migration ve project_persistence.
- Kapsam dışı: gerçek sensor ROI/binning/packed-format desteği veya lens düzeltmesi.

## ICM-04a — backend session ve handle ömrü

- Önkoşul: 02 karar kaydı, 03b.
- Dosyalar: hardware seçilmiş backend; GenTLTransport korunacak discovery dalı,
  CMake optional SDK düzeni; fake backend/producer test desteği.
- İş: load/init/open/exclusive device/close katmanları; owner worker ve RAII hata
  yolları. GenTL seçildiyse producer session paylaşımı/refcount tasarlanır; mevcut
  process kilidi kanıt olmadan kaldırılmaz. C API seçildiyse create/destroy handle eşleşir.
- Kabul: her init/open aşamasında failure injection sonrası doğru ters cleanup;
  busy device tanısı; aktif device varken DLL unload yok; SDK'sız core/UI açılır.
- Test: camera_transport + yeni backend lifetime testleri; gerçek open kanıtı ayrı.
- Kapsam dışı: callback içinde UI, stream veya trigger.

## ICM-04b — feature erişimi ve configure/readback

- Önkoşul: 04a.
- Dosyalar: hardware feature adapter, application configuration service ve profile tipleri.
- İş: SFNC destekli node listesi/type/access/range/increment/units; model/serial,
  payload ve geometry readback. Mono8/full sensor/manual exposure/gain ve trigger ayarları
  için güvenli sıra. Kısmi yazımda rollback doğrula, belirsizlikte Fault.
- Kabul: readonly/missing/locked node, int/float exposure, out-of-range, sessiz clamp,
  kısmi failure ve actual≠requested durumları açık; UserSetSave yazılmaz.
- Test: deterministic node-map fixture + varsa gerçek kamera feature raporu.
- Kapsam dışı: bütün vendor node'larını otomatik UI'a dökme veya ayarı kayıtlı reçeteden gizli değiştirme.

## ICM-04c — stream/buffer ve owned Mono8 frame

- Önkoşul: 04b.
- Dosyalar: hardware stream, application industrial adapter, ImageFrame dönüşüm sınırı;
  fake producer stream/buffer desteği.
- İş: pool allocate/announce/queue/start/wait/requeue/stop/flush/revoke/close sırası
  seçilen SDK'ya göre; önce bounded tek-frame probe. Owned copy bitmeden SDK buffer dönmez.
- Kabul: gerçek format/size/stride/payload; truncated/incomplete/overflow/packed/chunk
  durumları; cleanup sonrası frame okunabilir; eski buffer mutation sonucu değiştirmez.
  Pool byte limit allocation öncesi; leak/resource counter'ları dengeli.
- Test: padded Mono8 bilinen pikseller + invalid payload + her cleanup failure noktası;
  fiziksel varsa dosyaya gerçek raster ve actual metadata, sentetik etiket açık.
- Kapsam dışı: software trigger tazelik garantisi, calibrated production veya packed decode.

## ICM-04d — software trigger admission

- Önkoşul: 04c.
- Dosyalar: AcquisitionService/adapter, hardware trigger/frame event ve acquisition evidence.
- İş: stream-ready → tek in-flight → trigger → actual frame eşlemesi; monoton kalan
  bütçe her wait'e gider. Preview/free-run kalıntıları, counter wrap ve re-open resetleri
  generation ile ayrılır. Source/cycle/frame kimlikleri private admission'dan gelir.
- Kabul: önceki frame, duplicate, late trigger/frame, timeout, read sırasında cancel,
  geç SDK başarı, wrap/reset ve trigger-command failure hiçbir sahte DONE üretmez.
  Aynı pikselli iki gerçek yeni frame kabul edilebilir; hash tazelik oracle'ı değildir.
- Test: bağımsız fake hardware counter/event sırası; fiziksel tek trigger/tek frame raporu.
- Kapsam dışı: PLC mesajı=hardware trigger varsayımı veya external trigger desteği.

## ICM-04e — lifecycle, sahiplik ve preview

- Önkoşul: 04d.
- Dosyalar: CameraLifecycle, application camera session/lease ve JobSession bağlantısı.
- İş: controller owner thread ile SDK worker sınırı; preview latest-drop ve production
  FIFO ayrımı; aynı kamera için Calibration/Editor/Runtime exclusive lease. Stop ve
  disconnect sırasında generation/late-result/cleanup; reconnect açık akış.
- Kabul: preview→production geçişi eski buffer taşımaz; busy/queue full önce reddedilir;
  fault sırasında geç başarı iyileşme sayılmaz; worker bitmeden SDK yeniden açılmaz.
  Kesilemeyen call stop budget exceeded döndürür, DLL unload/force terminate yapmaz.
- Test: camera_lifecycle_contract, session_contract, acquisition_contract ve yeni lease corpus'u.
- G1: gerçek grab/Stop kanıtı varsa acquisition kapısı kapanır; fixture-only ise fiziksel
  G1 açık tutulur. Editor/Runtime production izni hâlâ ICM-08b'yi bekler.
