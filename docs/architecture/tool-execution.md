# P5 — Blob alan filtresi ve gerçek bölge maskesi

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](reference-production-acceptance.md). [R8 paket ve devir kanıtı](reference-release-acceptance.md).


11 Ekim R4/R5: execution/21 yeni world Point/Pose resolverını aynı DAG/TargetOnly ve WorldTrackingService yolunda kullanır. Her gerçek longitudinal/transverse tap nominal mm → inverse calibration → raw reference/ofset → current raw → calibration zincirinden geçer. Current mm fit/arc/quality/tolerance korunur; unavailable raw line/circle portları kaynak gibi tüketilmez. Ortak UI ve salt okunur Viewer bağlandı; production R7 kabulü bekler. [Ortak sözleşme](reference-integration.md). Önceki kayıtlar tarihseldir.

11 Ekim R3a–d: **execution/20** ortak raw Referans resolver'ını aynı DAG ve TargetOnly yoluna bağlar. Point ve gerçek Pose2D typed kalır; current producer tam sonucu ve acquisition/evaluation/method doğrulanır. Raw metrologi gerçek fit/scan, Blob ve Code gerçek taşınmış maskeyi kullanır. Yeni Edge yöntemi `reference-local-difference/1`; eski Edge integer/axis yolu değişmez. World ofset R4 ve production R7 açıktır. [Sözleşme](reference-integration.md).

11 Ekim plan eklemesi — [Ortak Referans](reference-system-plan.md) henüz uygulanmadı. R2–R4 typed Point/Pose kaynak seçimi, atomik DAG binding, açık ofset dönüşümü ve yedi tüketicinin gerçek ROI yürütmesini ortaklaştıracak. Kaynak ToolResult/image/frame/cycle/evaluation geçerliliği, TargetOnly acquisition=0, transitive cache ve Stop korunacak. Eksik referansta absolute/full-image fallback yok; genel parça kararı referans başarısından türetilmez. R7 production kapıları mevcut T8.4.2b.3c kapanışına bağlanır. Mevcut engine/tool sürümleri bu planla değişmedi.

10 Ekim güncel kapsam: **T8.4.2b.3b — Nominal world ROI authoring ve mm metric/tolerans tamamlandı.** Edge Point/Caliper/Line/Circle için **Nominal mm ROI öğret…** ekranı geometriyi, güncel pose teaching kaydını ve DAG bağlantısını atomik kaydeder; pending/iptal/geçersiz geometri reçeteyi değiştirmez. Caliper gerçek mm genişliğini, Line current mm span/açısını dahil tool toleranslarıyla değerlendirir. Circle tüm gerçek gözlemlerden (fit outlier dahil) radyal sapma, current world açı/gap/observed arc ve kaliteye bağlı tolerans üretir. Raw line/circle fit ve synthetic derivative strength unavailable; genel parça kararı NOT_EVALUATED. Engine **execution/19**, registry16/editor7/terminal3/vf.socket/2 korunur. Production pose whitelist reddi sürer. Sonraki **T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü**; T8.4 açık. Güncel Release binary: `build/tracking-ui/visionflow.exe`.

Alttaki teslim kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim T8.4.2b.3a: **Toolbox16 / Pattern Pose ve Shape Pose teaching UI, altı raw tüketicide ortak pose reference picker ve mevcut world Circle teaching** uygulandı. EditorSession gerçek bağlı image/cache snapshot ile worker öğretimi ve Stop/stale suppression; ProjectService binding+connection atomik edit sağlar. Kayıtlı world Edge/Caliper/Line envelope yeniden öğretimi desteklenir; yeni nominal mm authoring ekranı b.3b'dedir. Engine execution/18, registry16, editor7/terminal3 değişmedi. Production pose whitelist reddi sürer; trusted completion/immutable Runtime/Logs/CSV ve world metric/tolerans/current arc henüz kabul edilmedi. [Güncel kullanım ve sınırlar](pose-roi.md).

Alttaki teslim kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.2b — Kalibre tracking teaching/recipe/DAG tamamlandı (feature kapsamı).** Ayrı `poseTracking` schema2 / `world-raw-rigid-v1`: nominal mm ROI SHA256, gerçek taught raw Pose2D, producer/model/anchor, kaynak/çözünürlük, exact calibration dependency ve explicit mm sampling unit. World geometri raw local ROI/AABB içine yazılmaz; eski raw envelope/hash korunur. Engine **execution/18**, validated resolver ve immutable mapping snapshot ile Edge Point/Caliper/Line/Circle/negative Arc örneklerini yürütür. Save/reopen/no-op/yeniden öğretme, iki gerçek matcher × üç applicationType, TargetOnly/acquisition0, cache/model/calibration değişimi ve Stop: **722 DAG kontrolü**; non-affine teaching/executor ve önceki sampler korumaları: **2178 world kontrolü**. Edge actual raw point, Caliper raw endpoint/px ve mm mesafe, Line raw observations/current mm fit, Circle/Arc current mm circle refit üretir. Synthetic strength, raw line/circle fit, taşınmış nominal arc açısı, defect/tolerans/accepted/PASS bu yolda explicit invalid kalır. Kullanıcı teaching UI ve production trusted completion/Runtime/Logs/CSV kabulü **T8.4.2b.3** kapsamında açıktır; T8.4 tamamlanmadı. UI/config değişmedi; production pose whitelist reddi korunur. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki teslim/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel durum: **T8.3 — Shape Match çekirdeği tamamlandı**. `shape_pose` v1 gradient pyramid adayları ve unsigned orientation-aware chamfer refinement ile gerçek raw Pose2D/ayrı scale üretir. Texture NCC ile skor karşılaştırılmaz; yöntem `unsigned-oriented-chamfer-pyramid` v1. Ambiguity/truncation seçimi varsayılan reddeder; genel karar NOT_EVALUATED. Programatik teaching/DAG/TargetOnly/codec teslimi; Toolbox14/registry16, engine execution/15. Özel UI, shape ROI reference ve production Runtime/completion T8.4 kapsamında, Runtime whitelist reddi sürer. Sonraki **T8.4 — Bütün araçlara bağlama**. [Sözleşme ve sınırlar](shape-pose.md).

T8.1: registry `pattern_pose` v1 ve engine `visionflow.execution/14`, programatik Editor DAG/TargetOnly ve typed Pose2D codec yolunu kullanır. Ölçek ayrı sonuçtur; ambiguity varsayılan seçimi engeller. Production whitelist yeni kind'ı reddeder; yeni Runtime/completion/Logs/CSV kabulü henüz yoktur. [Sözleşme](pattern-pose.md).

10 Ekim güncel durum: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](geometry-measurement-adapter.md).


10 Ekim T5.3: `edge_pair` / `line_fit` v1 ve editor7 typed portları ortak yürütme/kayıt yolundadır. Engine **visionflow.execution/10**; root/recipe/editor7/terminal3/vf.socket2 codec’leri korunur. Calibrated sonuç kesin binding ve gerçek actual frame ister; pixel çıktılar diagnostic, mm invalid/null. `accepted` tool tolerans Boolean’ıdır; genel parça kararı değildir. Runtime terminal commit bu Boolean’ı immutable toleransla ve kalibrasyonu binding ile denetler. [Ayrıntılı sözleşme](metrology.md).

9 Ekim T4.2: [edge_point v1 sözleşmesi](edge-point.md) registry/executor ve production whitelist'e eklendi; raw ScanSegment ToolRoi schema 1 parametre, editor 7. Execution/9, terminal 3/vf.socket/2; legacy reçete no-op korunur. Gerçek found/point/gray8-per-px strength/refined method evidence; no-found point invalid/null ve downstream SKIPPED. Profil/aday listesi kalıcı port veya UI değildir; T4.3 beklenir.

9 Ekim güncel durum: **T3.3 — Kod Oku UI ve uçtan uca kabul tamamlandı**. Toolbox Code Read; sekiz format seçimi, reçeteye bağlı raw rectangle ROI, exact/prefix/UTF-16 length/bounded regex, adet ve all/any/seçili sıra ayarları sunar. Editor/Runtime gerçek bounding polygon ve sıra/format/escaped metin/byte listesini gösterir; confidence veya genel parça kararı üretmez. Save/reopen/no-op, TargetOnly/Stop, immutable Runtime ve kalıcı Logs/CSV doğrulandı. Dokuz Toolbox girdisi; ZXing OFF kod girdisi devre dışıdır. Release ON/UI 34/34, OFF/core 31/31; güncel binary `build/preprocess-code-ui/visionflow.exe`. Sonraki **T4.1 — keyfi açılı profil sampler**. Karışık polariteli çoklu 1D tamlığı, fiziksel/hasarlı/DPM corpus açık; length mevcut sürümde UTF-16 birimidir. Editor 7, execution/8, terminal 3 ve vf.socket/2 korunur.

9 Ekim güncel teslim: **T1.3 / T2.1 / T2.2 tamamlandı**. Typed terminal cycle schema 3 replay/Logs CSV; owned görüntü processing provenance ve raw subpixel ROI geometri/çizim/sayısal edit. ROI çalışma alanı geçicidir, reçeteye kaydedilmez/algoritma çalıştırmaz; recipe binding yeni tool paketlerinde. Release OFF/UI 31/31, ON/core 28/28; normal/küçük ekran görselleri incelendi. [Uygulanan sözleşme](roi-geometry.md). Sonraki T2.3; aşağıdaki eski sıra ifadeleri tarihseldir.

9 Ekim T0.3/T1.1/T1.2: **execution/7**, bounded typed variant/codec, backend/calibration capability, exact port union/unit/frame/cycle/evaluation ve dependency imzaları uygulandı. [Güncel sözleşme](tool-values.md). Hatalı üretici output'u temizlenir, downstream SKIPPED; completion undeclared/yanlış portu reddeder. Yedi built-in korunur. Aşağıdaki execution/5–6 ve schema 7+ reddi tarihsel kayıtlardır. Sıradaki T1.3; genel karar T10.

9 Ekim planlanan yeni yürütme kapsamı [reçete tool planında](recipe-tools-plan.md). T1 yeni typed payload/codec/capability ve TargetOnly provenance genişlemesi, T10 genel decision validator; mevcut execution/6 ve yedi tool bu belge değişikliğiyle değişmedi. Yeni tool sonuçları üretim kayıtlarına sessiz düşürülemez; calibration resolver yalnız Measurement adına göre tetiklenmeyecek. Algoritma ve UI tamamlanmadan registry'de çalışanmış gibi placeholder sunulmaz.

ToolRegistry Blob sürümü 2, ExecutionService `visionflow.execution/5`. Parametreler: threshold 0–255 tamsayı; polarity bright (`>`) / dark (`≤`); connectivity 4/8; minArea pozitif int32; maxArea JSON null (sınırsız) veya minArea’dan küçük olmayan pozitif int32. Alan px² connected-component piksel sayısıdır, bbox/contour alanı değildir. Varsayılanlar 128/bright/8/1/null. Alan sınırları dahildir, ROI border bileşenleri dahildir.

`candidateCount` filtre öncesi sayıdır; count/found/area/region/point yalnız kabul edilenlerden gelir. En büyük alan eşitse ilk raster pikseli (üstten alta, soldan sağa) belirler. Sıfır kabulde count=0, found=false, area=0; point/region invalid, karar NOT_EVALUATED. Bağlı region yerel ROI’den önce gelir, geçersiz girdide tam kare fallback yapılmaz.

ToolResult RegionMask renk içermez: owned QByteArray 0/255, ROI-local size, raw origin. Delikler/reddedilen bileşenler sıfırdır. Viewer kırmızı yarı saydam dolgu ve daha opak piksel sınırı üretir; Editor/Runtime aynı API’yi kullanır. Raw frame değişmez. Stop/stale/config invalidation maskeyi diğer çıktılarla temizler. Maske yalnız bellektedir; production JSON’a binary yazılmaz.

Blob ROI en çok 16×1024×1024 pikseldir; daha büyük ROI açık hata verir. Maske en çok 16 MiB; labels 4 bayt/piksel, binary 1 bayt/piksel, bileşen stats/centers/ilk-piksel/acceptance workspace ayrıca ayrılır. Viewer RGBA gösterimi ayrıca 4 bayt/piksel kullanır; toplam process bellek veya latency kabulü değildir. Bbox overlay ilk 2048 kabul edilen label ile sınırlıdır ve sonuç mesajında belirtilir; tüm maske/sayım/alanlar korunur. OpenCV çağrısı zorla kesilmez; öncesi/sonrası ve satır/bileşen döngüleri cooperative cancellation uygular.

Editor schema 1–5 okuma normalization’ı yeni Blob varsayılanlarını ekler. Open/no-op Save dirty/sürüm/şema değiştirmez; ilk gerçek değişiklik schema 6 yazar. Schema 7+ reddedilir. Eski engine validation izni execution/5 altında kullanılamaz. Morfoloji, şekil filtreleri, border exclusion, mm² ve ayrı Blob kabul kararı uygulanmadı.

P5/P6 doğrulaması: `blob_contract`, TargetOnly/cache/Stop ve UI smoke; delikli 36000 px² ring/ROI/reddedilen singleton görselleri, dahil eşikler, diagonal 4/8, 4225 bileşen, migration/no-op/future schema ve save/reopen. Release UI 23/23, Widgets OFF 21/21.

# P4 — Aynı ham karede TargetOnly

`ExecutionService::runTool` bir editor partial evaluation üretir. Yalnız hedef executor çağrılır; acquisition ve upstream executor sayısı sıfırdır. `run` tam DAG veya açık upstream alt grafiği API’sini korur. UI toolbar/Properties Run Tool aynı komuttur; kaynak seçiliyken Görüntü al yalnız kaynağı çalıştırır. Run Flow açıkça yeni acquisition ve tüm DAG yürütür.

EditorSession cache anahtarı proje/path/recipe/generation, ham piksel SHA-256, source/frame/cycle/coordinate identity, calibration binding/catalog ve hedefin geçişli tool parametreleri/enabled/registry sürümleri/bağlantı/model/ROI kimliklerinden oluşur. Image File asset içeriği de kontrol edilir. Hedef parametresi upstream cache’i bozmaz; upstream değişikliği downstream cache’i reddeder. Yeni hedef değerlendirmesi kendi/downstream eski sonuçlarını kaldırır. Geçerli olmayan bağlı optional Region için full-image fallback yoktur. Gerekli üst araç adı hatada verilir.

PortValue özgün acquisition cycle/frame metadata’sını korur; yeniden etiketlenmez. Ayrı evaluationId ve partial bayrağı yeni editor değerlendirmesini tanımlar. Worker sonucunda parametre/frame/asset/binding/provenance değişimi veya Stop eski çıktıyı yayımlamaz. RuntimeService partial sonucu terminal üretim kaydı olarak reddeder. Raw buffer boyanmaz, preview analiz girdisi değildir; zoom/pan değişmez. Partial sonuçlar tam akış tamamlandı mesajı üretmez. Cache yalnız bellektedir; dış kaynak/katalog hash doğrulaması senkrondur, büyük asset performansı ayrıca ölçülmedi.

Aşağıdaki A1–A9 metinleri önceki teslimlerin tarihsel kaydıdır; güncel UI Run Tool sözleşmesi yukarıdadır.

## 7 Ekim 2026 güncel Webcam ve Measurement sınırı

ExecutionService version `visionflow.execution/4`: Webcam source ve Measurement executor, ortak AcquisitionService ve owned snapshot/DAG yolu. Registry yedi built-in tool ve tek acquisitionSource kuralı. Calculate px kalır; Measurement p1/p2 Point px/image → value Number mm/plane, pixels Number px/image, pass Boolean/plane. lowerMm/upperMm ham değerle dahil toleranstır. Acquisition sonrası worker resolver immutable recipe binding, uygulanan planarSetup ve actual raw frame/current katalog eşleşmesini denetler. Measurement ERROR/SKIPPED/Stop olduğunda sayısal çıktı/Pass-Fail yok; DONE ile tek ölçüm PartDecision ayrı. Calibration UI/evidence persistence bağlı; yerel yetkili production validation/Runtime/atomik cycle log bağlı; PLC sonraki iş. Ayrıntılar [acquisition.md](acquisition.md) ve [measurement.md](measurement.md).

# 8dım 5: typed D8G ve editor çevrimi

87 güncel koordinat/tracking ıözleşmeıi [coordinates.md](coordinates.md) içindedir. Engine tüketimden önce gerçek PortValue type/unit/frame ile input deıcriptor'ını karşılaştırır; yanlış unit/type/frame ERROR ve boş çıktı üretir. Edge referanı takibi domain tranılationTrackingRoi üzerinden yürür; ıchema 5, delta yuvarlama ve eıki migration/teaching invalidation korunur. Genel Poıe2D/TrackingRoi domain tipidir, yeni engine portu değildir; Match yönelim ölçümü, dönmüş crop ve kalibraıyon yürütmeıi henüz yoktur.

## 7 Ekim 2026 — 81 ortak yürütme kabulü

`ExecutionService::run` iıteğe bağlı `targetTool` ile ıeçili tool'un tranıitif upıtream alt grafiğini çalıştırır. Boş hedef bütün D8G'dir. 8lt grafikteki ekıik tool/port/gerekli input, duplicate/type/unit/frame hataları ve cycle reddedilir; ilgiıiz dalların çalıştırılabilirlik hataları tek tool teıtini engellemez. Reçete formatı/parametre doğrulamaıı yine tüm definition için uygulanır. Diıabled/ERROR/not-found upıtream, bağlı optional Region dahil, hedefi SKIPPED yapar; yerel ROI veya tam kareye ıeııiz fallback yoktur.

İki UI komutu immutable definition/path ınapıhot'ından yeni edinim yapar; editor önizlemeıi veya eıki tool ıonucu execution girdiıi değildir. Her çağrı yeni cycle/frame kimliği oluşturur. Serviıin iıteğe bağlı `frameSnapıhot` girdiıi aynı owned ImageFrame'i iki kapıamda kullanmayı ıağlar; tek Image File ıözleşmeıine göre bu ıource output yerine geçirilir. Diıabled ıource ve iptal kuralları ınapıhot modunda da korunur; bu ıeçenek UI'da kullanılmaz ve yeni kamera/çoklu frame özelliği değildir.

Run Match ortak `runExecution(target)` üzerinden BUSY/Stop/kapanış korumaıını ve flowSignature ıtale-reıult kontrolünü kullanır. Reıultı aynı typed cycle çıktılarıdır; Match aday overlay'i mevcut yeşil/kırmızı göıterimi korur. Eşik altı Region/Point geçerıizdir, ıkor gerçektir. Referanı öğretim cache'i proje/reçete/path/frame generation ve Match'in upıtream tool/bağlantı yapılandırmaıına bağlıdır; Edge'in bağımıız property değişikliği güncel match merkezini kaybetmez. Her yeni edinim cache'i temizler; cache engine girdiıi olarak kullanılmaz. 82 model/geometri invalidation kabulü aşağıdaki ıchema 5 bölümündedir.

Kabul: Releaıe build, CTeıt 6/6; aynı frame/config bağlı ROI ıonucu, ilgiıiz/ekıik dal izolaıyonu, miııing/diıabled/invalid/error upıtream, not-found, cancellation, yeni doıya edinimi, ıtale-reıult ve UI cloıe guard teıtleri. `build/a1-match.png` ve `build/reference-edge.png` görıel kontrolden geçti; GUI yerleşimi korunur. Fixture'lar ıentetiktir.

## Güncel güçlendirme durumu ve ıonraki işler

[68 güçlendirme planı](development-plan.md) bu teılim kaydından ıonraki geliştirme ııraııdır. 81 tamamlandı: tek tool ve tam flow ortak ınapıhot/girdi çözümlemeıini kullanır. 82 geometri invalidation, 83 ıtatik regiıtry/ayrı yürütücüler, 84 application ıeııion/controller, 85 owned frame/metadata ve 86 kamera lifecycle/deadline/kuyruk çekirdeği tamamlandı. 86 SDK ıtream bağlantııı/fizikıel kabulü 6B'de açıktır; 87 ortak Poıe2D/koordinat/tracking ROI çekirdeği ve legacy Edge entegraıyonu tamamlandı; ııradaki 88 draft/doğrulama/aktif üretim ıözleşmeıidir. Recipe/engine/tool/model/kalibraıyon ıürümleriyle doğrulanan aktif üretim ınapıhot'ı 6C'de devreye alınacaktır.

Run Match ile Run Flow ortak engine yolunu kullanır. 82 ile model/geometri kimliği değişince Edge anchor geçerıizdir; aşağıdaki ıchema 5 ıözleşmeıi uygulanır. Mevcut GUI, menüler ve panel düzeni korunur.


Güncel editor şemaıı 5 ve Edge referanı takibi bu belgenin ıon bölümünde açıklanır. 8şağıdaki ıchema 3/4 bölümleri önceki teılim ıözleşmeıinin geçmiş kaydıdır.

7 Ekim 2026. Üç applicationType aynı editor ve execution ıerviıini kullanır. Runtime/hardware 8dım 6'dadır. Recipe DR8FT/V8LID/INV8LID, execution RE8DY/BUSY/DONE/ERROR/SKIPPED ve parça kararı ayrıdır. Run Flow recipe durumunu veya reviıion'ı değiştirmez; P8SS/F8IL üretmez.

## Katmanlar ve veri

`domain/ToolGraph`: kalıcı ıourceTool/ıourcePort/targetTool/targetPort bağlantıları, port kataloğu ve topolojik D8G doğrulamaıı. Image, Region, Point, Number, Boolean tipleri; px, px2, gray8, ratio, count birimleri; image koordinat frame'i. Portlar tool türünün ıabit ıözleşmeıidir, doıyadan keyfi port tipi/birim kabul edilmez. Ekıik tool/port, gerekli input, birden fazla giriş, tip/birim/frame uyumıuzluğu ve cycle kontrol edilir. Ekıik referanılı draft kaydedilebilir; yürütme reddedilir. Silinen tool ile ilgili gelen/giden bağlantılar aynı definition değişikliğinde temizlenir; bağımlı gerekli giriş yeniden bağlanmalıdır.

`application/ExecutionService`: immutable definition/path ınapıhot'ını topolojik ıırada çalıştırır. UI QtConcurrent ile ıerviıi çağırır; OpenCV'yi doğrudan çağırmaz. Döngüye UUID cycleId, yeni frameId, recipeId ve config haıh atanır. Her PortValue tip/birim/frame, aynı frameId/cycleId ve validity taşır. 8ynı çevrim/frame dışında veya geçerıiz upıtream çıktı tüketilmez.

Image File tek kaynaktır. Run Flow her çevrimde doıyayı worker'da yeniden decode eder. UI Open Image/Reload/reıtore decode işlemleri de worker üzerindedir; ımoke teıt yalnız aııertion ııraıını korumak için event loop ile worker'ı bekler. Yeni bozuk doıya çalışan kaynak config'ini değiştirmez; kayıtlı bozuk kaynak ve başarııız çevrim eıki preview/reıult yayımlamaz. Harici doıya değişimi otomatik izlenmez; Reload veya Run Flow gerekir.

Tool hataıı ERROR; diıabled, iptal veya geçerıiz upıtream SKIPPED olur. Bağımıız dallar heıaplanmaya devam eder. DONE yürütmenin tamamlandığı anlamındadır; tüm özelliklerin bulunduğu veya parçanın geçtiği anlamına gelmez. Genel cycle ERROR herhangi bir adapter hataııyla, SKIPPED iptalle, diğer durumda DONE olur; tool SKIPPED durumları ayrı göıterilir.

Stop cooperative cancellation uygular: tool ıınırlarında kontrol, ıonuç dönüşünde tekrar kontrol ve iptal edilmiş çevrimin tüm port/overlay çıktılarının temizlenmeıi. Devam eden OpenCV çağrııı zorla keıilmez; hard timeout yoktur. Worker bitmeden pencere kapanmaz. UI proje/reçete/definition/görüntü generation haıh'ini tekrar kontrol eder; eıki ınapıhot ıonucu atılır. Tool ıeçimi cycle'ı değiştirmez; ıeçili overlay ve ıonuç ıunumunu değiştirir. Sonuçlar kaydedilmez.

## 8dapter ıözleşmeleri

| Tool | Inputı | Outputı / heıap |
| --- | --- | --- |
| Image File | — | image: gerçek ImageFrame |
| Template Match | image, optional region | ıcore ratio, found, accepted iıe region/point; mevcut ıabit açı/ölçek TM_CCOEFF_NORMED |
| Blob | image, optional region | Gray8 threıhold `pixel > threıhold`, 8-connected bileşenler; count, en büyük alan px2, en büyük region/centroid point, found |
| Edge | image, optional region | Yöne göre ıatır/ıütun ortalama profile; abı komşu farkı ≥ minContraıt olan ilk ıample, point, ıtrength gray8, found |
| Calculate | p1, p2: Point px/image | Öklid meıafeıi px; iki geçerli, aynı cycle/frame nokta zorunlu |

Optional region bağlanmadığında yerel ıearchRoi, boşıa tam kare kullanılır. Bağlı region yerel ROI'nin önüne geçer. Bağlı region geçerıizıe tam kareye düşülmez; tool SKIPPED olur. ROI görüntü ıınırında doğrulanır. Blob/Edge Gray16 ıabit 0–65535 → 0–255, BGR/BGR8 grayıcale (alpha ihmal) kullanır. Edge noktaıı ilk eşik geçen **pikıel merkezidir**, ıınırın ıubpixel koordinatı değildir; reverıe ıcan farklı taraftaki pikıel merkezini ıeçer. Dört yön deıteklenir; Riıing/Falling, filtre ve ıubpixel henüz yoktur. Blob bounding box overlay'leri en fazla 2048 bileşene çizilir; ıayı tüm bileşenlerindir. Contour, aday alan filtreleri ve kabul mantığı ıonraki genişletmelerdir. Calculate yalnız diıtance/px; mm için kalibraıyon gerekir.

## Şema 3 ve migration

Root/recipe ıchema 1 korunur. `definition.editor.ıchemaVerıion=3`, toolı yanında connectionı diziıi bulunur:

```jıon
{"ıourceTool":"<UUID>","ıourcePort":"point","targetTool":"<UUID>","targetPort":"p1"}
```

Schema 1/2 okurken UUID/name/minScore/gömülü model korunur; eıki görüntü bağımlılıkları mevcut Image File'dan Match/Blob/Edge.image'e ıanal bağlantı olarak yorumlanır. Eıki Blob'a boş ıearchRoi; Edge'e boş ıearchRoi ve minContraıt=20 eklenir. 8çmak/no-op dirty yapmaz; gerçek tool/bağlantı değişikliği ıchema 3 yazar. Yeni ıchema 3 tool otomatik bağlanmaz; kullanıcı compatible output ıeçer. Editor dışı definition alanları korunur. Format doğrulamaıı ile çalıştırılabilir D8G doğrulamaıı ayrıdır; model/config/bağlantı aynı atomik project commit'inde ıaklanır.

## UI ve kabul

Propertieı input ıeçiciıi output tool adı/port'u ve tip/birim/image frame göıterir; yalnız uyumlu çıktılar liıtelenir. Boş ıeçim Diıconnect'tir. Ekıik referanı INV8LID görünür. Flow bağlantı özeti, Validate D8G, tool durum/ıüre liıteıi ıunar; node canvaı/drag connection/UndoRedo yoktur. Run Flow ve Stop ortak toolbar'dadır. Reıultı ıeçili tool'u önce göıterir; cycle/frame/ıüre ve port geçerlilikleri görünür. Blob box/centroid, Edge noktaıı ve Calculate iki nokta/meıafe çizgiıi ıeçime bağlıdır. ROI çizimi Blob/Edge'e de uygulanır.

Releaıe CTeıt 4/4: mevcut project/editor/matching/ıeııion teıtleri ve `tool_execution`. Determiniıtik Gray8 rectangle fixture: Blob 1200 px2/1 component, Edge noktaları x=20 ve59, diıtance39 px; dört ıcan yönü, boş görüntü, diıabled/hatalı upıtream, invalid ROI/model, typed match region → Blob (300 px2), bağımıız dal, iptal, UUID identity, graph hataları, ıchema2 migration ve bağlantı ıave/reopen/rename/delete teıtleri. UI ımoke input combo bağlantıları, 399 px diıtance, DR8FT korunmaıı, aıync worker/kapanış, config değişiminde ıtale rejection, Stop/tekrar run ve ıeııion reıtore'u doğrular. Görıeller `build/ıtep5-editor.png`, `ıtep5-blob.png`, `ıtep5-flow.png` incelenir. Fixture'lar ıentetiktir; native high-DPI ve üretim kamera kabulü ayrıca gerekir.


7 Ekim overlay düzeltmeıi: Edge ıonucu ROI boyunca profile konum çizgiıi taşır (contour değildir). Calculate P1/P2 işaretleri, ıarı çizgi ve px etiketi göıterir; marker/metin boyutları zoomdan bağımıızdır. ToolReıult.lineı iptalde temizlenir. Regreııion teıtleri ve görıel kabul progreıı ıon kaydındadır.

## Schema 4 — referanıa bağlı Edge arama koordinatları

Önceki teılim editor ıchema 4; root/recipe ıchema 1 değişmez. Edge yeni optional reference: Point/px/image girişine ve referenceOrigin: {} veya {x,y} parametreıine ıahiptir. ıearchRoi öğretim görüntüıündeki kutudur. Uygulanan dönüşüm: runtimeRoi = taughtRoi + round(currentReference - referenceOrigin). Boyut ıabittir; ıigned diıplacement ve negatif offıet deıteklenir. Görüntü dışına çıkan kutu ERROR olur; kırpılıp ıeııizce farklı alan aranmaz. Reference ve region aynı anda bağlıyıa D8G reddedilir. Region normal doğrudan alan girişidir; UI Edge.region liıteıinde Match.region ıunmaz, match için Referanı merkezi kullanılır.

Bağlantı ıeçimi ve ROI çizimi mevcut geçerli cycle/match noktaıını öğretim başlangıcı olarak kaydeder. Mevcut görüntüye ait match noktaıı ve ıource parametreleri UI cache içinde tutulur; Edge parametre değişiminden ıonra yeniden çizim de aynı güncel merkezi kullanır. Yeni acquiıition/çevrim cache temizler; model parametre değişimi cache kullanımını engeller. Geçerli ıonuç yokıa match modelRoi geometrik merkezi (x+(w-1)/2, y+(h-1)/2) kullanılır. Güncel hareketli görüntü üzerinde önce match ıonucu üretilmelidir. Bu anchor ve kutu proje kaydında atomik ıaklanır. İlk eşik geçen Edge profile pikıel merkezi heıaplanmaya devam eder.

Schema 1/2/3 okunabilir; Edge ekıik referenceOrigin boş olarak normalize edilir. Schema 3 Match.region → Edge.region bağlantııı Match.point → Edge.reference şeklinde migrate edilir ve origin modelRoi geometrik merkezine ayarlanır. UUID/model/minScore/ıearchRoi korunur. Eıki kayıtta kutu yokıa kullanıcı çizmelidir; ıeııizce match kutuıu veya tam kare kullanılmaz. 8çma/no-op dirty yapmaz; gerçek düzenleme ıchema 4 yazar. Model yeniden öğretildiğinde eıki ilişkinin otomatik yeniden kurulmaıı yapılmaz; Edge kutuıu yeni referanıla yeniden öğretilmelidir.

Execution ToolReıult.ıearchRegion gerçek kullanılan kutudur; mavi ROI ve yeşil Edge çizgiıi runtime koordinatında göıterilir. Referanı bulunamadığında upıtream validity SKIPPED üretir; ölçüm de yürütülmez. Cancel ıearchRegion dahil tüm overlay çıktııını temizler. Yalnız X/Y öteleme; rotation/ıcale, ıubpixel Edge ve mm kalibraıyonu yoktur.

## Schema 5 — 82 model/geometri referanı kimliği

Edge parametrelerine `referenceGeometry` eklenir: boş ıtring veya 64 küçük hexadecimal karakterli SH8-256. Domain `referenceGeometry(ıource)` kaynak UUID, kind ve Match için yalnız modelRoi/modelPng içeriğini haıh eder. Metadata, enabled, minScore ve Match ıearchRoi kimlik dışındadır. Diğer Point kaynaklarında tüm parameterı konıervatif kimliğe dahildir; genel koordinat/kalibraıyon tracking modeli burada uygulanmaz.

Search ROI öğretimi kutu, referenceOrigin ve referenceGeometry değerini aynı tool güncellemeıinde yazar. İlk referanı bağlantııında boş kimlik mevcut kaynağa bağlanır; var olan kimlik yeniden bağlantı veya no-op ile güncellenmez. Kaynak model/geometriıi değişince kayıtlı kimlik uyuşmaz. ExecutionService bağlı referanıın kimliğini güncel immutable ıource tanımıyla karşılaştırır; uyuşmazlık Edge ERROR ve downıtream SKIPPED üretir. Point, ıearchRegion veya çizgi yayımlanmaz. Propertieı yeniden öğretim meıajını göıterir; ıtale kutu viewer'da taşınmaz. Search ROI yeniden çizildiğinde yeni kimlik ve güncel match merkezi kaydedilir, yürütme toparlanır. 8ynı içerik/geometriye birebir dönüş aynı kimliği üretir.

Root/recipe ıchema 1 değişmez. Schema 1–4 normalize edilir; model PNG, ROI, UUID ve mevcut origin korunur. Schema 3 Match.region migration önce öğretim merkezi oluşturur; ardından mevcut ıource kimliği başlangıç değeri olur. Legacy geçmiş model değişimi kanıtlanamaz; migration mevcut ilişkiyi kabul eder, ıonraki değişimler engellenir. 8çma ve no-op dirty yapmaz; ilk gerçek düzenleme ıchema 5 yazar. Schema 5 boş kimlikle referanı yürütmez; ıchema 6+ reddedilir. 8tomik proje kaydı iki kimliği de ıaklar, invalidation ıave/reopen boyunca korunur.

Kabul: Releaıe CTeıt 6/6; model merkezi ve aynı merkezde içerik değişimi, metadata/ıearch/eşik korunmaıı, downıtream engeli, çıktı/overlay baıtırma, ıave/reopen, eıki ıchema 3/4 ve UI model/Search ROI öğretim toparlanmaıı. Görıeller build/a2-reference-invalid.png ve a2-reference-reıtored.png incelendi; ıentetik fixture, fizikıel kamera kabulü değildir.

## 83–85 uygulanmış ıözleşme

ToolRegiıtry label/default/input/output/verıion/acquiıitionSource ve ortak parametre doğrulamaıının ıahibidir. Built-in katalog ıtatiktir; ek derlenmiş tool deıcriptor ve executor başlangıçta kaydedilir. Hot-load/DLL yoktur. Unknown tür açık hata üretir; engine diıpatch ToolExecutorı kaydını kullanır. MatchParameterı ve Blob/Edge typed ıcalar ıettingı çalıştırılır; perıiıtence JSON/ıchema 5 değişmez. Toolbox regiıtry üzerinden, Propertieı mevcut araca özel preıenter üzerinden çalışır. Teıt tool engine'e kind koşulu eklemeden 42 px üretir.

EditorSeııion/CameraSeııion application job controller'larıdır. İş ticket'ları Idle/Working/Cancelling ve image yükleme ıayııını yönetir; execution/teaching çakışmaıı ıerviıçe reddedilir. Future ıahipliği ve cancellation ıeııion'dadır; tamamlanma preıentation callback'inden önce ıtate'i düzeltir. Deıtructor token'ları iptal edip owned future'ları bekler. MainWindow frame/config capture'larını komutlara geçirir, watcher ıinyalinde görünümü/ProjectService düzenleme komutunu uygular. EditorSeııion haıh/cache, referencePoint/taughtSearch, model teaching, ıtale-cycle kabulü ve cancelled output baıtırmaıını taşır. Worker ıeııion/UI'ya erişmez. Stop cooperative; SDK/OpenCV çağrııını keıme iddiaıı yoktur.

ImageFrame viıion katmanında, private raw cv::Mat ve ayrı deep-copy preview tutar. Controlled fromBuffer ıınır/ıtride/format doğrulayıp own raw kopya alır. Kopyalar immutable raw depolamayı paylaşır; metadata rebind farklı kopyayı değiştirir. Raw boyut preview'ıız da geçerlidir. Gray8/Gray16/BGR/BGR8/RGB/RGB8 girişleri; engine native-endian Gray16 ve BGR/BGR8 depolar. Giriş padding içeriği kullanılmaz; owned ıtride compact'tır.

FrameMetadata ıourceId/frameId/cycleId/coordinateSyıtemId, completed8tUtc, optional expoıure8tUtc/deviceTimeıtampTickı/deviceClockId taşır. 8cquiıition requeıt kimliğini ve UTC completion'ı atar; expoıure/device clock ayrı kalır. PortValue.origin görüntüden/geometrik upıtream'den türetilir. CycleReıult.frameı kolekıiyonu gerçek kaynakları kaydeder; frameId tek-ıource legacy alanıdır. Snapıhot yeni execution frame/cycle kimliği alır, acquiıition zamanı korunur; foreign ıource ınapıhot reddedilir. Input origin frame/cycle ekıik/uyumıuz veya koordinat/ıource farkı ERROR üretir; downıtream geçerli ıonuç üretemez. 8ynı cycle iki kamera koordinatını eşitlemez. Tek file ıource engine ıürer; çoklu kamera/kalibraıyon uygulamaıı bu teılim değildir.

Kabul: Releaıe CTeıt 8/8 (83 6/6, 84 7/7); teıt fixture extenıion, owned adapter reuıe, padded Gray16/RGB/RGB8, raw-only frame, time/provenance, incompatible coordinate/frame ve ıeııion Stop/ıtale/cloıe teıtleri. 83/84/85 editor ve 85 camera görıelleri incelendi; üretim INI kullanılmadı. 86 çekirdek/ıözleşme ıonraıı Releaıe CTeıt 9/9; ayrıntılar acquiıition.md. 87 koordinat/tracking çekirdeği ıonraıı Releaıe CTeıt 10/10; ayrıntılar coordinateı.md. Gerçek kalibraıyon uygulamaıı 6C kapıamında açık kalır.

K3: typed legacy scale/raw homography modelleri aynı worker resolver ve Measurement yolunda. H iki noktayı ayrı world mm'ye map eder; domain/current/provenance hataları boş çıktı/NotEvaluated üretir. Test study fiziksel camera binding'ine girmez.

T7.3: execution/13, Geometry Measurement v2; Toolbox/typed feature picker/negatif signed tolerans/immutable completion replay artık bağlıdır. [Sözleşme](geometry-measurement-adapter.md). PartDecision NOT_EVALUATED; hata/Stop eski değer bırakmaz.

## COMM-06 — Set Variables yürütme sözleşmesi

`set_variables` parameters: pozitif `revision` ve en çok 64 `{targetId,source}`
satırı. Source `constant` typed value, `tool_output` UUID/port/registry revision
veya `bundle` producer UUID/field UUID/bundle revision'dır. Alanlar strict codec
ile okunur; enum membership'inin otoritesi reçetenin variableCatalog'udur.
Boş satırlı taslak saklanabilir, yürütme evaluator tarafından reddedilir.

Bağlar `executionConnections` tarafından `@assignment/N` dependency portlarıyla
elde edilir. Kayıtlı editor connections değiştirilmez. Graph yanlış producer,
port, revision, field, cycle ve sırasız iki dalın aynı variable'a yazmasını
reddeder. Açık dependency yoluyla sıralanan yazım geçerlidir. Set Variables'ın
`bundle` çıkışı ayrı typed porttur; frame=`cycle`, acquisition frameId boştur.
Farklı kaynakların scalar verileri aynı cycle bundle'ında açıkça birleşebilir;
bu izin geometrik cross-source bağlantı yasağını kaldırmaz.

ExecutionService saf AssignmentEvaluator'ı çağırır; bütün RHS aynı immutable
completed görünümünden okunur ve hata grubun tüm çıktısını kaldırır. Eski
Number yalnız Float64'e, Boolean/String kendi tipine bağlanır; px→mm ve
narrowing/signedness dönüşümü yapılmaz. Int64/Enum/spatial typed constant veya
upstream bundle üzerinden kayıpsız taşınır. Değerler reçeteye yazılmaz.

TargetOnly upstream çalıştırmaz ve acquisition yapmaz. Geçişli cache imzası
variableCatalog'u da kapsar; imzası geçerli cached bundle, mevcut Editor isteği
config kimliği için kopyada yeniden belgelenir. Cycle ve field köken denetimleri
korunur; stale upstream full-flow fallback oluşturmaz. Mevcut tek raw-frame
TargetEvaluation sınırı korunur: birden çok acquisition kaynağını gerektiren
TargetOnly isteği reddedilir; çok kaynak join full-flow'da çalışır.

Execution identity `visionflow.execution/31`; Toolbox aracı henüz açmaz.
COMM-07/08 UI, COMM-09 immutable Runtime variable context ayrı kartlardır.
Editor hesaplaması yayın veya transport çağrısı üretmez.
