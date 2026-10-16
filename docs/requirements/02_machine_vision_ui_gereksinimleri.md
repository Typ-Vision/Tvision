# Endüstriyel Machine Vision Yazılımı — UI Sayfa ve Ekran Gereksinimleri

16 Ekim uygulama durumu — **KMC-0–8 tamamlandı (7a/7b ayrı)**. Reçetesiz iki sihirbaz/dosya denemesi, raster denetimli ayrı association, kurulum başına aktivasyon incelemesi, tek-source Editor/Runtime exact sourceSetup ve terminal6 Logs/CSV uygulandı. Legacy geçmiş açık beyanla bütün sürümleri koruyarak aktarılır; no-op/özgün hash ve eksik raster korunur. Bağımlılık import/export ve saved proje/history Save As bağlıdır; görüntüler otomatik paketlenmez. [Entegrasyon](../architecture/scoped-calibration-integration.md), [migration/taşıma](../architecture/measurement-setup-migration.md). **Sıradaki KMC-9 birleşik A kabulü**; aynı reçetede çok-source B, native/fiziksel/seri numarası/lens/robot/CUDA/dağıtım kabulü açıktır.


16 Ekim 2026 — **KMC-5–8 Sistem özeti ve entegrasyon uygulandı:** Kalibrasyon ana sayfasında
Sistem özeti seçildi. Kamera sayısı sabit değildir: sıfır kaynakta boş durum,
tek kamerada sade görünüm, birden çok kamerada sarılan N kart (8–10 dahil).
Reçetesiz kamera/dosya sihirbazı ve kurulum başına etkinlik ile çok-source
Recipe Editor/Runtime ayrı teslimlerdir. [E1–E7 şartnamesi ve KMC kabul planı](../architecture/independent-calibration-implementation-plan.md).

15 Ekim — **KUI-5 tamamlandı**: ana sayfa Etki incelemesi ve açık legacy aktarımı, bütün metrik araçlarda ortak etkin model, yeni araçta mm varsayılanı, raw/pointMm ayrımı, cache/TargetOnly/Runtime/Logs/CSV bağlıdır. Eski px tolerans/world ROI sayıları sessizce dönüştürülmez. Çalışan Runtime/worker/pending record/ACK değiştirmeyi engeller; Stop sonrasında açık değişim mümkündür. [Akış ve sınırlar](../architecture/system-calibration.md). Sıradaki KUI-6 tam Release/görsel kabulü. Alttaki kayıtlar tarihseldir.


15 Ekim — **KUI-4 tamamlandı**: üç kart, iki ayrı yeniden boyutlandırılan sihirbaz, görüntü üstünde gerçek Edge/Caliper noktaları, chessboard/circle grid, açık dosya/kurulum beyanı, adlandırılmış kayıt/tek etkin seçim ve yalnız robot şablonu. [1400×900/850×600 kabul ve sınırlar](../architecture/calibration-ui-acceptance.md). Bütün ölçüm tüketicileri henüz yeni etkin modele bağlı değildir; sıradaki KUI-5 ve KUI-6. Eski kayıt/kanıt ekranı uyumluluk için korunur. Alttaki kayıtlar tarihseldir.


15 Ekim — **KUI-2/KUI-3 application servisleri tamamlandı**; [gerçek nokta/plaka, confirm ve kayıt sözleşmesi](../architecture/calibration-wizard-services.md). UI henüz yeni session'ı kullanmaz. Sıradaki **KUI-4** üç kart, iki ayrı wizard, açık dosya/kurulum beyanı ve yalnız robot şablonu; mevcut eski Calibration ekranı henüz bu akış değildir. Ortak metrik/Runtime KUI-5'te bağlanacaktır.


15 Ekim — Kalibrasyon sihirbaz planında **KUI-1 application sözleşmesi tamamlandı**; [hazırlık/bağımsız kontrol/tek etkin seçim ve migration](../architecture/active-calibration-contract.md). Yeni servis mevcut UI/Editor/Runtime'a henüz bağlı değildir. Sıradaki KUI-2 gerçek Edge/Caliper session; ekranlar KUI-4, ortak metrik davranış KUI-5 kapsamındadır.

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](../architecture/reference-production-acceptance.md). [R8 paket ve devir kanıtı](../architecture/reference-release-acceptance.md).


11 Ekim güncel kapsam — **R4, R5a/b ve R6 tamamlandı.** Dört kalibre world tüketicisi ortak Point/Pose referansı ve raw px/açı ofsetini her gerçek sampling tap üzerinde kullanır; nominal mm geometri, current mm refit/metrik/tolerans ve exact calibration alan kontrolleri korunur. Yedi ROI tüketicisinde tek **Referans** bölümü kaynak/port, X/Y ve yalnız gerçek Pose için açı sunar. Seçim pending; Bağla/yeniden kaydet/İptal/kaldır ve atomik ofset düzenlemesi ortak servis yolundadır. Viewer güncel çözülen geometriyi salt okunur gösterir; missing/Stop'ta nominal fallback yoktur. Yeni world kayıt **ReferenceBinding schema2 / world-reference-v1**, raw schema1 ve opt-in editor8 korunur; engine **execution/21**. Eski Edge/raw/world pose kayıtları no-op açma/Save ile dönüştürülmez. **R6 birleşik Editor kabulü tamamlandı; sıradaki R7a immutable production/trusted completion, ardından R7b ve R8 nihai paket**; production yeni binding'i açıkça reddeder. Güncel Release UI binary `build/reference-ui/visionflow.exe`; config/local.ui.ini korunur. Alttaki kapsam/sıra kayıtları tarihseldir. [Sözleşme](../architecture/reference-integration.md).

11 Ekim yeni UI gereksinimi — Tek **Referans** bölümü: Görüntüye sabit veya araç/çıktı seçimi; X ofseti, Y ofseti, Açı ofseti; Referansa bağla/İlişkiyi yeniden kaydet/Referansı kaldır. Point kaynakta açı pasif ve sıfır, gerçek pose kaynakta etkin; çizilmiş ROI'nin temel ilişkisi korunur. Pattern/Shape anchor'ı merkez değilse doğru adlandırılır. Temel ofsetler Basic'te; nominal mm/öğretim ayrıntıları gelişmiş bölümdedir. Seçici değişimi pending, kayıt atomik, eksik/geçersiz referans görünürdür. [Ortak Referans planı](../architecture/reference-system-plan.md) R5a/R5b UI davranışı uygulandı; R1–R4 önkoşulları tamamlandı. R6 birleşik Editor ve R7/R8 production/paket kabulü tamamlandı; sonraki T8.4.2b.3c kalan corpus kabulüdür. Alttaki ROI pose etiketleri mevcut/tarihsel uygulamayı anlatır.

## 11 Ekim 2026 — Recipe Editor kullanım kolaylığı talebi (plan)

Kullanıcının sekiz talebi [ayrıntılı davranış ve kabul planına](../architecture/editor-usability-plan.md) işlendi: çizim altında kapalı Elle gir koordinatları; yeni araçta Preprocess öncelikli Görüntü kaynağı varsayılanı; kapalı Çıkışlar; Type/ID satırlarının kaldırılması; görüntü tıklamasının araç seçimini değiştirmemesi; Flow sağ tıkla bir sıra yukarı/aşağı; yeni çizilebilir araçta çizime hazırlık; karmaşık araçlarda Basic/Detailed. İki görünüm birbirini dışlayan seçimdir, ayar değerlerini değiştirmez. Manuel bağlantılar ve DAG doğrulaması korunur. Bu güncel istek önceki GUI koruma kararını yalnız belirtilen etkileşimler için günceller. **U1/U2 uygulandı:** dört metrologi aracında Basic/Detailed ve Elle gir, tüm araçlarda kapalı Çıkışlar/Type-ID sadeleşmesi, viewer seçim koruması, yeni araçta atomik varsayılan Image bağlantısı ve güncel rasterla çizim hazırlığı. **U3/U4 tamamlandı:** Flow sağ tık menüsünde kalıcı Bir sıra yukarı/aşağı; sınır ve Editor busy yönleri pasif. Tıklanan araç taşınırken seçili ID, parametre/ROI ve bağlantılar korunur; liste sırası Save/reopen ile kalır, yürütmeyi DAG belirler ve aktif Runtime snapshot değişmez. Tam Release core52/UI59 CTest ile normal 1400×900/küçük 850×600 görsel kabulü geçti. Native DPI/çoklu monitör ve fiziksel kabul açıktır. Sonraki mevcut T8.4.2b.3c.

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

9 Ekim tool kapsamı onayı: [T0–T15 planı](../architecture/recipe-tools-plan.md) yeni Properties, ok/halka/yay/döndürülmüş ROI/path çizimi, profil/sonuç listeleri, direct mm ve açıklanabilir final karar UI'sını kapsar. Mevcut tema/panel/navigation korunur; bu araçlar için gerekli etkileşim genişletmeleri onaylı kapsamdır. Dairesellikte mm radyal sapma, boyutsuz shape index ve çevre kapsamı ayrıdır; kalibrasyon yoksa sahte mm/PASS yoktur. OCR/renk/kusur/bead plan içinde, lens en son T15. Henüz uygulanmadı; ilk yeni iş T0.1. Eski P9 QR sırası T3 ile güncellendi.

9 Ekim güncel durum: **C1–C5 tamamlandı**. Setup → Calibration üç yöntem ve beş bölüm sunar: Manuel ölçek (px/mm), Bilinen uzunluktan ölçek ve Kalibrasyon plakası (perspektif). Manuel girdi boş başlar; px/mm ve salt okunur tersi, üretim sonucundan ayrı deneme, en az iki bağımsız doğrulama, katalog/recipe Save/reopen, gerçek Edge → Measurement → Runtime ve CSV bağlıdır. Güncel Release binary `build/manual-calibration-ui/visionflow.exe`; Measurement tool 2, execution/6. Sonraki P7/6D.2; K4 fiziksel kabul ertelenmiştir.


8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 P2 tamamlandı: Recipe Manager tek tıkla yalnız satır seçer; **Load Recipe / Edit Recipe** ayrıdır. Load kaydedilmiş bağımsız kopyayı statik kontrol eder; dirty durumda Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal sunulur. Runtime **Run / Stop / Tek çevrim** kullanır; Run görüntü veya parça sonucu üretmeden coordinator ile otomatik hazırlanır. Manuel çevrim, cooperative Stop ve aynı terminal kayıt retry coordinator üzerinden geçer. Seçili, editörde açık ve Runtime yüklü reçete ayrıdır. Eski validate/activate/rollback düğmeleri kaldırıldı; rollback servis API’si korunur. Bu P2 geçmiş kaydı P3/P4 ile genişletildi; güncel davranış yukarıdadır. Fiziksel kabul/K4 ertelenmiş kalır.

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Güncel kalibrasyon gereksinimleri ve kabul planı](../architecture/calibration-plan.md). Bu revizyon önceki geliştirme sırası ifadelerine göre önceliklidir.

## 7 Ekim 2026 — Mevcut GUI'yi koruma kararı

Kullanıcı mevcut GUI'den memnundur. [Onaylanan mimari plan](../architecture/development-plan.md) uygulanırken tema, görünüm, panel yerleşimleri, menü adları/sırası ve navigasyon değiştirilmeyecektir. Yeni menü, node canvas veya genel görsel yeniden tasarım bu çalışmanın kapsamında değildir. Sonraki işlevler mevcut ayrılmış ekranlara ve kontrollere bağlanır; bugünkü hazırlık ekranları çalışan özellik sayılmaz.

Run Match/Run Flow davranış tutarlılığı ve referans invalidation mevcut kontroller üzerinden düzeltilecek; session/controller ayrımı UI iç yapısında yapılacaktır. UI bağlantısı değişen adımlarda Release, ilgili CTest ve mevcut görünümle görsel karşılaştırma gereklidir. Aşağıdaki eski geliştirme sıralarına göre güncel 6A–6E planı önceliklidir; diğer görsel genişletmeler gelecekte ayrı kapsam kararı gerektirir.


**Ürün:** VisionFlow Industrial Vision Suite  
**Doküman sürümü:** 2.0  
**Revizyon tarihi:** 7 Ekim 2026  
**İlgili genel tanım:** `01_machine_vision_yazilim_tanimi.md`  
**Görsel prototip:** `../../prototypes/visionflow_ui_prototype.html`

Önceki UI tanımının 46 bölümlük kapsamı korunmuş ve yeni GUI kararına uyarlanmıştır. **Vision Editor, Defect Detect ve Measurement bağımsız sol menü sayfaları olmaktan çıkarılmıştır.** Robot Guidance, Defect Inspection ve Measurement yeni reçete oluştururken seçilen uygulama türleridir. Üç tür ortak **Recipe Editor** ekranına açılır. Bu doküman hedef ürün gereksinimlerini tanımlar; HTML prototipinin uyguladığı etkileşimler en sonda ayrıca belirtilmiştir.

## 1. UI Genel Tasarım Mimarisi

Ana arayüz modern, koyu temalı, endüstriyel kullanıma uygun ve yüksek kontrastlı olmalıdır. Renk tek başına durum göstergesi olarak kullanılmaz; metin ve ikon eşlik eder.

| Bölge | İçerik |
| --- | --- |
| Sol navigasyon | Workspace ve System ekranları |
| Üst bar | Proje, aktif reçete, uygulama türü, bağlantı/durum ve kaydetme |
| Editor sol panel | Toolbox |
| Editor orta panel | Image Viewer / isteğe bağlı genişletilmiş Flow Editor |
| Editor sağ panel | Properties |
| Editor alt panel | Flow, Results, Log, Variables, Communication, Diagnostics |

Panel genişlikleri/yükseklikleri değiştirilebilir, düzen kullanıcı bazında saklanabilir olmalıdır. Küçük ekranda paneller sekmeli veya alt alta görünmeli; toolbar ve Properties erişilebilir kalmalıdır.

## 2. Ana Menü

### Sol navigasyon

| Grup | Sayfalar |
| --- | --- |
| Workspace | Dashboard, Project Manager, Recipe Manager |
| System | Camera Manager, Calibration, Communication, Runtime, Diagnostics, Logs, Users, Settings |

Calibration içinde Camera / Pixel / Robot alt ekranları bulunur. HTML prototipinde Robot Calibration ve Camera Calibration bağlantıları ayrı korunur.

Recipe Editor, Recipe Manager'da seçilen reçeteden açılır. Uygulama türleri için ayrı navigasyon butonları oluşturulmaz. Yeni uygulama türleri New Recipe seçimlerine eklenir.

### Komut menüleri

| Menü | Komutlar |
| --- | --- |
| File | New Project, Open Project, Save Project, Save As, Import, Export, Exit |
| Project | Project Settings, Recipe Manager, User Management |
| Camera | Camera Manager, Camera Settings, Trigger |
| Calibration | Camera Calibration, Pixel Calibration, Robot Calibration |
| Recipe | Open Editor, Tool Library, Variables, Test, Validate, Version History |
| Communication | Robot, PLC, Protocol Settings |
| System | General Settings, Logging, Diagnostics, About |

Menü komutları rol yetkisine ve geçerli sistem durumuna göre etkinleştirilir.

## 3. Home / Dashboard

Program açıldığında Dashboard gösterilir. Proje, reçete, uygulama türü, kamera ve robot bilgileri üst bölümde görünür.

Orta bölümde canlı kamera görüntüsü ve FPS, resolution, exposure, current recipe, trigger status bulunur. Sağda Camera/Robot/PLC connected durumları, calibration geçerliliği ve recipe ready durumu gösterilir.

Alt bölüm son çevrimi ve son işlemleri gösterir: Cycle Time, Vision Result, Match Score veya uygulama türüne uygun ana sonuç, Robot Response / Output Status.

Durumlar Connected/Warning/Error metni ve yeşil/sarı/kırmızı göstergelerle verilir. Dashboard'da aktif reçeteyi Recipe Editor'de açma veya Runtime'a geçme komutu bulunabilir.

## 4. Project Manager

Yeni proje alanları: Project Name, Project Location, Camera, Robot Type (gerekiyorsa), Communication Protocol ve Calibration. Create / Cancel butonları bulunur.

Proje ağacı Cameras, Recipes, Calibration, Communication, Variables ve Logs düğümlerini içerir. Reçeteler uygulama türü etiketiyle listelenir.

Project Settings'te açıklama, varsayılan donanım ve proje sağlığı gösterilir. Proje ayarları ile aktif reçete parametreleri ayrı yönetilir.

## 5. Recipe Manager ve New Recipe

Recipe Manager operatörün doğrulanmış reçete seçtiği, Engineer'ın reçete oluşturup düzenlediği ekrandır.

Tablo sütunları: Recipe Name, **Application Type**, Description, Camera, Version, Status ve Modified. Search ve Application Type filtresi bulunur.

Komutlar: New, Open Editor, Copy, Delete, Import, Export, Version History. Delete kimliği açıkça gösterir; aktif runtime reçetesi için ürünün kullanım durumuna göre kısıtlanır.

### New Recipe diyaloğu

Önce üç karşılaştırılabilir kart sunulur:

| Kart | Açıklama | Başlangıç akışı |
| --- | --- | --- |
| Robot Guidance | Parçayı bul ve pozisyon/yönelimini robot için hesapla | Image → Match → Pick Reference → Transform → Output |
| Defect Inspection | Bölgesel kusurları incele ve PASS/FAIL üret | Image → Reference → Inspection ROI → Defect → Result |
| Measurement | Mesafe, çap, açı ve özellikleri ölç | Image → Features → Calculate → Limits → Result |

Recipe Name ve Camera girilir. Gerekliyse Calibration Reference ayrıca seçilir. **Create & Open Editor** ortak Recipe Editor'e geçer. Reçete türü `applicationType` alanında saklanır.

İsim benzersizliği, gerekli alanlar ve donanım referansları doğrulanır. Yeni reçete DRAFT durumunda oluşur; yalnızca oluşturulmuş olması onu VALID yapmaz.

Mevcut satır açıldığında uygulama türü otomatik yüklenir. Tür değiştirme komutu başlangıç flow'unu sessizce sıfırlamaz; var olan akışın uyumluluğunu doğrular.

## 6. Recipe Teaching / Ortak Recipe Editor

Önceki “Vision Editor” ortak **Recipe Editor** adını alır. Robot Guidance, Defect Inspection ve Measurement aynı ekran yerleşimini ve aynı tool/data-flow altyapısını kullanır.

Üst bölüm: Recipes'e geri dönüş, Recipe Name, Application Type, Camera, Version, Dirty/Draft durumu, Test/Run, Save, Export ve Runtime.

Ana bölümler Toolbox, Image Viewer, Properties ve alt Flow/Result/Log'dur. Uygulama türü toolbox önceliğini, başlangıç flow'unu ve sonuç kartlarını belirler. Ortak araçlar uyumlu oldukları sürece tüm türlerde kullanılabilir.

Kaydedilmemiş değişiklikler görünür olmalıdır. Reçete değiştirme veya ekran kapatma durumunda değişiklikler kaybolmamalı; draft korunmalı veya açık Save/Discard seçimi sunulmalıdır.

## 7. Toolbox

| Kategori | Araçlar |
| --- | --- |
| Acquisition | Camera, Image, Trigger, Exposure, Gain |
| Image Processing | Gray Scale, Threshold, Gaussian Filter, Median Filter, Sharpen, Normalize, Morphology, Erode, Dilate, Open, Close |
| ROI | Rectangle, Circle, Ellipse, Polygon, Free ROI, Rotated Rectangle |
| Pattern | Pattern Match, Shape Match, Template Match, Edge Match |
| Inspection | Blob, Brightness, Contrast, Defect, Presence, Absence, Missing Feature, Extra Feature |
| Measurement | Edge, Line, Circle, Corner, Distance, Angle, Diameter |
| Logic | IF, AND, OR, NOT, Compare, Range Check, Counter |
| Data / Output | Variable, Result, Robot Output, PLC Output |
| Robot Guidance | Pick Reference, TCP Offset, Tool Offset, Coordinate Transform |

Her nesne drag & drop ile Image Viewer veya Flow Editor'e bırakılabilmelidir. Klavye veya Add Tool komutu alternatif olmalıdır. Araç arama, açıklama ve giriş/çıkış tipleri gösterilir.

## 8. Image Viewer

İşlemler: Zoom In/Out, Fit Image, Pan, Rotate View, Pixel Inspection, Histogram, Grid, Crosshair.

Üst toolbar: Live, Capture, Open, Save Image, Zoom +/−, Fit, Grid, Crosshair, Undo, Redo.

Viewer rotasyonu yalnızca görüntüleme dönüşümü olabilir; gerçek image processing rotasyonu ayrı tool'dur. Koordinat dönüşümleri ROI/overlay seçimini doğru görüntü pikseline eşlemelidir. Piksel koordinatı, görüntü çözünürlüğü ve zoom görünürdür.

## 9. Image Overlay

Match: kırmızı contour, merkez, X/Y ekseni, angle, score ve seçilen match ID. Inspection: ROI ve kusur bölgeleri. Measurement: edge/line/circle, ölçüm çizgileri, değer ve birim.

Örnek: `X: 124.52 px`, `Y: 235.14 px`, `Angle: −7.2°`, `Score: 96.4%`.

Görüntü piksel koordinatı ile kalibre edilmiş mm/robot koordinatı aynı etikette karıştırılmaz. Her overlay kaynak tool ve cycle ID ile ilişkilidir. Disabled/skipped sonuçlar aktif sonuçlardan ayrılır.

## 10. Tool Selection

Image Viewer veya Flow üzerinde tool seçildiğinde Properties otomatik değişir. Seçim görüntü, flow ve sonuç tablosunda senkronize edilir.

Pattern Match örneği: General (Name, Enabled), Search Region (X, Y, Width, Height), Matching (Min Score, Angle Min/Max, Scale Min/Max), Results (X/Y/Angle/Score).

Seçilen tool vurgulanır; Escape seçimi temizler. Overlay'lerin üst üste olduğu bölgede tool seçme listesi sunulabilir.

## 11. Properties Panel

Panel dinamik olmalıdır; yalnızca seçilen tool'un ilgili özellikleri gösterilir. Ortak alanlar Name, Description, Enabled, Visible, Input, Reference, ROI, Coordinate System'dir.

General/Input/Parameters/Output/Display grupları kullanılır. Bağlı alanlarda referans kaynağı gösterilir. Parametre tipi, aralık ve birim doğrulanır. Kaydedilmemiş parametre değişikliği reçeteyi dirty/draft yapar ve eski sonucu geçersizleştirir.

## 12. Match Properties

Alanlar: Model seçimi, Search ROI, Min Score, Max Results, Angle Min/Max, Scale Min/Max, Overlap, Timeout.

Display: Show Contour, Show Center, Show Coordinate. Model status ve son teach zamanı görünür.

Sonuç listesi: Match #, X px, Y px, Angle °, Score %, Filtered/Enabled. Örnek: X 125.4, Y 235.2, Angle 8.2°, Score 96.5%.

Score gösteriminde 0–1 veya 0–100 ölçeği tutarlı seçilir; UI yüzde kullanabilir. Minimum/maksimum açı ve ölçek sınırları kontrol edilir.

## 13. Match Model Teaching

Create Model → Model ROI / ROI şekli → Teach → Test → Update Model → Save.

Kullanıcı görüntü üzerinde model bölgesini seçer. Model preview, modelin kaynak görüntüsü ve ROI'si görünür. Update Model, sonucu filtreleme işleminden ayrıdır. Model kaydı reçete sürümüyle ilişkilendirilir.

## 14. Match Result Filter / Brush

Detected Matches sayısı ve her match contour'u görüntü üzerinde gösterilir. Brush Size px ve Delete Match / Keep Match / Add Region modları bulunur.

Fırça ile işaretlenen bölgeler **result filtering mask** oluşturur; match modelini değiştirmez. Mask referansı Absolute veya Part Reference olarak seçilir. Excluded match sayısı, Undo/Redo ve Clear Mask bulunur.

## 15. ROI Editor

Şekiller: Rectangle, Circle, Ellipse, Polygon, Rotated Rectangle, Freehand.

Properties: X, Y, Width, Height, Radius, Rotation, point list ve Reference. Reference seçenekleri Absolute, Match Pose, Tool Output.

Örnek `Center X = Match_1.X + 20`, `Center Y = Match_1.Y - 15` bağları gösterilebilir. Parçanın rotasyonunu da izlemek için pose binding kullanılır; lokal offset match açısıyla birlikte dönüşür. ROI referans çizgisi ve lokal eksenleri görüntüde gösterilir.

## 16. Defect Inspection Çalışma Düzeni

Defect Inspection ayrı menü sayfası değil, bu türde açılan reçetenin Recipe Editor düzenidir.

Başlangıç akışı: Image → Filter → Reference Match → Tracking Inspection ROI → Blob/Defect → Result. Sabit fikstürde reference match isteğe bağlı olabilir.

Toolbox inspection araçlarını öne çıkarır. Properties seçilen blob/defect tool'a geçer. Viewer kusurları ve tracking ROI'yi; Results inspection listesi ve final PASS/FAIL'i gösterir.

## 17. Defect / Blob Properties

Inspection Type: Black Area, White Area, Missing Area, Extra Area. Threshold Min/Max. Blob Filtering: Min/Max Area, Min/Max Width, Min/Max Height, Circularity.

Result Logic: Presence/Absence, permitted count, permitted defect area veya seçilen karşılaştırma. Filtreler tespit edilen adayları seçer; kabul kriterleri parçanın PASS/FAIL kararını belirler. Bu iki grup aynı anlama geliyormuş gibi gösterilmez.

Örnek: aday filtresi 100–100000 px², en büyük izin verilen kusur 500 px². 734 px² kusur tespit edilirse FAIL. px² → mm² dönüşümü uygun kalibrasyon gerektirir.

## 18. Defect Result

İki seviyeli parça kararı: PASS/FAIL; ayrıca Detected Area, Expected/Allowed Area, Blob Count ve kusur detayları.

Birden fazla incelemede Inspection #1 PASS, #2 PASS, #3 FAIL, Final Result FAIL örneği gösterilir. Final logic AND/OR veya explicit expression olarak yapılandırılır.

ERROR ve SKIPPED yürütme durumları PASS/FAIL'den ayrıdır. Hatalı çalışmış inceleme PASS olarak kabul edilmez. Kusur listesinden ilgili overlay'e odaklanılabilir.

## 19. Measurement Çalışma Düzeni

Measurement ayrı menü sayfası değil, bu türde açılan reçetenin Recipe Editor düzenidir.

Başlangıç akışı: Image → Filter → ROI → Optional Reference Match → Edge_1 / Edge_2 → Calculate → Calibration / Limits → Result.

İki edge aynı kaynaktan beslenen ayrı feature araçlarıdır; biri diğerinin yerine kullanılmaz. Çıktı değeri, birimi, tolerans ve referans kalibrasyon açıkça gösterilir.

## 20. Edge Tool

Image üzerine edge tool bırakıldığında search ROI, scan direction ve bulunan edge noktası çizilir.

Properties: Direction (Left→Right, Right→Left, Top→Bottom, Bottom→Top), Edge Type (Rising/Falling/Any), Threshold, Filter, Min Contrast, Search Length.

Sonuç: Point X/Y, Edge Strength, Found, Execution Time. Algoritma destekliyorsa subpixel seçenekleri ve profile görünümü sunulur. Found=false durumunda bağlı measurement tool geçerli sonuç üretmez.

## 21. Calculate Tool

İki veya daha fazla tool çıktısı seçilebilir. Örnek P1 = `Edge_1.Point`, P2 = `Edge_2.Point`.

Calculation: Euclidean Distance, X Distance, Y Distance; ayrıca Angle, Diameter, Radius, Offset.

Örnek sonuç: Pixel Distance 523.42 px, Scale 0.025 mm/px, Value 13.0855 mm; UI üç ondalıkla 13.086 mm gösterir. Tolerans karşılaştırması yuvarlanmış metin yerine ham değerle yapılır.

Perspective/lens distortion durumunda tek sabit scale yerine kalibrasyon modelinin dönüştürülmüş geometrisi kullanılır. UI kalibrasyon türünü ve birimi gösterir.

## 22. Dinamik Referans Sistemi

11 Ekim revizyonu: kullanıcı yüzündeki tek ad **Referans** olacaktır. Araç/port kaynağı ve X/Y/açı ofset davranışı [ortak planın 2–4. bölümlerine](../architecture/reference-system-plan.md) göre uygulanır. Point kaynakların hareketi yalnız X/Y'dir; gerçek Pose2D kaynağı dönüş de sağlar. Mevcut sabit açılı Template Match ile Pattern/Shape Pose yetenekleri karıştırılmaz. Aşağıdaki pose örneği açı üreten kaynak içindir; bütün kaynakların Pose2D üretmesi zorunlu değildir. Ortak Referans UI kabulü henüz uygulanmadı; R1 yalnız domain paketidir.

Örnek: `Match_1.Pose2D → ROI_Inspection.Reference`; `ROI_Inspection.Region → Blob_1.Region`; `Edge_1.Point → Distance_1.P1`.

Referans seçici tool/output port listesini, veri tipini ve birimini gösterir. Bağlantılar kalıcı ID'lerle saklanır; tool adı değiştirmek bağlantıyı bozmaz. Silinen tool'a kalan referanslar INVALID olarak işaretlenir.

Parça hareket edip döndüğünde tracking ROI ve bağlı inspection tool'ları aynı pose dönüşümünü kullanır. Döngüsel bağımlılık, uyumsuz tip ve eski çevrim sonucu çalıştırma öncesinde/çalıştırırken raporlanır.

## 23. Robot Guidance Teaching Wizard

Wizard, Robot Guidance reçetesinden açılır; bağımsız uygulama ekranı değildir.

1. Kamera seç; exposure/trigger ayarlarını kontrol et ve görüntü al.
2. Match modeli ve arama ROI'sini öğret; sonucu test et.
3. Düzlemsel/3D model ve Eye-to-Hand/Eye-in-Hand kurulumunu seç.
4. Mevcut geçerli kalibrasyonu seç veya Robot Calibration wizard'ına geç.
5. Pick Reference öğret: robotun bilinen pozunu ve ilgili vision match'i eşleştir; robot frame, TCP/tool ve lokal offset'i kaydet.
6. Kalibrasyon + vision pose + pick reference üzerinden hedef robot pozunu hesapla.
7. Önce hesaplanan pozisyonu görüntüde ve sayısal olarak doğrula; gerektiğinde ayrı robot test komutunu kullan.
8. Robot Guidance verilerini reçeteyle kaydet.

Tek eşleştirilmiş pozisyon pick offset/reference öğretmek için kullanılabilir; genel kamera–robot dönüşümünün kalibrasyonu yerine geçmez. “Send Robot To Pick Position” ilk kalibrasyondan önce otomatik hareket adımı olarak sunulmaz. Test komutlarında hedef frame/pose ve iletişim sonucu açıkça gösterilir.

2D match'ten türetilen X/Y/Rz ile öğretimden gelen Z/Rx/Ry kaynakları UI'da ayrı etiketlenir.

## 24. Robot Calibration

Camera, Robot, Calibration Type, model türü, camera/robot frame, TCP/tool ve sample listesi bulunur. Eye-to-Hand ve Eye-in-Hand prosedürleri aynı varsayımlarla hesaplanmış gibi sunulmaz.

Sample tablosunda Vision X/Y veya seçilen modelin camera pose'u; Robot X/Y/Z/Rx/Ry/Rz; frame; timestamp; valid/rejected ve residual gösterilir.

Komutlar: Capture/Add Sample, Remove Sample, Calculate, Validate, Accept Calibration, Save. Seçilen modelin gerektirdiği yeterli ve uygun dağılımlı numuneler olmadan hesap/kabul etkinleştirilmez.

Error alanı birimi ve türüyle gösterilir: örneğin düzlemsel holdout RMS = 0.18 mm. Calibration INVALID mesajı, nedeni ve önerilen işlemle sunulur. Matris, sürüm ve numuneler korunur.

## 25. Camera / Pixel Calibration

14 Ekim 2026 kullanıcı yönlendirmesi: [Kalibrasyon ana sayfası ve sihirbaz planı](../architecture/calibration-wizard-plan.md) bu ekranın yeni hedefidir. **15 Ekim KUI-1 altyapısı tamamlandı; ekran/consumer entegrasyonu henüz uygulanmadı.** Ana sayfada Bilinen ölçü, Kalibrasyon plakası ve yalnız şablon Robot kartları; ayrı adım adım görüntü/gerçek nokta tespiti/mesafe veya plaka/hesaplama/kayıt pencereleri bulunacak. Chessboard ve simetrik circle grid hedeflenir. Hesaplama, tek aktif model seçimi ve bağımsız kontrol ayrı tutulacak; etkinleştirme yalnız ana sayfadan yapılacak, uyumlu tüm metrik araçlar aynı modeli varsayılan kullanacak. Ham X/Y/ID girişi ve tool başına kalibrasyon seçimi varsayılan akıştan kaldırılacak. Sabit 0,05 mm hedefi yoktur; lens ve robot hesabı bu teslimde uygulanmış sayılmaz. Aşağıdaki geniş ürün hedefleri korunur.

Calibration Target, Camera, Resolution, Lens, target ölçüleri, Acquire, Calculate ve Validate bulunur.

Sonuçlar: Pixel Size / planar scale, lens distortion model/coefficients, reprojection error, Camera Calibration Matrix ve Valid status. Tek “Distortion %” alanı model/ölçüm tanımı olmadan yeterli sayılmaz.

Kalibrasyon kamera/lens/çözünürlük ve gerekiyorsa çalışma düzlemiyle ilişkilidir. İlgili ayarlar değiştiğinde valid durumu yeniden değerlendirilir. Kamera kalibrasyonu ile robot dönüşümü ayrı fakat bağlı kayıtlar olarak sunulur.

## 26. Communication

8 Ekim 6D.1 uygulaması: birincil TCP/IP Socket, VisionFlow sunucu/PLC istemci. Mevcut Communication ekranında yerel IP/port/ACK süresi ve enabled Measurement adı seçilir; oturum içi ayarlar, asenkron dinleme/stop/cancel/kayıt retry ve gerçek trace. Runtime doğrula/etkinleştir önkoşuldur. Bu ilk sürüm JSON-LF session/request/recipe/config kimliği, tek peer/outstanding ve 128 accepted request/session sınırına sahiptir; kalıcı dedup/restart teslim kurtarma, vendor/register mapping, Modbus TCP/OPC UA ve fiziksel PLC kabulü henüz yoktur. [Sözleşme](../architecture/plc-socket.md).

Robot/PLC sekmeleri, Protocol (TCP Socket, Modbus TCP, OPC UA), IP Address, Port ve protokole özgü parametreler bulunur. Connect, Disconnect, Test Connection ve Save komutları servis katmanına gider.

Data mapping:

| Yön | Alanlar |
| --- | --- |
| Robot/PLC → Vision | Trigger, Recipe ID, Cycle/Request ID, Robot X/Y/Z/Rx/Ry/Rz |
| Vision → Robot/PLC | Ready, Busy, Done, Result Valid, PASS/FAIL, Vision/Robot X/Y/Z/Angle, Score, Error Code, Cycle ID |

Tip, signed/unsigned, ölçek, birim, register/variable adresi ve byte/word order gösterilir. Trigger/Busy/Done/Acknowledge handshake durumları ve timeout görünürdür. Sonuç pozisyonunun camera mı robot frame'inde mi olduğu açıkça yazılır.

## 27. Communication Diagnostic

Canlı monitor: Timestamp, Direction, Cycle ID, Payload/Data, Protocol ve Status. Örnek sıra RX TRIGGER, RX RECIPE, TX BUSY, TX X/Y, TX RESULT, TX DONE, RX ACK.

Filtreler RX/TX, device, error ve cycle. Pause, Clear, Export bulunur. Kayıtlar işlem zamanıyla ve milisaniye çözünürlüğüyle gösterilir; bağlantı hatası ile vision FAIL ayrı raporlanır.

## 28. Runtime

Üretim ekranı sade olmalıdır. Üstte Recipe, Application Type, Version ve System Ready/Busy/Error. Ortada kamera görüntüsü ve uygulama overlay'leri.

| Tür | Sonuç ve bilgi kartları |
| --- | --- |
| Robot Guidance | Vision validity, pick target, X/Y/Z/Rx/Ry/Rz, frame, score, output acknowledgement |
| Defect Inspection | PASS/FAIL, kusur sayısı, alan, inspection summary |
| Measurement | Value, unit, tolerance, PASS/FAIL, ölçüm summary |

Altta Cycle Time, Camera/Robot/PLC status, Trigger, Cycle ID ve result timestamp. Tek runtime, aktif reçetenin türüne uyarlanır. DRAFT/INVALID reçete üretime alınmadan gerekli doğrulama yapılır. Henüz çalışmamış reçetede sahte PASS veya eski reçetenin sayıları gösterilmez.

## 29. Manual Mode

Yetkiye göre Trigger Camera, Live, Run Vision, Send Result, Robot Request, Reset ve Clear Errors komutları bulunur. Vision test ile robot hareketi ayrı komutlardır.

Tool sonuç listesi: Match_1 PASS, Blob_1 PASS/FAIL, Edge_1 FOUND, Edge_2 FOUND, Distance_1 12.54 mm. Input image/cycle ve sonucu ne zaman ürettiği gösterilir.

## 30. Debug Mode

Engineer için Run All, Run Selected, Step, Stop ve dependency görünümü bulunur. Bağımlılığı eksik tool çalıştırılmaz veya gerekli upstream akış açıkça gösterilerek çalıştırılır.

Tool listesinde OK/FAIL/ERROR/SKIPPED/Disabled, execution time ve primary output görünür. Seçilen tool için input/output image, parameters, result, reference ve zaman gösterilir. Yürütme durumu ile parça kararı ayrıdır.

## 31. Flow Editor

Flow ortak Recipe Editor'ün parçasıdır; hem sıralı liste hem node görünümü sunulabilir. Tool'lar arasında gerçek veri bağlantıları ve typed input/output port'ları bulunur.

Örnek: Camera → Filter → Match. Match pose'u hem Tracking ROI/Blob'a hem Edge ROI'lerine referans olur; Edge_1 ve Edge_2 çıktıları Distance'a bağlanır. Bu dallanma tek sıra listede kaybolmamalıdır.

Add/Delete/Move Node, Connect/Disconnect, Zoom/Fit, Undo/Redo ve Validate bulunur. Grafik döngüsel bağımlılığı ve eksik port'u işaretler. Uygulama türü başlangıç şablonunu belirler; flow editörü ortaktır.

## 32. Tool Node Tasarımı

Node üzerinde icon, tool name, execution status, primary result ve execution time bulunur. Pattern Match örneği: Match_1, DONE, PASS, Score 96.5%, X/Y px.

Selected, Disabled, Running, Error ve Skipped görünümleri ayırt edilir. Seçildiğinde Properties ve görüntü overlay'i aynı tool'a odaklanır. Node üzerindeki kısa sonuç, ayrıntılı output listesinin yerine geçmez.

## 33. Tool Input / Output Portları

| Kaynak port | Hedef port |
| --- | --- |
| `Match_1.Pose2D` | `ROI_1.Reference` |
| `Match_1.X` | `Variable_MatchX.Value` |
| `Edge_1.Point` | `Distance_1.P1` |
| `Edge_2.Point` | `Distance_1.P2` |
| `Robot_Transform.Pose` | `Robot_Output.Pose` |

Portların tipi (Image, Region, Point, Line, Circle, Pose2D/Pose3D, Number, Boolean vb.), birimi ve koordinat sistemi görünür. Uyumsuz bağlantı engellenir veya açık dönüşüm tool'u istenir.

## 34. Variable System

Global ve Local variable desteği bulunur. Örnek Global: CameraScale, RecipeID, RobotX, RobotY. Local: MatchX, MatchY, BlobArea, Distance.

Properties içinden variable seçilebilir. Listede Name, Type, Scope, Unit, Value, Source, Read/Write ve Updated At bulunur. Recipe-local değişkenler başka reçeteye sessizce taşınmaz. İsim ve tip değişiklikleri bağlı alanlarda doğrulanır.

## 35. Alarm / Diagnostic

Alarm tablosu Time, Code, Description, Severity, Source, Cycle ID ve Acknowledged içerir. Örnek V001 Camera disconnected, V014 Match score too low, V031 Calibration invalid.

All/Warning/Error/Critical filtreleri, ayrıntı paneli ve ilgili tool/config ekranına gitme komutu bulunur. Parça FAIL üretim sonucu ile altyapı/algo hatası ayrı kategorilerdir.

## 36. Log

Filtreler Date, Recipe, Application Type, Version, Result, User, Error ve Cycle ID.

Kayıt: Timestamp, Recipe/Version, Image, Vision Result, Robot Position, Calibration Version, Cycle Time, Error, User. Görüntü ve overlay yeniden incelenebilir. Export filtreleri ve birimleri korur. Storage/retention ayarlarıyla ilişkilidir.

## 37. User Management

| Rol | UI yetkileri |
| --- | --- |
| Operator | Valid recipe select, Run, Stop, Reset, sonuç izleme |
| Engineer | Recipe create/edit, tools, calibration, communication, debug |
| Administrator | User/role management, system settings, project management |

Yetkisiz komutlar gizlenebilir veya nedenleriyle disabled olabilir. Servis katmanı aynı yetkileri doğrular. Aktif kullanıcı, rol ve oturum durumu görünür. Sistem ayarları için Engineer'a varsayılan Administrator yetkisi verilmez.

## 38. Settings

General: Language, Units, Theme, Date/Time. Vision: Default Score, Default Timeout, Processing Mode. Camera: Default Exposure, Trigger. Robot: Coordinate System, Unit, Orientation Convention, Timeout.

Logging: PASS/FAIL image storage, retention, output path. UI: panel layout, image/overlay defaults, decimal precision. Varsayılan değişikliği mevcut reçeteyi sessizce değiştirmez.

## 39. Save / Version Control

Kaydetme: Recipe, Application Type, Version, Description/Change Reason, User, Date. Eski versiyonlar korunabilir; production/validated versiyon ile draft ayrı tutulur.

Parametre, model, mask veya kalibrasyon referansı değişimi yeniden doğrulama gerektirir. Reçete sürümü, veri şeması sürümü ve plugin/engine sürümleri ayrı kaydedilir. Dirty göstergesi gerçek kaydetme sonucunda kapanır; hata olursa değişiklikler korunur.

## 40. Import / Export

Üretim recipe paketi örneği `Part_001.recipe`. İçerik: Application Type, tool config, parameters, models, ROI, masks, calibration reference/data, robot transformation, variables, output mappings ve version metadata.

Import sırasında schema/plugin sürümü, referanslar, gerekli modeller ve kalibrasyon uyumluluğu doğrulanır. İsim/ID çakışması açıkça yönetilir. Yeni import draft olarak alınır. JSON veya paket formatı engine mimarisi dokümanında kesinleştirilir; HTML demo JSON yalnızca prototip formatıdır.

## 41. Error Handling UI

Her hata Error Code, Description, Cause, Recommended Action, Source Tool/Device, Cycle ID içerir. İlgili ekrana gitme butonu bulunur.

Örnek V-102 MATCH FAILED: Score 42%, Minimum 70%; Possible Causes: Part missing, lighting problem, wrong recipe, camera moved, model incorrect; Open Match Tool.

Bağlantı/kalibrasyon/parametre hatası ile normal inspection FAIL ayrı mesajlar alır. Son geçerli sonucu yeni hatalı çevrimin sonucuymuş gibi gösterme.

## 42. Temel Kullanıcı Akışları

### Ortak başlangıç

Recipe Manager → New Recipe → Application Type + Name + Camera → Create → **Recipe Editor**.

### Robot Guidance

Capture → Exposure → Filter → Search ROI → Teach Match → Test Match → Filter Unwanted Results → Calibration → Pick Reference Teaching → Transform/Offset → Test Target → Save/Validate → Runtime.

### Defect Inspection

Capture → Filter → Optional Reference Match → Tracking Inspection ROI → Blob/Defect → Threshold + Candidate Filters → Acceptance Logic → Test PASS/FAIL → Save/Validate → Runtime.

### Measurement

Capture → Filter → ROI → Optional Match → Edge_1 / Edge_2 (veya Line/Circle) → Calculate → Calibration → Tolerances → Test → Save/Validate → Runtime.

Üç akışta da editör ve veri modeli ortaktır. Calibration veya Communication'a geçişte aktif reçete bağlamı korunur.

## 43. Kritik UI Kuralı

**Tool ekle → görüntü üzerinde yerleştir → Properties üzerinden ayarla → sonucu görüntü üzerinde gör → başka tool'ların çıktısına referans ver → test et → kaydet.**

Operator doğrulanmış uygulamayı kod yazmadan kullanır. Engineer aynı sistemin parametrelerini ve veri akışını yönetir. UI'da “Vision Editor” bir uygulama türü olarak gösterilmez; ortak çalışma ekranının adı Recipe Editor'dür.

## 44. AI Kod Geliştirme İçin Uygulama Kuralı

Bütün modüller tek seferde uygulanmaya çalışılmamalıdır. Geliştirme sırası:

1. Main Window ve navigasyon.
2. Project Manager.
3. Camera Manager / görüntü kaynağı abstraction.
4. Recipe Manager + Application Type seçimi.
5. Ortak Recipe Editor.
6. Image Viewer.
7. Toolbox.
8. Dinamik Properties System.
9. ROI System.
10. Match Tool / teaching.
11. Blob / Defect Tools.
12. Edge Tool.
13. Calculate Tool.
14. Flow Editor ve typed bağlantılar.
15. Calibration.
16. Robot / PLC Communication.
17. Tür bazlı Runtime.
18. Diagnostics.
19. User Management.
20. Logging / version history.

Her aşama sonunda uygulama compile edilebilir ve çalışabilir olmalıdır. Yeni modül önceki modülleri bozmamalıdır. Kritik akışlar için aşamaya uygun doğrulama yapılır.

## 45. UI Geliştirmede Temel Mimari Kural

UI vision algoritmalarına ve donanım API'lerine doğrudan bağlanmaz.

| Katman | Bileşenler |
| --- | --- |
| UI | Project Manager, Recipe Manager, Recipe Editor, Properties Editor, Runtime |
| Application Services | Recipe load/save, command handling, validation, authorization |
| Vision Engine | Acquisition pipeline, image processing, match, blob, edge, measurement, logic |
| Hardware / Communication | Camera adapters, robot adapters, PLC adapters, protocol services |

UI komut gönderir; sonuçları ve durumları servis üzerinden alır. Yeni kamera/robot protokolü aynı UI çatısını kullanabilir. Application Type bir recipe alanıdır; üç ayrı engine veya editor sınıf ailesi gerektirmez.

## 46. İlk Geliştirilecek UI ve Kabul Kriterleri

İlk ürün aşamasında Main Window, Project Manager, Recipe Manager, New Recipe ve ortak Recipe Editor çalışır hale getirilir. Editor Toolbox, Image Viewer, Properties, Flow ve Result alanlarını içerir.

İlk tool kapsamı Image, ROI, Pattern Match, Blob, Edge ve Calculate. Application Type seçimi aynı editöre farklı başlangıç şablonları yükler. Robot, calibration ve communication daha sonra eklenir.

Kabul kriterleri:

- Sol menüde Vision Editor / Defect Detect / Measurement ayrı uygulama sayfaları bulunmaz.
- Recipe Manager'da her reçetenin uygulama türü görünür ve filtrelenebilir.
- New Recipe üç tür arasından seçim yaptırır.
- Her tür aynı Recipe Editor yerleşimine açılır; başlıkta aktif reçete ve tür görünür.
- Tool seçimi Properties ve overlay'i senkronize eder.
- Tool/output referansları, birimler ve koordinat sistemleri kaydedilir.
- Parametre değişiklikleri dirty/draft yapar; eski sonuç güncel sonuç sayılmaz.
- Save / reopen reçete türünü, tool parametrelerini ve bağlantıları korur.
- Runtime aktif türün sonuçlarını gösterir; henüz çalışmamış reçete PASS görünmez.
- Yetkisiz kullanıcı recipe/system düzenlemesi yapamaz.

### Eşlik eden HTML prototipinin gerçek kapsamı

HTML modern koyu temalı tek dosyalık görsel/etkileşimli prototiptir. Mevcut Dashboard, Project, Calibration, Communication, Diagnostics ve Settings görselleri korunur. Recipe Manager, üç kartlı New Recipe, tek Recipe Editor ve tür bazlı Runtime yeni GUI kararına göre düzenlenmiştir.

Çalışan demo etkileşimleri: sayfa geçişi, reçete arama/tür filtresi, mevcut reçeteyi açma, yeni reçete oluşturma, kopyalama, örnek tool parametresi düzenleme, browser draft kaydı, JSON import/export, sekme değişimi ve Run Demo. Kalıcı draft saklama browser localStorage desteklediği ölçüde çalışır; JSON export bağımsız aktarım sağlar.

Demo kendi önceden tanımlı örnek sonuçlarını kullanır; parametrelerden gerçek vision sonucu hesaplamaz. Image Viewer/Flow/Properties düzenleri uygulama türüne göre örneklenir. Tool/node seçimi görsel olarak vurgulanır; tam dinamik tool editor, gerçek drag & drop, ROI çizimi, node bağlantısı, panel resize, vision engine, donanım bağlantısı, gerçek kalibrasyon ve kullanıcı yetkilendirme sonraki uygulama aşamalarıdır.

## 8 Ekim — Operatör görünümü ve Setup revizyonu

Run başarılı olduğunda Runtime tam ekran olur; sol navigasyon ve File/View/Setup gizlenir ve devre dışı kalır. Sayfa geçişi merkezi guard ile reddedilir. Stop sonrası worker/kayıt durdurma tamamlanınca önceki pencere durumuna dönülür; Runtime Stop ve kayıt retry erişilebilir kalır. Bu bir OS kiosk veya rol/parola sistemi değildir. Communication ve Calibration sol menüden kaldırıldı; File/View yanındaki Setup içinden açılır.

Communication: “TCP iletişimini etkinleştir” işaretlenmeden Dinlemeyi başlat pasiftir. Bu gerekçe ekranda ve tooltip içinde görünür. Ayarları kaydet etkinleştirme veya Run yapmaz; etkinleştir → kaydet → dinlemeyi başlat kullanılır. Otomatik dinleme yalnız uygulama açılışına aittir.

Sonraki geliştirme önerisi (henüz uygulanmadı): önce manuel px/mm ve kalibrasyon ekranını akışa göre sadeleştirme, ardından P7/6D.2 kalıcı dedup/outbox/restart ve kapasite politikası. P8 hardware, P9 OPC UA/QR ve 6E robot daha sonra; K4 fiziksel kabul ertelenmiş kalır. Kullanıcıyla sıra yeniden değerlendirilecek.
## 8 Ekim — Manuel ölçek uygulama planı hazır

[Manuel px/mm ve sade Calibration planı](../architecture/manual-scale-calibration-plan.md) önceki kısa kapsam notunun ayrıntılı devamıdır. C1 matematik/codec → C2 katalog/Measurement/Runtime → C3 sade ekran → C4 UI kabul → C5 birleşik test/belgeler. C1–C5 tamamlandı: matematik/codec, katalog/reçete/Runtime/CSV, sade üç yöntem ekranı ve birleşik yazılım kabulü mevcut. Yeni manual_planar_scale/1, record/dependency schema 3, eski hash/encode koruması, beş bölümlü üç yöntem UI ve doğrulamasız deneme/üretim ayrımı tanımlandı. Belgenin 7. bölümü GPT-6.1 Sol medium için kopyalanabilir başlangıç talimatıdır. P7 bu kapsamdan sonra; K4 ertelenmiş kalır.

16 Ekim KMC-6/7a: reçetesiz bilinen ölçü/plaka/dosya sihirbazları ve raster denetimli ayrı deneme türetimi; kaynak properties’de açık source→exact kurulum bağı uygulanmıştır. Bağlama scoped-source/1 politikasına geçer, bağı kaldırmak legacy fallback sağlamaz. Editor yalnız ilgili immutable kurulum modelini kullanır; raw cache/TargetOnly acquisition0 korunur. Tek-source sınırı sürer. [Sözleşme](../architecture/scoped-calibration-integration.md).
