# Endüstriyel Machine Vision Yazılımı — Genel Tanım

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](../architecture/reference-production-acceptance.md). [R8 paket ve devir kanıtı](../architecture/reference-release-acceptance.md).


11 Ekim güncel kapsam — **R4, R5a/b ve R6 tamamlandı.** Dört kalibre world tüketicisi ortak Point/Pose referansı ve raw px/açı ofsetini her gerçek sampling tap üzerinde kullanır; nominal mm geometri, current mm refit/metrik/tolerans ve exact calibration alan kontrolleri korunur. Yedi ROI tüketicisinde tek **Referans** bölümü kaynak/port, X/Y ve yalnız gerçek Pose için açı sunar. Seçim pending; Bağla/yeniden kaydet/İptal/kaldır ve atomik ofset düzenlemesi ortak servis yolundadır. Viewer güncel çözülen geometriyi salt okunur gösterir; missing/Stop'ta nominal fallback yoktur. Yeni world kayıt **ReferenceBinding schema2 / world-reference-v1**, raw schema1 ve opt-in editor8 korunur; engine **execution/21**. Eski Edge/raw/world pose kayıtları no-op açma/Save ile dönüştürülmez. **R6 birleşik Editor kabulü tamamlandı; sıradaki R7a immutable production/trusted completion, ardından R7b ve R8 nihai paket**; production yeni binding'i açıkça reddeder. Güncel Release UI binary `build/reference-ui/visionflow.exe`; config/local.ui.ini korunur. Alttaki kapsam/sıra kayıtları tarihseldir. [Sözleşme](../architecture/reference-integration.md).

11 Ekim kullanıcı gereksinimi — **Ortak Referans**: Edge/Edge Point/Caliper/Line/Circle/Blob/Code ROI'si uygun tool'un gerçek nokta/merkez veya pose sonucuna bağlanabilir olacak. Point kaynağı yalnız X/Y öteleme; gerçek Pattern/Shape pose kaynağı konum ve dönüş sağlar. X/Y ofsetleri ortak, açı ofseti yalnız açı üreten kaynakta etkin olacaktır; mevcut Template Match açı üretmez. Geçersiz referansla ölçüm/karar uydurulmaz. [R1–R8 davranış ve kabul planı](../architecture/reference-system-plan.md) R1–R8 tamamlandı; sıradaki T8.4.2b.3c eksik corpus/production kabulüdür. Production Runtime R7/R8 tamamlandı; T8.4.2b.3c kalan corpus/production kapanışı açıktır. Alttaki kayıtlar mevcut/tarihsel uygulamayı anlatır.

10 Ekim güncel kapsam: **T8.4.2b.3b — Nominal world ROI authoring ve mm metric/tolerans tamamlandı.** Edge Point/Caliper/Line/Circle için **Nominal mm ROI öğret…** ekranı geometriyi, güncel pose teaching kaydını ve DAG bağlantısını atomik kaydeder; pending/iptal/geçersiz geometri reçeteyi değiştirmez. Caliper gerçek mm genişliğini, Line current mm span/açısını dahil tool toleranslarıyla değerlendirir. Circle tüm gerçek gözlemlerden (fit outlier dahil) radyal sapma, current world açı/gap/observed arc ve kaliteye bağlı tolerans üretir. Raw line/circle fit ve synthetic derivative strength unavailable; genel parça kararı NOT_EVALUATED. Engine **execution/19**, registry16/editor7/terminal3/vf.socket/2 korunur. Production pose whitelist reddi sürer. Sonraki **T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü**; T8.4 açık. Güncel Release binary: `build/tracking-ui/visionflow.exe`.

Alttaki teslim kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel kapsam: **T8.4.2b.3a — Shape/Texture teaching ve ortak pose referansı editörü.** Toolbox artık16 araçtır; Pattern Pose / Shape Pose gerçek Gray8 model, explicit model-local anchor, açı/ölçek ve yöntem skor ayarlarıyla öğretilir. Edge Point/Caliper/Line/Circle/Code/Blob için ortak picker, güncel tam producer cache ve bağlı image ile ROI ilişkisini worker üzerinde öğretir; binding ve poseReference tek doğrulanmış edit olarak kaydedilir. Raw çözülen ROI salt okunur gösterilir; unavailable referansta nominal fallback yoktur. Mevcut nominal mm Circle öğretimi ve kayıtlı world envelope yeniden öğretimi aynı servistedir. Yeni Edge/Caliper/Line nominal mm ROI yazma ekranı, world metric/tolerans/current-world arc ve trusted production Runtime/Logs/CSV kabulü sonraki b.3b/b.3c kapsamındadır; **T8.4.2b.3/T8.4 açık**, production pose reddi sürer. Engine execution/18, registry16, editor7 ve terminal3 değişmedi. Güncel binary: `build/tracking-ui/visionflow.exe`. [Kullanım ve sınırlar](../architecture/pose-roi.md).

Alttaki teslim/sıra kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.2b — Kalibre tracking teaching/recipe/DAG tamamlandı (feature kapsamı).** Ayrı `poseTracking` schema2 / `world-raw-rigid-v1`: nominal mm ROI SHA256, gerçek taught raw Pose2D, producer/model/anchor, kaynak/çözünürlük, exact calibration dependency ve explicit mm sampling unit. World geometri raw local ROI/AABB içine yazılmaz; eski raw envelope/hash korunur. Engine **execution/18**, validated resolver ve immutable mapping snapshot ile Edge Point/Caliper/Line/Circle/negative Arc örneklerini yürütür. Save/reopen/no-op/yeniden öğretme, iki gerçek matcher × üç applicationType, TargetOnly/acquisition0, cache/model/calibration değişimi ve Stop: **722 DAG kontrolü**; non-affine teaching/executor ve önceki sampler korumaları: **2178 world kontrolü**. Edge actual raw point, Caliper raw endpoint/px ve mm mesafe, Line raw observations/current mm fit, Circle/Arc current mm circle refit üretir. Synthetic strength, raw line/circle fit, taşınmış nominal arc açısı, defect/tolerans/accepted/PASS bu yolda explicit invalid kalır. Kullanıcı teaching UI ve production trusted completion/Runtime/Logs/CSV kabulü **T8.4.2b.3** kapsamında açıktır; T8.4 tamamlanmadı. UI/config değişmedi; production pose whitelist reddi korunur. [Sözleşme ve sınırlar](../architecture/pose-roi.md).

Alttaki teslim/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel durum: **T8.4.2b.2a — Programatik world sampler/refit tamamlandı; T8.4.2b.2/T8.4 açık.** PoseRoiService::worldMap validated/current exact calibration ve aynı owned image snapshot ile nominal world → taught raw → current raw → current world zincirini kurar. WorldTrackingService Edge Point/Caliper/Line/Circle/Arc için her gerçek longitudinal/transverse tap üzerinde alan kontrolü, mm nokta/mesafe ve taşınmış gözlemlerden line/circle refit sunar. Bağımsız perspektif raster, gerçek iki matcher → uniform/Y-up world edge ve hata/iptal testleri **2134 kontrol** geçti. Bu programatik servis teslimidir; kalibre tracking teaching envelope/recipe/DAG/TargetOnly bağlantısı sıradaki **T8.4.2b.2b** işidir. UI/Runtime/Logs/CSV tracking kabulü b.3 kapsamında açık; production pose whitelist reddi ve execution/17 sürer. Standalone ToolRoi schema2 world scan/rectangle taşır; recipe Edge Point/Caliper/Line config bu world geometrileri henüz reddeder. UI ve yerel config değişmedi. [Sözleşme ve sınırlar](../architecture/pose-roi.md).

10 Ekim T8.4.2b.1 tarihsel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](../architecture/pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim T8.4.1 tarihsel durum: **T8.4.1 — Shape/texture ROI referans sözleşmesi tamamlandı; T8.4 devam ediyor.** Ortak PoseRoiService artık pattern_pose ve shape_pose kabul eder; kaynak kind için doğru method/version zorunludur. Model/ROI/anchor veya matcher türü değişirse teaching geçersizdir. Gerçek iki matcher ile altı geometri, dönmüş scan → gerçek EdgeFinder, shape kontrast tersleme, gerçek tekrarlı/not-found adaylar ve perspektif/lens korumaları: **398 tracking kontrolü**. Programatik servis kabulüdür; Code/Edge/Caliper/Line/Circle/Blob reference picker, recipe/DAG binding, world sampler ve production Runtime bağlantıları sıradaki **T8.4.2** kapsamındadır. Toolbox14/registry16, execution/15 ve Runtime whitelist reddi sürer; UI değişmedi.

10 Ekim T8.3 tarihsel durum: **T8.3 — Shape Match çekirdeği tamamlandı**. `shape_pose` v1 gradient pyramid adayları ve unsigned orientation-aware chamfer refinement ile gerçek raw Pose2D/ayrı scale üretir. Texture NCC ile skor karşılaştırılmaz; yöntem `unsigned-oriented-chamfer-pyramid` v1. Ambiguity/truncation seçimi varsayılan reddeder; genel karar NOT_EVALUATED. Programatik teaching/DAG/TargetOnly/codec teslimi; Toolbox14/registry16, engine execution/15. Özel UI, shape ROI reference ve production Runtime/completion T8.4 kapsamında, Runtime whitelist reddi sürer. Sonraki **T8.4 — Bütün araçlara bağlama**. [Sözleşme ve sınırlar](../architecture/shape-pose.md).

10 Ekim T8.1 tarihsel durum: **T8.1 — Gerçek Pose2D çekirdeği tamamlandı**. `pattern_pose` v1 programatik teaching/DAG: çok seviyeli açı/ölçek arama, anchor/refinement/NMS, ambiguity ve reject_ambiguous/best politikası. Pose2D gerçek raw açı/anchor taşır; scale ayrı porttur. PartDecision NOT_EVALUATED. Toolbox14 korunur (registry15); yeni özel UI/tracking/production Runtime henüz yok, Runtime açıkça reddeder. Engine execution/14, editor7/terminal3/vf.socket/2 korunur. Sonraki **T8.2 — Pose referanslı ROI**. [Sözleşme ve sınırlar](../architecture/pattern-pose.md).

10 Ekim T7.2 tarihsel kayıt: **T7.2 — Kalibre geometri adapter tamamlandı**. geometry_measurement v1 sabit raw typed feature/örnek girdileri, operation-specific DAG doğrulaması ve tek world düzleminde point mapping / line-circle refit sunar. Line Fit v2 observationsPx; engine execution/12. Alan/current/source/resolution/frame/cycle/evaluation kapıları ve TargetOnly bağlıdır. Programatik Editor/DAG teslimidir; Toolbox, tolerans ve production Runtime/kayıt kabulü **T7.3** sırasındadır. [Girdiler, refit ve sınırlar](../architecture/geometry-measurement-adapter.md).


10 Ekim T7.1 tarihsel kayıt: **T7.1 — Geometrik domain matematiği tamamlandı**. Mevcut framed Point2D/Line2D/Circle2D üzerinde mesafe, signed/absolute X/Y ve dik offset, segment uzaklığı, üç açı semantiği, sonsuz çizgi kesişimi/paralel aralık ve radius/diameter seçimi. Domain çekirdeğidir; yeni reçete aracı/UI/Runtime bağlantısı yok. Engine execution/11 ve mevcut UI build/circle-ui/visionflow.exe korunur. Sıradaki **T7.2 — Kalibre adapter**. [Sözleşme ve sınırlar](../architecture/geometry-measurements.md).


10 Ekim T6.4 tarihsel kayıt: **T6.4 — Circle / Arc UI ve kalibre Runtime tamamlandı**. Toolbox `circle_fit` v1; raw/world halka-yay öğretme, tutamaçlar, gerçek mm düzleminde fit ve raw inverse overlay. Ayrı çap/sampled sapma dahil limitleri; kalite yoksa NOT_EVALUATED, outlier kusurdan çıkarılmaz. TargetOnly, immutable Runtime, tamper replay, typed Logs/restart/CSV ve Stop bağlıdır. Engine **execution/11**, editor7/terminal3/vf.socket/2 korunur; world ROI opt-in schema2. Güncel Release UI `build/circle-ui/visionflow.exe`. Sonraki **T7.1 — Geometrik domain matematiği**. [Kullanım, birimler ve sınırlar](../architecture/circle-tool.md).

10 Ekim T6.3 tarihsel çekirdek kaydı: **T6.3 — Circle kalite/metrikler tamamlandı**. Fit dışı bulunan noktalar dahil radial min/max, signed inward/outward ve peak-to-valley; found coverage/nominal gap kapıları, full-circle/observed-arc/unavailable kapsamı. Circularity yalnız eksiksiz gerçek basit kapalı polygon'dan 4*pi*A/P²; missing/arc/self-intersection için unavailable, sahte kapanış yok. Yalnız çekirdek; Circle UI/registry/kalibre Runtime/Logs T6.4'tedir. Mevcut UI `build/metrology-ui/visionflow.exe`, execution/10/editor7/terminal3/vf.socket/2 korunur. Release OFF/core **40/40**, yeni metrik592 kontrol. Sonraki **T6.4 — UI ve kalibre Runtime**. [Sözleşme ve kanıt](../architecture/circle-metrics.md).

10 Ekim T5.3 tarihsel kayıt: **T5.3 — UI/kalibrasyon/kabul tamamlandı**. Toolbox Edge Pair / Caliper ve Line Fit; reçeteye bağlı ok/kutu/Properties, gerçek px veya güncel kalibrasyonla mm, world noktalarından yeniden çizgi fit ve fiziksel açı, dahil tool tolerans Boolean’ı ve raw destek/outlier overlay. TargetOnly, immutable Runtime, typed Logs/restart/CSV ve Stop bağlıdır. Genel parça kararı/confidence/pose üretilmez. Engine **execution/10**, editor7/terminal3/vf.socket/2 korunur. Güncel Release binary `build/metrology-ui/visionflow.exe`; kullanıcı uygulaması/config korunur. [Kullanım, portlar, 128-scan reçete sınırı ve kanıt](../architecture/metrology.md). Sonraki **T6.1 — radyal tarama**; fiziksel K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA kabulü açık. Aşağıdaki kayıtlar tarihseldir.

9 Ekim T4.3 tarihsel kabul: **T4.3 — Editor/Runtime/Measurement tamamlandı**. Toolbox Edge Point, reçeteye bağlı A→B ok çizimi/tutamaç/sayısal koordinatlar/ters yön ve polarity/selection/subpixel/örnekleme ayarları; gerçek ham/Gaussian/signed türev grafiği ve kabul/ret/seçili adaylar. İki yeni point → mevcut Measurement ile bağımsız 400 px stripe → 10 mm, immutable Runtime ve kalıcı typed sonuç/CSV doğrulandı. Canlı profil/aday tanısı gösterimde 2048 örnek/adayla sınırlı; arşivde profil yok, seçim tam veriden hesaplanır. Bulunamadı point invalid/downstream SKIPPED; Stop eski nokta/profil/mm bırakmaz. Execution/9, editor 7, terminal 3/vf.socket/2 korunur. Güncel Release binary `build/edge-editor-ui/visionflow.exe`; açık uygulama/config korunur. [Kullanım ve sınırlar](../architecture/edge-point.md). Sonraki **T5.1 — Edge Pair**. Fiziksel doğruluk/K4, P7/6D.2 ve lens T15 açık; aşağıdaki kayıtlar tarihseldir.

9 Ekim T3.3 tarihsel kabul: **T3.3 — Kod Oku UI ve uçtan uca kabul tamamlandı**. Toolbox Code Read; sekiz format seçimi, reçeteye bağlı raw rectangle ROI, exact/prefix/UTF-16 length/bounded regex, adet ve all/any/seçili sıra ayarları sunar. Editor/Runtime gerçek bounding polygon ve sıra/format/escaped metin/byte listesini gösterir; confidence veya genel parça kararı üretmez. Save/reopen/no-op, TargetOnly/Stop, immutable Runtime ve kalıcı Logs/CSV doğrulandı. Dokuz Toolbox girdisi; ZXing OFF kod girdisi devre dışıdır. Release ON/UI 34/34, OFF/core 31/31; güncel binary `build/preprocess-code-ui/visionflow.exe`. Sonraki **T4.1 — keyfi açılı profil sampler**. Karışık polariteli çoklu 1D tamlığı, fiziksel/hasarlı/DPM corpus açık; length mevcut sürümde UTF-16 birimidir. Editor 7, execution/8, terminal 3 ve vf.socket/2 korunur.

9 Ekim güncel teslim: **T1.3 / T2.1 / T2.2 tamamlandı**. Typed terminal cycle schema 3 replay/Logs CSV; owned görüntü processing provenance ve raw subpixel ROI geometri/çizim/sayısal edit. ROI çalışma alanı geçicidir, reçeteye kaydedilmez/algoritma çalıştırmaz; recipe binding yeni tool paketlerinde. Release OFF/UI 31/31, ON/core 28/28; normal/küçük ekran görselleri incelendi. [Uygulanan sözleşme](../architecture/roi-geometry.md). Sonraki T2.3; aşağıdaki eski sıra ifadeleri tarihseldir.

9 Ekim onaylı ürün genişlemesi: [reçete tool planı](../architecture/recipe-tools-plan.md). Kod okuma, yönlü kenar/caliper/line/circle/arc, kalibre geometri, dönen parça referansı, Blob/Presence, ön işleme ve kabul kuralları; ardından OCR/OCV, renk, kontur/yüzey kusuru ve bead aynı üç applicationType/Editor/Runtime içinde planlandı. Lens düzeltmesi en son T15. Bu hedefler henüz çalışan özellik değildir; yeni uygulama başlangıcı T0.1, tamamlanmış kalibrasyon/Runtime korunur.

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Güncel kalibrasyon gereksinimleri ve kabul planı](../architecture/calibration-plan.md). Bu revizyon önceki geliştirme sırası ifadelerine göre önceliklidir.

## 7 Ekim 2026 onaylı geliştirme önceliği

Ürün vizyonu korunur. Genel incelemedeki dokuz mimari öneri [geliştirme planına](../architecture/development-plan.md) alınmıştır: ortak yürütme, referans invalidation, registry, session/controller, owned frame, kamera yaşam döngüsü, koordinat modeli, doğrulanmış üretim snapshot'ı ve persistence/build/test güçlendirmesi. Önce 6A, sonra gerçek kamera (6B), Measurement/kalibrasyon/tolerans/Runtime (6C), PLC (6D), robot guidance (6E) ve ürünleştirme (7) uygulanır. Temel servis yetkisi, hata kodları ve cycle logları ilk Runtime'a dahildir.

**Mevcut GUI görünümü, panel yerleşimi, tema, menüler ve navigasyon korunacaktır.** Bu onay iç mimari ve proje akış planına aittir. Aşağıdaki geniş ürün hedefleri tamamlanmış özellik listesi değildir; geçmiş geliştirme sıraları için güncel plan önceliklidir.


**Ürün:** VisionFlow Industrial Vision Suite  
**Doküman sürümü:** 2.0  
**Revizyon tarihi:** 7 Ekim 2026  
**İlgili dosyalar:** `02_machine_vision_ui_gereksinimleri.md`, `../../prototypes/visionflow_ui_prototype.html`

Bu doküman, önceki yazılım tanımını yeni reçete merkezli GUI kararına göre düzenler. İlk tanımın konuşma aktarımında eksik kalan başlangıcı, mevcut gereksinim kapsamından yeniden oluşturulmuştur. Bu dosya güncellenmiş gereksinim dokümanıdır; önceki metnin birebir transkripti değildir.

## 1. Proje Amacı

Kod yazmadan endüstriyel makine görüş uygulamaları oluşturulabilen, mühendislerin araç parametreleri ve veri akışını ayrıntılı biçimde yönetebildiği bir geliştirme ve çalışma ortamı oluşturulacaktır.

İlk kapsam üç uygulama türünü içerir:

| Uygulama türü | Amaç | Temel çıktı |
| --- | --- | --- |
| Robot Guidance | Parçanın konumunu ve yönelimini bulup robot koordinat sistemine dönüştürmek | Robot pozisyonu, yönelim, skor, geçerlilik |
| Defect Inspection | Eksik/fazla özellikleri ve bölgesel kusurları kontrol etmek | PASS / FAIL ve inceleme detayları |
| Measurement | Kenar, çizgi, daire ve noktalardan ölçüm yapmak | Ölçüm değeri, birim, tolerans sonucu |

Bu türler üç bağımsız program değildir. Aynı vision çekirdeğini, tool sistemini, referans mekanizmasını ve Recipe Editor arayüzünü kullanırlar. Uygulama türü yeni reçete oluştururken seçilir; başlangıç araçları, akış ve sonuç sunumu bu seçime göre hazırlanır.

## 2. Genel Mimari

Katmanlar ve sorumlulukları:

| Katman | Sorumluluk |
| --- | --- |
| UI | Reçete yönetimi, görüntü, overlay, özellikler, flow, kullanıcı etkileşimi |
| Application Services | Proje/reçete işlemleri, çalıştırma komutları, doğrulama ve yetki kontrolü |
| Vision Engine | Tool yürütme, bağımlılık çözümleme, sonuç ve süre yönetimi |
| Tool / Plugin System | Acquisition, filter, match, inspection, measurement, logic, output |
| Hardware / Communication | Kamera sürücüleri, robot/PLC protokolleri, bağlantı yönetimi |
| Persistence | Projeler, reçeteler, modeller, kalibrasyonlar, versiyonlar ve kayıtlar |

UI butonları kamera/robot API'lerini doğrudan çağırmamalıdır. Application Services üzerinden komut üretmeli, durum ve sonuç değişikliklerini ilgili servislerden almalıdır.

## 3. Ana Sayfalar ve Navigasyon

Sol menü:

- **Workspace:** Dashboard, Project Manager, Recipe Manager.
- **System:** Camera Manager, Calibration, Communication, Runtime, Diagnostics, Logs, Users, Settings.

Robot Calibration ve Camera Calibration, Calibration altında ayrı sekmeler veya alt sayfalar olarak sunulabilir. HTML prototipinde mevcut iki kalibrasyon bağlantısı korunur.

**Robot Guidance, Defect Inspection ve Measurement sol menüde bağımsız sayfa bağlantıları olarak yer almaz.** Recipe Manager'da uygulama türü sütunu ve tür filtresi bulunur. Bir reçete açıldığında ortak Recipe Editor çalışma alanına geçilir.

Recipe Editor, seçilen reçetenin bağlamsal düzenleme ekranıdır. Ayrı bir genel amaçlı uygulama türü olarak gösterilmez. Üst bölümde proje, reçete adı, uygulama türü, kamera, sürüm ve kaydedilmemiş değişiklik durumu görünür.

## 4. Recipe Teaching Page / Recipe Editor Akışı

Temel kullanıcı akışı:

1. Recipe Manager'ı aç.
2. New Recipe ile ad, kamera ve uygulama türü seç.
3. Ortak Recipe Editor'e geç.
4. Görüntü al veya dosyadan yükle.
5. Tool ekle, görüntü üzerine yerleştir ve parametreleri ayarla.
6. Tool girişlerini başka tool çıktılarıyla ilişkilendir.
7. Tek tool veya tüm akışı test et.
8. Sonuçları ve overlay'leri incele.
9. Reçeteyi versiyon açıklamasıyla kaydet.
10. Doğrulanmış reçeteyi Runtime'da seçerek çalıştır.

Uygulama türü reçetenin üst düzey metadatasıdır. Tür, oluşturma sırasında başlangıç şablonunu belirler; birbiriyle uyumlu ortak tool'ların kullanılmasını gereksiz biçimde engellemez. Sonradan tür değişimi, mevcut akışın giriş/çıkış ve kalibrasyon gereksinimleri doğrulanarak yapılmalıdır.

## 5. Ortak Recipe Editor

| Bölüm | İçerik |
| --- | --- |
| Sol panel | Toolbox: kategoriler, araç arama ve araca ait kısa açıklama |
| Orta panel | Image Viewer: canlı/kayıtlı görüntü, ROI ve sonuç overlay'leri |
| Sağ panel | Properties: seçilen tool için dinamik girişler ve parametreler |
| Alt panel | Flow, Results, Log, Variables, Communication ve Diagnostics sekmeleri |

Panel boyutları değiştirilebilir olmalıdır. Görüntü ve flow seçimi aynı tool seçimini paylaşır. Bir tool görüntüden veya flow'dan seçildiğinde Properties paneli o tool'a geçer.

Ortak prensip: **Tool ekle → görüntü üzerinde yerleştir → Properties üzerinden ayarla → sonucu görüntü üzerinde gör → başka tool çıktısına referans ver → test et → kaydet.**

Toolbox ve örnek başlangıç flow'u uygulama türüne göre öne çıkarılır. Yeni araç türleri aynı editör çatısı içinde sunulur.

## 6. Toolbox Yapısı

### 6.1 Acquisition Tools

- Camera / Image Acquisition.
- Image File: tek görüntü veya test görüntü dizisi.
- Trigger: software veya external trigger kaynağı.
- Exposure ve Gain: kamera desteklediği ölçüde ayarlanır.

Çıktılar: görüntü, frame ID, zaman damgası, çözünürlük, edinim durumu.

### 6.2 Image Processing Tools

Gray Scale, Threshold, Gaussian Filter, Median Filter, Sharpen, Normalize, Morphology, Erode, Dilate, Open, Close.

Girişler: kaynak görüntü ve isteğe bağlı ROI. Çıktılar: işlenmiş görüntü veya region/mask. Kaynak görüntü referansı korunur.

### 6.3 ROI Tools

Rectangle, Circle, Ellipse, Polygon, Free ROI, Rotated Rectangle.

Parametreler: merkez/konum, genişlik, yükseklik, yarıçap, nokta listesi, açı ve koordinat referansı. ROI mutlak görüntü koordinatında veya Match/Tool Output referansında tanımlanabilir. Rotasyon ve öteleme birlikte takip edilmelidir.

### 6.4 Matching Tools

Pattern Match, Shape Match, Template Match, Edge Match.

Parametreler: model, model ROI, arama ROI, minimum score, minimum/maksimum angle, scale aralığı, maksimum sonuç sayısı, overlap ve timeout.

Çıktılar: X, Y, Angle, Score, Center, Pose2D, contour ve match listesi. X/Y için görüntü koordinatı ve birim açıkça belirtilir.

Gösterim: kırmızı kontur, merkez işareti ve koordinat ekseni. Birden fazla sonuç varsa seçilen/aktif match görünür biçimde ayrılır.

Model teaching ve sonuç filtreleme ayrı işlemlerdir. Brush ile istenmeyen match bölgesini dışlamak bir **result filtering mask** oluşturur; modelin kendisini değiştirmez.

### 6.5 Blob Tools

White Blob, Black Blob.

Parametreler: threshold aralığı, Min Area, Max Area, Circularity, Width, Height ve ROI.

Çıktılar: True/False karar, blob sayısı, alan, merkez ve blob listesi. Alan birimi varsayılan olarak px²'dir; mm² gösterimi kalibrasyon gerektirir.

Blob adaylarını seçen filtreler ile PASS/FAIL kabul kriterleri ayrı alanlarda gösterilir. Örneğin aday alan filtresi 100–100000 px², kabul kriteri “en büyük kusur alanı ≤ 500 px²” olabilir.

### 6.6 Defect Tools

Missing Feature, Extra Feature, Blob Inspection, Scratch Detection, Contrast Inspection.

Girişler: görüntü, inspection ROI, gerekiyorsa referans görüntü/model ve parça poz referansı. Sonuçlar: PASS/FAIL (OK/NOK ile eşdeğer kullanıcı gösterimi), kusur detayları ve overlay.

Algoritma/edinim hatası bir parçanın FAIL sonucu ile karıştırılmamalıdır. ERROR veya SKIPPED yürütme durumu ayrıca raporlanır.

### 6.7 Measurement Tools

Edge Detection, Line Detection, Circle Detection, Corner Detection.

Çıktılar: Point, Line, Circle; ayrıca confidence/strength, found durumu ve kaynak koordinat sistemi.

### 6.8 Calculation Tools

Distance, Angle, X Distance, Y Distance, Diameter, Radius, Offset.

Girişler diğer tool'ların point/line/circle/pose çıktılarıdır. Çıktı değer, birim, geçerlilik ve isteğe bağlı tolerans kararını içerir.

Örnek: `Distance_1.P1 = Edge_1.Point`, `Distance_1.P2 = Edge_2.Point`. 523.42 px × 0.025 mm/px = 13.0855 mm; üç ondalık gösterimde 13.086 mm.

### 6.9 Logic Tools

IF, AND, OR, NOT, Compare, Range Check, Counter.

Koşullar sonuçlara referans verebilir. IF dalları ve yürütülmeyen tool'ların SKIPPED durumları açıkça gösterilir. Counter'ın reset ve reçete değişimi davranışı tanımlanır.

### 6.10 Data Tools

Variable, Array, String, Integer, Float; ayrıca Boolean ve yapılandırılmış point/pose nesneleri.

Global ve recipe-local scope desteklenir. Değişkenin tipi, birimi ve read/write kaynağı görünür olmalıdır.

### 6.11 Robot Tools

Robot Pick Position, TCP Offset, Tool Offset, Coordinate Transform, Robot Output.

Girişler: vision pose, geçerli kalibrasyon, hedef robot frame'i, pick referansı ve offset'ler. Çıktılar: X, Y, Z, Rx, Ry, Rz, frame ID ve valid durumu.

2D kameradan elde edilen X/Y/Angle, tek başına tam 3D pose değildir. Düzlemsel uygulamada Z/Rx/Ry reçete veya öğretimden alınabilir; UI bu bileşenlerin kaynağını gösterir.

## 7. Properties Panel

Her tool için ortak yapı:

| Grup | Alanlar |
| --- | --- |
| General | Name, Description, Enabled, Visible |
| Input | Referenced Tool, Input Image, ROI, Coordinate System |
| Parameters | Tool'a özel parametreler ve doğrulama sınırları |
| Output | Typed output port'ları, birimler, değer ve geçerlilik |
| Display | Color, Thickness, Show Result, contour/center/axis seçenekleri |

Parametre değişiklikleri reçeteyi dirty/draft yapar ve eski sonucun güncel olmadığı belirtilir. Hatalı değerler alan üzerinde gösterilir. Başka tool'a referans verilen alanlarda sabit değer ile bağlı değer ayırt edilir.

## 8. Referans Sistemi ve Data Flow

Örnek referanslar:

```text
Match_1.X
Match_1.Y
Match_1.Angle
Match_1.Pose2D
Blob_2.Result
Edge_1.Point
Distance_1.Value
ROI_1.Center
```

Bir tool başka bir tool'un çıktısını kullanabilir:

```text
ROI_Inspection.Reference = Match_1.Pose2D
Blob_1.Region = ROI_Inspection.Region
Distance_1.P1 = Edge_1.Point
Distance_1.P2 = Edge_2.Point
```

Ekranda anlaşılır tool adları kullanılır; dosyada bağlantılar kalıcı tool ID ve output port ID ile saklanır. Yeniden adlandırma bağlantıları bozmamalıdır. Port tip uyumu, eksik referans ve döngüsel bağımlılık çalıştırmadan önce kontrol edilir. Aynı çevrimde alınan görüntü ve sonuçların cycle ID'si ortak olmalıdır.

Parça takibi için yalnızca `ROI.Center = Match_1.Center` kullanmak yeterli değildir; ROI'nin açısı ve lokal offset'i de match pozuyla dönüşmelidir. UI bunu Reference/Pose binding olarak sunar.

## 9. Robot Guidance Modülü

Başlangıç flow'u:

1. Acquire Image.
2. İsteğe bağlı Filters.
3. Search ROI.
4. Pattern/Shape Match.
5. Pick Reference.
6. Robot Transform.
7. Offset.
8. Robot Output.

Kamera görüntüsü alınır, parça koordinatı bulunur, geçerli kalibrasyonla robot frame'ine dönüşür, ilgili frame'de offset uygulanır ve sonuç iletişim servisine gönderilir.

Çıktılar: X/Y/Z, Rx/Ry/Rz, score, frame, recipe ID, cycle ID ve result-valid. Hatalı/eksik match veya geçersiz kalibrasyonda güncel geçerli pozisyon üretilmez. Robot hareket komutu ile vision sonucunun gönderimi ayrı işlemler olarak gösterilir.

## 10. Kalibrasyon Modülü

Pixel to mm, Camera Calibration, Lens Distortion, Eye-to-Hand, Eye-in-Hand, Perspective Correction.

Çıktılar: Calibration Matrix, Transform Matrix; ayrıca model türü, kaynak/hedef frame, birimler, numune sayısı, hata metriği, kamera/lens/çözünürlük bağlantısı, sürüm ve geçerlilik.

Düzlemsel pixel→robot dönüşümü ile tam 3D eye-to-hand/eye-in-hand kalibrasyonları ayrı prosedürlerdir. Tek bir eşleştirilmiş vision/robot pozisyonu genel dönüşüm matrisini belirlemek için yeterli sayılmaz. Wizard seçilen modele uygun çoklu numune ve doğrulama ister.

## 11. Recipe Yapısı

Reçete en az şunları içerir:

- Recipe ID, Name, Description, Application Type.
- Schema Version ve Recipe Version.
- Camera Settings / camera reference.
- Tool List, parameters ve input/output bağlantıları.
- Model, ROI ve result-filtering mask verileri.
- Robot Offset ve frame bilgileri.
- Calibration Data veya versioned calibration reference.
- Variables ve sonuç kabul kriterleri.
- Runtime output mapping.
- Kullanıcı, tarih, versiyon açıklaması ve doğrulama durumu.

Uygulama türü kalıcı alan olarak `applicationType` ile temsil edilir:

| UI adı | Dosya değeri |
| --- | --- |
| Robot Guidance | `robot_guidance` |
| Defect Inspection | `defect_inspection` |
| Measurement | `measurement` |

Reçete durumu DRAFT/VALID/INVALID; yürütme durumu READY/BUSY/DONE/ERROR; parça kararı PASS/FAIL ayrı kavramlardır. Recipe Version, Schema Version ve engine/plugin sürümleri birbirinden ayrılır.

## 12. Runtime Ekranı

Live Image, Tool Overlay, Match Result, Measurement, Robot Position, Communication Status ve Cycle Time gösterilir. Runtime, aktif reçetenin uygulama türüne göre sonuç kartlarını değiştirir; ayrı runtime programları oluşturulmaz.

| Tür | Öncelikli sonuç sunumu |
| --- | --- |
| Robot Guidance | Pick target, frame, robot pose, score, output status |
| Defect Inspection | Büyük PASS/FAIL, kusur sayısı ve alanı, inspection listesi |
| Measurement | Ölçüm, birim, tolerans, PASS/FAIL ve ölçüm overlay'i |

Sonuç çevrim ID'si ve zamanı görünürdür. Trigger bekleme, busy, iletişim hatası ve eski sonuç açıkça ayrılır. Engineer parametre düzenleme ile Operator üretim görünümü yetkiye göre ayrılır.

## 13. Kullanıcı Seviyeleri

| Rol | Yetki |
| --- | --- |
| Operator | Doğrulanmış reçete seçme, Run/Stop/Reset, üretim sonuçlarını izleme |
| Engineer | Reçete oluşturma/düzenleme, tool ayarlama, kalibrasyon, haberleşme |
| Administrator | Kullanıcı/yetki yönetimi, sistem ve proje ayarları |

Yeni reçete oluşturma Engineer/Administrator yetkisi ister. Yetki denetimi yalnızca UI butonlarını gizlemekle sınırlı tutulmaz; servis katmanında da uygulanır.

## 14. Veri Kayıtları

Görüntü, sonuç, alarm, robot verileri, tarih ve kullanıcı kaydedilir. Ayrıca recipe/version, calibration/version, cycle ID, süre, hata kodu ve output durumu kayıtla ilişkilendirilir.

PASS/FAIL görüntü saklama politikaları ve retention ayrı ayarlanır. Export kayıtları filtreleri ve birimleri korur. Reçete düzenleme ve sistem ayarı değişiklikleri audit kaydına işlenir.

## 15. Gelecekte Eklenebilecek Fonksiyonlar

Deep Learning Classification, OCR, Barcode, 3D Vision, Point Cloud, AI Defect Detection, Segmentasyon, Multi Camera, GPU Acceleration, Script Editor ve gelişmiş Vision Flow Editor.

Yeni uygulama türleri New Recipe şablonlarına eklenir. Yeni tool'lar ortak Recipe Editor ve engine/plugin sistemi üzerinden sunulur; her özellik için sol menüde yeni uygulama sayfası açılması gerekmez.

## 16. Temel Tasarım Felsefesi

Yazılım Vision Builder kullanım kolaylığını, HALCON'un esnekliğini, Cognex'in endüstriyel reçete/runtime yaklaşımını ve Zebra Aurora'nın veri akış mantığını ortak platformda birleştirmeyi hedefler.

Operator doğrulanmış reçetelerle üretim yapar. Engineer ortak Recipe Editor içinde araçları, kalibrasyonu ve veri akışını yönetir. Robot Guidance, Defect Inspection ve Measurement uygulama şablonlarıdır; çekirdek ve editör ortaktır.

Bu doküman “uygulama iskeletini oluştur”, “sınıf yapısını tasarla”, “UI sayfalarını üret” veya “tool mimarisini oluştur” görevlerine temel sağlar. Ayrıntılı Tool Architecture & Data Model dokümanı ayrıca tool port tiplerini, yürütme kurallarını, plugin sözleşmesini ve üretim recipe şemasını tanımlamalıdır.

### HTML prototipinin kapsamı

HTML dosyası görsel ve etkileşimli UI prototipidir. Reçete oluşturma, tür seçimi, ortak editöre geçiş, arama/filtreleme, kopyalama, browser draft kaydı, JSON import/export ve tür bazlı runtime görünümü örneklenir. Vision algoritmaları, kamera/robot bağlantısı, gerçek kalibrasyon, sürükle-bırak flow, ROI çizimi, yetkilendirme ve üretim doğrulaması uygulanmış değildir. Run Demo önceden tanımlı örnek sonuçlar gösterir.
