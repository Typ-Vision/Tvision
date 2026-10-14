# T7.1 — Geometrik domain matematiği

T7.3 güncellemesi: Geometry Measurement v2 Toolbox/typed feature/tolerans/Runtime exact completion replay tamamlandı; engine execution/13. [Uygulanan kullanım ve sınırlar](geometry-measurement-adapter.md). Aşağıdaki T7.2 UI/Runtime erteleme ifadeleri tarihsel kapsama aittir.

`src/domain/GeometryMeasurements.*` mevcut `Point2D`, `Line2D`, `Circle2D` ve `CoordinateFrame` tiplerini kullanır. Qt Core dışında algoritma bağımlılığı yoktur; OpenCV, görüntü edinimi, calibration resolver, UI, registry, recipe/codec veya Runtime bağlantısı eklenmez. Bu paragraf yalnız T7.1 domain sınıfının kapsamıdır. [T7.2 application adapter](geometry-measurement-adapter.md) ayrıca tamamlandı; T7.2 sırasında ürün engine execution/12 oldu; T7.3 UI/Runtime teslimi yukarıda günceldir; legacy Calculate/Measurement korunur.

## Birim ve geçerlilik

İkili işlemler aynı frame ID, LengthUnit ve AxisConvention ister. Frame boş veya enum geçersizse, koordinat/radius NaN/Inf ise, radius pozitif değilse ya da ara/sonuç double'da sonlu değilse `std::runtime_error` atılır. Geçersiz geometri sıfır ölçüm veya PASS olmaz. Sayısal sonuçlar caller frame'inin px/mm birimindedir; açılar derecedir. Bu API acquisition frame/cycle/evaluation/provenance, calibration currentness ve hull denetimi yapamaz; bunlar T7.2 application adapter sorumluluğudur. Ham piksel feature'a mm frame etiketi takmak dönüşüm sağlamaz.

Çizgi yönü `end-start` normalize edilerek kullanılır. Uzunluk **>1e-12 caller birimi** olmalıdır; eşit/kısa support segmenti reddedilir. Noktaların eşitliği geçerlidir ve mesafe sıfırdır. Circle yarıçapı >0; yarıçap/çap seçimi açık `CircleSize` enum'udur.

## İşlemler ve yön

| API | Tanım |
| --- | --- |
| pointDistance | `hypot(to.x-from.x, to.y-from.y)` |
| xOffset / yOffset | `to-from` ilgili bileşeni; Signed veya Absolute |
| pointLineDistance | Sonsuz destek doğrusuna signed `cross(unitDirection, point-start)`; Absolute mutlağını verir |
| pointSegmentDistance | Kapalı start/end segmentine en kısa mutlak mesafe; dik izdüşüm dışarıdaysa yakın uca uzaklık |
| lineAngleDegrees | `atan2(cross(u,v),dot(u,v))`; SignedDirected (-180,180], UnsignedDirected [0,180], AcuteUndirected [0,90] |
| lineIntersection | Sonsuz destek doğrularının Unique/Parallel/Coincident sınıfı; yalnız Unique için framed Point2D |
| parallelLineDistance | Paralellik kapısı geçerse ilk yönlü doğruya ikinci start'ın signed dik uzaklığı veya mutlağı |
| circleCentreDistance | Merkezler arası Öklid uzaklığı; çevreler arası clearance değildir |
| circleSize | Gerçek input circle radius veya 2*radius; fit/refit yapmaz |

Pozitif signed dik mesafe start→end'in frame eksenlerinde +90° tarafıdır. +X yönünde çizgi için +Y pozitif; Y-down görüntüde aşağı, Y-up düzlemde yukarıdır. Pozitif açı +X'ten +Y'ye döner. İlk çizgiyi ters çevirmek signed mesafeyi ters çevirir; ikinci paralel çizgiyi ters çevirmek aralığı değiştirmez. Antiparalel yönlerin signed açısı +180 olarak kanonikleşir. AcuteUndirected destek yönünün terslenmesinden etkilenmez. Ölçüm değerlerine sıfırlama/yuvarlama epsilon'u uygulanmaz (yalnız negatif sıfır kanonikleşir).

## Paralellik ve kesişim sınırı

Normalize yönler için `abs(cross(u,v)) <= 1e-12` sayısal paralellik kapısıdır. Kapı içinde `abs(cross(u,second.start-first.start)) <= 1e-12 + 16*double_epsilon*norm(second.start-first.start)` ise Coincident, aksi halde Parallel. Bunlar yakın paralel/çakışık geometri için konservatif sayısal sınıflardır; fiziksel tolerans veya tam sembolik eşitlik değildir. Paralel aralık kapı içinde ikinci start'a göre hesaplanır; küçük artık yön farkı boyunca sabit fiziksel boşluk iddiası yoktur. Paralel olmayan çizgiye spacing sorulursa hata verilir.

Unique kesişim destek uçları dışında olabilir; bu API segment kesişimi veya kalibrasyon alanı kabulü sağlamaz. T7.2 adapter türetilmiş noktanın measured support ve kalibrasyon alanını ayrıca denetler; domain sınıfı bu politikayı uygulamaz. Çok yakın paralel çizgilerde uzak/kararsız kesişim üretmek yerine kapı sonucu verilir; overflow ise hata olur. Çakışık çizgiler için uydurma başlangıç noktası dönmez.

Double girişlerde çok büyük offset/küçük özellik hassasiyeti kaybolabilir. `hypot` gereksiz kare taşmasını önler; diğer ara işlemlerde taşma konservatif ret verir, arbitrary precision yoktur. Min çizgi ve paralellik epsilon'ları fiziksel uncertainty/ISO/GD&T veya üretim kabulü değildir.

## Doğrulama

`geometry_measurements_contract`, yalnız `visionflow_projects` / Qt Core ile bağlanır. Analitik 3-4-5, diagonal/rotated line, +/−/absolute offset, segment uç/iç/dış, antiparalel/çakışık/ayrı paralel, support dışı kesişim, radius/diameter, px/mm ve Y-up/Y-down testleri vardır. Ölçek 1e-8–1e8 ve öteleme, 1e9 offset, 1e200 hypot, küçük signed residual, min çizgi nextafter sınırı, paralellik kapısının iki tarafı; yanlış frame/birim/eksen/enum/NaN/Inf/overflow/dejenere retleri doğrulanır. Expected değerler ürün hesabından üretilmez. Derleme/CTest kanıtı [progress](../progress.md) içindedir; raster veya fiziksel ölçüm kabulü değildir.
