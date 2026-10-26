# COMM-01 — Güncel kaynak haritası ve baseline

26 Ekim 2026 · Uygulayıcı: Codex. Ürün kodu değişmedi.
İncelenen başlangıç commit'i: `d4bfb02547c9e9362f5ef9b5be7523ce1b607c3b` (`main`).
Bu envanter COMM-02 ve sonraki kartlar için başlangıçtır; hedef sözleşmesi
[contract.md](contract.md), kart kabulü [verification.md](verification.md) içindedir.

## İndeks ve kaynak doğrulaması

`list_projects` iki kök döndürdü: `C:/OpenCode/TVis` ve `C:/OpenCode/TVision`.
AGENTS'teki eski `C:/Projects/Tvision` dizini yok; o mutlak yola `index_status`
`project not found or not indexed` verdi. Bu hata boş sembol araması sayılmadı.
Seçilen `C-OpenCode-TVision` projesinin canonical/worktree kökü
`C:/OpenCode/TVision`, branch'i `main`, HEAD'i yukarıdaki Git SHA ile aynıdır.
Böylece başka bir checkout'un indeksi kullanılmadı; sorgularda actual mutlak kök kullanıldı.

Tier 2: `get_graph_schema`, dar `search_graph`, `trace_path(direction=both,depth=1)`
ve kritik `get_code_snippet` çağrıları ardından doğrudan kaynak doğrulaması yapıldı.
İndeks generation `2026-10-26T18:31:49Z`; 10696 node, 60495 edge; watched/ready.
Sembol ve ToolGraph aramaları `has_more=false`; coverage 34 yolun tamamını içerir.

Kanıtlar `build/comm/COMM-01/` altında `graph-status.json`, `graph-trace.json`,
`graph-coordinator.json`, `graph-values.json`, `coverage.json`, satır numaralı
`source-evidence.txt` dosyalarıdır. Coverage best-effort'tür; temiz sonuç eksiksizlik
kanıtı değildir. Kısmi parse satırları doğrudan okundu:

| Dosya (`src/application/`) | Kısmi satırlar |
| --- | --- |
| ProjectService.hpp | 22, 26, 47, 51 |
| EditorSession.hpp | 81, 83, 88, 96, 98–100, 102 |
| ExecutionService.hpp | 74–75, 83, 86 |
| RuntimeService.hpp | 24, 44, 53, 55, 57, 66 |
| RuntimeCoordinator.hpp / .cpp | 16, 20, 34 / 10 |
| PlcRuntimeSocket.hpp | 23, 27, 76 |
| ProductionRecipeService.hpp | 73, 80 |

Diğer seçili yollar `no_recorded_issue/metadata_match` döndü. İlk aday
`IndustrialNativeFactory.cpp` mevcut bir dosya değildi; `freshness=missing`
sonucu üzerine gerçek factory `IndustrialCameraProvider.cpp` ile eşlendi ve
coverage yeniden kontrol edildi. Nihai harita bu hayali dosyaya dayanmaz.
Graph bazı C++ çağrılarını kaçırıyor veya kısa adları yanlış eşliyor:
`RuntimeCoordinator::begin` için sıfır inbound sonucu, socket kaynağındaki açık
çağrıyla çelişir; `connect/clear/run` gibi yanlış aday edge'ler doğrulanmış zincir
sayılmadı. Sorumluluk/zincirler aşağıda exact kaynakla tamamlandı. Graph sonucu
negatif veya eksiksiz çağrı envanteri iddiası olarak kullanılmaz.

## Actual dosya, sembol ve extension noktaları

Yollar repo köküne göredir; `.hpp/.cpp` çifti birlikte incelenmiştir.

| Actual yol / sembol | Mevcut sorumluluk | Önerilen genişletme sınırı |
| --- | --- | --- |
| `src/domain/ToolValues.*`: `ToolValueData`, `ToolValues::encode/decode` | Qt Core tabanlı bounded typed payload; Text, CodeResults, PointSet, Profile, Line2D, Circle2D, Arc2D, Pose2D, InspectionResults | Scalar/spatial değerleri burada veya aynı domain katmanında genişlet; acquisition/ağ bağımlılığı ekleme |
| `src/domain/ToolRegistry.*`: `ToolDescriptor`, `entries/registerType/get/validate` | Tool parametreleri, portları, minimum editor schema ve backend bildirimi | Set Variables/Result/Publish descriptor ve doğrulamaları ortak registry'ye eklenir |
| `src/domain/ToolGraph.*`: `PortType`, `PortDefinition`, `editorConnections`, `validateGraph/validateSourceGraph` | Typed port DAG, yürütme sırası, required kaynaklar ve source provenance; `ToolGraph` adında sınıf yok | Yeni bundle portu/dependency doğrulaması mevcut graph'ı genişletir; variable adıyla gizli dependency oluşturulmaz |
| `src/domain/ToolDefinition.cpp`: `editorTools`, `withEditorTools`, `requiredEditorSchema`, `validateEditorDefinition` | Strict editor/tool JSON, eski şema okuma ve açık edit ile yükseltme | Yeni kayıt alanlarının schema/minimumEditorSchema karar noktası |
| `src/application/ProjectService.*`: `save`, `setTools`, `setConnections`, `setRuntimeOptions` | Draft/dirty/revision ve explicit edit; no-op Save erkenden döner; disk snapshot conflict ve atomik save sonrası yayın | Variable/output-group edit komutları burada; JSON'a UI'dan doğrudan yazma yok |
| `src/persistence/ProjectRepository.*`: `load/encode/save`; `src/domain/Project.cpp`: `validateProject` | Root/recipe codec, 16 MiB proje sınırı, desteklenmeyen alan/sürüm reddi, QSaveFile | Yeni domain tanım codec'ini koruyarak root/recipe bump gerçekten gerekiyorsa açıkça seç |
| `src/application/ExecutionService.*`: `PortData`, `PortValue`, `CycleResult`, `run/runTool/toolSignature/version`; `ToolExecutors.cpp` | Worker yürütme, typed output ve cache/provenance; private completion authority; Run Tool partial evaluation | Atomik assignment evaluator ve cycle context mevcut yürütmeye bağlanır; Editor preview transport çağırmaz |
| `src/application/EditorSession.*`: `JobSession`, `signature/evaluation/reconcile/acceptTool/acceptCycle` | Immutable worker girdisi, generation/signature cache kabulü, cooperative Stop ve drain | Yeni output bundle cache/signature invalidation'a dahil edilir |
| `src/application/RuntimeService.*`: `prepare/requireReady/begin/execute/finish` | Immutable snapshot, kaynak verification, camera lifecycle, legacy/multi/experimental dispatch, terminal persistence | Variable/input/publication snapshot mevcut active snapshot'a bağlanır |
| `src/application/RuntimeCoordinator.*`: `load/requestStart/completeStart/begin/finish/stop/retryRecording` | Owner-thread yaşam döngüsü, trigger mode, recording/delivery kapıları; frozen completion retry | Ortak Trigger Router bu admission yolunu kullanır; transport doğrudan yeni acquisition açmaz |
| `src/application/ProductionRecipeService.*`: `recipeContent/requireReady/beginCycle/finishCycle`; `MultiSourceRuntimeService.cpp`: `capture/current/begin/finish` | Permission ve bağımlılık kontrolü, tek-cycle ticket, kalıcı begin/terminal, immutable completion digest | Snapshot hash ve terminal bundle burada genişler; yeni izin veya sentetik başarı üretilmez |
| `src/application/ProductionStore.*`: `append/recent`; `ProductionResultCodec.*`: `output/validate/csv`; `ToolResultCodec.cpp`: `payload/validate/encode/decode` | Append-only hash envelope, idempotent aynı ID/payload, strict terminal/typed port codec ve CSV | Outbox/ledger eventleri terminali overwrite etmeden eklenir; iki dosya yazımı atomik bundle sayılmaz |
| `src/application/PlcRuntimeSocket.*`: `receive/trigger/retryRecording/acknowledge/tick` | vf.socket/2 listener, bounded framing, tek peer/outstanding, session RAM dedup, commit→DONE→ACK | v2 korunur; v3 ayrı explicit endpoint/codec/ortak router portu kullanır |
| `src/application/IndustrialCameraProvider.cpp`: `CameraProviderFactory::capabilities/open`; `IndustrialExecution.cpp` | Normal factory native edinim yetkisi vermez; injected provider test sınırında | Haberleşme kartı factory/admission engelini kaldırmaz |

## Doğrulanmış zincir ve admission sınırları

Socket `receive` → `trigger`: önce exact duplicate/conflict, sonra pending/worker/
128-entry kapasite ve hazır olma kontrolleri; recipe/config exact snapshot eşleşmesi
ve Measurement mapping kontrolü; `PLC_REQUEST` append → coordinator `begin`
(SoftwareTrigger) → Runtime `begin` → legacy `beginCycle` veya multi `begin`.
Worker `RuntimeService::execute` yalnız dondurulmuş lease'i kullanır.
`retryRecording`: coordinator/Runtime `finish` terminal commit → `PLC_RESULT`
append → DONE gönderimi. `acknowledge`: `PLC_ACK` append → deliveryPending kaldırma
→ ACKED. Disk retry aynı sonucu korur, acquisition tekrarlamaz.

Coordinator Running ve doğru trigger mode ister; deliveryPending yeni begin'i
kapatır. Runtime source verification, principal/Run izni, aktif snapshot, busy ve
current dependency kapılarını korur. Stop generation/cancel ve kamera drain yoludur;
kalıcı terminal veya outstanding teslimi silmez. Partial Editor sonucu Runtime
terminali olamaz. Native factory varsayılan capabilities tamamı false ve açık
Unavailable gerekçesidir; `open` injected+open+configure ister. Mevcut deneysel
OECS yolu ve test injection native production kabulü oluşturmaz.

## Güncel şemalar ve bump noktaları

| Sözleşme | Kaynakla doğrulanan mevcut değer | Sonraki değişiklikte koruma |
| --- | --- | --- |
| `.vfproject` root / recipe | root 1; recipe 1 veya 2 (`calibrationBinding` yalnız 2) | Repository ve validateProject beraber; no-op byte/hash ve revision korunur |
| Editor | okuyucu 1–9; `requiredEditorSchema` explicit edit tabanı 6, dependency 7, reference/world 8; explicit multi-source 9 | Yeni alan/type için desteklenen üst sınır ve strict keys birlikte; bu kart bump yapmadı |
| Execution identity | `visionflow.execution/30` (`ExecutionService.hpp::version`) | Yürütme semantiği değişirse bump; eski engine28 metinleri güncel değildir |
| Typed payload / port | `ToolValues` 1 / `ToolResultCodec` 1 | Number=double ve Boolean=bool mevcut PortData'da; yeni Int64/Float64/envelope anlamı eski veri gibi okunmaz |
| Terminal | `ProductionResultCodec::validate` 1–9; yeni single-source 3/4/5/6/8, whole-set 7 veya industrial 9 | Legacy okuyucu korunur; yeni bundle alanı schema9 exact alan setine sessiz eklenemez |
| Production archive envelope / event | `visionflow.production` 1; CYCLE_BEGIN ve PLC event 1 | Terminal immutable; ACK/delivery ayrı olay. Store kind whitelist genişletmesi gerekiyorsa açık yapılır |
| Wire | `vf.socket/2`, `v=2`; strict STATUS/TRIGGER/ACK | Yeni TCP v3 ayrı endpoint; v2 PASS/FAIL anlamı korunur |

Store sınırları: 20 MiB dosya, 512 MiB toplam ve 10000 dosya; otomatik retention yok.
Terminal validator 16 MiB sınırını uygular. Socket input satırı 2048 byte, partial
2 s, read buffer 8192 byte, output queue 65536 byte; ACK default 2 s/3 retry.
Session RAM dedup process restart'ta yüklenmez; kalıcı v3 dedup/outbox henüz yok.

## CMake target ve ilgili CTest eşlemesi

`visionflow_projects`: domain, ProjectRepository, ProjectService; Qt Core.
`visionflow_acquisition`: Execution/Editor/Runtime/Production/Socket; Qt Network,
Gui/Concurrent/Multimedia, OpenCV ve hardware. `visionflow` Widgets ON uygulaması;
`visionflow_plc_simulator` UI OFF'ta da vardır. UI OFF, Qt Gui/acquisition
bağımlılığı yok demek değildir; Widgets uygulama/viewer hedefleri kapanır.

| Alan | CTest → executable target |
| --- | --- |
| Typed values/migration | `tool_values_contract` → `tool_values_test`; `tool_migration_contract` → `tool_migration_test` |
| Graph / execution | `multi_source_graph_contract` → `multi_source_graph_test`; `multi_source_execution`, `multi_source_target_only` → `multi_source_execution_test`; `tool_execution` → `execution_test` |
| Editor/cache/session | `editor_target_only` → `target_only_test`; `session_contract` → `session_test`; `editor_services` → `editor_services_test` (yalnız UI ON) |
| Project | `project_persistence` → `project_service_test` |
| Immutable production | `production_recipe_contract` → `production_recipe_test`; `multi_source_production_contract` → `multi_source_production_test` |
| Typed/whole-set terminal | `typed_terminal_trace` → `typed_trace_test`; `multi_source_terminal_trace` → `multi_source_terminal_test` |
| Runtime/store | `runtime_persistence` → `runtime_test`; `runtime_coordinator` → `runtime_coordinator_test`; `runtime_diagnostics` → `runtime_diagnostics_test` |
| TCP | `plc_socket_runtime` → `plc_socket_test` |
| Native kapısı | `industrial_runtime_admission_contract` → `industrial_runtime_admission_test`; `industrial_provider_contract` → `industrial_provider_test` |

## Exact baseline komutları

PowerShell, repo kökü `C:/OpenCode/TVision`. Eski checkout'a bağlı ZXing prefix
kullanılmadı. TEMP/TMP yalnız test process ortamında repo altına alındı; normal
kullanıcı TEMP kabulü veya native UI görsel kabulü iddia edilmez.

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
cmake -S . -B build/reference-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix
cmake --build build/reference-ui -j 6
cmake -S . -B build/reference-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF
cmake --build build/reference-core -j 6
$env:TEMP = (Resolve-Path build/comm/COMM-01).Path
$env:TMP = $env:TEMP
$commRegex = '^(tool_values_contract|tool_migration_contract|multi_source_graph_contract|multi_source_execution|multi_source_target_only|multi_source_production_contract|multi_source_terminal_trace|typed_terminal_trace|editor_target_only|runtime_diagnostics|runtime_coordinator|plc_socket_runtime|runtime_persistence|project_persistence|tool_execution|production_recipe_contract|industrial_runtime_admission_contract|industrial_provider_contract|session_contract|editor_services)$'
ctest --test-dir build/reference-ui -N
ctest --test-dir build/reference-ui -R $commRegex --output-on-failure
ctest --test-dir build/reference-core -N
ctest --test-dir build/reference-core -j 2 -R $commRegex --output-on-failure
# Localhost izni gereken testler için aynı ortamda ayrı koşum:
ctest --test-dir build/reference-ui -R '^(multi_source_terminal_trace|plc_socket_runtime)$' --output-on-failure
python scripts/check-context-docs.py
git diff --check
```

Baseline sonuçları ve koşum ayrımı [COMM-01 kapanışında](../../archive/2026-10-26-comm-01.md)
saklanır. Liste komutları full suite koşumu değildir; seçili regex UI ON'da 20,
UI OFF'ta 19 test seçer. Ürün/UI kodu değişmediğinden ekran görüntüsü kontrolü
koşulmadı. Fiziksel mm/PLC/robot, native correlation/Stop bariyeri, lens, native
DPI/çok monitör, CUDA ve dağıtım engelleri korunur.

## COMM-06 güncel genişletmesi

`set_variables` registry sürümü 1, minimum Editor şeması 7; mevcut reader 1–9
korunur. `AssignmentConfig.*` strict parametre/bundle codec'ini taşır.
`assignmentConnections` / `executionConnections` kayıtlı satırdaki producer UUID,
port ve revision'dan DAG kenarı türetir; variable adı üzerinden bağ kurulmaz.
`AssignmentExecution.*` saf evaluator'ı mevcut ExecutionService'e bağlar.
`PortType::AssignmentBundle` / `PortData::AssignmentBundle` cycle/config temellidir;
görüntü frame'i veya geometrik source yerine geçmez. `ToolResultCodec` typed
roundtrip'i doğrular. Execution kimliği **visionflow.execution/31** oldu.

EditorSession aynı dependency kenarlarını invalidation ve hedef kapsamı için
kullanır; toolSignature geçişli parametreler yanında variableCatalog'u içerir.
Yeni CTest `assignment_execution_contract` → `assignment_execution_test`.
ToolCatalog sunum filtresi bu aracı COMM-08 UI teslimine kadar gizler.
[Kabul ve sınırlar](../../archive/2026-10-26-comm-06.md).

## COMM-07 güncel UI genişletmesi

`src/app/MainWindowVariables.cpp`: ortak Editor Variables sekmesinde katalog,
add/edit/cancel/remove ve salt okunur system satırları. ProjectService variable
komutları kullanılır; UI JSON proje dosyasını yazmaz. Enum metadata ve opsiyonel
frame kısıtı aynı VariableDefinition doğrulamasına gider; UUID düzenlenemez.
`refreshFlowView` session reconcile sonrası kabul edilmiş cycle/bundle alanlarını
gösterir; variable tanımına değer yazılmaz. Geçersizleşmede Missing/boş görünür.
Katalog seçiliyken alt panelin minimum alanı satır/hata görünürlüğünü korur.

`tests/editor_variable_catalog.cpp`, `main.cpp --variables-smoke`, CTest
`ui_variable_catalog`: gerçek image_file → Set Variables Boolean false,
UUID/no-op/cancel, referanslı silme/tip hatası, sistem erişimi, Properties binding,
klavye/scroll ve 1400×900/850×600 görünür satır alanı. Mevcut
`tests/editor_property_commits.cpp` değişmedi; `ui_editor_usability` regresyonunda
çalıştı. Şema/execution sürümü değişmedi; Set Variables tool UI COMM-08'e kadar
kapalıdır. [Kapanış](../../archive/2026-10-26-comm-07.md).
