# T6.3 — Circle kalite ve metrikler

10 Ekim takip teslimi: [T6.4 Circle / Arc UI ve kalibre Runtime](circle-tool.md) tamamlandı; güncel engine execution/11 ve UI build/circle-ui. Aşağıdaki önceki paket kapsamı/kanıtları tarihseldir.


`CircleMetrics::measure(RadialSearch,FittedCircle,options)` T6.1 taraması ve T6.2 fit'inin **aynı sırada, birebir aynı koordinatlı** nominal-plane noktalarını alır. En çok4096 scan, scanId0…N−1 ve eşit signed nominal açılar denetlenir. Fit/raw/world birimleri caller'a aittir; mevcut RadialScanService kalibre world sampling ve kabul kapılarını sağlar. UI algoritma/SDK çağırmaz; Circle registry/Editor/Runtime/Logs/CSV entegrasyonu T6.4'tedir.

## Fit ve kontur birbirinden ayrıdır

Sonuç gerçek fit centre/radius/diameter (`2*r`), used noktaların yeniden hesaplanan fitRms/fitMaxResidual ve fitUsed/fitRejected/found/missing sayıları taşır. Her scan point, nominal angle, fitUsed, radialDistance ve dışa pozitif signedResidual (`distance−radius`) korunur. Missing slotta mesafe/residual yoktur; interpolasyon yapılmaz. Input fit residual/RMS/count ve search coverage/gap cache'leri ölçüm hesabında kullanılmaz; slotlar üzerinden yeniden hesaplanır. Bu API yeni bir solver veya terminal tamper güvenlik sınırı değildir; kabul edilmiş fit ve güvenilir unit/frame/calibration caller sorumluluğudur.

**Bütün found noktalar**, fit outlier'ları dahil, aynı fit merkezine göre şu gözlenen tanıyı üretir:

| Alan | Formül |
| --- | --- |
| minimumRadius / maximumRadius | min/max `distance(point,fit.centre)` |
| peakToValley | maximumRadius−minimumRadius |
| maxInward | max(0,fit.radius−minimumRadius) |
| maxOutward | max(0,maximumRadius−fit.radius) |

`observedDeviation` her zaman yalnız görülen örneklerin tanısıdır. `deviation` ise optional ve aşağıdaki kapsam kapılarına bağlıdır. Fit inlier RMS'i çapak/kusur metriği gibi kullanılamaz. Peak-to-valley fit merkezine göre sampled radyal aralıktır; minimum-zone/GD&T/ISO dairesellik veya fiziksel belirsizlik değildir. Birim uzunluk caller birimi, alan caller biriminin karesidir; circularity boyutsuzdur.

## Kapsam kapıları

Varsayılan `minFoundPoints=6`, `minCoverage=.9`, `maxGapDegrees=15` geliştirme profilidir; fiziksel yeterlilik standardı değildir. Coverage **found/N**; RANSAC outlier'ı hâlâ ölçülmüş kontur noktasıdır. Gap T6.1 ile aynı **nominal scan açıları** üzerindedir; fitted contour açıları değildir. Tam çevrede seam dahil en büyük found-to-found aralık; yayda yalnız taught interval içindeki found aralıkları ve eksik uç desteği. Eksiksiz72 ring gap5°; bir seam noktası eksikse10°. Yayın açık uçları arasında sahte kapanış aralığı eklenmez.

Min found/coverage dahil `>=`, max gap dahil `<=`; ham double kullanılır. Kapılar geçerse scope `FullCircle` veya `ObservedArc` ve optional deviation dolar. Geçmezse scope `Unavailable`, deviation boş ve `qualityReasons` içinde `insufficient_found_points`, `insufficient_scan_coverage`, `excessive_nominal_gap`; fit ve observed tanı korunur. Tolerans/parça accepted/PASS/FAIL üretilmez; T6.4 tüketicisi unavailable ölçümü NOT_EVALUATED olarak ele almalıdır. FullCircle yalnız bu eşiklerle nitelendirilmiş **örneklenmiş tam nominal halka** anlamındadır; gözlenmeyen bölgede kusur olmadığını kanıtlamaz. Arc sonucu yalnız görülen yay içindir.

## Gerçek kontur circularity

`closedContour` açıkça kapalı olduğu bilinen, ilk noktası sonda yinelenmeyen3–4096 vertex polygon alır. `measure` bunu yalnız full-circle scope geçerli ve **hiç missing yoksa** çağırır. Outlier vertex'ler çıkarılmaz. Arc `open_arc`; quality fail `quality_not_evaluated`; missing `missing_contour_samples` ile unavailable. Missing noktalar birbirine bağlanarak kapalı yüzey varsayılmaz. Çok az örnek, zero-length edge, tekrar kapanış vertex'i, collinear geri dönüş/overlap, non-adjacent contact/crossing ve zero area ayrıca unavailable döndürür. Geçersiz/nonfinite/çok büyük input ve Stop exception verir.

Difference/scale normalization ve long-double cross accumulation ile aynı gerçek polygon'dan shoelace `A=abs(sum cross(p_i,p_i+1))/2`, `P=sum edgeLength`; `circularityIndex=4*pi*A/P²`. Fitted circle area/perimeter veya pixel-count area kullanılmaz; ideal daireden sahte1 üretilmez, değer clamp edilmez. Saat yönü/tersi aynı unsigned alanı verir. Self-intersection circularity'yi geçersiz yapar; radyal mesafeler ayrı tanı/qualified sampled metric olarak kalabilir. Near-contact için normalize1e-12 konservatif ret; çok küçük feature/büyük offset ve yakın konturlar double hassasiyetiyle sınırlıdır.

Topology O(N²), en çok~8.4Mi segment çifti; memory O(N). Tüm ana döngü ve her segment çiftinde cooperative Stop; hard latency garantisi yok. Full circle fit/distance pass O(N). Görüntü kopyalama/işleme veya SDK yok; search origin/content/processing provenance sonuçta korunur, kalibrasyon kabulünü kendi başına yeniden üretmez.

## Bağımsız kabul

[Test](../../tests/circle_metrics_test.cpp): ideal72 polygon'un analitik area/perimeter/index'i; symmetric±.2 radial gerçek min/max/.4range; fit'in dışladığı burr+5/chip−3→PV8; lobed ve ellipse; full/partial/signed/cross-seam/missing boundary; inclusive/nextafter quality; actual damaged polygon için bağımsız polar area/cosine-law perimeter; square/triangle/winding/scale/1e9offset/topology/Stop. Birebir nextafter point mismatch ret; stale summary'ler yeniden hesaplanır.

Gerçek Gray16 erf sampling→fit→metric6 raster (radius24/40/60 × iki polarity), sigma1.4/step.5/width1/smoothing1; radial max **0.0149765px**, peak-to-valley max **0.0137575px**; ayrı .03/.04px gate. Gerçek polygon index min **0.999362**, analitik regular72 **0.999365**,1e-5 gate. Non-affine world7mm radius raster, iki accepted independent holdout ve T6.1 world scan: diameter **13.9955mm**, PV **0.00619884mm**, index **0.999357**; ayrı .06mm diameter/.02mm PV/1e-5index gate. Aynı raw perspective noktalarında PV **4.23533px** negatif kontrol; raw fit metriği scale edilmez. Bu yazılım kanıtı fiziksel ölçüm doğruluğu değildir.

**592 kontrol**; rapor `build/circle-core/circle-metrics-report.txt`, nihai Release/CTest ve ilk düzeltme kayıtları [progress](../progress.md) içindedir. UI değişmedi; mevcut binary `build/metrology-ui/visionflow.exe` ve config korunur, yeni UI build/görsel kabulü yok. Sıradaki **T6.4 — UI ve kalibre Runtime**. K4/P7/6D.2/lensT15/üretim corpus/yanlış gerçek edge/texture/ringing/kurulum/dağıtım/CUDA kabulü açık.

Nihai Release OFF/core tam CTest **40/40,26.75sn**, circle_metrics.36sn; son topology ve exact point-match değişikliklerinden sonradır. Localhost erişimli log `build/circle-core/t6-3-final-ctest.log`; önceki selected3/3/4.69sn ara kabuldür. Ellipse mean-radius analitik numerical oracle1e-6unit, bounded LM'in2.2687e-8unit farkını kapsar; raster/quality kapıları bu nedenle değiştirilmedi.
