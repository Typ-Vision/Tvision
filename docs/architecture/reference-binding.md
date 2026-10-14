# R1 — Ortak Referans domain sözleşmesi

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](reference-production-acceptance.md). [R8 paket ve devir kanıtı](reference-release-acceptance.md).


11 Ekim 2026. `ReferenceBinding` domain matematiği ve bağımsız strict codec sunar. R2 ile editor8 reçete parametresi/registry/DAG, R3 ile programatik Editor/cache/raw execution bağlandı; [uygulama sözleşmesi](reference-integration.md). World sampling/ofset ve UI R4/R5 ile bağlandı; birleşik Editor kabulü R6, production Runtime R7/R8 kapsamındadır. Bu kayıt tek başına kaynak sonucunun güvenilirliğini veya production iznini kanıtlamaz.

`ReferenceAnchor` Point2D veya Pose2D variant'ıdır. `translation` yalnız Point2D, `rigid` yalnız Pose2D kabul eder. Translation açısı sıfır olmak zorundadır; güncel/taught tip, frame, raw px/Y-down, çözünürlük ve anchor sınırları denetlenir. Scale tam 1 olmalıdır. Raw nominal ROI hiçbir zaman ofsetli sonuca dönüştürülerek saklanmaz. Yeniden öğretim çağrısı temel, ofsetsiz ROI ile yapılmalıdır; R3 servisi mevcut ofsetsiz nominal çizimi ve açık verilen ofsetleri kullanır; UI R5'tedir.

Point: `q' = q + (p-p0) + d`. Pose: `q' = p + R(a) * (R(da) * R(-a0) * (q-p0) + d)`. X/Y Point'te görüntü, Pose'da kaynak yerel eksenlerindedir. Açı radyandır; X/Y vektörü açı ofsetiyle ayrıca dönmez. `rawTransform` ileri world adapter için raw dönüşümü sağlar; R4 world adapter bu dönüşümü her gerçek sampling tap üzerinde calibration alan denetimiyle kullanır. `resolve` altı ToolRoi şeklini gerçek geometrisiyle taşır; scan genişliği, kutu boyutu, halka yarıçapları ve yay signed sweep korunur. Footprint dışı sonuç reddedilir, clamp/AABB/fallback yoktur.

Bağımsız `schemaVersion=1`, `mapping=raw-reference-v1` kaydı:

| Alan | Anlam |
| --- | --- |
| mode | translation / rigid |
| sourceTool, sourcePort | Kalıcı kaynak tool/port kimlikleri; capability doğrulaması R2 |
| geometryId | Kaynağın geometrik anlam kimliği, lowercase SHA256; üretim/doğrulama R3 |
| imageSource, width, height | Kaynak ve en fazla 16 MP raw çözünürlük |
| nominal, roiIdentity | Ofsetsiz raw ToolRoi ve compact JSON SHA256; codec hash uyuşmazlığını reddeder |
| taught | Translation için yalnız x/y (frame nominal'den); rigid için strict ToolValues Pose2D |
| offset | x/y, angleRadians, unit=px, axes=image/source-local; moda göre sabit semantik |
| calibration | null veya mevcut CalibrationDependency; raw frame/çözünürlük eşitliği gerekir |

Root ve iç kayıt alanları kesin, JSON en fazla 160 KiB; yanlış tip, bilinmeyen/eksik alan, gelecek sürüm, bozuk hash, NaN/Inf ve aşırı ROI/ofset reddedilir. Finite açılar mevcut radyan wrap sözleşmesiyle normalize edilir. Calibration varlığı current/validated kabulü değildir; application resolver ayrıca doğrulamalıdır.

Eski PoseRoi / WorldPoseTracking / Edge codec'leri genişletilmedi. Ortak geometri mapper PoseRoi'den paylaşıldı. R1 sırasında Editor7/execution19 korunmuştu; R2 yeni parametre için opt-in editor8, R3 davranış için execution20 tahsis etti. Tool ve terminal/socket sürümleri değişmedi. Eski kayıtları salt açma/no-op Save ile yeni biçime geçirme yoktur.

Kabul: `reference_binding_contract` bağımsız koordinat oracle'ları, 0/±90/wrap, subpixel, altı şekil, yeniden kullanımda tek ofset, tip/frame/scale/bounds ve strict codec retlerini test eder. Eski reçete/DAG/world regresyonları ayrıca çalıştırılır; UI ve fiziksel kabul sayılmaz.

R4 eklemesi: `world=true` kayıtları **schemaVersion=2 / mapping=world-reference-v1** ve ek zorunlu finite `mmPerSample` alanıyla yazılır. `nominal` exact nominal mm ROI'dir; `calibration` zorunludur ve nominal frame hedef düzlemle, rawSize resolution ile eşittir. Taught Point frame calibration.source'tur; Pose aynı raw frame'i taşır. `rawTransform` raw düzlemde kalır; `resolve` world kaydını reddeder, per-tap application mapper gerekir. Raw schema1 çıktısı değişmedi; schema1'e world alanı veya schema2'ye eksik/extra alan eklemek reddedilir. Yeni kayıt yalnız editor8 içinde kabul edilir. Engine execution21; eski WorldPoseTracking schema2 bağımsız codec olarak değişmeden kalır.
