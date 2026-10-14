# T4.1 — Yönlü profil örnekleme

9 Ekim 2026. `vision/metrology/ProfileSampler` çekirdeği uygulandı. T4.2 [EdgeFinder ve edge_point](edge-point.md) de tamamlandı; okla Editor/Runtime ve Measurement bağlantısı T4.3'te kalır. Legacy Edge değişmedi; T4.2 ile execution/9, editor 7, terminal 3 ve vf.socket/2 korunur. Metrologi örnek tipi, mevcut domain ProfileSample ile çakışmaması için `IntensityProfileSample` adını kullanır; hesaplama değişmedi.

## Girdi ve geometri

Salt okunur owned `ImageFrame`, raw px/XRightYDown `ToolRoi::ScanSegment`, `ProfileOptions` ve optional atomic cancel. ROI frame ID görüntünün coordinateSystemId'siyle aynı olmalı. A→B yönü ve tam dik şerit genişliği kullanılır. Bütün dört köşe `[0,w-1] × [0,h-1]` pixel-centre alanında olmalı; crop, clipping, AABB replacement, border padding veya pose/world mapping yoktur. Tam integer sınırdaki sıfır ağırlıklı dış komşu gerekmez; yalnız doğrulanmış geometrinin hesaplamasında 1e-9 px roundoff sınır noktasına snap edilebilir. Sınır dışı ROI önceden reddedilir.

Boyunca `ceil(length/stepPx)` eşit aralık kullanılır, iki uç dahildir. Gerçek step istenenden büyük değildir ve sonuçta ayrıca taşınır. Genişlikte `ceil(width/widthStepPx)` eşit aralık, iki şerit sınırı dahil; her noktada double bilinear yoğunluk, sonra eşit ağırlıklı aritmetik ortalama. Genişlik ortalaması sürekli alan integrali veya trapezoid ağırlığı değildir. Width=1 de ±0.5 px şerittir. Renk `.299R + .587G + .114B`; alpha göz ardı edilir. Gray16 `255/65535` double ölçeğiyle çalışır, 8 bit'e nicemlenmez. RGB/BGR girişlerinin ImageFrame normalizasyonu korunur. UI piksel erişimi açılmaz, full-image Mat kopyası gerekmez.

## Yumuşatma ve türev

Sigma=0 identity. Pozitif sigma için gerçek step üzerinden `ceil(3*sigmaPx/actualStepPx)` yarıçaplı normalize ayrık Gaussian. Tam kernel desteği olmayan uçlarda `smoothed` boştur; replicate/zero/reflection padding yoktur. En az iki komşu geçerli smooth örnek gerekir; yetersiz profil bütünüyle reddedilir.

Signed derivative iki komşu smooth örneğin farkı / gerçek step; konumu aralarındaki raw midpoint, birimi 0–255 yoğunluk/px. Gradient listesi yalnız tam desteği olan komşuları içerir. A→B ters çevrilince profile sırası tersine döner ve derivative işareti değişir. Sampler seçim yapmaz; rising/falling/first/last/strongest, plateau/subpixel tepe ve min contrast artık [T4.2 EdgeFinder](edge-point.md) sorumluluğudur. Gaussian kenar desteği boşluğu kenar bulunamadı kararı değildir.

## Sınırlar ve sonuç

| Alan | Sınır |
| --- | --- |
| Görüntü | 16 Mi piksel |
| İstenen step / width step | 0.05–16 px / 0.25–16 px, finite |
| Sigma / şerit genişliği | 0–16 px / >0–1024 px |
| Boyunca / genişlikte örnek | En çok 65536 / 1024 |
| Bilinear konum / Gaussian toplamı | En çok 4 Mi / 16 Mi tap |
| Gaussian yarıçapı | En çok 1024 örnek |

Sınırlar allocation/örnekleme öncesinde denetlenir. Sonuç iki bounded vektör, raw koordinat/mesafe/yoğunluk, optional smooth, signed derivative, actual steps, width count, kernel radius, tam input metadata/contentId/processingId taşır. Aynı piksellerin farklı preprocessing kimliği korunur. Cancel başlangıçta, her boyuna örnekte, en fazla 64 enine örnekte, her convolution/derivative satırında ve hashing öncesi/sonrasında denetlenir; exception'da partial sonuç dönmez. Hash çağrısı zorla kesilmez. Üretim latency/hard timeout kabulü yoktur. Yeni kalıcı recipe/result schema veya genel PASS/mm/pose üretilmez.

## Yazılım kabulü

`profile_sampler_contract`: bağımsız analitik bilinear polynomial ve midpoint türevleri; 0/45/90/135/17.3/78.1/167.2°, reverse, fractional dört komşu, altı input formatı/padded source/alpha/Gray16 hassasiyeti, Gaussian impulse enerji-simetri/affine koruma/boş uç desteği, rising/falling/stripe, erf blur/quantization, sıfır ortalamalı enine gürültü; exact border/outside, yanlış ROI/frame/unit, invalid/NaN/Inf/limit ve in-flight Stop. Raw hash/provenance korunur. Raster polynomial yoğunluk hatası raporlanır; edge localization RMS/max ancak T4.2'de ölçülebilir.

Yeni Release Widgets OFF/ZXing OFF `build/profile-sampler-core`; komutlar [toolchain](../toolchain.md), gerçek sonuç ve sınırlamalar [progress](../progress.md). Test raporu `build/profile-sampler-core/profile-sampler-report.txt`. UI değişmediği için yeni görsel kabul yok; güncel kullanıcı binary'si `build/preprocess-code-ui/visionflow.exe`. Fiziksel K4/lens T15 ve P7 restart teslim güvencesi açık kalır.
