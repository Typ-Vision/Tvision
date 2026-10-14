# Reçete tool test, doğrulama ve teslim planı

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](reference-production-acceptance.md). [R8 paket ve devir kanıtı](reference-release-acceptance.md).


11 Ekim ek kabul planı — [Ortak Referans R1–R8](reference-system-plan.md) için Point/Pose kaynak matrisi, yedi ROI tüketicisi, bağımsız X/Y/açı ofset oracle'ları, gerçek raster/maskeler, non-affine world mapping, eski no-op/hash ve atomik migration, zincir/cache/Stop, normal/küçük UI ve production replay/CSV kabulü tanımlandı. Paket/test eşlemesi yeni planın 6. bölümündedir. Üç applicationType aynı servisle sınanır; Point zinciri açı miras almaz. R1–R4 ve R5a/b testleri çalıştırıldı; gerçek komut/sonuçlar [progress](../progress.md) içindedir. Birleşik Editor R6, production R7 ve nihai tam paket R8 tamamlandı; kalan corpus/production kabulü T8.4.2b.3c açıktır. R7 kanıtı T8.4.2b.3c nihai corpus/production kabulünde tekrar uygulama olmadan kullanılır.

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

9 Ekim T4.3 tarihsel kabul: **T4.3 — Editor/Runtime/Measurement tamamlandı**. Toolbox Edge Point, reçeteye bağlı A→B ok çizimi/tutamaç/sayısal koordinatlar/ters yön ve polarity/selection/subpixel/örnekleme ayarları; gerçek ham/Gaussian/signed türev grafiği ve kabul/ret/seçili adaylar. İki yeni point → mevcut Measurement ile bağımsız 400 px stripe → 10 mm, immutable Runtime ve kalıcı typed sonuç/CSV doğrulandı. Canlı profil/aday tanısı gösterimde 2048 örnek/adayla sınırlı; arşivde profil yok, seçim tam veriden hesaplanır. Bulunamadı point invalid/downstream SKIPPED; Stop eski nokta/profil/mm bırakmaz. Execution/9, editor 7, terminal 3/vf.socket/2 korunur. Güncel Release binary `build/edge-editor-ui/visionflow.exe`; açık uygulama/config korunur. [Kullanım ve sınırlar](edge-point.md). Sonraki **T5.1 — Edge Pair**. Fiziksel doğruluk/K4, P7/6D.2 ve lens T15 açık; aşağıdaki kayıtlar tarihseldir.

9 Ekim T3.3 tarihsel kabul: **T3.3 — Kod Oku UI ve uçtan uca kabul tamamlandı**. Toolbox Code Read; sekiz format seçimi, reçeteye bağlı raw rectangle ROI, exact/prefix/UTF-16 length/bounded regex, adet ve all/any/seçili sıra ayarları sunar. Editor/Runtime gerçek bounding polygon ve sıra/format/escaped metin/byte listesini gösterir; confidence veya genel parça kararı üretmez. Save/reopen/no-op, TargetOnly/Stop, immutable Runtime ve kalıcı Logs/CSV doğrulandı. Dokuz Toolbox girdisi; ZXing OFF kod girdisi devre dışıdır. Release ON/UI 34/34, OFF/core 31/31; güncel binary `build/preprocess-code-ui/visionflow.exe`. Sonraki **T4.1 — keyfi açılı profil sampler**. Karışık polariteli çoklu 1D tamlığı, fiziksel/hasarlı/DPM corpus açık; length mevcut sürümde UTF-16 birimidir. Editor 7, execution/8, terminal 3 ve vf.socket/2 korunur.

9 Ekim güncel teslim: **T1.3 / T2.1 / T2.2 tamamlandı**. Typed terminal cycle schema 3 replay/Logs CSV; owned görüntü processing provenance ve raw subpixel ROI geometri/çizim/sayısal edit. ROI çalışma alanı geçicidir, reçeteye kaydedilmez/algoritma çalıştırmaz; recipe binding yeni tool paketlerinde. Release OFF/UI 31/31, ON/core 28/28; normal/küçük ekran görselleri incelendi. [Uygulanan sözleşme](roi-geometry.md). Sonraki T2.3; aşağıdaki eski sıra ifadeleri tarihseldir.

9 Ekim T0.3/T1.1/T1.2: Release ON/core 26/26, OFF/UI 28/28; normal/küçük ekran görselleri açılıp incelendi. [Uygulanan alanlar/limitler](tool-values.md), [komut ve kanıtlar](../progress.md). Sonraki T1.3; alttaki C1–C5 baseline ifadesi tarihsel başlangıçtır.

9 Ekim 2026. [Ana plan](recipe-tools-plan.md) ve [algoritma planı](recipe-tools-algorithms.md) için kabul eki. Aşağıdaki test adları yeni eklenmesi önerilen hedeflerdir; mevcut CTest listesinde çalışıyor diye sunulmaz. Son tamamlanmış baseline C1–C5'tir; güncel test sayısı her koşuda `ctest -N` ile görülür.

## 1. Her alt paketin bitiş koşulu

1. Parametre/port/sonuç sözleşmesi ve gerekli invalidation davranışı uygulanmış; hazırlık UI'sı ürün özelliği diye sunulmuyor.
2. Değişen algoritmayı anlamlı bağımsız oracle ile sınayan test ve ilgili legacy regresyon geçmiş. Implementation formülünü aynı şekilde kopyalayan test yeterli değil.
3. Persistence/dependency değiştiyse save/reopen/no-op, unknown/future schema ve tamper ret testleri geçmiş.
4. UI değiştiyse Release UI build, ilgili CTest ve gerçek sonuç durumlarının normal/küçük ekran görüntüleri **açılıp incelenmiş**. Görseli yalnız üretmek kabul değildir.
5. Progress kaydı: alt paket ID, kapsam, dosyalar, tam komut/binary/build yolu, test sonuçları, görsel yolları, sınırlamalar, sonraki ID. README/mimari uygulanmış durumla uyumlu.

Yalnız belge değişen bu plan oturumunda build/UI testi gerekmiyor; doküman yolları, sıra, modül/API kaynakları ve diff whitespace kontrol edilir.

## 2. Test veri düzeni ve bağımsızlık

Kod/generator `tests/` altında, üretilen fixture ve sonuçlar `build/tool-tests/<paket>/` altında. Test INI/proje/katalog/CSV/PNG/log/benchmark aynı kökte. `config/local.ui.ini` kullanılmaz. Sabit tohum, image boyutu, pixel format, ground-truth geometri/text, dönüşüm, noise/blur, expected validity ve lisans/kaynak manifestte bulunur.

Gerçek golden görüntüler kullanıcı/veri sahibinin izniyle ayrı corpus'a alınır; mevcut görüntüler dış servise yüklenmez. İdeal circle/line ground-truth analitik koordinat veya bağımsız yüksek çözünürlüklü render/downsample üzerinden gelir; aynı fitter'ın çıktısı expected olarak kullanılmaz. Kod fixture'ları mümkün olduğunca ikinci bağımsız encoder/standart örneklerden oluşturulur; yalnız aynı reader'ın writer'ıyla roundtrip testine dayanılmaz. Geçerli payload bir testte görselden bağımsız sabitlenir.

Fiziksel hedef parça, minimum kusur boyu, görüş alanı, çözünürlük, çalışma yüksekliği, çevrim süresi ve toleranslar henüz kullanıcıdan kesinleşmedi. Yazılım testleri buna bağlı olmadan yürür; fiziksel kabul manifestinde bu alanlar doldurulmadan “üretim doğruluğu kabul edildi” denmez. Plan bu eksikleri kod geliştirmesi için tekrar onay kapısına dönüştürmez.

## 3. Ana kabul matrisi

| Paket / hedef test | Pozitif ve sınır örnekleri | Olumsuz / hata örnekleri | Gerekli kanıt |
| --- | --- | --- | --- |
| T0 / API-dependency probes | Yerel 5.0 include/link/DLL, QR/fit/remap; ZXing altı format ailesi | Eksik DLL/header/backend, yanlış ABI, offline configure | Gerçek sürüm/commit/hash/build info ve decoded payload |
| T1 / tool_values, recipe migration | Yeni tip codec, legacy no-op/save, aynı cycle | Yanlış union/type/unit/frame; oversize; future schema; bozuk model hash | Strict ret, sıfır sahte output/karar |
| T2 / preprocessing, roi_geometry | Gray8/16, RGB/BGR/RGBA/BGRA, stride; ok/halka/polygon | Boş/sıfır ROI, NaN, image border, raw buffer mutasyonu | Geometri/metadata/content hash ve ownership |
| T3 / code_reader, code_tool | Her format, dönme, ters renk destekliyorsa, çoklu/aynı metinli iki kod, Türkçe/GS separator | No-read, bozuk checksum, yanlış içerik, limit aşımı, timeout | Payload/format/corners/count; read başarısı ile content kararı ayrı |
| T4 / profile_sampler, edge_point | 0/45/90/135° ve rasgele açı; iki polarity; reverse; blur; step width | Flat image, border peak, çoklu eşit tepe, düşük kontrast, sıfır ok | Ground-truth'a göre konum hatası, first/last/strongest doğru |
| T5 / edge_pair, line_fit | Bright/dark stripe; width sınırları; line outlier/noise | Yanlış polarity çifti, dar destek, paralel yanlış kenarlar | Doğru çift/çizgi/angle/span ve residual |
| T6 / circle_fit, circle_metrics | Full ring/arc, iç/dış edge, değişik merkez/radius, oval/lobed/chipped shape | Collinear, dar yay, singular, missing scans, yanlış iç kenar | Çap/merkez/sapma/kapsam ayrı; outlier çapak gizlemez |
| T6/T7 / calibrated_circle, geometry | Manual/uniform/raw-H; perspective görüntüde eliptik görünen gerçek daire; signed offset | Candidate/stale binding, hull dışında nokta, source/resolution mismatch | Önce world map sonra fit; sessiz scale fallback yok |
| T8 / pose_match, tracking_tools | Translate+rotate, subpixel, scale search; bütün ROI şekilleri | Simetri, iki eşdeğer aday, flat template, referans kaybı | Gerçek pose/ambiguous state; AABB fallback yok |
| T9 / blob_features, calibrated_area | Delik/concave/border component, 4/8 connectivity, 0 nesne | Contour self-intersection; horizon/domain; field limit | Eski px² aynı; mm² projective oracle; mask delikleri korunur |
| T10 / decision_rules, runtime | Geçerli all/any/not, dahil sınır, no-read→FAIL, count=0 | Required invalid/disabled/partial, boş rule, forged PASS | NOT_EVALUATED ayrımı; aktif recipe'den yeniden hesap |
| T11 / ocr_backend, ocv_rules | Tarih/lot, karakter box, selected charset/model, doğru/yanlış yazı | Model yok/hash değişmiş, blur, empty text, backend timeout | Ham/normal text, gerçek confidence, OCV reason |
| T12 / color_inspection | BGR/RGB eşdeğer, hue wrap, CIE76 bilinen çiftler | Gray image, achromatic hue, saturation/glare, empty mask | Doğru ölçüt ve coverage; color metric invalid ayrımı |
| T13 / reference_defect | Missing/extra/burr/notch, ignore region, registration | Yanlış alignment, güçlü illumination shift, yanlış model | Doğru defect mask/alan/uzunluk; sahte PASS yok |
| T13 / bead_inspection | Straight/curve/corner; değişken width, offset, kopuk path | Crossing/yanlış çift, missing scan, hull aşımı | Yol boyunca gap length; genişlik ile eksik sonuç ayrımı |
| T15 / lens_regression | Sentetik distortion + multi-view fit + ayrı holdout | Eski raw-H ile rectified point, lens/çözünürlük mismatch | Yeni frame chain; eski model/hash ve tool regresyonu |

## 4. Sayısal test politikası

- Salt geometri: double normalize ölçeğe bağlı absolute+relative epsilon; 3-4-5 mesafe, bilinen dik/paralel çizgiler, ters dönüşüm ve analitik radius. Büyük koordinat/çok küçük geometri ve overflow/NaN/Inf ayrıca.
- Raster sampling: başlangıç temiz sentetik hedefi edge RMS ≤0.15 px ve max ≤0.30 px; circle çap hatası ≤0.30 px. Bunlar **algoritma geliştirme hedefidir**, geçmiş/garantili doğruluk değildir. T4/T6'da blur/antialias/sampling manifestiyle ölçülür; test geçsin diye fixture veya eşik gerekçesiz gevşetilmez. Hedef sağlanamıyorsa ölçülen hata ve nedeni açık kaydedilir, kabul ölçütü kanıta göre revize edilir.
- Noisy/blurred/degraded kümelerde aynı temiz eşik zorunlu sayılmaz; error dağılımı, detection/false accept/false reject ve invalid oranı ayrı raporlanır. Invalid döndürmek doğru negatif olabilir; yanlış geçerli değer daha önemli ayrı testtir.
- mm oracle: örnek uniform 20 px/mm ve analitik 200 px çap için 10 mm; raw-H örneklerinde gerçek world shape project edilip bağımsız koordinat dönüşümüyle beklenen geometri kontrol edilir. Piksel çapına ortalama scale çarpan hatalı yöntem özellikle negatif kontrol olmalı.
- Dairesellik: kusursuz analitik nokta setinde residual sıfıra yakın; radial lob/chip ile beklenen peak-to-valley artar. Fit için silinen outlier defect metriğinde kalır. Full coverage başarısızlığı veya açık arc circularityIndex üretmez.
- Tolerans: lower/upper'a eşit ham değer dahil; sınırın hemen içi/dışı `nextafter` veya tarifli epsilon fixture; UI üç/altı ondalık yuvarlama kararı değiştirmez. Test edilmiş px hata ölçümü, görüntüde her yerde aynı mm hata anlamına gelmez.
- Colour/OCR/kod sonuçları için kabul corpus'un tamamında payload/expected text ve etiket doğruluğu esas; model confidence kendi başına doğruluk ölçütü değildir.

## 5. Runtime ve hata matrisi (her yeni tool ailesinde)

1. Run Tool aynı raw frame üstünde yalnız hedef executor; yeni acquisition yok. Upstream değişirse hedef ve downstream invalid, ilgisiz dal cache'i korunur. Preprocess chain/model/backend/ROI/calibration hash değişimi test edilir.
2. Valid `found=false` Boolean/count output'u karar aracınca tüketilebilir; invalid Point/Line/Circle ölçüme gidemez. Engine bütün sonucu tek “found yok” gerekçesiyle geçerli Boolean'a kadar silmez; port bazlı validity korunur.
3. Yeni acquisition, Stop, recipe değişimi, model teaching, calibration current/resolution/source değişimi eski metric/PASS/overlay bırakmaz. Geç worker sonucu generation/signature ile reddedilir.
4. Partial evaluation hiçbir zaman terminal üretim sonucu/final PASS değildir. Immutable snapshot çalışırken editor tolerans değişimi aktif çevrimi değiştirmez; yeni aktif sürüm ayrı Load/Run ister.
5. Store lock/write error ile yeni çevrim engeli; retry aynı cycle/result ile yazma, acquisition/execution tekrarı yok. Yeni text/geometry sonuçları persistence codec tarafından korunur; bilinmeyen tür sessiz kaybolmaz.
6. Numeric/text/collection boyutu ve JSON toplam sınırı, invalid UTF-8 veya binary payload, unknown schema, eski kayıt replay ve bozuk hash testleri. Liste truncate olmuşsa tam count/decision gibi sunulmaz.
7. vf.socket/2 legacy Measurement testleri korunur. T10.4 v3'te multiple typed results/final decision, version reject ve simulator testi; duplicate/reconnect/ACK davranışı ve P7 eksikliği ayrıca raporlanır. P7 bitmedikçe restart delivery güvenilirliği iddia edilmez.

## 6. UI ve görsel kabul

1400×900 ve 850×600: uzun Türkçe ad/hata, DPI/font kaynaklı kesilme; Properties ve result table scroll; nokta/ok/halka handles; zoom/pan/fit-image; son frame/recipe/tool seçimi. Kayıtlı ROI geometry sayısal edit ve mouse edit ile aynı sonuç üretmeli. ESC half-drawn ROI'yi kaydetmemeli; undo önceki geometry/result invalidation davranışını geri getirmeli.

Gerekli görseller: kod decoded/wrong-content/no-read; edge profile/found/missing; caliper çift; line fit rejected point; circle calibrated/pass-boundary/invalid-coverage; perspective raw overlay; dönen tracking ROI; Blob holes; Result FAIL reasons/NOT_EVALUATED; OCR incorrect text; renk maskesi; defect ve bead gap. Her tool için boş başlangıçta sahte örnek sayı/score/PASS görünmemeli. Offscreen görsel native DPI/çoklu monitör kabulü değildir.

## 7. Derleme ve koşum örneği

README yerel toolchain'i esas. Önce gerekli araçların PATH'te olduğunu kontrol et. Yeni dizinler çalışan binary'yi korur:

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/tools-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/tools-core -j 6
ctest --test-dir build/tools-core -N
ctest --test-dir build/tools-core --output-on-failure
cmake -S . -B build/tools-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/tools-ui -j 6
ctest --test-dir build/tools-ui -N
ctest --test-dir build/tools-ui --output-on-failure
```

ZXing/Tesseract prefix seçenekleri T0/T11'de gerçekten uygulanan CMake isimleriyle bu komutlara eklenir; bu belgede henüz olmayan option adları çalışıyor diye sunulmaz. Her alt pakette ilgili testler yeterli; T1 migration/runtime altyapısı ve T14/T15 kapanışında tam suite zorunlu. Yeni değişiklik/failure yokken aynı suite gereksiz tekrarlanmaz. Localhost sandbox izni sorunu ile ürün hatası ayrılır; izinli tekrar gerekirse gerçek sonuç ayrı kaydedilir.

## 8. Performans, fiziksel kabul ve teslim raporu

Benchmark: 1/5/16 MP; full-image/ROI, scan sayısı/uzunluğu, kod adedi, pose angle/scale aralığı ve preprocessing zinciri. Warm-up sonrası en az 100 cycle p50/p95/max, peak memory ve timeout/cancel latency; acquisition ve her tool ile bütün cycle ayrı. CPU/compiler/build/backend/model sürümü kaydedilir. Hedef süre kullanıcı istasyonu için henüz belirlenmedi; ölçülmeden “real-time”, CUDA hızı veya ürün latency kabulü yazılmaz.

Fiziksel metrologi K4: bağımsız referans parça/gauge, görüş alanı boyunca konumlar, farklı çevrim/yeniden yerleştirme/ışık/yükseklik etkisi ve tekrar edilebilirlik. T15 ertelendiğinden raw-H'nin lens hatası kalabilir. Gerçek veride tolerans hedefi sağlanmıyorsa yazılım testlerinin geçmesi o ölçümü üretime uygun yapmaz; kayıtlı alan/tolerans kapsamı kısıtlanır veya T15 sonrası yeniden kabul edilir.

Her teslim son yanıtı: tamamlanan alt paketler, yeni binary (varsa), kısa kullanım, gerçek test/görsel/benchmark sonucu, açık sınırlama ve tam sonraki ID. Donanım/kurulum/dağıtım lisans paketlemesi ayrı kabul; hiçbir test sayısı önceki oturumdan yeniymiş gibi kopyalanmaz.
