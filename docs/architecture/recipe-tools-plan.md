# Reçete tool geliştirme planı

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](reference-production-acceptance.md). [R8 paket ve devir kanıtı](reference-release-acceptance.md).


11 Ekim son kullanıcı kararı: **[Ortak Referans](reference-system-plan.md)** altında nokta/pose kaynağı, X/Y/açı ofsetleri ve yedi tüketici güncellemesinde **R1–R6 tamamlandı**. R1–R8 tamamlandı; güncel sıra **T8.4.2b.3c** eksik corpus/production kabulüdür. Point yalnız konum, gerçek Pattern/Shape pose konum+açı sağlar; Template Match'e açı uydurulmaz. Alt paketler ve GPT-6.1-sol / medium başlangıç talimatı yeni planın 6–7. bölümlerindedir. Alttaki tarihsel teslimler korunur.

10 Ekim güncel kapsam: **T8.4.2b.3a — Shape/Texture teaching ve ortak pose referansı editörü.** Toolbox artık16 araçtır; Pattern Pose / Shape Pose gerçek Gray8 model, explicit model-local anchor, açı/ölçek ve yöntem skor ayarlarıyla öğretilir. Edge Point/Caliper/Line/Circle/Code/Blob için ortak picker, güncel tam producer cache ve bağlı image ile ROI ilişkisini worker üzerinde öğretir; binding ve poseReference tek doğrulanmış edit olarak kaydedilir. Raw çözülen ROI salt okunur gösterilir; unavailable referansta nominal fallback yoktur. Mevcut nominal mm Circle öğretimi ve kayıtlı world envelope yeniden öğretimi aynı servistedir. Yeni Edge/Caliper/Line nominal mm ROI yazma ekranı, world metric/tolerans/current-world arc ve trusted production Runtime/Logs/CSV kabulü sonraki b.3b/b.3c kapsamındadır; **T8.4.2b.3/T8.4 açık**, production pose reddi sürer. Engine execution/18, registry16, editor7 ve terminal3 değişmedi. Güncel binary: `build/tracking-ui/visionflow.exe`. [Kullanım ve sınırlar](pose-roi.md).

Alttaki teslim/sıra kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.2b — Kalibre tracking teaching/recipe/DAG tamamlandı (feature kapsamı).** Ayrı `poseTracking` schema2 / `world-raw-rigid-v1`: nominal mm ROI SHA256, gerçek taught raw Pose2D, producer/model/anchor, kaynak/çözünürlük, exact calibration dependency ve explicit mm sampling unit. World geometri raw local ROI/AABB içine yazılmaz; eski raw envelope/hash korunur. Engine **execution/18**, validated resolver ve immutable mapping snapshot ile Edge Point/Caliper/Line/Circle/negative Arc örneklerini yürütür. Save/reopen/no-op/yeniden öğretme, iki gerçek matcher × üç applicationType, TargetOnly/acquisition0, cache/model/calibration değişimi ve Stop: **722 DAG kontrolü**; non-affine teaching/executor ve önceki sampler korumaları: **2178 world kontrolü**. Edge actual raw point, Caliper raw endpoint/px ve mm mesafe, Line raw observations/current mm fit, Circle/Arc current mm circle refit üretir. Synthetic strength, raw line/circle fit, taşınmış nominal arc açısı, defect/tolerans/accepted/PASS bu yolda explicit invalid kalır. Kullanıcı teaching UI ve production trusted completion/Runtime/Logs/CSV kabulü **T8.4.2b.3** kapsamında açıktır; T8.4 tamamlanmadı. UI/config değişmedi; production pose whitelist reddi korunur. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki teslim/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel durum: **T8.4.2b.2a — Programatik world sampler/refit tamamlandı; T8.4.2b.2/T8.4 açık.** PoseRoiService::worldMap validated/current exact calibration ve aynı owned image snapshot ile nominal world → taught raw → current raw → current world zincirini kurar. WorldTrackingService Edge Point/Caliper/Line/Circle/Arc için her gerçek longitudinal/transverse tap üzerinde alan kontrolü, mm nokta/mesafe ve taşınmış gözlemlerden line/circle refit sunar. Bağımsız perspektif raster, gerçek iki matcher → uniform/Y-up world edge ve hata/iptal testleri **2134 kontrol** geçti. Bu programatik servis teslimidir; kalibre tracking teaching envelope/recipe/DAG/TargetOnly bağlantısı sıradaki **T8.4.2b.2b** işidir. UI/Runtime/Logs/CSV tracking kabulü b.3 kapsamında açık; production pose whitelist reddi ve execution/17 sürer. Standalone ToolRoi schema2 world scan/rectangle taşır; recipe Edge Point/Caliper/Line config bu world geometrileri henüz reddeder. UI ve yerel config değişmedi. [Sözleşme ve sınırlar](pose-roi.md).

10 Ekim T8.4.2b.1 tarihsel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim T8.4.1 tarihsel durum: **T8.4.1 — Shape/texture ROI referans sözleşmesi tamamlandı; T8.4 devam ediyor.** Ortak PoseRoiService artık pattern_pose ve shape_pose kabul eder; kaynak kind için doğru method/version zorunludur. Model/ROI/anchor veya matcher türü değişirse teaching geçersizdir. Gerçek iki matcher ile altı geometri, dönmüş scan → gerçek EdgeFinder, shape kontrast tersleme, gerçek tekrarlı/not-found adaylar ve perspektif/lens korumaları: **398 tracking kontrolü**. Programatik servis kabulüdür; Code/Edge/Caliper/Line/Circle/Blob reference picker, recipe/DAG binding, world sampler ve production Runtime bağlantıları sıradaki **T8.4.2** kapsamındadır. Toolbox14/registry16, execution/15 ve Runtime whitelist reddi sürer; UI değişmedi.

10 Ekim T8.3 tarihsel durum: **T8.3 — Shape Match çekirdeği tamamlandı**. `shape_pose` v1 gradient pyramid adayları ve unsigned orientation-aware chamfer refinement ile gerçek raw Pose2D/ayrı scale üretir. Texture NCC ile skor karşılaştırılmaz; yöntem `unsigned-oriented-chamfer-pyramid` v1. Ambiguity/truncation seçimi varsayılan reddeder; genel karar NOT_EVALUATED. Programatik teaching/DAG/TargetOnly/codec teslimi; Toolbox14/registry16, engine execution/15. Özel UI, shape ROI reference ve production Runtime/completion T8.4 kapsamında, Runtime whitelist reddi sürer. Sonraki **T8.4 — Bütün araçlara bağlama**. [Sözleşme ve sınırlar](shape-pose.md).

10 Ekim T8.1 tarihsel durum: **T8.1 — Gerçek Pose2D çekirdeği tamamlandı**. `pattern_pose` v1 programatik teaching/DAG: çok seviyeli açı/ölçek arama, anchor/refinement/NMS, ambiguity ve reject_ambiguous/best politikası. Pose2D gerçek raw açı/anchor taşır; scale ayrı porttur. PartDecision NOT_EVALUATED. Toolbox14 korunur (registry15); yeni özel UI/tracking/production Runtime henüz yok, Runtime açıkça reddeder. Engine execution/14, editor7/terminal3/vf.socket/2 korunur. Sonraki **T8.2 — Pose referanslı ROI**. [Sözleşme ve sınırlar](pattern-pose.md).

10 Ekim T7.2 tarihsel kayıt: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](geometry-measurement-adapter.md).


10 Ekim T7.1 tarihsel kayıt: **T7.1 — Geometrik domain matematiği tamamlandı**. Mevcut framed Point2D/Line2D/Circle2D üzerinde mesafe, signed/absolute X/Y ve dik offset, segment uzaklığı, üç açı semantiği, sonsuz çizgi kesişimi/paralel aralık ve radius/diameter seçimi. Domain çekirdeğidir; yeni reçete aracı/UI/Runtime bağlantısı yok. Engine execution/11 ve mevcut UI build/circle-ui/visionflow.exe korunur. Sıradaki **T7.2 — Kalibre adapter**. [Sözleşme ve sınırlar](geometry-measurements.md).


9 Ekim 2026. Kullanıcı kapsamı onayladı; bu belge **uygulama planıdır, çalışan özellik listesi değildir**. Kod geliştirme küçük, sırayla kabul edilen alt paketler halinde ilerler. T0.1–T0.3, T1.1–T1.3 T2.1–T2.3 ve T3.1–T3.3, T4.1–T4.3 ve T5.1–T5.3, T6.1–T6.3 tamamlandı; [uygulanan sözleşme](tool-values.md). Preprocess kullanıcı aracıdır; Code Read kullanıcı aracı; format/içerik/ROI, gerçek polygon/list ve Runtime/Logs kabulü tamamlandı. Edge Point ok/Properties/profil/aday ve mevcut Measurement mm/Runtime kullanıcı akışı T4.3 ile kabul edildi. Lens bozulması düzeltmesi T15 ile en sona bırakıldı.

## 1. Başlangıç ve kapsam

C1–C5, K1–K3 ve P1–P6 tekrar yapılmaz. T0 öncesi yedi tool: Image File, Webcam, Template Match, Blob, Edge, Calculate, Measurement. Edge dört yönde ilk eşik geçen piksel merkezini döndürür; Template Match sabit açı/ölçeklidir. Measurement iki noktadan mm ve tolerans sonucu üretir. Manuel/bilinen uzunluk ölçeği ve raw homography mevcuttur. Genel reçete kararı, yeni araçlar, lens düzeltmesi ve fiziksel doğruluk kabulü mevcut değildir.

Onaylı genişleme: kod okuma; serbest yönlü Edge Point; Edge Pair/Caliper; Line; Circle/Arc; kalibre geometri; açıyla parça referansı; Blob/Presence; ön işleme; ortak kabul kuralları; OCR/OCV; renk; kontur/yüzey kusuru; bead inspection. Kütüphane ve algoritma kararları [teknik planda](recipe-tools-algorithms.md), test/teslim koşulları [kabul planında](recipe-tools-acceptance.md).

Sıradaki uygulama adımı **T8.4.2b.3c — Kalan corpus ve bütün production yollarının kabulü**; R1–R8 tamamlandı, R7 trusted Runtime/Logs/CSV uygulaması tekrarlanmaz. T8.4.2b.3b nominal world ROI authoring ve mm metric/tolerans/current-world arc tamamlandı; engine execution/19. T8.4.2b.3a ortak teaching/reference picker editörü uygulandı. T8.4.2b.2b feature teaching/recipe/DAG bağlantısı tamamlandı. T8.4.2b.2a programatik world sampler/refit teslim edildi. T8.4.2a raw metrologi DAG/cache ve T8.4.2b.1 Code/Blob raw maske bağlantısı tamamlandı. Tarihsel bağımlılık sırası: T8.1 pose, T8.2 ortak ROI adapter/codec, T8.3 shape_pose ve T8.4.1 shape/texture referans sözleşmesi tamamlandı. T8.4 bütün olarak açık; yeni referenceBinding Runtime kabulü R7 ile mevcut, standalone matcher/eski poseTracking ve eksik corpus kapıları korunur. [Pose ROI](pose-roi.md). K4/P7/6D.2/lensT15 ayrı açık işlerdir.

## 2. Sol 6.1 medium çalışma sözleşmesi

- Tek alt pakette tek ana davranış değişikliği; matematik, kayıt, UI ve birleşik kabul alt paketleri ayrı tamamlanır. Her paketin belirtilen önkoşulları tamamlanmadan ona geçilmez.
- Aşağıdaki yeni dosya adları **önerilen hedeflerdir**. Var olan aynı sorumluluğu taşıyan sınıf varsa onu genişlet; ikinci engine/editor/calibration sistemi kurma. Büyük toplu refactor yapma.
- Yeni tool çekirdeği `src/vision/` içinde, orchestration/application adapter `src/application/` içinde; parametre/tipler `src/domain/`, etkileşim `src/app/`. UI OpenCV, ZXing veya Tesseract çağırmaz.
- Mevcut dirty/untracked işleri koru. Her alt pakette progress'e tam alt ID, değişen dosyalar, gerçek komut/sonuç, sınırlama ve sıradaki alt ID yaz. Test başarısızsa paketi tamamlandı işaretleme.
- Test/PNG/INI/raporlar `build/` altında; `config/local.ui.ini` ve kullanıcının açık uygulaması korunur. Yeni build dizini kullan; çalışan uygulamayı kapatıp eski binary üzerine yazma.
- Uygulama talimatı yalnız T4.1 diyorsa kalan T4 altlarını kendiliğinden tamamlanmış sayma. Kullanıcı bir paket dizisini yetkilendirdiyse her alt pakette tekrar izin istemeden o diziye devam et. Gerçek eksik veri gerektiren kabulü açık bırakıp bağımsız işleri sürdür.
- Yeni model/SDK kurulumunu veya fiziksel testin başarısını varsayma. Bu plandaki sürüm sabitleme ve yerel ABI denemeleri uygulama görevleridir; henüz yapılmadı.

## 3. Ortak ürün ve veri kararları

### 3.1 Birimler, kalibrasyon ve geçerlilik

Yeni ölçüm araçları reçetenin aynı immutable kalibrasyon bağını kullanır. Kalibrasyon seçildiğinde mm doğrudan tool sonucunda görünür; kullanıcı katsayıyı araç başına kopyalamaz. Registry'de `usesCalibration` benzeri bir capability ile resolver bütün metrik tool'lar için çalışır; yalnız `kind == measurement` kontrolü yeterli değildir.

Yeni metrologi araçlarında `pixel_debug` ve `calibrated` çalışma seçimi bulunur. Port isim/tip/birimleri sabittir: örneğin `diameterPx` ve `diameterMm` farklı portlardır, aynı Number portunun birimi çalışma sırasında değişmez. Pixel debug mm/üretim kararı üretmez. Calibrated modda geçersiz binding/frame/current/alan ölçüm ERROR verir; px'ye sessiz fallback veya eski mm değeri yoktur. Ham tanı overlay'i varsa geçerli üretim çıktısı gibi işaretlenmez. Önizleme ile üretim sonucu açıkça ayrıdır.

Uzunluk mm, alan mm², açı derece, benzerlik ratio; dünyadaki düzlem kimliği ve eksenler ayrıca taşınır. Perspektifte tek ölçek çarpımı yerine noktalar ölçüm düzlemine çevrilir; fit orada yapılır. Geometri yalnız kalibre edilmiş destek alanında kullanılır. T15'e kadar `raw` koordinatı lens düzeltilmiş diye etiketlenmez. Yükseklik farkları için planar model 3D doğruluk iddia etmez.

### 3.2 Typed sonuç ve kayıt

Mevcut `PortType` yalnız Image/Region/Point/Number/Boolean, `PortData` ise ImageFrame/QRect/QPointF/double/bool taşır. T1 gerektikçe Text, CodeResults, PointSet/Profile, Line2D, Circle2D/Arc2D, Pose2D ve inspection sonuç koleksiyonlarını ekleyecek. Domain tipleri OpenCV sınıfına bağlı olmayacak. Kaynak/frame/cycle/evaluation, koordinat, birim, yöntem/sürüm, validity ve nedeni korunacak.

Geçerli boş koleksiyon (0 kod, 0 blob) ile invalid koleksiyon ayrıdır. Pozitif bir Boolean `found` sonucu, tool'nun metrologi çıktılarının geçerli olduğu anlamına gelmez. Skor ancak algoritmanın hesapladığı anlamla sunulur; ZXing'den olmayan confidence uydurulmaz.

`ProductionRecipeService` bugün yalnız Measurement için karar doğruluyor ve sınırlı PortData türlerini serialize ediyor. Yeni türler eklenirken completion doğrulama, kalıcı terminal kayıt, Logs, CSV, snapshot dependency ve cache ziyaretçilerinin tamamı aynı pakette genişletilir. Bilinmeyen sonucu sessiz atlayan serializer kabul edilmez. T10'a kadar yeni kontroller Boolean/typed ölçüm çıktıları verir, genel reçete PASS'i vermez; legacy Measurement'ın kendi kararı korunur.

### 3.3 Geriye uyumluluk

Güncel: root schema 1, recipe schema 1–2, legacy editor schema 6 + opt-in 7; uniform kayıt/dependency 1, homography 2, manuel 3; Measurement tool 2, Preprocess/Code Read tool 1, execution/8. T1.2 ile dependency/typed descriptor düzenlemesi schema 7 seçer; [alan sözleşmesi](tool-values.md). Eski aç/no-op Save diski veya recipe sürümünü yükseltmez; ilk gerçek düzenleme gereken şemayı yazar. Yeni alanlar henüz desteklenmiyorsa açık ret; kapalı tool içinde saklanmış bilinmeyen konfigürasyon da sessizce normalize edilmez.

Legacy `edge`, `template_match`, `calculate`, `measurement` davranışları örtülü değişmez. Yeni araçlar yeni kind kullanır; eski Edge'i alt piksele otomatik dönüştürme. Blob genişletmesi yöntem/parametre varsayılanlarıyla eski alan/sayım sonuçlarını korur. Yeni engine/registry kimliği eski validation iznini kullanmaz. Gelecek şema numaraları her gerçek teslimde kesinleştirilir; plan bütün alt paketler aynı schema 7 ile sınırsız alan kabul eder anlamına gelmez. Lens T15 ayrı yeni model/kayıt sürümüdür; eski hash fixture'ları korunur.

### 3.4 Kullanıcı etkileşimi

Toolbox aileleri: Görüntü, Bul/Konumlandır, Ölç, Kimlik/Yazı, İncele, Karar. Aynı editor ve Runtime korunur. Ok/döndürülmüş dikdörtgen/halka/yay/path çizimi, handle ile düzenleme, sayısal giriş, Escape iptal ve tek işlem undo desteklenir. Zoom/pan sonucu koordinat değişmez. Ayar değişikliği yalnız gereken downstream sonuçları invalid eder; Run Tool aynı raw kare/upstream cache kuralını korur.

## 4. Paket dizisi ve alt teslimler

İşaretli kutular kanıtıyla tamamlandı; diğerleri **planlandı** durumundadır. Alt paketler yazılı sıradadır; aksi belirtilmedikçe önceki T paketi önkoşuldur. Fiziksel örnekler beklenirken sentetik/yazılım kabulü ilerler; gerçek veri eksiği ayrıca kaydedilir.

### T0 — Yerel yetenek ve bağımlılık kabulü

- [x] **T0.1 — Baseline ve API probe.** Ayrı `build/tool-baseline-core` Release/Widgets OFF derlemesi ve 23 mevcut test doğrulandı (PLC sandbox dışında localhost tekrarında geçti). `tests/probes` ve `scripts/probe-opencv.ps1` ile QR decode, filtre, remap, fitLine, üç fitEllipse yöntemi, contour ve homography compile/link/run denemeleri geçti. `build/tool-probes/` JSON/build-info/include/import-lib/DLL SHA-256 ve x64 ABI kaydı; [kanıt ve sınırlar](../toolchain.md#t01--opencv-api-ve-baseline-kabulü). Yeni kullanıcı özelliği yok; QR kendi encoder'ıyla sınırlı probe'dur.
- [x] **T0.2 — ZXing bağımlılık spike.** ZXing 2.3.0/commit `d6068bc` kaynak SHA-256 ve lisans/embedded libzueci kaydı `third_party/manifest.json` içinde; static reader-only MinGW GCC 13.1/C++20 Release. Ayrı Zint 2.15.0 encoder'ından sekiz fixture ile QR/Data Matrix/Code128/Code39/EAN-8/13/UPC-A/E: text/raw bytes/format/köşe kontrolleri, 90° dönüş, padded stride, format dışlama ve blank no-read; 33 kontrol/0 hata. Bootstrap açık `-Download`, normal probe configure yerel prefix kullanır; cache'den indirmesiz tekrar geçti. [Kanıt ve komutlar](../toolchain.md#t02--zxing-reader-only-bağımlılık-kabulü). Ürüne backend bağlama/capability T0.3 ile tamamlandı; OpenCV kurulumu değiştirilmedi.
- [x] **T0.3 — Bağımlılık kapısı.** ZXing var/yok configure ve runtime capability davranışı, temiz makine için belgelenmiş build prefix/DLL yolu; Tesseract/Leptonica/tessdata için aynı süreç T11.1'de. Eksik reader varsa yeni code tool etkinleştirilemez; mevcut reçeteler çalışır. Yeni ürün dağıtımı kabul edilmiş sayılmaz.
- Dosyalar: CMakeLists.txt, yeni dependency manifest/bootstrap/probe hedefleri, docs/toolchain.md. Çıkış: doğrulanmış API listesi, sabit bağımlılık kimlikleri ve gerçek test sonuçları. Kütüphane kurulumu başarısızsa neden ve alternatif kayıtlıdır.

### T1 — Tipler, sürümleme ve sonuç altyapısı

- [x] **T1.1 — Tip/registry sözleşmesi.** `ToolGraph`, `Coordinates`, `ExecutionService`, `ToolRegistry`; yeni enum/struct ve bounded collection codec; mode'dan bağımsız sabit birimli portlar, capability, aynı cycle/frame kontrolleri. Veri payload'ını keyfi JSON portuyla geçirme. Tip ekleme ile bütün switch/variant ziyaretçilerini denetle.
- [x] **T1.2 — Recipe migration ve dependency.** `ToolDefinition`, `ProjectRepository`, `ProductionRecipeService`: schema 7 opt-in yazma, eski no-op/hash fixture, model/backend/asset içerik kimlikleri, eksik backend/model ret. Yeni port codec'lerini roundtrip/tamper ile sına.
- [x] **T1.3 — Runtime/Logs/cache altyapısı.** `ProductionRecipeService`, `ProductionStore`, `RuntimeService`, `EditorSession`, mevcut Results/Logs: typed sonuç kayıtları ve limitler, TargetOnly imzası, diagnostic/metric ayrımı. Legacy PLC vf.socket/2 aynı kalır. Genel yeni karar mantığı T10'a aittir.
- Dosya önerileri: `src/domain/ToolValues.hpp/.cpp`, `src/application/ToolResultCodec.hpp/.cpp`. Testler: yeni `tool_values_test`, mevcut tool_execution/project_persistence/production_recipe_contract/runtime_persistence/editor_target_only. Çıkış: yeni tipi taşıyan çekirdek hazır; toolbox'ta çalışanmış gibi boş tool sunulmaz.

### T2 — Owned görüntü erişimi, ROI çizimi ve ön işleme

- [x] **T2.1 — Ortak görüntü ve ROI sözleşmesi.** `ImageFrame` private piksel erişimi için UI'ya açılmayan salt okunur vision sınırı; immutable input, owned output ve processing provenance. Domain'de scan segment, rotated rectangle, annulus/arc ve polygon/path; sıfır boy/geçersiz/bounds davranışı. Pixel centre standardı korunur.
- [x] **T2.2 — Viewer etkileşimi.** Uygulandı: [geçici geometri çalışma alanı ve sınırlar](roi-geometry.md); recipe binding ilgili yeni tool paketinde. `ImageViewer`, `MainWindowEditor`: ok/halka/yay/dönmüş kutu/path çizimi ve handles; sayısal eşdeğer, Escape/undo, zoom/pan, valid-domain overlay. Mevcut rectangle ROI/kalibrasyon nokta seçimini bozma. Çizim girdisi subpixel QPointF kalır.
- [x] **T2.3 — Preprocess tool.** Gray/kanal seçimi, Gaussian/median, threshold/adaptive threshold, morphology; seçilebilir sıralı filtre parametreleri ve çıktı görüntüsü. İlk sürüm full-size/geometri korur. Crop/resize/lens düzeltmesi genel filtreye gizlenmez; sampling yalnız tool içinde yapılır ve koordinatlar geri eşlenir. Masked işlemin border/hole politikası açık olur.
- Dosya önerileri: `src/vision/ImageAccess.hpp`, `src/vision/Preprocessing.*`, `src/domain/ToolRoi.*`, UI drawing helper. Testler: `preprocessing_test`, `roi_geometry_test`, owned_frame_contract, editor_target_only ve UI. Çıkış: raw input değişmeden gerçek işlenmiş Image; downstream cache ve kalibrasyon bağı güvenli.

### T3 — Kod Oku (QR/Data Matrix/1D)

- [x] **T3.1 — Reader çekirdeği.** `src/vision/codes/CodeReader.*`, ZXing adapter. ROI, format allow-list, maxCount, deneme bütçesi; text/raw bytes/symbology/corners ve gerçek read state. Aynı metinli iki fiziksel kod tek sonuç yapılmaz. Görüntü dönüşümü varsa corners raw'a geri eşlenir.
- [x] **T3.2 — İçerik kontrolü ve kayıt.** `CodeReadService`, tool registry/executor: exact/prefix/length/regex, beklenen adet, deterministik seçim; çoklu listede açık any/all/seçili kod. Unicode/GS1 separator korunur; GS1 alan ayrıştırma ayrı sürümlü opsiyon, sessiz normalizasyon yok. No-read ile backend/config hatası ayrı.
- [x] **T3.3 — UI ve uçtan uca kabul.** Format seçimi, bounding polygon, sonuç listesi, içerik kontrolü; save/reopen, immutable Runtime/Logs, TargetOnly ve Stop. Sayısal confidence sunma. Kodla reçete değiştirme/URL açma/komut yürütme yok.
- Testler: `code_reader_test`, `code_tool_test`, production/runtime regresyonu, UI kod okuma senaryosu. Çıkış: tüm altı semboloji ailesi için gerçek fixture kanıtı; hasarlı/DPM baskı kapsamı ayrıca belirtilir. Okuma ISO baskı kalite derecesi değildir.

### T4 — Okla tek kenar ve ortak 1D profil çekirdeği

- [x] **T4.1 — Sampler/profil.** `src/vision/metrology/ProfileSampler.*`: keyfi açılı şerit, bilinear sampling, geçerli footprint, genişlik boyunca ortalama, smoothing ve signed derivative. Float/double hesap; sınır dolgusu sahte kenar üretmez. Parametre sınırları/bellek/iptal noktaları.
- [x] **T4.2 — Edge finder.** `EdgeFinder.*`: rising/falling/any tarama yönüne göre; first/last/strongest; plateau/tie kararı; subpixel peak refinement ve min contrast. `edge_point` yeni kind; legacy Edge aynen kalır. Profilde sampling step/width/strength tanımı kaydedilir.
- [x] **T4.3 — Editor/Runtime.** Ok çizimi ve Properties, profil grafiği, seçilen/reddedilen adaylar ve nokta; mevcut Measurement'a point bağlama; save/reopen, cache, stop. Bulunamadı başarılı arama sonucudur (`found=false`), nokta invaliddir; downstream ölçüm sayı üretmez.
- Testler: `profile_sampler_test`, `edge_point_test`, measurement_dag ve UI. Çıkış: yatay/dikey/eğik/ters/blur/noise fixture'ları ve subpixel analitik hata raporu; fiziksel hassasiyet iddiası yok.

### T5 — Edge Pair/Caliper ve Line

- [x] **T5.1 — Edge Pair.** Ortak profilden opposite-polarity çiftleri; açık/koyu stripe, ilk/son/en güçlü çift, beklenen genişlik aralığı. Eşleştirme tüm kontrast/destek kontrolünü geçen adayların komşuluğundan deterministik yapılır; kabul edilmiş ara kenar atlanmaz, rejected nicemleme tepeleri tanıda kalır; iki bağımsız strongest edge gelişigüzel birleştirilmez. Çıktı uçlar, orta nokta, px genişlik; calibrated modda iki uçtan gerçek mm.
- [x] **T5.2 — Line fit.** Döndürülmüş kutu boyunca çoklu scan; geçerli kenarlardan robust `fitLine`, gerektiğinde deterministik RANSAC ve yeniden fit. Sonsuz çizgi ile ölçülen destek segmenti ayrı; angle, residual RMS/max, kullanılan/elenen nokta ve kapsama.
- [x] **T5.3 — UI/kalibrasyon/kabul.** `MetrologyService`, registry/executors, caliper genişlik overlay'i ve line support; piksel/world fit ayrımı, tolerans Boolean'ları ve kayıt. Fiziksel geometrik açı düzlemde hesaplanır. Genel karar T10.
- Dosya önerileri: `EdgePairFinder.*`, `LineFitter.*`, `application/MetrologyService.*`. Testler: `edge_pair_test`, `line_fit_test`, `metrology_runtime_test`, UI. Çıkış: gap/stripe/çizgi gerçek ölçüleri; yanlış çift/yetersiz kapsam sonucu geçersiz.

### T6 — Circle/Arc ve dairesellik

- [x] **T6.1 — Radyal tarama.** İç/dış yarıçap ve start/sweep ile halka/yay; dıştan içe/içten dışa, edge polarity, eşit açılı scan ve angular gap takibi. Perspective modda ölçüm düzleminde tanımlanan nominal halka raw'a inverse map edilir; görüntüde zorunlu daire ROI varsayılmaz.
- [x] **T6.2 — Circle solver.** Normalize noktalarla algebraic başlangıç + geometric least-squares refinement, robust merkez/yarıçap fit'i; degenerate/narrow arc/condition ret. Bulunan contour örnekleri mm'ye çevrildikten sonra metrik fit. HoughCircles ve minEnclosingCircle nihai ölçüm hesabı değildir.
- [x] **T6.3 — Kalite/metrikler.** Diameter/radius/centre; fit RMS; tüm geçerli kontur örneklerinden radial min/max, signed residual ve peak-to-valley; fit inlier ve kusur noktaları ayrı. Tam çevre dairesellik iddiası için min coverage/max gap kapısı; arc'ta yalnız görülen yayın sapması. Kapalı kontur varsa açık formüllü circularity index; yoksa unavailable. Yöntemler algoritma planında sabittir.
- [x] **T6.4 — UI ve kalibre Runtime.** Doğrudan mm sonucu, bağlı calibration/version, kapsam, ayrı çap/sapma limitleri; halka/yay handles, elenen noktalar ve raw'a geri çizilen world fit. Tamper/save/reopen/cache/Stop/Logs/CSV kabulü. Ekrandaki yuvarlama kararı değiştirmez.
- Dosya önerileri: `RadialSampler.*`, `CircleFitter.*`, `CircleMetrics.*`, `MetrologyService` genişlemesi. Testler: `circle_fit_test`, `circle_metrics_test`, `calibrated_circle_test`, UI. Çıkış: yalnız çap değil yöntem/birim/kapsamı belirli sapma; MZ/ISO/GD&T uygunluğu iddia edilmez.

### T7 — Geometrik ölçümler

- [x] **T7.1 — Domain matematiği.** Nokta–nokta, X/Y offset, nokta–çizgi dik uzaklığı, iki çizgi açısı/kesişimi, paralel çizgi aralığı, daire merkezleri mesafesi ve yarıçap/çap seçimi. İşaretli/mutlak, çizgi/segment ayrımı ve paralel/dejenere davranışı sabitlenir.
- [x] **T7.2 — Kalibre adapter.** `geometry_measurement` sabit typed feature girdileri/operation-specific registry doğrulama; hesaplar aynı world frame'de. Homography'de piksel fit parametresi sadece dönüştürülmez, taşınan kaynak örneklerden world fit yeniden üretilir. Yanlış frame/birim/cycle reddi.
- [x] **T7.3 — UI/tolerans/kayıt.** Tipli feature seçici, yardımcı çizgi, nominal/alt/üst limit, signed offset için negatif aralık; boş/yanlış bağlantı açıklaması. Legacy Calculate px ve Measurement sözleşmesi korunur.
- Dosya önerileri: `domain/GeometryMeasurements.*`, `GeometryMeasurementService.*`. Testler: `geometry_measurements_test`, runtime ve UI. Çıkış: iki delik arası, kenara mesafe ve açı reçeteleri; geometrik formül semantiği ve birimler görünür.

### T8 — Parça referansı, dönme takibi ve Shape Match

- [x] **T8.1 — Gerçek Pose2D.** Yeni `pattern_pose` kind: sınırlı angle/scale aramalı çok seviyeli template matching + aday refinement/NMS. Model teaching anchor'ı korunur; score yöntem adıyla gelir. Simetri/benzer adaylar için ambiguity sonucu, seçili pose politikası. Ölçek pose rotation'dan ayrı taşınır.
- [x] **T8.2 — Pose referanslı ROI.** Mevcut Coordinates/TrackingRoi geometrisini kullan; yeni tool'ların local ok/halka/polygonlarını pose ile raw koordinata taşı. Dönmüş ROI'yi AABB'ye çevirip ölçme. Geçersiz referansta absolute fallback yok. Homography altında mm ölçüm ROI'si ve görüntü pose'u arasında açık dönüşüm zinciri.
- [x] **T8.3 — Shape Match.** `shape_pose`: edge/gradient template, pyramid candidate search ve orientation-aware chamfer refinement. GeneralizedHoughGuil karşılaştırma/prototip seçeneği; `matchShapes` tek başına pose detector değildir. HALCON ile eşdeğerlik iddiası yok. Texture/shape yöntemleri ayrı skor/kimlik taşır.
- [ ] **T8.4 — Bütün araçlara bağlama.** Code/Edge/Caliper/Line/Circle/Blob ROI reference picker ve teaching invalidation; taşınmış/dönmüş örnek, örtülme, simetri ve yanlış aday testleri. Kalibrasyon çalışma alanı ve lens değişimi korumaları sürer.
- [x] **T8.4.1 — Ortak shape/texture referans sözleşmesi.** PoseRoiService iki matcher kind'ını kendi method/version kimliğiyle kabul eder; kind/model/ROI/anchor invalidation, gerçek iki matcher/altı geometri/scan→EdgeFinder ve world/lens korumaları: 398 kontrol. Mevcut binding schema1/hash korunur.
- [ ] **T8.4.2 — Araç/DAG/teaching UI/world sampler/Runtime bağlantıları.** T8.4'ün yukarıdaki kullanıcı akışı ve uçtan uca kabul kapsamını tamamla; T8.4.1 programatik servis kabulünü ürün özelliği diye sunma.
- [x] **T8.4.2a — Raw metrologi DAG/cache.** Edge Point/Caliper/Line/Circle raw ROI; explicit recipe teaching ve poseReference; iki matcher/üç applicationType/TargetOnly/persistence negatif kabulü. UI ve Runtime değildir.
- [ ] **T8.4.2b — Kalan ürün bağlantıları.** Code/Blob raw b.1, world sampler/feature recipe/DAG b.2 ve ortak raw teaching UI b.3a tamamlandı. Yeni nominal world authoring, world metric/tolerans/current arc ve production trusted completion/immutable Runtime/Logs/CSV açık.
- [x] **T8.4.2b.1 — Raw Code/Blob.** Dönen pixel-centre maskesi; iki matcher/üç applicationType gerçek QR ve Blob, persistence/TargetOnly/cache retleri. UI/production tracking değildir.
- [x] **T8.4.2b.2 — World sampler/refit (feature kapsamı).** Nominal world ROI için her gerçek sampling tap boyunca exact calibration dependency ve hull/lens/resolution korumaları; taşınmış gözlemlerden world feature refit. Programatik servis a, kalibre feature teaching/recipe/DAG/TargetOnly b adımıyla tamamlandı; UI/production ve metric/tolerans bağlantıları b.3 kapsamında açık.
- [x] **T8.4.2b.2a — Programatik world sampler/refit.** Immutable mapping snapshot, bütün longitudinal/transverse tap kapıları; mm edge/pair ve current world line/circle/arc gözlem refit. 2134 fixture kontrolü; kullanıcı/üretim özelliği değildir.
- [x] **T8.4.2b.2b — Kalibre tracking teaching/recipe/DAG (feature kapsamı).** Nominal world ROI ve calibration dependency teaching sözleşmesi; exact envelope/identity/no-op/save/reopen, resolver ve executor context, kalibre TargetOnly/cache invalidation; üç applicationType kabulü. Mevcut raw binding içine world geometri veya raw AABB yazma.
- [ ] **T8.4.2b.3 — Ortak teaching UI ve production kabulü.** Reference picker, shape/texture teaching/invalidation; trusted completion, immutable Runtime, Logs/CSV, world metric/tolerans ve geçerli current-world arc semantiği; rotated/occluded/symmetric/wrong candidate uçtan uca kabulü.
- [x] **T8.4.2b.3a — Ortak raw reference picker ve Shape/Texture teaching UI.** Gerçek cached pose/image, atomic recipe edit, async teaching/Stop/stale suppression, resolved read-only ROI, altı raw tüketici, mevcut world Circle ve kayıtlı world reteaching.
- [x] **T8.4.2b.3b — World ROI authoring ve metric semantiği.** Edge/Caliper/Line/Circle nominal mm yazma/yeniden öğretme UI, dahil tool toleransları ve gerçek current-world arc/gap/radyal sapma/quality; genel karar NOT_EVALUATED, production reddi sürer.
- [ ] **Ortak Referans R1–R8 — Yeni kullanıcı önceliği.** Point/Pose kaynakları, X/Y/açı ofsetleri, yedi tüketici, migration/cache/world/UI ve production bağlantısı; [alt paketler ve kabul](reference-system-plan.md). Tamamlanan T8 alt işleri yeniden yapılmaz.
- [ ] **T8.4.2b.3c — Trusted production kabulü.** R1–R8 sonrasında; R7'nin immutable Runtime, exact trusted completion/replay ve Logs/restart/CSV kanıtını birleştirir; eksik rotated/occluded/symmetric/wrong-candidate corpus ve bütün production yollarının nihai kabulünü tamamlar. R7 uygulaması tekrarlanmaz.
- Dosyalar: mevcut `src/vision/matching/TemplateMatcher.*`, `MatchService`, yeni PoseMatcher/ShapeMatcher, `Coordinates`, `EditorSession`, Viewer. Testler: `pose_match_test`, `tracking_tools_test`, UI. Çıkış: dönen parçayı izleyen gerçek ROI; sabit legacy Match'ten sahte angle=0 pose üretimi yok.

### T9 — Blob, Presence, Count ve şekil analizi

- [ ] **T9.1 — Bölge özellikleri.** Var olan connected-component maskesinden her kabul edilen bileşenin alanı, çevresi, centroid, oriented box, aspect ratio, solidity, hole count ve circularity; border-touch politikası. Legacy pixel-count area ile contour area farklı alan adları.
- [ ] **T9.2 — Kalibre alan/şekil.** Uniform alan scale²; homography için piksel hücreleri/sınırların world alan integrasyonu, tek scale² yok. Shape filters/count/exact presence karar girdileri; 0 nesne geçerli sonuç, object geometry invalid. Delik ve border kesilmesi metriklerini koru.
- [ ] **T9.3 — UI/kayıt/regresyon.** Nesne listesi/ID seçimi, mevcut kırmızı maske, ölçüt/sınır ve neden; yeni filtreler varsayılan kapalı, eski sayım/alan/maske değişmez. T8 tracking bağını kullan.
- Dosyalar: Blob executor'ı gerekirse `src/vision/inspection/BlobAnalyzer.*` içine ayrılır; registry/UI/codec. Testler: blob_contract + `blob_features_test`, `calibrated_area_test`, UI. Çıkış: eksik vida/delik sayımı ve şekil filtreleri, gerçek maske.

### T10 — Ortak kabul kuralları, Runtime ve sonuç teslimi

- [ ] **T10.1 — Decision service.** Typed numeric inclusive range/equality/count/text checks; all/any/not bileşimi. Her kuralın mandatory input kümesi ve geçerlilik ön denetimi; eksik/stale/error girdiler Boolean false diye yutulmaz. Rule listesi/reason tree ve ham değerler kayıtlı.
- [ ] **T10.2 — Güvenilir completion/kayıt.** Aktif immutable recipe kurallarıyla kararı servis yeniden değerlendirir; UI'dan gelen PASS kabul edilmez. `ProductionRecipeService` Measurement'a özel kontrolü registry capability + explicit result validator ile genişletir. Terminal cycle commit öncesi yeni karar görünmez; retry yeni acquisition yapmaz. Eski archive codec'i korunur/yeni record version açıkça belirlenir.
- [ ] **T10.3 — Editor/Runtime UI.** Tek final Result tool; tek/çoklu ölçüm sonuç tablosu, fail nedenleri ve NOT_EVALUATED görünümü. Boş rule/disabled zorunlu test PASS değildir. Save/reopen/load/run/stop/partial ve store-lock kabulü.
- [ ] **T10.4 — PLC mapping sınırı.** vf.socket/2 legacy Measurement alanları korunur. Çoklu sonuç/text/final decision için ayrı versioned vf.socket/3 tasarımı + simulator/contract test; endpoint'te explicit sürüm seçimi, sessiz protokol değişimi yok. P7 restart kurtarma yine ayrı açık iştir; yeni wire onun tamamlandığı anlamına gelmez.
- Testler: `decision_rules_test`, production_recipe_contract/runtime_persistence/runtime_coordinator/plc_socket_runtime, yeni v3 simulator ve UI. Çıkış: QR içeriği + çap + adet ile gerçek final karar ve kalıcı nedenler.

### T11 — OCR ve OCV

- [ ] **T11.1 — Backend/asset.** Tesseract C++ + Leptonica ve tessdata sürüm/hash/lisans/MinGW build kabulü; model yokken explicit unavailable. Kullanım için uygulama içi sabit asset referansı; makine PATH'inden rastgele model alma yok.
- [ ] **T11.2 — OCR.** ROI'den text, kelime/karakter box ve backend confidence; orientation/reference, preprocessing, charset/PSM/dil konfigürasyonu. Ham metin ve uygulanmış normalizasyon ayrı. Model confidence olasılık veya baskı kalite skoru değildir.
- [ ] **T11.3 — OCV.** Beklenen metin/format ile OCR sonucunu karşılaştır; kayıp/fazla/yanlış karakter ve güven eşiği. Bu sürüm semantik yazı doğrulamasıdır; print-defect/karakter şekli kontrolü gerekiyorsa T13 reference inspection'a açık bağlanır. Tarih/lot formatı elle tanımlı kurallarla sınanır.
- [ ] **T11.4 — UI/kabul.** Öğretim/test/resim sonuçları, per-character ayrıntı varsa backend kaynaklı; model hash değişince snapshot invalid. Runtime Result/Logs/CSV ve eksik model testleri.
- Testler: `ocr_backend_test`, `ocv_rules_test`, UI ve gerçek örnek kümesi raporu. Çıkış: seçilen dil/font/ışık örneklerinde kanıt; evrensel OCR başarısı veya deep-learning kusur modeli iddiası yok.

### T12 — Renk kontrolü

- [ ] **T12.1 — Color analyzer.** RGB/BGR giriş doğrulama, HSV/Lab dönüşümü, hue wrap ve düşük saturation politikası, ROI oranları/ortalama/dağılım; referans renge açık formüllü CIE76 ΔE. Gray image renk ölçemez.
- [ ] **T12.2 — UI/Result.** Referans örnek öğretme, aralık/alan oranı/ΔE eşiği, model hash, tracking, mask overlay, gerçek renk ayrımı kuralları ve kayıt. Otomatik white-balance kaymasını fiziksel renk sabitliği gibi sunma.
- Testler: `color_inspection_test`, channel order/hue wrap/gray ret/model change, UI. Çıkış: tanımlanmış kamera/ışık koşullarında renk kontrolü; spektrofotometre doğruluğu iddiası yok.

### T13 — Kontur/yüzey kusuru ve bead inspection

- [ ] **T13.1 — Reference defect.** İyi örnek/model öğretimi, T8 registration, mask/ignore regions; signed difference ve contour distance ile eksik/fazla bölge, boy/alan/konum. Yanlış hizalama veya geçersiz referans kusur kararı üretmez. Aydınlık normalizasyonu tarifli ve sınırlı.
- [ ] **T13.2 — Defect UI/kabul.** Gerçek defect mask/list, min/max alan/uzunluk/severity kuralları, kenar çapak/çentik örnekleri. Çizik tespiti yalnız test edilen kontrast/ölçek kapsamı; genel anomaly AI eklenmez.
- [ ] **T13.3 — Bead çekirdeği.** Öğretilmiş polyline/spline yol boyunca normal scan ve edge-pair; genişlik, offset, süreklilik ve kopuk segment uzunluğu. Köşe/self-intersection/çoklu bead/yanlış çift politikası. Raw→world dönüşümüyle mm, eksik scan sonucu sıfır genişlik diye kaydedilmez.
- [ ] **T13.4 — Bead UI/Runtime.** Path öğretme/yerel toleranslar, renkli width profile ve gap bölgeleri; Result/Logs/CSV, save/reopen/Stop. Fit için elenen noktalar açıkça raporlanır.
- Dosyalar: `vision/inspection/ReferenceInspector.*`, `BeadInspector.*` ve application adapters. Testler: `reference_defect_test`, `bead_inspection_test`, UI. Çıkış: örnek kapsamı belli çapak/eksik bölge ve yapıştırıcı hattı kontrolleri.

### T14 — Lens öncesi birleşik yazılım kabulü

- [ ] **T14.1 — Reçete senaryoları.** Kod+çap+adet; dönmüş parça+delik mesafesi; OCR+renk; kusur+bead. Mevcut üç applicationType, aynı Editor/Runtime. Robot tipi kullanılsa da robot pose üretilmez.
- [ ] **T14.2 — Birleşik kalite.** Temiz Release UI/core, bütün CTest, 1400×900/850×600 görsel inceleme; Windows native DPI ayrı rapor. 1/5/16 MP benchmark ve gerçek corpus precision/recall/false accept/false reject/ölçüm hata raporu. Sonuçlar hedef donanım/kapsamla kayıtlı; gerçek veri yoksa fiziksel kapı açık.
- [ ] **T14.3 — Belge kapanışı.** README/gereksinim/mimari/backlog/progress gerçek durum; dependency inventory, recipe compatibility ve örnek kullanım. Lens eksikliği ve P7/K4/P8/P9/6E açık işleri net.
- Çıkış: tool yazılım paketi kabul kanıtı. Dağıtım/CUDA/PLC donanımı/fiziksel metrologi kabulü otomatik kazanılmaz.

### T15 — Lens bozulması düzeltmesi (en son)

- [ ] **T15.1 — Çok görüntülü intrinsic model.** Plaka ile farklı konum/açı görüntüleri, K/distCoeffs ve model/çözünürlük/lens/focus kimliği; gözlem dağılımı ve bağımsız holdout. Tek homography'yi lens kalibrasyonu diye kullanma.
- [ ] **T15.2 — Koordinat/persistence.** Yeni versioned lens+plane model; raw→undistorted→plane zinciri, inverse overlay, valid mask ve kurulum bağı. Legacy H raw'a ait olduğundan undistorted noktaya tekrar uygulanmaz; plane model yeni uzayda yeniden öğretilir/doğrulanır.
- [ ] **T15.3 — UI ve bütün tool regresyonu.** Kalibrasyon wizard/data kayıtları, bütün metrik tool'lar, model/cache/snapshot invalidation; lens disabled legacy eşitlik; sentetik distortion ve bağımsız fiziksel alan testi. Fiziksel veri yoksa yazılım tamamlanmış, fiziksel kabul açık raporlanır.
- Dosyalar: mevcut Calibration/Chessboard/PlaneMapping/Measurement services, Coordinates/codec/UI; yeni `LensCalibrationService.*`. Testler: `lens_calibration_test`, `lens_metrology_runtime_test`, mevcut bütün kalibrasyon hash/codec testleri ve T14 regresyonu. Fisheye ve robot calibration ayrı sonraki kapsamdır.

## 5. Güncel başlangıç — GPT-6.1-sol medium

Sıradaki uygulama **R1**. Kopyalanabilir güncel talimat [Ortak Referans planının 7. bölümündedir](reference-system-plan.md). Önce progress kontrol edilir; tamamlanmış U1–U4/T8 işleri yeniden başlatılmaz. R1–R8 alt paketleri ve kabul ölçütleri aynı planın 6. bölümündedir.

### Tarihsel T0 başlangıç talimatı

Aşağıdaki metin ilk T0 oturumunun tarihsel kaydıdır; güncel uygulamayı başlatmak için kullanılmaz.

```text
VisionFlow reçete tool planının yalnız T0.1 → T0.2 → T0.3 paketlerini uygula.

AGENTS.md, docs/progress.md, README.md, docs/architecture/recipe-tools-plan.md,
recipe-tools-algorithms.md ve recipe-tools-acceptance.md dosyalarını oku.
Tamamlanmış C1–C5/K1–K3/P1–P6'yı yeniden yapma. Dirty/untracked işleri koru.
Bu oturumda yeni kullanıcı tool'u, schema migration veya lens correction yazma.

Yerel OpenCV 5.0.0/Qt/MinGW modüllerini ve gerekli API'leri compile/link/run
probe ile doğrula. OpenCV 4.x örneklerini aynı modül adlarıyla kopyalama.
ZXing-C++ reader-only bağımlılığı için sabit upstream sürüm/commit, hash,
lisans ve offline build yolunu kaydet; Qt/MinGW C++20 ile fixture decode et.
Normal configure sırasında otomatik internetten en yeni sürümü çekme.
OpenCV/Qt sistem kurulumunu değiştirme. Tesseract hazırlık notunu kaydet,
OCR entegrasyonunu T11'e bırak. Gerçek backend yoksa sahte başarı üretme.

Her alt paketin anlamlı testini yap; README'deki Release toolchain'i kullan.
Test/probe/INI/PNG/CSV yalnız build altında; config/local.ui.ini ve açık
uygulama korunur. Her alt paket sonunda progress'e kanıt, sınırlama ve
sıradaki alt ID yaz. T0 bitince T1.1'i sonraki iş olarak bırak.
UI değişmediğinde UI görsel testi yaptığını iddia etme. Fiziksel doğruluk,
kurulum/dağıtım/CUDA kabulü iddia etme. Lens T15'te en sonda kalır.
```

Sonraki oturumlar için kısa şablon: “Progress'teki son kabulü kontrol et; yalnız **Tn.a–Tn.b** aralığını recipe-tools planına göre uygula. Önkoşul eksikse önce bildir ve aynı yetkili kapsamda tamamlanabilecek işi yap. Her alt paketi test/kanıt/progress ile kapat; kapsam dışı pakete geçme.”
