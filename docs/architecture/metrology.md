# T5.3 — Edge Pair / Caliper ve Line Fit kullanıcı sözleşmesi

10 Ekim güncel kapsam: **T8.4.2b.3b — Nominal world ROI authoring ve mm metric/tolerans tamamlandı.** Edge Point/Caliper/Line/Circle için **Nominal mm ROI öğret…** ekranı geometriyi, güncel pose teaching kaydını ve DAG bağlantısını atomik kaydeder; pending/iptal/geçersiz geometri reçeteyi değiştirmez. Caliper gerçek mm genişliğini, Line current mm span/açısını dahil tool toleranslarıyla değerlendirir. Circle tüm gerçek gözlemlerden (fit outlier dahil) radyal sapma, current world açı/gap/observed arc ve kaliteye bağlı tolerans üretir. Raw line/circle fit ve synthetic derivative strength unavailable; genel parça kararı NOT_EVALUATED. Engine **execution/19**, registry16/editor7/terminal3/vf.socket/2 korunur. Production pose whitelist reddi sürer. Sonraki **T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü**; T8.4 açık. Güncel Release binary: `build/tracking-ui/visionflow.exe`.

Alttaki teslim kayıtları tarihseldir; güncel kapsam yukarıdadır.

T7.3 güncellemesi: Geometry Measurement v2 Toolbox/typed feature/tolerans/Runtime exact completion replay tamamlandı; engine execution/13. [Uygulanan kullanım ve sınırlar](geometry-measurement-adapter.md). Aşağıdaki T7.2 UI/Runtime erteleme ifadeleri tarihsel kapsama aittir.

10 Ekim T7.2 tarihsel kayıt: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](geometry-measurement-adapter.md).


10 Ekim takip teslimi: [T6.4 Circle / Arc UI ve kalibre Runtime](circle-tool.md) tamamlandı; güncel engine execution/11 ve UI build/circle-ui. Aşağıdaki önceki paket kapsamı/kanıtları tarihseldir.


10 Ekim 2026. C++20/Qt Widgets ortak Recipe Editor ve Runtime, application `MetrologyService` üzerinden mevcut [Pair](edge-pair.md) ve [Line](line-fit.md) çekirdeklerini çalıştırır. UI vision algoritması çağırmaz. Registry `edge_pair` ve `line_fit` v1; editor 7 / terminal 3 / vf.socket/2 korunur, engine **visionflow.execution/10** olur. Eski production validation ve cache yeni engine ile yeniden hazırlanmalıdır; eski reçete/no-op dosya formatı değişmez.

## Kullanım

Toolbox **Edge Pair / Caliper** veya **Line Fit** ekleyin, Image input bağlayın. Önce görüntü edinin. Caliper A→B okunu; Line döndürülmüş kutusunu çizin veya raw subpixel koordinatlarını **ROI koordinatlarını uygula** ile öğretin. Tutamaçlar/Ctrl+Z/Escape mevcut ROI yolunu kullanır. Geçici ROI penceresi reçete alanını değiştirmez. Line kutusunun yerel genişlik ekseni çizgi destek yönü, −yükseklik→+yükseklik scan yönüdür; taught yön tersine çevrilince polarity/angle sınırlarını buna göre seçin.

`mode=pixel` varsayılandır: gerçek px çıktıları diagnostic işaretlidir; mm portları `PIXEL_MODE` nedeniyle invalid/null kalır. `mode=calibrated` mevcut Setup → Calibration kaydet/bağla yolundaki kesin, bağımsız holdout ile kabul edilmiş kalibrasyonu gerektirir. Kaynak/kurulum/latest version/content/acquisition root resolution/frame/cycle/time/alan kontrolleri mevcut resolver ve MeasurementService ile yapılır. File-test manuel kalibrasyon üretime giremez. Eksik/stale/alan dışı bağlamda px fallback veya kısmi mm sonuç yoktur.

**Run Flow** yeni acquisition; **Run Tool** yalnız seçili aracı mevcut owned raw kare ve doğrulanmış upstream cache ile çalıştırır. Save/reopen → Load Recipe → Run → Tek çevrim immutable snapshot'ı çalıştırır. Stop/ERROR/SKIPPED eski değer, kabul Boolean'ı veya overlay bırakmaz. Logs/CSV typed portları, method/version/evaluation, kaynak/frame/cycle, birim, validity ve kalibrasyon kanıtını korur. Görüntü veya tam profil arşivlenmez.

## Caliper

Ortak profilde komşu kabul edilmiş opposite-polarity kenarlar; bright/dark/any ve first/last/strongest. `minWidthPx/maxWidthPx` aday seçim kapısıdır ve her modda px'dir. `first/second/centre` raw Point; `widthPx` gerçek uç mesafesi; `widthMm` iki endpoint ayrı dönüştürülerek hesaplanır. `lower/upper` seçili modda px/mm genişlik toleransıdır; sınırlar ham double üzerinde dahildir. Bulunamadı DONE/found=false; point/width/accepted invalid/null ve bağlı ölçüm SKIPPED. `accepted=false` ise gerçek bulunan fakat tolerans dışı genişliktir; execution DONE kalır. Overlay raw uçlar/orta/mesafe ve gerçek seçili birim değerini gösterir.

## Line

Raw scan ve fit kapıları: `scanCount` 3–128 (en az minPoints), `scanWidthPx`, ortak profile/edge ayarları; `minPoints/minCoverage/minSpanCoverage`, `minSpanPx/inlierDistancePx/maxRmsPx/maxResidualPx`, ransac. LineFitter çekirdek API'sinin 4096 sınırı korunur; **reçete aracı 128 scan ile sınırlıdır**, böylece tüm slotlar mevcut bounded InspectionResults portunda kayıpsız saklanır. Sessiz tanı seyreltmesi yoktur. Sabit çekirdek condition≥25 / 256 RANSAC trial kapıları sürer.

Calibrated mod örnekleme öncesi ve her gerçek tap/bilinear komşusunda local raster footprint ve calibration hull denetler ([crop sınırı](roi-crop-contract.md#3-kalibrasyon-ve-sampling-apisi--efr-03a-4)); raw fit kapılarından sonra **bütün found scan noktalarını** (raw outlier dahil) doğrulayıp world mm'ye dönüştürür ve yeniden robust fit yapar. Homography ayrı nokta mapping'i; uniform/manual gerçek mm/px ve target Y eksen yönünü kullanır. `minSpanMm/inlierDistanceMm/maxRmsMm/maxResidualMm` world fit kapılarıdır; px limiti bir nominal scale ile mm'ye çevrilmez. Fiziksel açı world tangent `atan2` üzerinden [-180,180) hesaplanır. World ve raw consensus farklı olabilir. Raw modelin projected support uçları da calibration alanını geçmelidir; sınır yakınında konservatif ret mümkündür.

`linePx/lineMm` **ölçülen support** Line2D segmentleridir; sonsuz çizgi ayrı `originPx/tangentPx` veya `originMm/tangentMm` ile `origin+t*tangent` olarak temsil edilir. Tangent portları dimensionless unit-vector'dür. `anglePx/angleMm` deg; `span/rms/maxResidual` ayrı px/mm; `coverage/spanCoverage/usedCount/outlierCount/missingCount` seçili fitten gelir. `scans` her requested slot için found koordinatını, signed residual/birimini ve used/outlier/not_found nedenini tutar. Kayıtlı world koordinat çerçevesi lineMm'dedir; scan slot sırası raw çizimle eşleştirir.

`lower/upper` seçili modda destek uzunluğu; `angleLower/angleUpper` aynı modda yönlü açı için dahil toleranstır. `accepted` bu iki aralığın conjunction'ıdır. Fit kalite kapısı başarısızsa ERROR/çıktı yoktur; düşük coverage tolerans FAIL gibi sunulmaz. Inlier RMS tüm yüzey kusur metriği değildir. Genel parça kararı, confidence veya robot pose üretilmez; mevcut Measurement kararları bağımsız kalır.

Editor ve Runtime overlay **raw fit destek segmentini** ve raw gözlem noktalarını gösterir; world consensus ile elenen raw noktalar kırmızı `outlier` işaretlidir. World model/angle/metric ayrı typed sonuçtur; overlay world çizgisinin inverse projection'ı değildir. Results/Logs bütün scan residual kanıtını sunar. Runtime ROI salt okunurdur.

## Kabul ve sınırlar

`tests/metrology_runtime_test.cpp` 87 kontrolle bağımsız stripe raster, typed save/reopen, Pair→Calculate, gerçek calibrated mm/tolerans nextafter sınırı, TargetOnly/raw hash, immutable Runtime/restart/Logs/CSV, no-found/insufficient line, stale current/unbound/actual resolution/alan/Stop senaryolarını kapsar. Anisotropik homography raw `atan(.25)` ile world `atan(.125)` ayrımını gerçek scan ve world refit ile doğrular; nominal scale açısı negatif kontrolüdür. Ayrı gerçek projective H örneğinde world atan(.4) bağımsız analitik oracle ile sınanır. Mevcut Pair523/Line1155/Edge359/sampler3893 corpus kontrolleri korunur.

UI smoke gerçek Properties/ROI/save/reopen/TargetOnly, önceki bağımsız iki holdout kalibrasyonuyla 400px→10mm caliper ve 238px→5.95mm line, üç local displaced edge outlier, Runtime/Logs/CSV/Stop akışını çalıştırır. Normal ve 850×600 görseller `build/metrology-ui` altındadır. Offscreen kabul native DPI/çoklu monitör/fiziksel doğruluk değildir. Büyük görsellerde hash/image×scan/aggregate sampler kapıları hâlâ uygulanır; native hash/fitLine hard interruption yoktur. Üretim texture/yanlış paralel kenar/bağımsız fiziksel corpus, K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA kabulü açık kalır. [T6.1 radyal tarama](radial-scan.md) ve [T6.2 Circle solver](circle-fit.md) servis/çekirdek olarak tamamlandı; [T6.3 kalite/metrikler](circle-metrics.md) de tamamlandı; sonraki **T6.4 — UI ve kalibre Runtime**. Circle kullanıcı arayüzü henüz yoktur.
