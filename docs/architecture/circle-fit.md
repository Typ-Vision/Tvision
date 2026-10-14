# T6.2 — Circle solver

10 Ekim takip teslimi: [T6.4 Circle / Arc UI ve kalibre Runtime](circle-tool.md) tamamlandı; güncel engine execution/11 ve UI build/circle-ui. Aşağıdaki önceki paket kapsamı/kanıtları tarihseldir.


10 Ekim takip teslimi: [T6.3 kalite/metrikler](circle-metrics.md) tamamlandı. Aşağıdaki T6.2 kapsam sınırları solver API'sine aittir; bütün konturdan radial sapma, full/arc kapsam kapıları ve gerçek kapalı polygon circularity ayrı CircleMetrics API'sindedir. Sonraki T6.4 Circle kullanıcı entegrasyonudur.

T6.2 yalnız metrologi çekirdeğidir. `CircleFitter::fit` aynı düzlemdeki isteğe bağlı nokta slotlarını alır. Raw debug için px; kalibre kullanımda **önce bütün ölçülmüş noktalar world mm'ye taşınır**, sonra aynı solver çalışır. Fitter kalibrasyon kabulü, unit/frame/provenance denetimi veya uygulama resolver'ı değildir; bunlar caller sorumluluğudur. T6.1 `RadialScanService` bu kapıları ve world sampling'i sağlar. Raw fit merkezini map etmek veya radius×scale kullanmak perspektifte geçerli değildir.

## Model ve tanı

`FittedCircle` centre/radius, fit inlier RMS/maxResidual, used/rejected/missing sayıları, used/toplam slot coverage, gözlenen inlier angularSpanDegrees ve condition taşır. Bütün giriş slotları aynen sırada kalır. Her found noktada dışa pozitif `||point-centre||-radius` residual; inlier `used=true`, outlier `reason=outlier`; missing `not_found` ve residual yoktur. Girişteki eski used/residual/reason alanları yeniden hesaplanır. Outlier noktalar saklanır; fit RMS bir yüzey kusuru veya dairesellik metriği değildir.

Angular span, fitted merkez etrafında sıralanan inlier açılarında **360° eksi en büyük dairesel aralık**tır. 72 eşit tam halka örneği için 355°; kısmi yay için gözlenen desteğin yay açıklığıdır. T6.1 nominal scan açısı/maxGap ile aynı şey değildir. Min span kararsız dar yay fit'ini engeller; tam çevre max-gap/min-coverage kalite politikası ve bütün konturdan peak-to-valley/circularity **T6.3**'tedir. Bu API çap toleransı, genel parça kararı, PASS, confidence veya robot pose üretmez. Circle registry/Editor/Runtime/Logs/CSV bağlantısı **T6.4**'tedir; üç applicationType mevcut ortak yolu korur.

## Solver ve retler

1. Noktalar farklar üzerinden merkezlenip ölçeklenir. Double SVD ile `[2*x,2*y,1] * [cx,cy,k] = x²+y²` çözülür; pozitif `r²=k+cx²+cy²` gerekir. Rank eşiği singular value ratio >1e-10.
2. Geometrik `sum (||p-c||-r)²` amaç için en çok64 damped Gauss–Newton/LM adımı. Pozitif radius ve finite model; yalnız azalan/eşit amaçlı adım kabul edilir. Gradient, adım veya amaç iyileşmesi ile yakınsaklık gerekir; limit/çözümsüzlük ret verir. Native OpenCV SVD/solve öncesi/sonrası cooperative Stop kontrolü vardır.
3. Varsayılan RANSAC, local mt19937 seed `0x56464332`,256 (1–1024) üç nokta hipotezi; global OpenCV RNG değişmez. En çok inlier, sonra en az squared error; eşitlikte önceki hipotez korunur. RANSAC kapalıysa tüm noktaların algebraic+geometric başlangıcı kullanılır; outlier dayanımı garantisi yoktur. RANSAC da bounded olasılıksal aramadır; çoğunluğun yanlış gerçek kenar olması çözülemez.
4. En çok8 geometric consensus refit; üyelik sabitlenmelidir. Her consensus yeniden normalize edilir; uzak reddedilen noktalar final SVD hassasiyetini tüketmez. Radius/centre ve bütün residual'lar caller birimine döner.
5. Son Jacobian satırı `(dx/distance,dy/distance,-1)` üzerinden condition **(smax/smin)²**, yani normal matrix spectral condition. Rank/condition, angular span, used/toplam coverage, inlier RMS ve max residual kapıları uygulanır. Yetersiz/kararsız/dejenere sonuç exception verir; kısmi circle dönmez.

Varsayılanlar: min6 nokta, minCoverage.6, minAngularSpan90°, maxCondition1000; inlierDistance1, maxRms.35, maxResidual1 caller birimindedir. Bunlar geliştirme varsayılanlarıdır; mm caller kendi fiziksel eşiklerini açıkça seçmelidir. Teorik üç nokta ürün kabulü olarak sunulmaz. Duplicate/nonfinite, |coordinate|>1e12, spread≤1e-12, normalize threshold<1e-12 ret edilir. En çok4096 slot,1024 hipotez,8 refit,64 LM adımı; memory O(N), bounded iş. İptal nokta/hipotez/LM/üyelik/sonuç döngülerinde cooperative; native çağrı hard interruption veya üretim latency kabulü değildir. Çok büyük offset/küçük feature oranı double hassasiyetiyle sınırlıdır; ret/yuvarlama olabilir.

## Kabul kapsamı

[Test](../../tests/circle_fit_test.cpp) bağımsız analitik tam/kısmi/clockwise/cross-seam daire, offset/radius ölçekleri, geometrik optimum ve stationary equations, outlier/missing/burr, narrow arc/condition/rank/residual/parametre/Stop retlerini içerir. T4/T6.1 gerçek raster sampling üzerinden Gray16 erf temiz ve ayrı gürültülü corpus; kalibre non-affine world-circle raster ve bağımsız holdout kabulüyle world fit sınanır. Aynı fitter sonucu expected olarak kullanılmaz; raw fitted centre'i map etmenin yanlışlığı negatif kontrolle sınanır. Rapor ve test günlükleri `build/circle-core/` altındadır. Ölçülen sayılar ve son Release sonucu [ilerleme kaydında](../progress.md) tutulur.

Fiziksel hassasiyet/üretim corpus/texture/ringing/yanlış kenar, K4/P7/6D.2/lensT15, kurulum/dağıtım/CUDA kabulü açık. T6.3 kalite/metrikler sıradadır; bu teslim MZ/ISO/GD&T uygunluğu veya üretim izni değildir. UI dosyası değiştirilmedi; yeni Circle kullanıcı özelliği veya UI/görsel kabulü yoktur.

10 Ekim Release OFF/core kabulü: **295 kontrol**, tam CTest **39/39,16.94sn** (circle1.60sn), localhost erişimiyle `build/circle-core/t6-2-localhost-ctest.log`. İlk sandbox tam koşu38/39,31.73sn; yalnız PLC `expected server HELLO` hatası, aynı binary erişimli koşuda geçti. Temiz24 Gray16 raster (radius24/40/60 × iki merkez × polarity × yön),72scan/step.5/smoothing1/width1/sigma1.4: diameter RMS **0.0125033px**, max **0.0170644px**, centre max **0.000815373px**; diameter .15RMS/.30max hedefleri geçti. Ayrı24 degraded sigma2.6/±6gray noise/seed1234567/width4: diameter max **0.134291px**, ayrı .6max gate. Non-affine accepted homography world raster radius7mm: diameter error **−0.004514mm**, centre error **0.00024318mm**, .06/.03mm gate; fiziksel hassasiyet değildir. Rapor `build/circle-core/circle-fit-report.txt`. Analitik45 offset/radius/span kombinasyonu, simetrik.2 residual geometric optimum, irregular noisy geometric stationary equations ve burr5unit residual ayrıca sınandı. Sonraki kalite metrikleri için outlier slotları kayıpsızdır.
