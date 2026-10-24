# CCUX-23 — Editor düzenleme gecikmesini giderme planı

24 Ekim 2026 — CCUX-PLAN-06. Durum: 23b-1–5 tamamlandı; devam 23b-6.
Uygulayıcı: Codex. Uygulama alt görevleri için kullanıcı tercihi: Sol 6.1.

## Başlangıç ve sınır

[CCUX-23a analizi](../../archive/2026-10-23-ccux-23a.md) tamamlandı; tekrar
başlatılmaz. Oradaki CCUX-23b önerisi aşağıdaki alt kartlara ayrılır.
Kamerasız eski fixture'da Save medyan 961,16 ms, parametre 508,81 ms;
UI örneklemi yalnız üç tekrar, paint dahil değil. Bunlar yeni ölçüm değildir.
Kullanıcının canlı projesi ve framed 7×7 modelinin birebir baseline'ı değildir.

Hedef: Save/Add/parametre/ROI düzenlemesindeki UI beklemesini azaltmak;
kalibrasyon doğrulaması, source invalidation ve immutable Runtime'ı korumak.
Kalibrasyon solver'ı, SDK/timeout/bağlantı ömrü, lens, distorsiyon, PLC/robot,
CCUX-15 fiziksel mm ve CCUX-22f kabulü kapsam dışıdır.

Oku: AGENTS, progress, ana README; bu dizinin README'si, contract C0/C2a/C3/C4/C5,
bu planın ortak bölümleri ve yalnız seçili alt kart. Eski 23a kapanışı baseline
için okunur. Başka belgeler yalnız seçili değişikliğin bağımlılığı kadar açılır.

## Doğrulanan teknik başlangıç

İnceleme HEAD: `5a3673794fe46f17347a48666a0a3e21ff9be00d`.
Codebase Memory projesi `C:/Projects/Tvision`, Tier 2; indeks generation
`2026-10-24T07:12:40Z`. list_projects/index_status ardından search_graph,
trace_path, get_code_snippet ve check_index_coverage kullanıldı.

- `src/app/MainWindowEditor.cpp`: applyToolProperties (572–622) no-op durumda da
  updateTool/refreshWorkspace yapıyor; addSelectedTool ayrıca refresh yapıyor.
- `src/app/MainWindowWorkspace.cpp`: refreshWorkspace (280–317) bütün sayfaları
  yeniliyor; saveProject (318–323) apply ardından ikinci yenilemeye giriyor.
- `src/app/MainWindowExecution.cpp`: editorContext → systemCalibrationIdentity;
  kimlik oluşturma status/snapshot zincirini kullanıyor.
- `src/application/ScopedActiveCalibration.cpp`: status (129–144) records ve
  snapshots/experimentalSnapshots ile matematik ve oturum iznini sorguluyor.
- `src/application/ActiveCalibrationService.cpp`: constructor → read →
  decodeRecords; records → read → decodeRecords. Tekrarlı decode kaynakta teyitli.
- `src/application/MeasurementWorkspace.hpp:19`: environment registry reload yapıyor.
  `src/app/CalibrationOverview.cpp:77–116` kart/geçmiş/status yeniden kuruyor.
- `src/application/MeasurementEnvironment.cpp` repository load/bağımlılık
  doğrulaması katalog paylaşımının ikinci tüketicisi olarak ele alınmalı.

İlgili gerçek kaynak yolları coverage ile denetlendi. MeasurementWorkspace.hpp
65 ve 90 kısmi parse aralıkları kaynak okunarak tamamlandı. Yanlış tahmin edilen
MeasurementWorkspace.cpp ve src/ui/CalibrationOverview.cpp yolları kanıt değildir;
yukarıdaki gerçek yollarla düzeltildi. Diğer denetlenen yollarda kayıtlı kapsam
sorunu yok; bu tamlık garantisi değildir. Graph bazı genel isimli C++ çağrılarını
yanlış eşleyebilir; Save ve kimlik zincirleri kaynakla doğrulandı.

## Sıra ve devir

Çalışma sıralıdır: **23b-1 → 23b-2 → 23b-3 → 23b-4 → 23b-5 → 23b-6**.
Her kart tek alt görev, tek görev ID'li commit ve ayrı kapanıştır. Aynı anda
yalnız bir Sol 6.1 alt ajanı çalışır; yönetici uygulama sırasında aynı dosyaları
değiştirmez. Yönetici diff, yeni kanıt ve açık riskleri inceleyip sonraki karta
geçişi yürütür. Bu plan uygulamayı veya alt ajan başlatmayı gerçekleştirmez.

Her devirde: güncel HEAD, seçili kart, sahip olunan dosyalar, önceki kapanış,
graph projesi/tier/generation, kapsam boşlukları ve açık sorular verilir.
Alt ajan list_projects/index_status ile güncelliği denetler; değişecek yolları
coverage ile doğrular. Yeni sembolleri önce graph üzerinden bulur.
Başka kişilerin değişikliklerini geri almaz. Devir kaynağı repo belgeleridir.

## CCUX-23b-1 — Tekrarlanabilir ölçüm ve sayaçlar

Sahiplik: yeni `tests/` ölçüm fixture/probe'u, gerekirse CMake test kaydı ve
dar gözlem noktaları. Eski `build/ccux/ccux-23a/` kanıtını değiştirme.

23a probe'larını inceleyerek izlenen, tekrar derlenebilir bir ölçüm yolu kur;
private header kopyasını kalıcı çözüm yapma. Aynı eski fixture'ı ayrı kopyada
çalıştır; framed model, kalibrasyonsuz ve çok kaynaklı vakaları ayrı etiketle.
Fixture kurulumunu zamanlanan bölgeden çıkar. Her değişiklik gerçekten işlensin;
dirty Save ile no-op Save ayrı ölçülsün. Add/ROI için başlangıcı her tekrarda
eşdeğer duruma getir; giderek büyüyen reçeteyi sabit fixture diye raporlama.

Bir ısınma + en az 20 ölçüm, ham CSV, medyan ve nearest-rank p95 kaydet.
İşlem süresi, ilk ilgili paint'e kadar süre ve event-loop gecikmesini ayır.
Offscreen'de fiziksel ekran sunum süresi ölçüldüğünü iddia etme.
Refresh, context, katalog okuma/decode/replay sayaçlarını raporla; atomik Save'in
disk süresini UI yenilemesinden ayır. Donanım çağrısını test factory ile say.

Kabul: güncel Release'te yeniden üretilebilir baseline, normal process exit,
başarısız örneklerin ayrı raporu. Mutlak ms eşiğini gürültülü CTest'e bağlama;
deterministik sayaç/semantik kontrollerini test et. Ürün optimizasyonu bu kartta yok.

## CCUX-23b-2 — Properties commit ve tek yenileme

Bağlı: 23b-1. Sahiplik: MainWindowEditor.cpp, MainWindowWorkspace.cpp,
ilgili MainWindow bildirimi ve Editor testleri.

Properties okuma/doğrulama/mutation ile ekran yenilemeyi ayır. Sonuç no-op,
değişti veya başarısız ayrımını taşısın; mevcut hata davranışı korunsun.
No-op karşılaştırması normalize edilmiş mevcut serialized anlamı korusun;
legacy default ekleyerek sahte dirty/hash farkı üretmesin. Geometry operation
değişimindeki bağlantı temizliğini koru. Save/Add öncesi pending property bir kez
commit edilsin; hata halinde Save/Add ilerlemesin. Bir eylem en fazla bir Editor
yenilemesi üretsin; no-op property tam workspace refresh başlatmasın.

Kabul: pending son değer diske gider; Save başarısızlığı dirty durumunu korur;
Add tek araç ekler, seçim/focus korunur; no-op hash/status değişmez. Yeni sayaç
kontrolleri, editor_services, ui_editor_usability, ui_multi_source_editor,
ui_recipe_fields, ui_editor_camera_sessions. Release + normal/küçük görsel kontrol.
Kamera Save/parametre düzenlemesinde yeniden açılmamalı.

## CCUX-23b-3 — Görünür sayfaya göre yenileme

Bağlı: 23b-2. Sahiplik: workspace/navigation refresh bağlantıları, ilgili
MainWindow bildirimi, Calibration/Runtime sayfa girişleri ve UI regresyonları.
Kesin navigation sembollerini graph ile bul; kapsamı bütün UI refactor'una açma.

Başlık/dirty/komut durumunu hafif güncellemeden, sayfa içeriğini yeniden kurmadan
ayır. Gizli Calibration/Runtime için değişim nedeni/generation kaydet; sayfaya
girişte gerekli içeriği bir kez yenile. Gerçek runtime sonuç/Stop bildirimlerini
ertelenmiş görsel yenilemeyle karıştırma. Yeni proje/reçete ve hata yolları eski
sayfa verisi göstermesin. Calibration değişimi ilgili Editor kaynağını invalid etsin.

Kabul: Editor parametre/Save sırasında gizli Calibration rebuild sayısı sıfır;
sayfaya girişte güncel model/durum görülür. Proje değişimi, geri dönüş, hata,
kalibrasyon aktivasyonu, Running/Stop geçişleri doğru. ui_calibration_overview,
ui_scoped_calibration_activation, ui_multi_source_runtime, ui_runtime_camera_sessions
ve 23b-2'nin etkilenen testleri; Release + normal/küçük görsel kontrol.

## CCUX-23b-4 — Doğrulanmış katalog/model paylaşımı

Bağlı: 23b-3. Sahiplik: ActiveCalibrationService, MeasurementEnvironment,
MeasurementWorkspace, ScopedActiveCalibration ve bu servislerin testleri.
Önce küçük API/ömür/invalidation tablosunu bu kart altında somutlaştır; sonra uygula.

23b-4 uygulama sözleşmesi:

| API / sahip | Ömür ve invalidation |
| --- | --- |
| `ValidatedCalibrationCatalog` / repository-workspace | Tek mutlak yol + proje ID'sine bağlı, mutex korumalı tek immutable nesil; global değil |
| `ActiveCalibrationService::catalogView()` | Her erişimde bounded tek byte dizisi okunur ve SHA-256 alınır; aynı digest'te doğrulanmış belge/model vektörü paylaşılır |
| Repository kopyası / scoped / legacy | Aynı proje cache tutamacını paylaşır; yeni proje ve Save As ayrı tutamaç oluşturur |
| Eksik / bozuk / farklı içerik | Eski görünüm cache'den çıkarılır; yeni içerik tümüyle doğrulanmadan yayınlanmaz; eksik dosya boş katalogdur, seçili bağımlılık reddedilir |
| Permission / environment | Cache'e alınmaz; revision, profile, exact kurulum, epoch ve işlem sonu fingerprint karşılaştırmaları sürer |

Dosya okuma/hash her erişimde korunur; tasarruf JSON/model decode ve replay'dir.
Sayaçlar dosya okuma ile replay'i ayrı raporlar. Dönen eski immutable tutamaçlar
Runtime snapshot'ını değiştirmez; yeni admission eski tutamaçtan izin çıkarmaz.

Proje/workspace ömürlü, sınırlı immutable doğrulanmış katalog görünümü oluştur.
Aynı içerik decode/replay sonucunu paylaş; environment bağımlılık kontrolü ve
records aynı doğrulanmış veriyi tüketebilsin. Matematik cache'i ile oturum kullanım
iznini ayrı tut. Sadece mtime/size, global singleton veya file watcher güvence değildir.
Tam içerik digest'iyle değişimi doğrula; dosya okuma ve replay tasarrufunu ayrı ölç.
Okunmuş tek byte dizisini doğrula ve yayınla; yarım yazım/TOCTOU için mevcut kilit,
revision ve işlem sonu tutarlılık denetimlerini zayıflatma. Başarısız doğrulamada
eski cache ile başarıya devam etme. Proje kimliği + yol ve içerik nesilleri ayrılsın.

Zorunlu invalidation/test matrisi:

| Değişim | Beklenen |
| --- | --- |
| Aynı doğrulanmış içerik | Model replay tekrarlanmaz; izin ayrıca kontrol edilir |
| Katalog save/activate/import veya dış dosya değişimi | Yeni içerik doğrulanır; bağımlı görünüm yenilenir |
| Aynı size/mtime ile değiştirme, bozuk/silinmiş/yanlış proje dosyası | Algılanır, eski başarı dönmez |
| Environment revision, setup/profile/geometri/model seçimi | İlgili kaynak kimliği ve uygunluğu yeniden değerlendirilir |
| Disconnect/izin epoch/restart, A→B→A | Eski kullanım izni geri gelmez |
| Rename, no-op metadata | Gereksiz serialized revision/hash değişmez |
| Proje değişimi/Save As | Projeler arasında cache/izin taşınmaz |
| Çalışan Runtime | Yeni modele sessiz geçmez; Stop/Load sözleşmesi korunur |

Kabul: aynı içerikte bir doğrulama nesli için bir model decode/replay; tüm negatif
vakalar mevcut ret davranışını korur. active_calibration_contract,
measurement_environment_persistence, scoped_active_calibration,
calibration_save_use, scoped_calibration_editor, scoped_calibration_runtime,
experimental_metric_runtime_contract, framed_calibration_plate_contract.
Release servis testleri; UI değişmediyse bu kartta görsel zorunlu değil.

## CCUX-23b-5 — İşlem başına Editor context paylaşımı

Bağlı: 23b-4. Sahiplik: MainWindowExecution.cpp, MainWindowEditor.cpp,
ilgili Editor context tüketicileri/bildirimleri ve testler.

Tek senkron refresh işlemi içinde environment/kalibrasyon kimliği/context'i
bir kez oluşturup tüketicilere geçir. Paylaşım sınırı global veya süresiz olmasın;
reentrant işlem/generation değişimi eski görünümü yeniden kullanmasın.
Salt gösterim kimliği execution admission yetkisi değildir. Run Flow, Runtime
Load/Run ve aktivasyon sınırlarında güncel exact bağ/izin doğrulaması sürsün.
Reconcile/overlay/flow'un farklı kaynak ve Raw/px anlamı korunmalı. Yeni SDK,
worker mimarisi veya genel async dönüşümü ekleme.

Kabul: bir refresh'te tek tutarlı context; kaynak A değişince B modeline fallback
yok; mm/downstream/world ROI invalidation doğru; ayar değişimi dirty/DRAFT
davranışı ve immutable Runtime korunur. image_refresh_contract,
roi_viewer_interaction, scoped_calibration_editor, editor_services,
ui_multi_source_editor, ui_editor_camera_sessions ve ölçüm sayaçları.
Release + normal/küçük görsel kontrol.

## CCUX-23b-6 — Birleşik performans ve doğruluk kabulü

Bağlı: 23b-5. Sahiplik: ölçüm/acceptance fixture'ları ve kapanış belgeleri.
Yeni optimizasyon büyürse bu kart içine gizleme; ayrı kart önerisiyle kaydet.

Aynı makine/Release/fixture ile 23b-1 karşılaştırmasını en az 20 tekrarla yap;
warm/cold ayrımını, model/kaynak sayısını, HEAD ve ayarları raporla. Üç applicationType,
çok kaynak, framed model ve kalibrasyonsuz akışı kapsa. Kirli Save disk maliyetini
ayrı raporla. Önerilen hedef salt düzenleme için medyan <50 ms, p95 <100 ms;
bu planın mühendislik hedefidir, mevcut başarı veya kullanıcı kabulü değildir.
Hedef aşılırsa kalan sıcak yolu ölçüp açık yaz; doğruluğu hız için gevşetme.

Kullanıcının canlı projesinin kopyasında bağlı kamera ve aktif 2448×2048 kareyle
Save/Add/parametre/ROI, tıklama→paint ve event-loop ölç. Donanım/proje yoksa
yazılım kabulünü tamamla, canlı kabulü açık bırak. Fake sayaç fiziksel kanıt değildir.
Normal/küçük pencerede seçim/focus/ROI, dirty, Calibration ve Runtime görünümü kontrolü.

İlgili testlerin birleşimini UI açık Release'te, servis regresyonlarını UI kapalı
Release'te bir kez çalıştır. Önce ctest -N ile kayıtlı adları doğrula. Sıfır test
başarı değildir. Mevcut multi_source_production_contract Missing result engelini
koru; yeni başarısızlıkları tarihsel engel diye sınıflama. Test başarısı fiziksel
mm, SDK production, CUDA veya dağıtım kabulü oluşturmaz.

## Her kartın kapanışı ve kopyalanabilir devir

Kanıt: `build/ccux/ccux-23b-N/`; komut, tarih, HEAD, exit code, ham CSV,
sayaçlar, test raporu ve gerekiyorsa görseller. progress güncel özetini değiştir;
`docs/archive/YYYY-MM-DD-ccux-23b-N.md` oluştur, arşiv dizinine bağla.
Uygulayıcı: Codex; alt ajan modeli Sol 6.1 ayrıca belirtilebilir.
`python scripts/check-context-docs.py` ve `git diff --check` çalıştır.
Her kart tek `CCUX-23b-N: …` commit; push yok. README/contract yalnız gerçek
davranış/sözleşme değiştiğinde güncellenir; tarihsel 23a kaydı değişmez.

> Sol 6.1 ile yalnız CCUX-23b-1 kartını uygula. AGENTS, docs/progress.md ve
> README.md ardından docs/architecture/camera-setup/README.md, contract.md ve
> 23-editor-responsiveness.md ortak bölümleri ile seçili kartı oku. Codebase
> Memory Tier 2, project C:/Projects/Tvision kullan; güncel index ve yol kapsamını
> doğrula. Dosya sahipliğine uy, başka değişiklikleri geri alma. Paralel ajan
> başlatma. Yeni kanıtla kabulü doğrula; progress/kapanış/arşiv dizinini güncelle,
> görev ID'li tek commit oluştur, push yapma. Sonraki karta kendiliğinden geçme.

Sonraki devirde yalnız kart ID'sini ve önceki kapanış bağlantısını değiştir.
