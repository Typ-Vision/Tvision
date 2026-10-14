# Shape Pose — T8.3

10 Ekim T8.4.2b.3a: **Toolbox16 / Pattern Pose ve Shape Pose teaching UI, altı raw tüketicide ortak pose reference picker ve mevcut world Circle teaching** uygulandı. EditorSession gerçek bağlı image/cache snapshot ile worker öğretimi ve Stop/stale suppression; ProjectService binding+connection atomik edit sağlar. Kayıtlı world Edge/Caliper/Line envelope yeniden öğretimi desteklenir; yeni nominal mm authoring ekranı b.3b'dedir. Engine execution/18, registry16, editor7/terminal3 değişmedi. Production pose whitelist reddi sürer; trusted completion/immutable Runtime/Logs/CSV ve world metric/tolerans/current arc henüz kabul edilmedi. [Güncel kullanım ve sınırlar](pose-roi.md).

Alttaki teslim kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

`shape_pose` v1 gerçek raw anchor/açı ve ayrı scale üretir. Programatik teaching, ortak Editor DAG, TargetOnly ve typed codec teslimidir. T8.4.1 ile [ortak PoseRoiService](pose-roi.md) shape referansını da method/geometry kimliğiyle kabul eder. Özel Toolbox/Properties/Viewer öğretimi ve production Runtime/trusted completion/Logs/CSV T8.4.2 kapsamında açıktır. Toolbox14, registry16; `visionflow.execution/15`, editor7/terminal3/vf.socket/2. Production whitelist hem pattern_pose hem shape_pose reddeder. Genel part decision NOT_EVALUATED; robot pose veya fiziksel hassasiyet iddiası yoktur.

## Model ve kullanım

`ShapeMatchService::teach(frame, rectangle, modelLocalAnchor, poseMatchDefaults())` Gray8 owned görüntüden model PNG/modelRoi/anchor parametrelerini üretir. Rectangle görüntü içinde ve model16–256 px/eksen; anchor model-local pixel-centre, subpixel ve merkez dışı olabilir. En az16 iç gradient destek pikseli gerekir. Model PNG ortak bounded MatchService codec ile tutulur; derived gradient model her execution sırasında aynı sabit yöntemle yeniden çıkarılır. Gizli global model/cache yoktur. Parametrelerin strict ortak pose doğrulaması, unknown/missing/NaN/grid bütçe retleri uygulanır. Tek required `image` portu vardır; renk/16bit için explicit Preprocess gerekir.

Yeni ToolDefinition kind `shape_pose`, parameters öğretimin dönüşü, connections source.image → shape.image. Üç applicationType aynı executor'ı kullanır. `PoseMatchService` ortak typed sonuç adapter'ıdır; ShapeMatchService shape kind/teaching model kapısını sunar. UI algoritma çağırmaz. Anchor/model/parametre değişimi mevcut definition signature/cache invalidation yolunu kullanır. Save/reopen/no-op ortak editor7 schema'dır; yeni schema/model migration yoktur. Legacy Template Match'ten angle=0 pose türetilmez.

## Algoritma

Gray8 → isolated ROI Gaussian3×3/sigma0.8 → float Sobel dx/dy. Gradient magnitude/4 saturate Gray8 yalnız proposal görüntüsüdür. Magnitude≥60 (ham Sobel birimi) olan iç pikseller gradient support'tur; crop kenarından iki piksel hariçtir. Eşit gradient sırtlarının iki tarafı da korunur. Canny'nin tek taraflı ridge tie-break davranışı 90°/180° simetride konum/skor bias ürettiğinden nihai model desteği için kullanılmaz. Gerçek kontur crop içine sığmalıdır; crop sınırından obje konturu türetilmez.

Sobel normal açısı unsigned moduloπ; kontrast tersleme destek/yönü korur, signed polarity ayrımı sunulmaz. Sekiz orientation bin'i, nearest normal bin; dönen model normaline raw pose açısı eklenir. Model support row-major deterministik eşit indeks örneklemesiyle en çok256 noktaya indirilir. Bütün güçlü gradientler sahne distance map'lerinde kullanılır.

Adaylar mevcut PoseMatcher bounded pyramid mekanizmasıyla **gradient magnitude** üzerinde aranır; raw texture NCC kullanılmaz. Pyramid1–3, angle−180…180°/step1…90°, scale0.5…2/step0.02…0.5; en çok37×17 ve toplam255 transform. Sekiz peak/transform,128 ara refinement seed; en çok32 gradient proposal nihai chamfer'a gelir. Proposal NCC score discard edilir. Bu recall/global optimum veya bütün nesneleri bulma garantisi değildir; gradient proposal listesinde elenen aday chamfer ile kurtarılamaz.

Her orientation bin için exact L2 distanceTransform,6 px cap. Model tap'ı rigid angle + ayrı scale ile gerçek local anchor çevresinde taşınır. Distance map bilinear örneklenir. Tap maliyeti:

`min(1, min_b(distance_b/6 + 0.5 * circularBinDistance(expected,b)/4))`

Nihai `score = 1 - mean(tapCost)`;0–1 aralığında unsigned oriented chamfer similarity. Olasılık, görünür coverage, parça sayımı, fiziksel doğruluk veya texture NCC ile karşılaştırılabilir confidence değildir. Kalın gradient support küçük geometri farklarını örtebilir; sample bütçesi ince detayları kaçırabilir. Bir scene kenarı birden çok model tap'ını destekleyebilir; one-to-one contour assignment değildir. Eksik support maliyete katılır, occlusion için kalan görünür tap'lara normalize edilmez. Ekstra sahne kenarları ayrıca cezalandırılmaz; clutter/yanlış benzer şekil kabulü gerçek corpus ile incelenmelidir.

Dört bounded coordinate-descent pass: translation1/0.5/0.25/0.125 px; angle step/2, /4, /8, /16; scale step/2, /4, /8, /16. Her proposal search sınırları içinde olmalıdır. Score tam raw gradient alanından tekrar hesaplanır; OpenCV warp kullanılan proposal interpolation score'u değildir. Model dört köşesinin tam transformed footprint'i search ROI içinde olmalıdır; clipping/AABB ölçümü yoktur. Image≤16Mi piksel; büyük görüntüde sekiz float distance map ve gradient buffers yüksek bellek kullanır. CPU latency/bellek hedefi doğrulanmadı; tight ROI önerilir.

Final NMS anchor distance ve clamp(1.5×angleStep,12°,20°) wrap-around açı kapısıdır; scale aynı hipotezin parçası. minScore filtresinden geçen candidate listesinin maxCandidates(2–32) sınırını aşması `truncated=true`; liste toplam nesne sayısı değildir. `truncated` yalnız final listeye aittir, proposal recall kaybını belgelemez. İlk iki final skor farkı≤ambiguityGap veya final truncation → ambiguity. Varsayılan reject_ambiguous pose seçmez; explicit best ambiguity kanıtını korur. Simetrik şekil açısı fiziksel parça kimliği kanıtı değildir.

İptal aşamalar/gradient satırları/candidate refinement arasında denetlenir; OpenCV çağrısı zorla kesilmez. Hata/iptal kısmi çıktı döndürmez. Raw görüntü değiştirilmez. GeneralizedHoughGuil karşılaştırması opsiyoneldir, bu backend'de uygulanmadı. matchShapes pose detector olarak kullanılmadı; HALCON eşdeğerliği iddiası yoktur.

## Sonuç ve sınırlar

`pose` typed Pose2D raw Y-down clockwise radyan; `scale` ratio, `score` ratio. `found`, `ambiguous`, `truncated` valid Boolean; bounded `candidates` Text JSON listesinde x/y/angleRadians/scale/score. Bütün portlarda method **unsigned-oriented-chamfer-pyramid**, version1 ve actual image source/frame/cycle/coordinate/time kökeni; engine evaluation identity mevcut ortak yolca atanır. Texture method masked-ccoeff-normed-pyramid v1 olarak kalır. Yöntemlerin eşikleri ayrı yorumlanmalıdır.

Seçim yoksa pose/scale/score invalid ve overlay boş; sahte sıfır/PASS üretilmez. DONE/not-found, execution hata ve part decision ayrı kalır. T8.4.1 ortak ROI adapter shape/texture method validation ve kind dahil binding kimliğini destekler; bütün araçların reference picker/teaching invalidation'ı ve DAG/Runtime bağlantısı T8.4.2 işidir. Shape pose doğrudan world angle veya robot pose değildir.

## Yazılım kabulü

`shape_match_contract`:104 kontrol. Ürün warp'ından bağımsız inverse rasterizer;−27.5/0/+27.5°,0.85/1/1.15 scale, subpixel/noncentral anchor, absolute ROI ve pyramid1/2/3. Oracle1.5px/.05rad/.045scale yazılım fixture kapısıdır. Kontrast tersleme ve raw texture NCC negatif kontrolü, affine aydınlık, ayrı clutter, explicit .75 threshold ile sınırlı partial occlusion/gerçek score düşüşü, yanlış contour/açı retleri. 90°/180° simetri, repeat/explicit best/final truncation, blank/flat/NaN/range/cancel retleri. ROI dışı piksel değişiminde exact aynı pose/score, Gray8/model-size/NaN anchor retleri. Typed method/frame/evaluation/codec, invalid seçim boş metrik/overlay, üç applicationType PNG→DAG, Save/reopen/no-op, TargetOnly acquisition0/raw hash korunması, anchor edit ve Stop. Runtime unsupported-tool reddi doğrulanır.

Testlerin sentetik kapsamı genel occlusion dayanıklılığı/corpus precision-recall, fiziksel metrologi, native DPI, kamera/PLC/robot, lens/CUDA/kurulum/dağıtım kabulü değildir. [Gerçek koşum sonuçları ve sonraki adım](../progress.md).
