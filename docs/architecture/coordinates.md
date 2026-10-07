# A7 koordinat ve tracking ROI sözleşmesi

7 Ekim 2026. Uygulanan çekirdek `src/domain/Coordinates.hpp/.cpp` içindedir; Qt Core değer tipleri kullanır, OpenCV/Widgets/SDK çağırmaz. Mevcut Edge point-reference takibi bu çekirdeğe bağlıdır. Pose2D ve dönmüş dört köşeli ROI domain kabulüdür; Match açı bulma, yeni Pose portu, dönmüş ROI raster işleme, kalibrasyon hesabı veya robot çıktısı teslimi değildir.

## Kimlik, birim ve eksen

`CoordinateFrame` geometrik sistemin kalıcı ID'si, `Pixel`/`Millimetre` birimi ve `XRightYDown`/`XRightYUp` eksen konvansiyonudur. Kimlik boş olamaz. Aynı ID tek başına uyumluluk sağlamaz; bütün descriptor eşit olmalıdır. Yerel parça sistemi hedef sistemden farklı ID taşır.

ImageFrame.metadata.coordinateSystemId bu geometrik kimliği belirtir. Güncel dosya edinimi `<source UUID>/image` kullanır; engine image portları px ve XRightYDown kabul eder. Acquisition `frameId` tek görüntü örneği, `cycleId` execution çevrimi, `sourceId` edinim kaynağıdır. Bunlar geometrik ID yerine kullanılamaz. Engine aynı cycle'daki farklı frame/source/geometrik ID'leri otomatik birleştirmez. Gelecekte açık dönüşüm tool'u bu provenance ilişkisini ayrıca taşımalıdır.

Image'de sol üst piksel merkezi (0,0), +X sağa, +Y aşağıdır. Son piksel merkezi (width−1,height−1); piksel hücresinin sınırları merkezden ±0.5'tir. QRect(x,y,w,h) merkez indeksi aralığını x…x+w−1 ve y…y+h−1 olarak tutar; merkezi (x+(w−1)/2,y+(h−1)/2)'dir. Genel TrackingRoi köşeleri kesintisiz geometrik koordinatlardır. Bir QRect'in kapsadığı piksel alanını poligona çevirmek gerekirse köşeler (x−0.5,y−0.5)…(x+w−0.5,y+h−0.5) olur; bu raster dönüşümü henüz uygulanmadı. Viewer zoom/pan veya preview ölçeği bu koordinatları değiştirmez.

## Pose ve dönüşüm yönü

Pose2D = position X/Y + angleRadians + hedef CoordinateFrame. Yalnız düzlemsel konum/yönelim taşır; acquisition zamanı, validity veya 3D Z/Rx/Ry üretmez. Kullanıcı/algoritma geçerli sonuç sağlamadan pose yayımlanamaz. Point çıktısı yönelim ölçümü değildir; legacy öteleme adapter'ındaki sıfır açı yalnız öteleme varsayımıdır.

Transform2D `T_target_source` yönündedir:

`p_target = R(angleRadians) * p_source + translation`

`R = [[cos(a), -sin(a)], [sin(a), cos(a)]]`

Translation hedef eksenlerindedir. Pozitif açı +X'i +Y'ye döndürür: Y-down görüntüde saat yönü, Y-up düzlemde saat yönünün tersi. Açılar radyandır; trig/composition için 2π modunda normalize edilir, sonuç −π…π aralığındadır. NaN/Inf koordinat/açı ve hesap taşması reddedilir.

`A.then(B)` önce A, sonra B uygular; matris sırası B·A'dır. A.target ve B.source descriptor'ları birebir eşleşir. `inverse()` kaynak/hedefi değiştirir ve offset'i ters döndürür. Pose mapping hem konumu hem açıyı dönüştürür. `poseTransform(reference, localFrame)` lokal eksenlerden reference.frame'e dönüşümdür; lokal offset önce reference açısıyla döner, sonra konuma eklenir.

Rigid Transform2D aynı birim ve aynı eksen konvansiyonu gerektirir. px→mm scale/homography, distortion veya Y-down→Y-up yansıması bu API'nin işi değildir; 6C/6E'de açık, doğrulanmış kalibrasyon dönüşümü gerekir. Rigid dönüşüme örtülü unit scale veya axis flip eklenmez.

Örnek: local (2,3), açı π/2, translation (10,20) → image (7,22). Sonra (5,−4) ötelemesi → (12,18). İkinci dönüşüm π/2 ise ilk translation da döner; sonuç (−22,7). Ters dönüşüm başlangıç noktasını geri verir. Bu değerler `coordinates_contract` kabulündedir.

## Araçtan bağımsız tracking ROI

TrackingRoi öğretimi, aynı reference.frame'deki dört sıralı köşeyi reference pose'un ters dönüşümüyle localFrame'e alır. Kayıt localFrame, referenceFrame ve localCorners tutar. Yeni pose ile aynı köşeler lokalden hedefe dönüştürülür. Sadece ROI merkezi değil, offset ve bütün köşeler birlikte döner. Dört köşe, sonlu değer ve kesin konveks sıra zorunludur; boş/dejenere/kesişen poligon reddedilir. Kayıtlar public değer tipidir; resolve tekrar doğrular.

Örnek: reference=(10,20,0), ROI=(12,20),(16,20),(16,22),(12,22). Yeni reference=(30,40,π/2) → (30,42),(30,46),(28,46),(28,42). Döndürülmüş teaching pose da ters dönüşüm/resolve ile aynı köşeleri geri verir. Farklı referenceFrame veya birim sessizce kabul edilmez.

Çekirdek döndürülmüş ROI'yi dört köşe olarak verir. Axis-aligned bounding box'a düşürmek arama alanını değiştirir; bu otomatik yapılmaz. Mevcut Edge/Blob/Match crop yürütücüleri yalnız QRect kabul eder. Bu tools için döndürülmüş sampling, scan yönü ve boundary policy sonraki özellik kabulünü gerektirir.

Genel TrackingRoi resolver görüntü boyutu almaz; dört köşenin görüntü içinde olup olmadığı tüketicinin sınır politikasıdır. Güncel legacy Edge adapter'ı görüntü boyutunu alır ve sınır dışını açıkça reddeder.

## Legacy Edge ve migration

Editor şeması **5** ve root/recipe şeması **1** korunur. Yeni kayıt alanı veya dirty-on-open yoktur. Schema 3 Match.region→Edge.region migration point anchor'a dönüşür; schema 4 origin ve schema 5 referenceGeometry baseline/invalidation kuralları korunur. Pose/TrackingRoi genel tipleri henüz editor JSON'una yazılmaz.

`translationTrackingRoi` referenceOrigin, güncel point ve öğretilmiş QRect'i ortak poseTransform/inverse/then üzerinden çözer. Öğretim/current sıfır açılıdır; fark bir kez en yakın tamsayıya yuvarlanır (yarımlar sıfırdan uzağa), width/height aynen korunur. Öğretim/current ayrı ayrı yuvarlanmaz. Boş kutu veya görüntü dışına çıkan çözüm ERROR olur, crop/clamp/fallback yapılmaz. Eski teaching kutusu güncel farklı görüntünün sınırlarında olmak zorunda değildir; çözülen güncel kutu sınır içinde olmalıdır.

Engine tüketimden önce gerçek PortValue type/unit/frame descriptor'ını beklenen input ile karşılaştırır. Tracking adapter ayrıca point/px/image ve frame/cycle/source/geometrik kimlik eşitliğini kontrol eder. Eski cycle/not-found sonucu SKIPPED; model kimliği değişimi ERROR ve downstream SKIPPED kuralları sürer. Testler taşınmış Edge konumunu, mesafe invariant'ını, sınır dışını, model invalidation, schema 3/4 migration ve save/reopen'ı kapsar.

## Kalibrasyon bağımlılık kaydı

CalibrationDependency: calibrationId, pozitif version, contentId, cameraId, lensId, resolution, workingPlaneId ve source/target CoordinateFrame. ContentId boş olmayan içerik kimliğidir; gerçek kalibrasyon servisi 6C'de güvenilir içerik hash'ini üretir. `requireCurrent` tüm alanları doğrular ve birebir karşılaştırır. ID/sürüm/içerik/kamera/lens/çözünürlük/düzlem/frame/eksen/birim değişimi eski bağımlılığı reddeder.

`toJson/fromJson` bağımsız schemaVersion=1 kayıt sözleşmesidir; units px/mm, axes x-right-y-down/x-right-y-up. Eksik/ekstra alan, future schema, yanlış JSON tipi, kesirli sürüm/boyut ve bilinmeyen birim reddedilir. Atomik proje kaydına entegrasyon, kalibrasyon katsayıları, validation evidence ve execution result invalidation **6C** işidir. Bu kayıt tek başına geçerli kalibrasyon veya px→mm dönüşümü kanıtlamaz. Robot frame/TCP/pick reference ve gerçek robot dönüşümü **6E**'dedir; 2D veriden tam 3D pose türetilmez.

## Kabul ve sınırlar

Release build ve CTest 10/10: yeni coordinates_contract bilinen öteleme/dönme/sıra/inverse, Y-up mm, lokal offset/dört köşe, nonzero teaching, frame/unit/axis/NaN/Inf/dejenere/boundary ve kalibrasyon dependency JSON/invalidity kabulünü sınar. owned_frame_contract artık çalışma sırasında yanlış unit/type/port-frame üreten extension sonucunun ölçüm üretmesini de engeller. tool_execution ve ui_shell_smoke mevcut teaching/migration/reopen/regression davranışlarını korur.

build/a7-reference-edge.png ve build/a7-editor.png güncel offscreen smoke çıktılarından alınıp incelendi. UI kaynakları, menüler, tema ve panel düzeni değişmedi. Fixture'lar sentetiktir; fiziksel kamera, native DPI, kurulum ve CUDA kabulü yoktur. Match açı/ölçek tespiti, Pose portu, robot/kalibrasyon yürütmesi veya Runtime uygulanmış özellik olarak sunulmaz.
