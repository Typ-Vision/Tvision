# T7.2/T7.3 — Geometry Measurement, UI ve Runtime

`geometry_measurement` v2, `GeometryMeasurementConfig` ve application `GeometryMeasurementService`, [T7.1 domain matematiğini](geometry-measurements.md) ortak executor/typed DAG yoluna bağlar. C++20/Qt/OpenCV mevcut servisleri kullanılır. Üç applicationType aynı editor definition ve ExecutionService'i kullanır. Varsayılan `calibrated`; `pixel` yalnız diagnostic px/açı verir. PartDecision daima NOT_EVALUATED; isteğe bağlı accepted Boolean yalnız bu tool toleransını belirtir.

T7.3 ile Toolbox ve ortak Editor/Runtime açıldı. Legacy Calculate px ve Measurement mm/PASS/FAIL sözleşmesi korunur. Geometry genel parça kararı veya PLC result mapping üretmez; vf.socket/2 mevcut Measurement yolunu kullanır.

## Kullanım ve tolerans

Toolbox → Geometry Measurement → operation/mode seçin → image ve yalnız gerekli raw feature girişlerini bağlayın. Line/Circle feature seçilince calibrated modda aynı producer observationsPx otomatik bağlanır; samples alanı salt okunur görünür. Tip/birim/frame uyuşmayan port listelenmez; eksik giriş açıklaması ve INVALID eski bağlantı görünür. UI pixel Line/Circle üreticilerini listeler; programatik Line Fit raw outputs yolu korunur. Operation/mode değişimi image dışındaki bağlantıları temizler; yeni işlem için açık seçim gerekir. Refit ayarları açılır paneldedir; eşikler px veya mm modundadır.

`toleranceEnabled` varsayılan false; `nominal/lower/upper` finite −1e9…1e9, lower ≤ nominal ≤ upper. Nominal referans değerdir, ölçümden çıkarılmaz. Signed negatif aralık geçerlidir. Length toleransı seçili mod px/mm, line_angle toleransı derece; raw double için lower ≤ measured ≤ upper karşılaştırılır, ekran yuvarlaması kullanılmaz. Intersection scalar değildir; tolerans devre dışıdır. Tolerans kapalı veya scalar yoksa accepted invalid/NOT_EVALUATED. Pixel accepted diagnostic'tir. Eski T7.2 dört tolerans alanı olmayan parametreler aynen kabul edilir; no-op aç/kaydet implicit tolerans eklemez.

Run Flow yeni acquisition, Run Tool aynı ham karede yalnız hedef ve doğrulanmış upstream cache kullanır. Noktalar/merkezler arası yardımcı segment, dik ayak veya clamped segment ayağı ve measured line support raw inverse overlay olarak gösterilir. Yönlü açı +X→+Y; Y-down clockwise, Y-up CCW. Pixel sonsuz çizgi ve calibrated support/hull sınırları aşağıdadır.

Save → Load Recipe → Run → Tek çevrim ortak değişmez snapshot yolunu kullanır. Stop eski metric/accepted/helper bırakmaz. Logs gerçek typed değer/birim/diagnostic/method/invalid nedeni ve CSV'yi tutar; restart aktif çalışma iznini geri getirmez.

## Completion doğrulaması ve kayıt sınırı

ProductionRecipeService snapshot bağlantılarından aynı çevrimin DONE upstream portlarını toplar; actual owned ImageFrame, tüm provenance kontrolleri ve güvenilen current calibration resolver ile GeometryMeasurementService'i yeniden çalıştırır. Tüm output envelope/payload/validity/invalidReason/diagnostic/method/version, exact double, calibration dependency ve helper noktaları/çizgileri eşit olmalıdır. Eksik/fazla port veya değiştirilmiş accepted/metric/status/frame/method/overlay ret edilir. Kalibre completion resolver yoksa varsayılan ret; UI kendi algoritmasını veya SDK'yı çağırmaz. Tolerans için editor draft kullanılmaz.

Bu canlı completion iç tutarlılık replay'idir; acquisition bağımsız doğruluk/imza değildir. Görüntüler arşivlenmez; restart'ta matematik görüntüden tekrar çalıştırılmaz. Atomik/hash kontrollü archive + bounded typed envelope/payload replay ve CSV mevcut terminal3 yoludur. Koordine upstream+sonuç sahteciliğine karşı kriptografik güven garantisi yoktur.


## Sabit typed portlar ve işleme göre zorunluluk

Her işlem `image: Image px/image` ister: bu, actual acquired frame/provenance/resolution denetimini sağlar. Aşağıdaki feature portlarının hepsi **raw px/image** sözleşmesindedir; world feature'a px etiketi takarak bağlantı yapılamaz. `p1/p2` Point, `line1/line2` Line2D, `circle1/circle2` Circle2D, `samples1/samples2` PointSet. Statik port tip/birim/frame değişmez; tüm feature portları deklarasyonda optional olup kesin operation/mode kümesi DAG'de ve application'da ayrıca doğrulanır.

| operation | image dışında gerekli feature portları | calibrated modda ek |
| --- | --- | --- |
| point_distance, x_offset, y_offset | p1, p2 | yok |
| point_line_distance, point_segment_distance | p1, line1 | samples1 |
| line_angle, line_intersection, parallel_distance | line1, line2 | samples1, samples2 |
| circle_centre_distance | circle1, circle2 | samples1, samples2 |
| circle_radius, circle_diameter | circle1 | samples1 |

Eksik **ve fazla/işlemde kullanılmayan** bağlantı reddedilir. Calibrated feature ve ona ait samples aynı sourceTool'dan gelmelidir; application ayrıca method/version/evaluation eşitliğini ister. İki farklı feature'ın evaluationId'si eşit olmak zorunda değildir: aynı acquisition/frame/cycle üzerinde ayrı Run Tool geçerlidir. Her companion feature/sample çifti kendi değerlendirmesine ait olmalıdır. Derlenmiş custom producer'ın doğru feature/sample ilişkisi sağlama sorumluluğu vardır; bu bir görüntüden bağımsız doğruluk veya dış saldırgana karşı imza değildir.

Mevcut raw producer bağlantıları:

- Edge/Edge Point/Edge Pair Point portları → p1/p2.
- Line Fit.linePx → line1/line2; **Line Fit v2 observationsPx** → ilgili samples portu. Found raw scan noktaları sırayla, fit outlier'ları dahil taşınır; missing slotlar PointSet'te yoktur. Yeni port calibrated Line Fit'te de ham gözlemleri taşır.
- Pixel Circle.circlePx + observationsPx → circleN/samplesN. Circle'ın calibrated modunda bu Px portları invalid'dir; world circleMm doğrudan bu raw adapter girdisine bağlanmaz. Raw circle seed, homography'de gerçek fiziksel dairenin fit'i olarak kullanılmaz; raw görüntüdeki eliptik destek varsayılan pixel Circle gate'lerini geçemeyebilir. Böyle kaynaklarda raw seed üretiminin kendi fit kabulü ayrıca sağlanmalıdır. Feature picker yardım metni bu raw/world ayrımını açıklar.

Arc2D veya serbest polygon girdisi bu sürümde yoktur. Circle2D girdisi kısmi yaydan fit edilmiş olabilir; adapter nominal angular slot/full-circle coverage veya circularity üretmez.

## Kalibrasyon ve refit

Her girdi valid, beklenen typed payload ve aynı actual image source/coordinate ID/frame/cycle/completedAt/exposure/device clock metadata'sına sahip olmalıdır. Pixel typed geometri kendi raw frame'ini taşır. Yanlış unit/frame/axes/cycle/evaluation veya NaN/Inf reddedilir. Raw noktalar actual image bounds içinde olmalıdır.

Calibrated modda resolver rawSize actual image.size ile aynı olmalıdır. Accepted PlanarCalibration ve independently resolved current dependency exact version/content/installation/source/target/axes/resolution eşleşmesi gerekir. File-test calibration reddedilir. Point mapping mevcut `CircleToolService::mapPoint` → MeasurementService/PlaneMappingService kabul/current/provenance/hull kapılarını yeniden kullanır; ikinci calibration modeli yoktur. Uniform/manual scale ve Y-up desteklenir; non-affine raw homography her noktayı ayrı map eder.

- Point işlemleri: her uç önce world mm'ye map edilir, sonra domain hesabı yapılır.
- Line işlemleri: **bütün samples** world'e map edilir; mevcut LineFitter ile membership/support/geometri world'de yeniden hesaplanır. Raw Line2D uçları yalnız mapped yön ve measured support guard için kullanılır. Piksel açı taşınmaz veya ölçeklenmez.
- Circle işlemleri: bütün samples world'e map edilir; CircleFitter yeniden merkez/yarıçap hesaplar. Raw centre/radius sonuç hesabına katılmaz. `map(rawCentre)` veya `radiusPx*scale` yolu yoktur.

Found sample başına map, iptal kontrolü ve bounds/hull kapısı çalışır; outlier olsa da alan dışı sample sessizce çıkarılmaz. Sample sayısı fitMinPoints–128; default6. RANSAC default true, native core bounded iteration/Stop sözleşmeleri korunur. Fit inlier coverage sabit.6; Line spanCoverage.5; bunlar supplied found observations üzerindedir, missing scan/full contour kabulü değildir. Default mm inlierDistance.1, RMS.035, maxResidual.1, line minSpan.1/minVarianceRatio25, circle minAngularSpan90°/maxCondition1000; finite bounded schema doğrulanır. Robust fit outlier'ları fiziksel kusur kabulünden çıkarmak anlamına gelmez; bu adapter kusur metriği üretmez.

## Türetilmiş geometri alanı

World fit line support uçları, gerçek fitted circle centre ve tüm point uçları raw'a inverse-map edilerek tekrar domain guard'dan geçer. `point_line_distance` dik ayağı measured line support içinde olmalıdır; calibrated infinite-line extrapolation reddedilir. `point_segment_distance` uçlara clamp eder ve dıştaki nokta için geçerli uç mesafesi hesaplayabilir. `parallel_distance` ikinci support start'ının ilk support'a dik ayağını denetler. Unique line intersection her iki measured support içinde ve kalibrasyon alanında olmalıdır. Support üyeliği ham projeksiyonda dahil [0,length] ile değerlendirilir; ek fiziksel tolerans/clamp yoktur, çok yakın sınırda double/solver roundoff konservatif ret verebilir.

Circle centre ve supplied observed sample domain denetimi, görülmeyen bütün çevrenin hull içinde olduğunu kanıtlamaz. Circle size bu örneklerden fit edilen radius/diameter'dır; tam çevre/ISO/GD&T/uncertainty iddiası değildir. T7.1 paralellik/coincidence sayısal epsilon'ları korunur. Pixel modda sonsuz-line matematiği kullanılır, gösterilecek türetilmiş noktalar yine raw image bounds içinde olmalıdır.

## Sabit çıktılar ve hata

`valuePx/valueMm` Number px/mm, `anglePx/angleMm` Number deg, `intersectionPx/intersectionMm` Point; yalnız işlem ve modun ilgili portu valid olur. `status` Text: MEASURED/PARALLEL/COINCIDENT. Paralel/çakışık line intersection DONE + invalid intersection verir; nokta uydurmaz. Nonparallel spacing, alan/support dışı türetilmiş nokta, stale binding veya fit failure hata verir. Engine ERROR'da tüm output/overlay/calibration'ı temizler; upstream invalid ise SKIPPED, Stop'ta eski değer kalmaz. Pixel çıktılar diagnostic; calibrated sonuç exact calibration dependency taşır. `accepted` Boolean plane portu yalnız scalar ve etkin toleransla valid olur. Output metod kimliği `geometry-refit/<operation>` v2; typed evaluation engine tarafından atanır.

## Sürüm ve kabul

Engine **visionflow.execution/13**, Line Fit **v2** (observationsPx portu), geometry_measurement v2. Editor7, root/recipe schema ve terminal3/vf.socket/2 korunur. Eski dosyada no-op save byte/revision korunması sürer; engine/tool validation/cache eski identity ile çalışmaz. Recipe parametrelerine fit örnekleri gömülmez; upstream typed sonuç olarak edinilir.

`geometry_measurement_service_contract`: uniform/manual/Y-up, tüm işlemler, px/mm ayrımı, stale/source/frame/cycle/time/unit/evaluation/fit/Stop retleri; independent non-affine projected circle/line, mapped raw-centre negatif kontrolü ve actual fitted centre hull dışı gerekçeli ret. Gerçek raster Edge Point/Line → 100px/10mm, Circle → yaklaşık8mm, Save/reopen/no-op, TargetOnly acquisition0/raw hash, upstream cache invalidation, missing→SKIPPED; gerçek Runtime exact replay/tamper, iki delik merkezi 9.6mm, dik raster kenarları 90°, Logs/restart/CSV ve Stop testleri vardır. `ui_geometry_smoke` gerçek Toolbox/Properties/raw feature/sample seçimi, signed −10mm, açı derece, dik helper, Save/reopen/TargetOnly/Load/Run/Stop ve görselleri denetler. [İlerleme ve gerçek test sonuçları](../progress.md). Test toleransları yazılım oracle'larıdır; fiziksel K4/lens/kurulum/dağıtım/CUDA kabulü değildir.
