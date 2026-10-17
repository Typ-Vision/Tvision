# VisionFlow iş listesi

17 Ekim KMC-10c2/10d uygulandı: editor9 bütün-set Runtime preparation, terminal7/restart/Logs/CSV ve vf.socket2 scalar mapping; N kaynak hazırlık tablosu, seçili gerçek frame/overlay ve immutable model gösterimi. [Kalıcı Runtime sözleşmesi](architecture/multi-source-runtime-terminal.md). **KMC-10e B yazılım kabulü ve fiziksel devir dokümanı tamamlandı: tam Release UI/ZXing ON 87/87; Widgets OFF/ZXing OFF 73/73**; [B kabul ve fiziksel devir](architecture/multi-source-release-acceptance.md). Aşağıdaki tarihli teslim kayıtları kendi kapanış durumlarını anlatır; güncel durum bu paragraftır. Fiziksel/native/lens/industrial/PLC/robot/CUDA/kurulum/dağıtım kabulü açık.

17 Ekim KMC-10c1 immutable kaynak seti ve trusted completion çekirdeği uygulandı. [Programatik kapsam](architecture/multi-source-production-contract.md); sıradaki KMC-10c2 terminal/restart/CSV.

16 Ekim KMC-10b execution/Editor uygulandı: N raw frame/cache, per-source model, TargetOnly, Viewer ve bütçeler. [Kullanım ve sınırlar](architecture/multi-source-execution-editor.md). Sonraki KMC-10c1; editor9 production kapısı KMC-10c2 kabulüne kadar kapalıdır.

16 Ekim — **KMC-0–9 tamamlandı (7a/7b ayrı)**. Kullanıcının sonraki dört işi KMC-6 → KMC-7a → KMC-7b → KMC-8 sıralı tamamlandı. Reçetesiz iki yöntem/dosya, scoped tek-source Editor/Runtime, activation review, legacy history ve dependency transfer uygulanmıştır. **KMC-9 A yazılım kabulü tamamlandı:** tam Release UI/ZXing ON 80/80 ve Widgets OFF/ZXing OFF 68/68. [Kabul matrisi](architecture/independent-calibration-acceptance.md). **KMC-10a çok kaynaklı DAG/frame sözleşmesi uygulandı**; [programatik kapsam](architecture/multi-source-graph-contract.md). **Sıradaki KMC-10b execution/Editor**. Çok-source execution/production kapıları kapalıdır. [Plan](architecture/independent-calibration-implementation-plan.md), [entegrasyon](architecture/scoped-calibration-integration.md), [migration/taşıma](architecture/measurement-setup-migration.md). Tek-source sınırı sürer; B çok-source Editor/Runtime, native DPI/fiziksel KUI-6/T8.4.2b.3c/CUDA/dağıtım açık.

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](architecture/reference-production-acceptance.md). [R8 paket ve devir kanıtı](architecture/reference-release-acceptance.md).


12 Ekim: R1–R8 tamamlandı. Sonraki **T8.4.2b.3c eksik corpus ve bütün production yollarının kabulü**. U1–U4/T8 işleri yeniden yapılmaz. [Kapsam](architecture/reference-integration.md).

## 11 Ekim 2026 — Son kullanıcı önceliği: Ortak Referans

[Referans planı](architecture/reference-system-plan.md) **R1–R8 tamamlandı**; [kayıt sözleşmesi](architecture/reference-binding.md). R6 Editor, R7 production ve R8 tam paket kabulü tamamlandı; sıradaki **T8.4.2b.3c**. Bu sıra alttaki tarihsel kayıtların önündedir; U1–U4 tekrar yapılmaz.

- [x] R1 — Point/Pose ayrımı, X/Y/açı ofset matematiği ve strict binding codec.
- [x] R2 — Kaynak/tüketici registry, DAG, atomik edit ve eski reçete geçişi.
- [x] R3a — Güncel kaynak çözümleme/Editor cache; R3b — raw metrologi; R3c — legacy Edge; R3d — Blob/Code.
- [x] R4 — Kalibre world tüketicileri ve raw px ofset dönüşümü.
- [x] R5a — Tek Referans Properties; R5b — viewer ve etkileşim.
- [x] R6 — Üç applicationType, araç matrisi, zincir, migration ve birleşik Editor kabulü.
- [x] R7a — Immutable production/trusted completion; R7b — kayıt/restart/Logs/CSV.
- [x] R8 — Tam Release core/UI, görsel kabul, belge/devir.
- [ ] T8.4.2b.3c — R7 kanıtını tekrar uygulamadan birleştiren son production/corpus kabulü.

Yeni kapsam: Edge/Edge Point/Caliper/Line/Circle/Blob/Code için ortak Referans; Point kaynak yalnız X/Y, gerçek Pattern/Shape pose X/Y/açı. Yeni referenceBinding production yolu R7 ile açıktır; standalone Pattern/Shape ve eski poseTracking kapıları kalan corpus kabulüne kadar kapalıdır.

## 11 Ekim 2026 — Yeni kullanıcı önceliği: Editor kullanım kolaylığı

[Ayrıntılı plan ve proje değerlendirmesi](architecture/editor-usability-plan.md) hazır; U1–U4 uygulandı ve birleşik kabul tamamlandı. Devam sırası:

- [x] U1 — Properties Basic/Detailed, Elle gir/Çıkışlar, Type/ID sadeleşmesi ve görüntü tıklamasında seçimi koruma.
- [x] U2 — Yeni araçta varsayılan Görüntü kaynağı ve birincil çizim hazırlığı.
- [x] U3 — Flow sağ tıkla kalıcı yukarı/aşağı sıralama; bağlantı ve DAG kurallarını koruma.
- [x] U4 — Release, ilgili ve final tam CTest, normal/küçük ekran görsel kabulü ve belge kapanışı.
- [ ] Ardından mevcut T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü.

Aşağıdaki tamamlanan teslimler geçerlidir; yeni UI talebi bu alanlarda önceki görünümü koruma kararını günceller. Fiziksel kabul ve diğer açık ürün işleri kapanmış sayılmaz.

10 Ekim güncel kapsam: **T8.4.2b.3b — Nominal world ROI authoring ve mm metric/tolerans tamamlandı.** Edge Point/Caliper/Line/Circle için **Nominal mm ROI öğret…** ekranı geometriyi, güncel pose teaching kaydını ve DAG bağlantısını atomik kaydeder; pending/iptal/geçersiz geometri reçeteyi değiştirmez. Caliper gerçek mm genişliğini, Line current mm span/açısını dahil tool toleranslarıyla değerlendirir. Circle tüm gerçek gözlemlerden (fit outlier dahil) radyal sapma, current world açı/gap/observed arc ve kaliteye bağlı tolerans üretir. Raw line/circle fit ve synthetic derivative strength unavailable; genel parça kararı NOT_EVALUATED. Engine **execution/19**, registry16/editor7/terminal3/vf.socket/2 korunur. Production pose whitelist reddi sürer. Sonraki **T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü**; T8.4 açık. Güncel Release binary: `build/tracking-ui/visionflow.exe`.

Alttaki teslim kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel kapsam: **T8.4.2b.3a — Shape/Texture teaching ve ortak pose referansı editörü.** Toolbox artık16 araçtır; Pattern Pose / Shape Pose gerçek Gray8 model, explicit model-local anchor, açı/ölçek ve yöntem skor ayarlarıyla öğretilir. Edge Point/Caliper/Line/Circle/Code/Blob için ortak picker, güncel tam producer cache ve bağlı image ile ROI ilişkisini worker üzerinde öğretir; binding ve poseReference tek doğrulanmış edit olarak kaydedilir. Raw çözülen ROI salt okunur gösterilir; unavailable referansta nominal fallback yoktur. Mevcut nominal mm Circle öğretimi ve kayıtlı world envelope yeniden öğretimi aynı servistedir. Yeni Edge/Caliper/Line nominal mm ROI yazma ekranı, world metric/tolerans/current-world arc ve trusted production Runtime/Logs/CSV kabulü sonraki b.3b/b.3c kapsamındadır; **T8.4.2b.3/T8.4 açık**, production pose reddi sürer. Engine execution/18, registry16, editor7 ve terminal3 değişmedi. Güncel binary: `build/tracking-ui/visionflow.exe`. [Kullanım ve sınırlar](architecture/pose-roi.md).

Alttaki teslim/sıra kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.2b — Kalibre tracking teaching/recipe/DAG tamamlandı (feature kapsamı).** Ayrı `poseTracking` schema2 / `world-raw-rigid-v1`: nominal mm ROI SHA256, gerçek taught raw Pose2D, producer/model/anchor, kaynak/çözünürlük, exact calibration dependency ve explicit mm sampling unit. World geometri raw local ROI/AABB içine yazılmaz; eski raw envelope/hash korunur. Engine **execution/18**, validated resolver ve immutable mapping snapshot ile Edge Point/Caliper/Line/Circle/negative Arc örneklerini yürütür. Save/reopen/no-op/yeniden öğretme, iki gerçek matcher × üç applicationType, TargetOnly/acquisition0, cache/model/calibration değişimi ve Stop: **722 DAG kontrolü**; non-affine teaching/executor ve önceki sampler korumaları: **2178 world kontrolü**. Edge actual raw point, Caliper raw endpoint/px ve mm mesafe, Line raw observations/current mm fit, Circle/Arc current mm circle refit üretir. Synthetic strength, raw line/circle fit, taşınmış nominal arc açısı, defect/tolerans/accepted/PASS bu yolda explicit invalid kalır. Kullanıcı teaching UI ve production trusted completion/Runtime/Logs/CSV kabulü **T8.4.2b.3** kapsamında açıktır; T8.4 tamamlanmadı. UI/config değişmedi; production pose whitelist reddi korunur. [Sözleşme ve sınırlar](architecture/pose-roi.md).

Alttaki teslim/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel durum: **T8.4.2b.2a — Programatik world sampler/refit tamamlandı; T8.4.2b.2/T8.4 açık.** PoseRoiService::worldMap validated/current exact calibration ve aynı owned image snapshot ile nominal world → taught raw → current raw → current world zincirini kurar. WorldTrackingService Edge Point/Caliper/Line/Circle/Arc için her gerçek longitudinal/transverse tap üzerinde alan kontrolü, mm nokta/mesafe ve taşınmış gözlemlerden line/circle refit sunar. Bağımsız perspektif raster, gerçek iki matcher → uniform/Y-up world edge ve hata/iptal testleri **2134 kontrol** geçti. Bu programatik servis teslimidir; kalibre tracking teaching envelope/recipe/DAG/TargetOnly bağlantısı sıradaki **T8.4.2b.2b** işidir. UI/Runtime/Logs/CSV tracking kabulü b.3 kapsamında açık; production pose whitelist reddi ve execution/17 sürer. Standalone ToolRoi schema2 world scan/rectangle taşır; recipe Edge Point/Caliper/Line config bu world geometrileri henüz reddeder. UI ve yerel config değişmedi. [Sözleşme ve sınırlar](architecture/pose-roi.md).

10 Ekim T8.4.2b.1 tarihsel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](architecture/pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim T8.4.1 tarihsel durum: **T8.4.1 — Shape/texture ROI referans sözleşmesi tamamlandı; T8.4 devam ediyor.** Ortak PoseRoiService artık pattern_pose ve shape_pose kabul eder; kaynak kind için doğru method/version zorunludur. Model/ROI/anchor veya matcher türü değişirse teaching geçersizdir. Gerçek iki matcher ile altı geometri, dönmüş scan → gerçek EdgeFinder, shape kontrast tersleme, gerçek tekrarlı/not-found adaylar ve perspektif/lens korumaları: **398 tracking kontrolü**. Programatik servis kabulüdür; Code/Edge/Caliper/Line/Circle/Blob reference picker, recipe/DAG binding, world sampler ve production Runtime bağlantıları sıradaki **T8.4.2** kapsamındadır. Toolbox14/registry16, execution/15 ve Runtime whitelist reddi sürer; UI değişmedi.

10 Ekim T8.3 tarihsel durum: **T8.3 — Shape Match çekirdeği tamamlandı**. `shape_pose` v1 gradient pyramid adayları ve unsigned orientation-aware chamfer refinement ile gerçek raw Pose2D/ayrı scale üretir. Texture NCC ile skor karşılaştırılmaz; yöntem `unsigned-oriented-chamfer-pyramid` v1. Ambiguity/truncation seçimi varsayılan reddeder; genel karar NOT_EVALUATED. Programatik teaching/DAG/TargetOnly/codec teslimi; Toolbox14/registry16, engine execution/15. Özel UI, shape ROI reference ve production Runtime/completion T8.4 kapsamında, Runtime whitelist reddi sürer. Sonraki **T8.4 — Bütün araçlara bağlama**. [Sözleşme ve sınırlar](architecture/shape-pose.md).

10 Ekim T8.1 tarihsel durum: **T8.1 — Gerçek Pose2D çekirdeği tamamlandı**. `pattern_pose` v1 programatik teaching/DAG: çok seviyeli açı/ölçek arama, anchor/refinement/NMS, ambiguity ve reject_ambiguous/best politikası. Pose2D gerçek raw açı/anchor taşır; scale ayrı porttur. PartDecision NOT_EVALUATED. Toolbox14 korunur (registry15); yeni özel UI/tracking/production Runtime henüz yok, Runtime açıkça reddeder. Engine execution/14, editor7/terminal3/vf.socket/2 korunur. Sonraki **T8.2 — Pose referanslı ROI**. [Ayrıntılar](architecture/pattern-pose.md).


10 Ekim güncel durum: **T7.3 — Geometri UI/tolerans/kayıt tamamlandı**. Toolbox14 içinde Geometry Measurement v2; işleme göre raw typed feature seçimi ve aynı producer observations bağlantısı, yardımcı geometri, nominal/alt/üst ve negatif signed limitler. accepted yalnız tool toleransıdır; genel part decision NOT_EVALUATED. Immutable Runtime completion aynı actual image/upstream sonuçları ve güncel kalibrasyonla exact replay edilir; Save/reopen/TargetOnly/Load/Run/Stop/Logs/restart/CSV bağlıdır. Engine execution/13; editor7/terminal3/vf.socket/2 korunur. Sonraki **T8.1 — Gerçek Pose2D**. [Kullanım ve sınırlar](architecture/geometry-measurement-adapter.md).

10 Ekim T7.2 tarihsel kayıt: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](architecture/geometry-measurement-adapter.md).


10 Ekim T7.1 tarihsel kayıt: **T7.1 — Geometrik domain matematiği tamamlandı**. Mevcut framed Point2D/Line2D/Circle2D üzerinde mesafe, signed/absolute X/Y ve dik offset, segment uzaklığı, üç açı semantiği, sonsuz çizgi kesişimi/paralel aralık ve radius/diameter seçimi. Domain çekirdeğidir; yeni reçete aracı/UI/Runtime bağlantısı yok. Engine execution/11 ve mevcut UI build/circle-ui/visionflow.exe korunur. Sıradaki **T7.2 — Kalibre adapter**. [Sözleşme ve sınırlar](architecture/geometry-measurements.md).


10 Ekim T6.4 tarihsel kayıt: **T6.4 — Circle / Arc UI ve kalibre Runtime tamamlandı**. Toolbox `circle_fit` v1; raw/world halka-yay öğretme, tutamaçlar, gerçek mm düzleminde fit ve raw inverse overlay. Ayrı çap/sampled sapma dahil limitleri; kalite yoksa NOT_EVALUATED, outlier kusurdan çıkarılmaz. TargetOnly, immutable Runtime, tamper replay, typed Logs/restart/CSV ve Stop bağlıdır. Engine **execution/11**, editor7/terminal3/vf.socket/2 korunur; world ROI opt-in schema2. Güncel Release UI `build/circle-ui/visionflow.exe`. Sonraki **T7.1 — Geometrik domain matematiği**. [Kullanım, birimler ve sınırlar](architecture/circle-tool.md).

10 Ekim T6.3 tarihsel çekirdek kaydı: **T6.3 — Circle kalite/metrikler tamamlandı**. Fit dışı bulunan noktalar dahil radial min/max, signed inward/outward ve peak-to-valley; found coverage/nominal gap kapıları, full-circle/observed-arc/unavailable kapsamı. Circularity yalnız eksiksiz gerçek basit kapalı polygon'dan 4*pi*A/P²; missing/arc/self-intersection için unavailable, sahte kapanış yok. Yalnız çekirdek; Circle UI/registry/kalibre Runtime/Logs T6.4'tedir. Mevcut UI `build/metrology-ui/visionflow.exe`, execution/10/editor7/terminal3/vf.socket/2 korunur. Release OFF/core **40/40**, yeni metrik592 kontrol. Sonraki **T6.4 — UI ve kalibre Runtime**. [Sözleşme ve kanıt](architecture/circle-metrics.md).

10 Ekim T5.3 tarihsel kayıt: **T5.3 — UI/kalibrasyon/kabul tamamlandı**. Toolbox Edge Pair / Caliper ve Line Fit; reçeteye bağlı ok/kutu/Properties, gerçek px veya güncel kalibrasyonla mm, world noktalarından yeniden çizgi fit ve fiziksel açı, dahil tool tolerans Boolean’ı ve raw destek/outlier overlay. TargetOnly, immutable Runtime, typed Logs/restart/CSV ve Stop bağlıdır. Genel parça kararı/confidence/pose üretilmez. Engine **execution/10**, editor7/terminal3/vf.socket/2 korunur. Güncel Release binary `build/metrology-ui/visionflow.exe`; kullanıcı uygulaması/config korunur. [Kullanım, portlar, 128-scan reçete sınırı ve kanıt](architecture/metrology.md). Sonraki **T6.1 — radyal tarama**; fiziksel K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA kabulü açık. Aşağıdaki kayıtlar tarihseldir.

9 Ekim T4.3 tarihsel kabul: **T4.3 — Editor/Runtime/Measurement tamamlandı**. Toolbox Edge Point, reçeteye bağlı A→B ok çizimi/tutamaç/sayısal koordinatlar/ters yön ve polarity/selection/subpixel/örnekleme ayarları; gerçek ham/Gaussian/signed türev grafiği ve kabul/ret/seçili adaylar. İki yeni point → mevcut Measurement ile bağımsız 400 px stripe → 10 mm, immutable Runtime ve kalıcı typed sonuç/CSV doğrulandı. Canlı profil/aday tanısı gösterimde 2048 örnek/adayla sınırlı; arşivde profil yok, seçim tam veriden hesaplanır. Bulunamadı point invalid/downstream SKIPPED; Stop eski nokta/profil/mm bırakmaz. Execution/9, editor 7, terminal 3/vf.socket/2 korunur. Güncel Release binary `build/edge-editor-ui/visionflow.exe`; açık uygulama/config korunur. [Kullanım ve sınırlar](architecture/edge-point.md). Sonraki **T5.1 — Edge Pair**. Fiziksel doğruluk/K4, P7/6D.2 ve lens T15 açık; aşağıdaki kayıtlar tarihseldir.

9 Ekim T3.3 tarihsel kabul: **T3.3 — Kod Oku UI ve uçtan uca kabul tamamlandı**. Toolbox Code Read; sekiz format seçimi, reçeteye bağlı raw rectangle ROI, exact/prefix/UTF-16 length/bounded regex, adet ve all/any/seçili sıra ayarları sunar. Editor/Runtime gerçek bounding polygon ve sıra/format/escaped metin/byte listesini gösterir; confidence veya genel parça kararı üretmez. Save/reopen/no-op, TargetOnly/Stop, immutable Runtime ve kalıcı Logs/CSV doğrulandı. Dokuz Toolbox girdisi; ZXing OFF kod girdisi devre dışıdır. Release ON/UI 34/34, OFF/core 31/31; güncel binary `build/preprocess-code-ui/visionflow.exe`. Sonraki **T4.1 — keyfi açılı profil sampler**. Karışık polariteli çoklu 1D tamlığı, fiziksel/hasarlı/DPM corpus açık; length mevcut sürümde UTF-16 birimidir. Editor 7, execution/8, terminal 3 ve vf.socket/2 korunur.

9 Ekim güncel teslim: **T1.3 / T2.1 / T2.2 tamamlandı**. Typed terminal cycle schema 3 replay/Logs CSV; owned görüntü processing provenance ve raw subpixel ROI geometri/çizim/sayısal edit. ROI çalışma alanı geçicidir, reçeteye kaydedilmez/algoritma çalıştırmaz; recipe binding yeni tool paketlerinde. Release OFF/UI 31/31, ON/core 28/28; normal/küçük ekran görselleri incelendi. [Uygulanan sözleşme](architecture/roi-geometry.md). Sonraki T2.3; aşağıdaki eski sıra ifadeleri tarihseldir.

9 Ekim T0.3/T1.1/T1.2 tamamlandı; güncel sıradaki iş **T1.3 — Runtime/Logs/cache typed sonuç altyapısı**. [Uygulanan sözleşme](architecture/tool-values.md), [ilerleme kanıtı](progress.md). Yeni code tool T3, OCR T11, lens T15; P7/6D.2 ve K4 ayrı açık kalır.

9 Ekim kullanıcı tool kapsamını onayladı; ayrıntılı [T0–T15 planı](architecture/recipe-tools-plan.md) ve [algoritma](architecture/recipe-tools-algorithms.md)/[kabul](architecture/recipe-tools-acceptance.md) ekleri hazır. **Bütün T paketleri planlandı; ilk uygulama T0.1.** Kod okuma T3, metrologi T4–T7, tracking T8, Blob T9, karar T10, OCR T11, renk T12, kusur/bead T13, birleşik kabul T14, lens en son T15. Eski P9 QR işi T3'e taşındı. P7/6D.2, P8 hardware, P9 OPC UA, 6E robot ve K4 açık kalır. Aşağıdaki eski “sonraki” ifadeleri tarihsel; tamamlanan C1–C5/P1–P6 yeniden yapılmaz.

9 Ekim güncel durum: **C1–C5 tamamlandı**. Setup → Calibration üç yöntem ve beş bölüm sunar: Manuel ölçek (px/mm), Bilinen uzunluktan ölçek ve Kalibrasyon plakası (perspektif). Manuel girdi boş başlar; px/mm ve salt okunur tersi, üretim sonucundan ayrı deneme, en az iki bağımsız doğrulama, katalog/recipe Save/reopen, gerçek Edge → Measurement → Runtime ve CSV bağlıdır. Güncel Release binary `build/manual-calibration-ui/visionflow.exe`; Measurement tool 2, execution/6. Sonraki P7/6D.2; K4 fiziksel kabul ertelenmiştir.


8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4 tamamlandı: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 P2 tamamlandı: Recipe Manager tek tıkla yalnız satır seçer; **Load Recipe / Edit Recipe** ayrıdır. Load kaydedilmiş bağımsız kopyayı statik kontrol eder; dirty durumda Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal sunulur. Runtime **Run / Stop / Tek çevrim** kullanır; Run görüntü veya parça sonucu üretmeden coordinator ile otomatik hazırlanır. Manuel çevrim, cooperative Stop ve aynı terminal kayıt retry coordinator üzerinden geçer. Seçili, editörde açık ve Runtime yüklü reçete ayrıdır. Eski validate/activate/rollback düğmeleri kaldırıldı; rollback servis API’si korunur. Bu P2 teslimi P3/P4 ile genişletildi; güncel socket ve Run Tool davranışı yukarıdadır. Fiziksel kabul/K4 ertelenmiş kalır.

8 Ekim 2026 güncel öncelik: [Runtime/Editor iş planı](architecture/runtime-editor-workflow-plan.md) P1–P6 sıradadır; ardından 6D.2/P7 restart recovery, ileri aşamada P8–P9 hardware trigger/OPC UA/QR. P1–P4 tamamlandı; sıradaki P5, ardından P6. Kullanıcı gerekli Load/Run/Run Tool/Blob UI değişikliklerini onayladı; eski genel GUI değişmezliği kuralı bu kapsamı engellemez.

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Ayrıntılı plan](architecture/calibration-plan.md).

7 Ekim 2026. Tamamlananların kabul kanıtları `progress.md` içindedir.

## Güncel onaylı sıra

Genel incelemenin tüm önerileri kabul edildi. Ayrıntı ve kabul koşulları [geliştirme planında](architecture/development-plan.md). **GUI görünümü, tema, panel yerleşimleri, menüler ve navigasyon değişmeyecek.** A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı; kullanıcı yönlendirmesiyle webcam ve bağımsız 6C ilerler. Diğer yeni işler uygulanmış özellik değildir; sonraki tarihsel kayıtlar eski teslim kapsamını gösterir.

| Sıra / iş | Durum | Bağımlılık / teslim |
| --- | --- | --- |
| 6A/A1 — Run Match ve Run Flow ortak giriş/yürütme kuralları | Tamamlandı | Ortak engine, upstream alt grafiği, aynı snapshot/ROI, iptal ve stale-result kabulü |
| 6A/A2 — Model/geometri sürümüne bağlı referans invalidation | Tamamlandı | Schema 5 içerik/geometri kimliği, Edge ERROR/downstream SKIPPED; yeniden öğretim, migration, save/reopen kabulü |
| 6A/A3 — Statik ToolRegistry, ayrı tool yürütücüleri ve parametre doğrulama | Tamamlandı | Registry/ayrı yürütücüler, typed settings; engine değişmeden extension kabulü |
| 6A/A4 — MainWindow session/controller ayrımı | Tamamlandı | EditorSession/CameraSession job/future/cache/Stop ve teaching; GUI aynı |
| 6A/A5 — Kaynaktan bağımsız owned frame ve metadata | Tamamlandı | Owned buffer, metadata/coordinate origin, raw boyut; adapter reuse ve mismatch kabulü |
| 6A/A6 — Kamera yaşam döngüsü/deadline/kuyruk sözleşmesi | Tamamlandı (çekirdek/sözleşme) | Lifecycle/generation, preview/production kuyruk, deadline, Stop/shutdown budget; SDK bağlantısı ve fiziksel kabul 6B |
| 6A/A7 — Ortak Pose2D/koordinat/tracking ROI sözleşmesi | Tamamlandı (çekirdek/sözleşme) | Rigid 2D, birim/frame/eksen/sıra, dört köşe tracking, legacy Edge ve dependency kaydı; kalibrasyon uygulaması 6C |
| 6A/A8 — Draft/doğrulama/aktif üretim snapshot sözleşmesi | Tamamlandı (çekirdek/sözleşme) | Immutable snapshot/dependency, servis policy, activation/rollback/cycle sınırı ve transient trace; Runtime/Users/kalıcı log 6C |
| 6A/A9 — Varlık paketi/migration planı, bağımsız core build/test ve kaynak sürümleme | Tamamlandı (plan/build/test) | Manifest/migration/recovery planı; Widgets OFF 9/9, UI ON 11/11; yerel Git; gerçek paket/geçmiş 6C/7 |
| 6B — GenApi/vendor API ve tek gerçek kamera trigger/grab → DAG | Başladı; GigE scan/total budget ayrımı tamamlandı | Galaxy GigE 1000/3000 ms DONE/0 cihaz; dar C ABI önceki 0/0/0. Basler önceki Runtime yolu bu oturumda erişilemiyor, Developer formu açık. Sonraki feature/worker/owned frame → DAG; fiziksel trigger/grab/disconnect/recovery açık |
| 6B ek — Webcam tek kare → owned frame → DAG | Tamamlandı | Qt Multimedia worker, Properties/save/reopen; gerçek 1280×720 capture ve DAG DONE. Live/physical unplug/industrial trigger kabulü açık |
| 6C ilk — Planar scale/Measurement/tolerans servis çekirdeği | Tamamlandı (servis) | Teaching/holdout, dependency/provenance invalidation, ham mm tolerans; UI/persistence/Runtime yok |
| 6C kayıt — Kalibrasyon persistence/current resolver ve reçete binding | Tamamlandı (servis) | Atomik versioned evidence, kanıt replay, bağımsız kurulum, schema 2 binding ve eski production validation reddi; UI bağlantısı yok |
| 6C UI — Ortak Measurement tool/Properties ve planar Calibration | Tamamlandı | Gerçek acquisition → Edge → mm/tolerans; Hesapla/Doğrula/Kaydet-bağla ve kanıt reopen; lens/perspective/robot/Runtime yok |
| 6C devam — Yetkili production validation, Runtime ve cycle log | Tamamlandı (configuration scope) | Yerel OS Engineer policy; fresh acquisition/current resolver; immutable sürüm, activation/rollback/Stop; snapshot/event/cycle atomik arşiv, Logs ve failed-write retry. Fiziksel üretim/çok kullanıcı kabulü açık |
| 6C-K — Plaka tabanlı kamera/düzlem kalibrasyonu | K1–K3 tamamlandı; K4 fiziksel kabul açık | K1 model/import/plaka, K2 homography/mm denemesi, K3 kayıt/Measurement/Runtime, gerektiğinde KL lens, K4 fiziksel kabul |
| 6D — PLC handshake ve sonuç teslimi | 6D.1 yazılım tamamlandı; 6D.2 açık | TCP/IP socket/Runtime/ACK/simülatör; kalıcı dedup/outbox/restart recovery ve fiziksel PLC sonraki iş; [sözleşme](architecture/plc-socket.md) |
| 6E — Robot dönüşümü/pick reference/guidance | Bekliyor | Kalibrasyon ve frame doğrulaması; hareket ayrı komut |
| 7 — Ürünleştirme | Bekliyor | Ayrıntılı Users/Diagnostics, audit/retention, paketleme/geçmiş, app-data/kurulum |
| Sonraki — Gelişmiş tool, multi-camera ve GPU | Bekliyor | Gerçek veri ve performans kabulü; GUI yeniden tasarımı kapsam dışı |

## Önceki teslimler ve kapsam kayıtları

| İş | Durum | Kayıt |
| --- | --- | --- |
| Son açık projeyi başlangıçta otomatik aç | Tamamlandı | Proje yolu, aktif reçete ve ekran bağlamı yerel INI; eksik/bozuk dosyada hata ve manuel açma |
| Adım 4: ROI, model teaching, gerçek template match | Tamamlandı | Editor schema 2, gömülü model PNG, background match, sonuç invalidation |
| Adım 5: typed port/tool sözleşmesi ve DAG | Tamamlandı | Schema 3, typed input seçicileri, DAG validation |
| Adım 5: genel background execution ve sonuç/cycle modeli | Tamamlandı | READY/BUSY/DONE/ERROR/SKIPPED; recipe durumundan ayrı |
| Adım 5: Blob / Edge / Calculate yürütmesi | Tamamlandı | Threshold bileşenler, profile edge, px distance ve overlay |
| Background görüntü acquisition | Tamamlandı | Open/Reload/restore/Run Flow decode worker üzerinde |
| Üretim görüntüleriyle match kabul/performance | Bekliyor | Gerçek pozitif/negatif veri, false accept/reject; mevcut fixture'lar sentetik |
| Adım 6: kamera, kalibrasyon, robot/PLC, Runtime | Devam ediyor | 6.1–6.2 tamamlandı; kalan işler yukarıdaki 6A–6E sırasına ayrıldı |
| Adım 7: diagnostics/log, dağıtım | Bekliyor | Temel servis yetkisi/hata kodları/cycle logları 6C'ye çekildi; ayrıntılı ekranlar ve dağıtım 7'de |

ROI şu aşamada yeniden çizilerek düzenlenir; tutamaçla taşıma/boyutlandırma ve Undo/Redo sonraki UI iyileştirmeleridir. Angle/scale, çoklu sonuç/NMS ve CUDA ölçümü ayrı genişletmelerdir.

Adım 5 sınırları: cooperative Stop, yalnız pixel birimi, sabit port kataloğu, Flow liste/bağlantı özeti. Node canvas, UndoRedo, Blob contour/filtre/kabul, Edge subpixel/Rising/Falling ve gelişmiş Calculate ayrı genişletmelerdir. Ayrıntılar `architecture/tool-execution.md`.

7 Ekim UI düzeltmesi tamamlandı: tool silmede bağlantı temizliği, Edge konum çizgisi, Measurement P1/P2 çizgisi ve px etiketi. Kabul: progress.md son kayıt.

Referansa bağlı Edge ROI takibi tamamlandı: Match merkeziyle öğretimdeki ilişki ve kutu boyutu korunur, X/Y hareketi boyunca taşınır. Schema 4, eski Match.region → Edge.region migration ve save/reopen kabulü progress son kaydında.

Adım 6.1 tamamlandı: ortak acquisition sözleşmesi ve dosya adapter'ı, Run Flow entegrasyonu, CTest 5/5. Kamera kararı GenICam/GenTL + gerektiğinde vendor adapter. Sıradaki: gerçek SDK/producer ve ABI doğrulaması, kamera discovery/grab adapter. Kamera/kalibrasyon/robot-PLC/Runtime henüz tamamlanmadı. Ayrıntılar architecture/acquisition.md ve progress.md.


Adım 6.2 tamamlandı: GenTL producer discovery, read-only open/close, Camera Manager worker UI, camera_probe CLI; Release/CTest 6/6 ve offscreen görsel kabul. Gerçek local probe NO_PRODUCER. Sıradaki GenApi feature erişimi, trigger/stream/grab ve owned frame conversion; fiziksel SDK/marka kabulü henüz yok.


## 8 Ekim — Operatör görünümü ve Setup revizyonu

Run başarılı olduğunda Runtime tam ekran olur; sol navigasyon ve File/View/Setup gizlenir ve devre dışı kalır. Sayfa geçişi merkezi guard ile reddedilir. Stop sonrası worker/kayıt durdurma tamamlanınca önceki pencere durumuna dönülür; Runtime Stop ve kayıt retry erişilebilir kalır. Bu bir OS kiosk veya rol/parola sistemi değildir. Communication ve Calibration sol menüden kaldırıldı; File/View yanındaki Setup içinden açılır.

Communication: “TCP iletişimini etkinleştir” işaretlenmeden Dinlemeyi başlat pasiftir. Bu gerekçe ekranda ve tooltip içinde görünür. Ayarları kaydet etkinleştirme veya Run yapmaz; etkinleştir → kaydet → dinlemeyi başlat kullanılır. Otomatik dinleme yalnız uygulama açılışına aittir.

Sonraki geliştirme önerisi (henüz uygulanmadı): önce manuel px/mm ve kalibrasyon ekranını akışa göre sadeleştirme, ardından P7/6D.2 kalıcı dedup/outbox/restart ve kapasite politikası. P8 hardware, P9 OPC UA/QR ve 6E robot daha sonra; K4 fiziksel kabul ertelenmiş kalır. Kullanıcıyla sıra yeniden değerlendirilecek.
## 8 Ekim — Manuel ölçek uygulama planı hazır

[Manuel px/mm ve sade Calibration planı](architecture/manual-scale-calibration-plan.md) önceki kısa kapsam notunun ayrıntılı devamıdır. C1 matematik/codec → C2 katalog/Measurement/Runtime → C3 sade ekran → C4 UI kabul → C5 birleşik test/belgeler. C1–C5 tamamlandı: matematik/codec, katalog/reçete/Runtime/CSV, sade üç yöntem ekranı ve birleşik yazılım kabulü mevcut. Yeni manual_planar_scale/1, record/dependency schema 3, eski hash/encode koruması, beş bölümlü üç yöntem UI ve doğrulamasız deneme/üretim ayrımı tanımlandı. Belgenin 7. bölümü GPT-6.1 Sol medium için kopyalanabilir başlangıç talimatıdır. P7 bu kapsamdan sonra; K4 ertelenmiş kalır.
