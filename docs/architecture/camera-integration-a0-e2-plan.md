# INT — A0–E2 çalışmalarını CameraIntegration üzerine taşıma

25 Ekim 2026. Uygulayıcı: Codex. Durum: **INT-03 tamamlandı; tanı/kurtarma aktarıldı**.
Snapshot `230cc72`; taban `CameraIntegration@3793198`; çalışma dalı
`codex/camera-a0-e2-integration`. [INT-01 kapanışı](../archive/2026-10-25-int-01.md).
Sonraki kart INT-04; ayrıca yetkilendirilmeden başlatılmaz.
[INT-03 kapanışı](../archive/2026-10-25-int-03.md).
[INT-02 matrisi](camera-integration-int02-matrix.md), [envanter](camera-integration-int02-inventory.md),
[baseline/kapanış](../archive/2026-10-25-int-02.md): ayrı Release ve görseller hazır;
Missing result + pre-cancel + Camera Manager assertion'ları açık. İki HELLO testi
erişimli tekrarda geçti; baseline birleşik kabul değildir.
Kullanıcı uygulamayı sol6.1 ile ayrı, sıralı görevlerde yaptıracak.

## Amaç ve doğrulanmış başlangıç

7×7 çerçeveli daire plakasını, yeni kamera/kalibrasyon GUI'sini ve CCUX-23b
performans çalışmalarını koruyarak A0–E2 kapsamını aynı üründe birleştirmek.
Nihai çalışma dalı `CameraIntegration` olacak. Ana dalı güncelleme/push bu planın
parçası değildir. Kalibrasyon tekrar yazılmayacak, yeni ürün özelliği eklenmeyecek.

- Mevcut HEAD/main: `cd9bdf2`; CalibrationImprovements merge'i.
- İleri dal CameraIntegration: `3793198`; main bunun atası, arada 117 commit var.
- İleri dal progress'i kullanıcının plaka kalibrasyonunu başardığını kaydediyor.
  Bağımsız fiziksel mm kabulü bundan ayrı ve açık.
- A0–E2 mevcut main üzerinde tracked/untracked değişiklikler; tek commit değil.
  Adlandırma kullanıcı kapsamıdır: A1, B0–B4, C1, D1–D4, E1/E2 teslimleri var;
  A2 kaldırılmış, C2 koşullu ertelenmiş. Var olmayan A0 uygulaması türetilmeyecek.
- İki hattın commit tabanları 469 dosyada farklı. Mevcut aktarımın 27 tracked
  C++/test/CMake yolu ileri dalda da değişmiş. Bu sayı Git conflict sayısı değildir.
- İlk yedek dal girişimi Git ref yazma iznine takıldı; dal/commit/index değişmedi.
  `build/integration-a0-e2/original-tracked.patch` ve `original-status.txt` var.
  **Bu patch untracked dosyaları içermez; tam yedek sayılmaz.**
- Yerel remote-tracking ref'ler incelendi; sunucuya fetch yapılmadı.

## Uygulama kuralları

1. Kartlar sırayla, tek ajan/tek görev olarak yürütülür. Her kart önkoşul kaydını
   okur; tamamlanmış işleri tekrar başlatmaz. Sonraki kart otomatik başlamaz.
2. İlk kart tüm mevcut çalışmayı doğrulanmış snapshot ile korur. Ardından
   `CameraIntegration` tabanlı `codex/camera-a0-e2-integration` üzerinde çalışılır.
   Son kart doğrulanmış sonucu CameraIntegration'a fast-forward eder.
3. Büyük snapshot'ı tek cherry-pick/merge ile uygulama. Eski ve yeni API'leri
   karşılaştırarak davranış bazında taşı. Yeni GUI/7×7 algoritması/session/cache
   dosyalarını eski sürümleriyle değiştirme. `ours/theirs` toplu çözümü yok.
4. Yapısal inceleme codebase-memory ile; dal değişiminden sonra kök/branch,
   index freshness ve dayanak dosya coverage tekrar doğrulanır. Mevcut graph
   main'i temsil eder; CameraIntegration kaynakları başlangıçta `git show` ile
   okundu. Graph sonucu ileri dalı doğruluyor diye sunulmaz.
5. Hedef dal AGENTS uygulanır: kısa progress/README, ayrıntılı kapanış arşivi,
   `scripts/check-context-docs.py`, görev ID'li commit. Her kapanışta kapsam,
   değişen dosyalar, komut/sonuç/kanıt, sınırlar ve sonraki kart yer alır.
6. Günlük başlatıcı `VisionFlow.cmd` korunur. Kullanıcı projeleri, kalibrasyon
   katalogları, kamera ayarları ve `config/local.ui.ini` değiştirilmez. Fixture,
   test ayarı, log ve PNG build altında; gerçek kullanıcının projesi test girdisi
   olarak yazılmaz. Fiziksel çekim kendiliğinden başlatılmaz.
7. Git izin hatasında aynı yetkili işlem için uygun escalation kullanılır;
   izin başarısı varsayılmaz. Push/force-push/reset/clean yapılmaz.

## Değişmezler ve çatışma noktaları

- Yeni CameraManager, kamera merkezli Calibration, iki yöntem ve çerçeveli 7×7
  plaka (49 merkez/yön/model/replay) korunur. Eski kalibrasyon gezinmesi dönmez.
- Load çevrimdışı kalır. Açık kaynak doğrulama yalnız oturum izni verir; Run,
  üretim çevrimi, sonuç veya kalibrasyon hesabı başlatmaz.
- İleri dal `prepareRemembered`, availability ve trusted/experimental ayrımı
  getiriyor; A0–E2 `reauthorizeRemembered` ve configuration snapshot getiriyor.
  Aynı doğrulamanın iki farklı uygulaması oluşmayacak. İzin diskten canlanmaz.
- İleri dal Run ile kamera prepare/Starting ve Stop ile gerçek drain uygular.
  A0–E2 verification bu sahiplikle uyarlanmalı; verification bittiğinde sahip
  bırakılmadan Run açılmamalı. Gerekirse mevcut session servisi üzerinden ortak
  probe; paralel ikinci SDK sahibi veya kare başına yeniden aç/kapat eklenmez.
- Webcam, dosya replay, native industrial ve normal OECS yolu ayrı politika
  taşır. Experimental model/probe trusted production iznine yükseltilmez;
  PLC/robot kapıları gevşetilmez. Görünür ürün akışına eski opt-in kutuları dönmez.
- Aynı kamerayı paylaşan logical kaynaklarda ileri dalın uyumlu profil/context
  paylaşımı korunur. Eski B3 duplicate reddi körlemesine taşınmaz; çelişen profil,
  ayrı source sonuç kimliği ve bütün-set kabulü birlikte test edilir.
- Typed hata/ilk neden, immutable completion, aynı sonuca kayıt retry ve ACK
  kuralları korunur. Retry yeni çekim/yürütme başlatmaz.
- Output filtresi yalnız görünüm: actual payload, DAG, hesap, Logs/CSV ve mevcut
  izinli PLC çıktısı değişmez. İleri dalın experimental Runtime özetindeki ek
  scalar satırları da gizlenmiş çıktıyı geri sızdırmamalı.
- CCUX-23b catalog cache/display context, lazy sayfa yenileme ve Properties
  commit ayrımı korunur; checkbox yeni decode/replay veya model fit üretmez.
- Eski E1 94/94 ve 77/77 kanıtları yalnız eski tabana aittir. Yeni kabul yerine
  kullanılamaz. CameraIntegration'da kaydedilmiş `multi_source_production_contract`
  “Missing result” hatası baseline'da yeniden değerlendirilir.

## INT-01 — Tam koruma ve entegrasyon dalı

**Önkoşul:** bu plan ve mevcut status; henüz ürün değişikliği yok.

- Tracked diff, bütün untracked ürün/test/belge dosyaları ve branch SHA'larını
  envanterle. `.codex/config.toml` yerel ayardır; ayrı kopya/hash ile koru,
  ürün commit'ine katma. Ignored build kanıtları ve kullanıcı varlıklarını koru.
- `codex/a0-e2-preserved` üzerinde mevcut A0–E2 kaynak/test/belge/başlatıcı ve
  bu planı snapshot commit'ine al; commit dosya listesi/hash ile tamlığı denetle.
  main'i ilerletme. Snapshot SHA'yı kapanışa yaz. Git bundle veya eşdeğer ikinci
  kurtarma kopyasını build altında oluştur ve okunabildiğini doğrula.
- Hedef CameraIntegration SHA'sını doğrula; ondan entegrasyon dalını oluştur.
  Yerel config hedef daldaki tracked dosyayla çakışırsa önceden içerik karşılaştır;
  kullanıcı ayarını kaybetme, sessizce üzerine yazma.
- Planı hedef dala getir. Eski büyük progress'i tarihsel arşive taşı/bağla;
  hedefin kısa progress ve güncel README biçimini koru.

**Kabul:** iki kaynak hattı geri alınabilir, hedef ancestry doğru, tüm untracked
ürün dosyaları snapshot'ta, kullanıcı varlıkları aynı; ürün portu başlamadı.
**Commit:** `INT-01: preserve A0-E2 and establish integration baseline`.

## INT-02 — Davranış/API matrisi ve hedef baseline

**Durum: tamamlandı — 25 Ekim 2026.** Matris/64 yol/185 hunk envanteri ve ayrı
Release baseline [kapanıştadır](../archive/2026-10-25-int-02.md). 43 farklı hedefte
erişimli tekrarlar dahil 40 başarı/3 açık assertion; tek yeşil corpus iddiası yok.
Ürün aktarılmadı, INT-03 başlatılmadı.

**Önkoşul:** INT-01 snapshot SHA ve entegrasyon dalı.
**Oku:** hedef README/progress, CCUX contract C0/C2a/C3, acquisition-sessions,
framed-plate sözleşmesi; snapshot A0–E2 fix-plan/source-verification/error-recovery.

- `MeasurementWorkspace`, `ScopedActiveCalibration`, `RuntimeService`,
  `RuntimeCoordinator`, `RuntimeCameraSessions`, `ExperimentalRuntime` ve
  `IndependentCalibrationCapture` giriş/worker/owner/Stop ilişkilerini incele.
- Dört kaynak politikası × Load/Verify/Run/Cycle/Stop/Retry tablosu yaz.
  configuration-only ve operational snapshot'ın nerede kullanılacağını,
  `prepareRemembered`/`reauthorizeRemembered` tek uygulama stratejisini belirle.
- Eski değişen her dosya/sembol için taşınacak/eşdeğer mevcut/uyarlanacak/
  tarihsel kayıt kararını gerekçeyle envanterle; hiçbir A0–E2 parçası kaybolmasın.
- Hedefte ayrı Release baseline build; CTest isimlerini `-N` ile doğrula.
  Plaka/calibration/session/multi-source/editor responsiveness baseline testleri
  ve kamera/kalibrasyon normal-küçük ekran PNG'lerini al, gerçekten incele.
  Eski Missing result hatasını tekrar üret veya temiz geçtiğini kanıtla.

**Kabul:** API matrisi somut, baseline log/PNG hazır, yeni/eski başarısızlık ayrımı
var. Başarısız baseline birleşik kabul diye işaretlenmez.
**Commit:** `INT-02: define integration contracts and capture baseline`.

## INT-03 — Tanı ve hata/kayıt kurtarma

**Durum: tamamlandı — 25 Ekim 2026.** A1/C1 mevcut Starting/Stop/drain ile
birleştirildi; Release/test/görsel ve üç baseline hata ayrımı
[kapanıştadır](../archive/2026-10-25-int-03.md). INT-04 başlatılmadı.

**Önkoşul:** INT-02.
**Sahiplik:** RuntimeDiagnostics, RuntimeCoordinator, RuntimeService,
PlcRuntimeSocket, ilgili testler/CMake ve gereken Runtime hata sunumu.

- A1 tanı manifest/first-failure/ticket/cycle zincirini yeni Starting,
  camera preparation/drain ve experimental yollarına uyarla.
- C1 PersistenceFailure/InvalidCompletion/Busy/edinim/Stop ayrımını taşı.
  Yeni kamera sahibi kapanışını eski state-machine koduyla ezme.
- Dondurulmuş completion retry, pending ACK ve geç sonuç baskılamayı test et.
  Katkısız kaynak kodu kopyalamak yerine ilgili davranışları taşı.

**Kabul:** Release ilgili core/Runtime/PLC tests; UI değiştiyse ilgili smoke ve
görsel kontrol. Retry çekim sayısını artırmaz, Stop gerçek drain'i bekler.
**Commit:** `INT-03: integrate runtime diagnostics and recovery`.

## INT-04 — Statik model çözümü ve exact yeniden izin

**Önkoşul:** INT-03 ve INT-02 politika matrisi.
**Sahiplik:** ScopedActiveCalibration, MeasurementWorkspace configuration yolu,
ProductionRecipeService/MultiSourceRuntimeService bağımlılık sağlayıcıları.

- B1/B2'yi ileri dal availability, permission epoch, profile/device revision,
  experimental modeller ve framed model kayıtlarıyla birleştir.
- Load kayıtlı exact modeli oturum izni istemeden inceleyebilsin; çalışma snapshot'ı
  güncel izin istesin. Mevcut prepareRemembered ile tek doğrulama uygulaması kullan.
- Seçim/model dosyasını değiştirmeden yeniden izin; model/content/version,
  kamera/kurulum/profil/geometri, değişmiş raster ve A→B→A invalidation sınansın.
- Trusted ve experimental negative testleri, legacy/no-op serialization/hash,
  framed model save/reopen/replay ve catalog cache regresyonlarını kapsa.

**Kabul:** restart model hesabı gerektirmez; yanlış probe ve yanlış izin sınıfı
reddedilir; kalıcı kayıtlar değişmez; ilgili Release testleri geçer.
**Commit:** `INT-04: reconcile remembered calibration and runtime configuration`.

## INT-05 — Kaynak doğrulama ve kamera sahipliği

**Önkoşul:** INT-04.
**Sahiplik:** SourceVerificationService, MeasurementWorkspace, independent capture
bağlantıları ve gerekli camera session adaptasyonu; B3 application katmanı.

- Immutable worker job/owner accept, fingerprint, Ready invalidation, whole-set
  admission ve cancellation'ı ileri dal kamera sahipliğiyle birleştir.
- Webcam/OECS/native ve offline replay için INT-02 kararlarını uygula. Normal
  OECS yolunu legacy webcam probe'una gönderme; unsupported yolu açık reddet.
- Aynı fiziksel kamera/uyumlu profil logical kaynak paylaşımı ve çelişen profil
  reddi; 1/2/10 kaynak, raw-only ve karışık dalları test et.
- Stop, geç/duplicate completion, proje değişimi, preview/Editor/Runtime Busy,
  başarısız son kaynak ve gerçek drain öncesi yeniden sahiplik senaryoları.

**Kabul:** verification model fit/production cycle/kayıt/PASS üretmez; kısmi başarı
bütün sete izin vermez; yeni generation'a eski completion geçmez; testler geçer.
**Commit:** `INT-05: integrate source verification with camera sessions`.

## INT-06 — Runtime kullanıcı akışı ve restart regresyonu

**Önkoşul:** INT-05.
**Sahiplik:** MainWindowRuntime/Workspace/Calibration/headers, Geometry hata
bağlamı ve RestartRuntimeSmoke/RuntimeFailureSmoke.

- B4 Load → Kaynağı doğrula → Run → Tek çevrim akışını yeni Runtime'a bağla.
  Load/restore capture açmaz. Run'ın async prepare ve Stop/drain davranışı kalır.
- Model adı/sürümü, readiness ve kaynak hatası görünür; başarılı verify Run'ı
  otomatik başlatmaz. Ham ve replay dallarına gereksiz kalibrasyon kapısı ekleme.
- Native ve normal OECS/experimental sonuç/Logs yollarını koru; Geometry hata
  yönlendirmesi mevcut yeni Camera/Calibration ekranına gitsin.
- Üç applicationType, tek/N kaynak ve restart süreç testi; verification sırasında
  Stop/proje değişimi, disk retry/ACK ve mevcut kamera session smoke'ları.

**Kabul:** Release ilgili CTest ve normal/850×600 PNG incelemesi; eski kalibrasyon
sayfaları dönmez; yanlış trusted üretim/PLC yolu açılmaz.
**Commit:** `INT-06: connect restart verification to current runtime UI`.

## INT-07 — Output tercihi, Editor ve Runtime sunumu

**Önkoşul:** INT-06.
**Sahiplik:** presentation OutputVisibilityPolicy/Preferences,
MainWindowEditor/Execution/Runtime ve ilgili smoke/test/CMake.

- D1–D4 port/default/explicit-empty/reset ve atomik INI tercihini taşı.
  Mode değişimi, tool/project/recipe kimliği ve immutable Runtime kimliği korunsun.
- Yeni Properties commit/display-context cache akışına uyum; toggle model
  replay/decode/fit veya execution başlatmasın, recipe dirty yapmasın.
- Experimental Runtime headline dahil tüm sonuç listelerinde filtre uygulanır;
  hidden invalid/durum/gerçek karar korunur. Viewer overlay ayrı kalır.
- DAG/payload/cache/terminal/Logs/CSV ve izinli PLC tesliminde before/after
  değişmezlik; hidden bağlı port, empty/reset/restart ve mod değişimi testleri.

**Kabul:** Release ilgili testler, yeni Editor/Runtime görselleri; CCUX-23b
responsiveness sayaç/baseline karşılaştırması. Güvenlik kapısı gevşetilmez.
**Commit:** `INT-07: integrate output preferences without editor regressions`.

## INT-08 — Birleşik yazılım ve görsel kabul

**Önkoşul:** INT-03–07 başarılı; INT-02 baseline sorunları kayıtlı.

- Final kaynakta Release UI/ZXing ON ve ayrı UI OFF/ZXing OFF build; her birinin
  tam mevcut CTest corpus'unu çalıştır. XML/log, kaynak/binary/cache manifesti tut.
  Socket erişim engelini assertion hatasından ayır; erişimli tekrar doğru raporlanır.
- Plaka 49 merkez/yön/fit/save/reopen/replay; bilinen ölçü/chessboard;
  camera/session/Stop/drain; restart/whole-set; output/retry/Logs/CSV matrisi.
- INT-02 Missing result, schema9 pre-cancel ve Camera Manager kayıt/ayar
  assertion'ları sürüyorsa her birini dar failing regression ile çöz; geniş yeni
  özellik açma. Başarısızlık kalırsa kartı tamamlandı diye sunma.
- Kamera ekleme/ayar, kamera merkezli kalibrasyon, 7×7 wizard, Editor ve Runtime
  normal/850×600 ekranları gerçekten açıp incele. Fixture fiziksel kabul değildir.
- Son kaynakta CCUX-23b performans kabulünü yeniden ölç; eski hız rakamını yeni
  sürümün ölçümü olarak kullanma. Gereksiz decode/replay/hidden refresh olmamalı.

**Kabul:** iki build ve tam corpus yeşil, görsel/performance kontrolü tamam,
aktarılacak envanterde her parça bir final karşılığa bağlı. Fiziksel E2 ayrı açık.
**Commit:** `INT-08: verify combined camera calibration and runtime acceptance`.

## INT-09 — Belgeler, başlatıcı ve CameraIntegration'a kapanış

**Önkoşul:** INT-08 kabul manifesti final kaynakla aynı.

- Kısa README/progress ve ilgili gereksinim/mimariyi birleşik gerçekle eşitle.
  Eski A0–E2 “industrial grab yok” gibi eski tabana ait hükümleri tarihsel kapsam
  olarak işaretle; yeni OECS edinimi ile native production engelini doğru ayır.
  Eski E1/E2 kanıtlarını koru, birleşik kanıtlarla karıştırma.
- VisionFlow.cmd ve run-local.ps1'i taşı/koru; varsayılan reference-ui EXE'sinin
  birleşik kaynakla derlendiğini hash/build kaydı ve launcher smoke ile doğrula.
  Başlatıcı ürün davranışını değiştirmez. config/local.ui.ini aynı kalır.
- Kısa doküman sınır kontrolü, diff whitespace, conflict marker, status ve
  envanter tamlığı. Açık fiziksel/SDK/native/PLC/robot/lens sınırları kaybolmaz.
- Görev commit'ini oluştur; CameraIntegration hâlâ beklenen ata ise yerelde
  `--ff-only` ile entegrasyon dalına ilerlet ve çalışma dalını CameraIntegration
  bırak. Dal değişmişse yeniden karşılaştır; force kullanma. main'e/push'a dokunma.
- Snapshot/yedek ref ve bundle korunur. Nihai commit, dal ve başlatma komutu ile
  fiziksel restart gözlemi için gereken kaynak/reçete bilgisi raporlanır.

**Kabul:** kullanıcı yeni GUI + 7×7 + A0–E2 birleşik sürümünü mevcut başlatıcıyla
açar; CameraIntegration aktif; yazılım kabulü kanıtlı, fiziksel kabul dürüstçe açık.
**Commit:** `INT-09: finalize integration handoff and local launcher`.

## sol6.1'e verilecek görev metni

Her kart için aynı şablonu, yalnız ID'yi değiştirerek kullan:

> C:/Projects/Tvision deposunda docs/architecture/camera-integration-a0-e2-plan.md
> planındaki yalnız INT-01 görevini uygula. Önce AGENTS.md, README.md ve
> docs/progress.md; sonra kartın önkoşul/kapanış kayıtlarını oku. Tamamlanmış işleri
> yeniden yapma. Mevcut A0–E2 ve CameraIntegration çalışmalarını koru. Planın
> branch/yedekleme, API uyarlama, doğrulama ve kapanış kurallarını izle; sonraki
> karta geçme. İlgili build/test/görsel kontrolleri tamamla, kanıt yollarını kaydet,
> progress/arşivi güncelle ve görev ID'li commit oluştur. Push yapma. Önkoşul
> eksikse somut eksikliği raporla; tamamlandı diye işaretleme.

Sıra: **INT-01 → INT-02 → INT-03 → INT-04 → INT-05 → INT-06 → INT-07 → INT-08 → INT-09**.
INT-02 kaynak incelemesi kart büyüklüğünü artırırsa ilgili kartı başlamadan önce
daha küçük alt kartlara böl; sözleşme ve kabul kapılarını kaldırma.
