# Reçete tool algoritmaları ve OpenCV bağımlılık planı

10 Ekim güncel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim T8.4.1 tarihsel durum: **T8.4.1 — Shape/texture ROI referans sözleşmesi tamamlandı; T8.4 devam ediyor.** Ortak PoseRoiService artık pattern_pose ve shape_pose kabul eder; kaynak kind için doğru method/version zorunludur. Model/ROI/anchor veya matcher türü değişirse teaching geçersizdir. Gerçek iki matcher ile altı geometri, dönmüş scan → gerçek EdgeFinder, shape kontrast tersleme, gerçek tekrarlı/not-found adaylar ve perspektif/lens korumaları: **398 tracking kontrolü**. Programatik servis kabulüdür; Code/Edge/Caliper/Line/Circle/Blob reference picker, recipe/DAG binding, world sampler ve production Runtime bağlantıları sıradaki **T8.4.2** kapsamındadır. Toolbox14/registry16, execution/15 ve Runtime whitelist reddi sürer; UI değişmedi.

10 Ekim T8.3 tarihsel durum: **T8.3 — Shape Match çekirdeği tamamlandı**. `shape_pose` v1 gradient pyramid adayları ve unsigned orientation-aware chamfer refinement ile gerçek raw Pose2D/ayrı scale üretir. Texture NCC ile skor karşılaştırılmaz; yöntem `unsigned-oriented-chamfer-pyramid` v1. Ambiguity/truncation seçimi varsayılan reddeder; genel karar NOT_EVALUATED. Programatik teaching/DAG/TargetOnly/codec teslimi; Toolbox14/registry16, engine execution/15. Özel UI, shape ROI reference ve production Runtime/completion T8.4 kapsamında, Runtime whitelist reddi sürer. Sonraki **T8.4 — Bütün araçlara bağlama**. [Sözleşme ve sınırlar](shape-pose.md).

10 Ekim T8.1 tarihsel durum: **T8.1 — Gerçek Pose2D çekirdeği tamamlandı**. `pattern_pose` v1 programatik teaching/DAG: çok seviyeli açı/ölçek arama, anchor/refinement/NMS, ambiguity ve reject_ambiguous/best politikası. Pose2D gerçek raw açı/anchor taşır; scale ayrı porttur. PartDecision NOT_EVALUATED. Toolbox14 korunur (registry15); yeni özel UI/tracking/production Runtime henüz yok, Runtime açıkça reddeder. Engine execution/14, editor7/terminal3/vf.socket/2 korunur. Sonraki **T8.2 — Pose referanslı ROI**. [Sözleşme ve sınırlar](pattern-pose.md).

10 Ekim T7.2 tarihsel kayıt: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](geometry-measurement-adapter.md).


10 Ekim T7.1 tarihsel kayıt: **T7.1 — Geometrik domain matematiği tamamlandı**. Mevcut framed Point2D/Line2D/Circle2D üzerinde mesafe, signed/absolute X/Y ve dik offset, segment uzaklığı, üç açı semantiği, sonsuz çizgi kesişimi/paralel aralık ve radius/diameter seçimi. Domain çekirdeğidir; yeni reçete aracı/UI/Runtime bağlantısı yok. Engine execution/11 ve mevcut UI build/circle-ui/visionflow.exe korunur. Sıradaki **T7.2 — Kalibre adapter**. [Sözleşme ve sınırlar](geometry-measurements.md).


10 Ekim T6.4 tarihsel kayıt: **T6.4 — Circle / Arc UI ve kalibre Runtime tamamlandı**. Toolbox `circle_fit` v1; raw/world halka-yay öğretme, tutamaçlar, gerçek mm düzleminde fit ve raw inverse overlay. Ayrı çap/sampled sapma dahil limitleri; kalite yoksa NOT_EVALUATED, outlier kusurdan çıkarılmaz. TargetOnly, immutable Runtime, tamper replay, typed Logs/restart/CSV ve Stop bağlıdır. Engine **execution/11**, editor7/terminal3/vf.socket/2 korunur; world ROI opt-in schema2. Güncel Release UI `build/circle-ui/visionflow.exe`. Sonraki **T7.1 — Geometrik domain matematiği**. [Kullanım, birimler ve sınırlar](circle-tool.md).

10 Ekim T6.3 tarihsel çekirdek kaydı: **T6.3 — Circle kalite/metrikler tamamlandı**. Fit dışı bulunan noktalar dahil radial min/max, signed inward/outward ve peak-to-valley; found coverage/nominal gap kapıları, full-circle/observed-arc/unavailable kapsamı. Circularity yalnız eksiksiz gerçek basit kapalı polygon'dan 4*pi*A/P²; missing/arc/self-intersection için unavailable, sahte kapanış yok. Yalnız çekirdek; Circle UI/registry/kalibre Runtime/Logs T6.4'tedir. Mevcut UI `build/metrology-ui/visionflow.exe`, execution/10/editor7/terminal3/vf.socket/2 korunur. Release OFF/core **40/40**, yeni metrik592 kontrol. Sonraki **T6.4 — UI ve kalibre Runtime**. [Sözleşme ve kanıt](circle-metrics.md).

10 Ekim T5.3 tarihsel kayıt: **T5.3 — UI/kalibrasyon/kabul tamamlandı**. Toolbox Edge Pair / Caliper ve Line Fit; reçeteye bağlı ok/kutu/Properties, gerçek px veya güncel kalibrasyonla mm, world noktalarından yeniden çizgi fit ve fiziksel açı, dahil tool tolerans Boolean’ı ve raw destek/outlier overlay. TargetOnly, immutable Runtime, typed Logs/restart/CSV ve Stop bağlıdır. Genel parça kararı/confidence/pose üretilmez. Engine **execution/10**, editor7/terminal3/vf.socket/2 korunur. Güncel Release binary `build/metrology-ui/visionflow.exe`; kullanıcı uygulaması/config korunur. [Kullanım, portlar, 128-scan reçete sınırı ve kanıt](metrology.md). Sonraki **T6.1 — radyal tarama**; fiziksel K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA kabulü açık. Aşağıdaki kayıtlar tarihseldir.

9 Ekim güncel durum: **T3.3 — Kod Oku UI ve uçtan uca kabul tamamlandı**. Toolbox Code Read; sekiz format seçimi, reçeteye bağlı raw rectangle ROI, exact/prefix/UTF-16 length/bounded regex, adet ve all/any/seçili sıra ayarları sunar. Editor/Runtime gerçek bounding polygon ve sıra/format/escaped metin/byte listesini gösterir; confidence veya genel parça kararı üretmez. Save/reopen/no-op, TargetOnly/Stop, immutable Runtime ve kalıcı Logs/CSV doğrulandı. Dokuz Toolbox girdisi; ZXing OFF kod girdisi devre dışıdır. Release ON/UI 34/34, OFF/core 31/31; güncel binary `build/preprocess-code-ui/visionflow.exe`. Sonraki **T4.1 — keyfi açılı profil sampler**. Karışık polariteli çoklu 1D tamlığı, fiziksel/hasarlı/DPM corpus açık; length mevcut sürümde UTF-16 birimidir. Editor 7, execution/8, terminal 3 ve vf.socket/2 korunur.

9 Ekim T0.3/T1.1/T1.2: offline ürün capability/eksik bağımlılık kapısı ve typed codec/migration tamamlandı. [Uygulanan sözleşme](tool-values.md). T0.2 reader seçimi kesinleşmiştir; alttaki açık capability/sürüm seçimi ifadeleri tarihsel plan notlarıdır. Yeni code backend algoritması T3, OCR T11, lens T15'tedir.

9 Ekim 2026. [Ana planın](recipe-tools-plan.md) teknik eki. Kararlar uygulama hedefidir; **header/modül gözlemi ile compile/link/run veya fiziksel kabul aynı şey değildir**. Lens düzeltmesi T15'e kadar uygulanmaz.

## 1. Doğrulanan yerel envanter

Bu plan oturumunda dosya incelemesiyle görülenler:

- `CMakeLists.txt`: C++20; Qt Core/Gui/Concurrent/Multimedia/Network ve UI'da Widgets. OpenCV REQUIRED bileşenleri `core imgproc imgcodecs objdetect calib geometry`.
- `C:/OpenCV/5.0.0/x64/mingw/lib/OpenCVConfig.cmake`: sürüm 5.0.0, shared; mevcut bileşenler `calib core features flann geometry imgcodecs imgproc objdetect stereo`. `features`/`flann` kurulu ancak proje bunları açıkça talep etmiyor.
- `opencv2/objdetect.hpp` ve `opencv2/objdetect/barcode.hpp` mevcut. QR/Barcode API varlığı decode performansını veya bütün format desteğini kanıtlamaz.
- Yerel `opencv2/geometry/2d.hpp`: fitLine, fitEllipse/AMS/Direct, moments, minAreaRect, minEnclosingCircle, matchShapes, arcLength, contourArea, pointPolygonTest. Bunları OpenCV 4.x dokümanındaki imgproc modül adıyla varsayma.
- Yerel `opencv2/geometry/3d.hpp`: findHomography, estimateAffinePartial2D, undistortPoints. `calib.hpp`: calibrateCamera. `imgproc.hpp`: remap/warp, filtreler, Canny, HoughCircles, connectedComponentsWithStats, findContours, GeneralizedHough ve distortion map API'leri.
- Kurulu bileşen listesinde `text`, `dnn`, `wechat_qrcode`, `ximgproc`, CUDA modülleri yok. ZXing/Tesseract'ın kullanılabilirliği bu oturumda build ile doğrulanmadı. T0/T11 ayrı bağımlılık kabulü yapacak.

T0 probe'ları include, imported target, DLL load ve gerçek küçük fixture çağrısını ayrı raporlar. `cv::getBuildInformation()` sonucu `build/tool-probes/` altında tutulur; QR decoder için derleme seçeneği/bağımlılık kaynaklı runtime eksikliği ayrıca yakalanır. Mevcut kurulumu değiştiren toplu OpenCV rebuild başlangıç gereksinimi değildir.

9 Ekim **T0.1 kabulü** bu dosya envanterini compile/link/run ile doğruladı: C++20/GCC 13.1.0 x86_64 ile OpenCV 5.0.0 shared (kütüphanenin kendi build standardı C++17). Filtreler, bilinear remap, fitLine, fitEllipse/AMS/Direct, contour/moments ve bağımsız noktalı homography probe'ları geçti. QRCodeDetector kendi encoder'ından üretilen fixture'da `VISIONFLOW-T0.1` ve dört köşe döndürdü; boş görüntü no-read. Bu yalnız API smoke kabulüdür, bağımsız reader corpus'u değildir. Envanterle modül farkı yok; headers/import libs/DLL SHA-256, CMake link yolları ve runtime build-info `build/tool-probes/` altında. [Komutlar ve sınırlar](../toolchain.md#t01--opencv-api-ve-baseline-kabulü); ZXing seçimi/bağımsız fixture kabulü T0.2'de açık.

## 2. OpenCV / harici kütüphane / özel kod matrisi

Modül sütunu **yerel OpenCV 5** içindir. API açıklamalarının bir bölümü resmi 4.x belgelerinden araştırılmıştır; yerel compile probe son sözü söyler.

| İş | Kullanılacak temel | Yerel modül / harici | VisionFlow'da geliştirilecek bölüm |
| --- | --- | --- | --- |
| Gray, blur, threshold, morphology | cvtColor, GaussianBlur, medianBlur, threshold/adaptiveThreshold, morphologyEx | imgproc | Sıralı filtre parametreleri, owned output, provenance, mask/border ve cache |
| Keyfi açıda 1D kenar | remap veya kontrollü double bilinear sampler, reduce/separable filter | imgproc/core | Ok geometrisi, signed profile derivative, aday/polarity seçimi, subpixel, validity |
| Edge pair / caliper | Aynı 1D altyapı | imgproc/core | Zıt polarite eşleştirme, width/centre ve gap kuralları |
| Line | fitLine + solve/SVD gerektiğinde | geometry/core | Çoklu scan, RANSAC/robust seçim, destek segmenti ve residual/kapsam |
| Circle/Arc | solve/SVD; HoughCircles yalnız aday karşılaştırması | core; isteğe bağlı imgproc | Radyal sample, geometrik circle fit/refinement, condition, coverage, sapma |
| Ellipse | fitEllipse/AMS/Direct | geometry | İleride bağımsız elips tool'u; Circle'ı elips eksen ortalamasına dönüştürme yok |
| Kalibre geometri | Mevcut MeasurementService/PlaneMappingService + transform/solve | core/geometry | PointSet mapping, world fit ve frame/birim/domain denetimi |
| QR | QRCodeDetector referans probe; ürün için ZXing reader | objdetect / ZXing-C++ | Reader adapter, format seçimi ve ortak sonuç sözleşmesi |
| Data Matrix/Code128/Code39/EAN/UPC | ZXing-C++ | Harici native C++ | Sonuç normalize etme, bytes/text, ROI ve içerik kuralları |
| Açı/ölçek Pattern Match (T8.1 çekirdek tamamlandı) | pyrDown, warpAffine, matchTemplate, minMaxLoc | imgproc/core | Masked CCOEFF_NORMED, coarse-to-fine, anchor/refinement/NMS/ambiguity; tracking/UI/Runtime ayrı |
| Shape Match (T8.3 çekirdek tamamlandı) | Gaussian/Sobel, gradient pyramid, distanceTransform | imgproc; geometry | Unsigned8bin chamfer/raw refinement; shape/texture ayrı score kimliği, GeneralizedHoughGuil opsiyonel uygulanmadı |
| Feature match alternatifi | ORB/SIFT, BFMatcher; estimateAffinePartial2D | features + geometry | Yalnız veriyle gerekirse ayrı backend; düz metal konturda zorunlu değil |
| Blob/shape | connectedComponentsWithStats/findContours + moments/area/perimeter/hull | imgproc + geometry | Nesne listesi, hole/border politikası, dünya alanı ve filtreler |
| OCR | Tesseract C++ API + Leptonica | Harici | ROI pipeline, model assets, text/confidence/box, iptal/timeout politikası |
| OCV | OCR sonuçları + Qt string/regex | Qt Core | Beklenen text, format, normalizasyon ve karakter farklılık kuralları |
| Renk | cvtColor, inRange, meanStdDev | imgproc/core | HSV wrap/achromatic, Lab float normalizasyonu, CIE76, referans öğretimi |
| Kontur/yüzey kusuru | absdiff, threshold, morphology, distanceTransform, components | core/imgproc/geometry | Referans hizalama, signed defect, ignore masks, tolerans ve kusur listesi |
| Bead | 1D pair sampler + eğri geometrisi | Ortak özel kod + core/geometry | Normal taramalar, width/offset, gap/length ve köşe davranışı |
| Karar | Sınırlı typed rule evaluator | C++/Qt; OpenCV gerekmez | Validity önkoşulları, reason tree, immutable replay ve mapping |
| Lens (T15) | calibrateCamera, undistortPoints, initUndistortRectifyMap/remap | calib + geometry + imgproc | Çoklu observation, yeni codec, holdout, raw/undistorted/plane zinciri |

OpenCV API'leri yapıtaşıdır; endüstriyel caliper, circle metrology, bead, pose tracking ve karar sözleşmeleri tek hazır çağrıyla tamamlanmaz. Yerel incelenen API'lerde planın gerektirdiği robust circle-fit + coverage + roundness paketinin karşılığı bulunmadı; kendi kontrollü C++ solver'ımız olacak.

## 3. Bağımlılık kararı

9 Ekim **T0.2 kabulü**: ZXing 2.3.0 (`d6068bcebeb8fd9f0d35a99b00d202be86a14dbe`) static reader-only MinGW GCC 13.1/C++20 Release doğrulandı. Kaynak archive SHA-256, gerçek CMake seçenekleri ve Apache-2.0/embedded libzueci BSD-3-Clause/DFA attribution kaydı [manifestte](../../third_party/manifest.json). Bağımsız Zint 2.15.0 yalnız fixture generator'a bağlanır; ZXing writer/örnek/test/binding kapalıdır. Sekiz fixture/33 kontrol ve indirmesiz cache bootstrap geçti; [kanıtlar](../toolchain.md#t02--zxing-reader-only-bağımlılık-kabulü). Aşağıdaki başlangıç kararının sürüm/static seçimi bu kabul ile kesinleşti. T0.3 ürün capability/eksik bağımlılık kapısı açık; bu probe ürün code tool'u değildir.

### 3.1 Barkod

Resmi OpenCV barcode tutorial EAN-8/EAN-13/UPC-A/UPC-E desteğini listeliyor; bu nedenle QR dışındaki geniş kapsamı BarcodeDetector varlığına dayanarak vaat etmiyoruz. **Ürün reader backend'i ZXing-C++**: QR Model 2, Data Matrix ECC200, Code128, Code39, EAN8/13, UPCA/E test edilmiş allow-list. OpenCV QR yalnız T0 kıyas/probe olabilir; runtime'da sonuç veya engine kimliğini değiştiren gizli fallback yok.

ZXing upstream release/commit ve kaynak SHA-256 T0.2'de sabitlendi; static reader-only, örnekler/writer/binding paketleri kapalı. Kullanılan sürümün gerçek CMake seçenekleri kaynakta ve MinGW probe'da doğrulandı; manifest uygulanmış ayarları taşır. MSVC binary'si MinGW ile karıştırılmaz. License/NOTICE, embedded libzueci ve runtime imports listesi toolchain kaydındadır. Normal probe configure local prefix kullanır; en yeni master'ı indirmez. Cache arşivlerinden indirmesiz bootstrap tekrarı geçti; temiz makine ve ürün capability kabulü T0.3'te kalır.

Harici reader kullanılamazsa OpenCV EAN/QR alt kapsamı bütün code tool'un tamamlanması sayılmaz. Geçici capability yalnız gerçekten doğrulanan formatları bildirir; kabul edilen ana kapsam eksik kalır.

### 3.2 OCR

Kurulu OpenCV'de text modülü yok. `cv::text::OCRTesseract` da Tesseract bağımlılığını ortadan kaldırmıyor. Bu nedenle **doğrudan Tesseract C++ adapter** tercih edilir; yalnız OCR için tüm OpenCV/contrib rebuild zorunlu kılınmaz. Tesseract + Leptonica + seçilmiş tessdata dosyalarının sürüm/hash/lisansları T11.1'de sabitlenir. Varsayılan başlangıç Latin harf/rakam için eng modeli; Türkçe metin gerekiyorsa tur modeli ayrı asset ve fixture olarak eklenir. Dil/model seçimi reçetede açık.

MinGW ABI veya bağımlılık build'i gerçek engel çıkarırsa ikinci seçenek sabitlenmiş Tesseract CLI worker adapter'ıdır: shell kullanmadan QProcess argüman listesi, sınırlı TSV/çıktı, build/app-data altında görev bazlı geçici dosya, iptal/timeout/process cleanup ve quoted path testleri. Bu seçenek sessizce native adapter'ın yerine geçmez; backend ID/sürüm ve test matrisi değişir. Paddle/ONNX/DNN yeni bir ürün/backend kararı olmadan eklenmez.

## 4. Sampling ve Edge ayrıntıları

T4.1 ve T4.2 uygulandı: [double profil sözleşmesi](profile-sampling.md), [EdgeFinder seçim/refinement/epsilon/birim sözleşmesi](edge-point.md). `ProfileSampler` ham/optional smooth yoğunlukları ve tam destekli midpoint signed derivative üretir. Genişlikte iki sınır dahil eşit ağırlıklı ortalama, gerçek step üzerinden normalize 3σ Gaussian; border padding yoktur. Gray16 double ölçeklenir. Okla UI/profil grafiği T4.3'te kalır.

Scan tanımı A→B, uzunluk L, birim yön u=(B−A)/L, normal n=(-uy,ux), fiziksel anlamı raw piksel koordinatıdır. `p(s,t)=A+s*u+t*n`, s başlangıçtan sona artar. Şerit genişliği, sampling step ve smoothing sigma piksel biriminde kaydedilir. Bilinear footprint'in bütün komşuları geçerli olmalı; sınırda border replicate/0 padding kullanıp dışarıdan kenar yaratma. Yetersiz valid profil explicit invalid verir.

İlk uygulama kontrollü CV_64F bilinear sampler olabilir; cv::remap hız alternatifi ayrıca aynı fixture ile kıyaslanır. Bazı interpolation yollarındaki katsayı nicemlemesi subpixel sonucu etkileyebilir; yöntem değişikliği sürümlenir. Şeritte ortalama profil → tarifli smoothing → signed derivative; threshold türev genliği birimindedir, legacy gray8 komşu farkıyla aynı kabul edilmez.

Rising/falling **tarama yönüne göredir**. Ok ters çevrilince signed gradient değişir; fiziksel aynı kenarı bulmak için polarity de değişebilir. First/last tarama sırası, strongest mutlak gradient; eşitlikte ilk s. Çok örnekli tepe plateau politikasını testle sabitle. Peak'in üç komşusuna ikinci derece refinement yalnız iç noktada, yeterli eğrilik ve sınırlı offset ile uygulanır; geçersiz refinement ayrı kalite bayrağı veya açık non-subpixel mode, sessiz hassasiyet iddiası yok.

Subpixel interpolasyon ölçüm doğruluğu garantisi değildir. Edge profil grafiği ham/smooth/gradient eğrilerini ve seçimi gösterebilir. Çoklu kenarlar aynı çekirdekle üretilir; ilk UI tek nokta, pair ve line/circle kullanıcılarının ihtiyacı kadar parametre sunar.

## 5. Çizgi, daire ve kalibrasyon

### 5.1 Çizgi

Birden çok paralel scan'den gelen valid nokta seti fitLine'a gider. En az örnek sayısı, scan coverage, min spatial span, residual ve model condition kabul parametreleridir. Line tangent işareti taught direction ile deterministik tutulur; açı [-180,180) standardıyla raporlanır. Destek segmenti ile sonsuz matematiksel çizgi ayrı port/metadata semantiğidir. RANSAC gerekiyorsa sabit local RNG seed, bounded iterations ve iptal noktaları kullanılır; global OpenCV RNG diğer tool'larla yarışmaz.

### 5.2 Daire ve yay

1. Nominal halka/yay üzerinde düzenli scan açıları üret; içine/dışına doğru T4 edge bul. Scan ID, angle, valid/missing, point, strength ve seçim nedeni korunur.
2. Pixel debug'da raw noktalarla pixel fit. Calibrated mode'da bütün valid ölçüm noktalarını mevcut calibration resolver ile mm'ye çevir; fit ve residual **bu düzlemde**. İptal/current/frame/domain hatasında mm sonucu yok.
3. Noktaları merkezleyip ölçekle; algebraic least-squares başlangıcını SVD ile çöz. Ardından geometrik amaç `sum w_i*(||p_i-c||-r)^2` için sınırlı Gauss–Newton/LM refinement uygula. Robust weighting/RANSAC yalnız fit modelini kararlı bulmak içindir. Radius pozitif finite, Jacobian rank/condition, iteration limit ve yakınsaklık denetlenir.
4. Teorik üç nokta circle için yeterli olsa da ürün kabulü için yeterli değildir. Min valid scans, min angular span, max consecutive gap, residual ve condition konfigürasyonu zorunlu. Dar yayda kararsız büyük yarıçap sonucu kabul edilmez. Başlangıç fixture profilinde full circle için örneğin 72 scan / en az %90 kapsam / en çok 15° boşluk; bunlar ürün varsayılanı için deneyle doğrulanacak değerlerdir, fiziksel yeterlilik standardı değildir. Arc ayrı minSpan ve condition ile değerlendirilir.

Perspective ROI: world düzlemde nominal centre/radius/annular width tanımlanır. Taught raw kontur/seed noktaları world'e çevrilerek başlangıç geometri çıkarılır; dairesel raw ROI'nin perspektifte zorunlu daire olduğu varsayılmaz. World radial scan örnekleri H'nin kontrollü tersiyle raw'a taşınır, raw yoğunluk orada örneklenir. Valid domain dışındaki footprint reddedilir. Overlay world fit örneklerini raw'a geri taşır; doğrudan piksel circle çizmek yerine gerekirse polygon/conic görünür.

`PlaneMappingService::map` raw→world sağlar; T6.1 inverseMap finite/singularity/horizon/bounds/hull/roundtrip kontrollü world→raw API ekledi. Uniform/manual inverse scale ve target Y-axis kullanır. Her world intensity tap ayrı raw koordinatına taşınır; [sözleşme](radial-scan.md). OpenCV undistort çağrıları bu yola T15'ten önce eklenmez.

### 5.3 Dairesellik çıktı tanımları

| Alan | Tanım | Kapsam |
| --- | --- | --- |
| diameterMm | 2*r (world fit) | Valid calibrated fit |
| centreMm | Fit merkezinin world frame koordinatı | Centre/domain politikası geçerli |
| fitRmsMm | Fit için kullanılan noktaların radyal residual RMS'i | Nokta kümesi/method ile kayıtlı |
| radialPeakToValleyMm | Aynı fit merkezine uzaklıklarda max(ri)−min(ri) | Bütün valid ölçülmüş kontur noktaları; fit outlier'ları dahil |
| maxInward/OutwardMm | r−ri ve ri−r yönlü maksimum sapmalar | Negatif değer yerine ilgili yön için max(0,...) |
| coverageRatio / maxGapDeg | Valid scan sayısı/toplam ve dairesel sıralamada en uzun eksik yay | Eksik/kötü kontrast scan dahil |
| circularityIndex | Kapalı basit konturda `4*pi*A/P²` | Aynı koordinat uzayında area/perimeter; eksik yayda yok |

Fit inlier listesi ile defect sample listesi ayrıdır; outlier elemek çapı iyileştirirken radialPeakToValley'yi yapay küçültmez. Eksik scan için nokta uydurulmaz. Full-circle coverage başarısızsa fit tanı için görülebilir, tam çevre dairesellik ölçümü invalid ve acceptance NOT_EVALUATED olur. Arc residual'ı yalnız görülen yay adıyla raporlanır.

Circularity, ideal fit dairesinden hesaplanmaz (aksi halde her zaman 1 olur); gerçek kapalı konturdan gelir. Eksik örnekler otomatik kapatılmaz. Self-intersection, yetersiz örnek ve sıfır çevre reddedilir. Aynı polygon'dan A ve P üret; pixel-count area ile contour perimeter karıştırma. Raster/sampling kaynaklı aralık sapmalarını saklama: ham değer ve geçerlilik korunur, yalnız görsel clamp varsa karar ham tanımlı değerden veya açık invalid'den gelir. Delikli Blob için dış kontur index ve hole ölçütleri ayrı; HALCON circularity ile sayısal eşitlik beklenmez.

Peak-to-valley, burada **fit merkezine göre radyal sapmadır**. Minimum-zone (iki eşmerkezli daire arası en küçük bant) optimizasyonu ve standart GD&T dairesellik bu sürümün kabulü değildir. Fit RMS veya calibration holdout RMS tool'nun fiziksel belirsizliği olarak sunulmaz.

### 5.4 Genel geometri ve alan

World fit için PointSet kaynak örnekleri taşınır; homography altında `map(centerPx)` ve `radiusPx*scale` kullanılmaz. Nokta/line/circle feature'ları provenance ve destek alanıyla bağlanır. Doğruların kesişimi destek/kalibrasyon alanı dışındaysa türetilmiş ölçüm politikası explicit ret verir. Paralel olmayan sonsuz iki çizgi arası mesafe tek sayı olarak anlamlı bir gap değildir; intersection/angle veya tanımlı finite destek mesafesi seçilir.

Blob px², mevcut kabul edilen hücre sayısıdır. Uniform alan her piksel için scale² ile bulunabilir. Homography alanı için kabul edilen hücre köşeleri (pixel centre ±0.5) world'e taşınıp quadrilateral alanları toplanır; delikler dışarıda kalır, katlanma/singularity/domain aşımı reddedilir. Kısmi sınır hücresi otomatik kırpılıp daha küçük alan diye geçmez. Bu maliyet ayrıca benchmark edilir; contour alanı alternatifse farklı metric/method adı gerekir.

## 6. Pose/ROI ve ön işleme kökeni

ImageFrame raw buffer değişmez. Preprocess çıktı aynı acquisition frame/cycle'ı korur ama ayrı processing identity/chain hash taşır; contentId ve tool signature raw'dan türetilen işlem sırasını da kapsar. Image/frame kimliğini değiştirip stale veri aynı kareymiş gibi etiketleme yok. Geometri koruyan filtre raw koordinatları kullanır. Yeni uzay üreten transform (lens, rescale/crop) açık mapping ve validity mask gerektirir; T2 genel filtresinde kapsam dışıdır.

Pose2D translation/rotation taşır. Match aramasındaki scale, rigid Transform2D'ye gizlice eklenmez; ayrı similarity transform tipi ve kabul edilmiş scale range gerekir. İlk tracking sürümü rigid olabilir; o durumda matching scale yalnız diagnostic olup scale≠1 olan aday tracking için reddedilir. Scale tracking desteği T8.2 tamamlanma kriterine dahil edilecekse ayrı typed Similarity2D ve testleri aynı alt pakete eklenir; bu planın varsayılanı **rigid tracking, scale aramasında ayrı sonuç**tur.

Parça image pose'u pixel ROI'lerini taşır; metric ROI'lerde teach/current referans geometrisi world'e dönüştürülerek world rigid alignment hesaplanır. Perspective H üzerinden image angle'ı world açısı diye kullanma. T8.2 için alternatif güvenli yol world nominal ROI→raw mapping zinciridir; seçilen yol model metadata'sıyla sabitlenir. Referans bulunamazsa araç SKIPPED/invalid, eski veya absolute ROI'ye fallback yok.

Template mask, sabit model variance/kontrast, rotation padding ve arama alanı önemlidir. Tamamen sabit görüntüde NCC'nin sayısal davranışı yüksek skor diye kabul edilmez. Chamfer score ile NCC score karşılaştırılmaz; yöntem ve threshold farklıdır. Simetrik modelde seçilen açı bir robot pose kanıtı değildir.

## 7. Code / OCR / karar ayrıntıları

CodeResults: bounded vector; item ID o çevrim içinde deterministik, symbology, text, raw bytes, encoding/GS1 metadata mevcutsa, corners, decoded flag/error. Text'in boş olması tek başına no-read testi değildir; backend success ayrı taşınır. Unicode, embedded NUL ve group separator bytes kayıtta korunur; insan görünümünde escaped sunulur. Herhangi bir kod metni komut/URL/reçete seçimi olarak otomatik yürütülmez.

Reader count sınırı aşılırsa eksik listeden PASS verilmez; limit-exceeded açık olur. Bir decoder aday konumu sağlayamıyorsa `found=false` fiziksel olarak hiç sembol yok anlamına gelmez; kullanıcıya “okunamadı” sunulur. Generic confidence yok; backend gerçekten sağlayan OCR confidence ayrı yöntem aralığında saklanır. Uygulanan code_read v1 length kuralı açıkça UTF-16 code unit sayar; UI birimi gösterir ve supplementary karakter testi bu sözleşmeyi sabitler. Unicode code point varsayılanı bu sürümde uygulanmaz: eski reçetelerin karar semantiği değiştirilmedi. Gelecekte code point seçeneği ayrı sürüm/migration gerektirir. Raw byte uzunluğu sonuç listesinde ayrı gösterilir.

Kabul mantığı: her rule mandatory input validity kontrolünü önce yapar. Missing/ERROR/SKIPPED veya timeout girdisi `NotEvaluated` ve reason üretir. İlk sürümde any/all içinde invalid gerekli giriş varsa bütün Result NotEvaluated; diğer kural PASS'i bunu gizleyemez. Bütün girişler valid olduktan sonra Boolean cebiri yürür. Presence yokluğu ve no-read, arama başarıyla tamamlanmışsa geçerli `false` olup beklenen varlık/okuma kuralını FAIL yapabilir. Timeout'ta tamamlanmayan arama no-read/FAIL değildir.

Kurallar literal tolerance/type/frame ile validate edilir; NaN/Inf, ters sınır, bilinmeyen port, boş final rule reddedilir. Arbitrary eval/script yok. Regex Qt Core ile sınırlı pattern/input uzunluğu ve adversarial testler gerektirir; cooperative Stop regex çağrısını kesin süre içinde durdurma garantisi değildir. Gerekiyorsa güvenli regex altkümesi/ayrı worker bütçesi tasarlanır.

## 8. Kusur, renk ve bead kapsamı

Renk için Lab dönüşümünde CV_32F normalize RGB/BGR aralığı belgelenir; uint8 Lab kodlarını fiziksel L*a*b* gibi kullanma. CIE76 Euclidean delta-E başlangıçtır; CIEDE2000 gerekirse ayrı testli matematik paketi. Hue 0/360 wrap, düşük saturation ve parlama için mask/coverage kuralları bulunur.

Reference inspection piksel farkını doğrudan kusur saymaz: referans alignment kabul edilmiş, ignore mask ve aydınlık normalizasyonu tanımlı olmalı. Shape distance ve signed intensity difference ayrı metric'tir; öğrenilmiş anomaly detection iddia edilmez. Frame'ler aynı recipe/cycle bağlamındaki model asset ve acquired image olarak ilişkilendirilir, reference asset acquisition frame diye yeniden etiketlenmez.

Bead yolu için normal/tangent sürekli ve yönlü tanımlanır; keskin köşede miter/ayrı segment politikası, self-intersection için açık ret veya ayrı kollar. Gap uzunluğu yol üzerindeki eksik segment uzunluğudur; görüntüde iki endpoint arasındaki chord değildir. Eğri width, kendi kesit normalindeki edge-pair uzaklığıdır. World yol ve örnek noktalar ile mm hesaplanır; inner/outer daire çap farkının yarısı eksantrik bead genişliği yerine kullanılmaz.

## 9. Bütçe ve hata sınırları

Başlangıç geliştirme sınırları: tool ROI en çok mevcut Blob sınırı olan 16 Mi piksel; 1D scan en çok 16384 sample ve 256 genişlik sample'ı; shape scan sayısı en çok 4096; code sonuç sayısı en çok 256, payload en çok 64 KiB/item ve toplam 1 MiB; OCR text en çok 64 KiB; profile/result koleksiyonları için toplam 16 MiB/tool. Bunlar uygulanmadan ölçülecek ve gerekirse gerekçeyle revize edilecek **kaynak bütçeleridir**, performans vaadi değildir. Çarpım/bellek hesabı overflow kontrollü yapılır; nested scan×sample×width toplam bütçesi ayrıca sınırlandırılır.

İlk toplam sampling bütçesi 16 milyon intensity sample/tool; contour/point set en çok 262144 point/tool. Limit aşımı explicit ERROR; sessiz truncation ile adet/kapsam/ölçüm kararı verilmez. Diagnostic grafik decimation yalnız görünümde, hesap ve kaydedilen scalar sonuç değişmeden yapılabilir. Production JSON için T1.3 ayrı bounded evidence summary/hash politikası belirler; raw image, dev mask ve bütün profile dizileri cycle JSON'a kontrolsüz gömülmez. Denetim için gereken metrik/validity/coverage/kural inputları korunur.

Her library çağrısı hard-cancellable değildir. Ön/son cancel ve deadline kontrolü, kendi döngülerimizde düzenli poll; deadline sonrasında bulunan sonuç yayımlanmaz. Hard timeout gerekiyorsa süreç izolasyonu ayrı tasarım ve kabul ister. OpenCV assertion/SDK exception→structured error; output/PASS temizliği ve diğer bağımsız dalların mevcut engine davranışı korunur.

## 10. Lens — yalnız T15

Başlangıç lens modeli pinhole Brown–Conrady; çok konum/açılı observation ile intrinsic K ve radial/tangential coefficients. Planlanan API calibrateCamera; farklı solver/model seçenekleri tek sürüm gibi saklanmaz. Holdout görüntüleri fit'te kullanılmaz, gerçek mm mesafeleri ayrıca doğrulanır. Model fit RMS tek başına fiziksel kabul değildir.

Raw→undistortPoints (çıktı normalized mı rectified-pixel mı açık)→yeniden öğretilmiş plane mapping. Image remap yolu seçilirse rectified image ayrı coordinate frame ve validity mask taşır, bütün tool/overlay eşlemesi buna uyar. Eski raw homography'ye undistorted pixel vermek yasaktır. Lens/focus/resolution/setup değişimi model binding'i invalid eder. Eski uniform/manual/raw-H kayıtları olduğu gibi okunur; yeni combined model için ayrı schema/identity ve compatibility fixture gereklidir.

## 11. Araştırma kaynakları

9 Ekim 2026 tarihinde incelendi. API sayfaları yerel 5.0 ABI testi yerine geçmez; üçüncü taraf ürünlerin tasarımları örnektir, entegrasyon/lisans satın alma kararı değildir.

- [OpenCV QRCodeDetector](https://docs.opencv.org/4.x/de/dc3/classcv_1_1QRCodeDetector.html) ve [barcode desteği](https://docs.opencv.org/4.x/d6/d25/tutorial_barcode_detect_and_decode.html): QR ve sınırlı EAN/UPC reader yaklaşımı.
- [OpenCV shape/geometri yapıtaşları](https://docs.opencv.org/4.x/d3/dc0/group__imgproc__shape.html), [remap/geometrik dönüşümler](https://docs.opencv.org/4.x/da/d54/group__imgproc__transform.html), [Hough circle](https://docs.opencv.org/4.x/d4/d70/tutorial_hough_circle.html): temel fonksiyonlar; metrologi sözleşmesi projeye aittir.
- [OpenCV calibration](https://docs.opencv.org/4.x/d9/d0c/group__calib3d.html): intrinsic/lens yapıtaşları, T15.
- [ZXing-C++ upstream](https://github.com/zxing-cpp/zxing-cpp): C++ reader ve desteklenen sembolojiler; kesin bağımlılık sürümü T0.2'de sabitlenecek.
- [Tesseract upstream](https://github.com/tesseract-ocr/tesseract), [C++ API örnekleri](https://tesseract-ocr.github.io/tessdoc/APIExample.html), [OpenCV OCRTesseract bağımlılığı](https://docs.opencv.org/4.x/d7/ddc/classcv_1_1text_1_1OCRTesseract.html).
- [HALCON 1D Measuring](https://www.mvtec.com/doc/halcon/2511/en/toc_1dmeasuring.html), [2D Metrology](https://www.mvtec.com/doc/halcon/2511/en/toc_2dmetrology.html), [circularity tanımı](https://www.mvtec.com/doc/halcon/2511/en/circularity.html): kenar/fit/ölçüm ayrımına referans.
- [NI Vision Builder araçları](https://www.ni.com/pdf/manuals/374910a.html), [NI uygulama sınıfları](https://www.ni.com/en/shop/electronic-test-instrumentation/application-software-for-electronic-test-and-instrumentation-category/what-is-vision-builder-for-automated-inspection/10-considerations-when-choosing-vision-software.html).
- [Zebra Aurora 1D Edge Detection](https://docs.adaptive-vision.com/5.6/studio/filters/1DEdgeDetection/index.html), [Zebra measurement/bead/metrology kapsamı](https://www.zebra.com/content/dam/zebra_dam/en/tech-specs/imaging-library-tech-specs-en-us.pdf).
