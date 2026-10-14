# T5.2 — Çoklu tarama ve Line fit çekirdeği

T7.3 güncellemesi: Geometry Measurement v2 Toolbox/typed feature/tolerans/Runtime exact completion replay tamamlandı; engine execution/13. [Uygulanan kullanım ve sınırlar](geometry-measurement-adapter.md). Aşağıdaki T7.2 UI/Runtime erteleme ifadeleri tarihsel kapsama aittir.

10 Ekim T7.2 tarihsel kayıt: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](geometry-measurement-adapter.md).


10 Ekim güncellemesi: **T5.3 tamamlandı**; kullanıcı aracı/registry/recipe/kalibre world Line/Runtime/Logs/CSV ve UI kabulü [Metrology sözleşmesinde](metrology.md). Engine execution/10, güncel binary build/metrology-ui/visionflow.exe. Aşağıdaki T5.1/T5.2 kapsam ve kanıtı tarihsel çekirdek teslimidir; “T5.3’te” ifadeleri o teslimin sınırını anlatır.

9 Ekim 2026. `vision/metrology/LineFitter` sayısal çekirdeğidir; yeni registry kind/recipe config/port, application resolver, UI, Runtime/Logs/CSV ve kalibre çizgi kullanıcı akışı **T5.3**'te kalır. Mevcut [Edge Pair](edge-pair.md), [Edge Point](edge-point.md), UI binary ve execution/9/editor7/terminal3/vf.socket/2 değişmez. Sıradaki adım T5.3 UI/kalibrasyon/kabul.

## Tarama geometri sözleşmesi

`find` raw `RotatedRectangle` ister. Kutunun yerel +X yönü taught tangent `u`, normal `n=(-uy,ux)`; width boyunca scan merkezleri eşit aralıklıdır. İlk/son merkez kutu kenarından `scanWidthPx/2` içeridedir. Her scan yerel −Y→+Y boyunca kutunun height uzunluğundadır; `scanWidthPx` tangent yönünde ortalama şerididir. Böylece bütün şerit footprint'i döndürülmüş kutuda kalır. Raw pixel-centre alanı/frame/format tam denetlenir, clipping/AABB/border padding yoktur.

Her scan aynı [T4 sampler/edge finder](profile-sampling.md) ile çalışır; profile/polarity/first-last-strongest/subpixel/kontrast sözleşmesi korunur. Bulunamayan scan eksik gözlemdir; geçersiz profil/footprint/bütçe ERROR exception'dır, eksik edge diye gizlenmez. Birikmiş bütün profiller bellekte tutulmaz; bir scan'in geçici profili, her scan için optional seçili point ve fit tanısı tutulur. `LineSearch` gerçek FrameMetadata/input content/processing kimliği taşır.

## Fit ve kararlılık

`fit` requested scan sayısı kadar sıralı slot, her slotta optional point ve taught tangent alır. Eksik slotlar çıkarılmaz; scan ID vektör indeksidir. Aynı koordinatlı tekrar gözlem, NaN/Inf, mutlak koordinat >1e12, sıfır/finite olmayan yön veya dejenere veri reddedilir. Numeric API birim dönüşümü yapmaz: noktalar, taught tangent, minSpan ve mesafe limitleri **aynı caller koordinat sisteminde** olmalıdır. World noktalarının mevcut resolver ile doğrulanması/mapping'i T5.3 sorumluluğudur; `find` yalnız px üretir.

1. Noktalar offset/scale ile normalize edilir. Başlangıç `cv::fitLine(DIST_HUBER)`, Huber eşiği `inlierDistance/scale`; solver toleransı 1e-8. Normalize inlier eşiği <1e-12 veya finite olmayan eşik reddedilir; subpixel/fiziksel hassasiyet garantisi yoktur.
2. Varsayılan açık deterministik RANSAC, yerel `mt19937` seed `0x56464c31`, 256 trial (1–1024). İki farklı nokta hipotezi; taught tangent ile cos≥.5 olan yönler. En çok inlier; eşit sayıda en küçük squared orthogonal error; tam tie önceki hipotezi korur. Global OpenCV RNG değiştirilmez. RANSAC kapatılırsa Huber consensus ile devam edilir.
3. Consensus `DIST_L2` ile yeniden fit edilir; her refit **yalnız o consensus noktalarıyla yeniden normalize edilir**. Uzak rejected noktalar son float fitLine temsilinin hassasiyetini düşürmez. Üyelik en çok sekiz refit içinde sabitlenmelidir; aksi halde ret. Final tangent taught direction ile aynı işaretlidir ve cos≥.5 kontrol edilir.
4. Inlier covariance'ın büyük/küçük özdeğer oranı varsayılan en az25; isotropic/kararsız dağılım reddedilir. Raporlanan varianceRatio finite olması için en çok1e15'tir; confidence değildir. Ayrıca minimum uzamsal destek, scan coverage ve residual kapıları zorunludur. Aykırı noktalar fitten çıkarılır fakat residual ve nedenleri tanıda kalır.

## Çıktı ve kabul kapıları

- Sonsuz çizgi `origin + t*tangent` ile; **ölçülen destek** yalnız inlier noktaların çizgi üzerindeki min/max projeksiyonlarından `supportStart/End` ile verilir. Kutunun tamamı ölçülmüş gibi uzatılmaz.
- Angle `atan2(tangent.y,tangent.x)` ile [-180,180); raw Y aşağı olduğu için ekranda pozitif açı saat yönündedir. Taught yön ters çevrilirse tangent/destek uçları ters döner, aynı fiziksel çizgi korunur. Fiziksel world açı T5.3'te world fitten hesaplanacaktır.
- `usedCount`, `rejectedCount` (found outlier), `missingCount` (no edge); her found point için signed perpendicular residual `cross(point-origin,tangent)`, used bayrağı, `outlier/not_found` nedeni. RMS ve max **inlier** artık hatalarıdır; rejected noktalar ayrı açık kalır.
- Coverage = used/requested; spanCoverage = (son used scan ID−ilk used scan ID)/(requested−1). Varsayılan minPoints6, coverage≥.6, spanCoverage≥.5, spatial span≥10, inlier distance≤1, RMS≤.35, max residual≤1; sınırlar dahildir ve `find` için mesafeler px'dir. Bunlar yazılım varsayılanıdır, fiziksel yeterlilik sınırı değildir.
- Yetersiz nokta/coverage/span/condition veya residual/iptal hatası exception üretir; partial geçerli line dönmez. Yeni UI/engine portu veya parça PASS/FAIL/confidence/pose üretilmez.

## Kaynak ve iptal sınırları

En çok4096 scan/nokta, tek image≤16 Mi pixel, width≤1024 px ve kutu width'inden küçük. Sampler tek profil ≤65536 sample/1024 enine sample, ≤4 Mi tap; **bütün scan'ler toplamı** ≤16 Mi intensity tap ve ≤16 Mi convolution tap, edge peak için en az üç türev desteği preflight'ta kontrol edilir. Mevcut sampler scan başına content hash hesapladığı için image pixels×scanCount≤512 Mi ek iş kapısı vardır; bu gate bazı büyük görüntü/çok scan konfigürasyonlarını reddeder. Optimizasyon veya hız vaadi değildir. RANSAC≤1024×4096 sınırlı hypothesis-point işi; refit≤8 ve geçici tanı O(scanCount+tek profil).

Kendi örnekleme/fit üyelik/RANSAC döngülerinde ve OpenCV öncesi/sonrası cooperative iptal kontrolü; OpenCV fitLine/hash hard interruption değildir. Çok scan aynı görüntüyü tekrar hash eder; latency/performans kabulü yoktur.

## Bağımsız yazılım kanıtı

`tests/line_fit_test.cpp` **1155 kontrol**; Release OFF/core tam CTest **36/36**, 19.16 sn. Rapor `build/line-fit-core/line-fit-report.txt`. Analitik yatay çizgi/destek/yön tersleme; simetrik sıfır covariance residual ile bilinen RMS/max; üç outlier ve high-leverage outlier (Huber-only ret, RANSAC15/21 recovery); deterministic tekrar/global RNG korunması; eksik slot, dar count/span coverage, yetersiz spatial span, isotropic/yanlış yön/duplicate/nonfinite/dejenere/büyük offset/parametre/bütçe/iptal.

Bağımsız Gray16 erf raster 192×192: yedi açı × üç subpixel phase × iki polarity = **42 clean**, blurσ1.4, 21 scan, step.5/width2/sigma1, nominal support118 px. Her scan'in noktası ayrıca gerçek analitik çizgiyle sınanır. Line offset RMS **0.00111244 px**, max **0.00234228 px**, angle max **0.000490212°**; .15 RMS/.30 max offset/.1° angle geliştirme kapıları geçti. Ayrı **42 degraded**, blurσ2.6/±6 gray-level noise/seed1234567: offset RMS **0.0329876 px**, max **0.0748507 px**, ayrı .5 px max kapısı.

Gerçek bağımsız bozulmuş raster: 21 scan'de üç displaced edge outlier, iki eksik edge, 16 used; gerçek y95.5 px çizgi korunur. Bu sentetik yazılım kabulüdür. Gerçek üretim texture/multiple parallel edge/yanlış taught polarity, fiziksel doğruluk/K4, kalibre world line, UI/Runtime/kayıt T5.3 ve lensT15/P7/6D.2/kurulum/dağıtım/CUDA kabulü açık.
