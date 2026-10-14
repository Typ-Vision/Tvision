# R6 — Birleşik Referans Editor kabulü

Tarihsel R6 kabulü (11 Ekim 2026). Aşağıdaki R7 retleri bu koşum tarihine aittir; yeni referenceBinding production yolu R7 ile açılmış, R8 tam paket kabulü tamamlanmıştır. [Güncel paket ve sınırlar](reference-release-acceptance.md). R1–R5 uygulamasının birleşik Editor/geçiş kabulüdür. Yeni production izni vermez; yeni binding için R7 ret kapısı korunur. [Davranış planı](reference-system-plan.md), [servis ve kayıt](reference-integration.md), [koşum kanıtları](../progress.md).

| Kabul | Gerçek kanıt / test |
| --- | --- |
| Kaynak/tüketici yetenekleri | `reference_graph_contract`: registry'den türetilmeyen açık tablo; 11 kaynak-port × 7 tüketici = 77 çift. Her çift için typed DAG, strict editor codec roundtrip, kapalı kaynak, yanlış Point/Pose girişi, eksik port ve self retleri. Registry'nin diğer çıktıları da açık tabloya karşı kontrol edilir. |
| Üç applicationType | `reference_execution_contract`, `reference_inspection_contract`, `world_tracking_dag_contract`, `ui_pose_tracking_smoke`: Measurement, DefectInspection, RobotGuidance aynı servis/Editor yolunu kullanır. |
| Gerçek raw yürütme | Template Point ve Pattern/Shape Pose; Edge Point/Caliper/Line/negative Arc/legacy Edge/Blob bağımsız taşınmış/dönmüş rasterlarda kendi algoritmasını çalıştırır. `reference_inspection_contract` ZXing ON'da her üç türde gerçek bağımsız Zint QR metni/köşelerini doğrular; OFF açık capability reddini sınar. |
| Zincir ve geçiş | Pattern/Shape → Circle → Edge Point, sabit `(77,95)` raster oracle'ı; son araç Circle merkezinin yalnız X/Y'sini izler. Zincirin normal Save/reopen/no-op byte ve revision korunması, yeniden yürütmede aynı gerçek sonuç. Legacy Edge/raw/world pose no-op ve migration mevcut testlerle birlikte çalışır. |
| Upstream / TargetOnly | Circle eşik editi matcher cache'ini korur, Circle ve downstream Edge cache'ini geçersiz kılar. Edge gizli acquisition/upstream yapmadan SKIPPED olur. Explicit Circle Run Tool ardından Edge Run Tool aynı oracle'ı verir; iki çağrıda da acquisition=0, tek executor, yeniden öğretim yoktur. `editor_target_only` bağımsız dal ve geç callback/Stop kontrollerini korur. |
| Ortak Properties / Viewer | `ui_pose_tracking_smoke`: yedi tüketicide tek Referans, Point açı sıfırlama, pending/iptal/bağla/kaldır, tek ofset, invalid rollback, read-only gerçek mouse, nominal kayıt koruması, raw/world teaching, missing/ambiguity/Stop temizliği. |
| U1–U4 | `ui_editor_usability`, `roi_viewer_interaction`, `tool_creation_policy`, `tool_order_policy`: Basic/Detailed/Elle gir/Çıkışlar tercihleri, kaynak ekleme/çizim/scroll/seçim/Escape/Stop/geç görüntü ve Flow sırası. |
| Production sınırı | `production_recipe_contract`, ortak execution/UI testlerindeki Runtime configuration retleri, `runtime_persistence`, `typed_terminal_trace`: Editor binding'i production izni üretmez; eski terminal yolları korunur. |

77 çift capability/graph/codec kabulüdür; her çift için ayrı kamera corpus kabulü anlamına gelmez. Gerçek raster kabulü her tüketicide Point ve iki Pose üreticisiyle; kalibre dört tüketicide ayrıca uniform/Y-up/non-affine mapping ile yapılır. UI görüntüleri offscreen 1400×900 / 850×600'dir; native DPI ve fiziksel cihaz kabulü değildir.

Yerel Release ve DLL PATH README'deki gibidir. `build/reference-core` Widgets OFF/ZXing OFF; `build/reference-ui` Widgets ON/pinned offline ZXing ON. Yeni indirme/kurulum gerekmez. R6 logları her dizinde `r6-*.log`, UI görselleri ve INI dosyaları `build/reference-ui` altındadır. Üretim `config/local.ui.ini` ve açık kullanıcı uygulaması korunur.

R7a/b ve R8 tamamlandı; sıradaki **T8.4.2b.3c corpus/production kapanışı**. K4/P7/6D.2/6E/lensT15, hassasiyet/latency/CUDA/kurulum/dağıtım ayrı açık kalır.
