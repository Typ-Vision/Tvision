# INT-08 — Birleşik kabul matrisi ve aktarım karşılıkları

25 Ekim 2026. Uygulayıcı: Codex. Yazılım/görsel kabul tamamlandı.
[Plan](camera-integration-a0-e2-plan.md), [kapanış](../archive/2026-10-25-int-08.md),
[özgün hunk/blob envanteri](camera-integration-int02-inventory.json).
Kanıt kökü `build/int08/`; `manifest.json` kaynak, EXE/cache ve kanıt hashlerini taşır.
Üç medyan mühendislik hedefi aşımı kapanışta açıktır; doğruluk kapısı değildir.
Fiziksel E2 ve native üretim kabulü bu sonuçtan ayrı ve açıktır.

## Kabul matrisi

| Davranış | Son kaynakta kanıt |
| --- | --- |
| Release UI/ZXing ON | Tam CTest 135/135; `on-accepted.xml/.log`, exit 0 |
| Release UI/ZXing OFF | Tam CTest 107/107; `off-accepted.xml/.log`, exit 0 |
| 7×7 / 49 merkez / yön / fit / save-reopen-replay | framed_calibration_plate_contract/external, calibration_plate_contract, calibration_wizard_contract, ui_calibration_wizard, ui_independent_calibration_wizard |
| Bilinen ölçü / chessboard / model-scope | chessboard_contract, calibration_persistence, scoped_active_calibration, scoped_calibration_runtime/editor, ui_calibration_overview ve activation testleri |
| Kamera kimliği / ayar / Stop-drain / sahiplik | ui_camera_manager, camera_settings, camera_acquisition_sessions_contract, runtime_camera_sessions_contract, calibration_camera_sessions_contract ve Editor/Runtime UI session testleri |
| Restart / exact Verify / whole-set | source_verification, ui_restart_runtime, multi_source_graph_contract, multi_source_production_contract, industrial_runtime_admission_contract |
| Output / immutable sonuç / retry / Logs-CSV / ACK | output_visibility_preferences/policy, ui_runtime_visibility, ui_multi_source_runtime, runtime_coordinator, runtime_persistence, multi_source_terminal_trace, plc_socket_runtime, ui_shell_smoke |
| Performans | 600 yeni ölçüm; medyan hedefi 27/30, p95 hedefi 30/30; warm decode/replay sıfır; `performance/evidence-validation.json`, `hot-path-report.json` |
| Görsel | 24 PNG, normal 1400×900 ve 850×600 çiftleri; `visual-reviewed.json`, `visual-review.md`, dört temas sayfası |

Fixture gerçek fiziksel doğrulama değildir. Gizli çıkışlar payload/DAG/Logs/CSV/PLC
verisini değiştirmez; experimental kaynak production/PLC/robot izni kazanmaz.

## 64 yol / 185 değişiklik bloğunun final eşlemesi

Her satır o dosyanın bütün hunk aralıklarını kapsar; kesin aralık/blob/scope özgün
JSON'da, final SHA-256 ve kart kapanışları `build/int08/transfer-final-map.json` içindedir.
INT-03–07 kapanışları davranış uyarlamasını açıklar. INT-09'a ayrılmış belge/başlatıcı
son düzenlemesi bu kartta başlatılmaz; mevcut karşılıkları aşağıda gösterilir.
Üç eski kabul/fix-plan/handover belgesi snapshot `230cc72` içinde doğrulandı ve
eski tabana ait tarihsel kanıt olarak kalır. Kaybolmuş ürün parçası diye sayılmaz.

| Snapshot yolu | Hunk | Final karşılık / kapsam |
| --- | ---: | --- |
| `CMakeLists.txt` | 7 | `CMakeLists.txt`; Davranış karşılığı INT-03/05/06/07 |
| `README.md` | 5 | `README.md`; Tarihsel snapshot; yeni kabul değildir |
| `VisionFlow.cmd` | 1 | `VisionFlow.cmd`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/architecture.md` | 1 | `docs/architecture/architecture.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/camera-integration-a0-e2-plan.md` | 1 | `docs/architecture/camera-integration-a0-e2-plan.md`; Tarihsel snapshot; yeni kabul değildir |
| `docs/architecture/output-visibility.md` | 1 | `docs/architecture/output-visibility.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/runtime-calibration-output-acceptance.md` | 1 | `git:230cc72:docs/architecture/runtime-calibration-output-acceptance.md`; Tarihsel snapshot; yeni kabul değildir |
| `docs/architecture/runtime-calibration-output-fix-plan.md` | 1 | `git:230cc72:docs/architecture/runtime-calibration-output-fix-plan.md`; Tarihsel snapshot; yeni kabul değildir |
| `docs/architecture/runtime-calibration-output-handover.md` | 1 | `git:230cc72:docs/architecture/runtime-calibration-output-handover.md`; Tarihsel snapshot; yeni kabul değildir |
| `docs/architecture/runtime-diagnostics.md` | 1 | `docs/architecture/runtime-diagnostics.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/runtime-editor-workflow-plan.md` | 1 | `docs/architecture/runtime-editor-workflow-plan.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/runtime-error-recovery.md` | 1 | `docs/architecture/runtime-error-recovery.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/scoped-active-calibration.md` | 1 | `docs/architecture/scoped-active-calibration.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/scoped-calibration-integration.md` | 1 | `docs/architecture/scoped-calibration-integration.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/architecture/source-verification.md` | 1 | `docs/architecture/source-verification.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/progress.md` | 2 | `docs/progress.md`; Tarihsel snapshot; yeni kabul değildir |
| `docs/requirements/01_machine_vision_yazilim_tanimi.md` | 1 | `docs/requirements/01_machine_vision_yazilim_tanimi.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/requirements/02_machine_vision_ui_gereksinimleri.md` | 1 | `docs/requirements/02_machine_vision_ui_gereksinimleri.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/requirements/03_cpp_qt_opencv_gereksinimleri.md` | 1 | `docs/requirements/03_cpp_qt_opencv_gereksinimleri.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `docs/windows-workstation.md` | 3 | `docs/windows-workstation.md`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `scripts/run-local.ps1` | 1 | `scripts/run-local.ps1`; Mevcut karşılık; belge/başlatıcı son eşleme INT-09 |
| `src/app/EditorUsabilitySmoke.cpp` | 6 | `src/app/EditorUsabilitySmoke.cpp`; Davranış karşılığı INT-07 |
| `src/app/MainWindow.cpp` | 2 | `src/app/MainWindow.cpp`; Davranış karşılığı INT-06/07 |
| `src/app/MainWindow.hpp` | 8 | `src/app/MainWindow.hpp`; Davranış karşılığı INT-06/07 |
| `src/app/MainWindowCalibration.cpp` | 1 | `src/app/MainWindowCalibration.cpp`; Davranış karşılığı INT-06 |
| `src/app/MainWindowEditor.cpp` | 3 | `src/app/MainWindowEditor.cpp`; Davranış karşılığı INT-07 |
| `src/app/MainWindowExecution.cpp` | 1 | `src/app/MainWindowExecution.cpp`; Davranış karşılığı INT-07 |
| `src/app/MainWindowRuntime.cpp` | 26 | `src/app/MainWindowRuntime.cpp`; Davranış karşılığı INT-03/06/07 |
| `src/app/MainWindowWorkspace.cpp` | 2 | `src/app/MainWindowWorkspace.cpp`; Davranış karşılığı INT-06 |
| `src/app/MultiSourceSmoke.cpp` | 3 | `src/app/MultiSourceSmoke.cpp`; Davranış karşılığı INT-07 |
| `src/app/RestartRuntimeSmoke.cpp` | 1 | `src/app/RestartRuntimeSmoke.cpp`; Davranış karşılığı INT-06 |
| `src/app/RuntimeFailureSmoke.cpp` | 1 | `src/app/RuntimeFailureSmoke.cpp`; Davranış karşılığı INT-03/06 |
| `src/app/RuntimeVisibilitySmoke.cpp` | 1 | `src/app/RuntimeVisibilitySmoke.cpp`; Davranış karşılığı INT-07 |
| `src/app/main.cpp` | 3 | `src/app/main.cpp`; Davranış karşılığı INT-06/07 |
| `src/application/ExecutionService.cpp` | 1 | `src/application/ExecutionService.cpp`; Davranış karşılığı INT-06 |
| `src/application/GeometryMeasurementService.cpp` | 1 | `src/application/GeometryMeasurementService.cpp`; Davranış karşılığı INT-06 |
| `src/application/MeasurementWorkspace.hpp` | 14 | `src/application/MeasurementWorkspace.hpp`; Davranış karşılığı INT-04/05 |
| `src/application/MultiSourceRuntimeService.cpp` | 1 | `src/application/MultiSourceRuntimeService.cpp`; Davranış karşılığı INT-04 |
| `src/application/MultiSourceRuntimeService.hpp` | 1 | `src/application/MultiSourceRuntimeService.hpp`; Davranış karşılığı INT-04 |
| `src/application/PlcRuntimeSocket.cpp` | 5 | `src/application/PlcRuntimeSocket.cpp`; Davranış karşılığı INT-03 |
| `src/application/PlcRuntimeSocket.hpp` | 1 | `src/application/PlcRuntimeSocket.hpp`; Davranış karşılığı INT-03 |
| `src/application/ProductionRecipeService.hpp` | 4 | `src/application/ProductionRecipeService.hpp`; Davranış karşılığı INT-03/05 |
| `src/application/RuntimeCoordinator.cpp` | 9 | `src/application/RuntimeCoordinator.cpp`; Davranış karşılığı INT-03/05 |
| `src/application/RuntimeCoordinator.hpp` | 5 | `src/application/RuntimeCoordinator.hpp`; Davranış karşılığı INT-03 |
| `src/application/RuntimeDiagnostics.cpp` | 1 | `src/application/RuntimeDiagnostics.cpp`; Davranış karşılığı INT-03 |
| `src/application/RuntimeDiagnostics.hpp` | 1 | `src/application/RuntimeDiagnostics.hpp`; Davranış karşılığı INT-03 |
| `src/application/RuntimeService.cpp` | 13 | `src/application/RuntimeService.cpp`; Davranış karşılığı INT-03/04/05 |
| `src/application/RuntimeService.hpp` | 8 | `src/application/RuntimeService.hpp`; Davranış karşılığı INT-03/04/05 |
| `src/application/ScopedActiveCalibration.cpp` | 4 | `src/application/ScopedActiveCalibration.cpp`; Davranış karşılığı INT-03/04 |
| `src/application/ScopedActiveCalibration.hpp` | 3 | `src/application/ScopedActiveCalibration.hpp`; Davranış karşılığı INT-03/04 |
| `src/application/SourceVerificationService.hpp` | 1 | `src/application/SourceVerificationService.hpp`; Davranış karşılığı INT-05 |
| `src/presentation/OutputVisibilityPolicy.cpp` | 1 | `src/presentation/OutputVisibilityPolicy.cpp`; Davranış karşılığı INT-07 |
| `src/presentation/OutputVisibilityPolicy.hpp` | 1 | `src/presentation/OutputVisibilityPolicy.hpp`; Davranış karşılığı INT-07 |
| `src/presentation/OutputVisibilityPreferences.cpp` | 1 | `src/presentation/OutputVisibilityPreferences.cpp`; Davranış karşılığı INT-07 |
| `src/presentation/OutputVisibilityPreferences.hpp` | 1 | `src/presentation/OutputVisibilityPreferences.hpp`; Davranış karşılığı INT-07 |
| `tests/geometry_measurement_service_test.cpp` | 1 | `tests/geometry_measurement_service_test.cpp`; Davranış karşılığı INT-06 |
| `tests/multi_source_production_test.cpp` | 2 | `tests/multi_source_production_test.cpp`; Davranış karşılığı INT-04/05 |
| `tests/output_visibility_preferences_test.cpp` | 1 | `tests/output_visibility_preferences_test.cpp`; Davranış karşılığı INT-07 |
| `tests/output_visibility_test.cpp` | 1 | `tests/output_visibility_test.cpp`; Davranış karşılığı INT-07 |
| `tests/runtime_coordinator_test.cpp` | 4 | `tests/runtime_coordinator_test.cpp`; Davranış karşılığı INT-03 |
| `tests/runtime_diagnostics_test.cpp` | 1 | `tests/runtime_diagnostics_test.cpp`; Davranış karşılığı INT-03 |
| `tests/scoped_active_calibration_test.cpp` | 4 | `tests/scoped_active_calibration_test.cpp`; Davranış karşılığı INT-04 |
| `tests/scoped_calibration_runtime_test.cpp` | 1 | `tests/scoped_calibration_runtime_test.cpp`; Davranış karşılığı INT-04/05 |
| `tests/source_verification_test.cpp` | 1 | `tests/source_verification_test.cpp`; Davranış karşılığı INT-05 |
