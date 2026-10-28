# T5.1 — Edge Pair çekirdeği ve ölçüm servisi

29 Ekim 2026: güncel local raster/crop ve tam kalibre sampling hull kapıları
[ortak ROI sözleşmesindedir](roi-crop-contract.md#3-kalibrasyon-ve-sampling-apisi--efr-03a-4).
Aşağıdaki tarihsel endpoint doğrulamasına ek olarak bütün gerçek tap ve bilinear
komşularının footprint/hull denetimi örnekleme sırasında yapılır.

10 Ekim güncellemesi: **T5.3 tamamlandı**; kullanıcı aracı/registry/recipe/kalibre world Line/Runtime/Logs/CSV ve UI kabulü [Metrology sözleşmesinde](metrology.md). Engine execution/10, güncel binary build/metrology-ui/visionflow.exe. Aşağıdaki T5.1/T5.2 kapsam ve kanıtı tarihsel çekirdek teslimidir; “T5.3’te” ifadeleri o teslimin sınırını anlatır.

9 Ekim 2026. Bu teslim vision/application API'sidir. Toolbox, reçete kind/port/config, Runtime/Logs/CSV ve caliper overlay bağlantısı T5.3'te uygulanacak; çalışan kullanıcı aracı olarak sunulmaz. Sıradaki alt paket T5.2 Line fit. Mevcut Edge Point/Measurement/UI ve execution/9/editor7/terminal3/vf.socket/2 değişmez.

## Profil ve eşleştirme

`EdgePairFinder::find` aynı [ProfileSampler](profile-sampling.md) ve [EdgeFinder](edge-point.md) üzerinden **tek profil** hesaplar. Bütün rising/falling adaylar, gerçek signed strength, refinement ve ret nedenleri korunur. Tek EdgeFinder seçimi pair seçimi olarak kullanılmaz.

- Kontrast ve tam destek kontrolünü geçen adaylar tarama sırasıyla değerlendirilir. Yalnız bu listedeki **komşu zıt kutuplu** kenarlar çift olabilir. Kabul edilmiş ara kenar atlanmaz; tüm olası O(N²) eşleştirme yapılmaz. Kontrast altı nicemleme/gürültü tepeleri eşleştirmeyi bölmez; edge tanısında kalır. Aynı kutuplu ardışık tepeler birleştirilmez, çift oluşturmaz.
- Bright stripe rising→falling, dark stripe falling→rising; any ikisini kabul eder. Polarite A→B tarama yönüne göredir. Şerit ters tarandığında bright/dark değişmez, uçlar yer değiştirir.
- Genişlik subpixel uçların profil mesafeleri farkıdır; `minWidthPx` ve `maxWidthPx` dahil sınırlar, sıfır genişlik reddedilir. Min finite ≥0, max finite >0 ve ≤65536, min≤max. Bunlar px arama kapısıdır; mm toleransı değildir.
- First/last uygun çiftin başlangıç sırasıdır. Strongest çift gücü `min(edge1.strength, edge2.strength)`; 1e-9 ölçekli tie epsilon içinde ilk çift kazanır. Birbirinden bağımsız strongest kenarlar birleştirilmez. Width kapısı komşuluk kuralını değiştirmez.
- Pair tanısı gerçek edge indeksleri, width/strength/accepted/reason ve seçili pair indeksini taşır. Eşleştirme O(N) ek zaman/bellek ve en çok N−1 pair üretir. Sampler'ın frame/format/footprint/tap/convolution/bellek sınırları devralınır; filtreleme/eşleştirme döngülerinde cooperative iptal kontrolü vardır. Partial sonuç dönmez. Hash/native işlemler hard interruption değildir.

## Application ölçüm sözleşmesi

`EdgePairService::measure` PixelDebug veya Calibrated modu alır. Bulunan çift first/second/centre (raw px) ve widthPx verir. Centre raw uçların aritmetik orta noktasıdır; perspektifte world midpoint diye sunulmaz. PixelDebug mm veya kalibrasyon kanıtı üretmez. Hiç çift bulunmaması DONE/found=false ve boş geometri/genişliktir; konfigürasyon/kalibrasyon/iptal hatası ERROR, bütün metrikler boş ve hata nedeni doludur.

Calibrated modda çağıran mevcut resolver'dan elde ettiği validated `PlanarCalibration` ile bağımsız current dependency'yi sağlamalıdır. Servis aynı `MeasurementService::distance` kontrollerini kullanır: accepted bağımsız holdout, güncel content/version/source/lens/plane/çözünürlük, acquisition frame/cycle/time ve raw alan. Uniform/manual modellerde mevcut scale sözleşmesi; homography'de **iki uç ayrı world mm'ye çevrilir**, mesafe sonra alınır. Ortalama/nominal px/mm çarpanı veya px fallback yoktur. Sonuç widthMm ve calibration dependency taşır. Pair found=false durumunda da current/provenance/alan kontrolü scan uçlarıyla yapılır; kalibrasyon hatası boş başarılı arama diye gizlenmez.

Mevcut distance API'si içeride [0, double max] aralığıyla kullanılır; bu internal Measurement kararı sonuçtan dışarı aktarılmaz. Edge Pair sonucu tolerance/PASS/FAIL/confidence/robot pose içermez. Reçete resolver/registry/cache/persistence kullanıcı bağlantısı T5.3'te yapılacaktır; bu servis bağımsız bir kalibrasyon sistemi kurmaz.

## Sayısal kabul ve sınırlar

`tests/edge_pair_test.cpp`: ilk/son/en güçlü/tie, açık/koyu/any, bağımsız strongest tuzağı, kabul edilmiş ara kenarı atlamama, min/max dahil ve nextafter, border/flat/single/wrong polarity, invalid ROI/frame/parametre ve iptal. Bağımsız erf raster corpus: 128×128 Gray16, yedi açı × üç subpixel phase × iki stripe polarity = **42 temiz**; nominal 24 px, blur σ1.4, step .5/sigma1/width4. Uçlar/orta/width/ters yön/input hash ayrıca sınanır. Genişlik RMS **0.00108481 px**, max **0.00418522 px**; .15 RMS/.30 max kapıları geçti.

Ayrı 42 degraded: blur σ2.6, ±6 gray-level deterministic noise, seed1234567; width RMS **0.2763 px**, max **0.568233 px**, ayrı .6 px max kapısı. Test raporu `build/edge-pair-core/edge-pair-report.txt`. Manuel ve uniform bağımsız holdout → 24 px/2.4 mm; analitik projective dönüşümün bağımsız tersiyle widthMm oracle, nominal scale negatif kontrolü; stale/candidate/source/resolution/frame/alan/file_test ret ve no-found metrik temizliği.

Sentetik yazılım kabulüdür; fiziksel genişlik/hassasiyet, düşük kontrastlı nesne sınırları, texture/ringing/üretim corpus kabulü değildir. Kontrast altı gerçek sınırlar gürültüden ayrıştırılmış sayılmaz. Tek profil caliper çekirdeğidir; çoklu scan/Line ve UI kabulü T5.2/T5.3'te kalır. K4, P7/6D.2, lens T15, kurulum/dağıtım/CUDA kabulü açık.
