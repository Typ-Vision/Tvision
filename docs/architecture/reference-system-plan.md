# Ortak Referans sistemi — uygulama planı

11 Ekim 2026. Kullanıcı kararı: **ROI pose ve nokta referansı kullanıcı arayüzünde Referans adıyla birleşir; X/Y/açı ofsetleri aynı bölümde bulunur.** R1–R8 tamamlandı. [Domain sözleşmesi](reference-binding.md), [servis/UI/kayıt sözleşmesi](reference-integration.md). R7a/b ortak Referans production yolu [uygulandı](reference-production-acceptance.md). R8 nihai OFF/core 57/57 ve ON/UI 64/64 paket kabulü tamamlandı; [kanıt ve devir](reference-release-acceptance.md). Güncel sıra **T8.4.2b.3c** kalan corpus/production kabulüdür; tamamlanan U1–U4/T8 işleri yeniden yapılmaz.

## 1. Plan başlangıcındaki durum ve değişiklik sınırı (R1 öncesi)

Mevcut `MainWindowTracking.cpp` seçicisi yalnız `pattern_pose` ve `shape_pose` kabul eder. Altı tüketici (`edge_point`, `edge_pair`, `line_fit`, `circle_fit`, `blob`, `code_read`) `poseReference` ve `poseTracking` kullanır. Eski `edge` ise Point `reference`, `referenceOrigin` ve `referenceGeometry` üzerinden öteleme yapar; Editor referans cache/öğretimi Template Match'e özeldir. Sadece etiketi değiştirmek genel nokta referansını sağlamaz.

Mevcut `template_match` sabit açı/ölçeklidir ve Point üretir. Ona bu paketle açı bulma algoritması eklenmez. Gerçek dönüş Pattern Pose/Shape Pose sonucundan alınır. UI ortaklaşırken typed Point/Pose2D ayrımı korunur; bir noktadan açı=0 ölçülmüş Pose2D uydurulmaz. [Mevcut pose kapsamı](pose-roi.md), [tool planı](recipe-tools-plan.md), [U1–U4 kabulü](editor-usability-plan.md).

Hedef tüketiciler: **Edge, Edge Point, Edge Pair/Caliper, Line Fit, Circle/Arc, Blob ve Code Read**. Bu araçlar ROI geometrilerini referansa göre taşır, ardından kendi algoritmasını çalıştırır. Ölçülen sonuç geometrisi yalnız referans kopyalanarak üretilmez.

Template/Pattern/Shape bu pakette referans üreticisidir. Matcher'ın kendi arama alanını başka matcher'a bağlamak, yeni OCR/kusur araçları ve yeni merkez türetme operatörleri ayrı kapsamdır. Image File/Webcam/Preprocess için Referans alanı eklenmez. Measurement/Calculate/Geometry Measurement mevcut feature girişlerini kullanır; ROI tüketicisi gibi ikinci kez taşınmaz.

## 2. Kullanıcı davranışı

- Tek bölüm adı **Referans**. İlk seçenek **Görüntüye sabit**; diğer seçenekler `Araç adı · Merkez/Nokta/Model referansı` biçiminde kaynak ve portu açıkça belirtir. Bağlantı tool ID + port ID ile saklanır; ad değişimi bozmaz.
- **X ofseti [px], Y ofseti [px], Açı ofseti [°]** aynı bölümde yer alır. Başlangıç değerleri sıfırdır. Bunlar çizimde kaydedilen temel ilişkiye ek düzeltmelerdir; sıfır ofset ROI'yi kaynağın üstüne taşımaz, öğretilmiş uzaklığı korur.
- Nokta kaynağında X/Y etkin, açı ofseti pasif ve sıfırdır; açıklama: “Bu kaynak yalnız konum üretir.” Çizilmiş ROI'nin kendi yönü korunur. Line yönü, Edge tarama açısı veya Circle geometrisinden otomatik parça açısı türetilmez.
- Pattern/Shape kaynağında gerçek konum+açı otomatik izlenir; açı ofseti etkinleşir. Ayrı ROI pose modu veya zorunlu dönüş checkbox'ı yoktur. Angle/scale arama ayarları matcher'ın kendi ayarları olarak kalır; mevcut tracking scale=1 sınırı korunur.
- Güncel kaynak sonucu ve ROI hazırken **Referansa bağla** çizilmiş ilişkinin öğretimini ve bağlantıyı atomik kaydeder. Seçici değişimi tek başına kayıt yapmaz. Mevcut “ROI ilişkisini öğret” eylemi bu ad altında devam eder; gerektiğinde **İlişkiyi yeniden kaydet** kullanılır. Normal proje Save diske yazar.
- X/Y/açı ofseti düzenlemek yeniden kaynak/model öğretimi gerektirmez; geometri/kimlik ve alan doğrulamasından geçen tek edit reçeteyi DRAFT/dirty yapar, hedef ve downstream cache'i geçersiz kılar. Upstream sonucu aynı karede geçerliyse korunur. Geçersiz edit eski kayıtlı ayarı bozmaz.
- Pose kaynağından Point kaynağına geçişte açı ofseti otomatik uygulanmaya devam etmez: pending açı sıfır ve pasiftir. Yeni ilişki ancak Bağla ile kaydedilir; İptal eski ilişkiyi korur. Geçerli güncel çizim ilişkisi kurulamazsa geçiş kaydedilmez.
- **Referansı kaldır** ROI'yi kayıtlı nominal/sabit geometrisine döndürür; son runtime konumunu sessizce reçeteye yazmaz. Kalibre Edge/Pair/Line'ın mevcut world ROI kaldırma davranışı açık etiketle korunur; Circle'ın bağımsız world ROI desteği korunur.
- Kaynak yok/bulunamadı/eski sonuç/Stop durumunda eski ROI ve ölçüm gösterilmez. Eksik güncel sonuç için kullanıcıya kaynağı çalıştırması söylenir. Run Tool gizli acquisition veya bütün akış çalıştırmaz.
- Basic görünümde kaynak ve uygun ofsetler erişilebilir. Öğretim kimliği, nominal mm ROI ve örnekleme ayrıntıları kapalı gelişmiş bölümde bulunur; mm ROI temel referans takibinin önkoşulu değildir. U1–U4 seçim/çizim/scroll/bölüm tercihleri korunur.

## 3. Kaynak portları ve tüketici matrisi

| Kaynak | Kullanılacak gerçek çıktı | Takip |
| --- | --- | --- |
| Template Match | `point` — model merkezi | X/Y |
| Pattern Pose / Shape Pose | `pose.position` + `pose.angle` — öğretilmiş model anchor'ı | X/Y/açı |
| Edge / Edge Point | `point` — bulunan kenar noktası | X/Y |
| Edge Pair / Caliper | `centre`; isteğe göre açık seçilmiş `first` / `second` | X/Y |
| Circle / Arc | `centrePx` — geçerli raw merkez | X/Y |
| Line Fit | `originPx` — mevcut fit referans noktası; etiket “Çizgi referans noktası” | X/Y |
| Blob | `point` — mevcut seçili sonuç noktası | X/Y |

Pattern/Shape anchor'ı her zaman model merkezi değildir. Eski kayıtlı anchor korunur ve seçicide merkez diye yanlış adlandırılmaz; model merkezine bağlanmak isteyen kullanıcı matcher öğretiminde anchor'ı merkeze yerleştirir. Bu plan matcher öğretim algoritmasını veya eski anchor'ları değiştirmez. Line origin de segment orta noktası diye sunulmaz. Blob'da başka blob'a ait nokta/önceki aday seçilmez; mevcut executor'ın geçerli seçili noktası tüketilir.

İlk sürüm kaynakları raw `px` ve aynı görüntü koordinat sistemindedir. `centreMm`, unit-vector, geometrinin tamamı, skor/sayı/bool veya Code sonuç listesinden otomatik nokta dönüşümü yapılmaz. Kalibre world Circle yolunda `centrePx` unavailable ise bu kaynak geçersizdir; mm merkezi px diye kullanılmaz. Yeni world→raw kaynak adapter'ı ayrı sözleşme gerektirir. Kaynağın merkez üretmesi yeterli değildir: o çevrimde ilgili port geçerli olmalıdır.

| Tüketici | Taşınan geometri / uygulama işi |
| --- | --- |
| Edge (legacy) | Search rectangle ve dört tarama yönü. Eski Point yolu aynen okunur; yeni pose yolu yerel tarama yönünü de döndürmelidir. AABB içinde eski yatay/dikey arama dönüş desteği sayılmaz. |
| Edge Point | A→B scan, tarama genişliği ve yön |
| Edge Pair / Caliper | Scan ve dik genişlik; iki gerçek kenardan merkez/mesafe |
| Line Fit | Dönük kutu, scan yerleşimleri ve fit gözlemleri |
| Circle / Arc | Merkez, halka yarıçapları, yay başlangıcı/signed sweep ve radyal scan'lar |
| Blob | Gerçek taşınmış/dönmüş maske; alan/delik/merkez davranışı korunur |
| Code Read | Maske ve decoded köşelerin içeride kalma kontrolü; yeni decode/skor algoritması yok |

Her tüketici Point ve Pose kaynak modlarını destekler. Kendi kendine veya downstream kaynağa bağlantı reddedilir. Point ve pose art arda zincirlenebilir; örneğin Pattern → Circle → Edge Point zincirinde son araç yalnız Circle merkezinin X/Y hareketini izler, Pattern açısını örtük miras almaz.

## 4. Ofset ve dönüşüm sözleşmesi

`q`: öğretilen ROI üzerindeki nokta; `p0`, `p`: öğretim/güncel kaynak noktası; `a0`, `a`: yalnız gerçek pose kaynağının öğretim/güncel açısı. Raw görüntüde +X sağa, +Y aşağı, pozitif açı saat yönüne. UI derece, iç matematik mevcut radyan sözleşmesiyle çalışır. `d=(dx,dy)`, `da` kullanıcı ofsetidir.

- Point modu: `q' = q + (p - p0) + d`. Ofsetler görüntü X/Y eksenlerindedir. Açı yoktur; `da != 0` codec/servis tarafından da reddedilir.
- Pose modu: önce `l = R(-a0) * (q - p0)`; sonra `q' = p + R(a) * (R(da) * l + d)`. Ofset X/Y kaynağın yerel eksenlerindedir ve kaynak döndükçe onunla döner. Açı ofseti ROI'yi kaynak anchor'ı etrafında döndürür; X/Y ofset vektörünü ayrıca döndürmez. UI yardımında “Model eksenlerinde X/Y” belirtilir.
- Öğretim sırasında sıfır ofset no-op'tur. Tekrar ofset uygulama veya yeniden öğretimde çift ofset oluşmaz: öğretim temel ROI/ilişkiyi saklar, kullanıcı ofseti tek yerde uygulanır. Mevcut ofsetlerle yeniden öğretim servisi bu farkı açıkça korur; teste bağımsız hesap oracle'ı eklenir.
- Örnek: `p0=(100,100)`, `q=(140,100)`, `p=(120,110)`, `d=(5,-2)`. Point sonucu `(165,108)`. Pose `a0=0°, a=90°, da=0°` sonucu `(122,155)`; ayrıca `da=90°` olursa `(82,115)`. Bu değerler formülü kopyalayan test yerine sabit oracle olarak kullanılır.
- Ofset alanları finite double, subpixel hassasiyetlidir. NaN/Inf/aşırı geometri, footprint dışı ve taşma açık ret üretir; sessiz clamp/kırpma/full-image fallback yapılmaz. Halka, yay, kutu ve scan gerçek geometrisi korunur.
- World ROI için aynı dönüşüm raw ara düzlemde uygulanır: `nominal mm → H^-1 → referans/ofset dönüşümü → current raw → H → current mm`. Ofset alanları burada da **px/°** olarak etiketlenir; px/mm takibi ölçüm sonucunun birimiyle karıştırılmaz. Her gerçek sampling tap mevcut kalibrasyon/hull/horizon/çözünürlük kapılarından geçer. Image açısı world açı diye gösterilmez; mevcut mm refit/metrik/tolerans korunur.

## 5. Domain, kayıt ve yürütme

Ortak uygulama servisi kaynak yeteneklerini, typed sonucu, teaching ilişkisini ve ofsetleri çözer. UI algoritma/SDK çağırmaz; üç applicationType aynı servis/Editor/Runtime kullanır. Mevcut PoseRoiService/WorldTrackingService ve Point yolu genişletilir; ikinci execution engine kurulmaz.

Yeni binding sürümü sourceTool/sourcePort, açık `translation` veya `rigid` modu, taught point veya taught pose, temel ROI kimliği, ofset/birim/eksen semantiği, kaynak görüntü/çözünürlük ve gerekli kalibrasyon bağımlılığını taşır. R1'de mevcut codec sınırlarına göre kesin alan listesi ve sürüm tahsisi kodla birlikte sabitlenir. Yeni kalıcı alanlar eski strict JSON'a sessiz eklenmez; engine davranış sürümü ve opt-in editor/tool schema gereksinimi birlikte değerlendirilir. Bu plan yürürlükteki sürüm numaralarını değiştirmez.

UI'da tek Referans bölümü olması portların tek belirsiz tipe dönüştürülmesini gerektirmez. İçeride Point ve Pose2D girişleri typed kalabilir; aynı tüketicide yalnız bir referans modu etkin olur. `region` ile ortak referans çatışması atomik reddedilir. Binding/bağlantı/sourcePort/mod uyuşmazlığı, silinen/disabled kaynak ve cycle DAG doğrulamasında görünür olur; UI'da gösterilen eksik referans sessizce Sabit'e çevrilmez.

Eski `edge.reference` ve raw/world `poseTracking` reçeteleri salt açma/no-op Save ile yeniden yazılmaz. Runtime'da uyumluluk adapter'ı eski anlamı korur: legacy integer ötelemenin bir kez yuvarlama davranışı, eski pose için sıfır ek ofset, model anchor ve world zinciri korunur. Kullanıcı ortak referansı gerçekten düzenlerse hedef binding açık sürümlü yeni biçime atomik geçirilir. Eski production snapshot/kayıt hash'leri ve eski codec yolları korunur; değişen reçete yeniden doğrulama ister.

Geçerlilik iki ayrı kavramdır: güncel sonucun parametre/transitive cache imzası ve öğretilmiş geometrinin kimliği. Model/anchor, kaynak portunun geometrik anlamı, tüketici temel ROI'si, source/coordinate/resolution veya gerekli calibration değişimi yeniden öğretim gerektirir. Eşik gibi geometri anlamını değiştirmeyen parametreler cache'i geçersiz kılar; yeni güncel sonuçtan sonra sırf eşik değişti diye yeniden ilişki öğretimi zorunlu olmaz. Noktanın her karede yer değiştirmesi öğretimi geçersiz kılmaz.

Kaynak tam ToolResult/owned image/frame/cycle/evaluation/method ile doğrulanır. Point için kendi üreticisinin port/validity sözleşmesi, Pose için mevcut beş-port/method/scale/ambiguity kapıları uygulanır. Not-found kaynak downstream SKIPPED; bozuk/stale/type/frame/calibration binding açık hata üretir. Önceki sonuç/nominal ROI fallback yoktur. Silme, kaynak değiştirme, ofset edit, Stop, geç worker/file callback, Save/reopen ve Flow taşıma mevcut generation/immutable snapshot ilkelerini korur.

## 6. GPT-6.1-sol / medium uygulama paketleri

Hedef çalışma profili kullanıcının belirttiği **GPT-6.1-sol, medium**. Bu bölüm devir talimatıdır; model/oturum ayarı değiştirildiği anlamına gelmez. Her alt paket tek ana sorumluluk, açık dosya sınırı ve ölçülebilir kabul taşır. Dosya adları hedef sorumluluklardır; eşdeğer mevcut sınıf genişletilir. Bir paket tamamlanmadan bağımlısı başlatılmaz; yetkilendirilmiş dizide rutin alt adımlar için tekrar onay istenmez.

| Paket | Önkoşul / iş ve dosya sınırı | Çıkış kabulü |
| --- | --- | --- |
| R1 — Domain matematiği ve binding | Plan; `domain/Coordinates`, `PoseRoi`, `PoseTrackingConfig`, `ToolValues`; yalnız gerekirse ortak ReferenceBinding. Ofset sırası, Point/Pose ayrımı ve strict codec. | Bağımsız yukarıdaki koordinat oracle'ları; 0/90/-90/wrap, subpixel, altı ROI şekli, Point açı reddi, finite/bounds, codec roundtrip/unknown/future retleri. UI/production açılmaz. |
| R2 — Registry, graph, eski reçete | R1; `ToolRegistry`, `ToolDefinition`, `ToolGraph`, `ProjectService`, project-format. Capability/sourcePort matrisi, atomik binding+connection, lazy migration. | Kaynak/tüketici uyumu, self/cycle/region/çift referans retleri, kaynak silme/rename/Flow taşıma, eski Edge/raw pose/world pose no-op byte/hash ve gerçek edit/save/reopen. |
| R3a — Kaynak çözümleme ve Editor cache | R2; `ToolDependencyResolver`, `EditorSession`, `PoseRoiService`, `ExecutionService`. Template'e özel referans cache yerine ortak current output yolu. | Matristeki gerçek Point/Pose üreticileri; model/ROI/port/source/frame/evaluation retleri, yalnız hedef yürütmede acquisition=0, transitive invalidation/Stop/geç callback. |
| R3b — Raw metrologi tüketicileri | R3a; Edge Point/Pair/Line/Circle adapter'ları ve execution context. | Her dört tüketici × Point/Pose; gerçek rasterda taşınan ROI + algoritmadan gerçek sonuç; ofset sırası, negatif yay, strict scale1. |
| R3c — Legacy Edge | R3b; `ToolExecutors`, gerekirse `vision` yerel yön sampler'ı. Yeni binding'de dönen kutu ve yerel dört yön; eski yolu koru. | Eski integer oracle aynı; yeni pose 90°/eğik rasterda gerçek yerel scan ve bulunan nokta; AABB/axis-aligned fallback yok. Yeni algoritma semantiği yöntem/sürümle açıkça ayrılır. |
| R3d — Blob ve Code | R3c; mevcut RectangleMask ve ilgili adapter'lar. | Point/Pose ve ofsetli gerçek maskeler; Blob delik/alan/merkez, QR iç/dış köşe ve frame sınırı; ZXing OFF açık capability, ON gerçek decode. |
| R4 — Kalibre world tüketicileri | R3d; `WorldTrackingService`, `PoseRoiService`, world binding/mapper; yeni kalibrasyon sistemi yok. | Dört world tüketici × Point/Pose; uniform/Y-up/non-affine H oracle, raw px ofset, current mm refit/arc/tolerans, her tap alan denetimi; unavailable centrePx kaynak reddi. |
| R5a — Ortak Referans Properties | R4; `MainWindowTracking`, `MainWindowEditor`, `MainWindowMatching`, ilgili tool editor'ları. | Yedi tüketicide tek bölüm; Sabit, portlu kaynak etiketi, X/Y/açı enable, pending/bağla/iptal/kaldır/yeniden kaydet; Basic/Detailed ve mm gelişmiş alanları. Normal/küçük görsel kabul. |
| R5b — Viewer ve etkileşim | R5a; ImageViewer ve Editor etkileşimleri. | Güncel çözülen geometri; ofset değişiminde tek uygulama, missing/Stop'ta temizleme, seçim/Escape/yeni çizim/geç file callback, kayıtlı nominal ROI'nin korunması. Normal/küçük görsel kabul. |
| R6 — Birleşik Editor ve geçiş kabulü | R5b; tests, UI smoke ve migration fixture'ları. | Üç applicationType, tüm üretici/tüketici çiftlerinin capability testi ve her tüketicide gerçek Point/Pose raster yürütmesi; Pattern→Circle→Edge zinciri, save/reopen/no-op, upstream/target-only, U1–U4 regresyonu. Editor teslimi; yeni production izni verilmez. |
| R7a (tamamlandı) — Immutable production ve trusted completion | R6; `ProductionRecipeService`, `ExecutionService`, Runtime snapshot/validator. | Yeni binding/port/ofset/engine kimliklerinin snapshot'a alınması; aynı doğrulanmış resolver ile completion evidence, tamper/source/result/ofset değişimi retleri; aktif cycle taslak editinden etkilenmez. Whitelist kabul tamamlanmadan açılmaz. |
| R7b (tamamlandı) — Kayıt, restart, Logs/CSV | R7a; production/result codec/store/Runtime ve Logs. | Gerçek sonuç ile kullanılan referans/offset/config kimliği ilişkilidir; reopen/replay/CSV aynı veriyi korur; invalid/Stop eski sonuç bırakmaz, eski terminal kayıtları okunur. Socket şeması gerekmedikçe değişmez; PLC restart recovery P7 yerine geçmez. |
| R8 (tamamlandı) — Release, belgeler ve devir | R7b; README/progress/mimari/gereksinim/backlog. | OFF/core ve ON/UI Release tam CTest; normal 1400×900/küçük 850×600 Referans görselleri incelenmiş; yeni paket kanıtları ve kalan T8.4.2b.3c corpus/production kapıları açıkça kaydedilmiş. |

R7a/b, T8.4.2b.3c'nin ortak referans için gereken production işlerini gerçekleştirir. Sonraki T8.4.2b.3c kabulü bunları yeniden uygulamaz; aynı son binary üzerinde eksik corpus (rotated/occluded/symmetric/wrong-candidate), bütün production yolları ve izlenebilir kanıtı birleştirir. Bu kapanış olmadan T8.4 veya production tracking tamamlandı denmez.

Her alt pakette progress kaydı: kapsam, tam alt ID, dosyalar, gerçek build/test komutları ve sonuçları, kalan sınırlama, sonraki ID. Yeni kabul testleri davranışı bağımsız raster/oracle ile doğrular; implementasyon formülünü kopyalamaz. UI değiştiği R5/R6/R7b aşamalarında Release + ilgili CTest + görsel kontrol birlikte gerekir.

Mevcut ilgili test adları: `coordinates_contract`, `roi_geometry_contract`, `tool_migration_contract`, `tracking_tools_contract`, `tracking_dag_contract`, `tracking_inspection_contract`, `world_tracking_contract`, `world_tracking_dag_contract`, `editor_target_only`, `editor_services`, `ui_pose_tracking_smoke`, `ui_editor_usability`, `roi_viewer_interaction`, `production_recipe_contract`, `runtime_persistence`, `typed_terminal_trace`. Her paket ilgili alt kümeyi seçer; R8 tam paketleri çalıştırır. Yeni ortak referans testleri uygulamada CMake'e eklenir; mevcut test sayıları hedef sayı diye sabitlenmez.

README toolchain/DLL PATH kullanılacak; örneğin yeni `build/reference-core` ve `build/reference-ui` altında Release build/test/PNG/INI/log tutulacak. Mevcut dependency/cache kullanılacak; bu plan yeni kurulum gerektirmez. Çalışan uygulama ve `config/local.ui.ini` korunacak. K4/P7/6D.2/6E/lensT15, native DPI/çoklu monitör ve fiziksel hassasiyet/latency/CUDA/dağıtım kabulü ayrı açık kalacak.

## 7. Sonraki uygulama oturumu için başlangıç talimatı — T8.4.2b.3c

```text
VisionFlow T8.4.2b.3c kalan corpus/production kabulüyle devam et.
Önce AGENTS.md, docs/progress.md, README.md, bu plan ve
reference-release-acceptance.md / reference-production-acceptance.md oku.
R1–R8 ve U1–U4 tamamlandı; uygulamayı yeniden başlatma.
R7 immutable snapshot/trusted completion/Runtime/Logs/restart/CSV kanıtını koru.
Eksik rotated/occluded/symmetric/wrong-candidate corpusunu bağımsız raster/oracle
ile tamamla; bütün production yollarının kabul eşlemesini çıkar.
Standalone matcher ve eski poseTracking kapılarını kabul kanıtı olmadan açma.
Non-affine H production kapsamını ayrıca doğrula; mevcut programatik H testi
production kabulünün yerine geçmez. Gerçek sonuç olmadan PASS/skor/pose üretme.
README toolchain ile Release ve ilgili CTest/görsel kontrolleri yap;
progress'e gerçek kapsam, dosyalar, kanıtlar ve sonraki alt adımı kaydet.
Dirty/untracked dosyaları, açık uygulamayı ve config/local.ui.ini'yi koru.
K4/P7/6D.2/6E/lensT15, native DPI ve fiziksel/dağıtım kabulü ayrı açık kalır.
```

R6 [Editor kabul eşlemesi](reference-editor-acceptance.md), R7 [production sözleşmesi](reference-production-acceptance.md), R8 [nihai paket kabulü](reference-release-acceptance.md); tam koşumlar [ilerleme kaydındadır](../progress.md).
