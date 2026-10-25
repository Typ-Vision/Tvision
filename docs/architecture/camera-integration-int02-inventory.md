# INT-02 — Snapshot aktarım envanteri

25 Ekim 2026. Snapshot `230cc72` (ebeveyn `cd9bdf2`) → hedef `259e41f`.
Yalnız karar kaydı; ürün aktarımı yapılmadı. [API matrisi](camera-integration-int02-matrix.md).

64/64 snapshot değişiklik yolu aşağıdadır. Blob kimlikleri, her değişen hunk aralığı ve
adlandırılmış scope/sembol adayları [kalıcı JSON envanterindedir](camera-integration-int02-inventory.json).
Build kopyası `build/integration-a0-e2/int02/transfer-inventory.json`, tam satır
farkları aynı kökte `diffs/` altındadır. Hunk aralıkları adsız
lambda, inline üye, include ve CMake değişikliklerini de kapsar; regex sembol listesi
tam AST iddiası değildir. Her dosyadaki bütün hunks bu satırdaki karara tabidir;
karma dosyalarda aşağıdaki matrisin davranış ayrımları önceliklidir. Gelecek kart
kapanışında her hunk/sembol final karşılık veya gerekçeli tarihsel kayıt ile kapatılır.

| Snapshot yolu | Karar / kart | Gerekçe ve sembol kapsamı |
| --- | --- | --- |
| `CMakeLists.txt` | uyarlanacak / INT-03/05/06/07 | Tanı define/library, yeni service/presentation testleri ve smoke hedeflerini mevcut session/plate/responsiveness hedeflerini koruyarak ekle. |
| `README.md` | tarihsel kayıt / INT-09 | Snapshot geçmiş teslim/kanıt bağlamıdır; eski kabul sayıları yeni hedefe taşınmaz. Planın güncel INT sürümü ve kısa hedef giriş belgeleri korunur. |
| `VisionFlow.cmd` | eşdeğer mevcut / INT-09 | INT-01 ile snapshot blob aynen korundu; final launcher/binary eşleştirmesi INT-09. |
| `docs/architecture/architecture.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/camera-integration-a0-e2-plan.md` | tarihsel kayıt / INT-09 | Snapshot geçmiş teslim/kanıt bağlamıdır; eski kabul sayıları yeni hedefe taşınmaz. Planın güncel INT sürümü ve kısa hedef giriş belgeleri korunur. |
| `docs/architecture/output-visibility.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/runtime-calibration-output-acceptance.md` | tarihsel kayıt / INT-09 | Snapshot geçmiş teslim/kanıt bağlamıdır; eski kabul sayıları yeni hedefe taşınmaz. Planın güncel INT sürümü ve kısa hedef giriş belgeleri korunur. |
| `docs/architecture/runtime-calibration-output-fix-plan.md` | tarihsel kayıt / INT-09 | Snapshot geçmiş teslim/kanıt bağlamıdır; eski kabul sayıları yeni hedefe taşınmaz. Planın güncel INT sürümü ve kısa hedef giriş belgeleri korunur. |
| `docs/architecture/runtime-calibration-output-handover.md` | tarihsel kayıt / INT-09 | Snapshot geçmiş teslim/kanıt bağlamıdır; eski kabul sayıları yeni hedefe taşınmaz. Planın güncel INT sürümü ve kısa hedef giriş belgeleri korunur. |
| `docs/architecture/runtime-diagnostics.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/runtime-editor-workflow-plan.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/runtime-error-recovery.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/scoped-active-calibration.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/scoped-calibration-integration.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/architecture/source-verification.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/progress.md` | tarihsel kayıt / INT-09 | Snapshot geçmiş teslim/kanıt bağlamıdır; eski kabul sayıları yeni hedefe taşınmaz. Planın güncel INT sürümü ve kısa hedef giriş belgeleri korunur. |
| `docs/requirements/01_machine_vision_yazilim_tanimi.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/requirements/02_machine_vision_ui_gereksinimleri.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/requirements/03_cpp_qt_opencv_gereksinimleri.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `docs/windows-workstation.md` | uyarlanacak / INT-09 | İlgili A1/B/C/D ürün sözleşme farkı final davranışa göre hedef belgeye işlenir; eski kamera/native sınırlamaları ve hedef CCUX sözleşmeleri korunur. |
| `scripts/run-local.ps1` | uyarlanacak / INT-09 | Eksik EXE mesajını gerekirse taşı; hedefteki launcher ve DLL ortamını koru. |
| `src/app/EditorUsabilitySmoke.cpp` | uyarlanacak / INT-07 | verifyEditorUsability görünürlük/default/empty/cache kontrollerini yeni property commit ve display context sayaçlarıyla birleştir. |
| `src/app/MainWindow.cpp` | uyarlanacak / INT-06/07 | Constructor outputPreferences ve yeni smokeMode bayraklarını hedefin lazy refresh/session çıkış sahipliğine ekle. |
| `src/app/MainWindow.hpp` | uyarlanacak / INT-06/07 | completeRuntimeResult/runtimeSourcesReady, verification ve Output preferences üyeleri; hedef session/cache üyeleri kalır. |
| `src/app/MainWindowCalibration.cpp` | uyarlanacak / INT-06 | measurementWorkspace girişinde verification cancel/ömür koruması; eski makeCalibration gezinmesi kopyalanmaz. |
| `src/app/MainWindowEditor.cpp` | uyarlanacak / INT-07 | refreshOutputVisibilityControls/showToolProperties/applyToolProperties; toggle yalnız sunum, hedef no-op commit ve display cache korunur. |
| `src/app/MainWindowExecution.cpp` | uyarlanacak / INT-07 | showExecutionResult gerçek availability/hidden invalid sunumu; kaynak/frame kimliği ve display context korunur. |
| `src/app/MainWindowRuntime.cpp` | uyarlanacak / INT-03/06/07 | ensureRuntime sağlayıcı ayrımı, verify action/readiness, completeRuntimeResult typed retry ve sonuç filtresi; Starting/poll/Stop/drain ve experimental scalar özeti korunarak uyarlanır. |
| `src/app/MainWindowWorkspace.cpp` | uyarlanacak / INT-06 | loadSelectedRecipe verification hatası reseti ve resolveUnsavedChanges iptal bariyeri mevcut kamera/execution drain çıkışına eklenir. |
| `src/app/MultiSourceSmoke.cpp` | uyarlanacak / INT-07 | verifyMultiSourceRuntime Logs/CSV/PLC payload değişmezliği; normal OECS modeli/experimental sonuçlar ayrıca sınanır. |
| `src/app/RestartRuntimeSmoke.cpp` | uyarlanacak / INT-06 | verifyRestartRuntime legacy/editor9 üç tür restart matrisi; yeni kamera merkezli GUI ve asenkron prepare ile. |
| `src/app/RuntimeFailureSmoke.cpp` | uyarlanacak / INT-03/06 | verifyRuntimeFailures disk lock/InvalidCompletion/Stop/Busy/ilk neden; kamera drain ek negatifleri gerekir. |
| `src/app/RuntimeVisibilitySmoke.cpp` | uyarlanacak / INT-07 | verifyRuntimeVisibility actual digest/terminal/CSV/PLC; experimental ek scalar sızıntısı da kapatılır. |
| `src/app/main.cpp` | uyarlanacak / INT-06/07 | Yeni smoke argümanları mevcut session/responsiveness dispatch yanına eklenir; üretim başlangıcı capture açmaz. |
| `src/application/ExecutionService.cpp` | uyarlanacak / INT-06 | run içindeki adapter factory seam tek kaynağı da kapsar; varsayılan gerçek edinim yolu ve industrial policy korunur. |
| `src/application/GeometryMeasurementService.cpp` | uyarlanacak / INT-06 | execute resolver error bağlamını korur; eylem yeni Camera/Calibration sayfasına gider, model kapısı gevşemez. |
| `src/application/MeasurementWorkspace.hpp` | uyarlanacak / INT-04/05 | configurationForRecipe/productionSources(configurationOnly), verifySources/sourceVerification ve gate/fingerprint; mevcut repository/catalog cache, industrial/experimental provider ve availability korunur. |
| `src/application/MultiSourceRuntimeService.cpp` | uyarlanacak / INT-04 | checkConfiguration salt okunur aday kontrolü; capture/current trusted admission ve industrial session paylaşımı korunur. |
| `src/application/MultiSourceRuntimeService.hpp` | uyarlanacak / INT-04 | checkConfiguration deklarasyonu; ayrı configuration provider için dar API. |
| `src/application/PlcRuntimeSocket.cpp` | uyarlanacak / INT-03 | state/trigger/retryRecording typed hata ayrımı; pending ACK ve durable event sırası, experimental blok korunur. |
| `src/application/PlcRuntimeSocket.hpp` | uyarlanacak / INT-03 | retry/fault contract yorumları ve pending sahipliği; DONE/ACK uydurulmaz. |
| `src/application/ProductionRecipeService.hpp` | uyarlanacak / INT-03/05 | ProductionCycle diagnostics ve recipeContent salt okunur erişimi; trusted digest/admission private sınırı korunur. |
| `src/application/RuntimeCoordinator.cpp` | uyarlanacak / INT-03/05 | firstFailure/failure/recovery/retryRecording ve verification cancellation; requestStart/completeStart/poll/stop kamera yaşam döngüsü yerinde kalır. |
| `src/application/RuntimeCoordinator.hpp` | uyarlanacak / INT-03 | RuntimeFailure/RuntimeRecovery, frozen completion/ticket ve tanı üyeleri mevcut Starting/Stopping API yanına. |
| `src/application/RuntimeDiagnostics.cpp` | uyarlanacak / INT-03 | manifest/publish/append/snapshot/recipeContext; industrial source türü ve kamera prepare/drain olayları eklenir; izin vermez. |
| `src/application/RuntimeDiagnostics.hpp` | uyarlanacak / INT-03 | Bounded thread-safe manifest/event/firstFailure API; immutable cycle ile paylaşım. |
| `src/application/RuntimeService.cpp` | uyarlanacak / INT-03/04/05 | diagnostics/sameCompletion, configuration sağlayıcıları, verification busy/admission; automatic/experimental dispatch ve cameraComposition korunur. |
| `src/application/RuntimeService.hpp` | uyarlanacak / INT-03/04/05 | Ctor configurationScoped/configurationMulti/verification ve tanı API; mevcut kamera preparation/poll/drain üyeleri korunur. |
| `src/application/ScopedActiveCalibration.cpp` | uyarlanacak / INT-03/04 | disconnect tanısı, exact reauthorizeRemembered ve ortak activation; hedef activateImpl, permissionEpoch/cameraEpoch/profile/device ve cache korunur. |
| `src/application/ScopedActiveCalibration.hpp` | uyarlanacak / INT-03/04 | Exact expected-selection wrapper; prepareRemembered aynı ortak çekirdeğe delegasyon. İkinci aktivasyon uygulaması yok. |
| `src/application/SourceVerificationService.hpp` | uyarlanacak / INT-05 | begin/run/accept/cancel/state/admissionAllowedFor immutable worker/owner; duplicate kamera reddi yerine context/profile paylaşımı ve gerçek close/drain. |
| `src/presentation/OutputVisibilityPolicy.cpp` | taşınacak / INT-07 | D1/D2 ayrı presentation resolve/defaults/allPorts veya load/save/reset/showAll; execution/recipe şeması değişmez, explicit empty ve atomik yerel INI korunur. |
| `src/presentation/OutputVisibilityPolicy.hpp` | taşınacak / INT-07 | D1/D2 ayrı presentation resolve/defaults/allPorts veya load/save/reset/showAll; execution/recipe şeması değişmez, explicit empty ve atomik yerel INI korunur. |
| `src/presentation/OutputVisibilityPreferences.cpp` | taşınacak / INT-07 | D1/D2 ayrı presentation resolve/defaults/allPorts veya load/save/reset/showAll; execution/recipe şeması değişmez, explicit empty ve atomik yerel INI korunur. |
| `src/presentation/OutputVisibilityPreferences.hpp` | taşınacak / INT-07 | D1/D2 ayrı presentation resolve/defaults/allPorts veya load/save/reset/showAll; execution/recipe şeması değişmez, explicit empty ve atomik yerel INI korunur. |
| `tests/geometry_measurement_service_test.cpp` | uyarlanacak / INT-06 | resolver nedeninin gerçek hata metninde korunması |
| `tests/multi_source_production_test.cpp` | uyarlanacak / INT-04/05 | restart configuration ve current permission ayrımı; baseline Missing result ayrıca izlenir |
| `tests/output_visibility_preferences_test.cpp` | taşınacak / INT-07 | atomik INI/kimlik/restart/empty/reset |
| `tests/output_visibility_test.cpp` | taşınacak / INT-07 | registry/default/mode/hidden invalid matrisi |
| `tests/runtime_coordinator_test.cpp` | uyarlanacak / INT-03 | frozen completion retry/typed failure/Stop; mevcut kamera durumları da korunur |
| `tests/runtime_diagnostics_test.cpp` | uyarlanacak / INT-03 | bounded manifest/first failure/thread güvenliği |
| `tests/scoped_active_calibration_test.cpp` | uyarlanacak / INT-04 | exact remembered/probe/selection negatifleri; industrial/experimental epoch testleriyle birleştir |
| `tests/scoped_calibration_runtime_test.cpp` | uyarlanacak / INT-04/05 | configuration Load vs operational begin; snapshot testi hedef prepareRemembered yoluna uyarlanır |
| `tests/source_verification_test.cpp` | uyarlanacak / INT-05 | 1/2/10 kaynak, worker/owner, iptal/fingerprint; uyumlu kamera paylaşımı ve OECS/session drain eklenir |

A0 için olmayan uygulama türetilmez; A2 kaldırılmıştır. C2 rapora özgü failing
regression olmadan uygulanmaz. A1/B1–4/C1/D1–4 davranışları yukarıdaki kartlara
dağıtılmıştır. E1/E2 yalnız tarihsel kanıttır; birleşik kabul INT-08/09 içindir.
`VisionFlow.cmd` eşdeğerliği blob düzeyindedir; launcher kabulü değildir.
`.codex/config.toml` ürün envanterine dahil değildir; INT-01 ayrı yedeği korunur.
