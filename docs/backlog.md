# VisionFlow iş listesi

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

[Manuel px/mm ve sade Calibration planı](architecture/manual-scale-calibration-plan.md) önceki kısa kapsam notunun ayrıntılı devamıdır. C1 matematik/codec → C2 katalog/Measurement/Runtime → C3 sade ekran → C4 UI kabul → C5 birleşik test/belgeler. C1 matematik/codec tamamlandı; C2 katalog/Runtime/CSV ve C3–C5 ekran/kabul işleri açık. Yeni manual_planar_scale/1, record/dependency schema 3, eski hash/encode koruması, beş bölümlü üç yöntem UI ve doğrulamasız deneme/üretim ayrımı tanımlandı. Belgenin 7. bölümü GPT-6.1 Sol medium için kopyalanabilir başlangıç talimatıdır. P7 bu kapsamdan sonra; K4 ertelenmiş kalır.