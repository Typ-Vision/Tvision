# T6.1 — Radyal tarama

10 Ekim takip teslimi: [T6.4 Circle / Arc UI ve kalibre Runtime](circle-tool.md) tamamlandı; güncel engine execution/11 ve UI build/circle-ui. Aşağıdaki önceki paket kapsamı/kanıtları tarihseldir.


10 Ekim takip teslimi: [T6.2 Circle fit](circle-fit.md) ve [T6.3 kalite/metrikler](circle-metrics.md) tamamlandı. Bu belgedeki sampling kapsamı korunur; Circle kullanıcı entegrasyonu T6.4'tedir.

10 Ekim 2026. `RadialScanner` vision çekirdeği ve `RadialScanService` application API'si tamamlandı. Daire fit'i/çap/daire kararı, registry/recipe config, Toolbox, overlay ve Runtime/Logs/CSV bağlantısı yoktur. Kullanıcı binary'si `build/metrology-ui/visionflow.exe`; execution/10/editor7/terminal3/vf.socket/2 korunur. Sonraki **T6.2 — Circle solver**, sonra T6.3 kalite/metrikler ve T6.4 UI/kalibre Runtime.

## Geometri ve seçim

[RadialScanner](../../src/vision/metrology/RadialScanner.hpp) mevcut strict raw `Annulus`/`ArcRoi` sözleşmesini kullanır: merkez, pozitif inner/outer radius, start ve 0 < |signed sweep| ≤ 360°. Açılar +X'ten +Y'ye gider; raw Y-down'da saat yönü, Y-up world'de tersidir. Scan ID tarama sırasıdır; açılar wrap edilmez. Tam ±360° halkada N farklı açı, son nokta yinelenmez. Kısmi yay iki ucu içerir. Varsayılan72, çekirdek3–4096 scan; gelecekteki recipe/persistence sınırı değildir.

Outward inner→outer, Inward outer→inner. Polarity tarama yönüne göredir. T4 EdgeFinder first/last/strongest, plateau/parabolic refinement ve contrast kapısını kullanır. Çoklu boundary'de seçim nesne kimliğini doğrulamaz. Her slot scan ID/açı, nominal strip, raw uçlar, optional nominal/raw point, strength/signedStrength, refinement ve missing reason taşır. Missing silinmez veya uydurulmaz. No-found hata değildir; found0/coverage0, point/strength boş. Hata/iptal exception verir, kısmi sonuç dönmez.

Coverage found/N. MaxGap tam halkada found komşuları arasındaki en büyük açısal boşluk, seam dahil; tek found/hiç found için360°. Kısmi yayda found komşuları ve başlangıç→ilk found/son found→bitiş aralıklarının maksimumu; hiç found için |sweep|. Tam72-scan halkada5°, sıfır değildir. Bunlar nominal scan açılarıdır, fitted merkez çevresindeki contour açıları değildir. T6.1 kalite kabulü veya tam çevre iddiası üretmez.

## World örnekleme ve kalibrasyon

[RadialScanService](../../src/application/RadialScanService.hpp) mm merkez/radius/width ve bağımsız current dependency alır. Mevcut MeasurementService kapısı validated/current/version/content/setup/source/actual resolution/frame/cycle/time ve file-test ret kurallarını merkez ve raw uç/selected noktalarda uygular. Nominal merkez de geçerli alanda olmalıdır; konservatif ret mümkündür. Candidate/stale/no-found için bypass veya px fallback yoktur.

PlaneMappingService inverseMap finite/invertible matrix, relative horizon denominator, raw bounds, mevcut inner-corner hull ve forward roundtrip denetler. PlaneMapping yalnız bounded fit/replay ile oluşturulur; yeni codec/matrix güveni yoktur. Uniform/manual inverse gerçek scale ve target Y-axis kullanır; homography kendi target-local mapping'ini kullanır.

World radyal mesafeler ve transverse width örnekleri **her intensity tap için ayrı** raw'a inverse-map edilir. Raw endpoints arasında eşit mesafeli sampler veya nominal radius×scale kullanılmaz. Perspektifte raw ROI daire olmak zorunda değildir. Subpixel seçili world mesafesi tam inverse-map ile raw'a taşınır; raw seed'den world geometri öğretme UI'sı T6.4'tedir.

Ortak ProfileSampler sampleMapped ve EdgeFinder findMapped synthetic sampling plane kullanır. `unitsPerSample` synthetic pixel başına nominal birimdir: raw API'de tam1px, world API'de açık pozitif mm. Strip bu değere bölünür; stepPx/widthStepPx/sigmaPx synthetic koordinattadır. .25mm/unit × .5step = .125mm aralık. Strength/minContrast intensity/synthetic unit türevidir, intensity/mm veya fiziksel uncertainty değildir. Bu değer değişince smoothing/threshold aynı fiziksel anlamı korumaz. Mapped overload'da Px son ekli profile alanları synthetic plane içindir; raw overload eski px sözleşmesini korur. Nominal point mm'ye geri çarpılır, rawPoint ayrı tutulur. Kalibrasyon dependency target frame'i sonuçta saklanır.

## Domain, bütçe ve iptal

Clipping/padding yoktur. Önce bütün strip köşe/uçları, sonra her actual tap map/domain kapısından geçer. Homography düz çizgileri ve konveks alanı korur; geçerli köşeler projective strip'i sınırlar. Generic mapper kendi domain kontrolünü sağlar. Read-only bilinear Gray8/16/renk ve raw hash/processing ID/provenance korunur.

Image≤16Mi pixel, image×scanCount≤512Mi hash işi. Her strip≤65536 longitudinal/1024 transverse sample, width≤1024 synthetic unit, ≤4Mi intensity tap, Gaussian radius≤1024. Bütün scan'ler toplam≤16Mi intensity ve16Mi convolution tap, gerçek endpoint length/ceil ile önceden denetlenir. Step [.05,16], widthStep [.25,16], sigma [0,16]. Edge için≥3 derivative sample. Sampler her profile hash alır. İptal preflight/scan/tap/selection ve application map döngülerinde cooperative; native inverse/hash hard interruption veya üretim latency kabulü yoktur.

## Bağımsız kabul ve sınırlar

[Test](../../tests/radial_scan_test.cpp):24 bağımsız Gray16 erf raster, radius24/40/60 × iki subpixel merkez × iki polarity × iki yön.72scan, sigma1.4px, step.5/smoothing1/width1.1728 observation RMS **0.00691883px**, max **0.0167073px**; .15/.30 hedefleri geçti. Her noktanın bağımsız analitik yarıçap farkıdır, circle fit/çap kabulü değildir.

Aynı24 degraded raster sigma2.6px/±6 gray noise/seed1234567: width1 RMS **0.220077**, max **0.673565px**, ilk .6px gate başarısız; karakterizasyon olarak tutuldu. Aynı raster width4 transverse ortalama RMS **0.146164**, max **0.440522px**, ayrı .6gate geçti. Gürültülü width1 için .6 garantisi yoktur; threshold/gürültü azaltılmadı.

Uniform/manual bağımsız accepted holdout, Y-up, candidate/current/file-test/source/actual resolution/time/Stop ret; bağımsız non-affine projective raster radius7mm, max radial error **0.00497144mm**, ayrı .10mm gate. Sigma.4mm/.25mm sampling-unit kanıtı fiziksel hassasiyet değildir. Full/partial/negative/cross-wrap/missing seam/flat/determinism, first-last boundary ve kaynak/iptal sınırları dahil **7528 kontrol**. Report/log yalnız build/radial-core altında.

Release OFF/core tam CTest **38/38,23.19sn** (radial2.36sn), localhost PLC dahil. UI dosyası değiştirilmedi; yeni UI build/görsel kabulü yapılmadı. Üretim corpus/texture/ringing/yanlış kenar, fiziksel K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA açık. [T6.2 normalize algebraic+geometric solver](circle-fit.md) tamamlandı; fit inlier ve defect sample ayrımı T6.3'te korunacak.
