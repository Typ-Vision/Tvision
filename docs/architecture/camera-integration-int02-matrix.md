# INT-02 — Davranış/API matrisi ve uyarlama kararları

25 Ekim 2026. Uygulayıcı: Codex. Bu belge **gelecek aktarım sözleşmesidir**;
INT-02 ürün kodunu değiştirmez. Kaynak snapshot `230cc72`, karşılaştırılan hedef
`259e41f` (CameraIntegration tabanı `3793198`).
[Plan](camera-integration-a0-e2-plan.md), [64 dosya envanteri](camera-integration-int02-inventory.md),
[baseline/kapanış](../archive/2026-10-25-int-02.md).

## Dört kaynak politikası × altı komut

Tablo birleşik sürümün hedefidir. Verify application katmanı INT-05'te aktarıldı
([sözleşme](source-verification.md)); kullanıcı akışı INT-06'dadır.
Load SDK açmaz; dosya/model okuması edinim veya production izni değildir.
Raw/px dalı kullanılmayan kalibrasyona bağlanmaz. Her satır üç applicationType
için aynı application servisleriyle uygulanır.

| Kaynak politikası | Load | Verify | Run | Cycle | Stop | Retry |
| --- | --- | --- | --- | --- | --- | --- |
| Webcam | Statik reçete, dosya varlıkları ve gerekiyorsa exact kayıtlı model; izin yok | Yalnız metrik required kaynaklarda immutable independent job ile discovery/resolve + yeni kare/geometri; owner exact aktivasyonu | Bütün-set güncel verification/operational bağımlılık kontrolü; industrial context yok | Mevcut webcam adapter; yeni source/frame/cycle kimliği; gerçek sonuç | Worker cooperative iptal; geç completion bastırılır, iş bitmeden sahiplik bırakılmaz | Probe hatasında açık Verify; kayıt hatasında yalnız frozen completion; otomatik çekim yok |
| Dosya / replay | Okunabilir raster/sidecar ve exact model/configuration uyumu; köken korunur | Raw dal probe istemez. Metrik dosya canlı fiziksel izin yenileyemez; yeniden izin gerekiyorsa açık desteklenmeyen durum. Aynı-oturum mevcut replay yolu korunur | Hash/raster/geometri/model/köken ve bütün-set operational kuralları; eksik canlı izni varsayma | Gerçek dosya rasterı; experimental sidecar varsa ayrı ExperimentalRuntime yolu | Geç dosya completion'ı da iptal/generation denetiminden geçer | Dosya düzeltildikten sonra açık hazırlık; kayıt retry dosya yeniden okumaz |
| Native industrial | Reference/profile/setup/rota/model deklarasyonunu configuration yolu inceler; discovery/open/admission yok | Destekli backend ile ortak session üzerinden exact probe; native acceptance eksikse açık ret, sahte trusted evidence yok | requestStart → Starting → ortak prepare → allReady → güncel dependency/admission → Running; gerçek native G0–G4 kapıları korunur | Prepared context/generation, tek pending edinim ve bağımsız logical source metadata; trusted completion gerekir | Starting/idle/cycle hepsinde retire + gerçek close/drain; hatalı close karantina/Fault | Edinim hatasında Stop/drain sonrası açık Verify/Run; kayıt retry yeni prepare/acquire/execute çağırmaz |
| Normal OECS / experimental köken | Automatic politika uygun yolu seçer; kalıcı experimental model ve PGM+JSON statik incelenir; legacy opt-in UI yok | Webcam probe'una gönderilmez. Automatic independent request + ExperimentalOecs typed evidence; yalnız experimental izin yeniler | Mevcut automaticProvider ve ExperimentalRuntime; Starting/allReady/drain sahipliği korunur; manuel çevrim, PLC/robot kapalı | experimental_runtime/2, immutable model/hash/readback/source kimliği; NOT_EVALUATED parça kararı; trusted completion'a yükselmez | Ortak context gerçek drain; geç kare bastırma; OECS stream cleanup kare başına sürer | Yeni açık kaynak denetimi/Run; experimental frozen record retry aynı digest/kayıt; pending-command engeli çözülmüş sayılmaz |

## Configuration ve operational snapshot

| Giriş / mevcut durum | Uyarlama kararı | Kart |
| --- | --- | --- |
| MeasurementWorkspace::forRecipe / productionSources şu an snapshots veya experimentalSnapshots kullanır | Operational API aynı kalır; izin/model/kurulum/profil/geometri ve whole-set invariant zorunlu. Configuration için ayrı isimli sağlayıcı tercih edilir; bool seçeneği kullanılacaksa yalnız iç implementasyonda kalır | INT-04 |
| Snapshot configurationForRecipe ve productionSources(configurationOnly) | Exact persisted selection/model ID/content/version, operational setup, geometry ve arşiv retlerini hedef repository/catalog cache üstünden salt okunur çöz. Kayıtlı matematik uyumu kullanılabilirlik veya izin değildir. Trial/rejected ve yanlış trusted/experimental sınıfı reddedilir | INT-04 |
| RuntimeService::checkConfiguration | Hedefte industrial deklarasyon dalı var; non-industrial checker operational provider'a, automatic OECS checker ExperimentalRuntime::prepare'e gider. Bu nedenle Load kamera açmasa da restart iznine takılabilir veya industrial exact modeli eksik denetleyebilir. configurationScoped/configurationMulti ve experimental configuration karşılığıyla ayrılmalı | INT-04 |
| RuntimeService::prepare / requestCameraPreparation / cameraComposition / begin / requireReady | Configuration nesnesi bu girişlerden operational yetki alamaz. Run hazırlığında ve begin öncesi güncel izin/fingerprint/set kontrolü; session compose sağlıklı exact profile/model/provider kimliğini tekrar denetler | INT-04/05 |
| ScopedActiveCalibrationService::status / candidate | status kayıtlı matematik ve sessionPermitted ayrımını sağlar. candidate tek başına exact seçilmiş model veya kullanım izni sayılmaz; configuration resolver ayrıca persisted selection ve installation denetler | INT-04 |

Statik ve operational veri aynı alan tipini paylaşsa bile çağrı yeri açık ayrılır;
configuration snapshot execution'a veya trusted completion'a verilemez. Kalıcı
izin biti eklenmez. Load fit/replay ile modeli yeniden öğretmez; mevcut hash/cache
doğrulaması yeni katalog okumasını gereksiz çoğaltmaz. Kayıtlı çerçeveli 7×7 model
ve legacy no-op codec anlamı değişmez.

## Tek aktivasyon uygulaması

Hedef `prepareRemembered(ref, probe)` → `status(ref)` → `activate(ref, contentId, probe)`
→ `activateImpl` kullanır. Snapshot `reauthorizeRemembered(expected, probe)` exact
RememberedCalibration eşitliğini kontrol edip eski `activateSelection` çekirdeğine
gider. Eski çekirdek kopyalanmayacak.

INT-04'te expected-selection alan tek wrapper, hedef `activateImpl` yolunu kullanır.
Mevcut prepareRemembered istemcileri aynı çekirdeğe delegasyon yapar. Beklenen
selection activation öncesinde ve commit sınırında tekrar kontrol edilir; model
content/version, target revision, operational setup, profile/device revision,
geometry, permissionEpoch/cameraEpoch ve kullanılan probe denetimleri korunur.
Experimental probe yalnız experimental izin üretir; trusted hazırlığa düşmez.
Restart/replug ve A→B→A eski izin canlandırmaz. Model fit/kalıcı seçim değişimi
gerekmeksizin yeni exact kanıtla aynı matematik kullanılabilir. SaveUse'ın önceki
seçimi koruyan atomik davranışı ve catalog cache paylaşımı korunur.

## Giriş, worker, owner ve Stop sınırları

| Bileşen / kaynak dayanağı | Mevcut ilişki | Aktarım sınırı |
| --- | --- | --- |
| MeasurementWorkspace.hpp:17–30, 91–120 | MainWindow proje sahibi; registry/repository + scoped izin. productionSources tek before/after environment ve gerekli model setini doğrular | verifySources owner plan/fingerprint, configuration sağlayıcıları ve verification busy gate eklenir; UI/worker workspace mutasyonu yapmaz |
| ScopedActiveCalibration.cpp:49–87, 145–148, 172–190 | Owner thread aktivasyon; activateImpl trusted/experimental; snapshots izin gerektirir | Snapshot reauthorize/diagnostics davranışı mevcut epoch/cache/policy çekirdeğine eklenir |
| RuntimeService.cpp:37–72, 143–190, 212–246 | requestCameraPreparation ortak RuntimeCameraSessions; prepare native/multi/experimental dispatch; execute immutable ProductionCycle worker girişidir | A1 tanı ve C1 typed retry bu dispatch'i korur; configuration provider yalnız Load'da; whole-set verification prepare/begin/requireReady'de |
| RuntimeCoordinator.cpp:13–84 | Load offline; requestStart Starting; completeStart prepare/requireReady; poll future tüketir; stop generation artırır, cancel ve stopCameras; Stopping gerçek kamera işi bitene kadar sürer | Snapshot'ın daha eski state machine'i kopyalanmaz. First failure/frozen completion retry bu durumlara uyarlanır; kayıt beklerken kamera kapanabilir, ACK engeli kalır |
| RuntimeCameraSessions.cpp:40–75 | start immutable profile/model/provider identity; poll allReady hatasında bütün context kapanır; compose güncel identity kontrolüyle capability verir | Verification context'i Run'dan önce kapanmış/drained olmalı; ikinci SDK sahibi açılmaz. closeBudgetExceeded sahipliği bırakmaz |
| ExperimentalRuntime.cpp:67–143 | Ayrı snapshot/evidence/digest; provider model/köken kontrolü; execute gerçek sonuç, finish immutable pending record | Tanı/verification/output filtresi bu yolu kapsar. Retry trusted kayıt biçimine dönüştürmez; PLC/robot hâlâ kapalı |
| IndependentCalibrationCapture.cpp:56–87, 88–173 | prepareCamera canlı industrial giriş; begin owner immutable job; run worker actual edinim; accept owner ticket/generation; closeCamera/cancel oturumu emekliye ayırır | Verify ortak servis ve bu typed kanıt yolunu kullanır. prepare sadece bağlantıdır; model izni için gerçek probe gerekir. accept sonrası close + gerçek drain tamamlanmadan Ready/Run açılmaz |

Verification için önerilen sıra: owner plan → immutable worker probe → owner
fingerprint/exact kabul → close isteği → gerçek drain → bütün-set Ready. Worker
dönmesi industrial lease'in kapanması demek değildir. İptal sırasında ticket,
context ve proje sahibi worker + drain tamamlanana kadar yaşar. Geç/duplicate
completion yeni generation'a izin vermez. Kısmi başarıdaki önceki/geçerli bireysel
izinler korunabilir; bütün-set başarısızken Run kapalı kalır. Runtime Stop ile
bitmiş Ready ancak fingerprint/izin aynıysa korunabilir; yeni Load set kimliği
yeniden değerlendirilir.

Aynı fiziksel kameranın uyumlu industrial logical kaynakları aynı context/worker'ı
paylaşır; kaynak başına ayrı acquisition ve source/frame/cycle kimliği vardır.
Çelişen profile/route/geometry hazırlıktan önce reddedilir. Snapshot B3'ün blanket
duplicate CameraId/backend-index reddi industrial session paylaşımına uygulanmaz;
webcam için kanıtlanmış reuse politikası yoksa duplicate sahiplik reddi sürer.
İki logical kaynak aynı setup modelini kullanıyorsa snapshot çözümünde unique
SetupReference denetimiyle modeli bir kez çöz, kaynaklara ayrı bağla; mevcut
snapshots duplicate-ref reddini kapatarak çözme. Testler 1/2/10 kaynak, mixed/raw,
son kaynak hatası, lease Busy, Stop/late completion ve conflict profile kapsar.
Verification planı da aynı exact kamera/setup/profile/model için tek probe ve
tek reauthorization üretir; kaynak kimliklerine readiness/hata ayrı dağıtılır.
Aynı probe'u iki kez activate ederek usedProbes koruması aşılmaz. Uyuşmayan
operational setup veya farklı model seçimi paylaşım adı altında kabul edilmez.

## Kalibrasyon / Editor / sonuç sunumu korunacakları

CameraManager ve kamera merkezli CalibrationOverview, iki yöntem ve yeni 7×7
CalibrationWizard hedefte kalır. Eski makeCalibration sayfa gövdesi taşınmaz;
yalnız workspace verification ömrü mevcut girişe eklenir. Framed observe 49 gerçek
merkez/yön, raw köken/hash, fit/holdout kapısı, save/reopen/replay korunur. Görsel
fixture bağımsız mm veya fiziksel ayna/eksen kabulü değildir.

OutputVisibilityPolicy/Preferences ayrı presentation bileşenidir. Registry/DAG,
hesap/payload/cache/hash/dirty, immutable terminal, Logs/CSV ve izinli PLC scalar
değişmez. Yeni Properties commitToolProperties/showToolPropertiesView ve işlem
başına display context/cache API'lerine bağlanır. Toggle decode/replay/fit veya
execution tetiklemez. Runtime filtresi immutable runtime project/recipe/tool
kimliğini kullanır; experimental headline/sourceSummary ek scalar satırları dahil
tüm tekrar sunumlar filtreyi tüketir. Durum, hidden-invalid, kaynak/frame kimliği
ve gerçek karar gizlenmez; viewer overlay ayrı kalır.

## İnceleme güveni ve kabul kapısı

Codebase-memory Tier 2 kök/dal ready, ilk generation `2026-10-25T08:07:17Z`,
ek dayanak coverage generation `2026-10-25T08:15:08Z` (ürün kaynakları aynı);
arama → çağrı izi → snippet ve source diff kullanıldı. prepareRemembered izinde
caller sayısı snippet ile farklı, completeStart izinde prepare/requireReady
çözümlemesi kaynakla tam örtüşmüyor; doğrudan kaynak esas alındı. Graph yokluk
veya tam bağımlılık kanıtı sayılmadı. 14 ilk dayanakta coverage sorgusu; partial
MeasurementWorkspace 65/90, ScopedActiveCalibration.hpp 62/63/68,
RuntimeService.hpp 22/37/43/45/54, IndependentCalibrationCapture.hpp 29/76/80
satırları doğrudan okundu. Geniş scope raporu ve ek dayanak kontrolleri build
kanıtındadır; snapshot graph ile doğrulanmadı, git show/diff ile okundu.

INT-02 baseline başarısızlıkları birleşik ürün kabulü değildir. Özellikle bilinen
Missing result baseline ile ayrılır; bu kartta düzeltme/ürün aktarımı yapılmaz.
Bu koşumda ayrıca multi_source_graph_contract pre-cancel denetimi başarısızdır;
eski kapanışta sınandığı kanıtlanmayan bu test yeni gözlenen hedef baseline
bulgusu olarak kaydedilir. HELLO hataları erişimli tekrar sonucuyla ayrı tutulur.
INT-03 ancak ayrı kullanıcı göreviyle başlar. Native OECS pending-command,
fiziksel kamera/mm/lens/PLC/robot, MUI-04, DPI/çok monitör, CUDA, kurulum/dağıtım
sınırları açık kalır. C2 kullanıcıya özgü süreklilik olayı yeni rapor/failing
regression olmadan çözülmüş sayılmaz.
