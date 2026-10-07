# VisionFlow iş listesi

7 Ekim 2026. Tamamlananların kabul kanıtları `progress.md` içindedir.

## Güncel onaylı sıra

Genel incelemenin tüm önerileri kabul edildi. Ayrıntı ve kabul koşulları [geliştirme planında](architecture/development-plan.md). **GUI görünümü, tema, panel yerleşimleri, menüler ve navigasyon değişmeyecek.** A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı; sıradaki 6B. Diğer yeni işler uygulanmış özellik değildir; sonraki tarihsel kayıtlar eski teslim kapsamını gösterir.

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
| 6B — GenApi/vendor API ve tek gerçek kamera trigger/grab → DAG | Bekliyor | 6A frame/session altyapısı; gerçek SDK/ABI/disconnect/recovery kabulü |
| 6C — Measurement/kalibrasyon/tolerans/doğrulanmış reçete/Runtime | Bekliyor | Temel servis yetkisi, hata kodları ve cycle logları bu aşamada |
| 6D — PLC handshake ve sonuç teslimi | Bekliyor | Cycle/request ID, ACK, timeout, duplicate/retry politikası |
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

