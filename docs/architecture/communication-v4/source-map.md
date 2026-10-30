# COMM4-01 — Güncel kaynak ve davranış haritası

29 Ekim 2026; kaynak baseline `7f71becee126e7c1575f4dc87eb4491f56e41ba3`,
dal `Variables`. **Envanterdir; COMM4 uygulaması veya fiziksel kabul değildir.**
Kapsam: tek loaded recipe, endpoint ömrü, publication/terminal/ACK zinciri,
kalibrasyon servis sınırı ve gerçek CMake kayıtları. Tarihsel arşiv yeniden
audit edilmedi. Sözleşme hedefi [S0–S5](contract.md), binary taslak [W0–W8](wire.md).

## Doğrulama yöntemi ve kaynak kimliği

`list_projects` → `index_status(project="D:/TVision")`: `D-TVision`, kök
`D:/TVision`, ready; graph generation `2026-10-29T15:25:41Z`. Tier 2 kullanıldı.
Symbol araması, gerçek `trigger` outbound trace'i ve `acknowledge` snippet'i
alındı. Trace bazı generic `begin/active/connect` eşleşmelerini yanlış servislere
bağlıyor; aşağıdaki zincir bu edge'lerden değil **doğrudan kaynak okumadan** gelir.
Graph temizliği tam kapsama kanıt değildir.

Dayanılan dosyaların tamamı `check_index_coverage` ile kontrol edildi.
`build/comm4/COMM4-01/coverage.json` ve `coverage-extra.json` ham sonuçlardır;
`coverage-fallback.txt` bildirilen eksik satırların doğrudan kaynak okumasıdır.
Kısmi kapsamlar:

| Yol (`src/` altında) | Okunan eksik satırlar |
| --- | --- |
| application/RuntimeCoordinator.hpp / .cpp | 16,20,37 / 10 |
| application/RuntimeService.hpp | 24,44,53,55,57,66 |
| application/CommunicationTcpEndpoint.hpp | 67 |
| application/DeliveryStore.hpp | 19 |
| application/CalibrationService.hpp | 18 |
| application/CameraAcquisitionSessions.hpp / .cpp | 42,45,46 / 98,103,124 |
| application/CalibrationWizardSession.hpp | 75,84,87,94,102 |
| application/IndependentCalibrationCapture.hpp | 29,76,80 |
| application/ActiveCalibrationService.hpp | 35,108 |
| application/ScopedActiveCalibration.hpp | 57,74,75,80 |
| app/MainWindow.hpp | 120,134,153 |
| app/CalibrationWizard.hpp | 15,22 |

İlk aramada aday `src/infrastructure/ProjectSerializer.cpp` için freshness missing
geldi. Dosya kaynak ağacında yok; gerçek port `src/persistence/ProjectRepository.*`
bulunup ayrıca coverage ve kaynakla doğrulandı. Bu aday gerçek dosya sayılmadı.
Diğer dayanılan yollarda metadata_match/no_recorded_issue alındı.

Kullanıcıya ait kök README diff'i ve `prototypes/newUI_prototypes/` dosyaları
SHA-256 ile başta/sonda karşılaştırılır; görev bunları değiştirmez veya stage etmez.
Kanıt: `preserved-before.json`, `preserved-after.json`, `README-before.diff`,
`README-after.diff`, `validation.json`. Build altındaki kanıtlar Git teslimi değildir;
kalıcı bulgular bu dosya ve kapanış kaydındadır.

## Gerçek katman ve sahiplik sınırları

Bağlantılar repo içindeki gerçek yollardır. Satırlar baseline için çapadır;
sonraki kart kendi başlangıcında coverage ve güncelliği tekrar doğrular.

| Katman / gerçek dosya | Bugünkü sorumluluk ve yeniden kullanım sınırı |
| --- | --- |
| [RuntimeCoordinator.hpp](../../../src/application/RuntimeCoordinator.hpp), [cpp](../../../src/application/RuntimeCoordinator.cpp) | Tek `optional<Recipe> loaded_`, tek RuntimeService referansı; owner-thread lifecycle, generation, cancellation, kayıt retry ve delivery owner bariyerleri. `replaceable` (24–28) pending/running/preparing varken değişimi kapatır. Station koleksiyonu değildir. |
| [RuntimeService.hpp](../../../src/application/RuntimeService.hpp), [cpp](../../../src/application/RuntimeService.cpp) | `active()` (94) production/multi/experimental'dan tek snapshot seçer. Begin/finish (244–267) mevcut production kapısına gider; execute (285–319) immutable lease'i worker'da ExecutionService'e verir. Çok kaynaklı reçete, bağımsız istasyon değildir. |
| [CommunicationTcpEndpoint.hpp](../../../src/application/CommunicationTcpEndpoint.hpp), [cpp](../../../src/application/CommunicationTcpEndpoint.cpp) | Application adapter; Qt Network server/socket, framer, watcher, tek peer ve group profile. Constructor ağ açmaz; `listen` (48–57) explicit. Fleet (20–46) aynı Runtime/Coordinator/ledger ve thread ister. Her grup ayrı server; ilk target trigger kaynağı. |
| [TriggerRouter.cpp](../../../src/application/TriggerRouter.cpp) | InputSnapshotStore + admission. Endpoint/peer/input revision, monotonic freshness, shape/type/unit/frame, recipe/config, running/mode/busy/delivery kapılarını denetler; `beforeBegin` kalıcı rezervden sonra coordinator begin olur. Station activation/Auto yok. |
| [PublicationConfig.hpp](../../../src/domain/PublicationConfig.hpp) | Domain OutputGroup/PublishConfig; producer bundle UUID+revision, field eşlemesi, profil+contract revision. En çok 16 grup, 64 field, 65.536 byte publication. COMM4 sabit slot template'i değildir. |
| [PublicationTemplate.hpp](../../../src/domain/PublicationTemplate.hpp) | COMM4-11: recipe dışı sabit16/32/4/1 revision DTO, SlotAssignment/authoritative source metadata, strict JSON/type/unit/whole Pose kapıları. [Sınırlar](publication-template.md). |
| [FixedSlotPublication.hpp](../../../src/application/FixedSlotPublication.hpp), [cpp](../../../src/application/FixedSlotPublication.cpp) | COMM4-12: kopyalanmış immutable capture, aynı cycle'da committed variable/direct output, exact provenance/DAG, bounded exact rational conversion ve seal öncesi slot validity. [Composition sınırı](publication-evaluation.md); socket/ledger/main entegrasyonu değildir. |
| [PublicationService.hpp](../../../src/application/PublicationService.hpp), [cpp](../../../src/application/PublicationService.cpp) | Capture (15–21) profil kopyası/digest; dependency/requireCurrent (22–29) güncellik; evaluate (30–47) yalnız committed aynı cycle/config producer bundle'ından intent üretir. Soket açmaz. Doğrudan tool portu publication kaynağı şu modelde yok. |
| [CommunicationProfiles.hpp](../../../src/application/CommunicationProfiles.hpp), [cpp](../../../src/application/CommunicationProfiles.cpp) | Machine-local port ve QSaveFile JSON store burada; ayrı infrastructure serializer varsayma. TCP client/server profil verisi; load≤1 MiB/256 profil; save açık komuttur, değişen profil revision artırır. V4 registry/manifest değildir. |
| [CommunicationWireCodec.hpp](../../../src/application/CommunicationWireCodec.hpp), [cpp](../../../src/application/CommunicationWireCodec.cpp) | Application içinde network içermeyen v3 JSON+LF shape codec/framer. maxLineBytes=65.536, partialDeadline=2.000 ms. resultPayload session/hash hariç canonical JSON; v4 binary codec'i değildir. |
| [ProductionResultCodec.cpp](../../../src/application/ProductionResultCodec.cpp) | Kalıcı terminal/bundle validasyonu ve typed çıkışlar; terminal identity/decision/execution ayrı. V4 body/terminal digest sözleşmesi bununla aynı değildir. |
| [ProductionRecipeService.cpp](../../../src/application/ProductionRecipeService.cpp), [MultiSourceRuntimeService.cpp](../../../src/application/MultiSourceRuntimeService.cpp) | Completion lease/ticket/hash doğrulaması, terminal freeze ve persist callback. MultiSource finish (80–100) aynı reçetenin dallarını bir terminale bağlar. İkinci station motoru olarak kopyalanmamalı. |
| [ProductionStore.cpp](../../../src/application/ProductionStore.cpp), [DeliveryStore.hpp](../../../src/application/DeliveryStore.hpp), [cpp](../../../src/application/DeliveryStore.cpp) | Append-only QSaveFile arşiv + hash zincirli delivery journal, namespace retirement, exact payload/ACK, recovery. Execution/network/Run yetkisi vermez. Gerçek disk bütçesi aşağıdadır. |
| [StationDeliveryLedger.hpp](../../../src/application/StationDeliveryLedger.hpp), [cpp](../../../src/application/StationDeliveryLedger.cpp) | COMM4-13: ortak archive bütçesinde station-keyed immutable journal; admission/control adapter'ları, sealed terminal/recovery ve exact ACK. [Composition sınırı](station-delivery-ledger.md); normal main entegrasyonu sonraki kartlarda. |
| [StationDeliveryScheduler.hpp](../../../src/application/StationDeliveryScheduler.hpp), [cpp](../../../src/application/StationDeliveryScheduler.cpp) | COMM4-14: station FIFO/round-robin, bounded shared socket, partial offset ve ledger replay/2s/3retry alarmı. PeerConnection gerçek transportu kullanır; `setDeliveryLedger` ACK/RECOVER ve otomatik replay'i bağlar. [Ömür/diagnostics sınırı](station-delivery-scheduler.md). |
| [CommunicationMigration.hpp](../../../src/application/CommunicationMigration.hpp), [cpp](../../../src/application/CommunicationMigration.cpp) | COMM4-15: gerçek legacy serializer/journal analizi, exact backup+manifest, immutable staging ve CAS atomik activation/rollback pointer. Archive writer lease ve stopped-listener preflight; [rollback/composition sınırı](migration.md). |
| [ProjectRepository.hpp](../../../src/persistence/ProjectRepository.hpp), [cpp](../../../src/persistence/ProjectRepository.cpp) | Persistence portu: proje encode/load/save; recipe definition ve kimlikleri. save (97–106) QSaveFile/direct fallback kapalı,≤16 MiB. V4 transaction migration yeni iştir. |
| [SpatialTransform.hpp](../../../src/domain/SpatialTransform.hpp) | Domain FrameRef + revision, mm Float64 translation, quaternion wxyz; right-handed column-vector T_target_source, exact frame composition. Frame revision bugün int; v4 immutable UUID/numeric catalogue dönüşümü ayrıca tasarlanmalı. Robot controller adapter'ı değildir. |
| [MainWindow.hpp](../../../src/app/MainWindow.hpp), [MainWindowRuntime.cpp](../../../src/app/MainWindowRuntime.cpp) | UI composition: tek runtime_/runtimeCoordinator_; loadRuntime (109–121) oluşturur. Run dalı startRuntimeTcp (264) çağırır. Yerel production izinleri restore edilmez. |
| [MainWindowCommunication.cpp](../../../src/app/MainWindowCommunication.cpp) | `withPublication`, reset/startRuntimeTcp, v2 initialize/listen, v3 profil ve diagnostics tabları. startRuntimeTcp (32–58) ortak Runtime/ledger/input/router ile grup başına endpoint kurar. UI socket parser/algoritma yürütmez. |
| [PublishDataEditor.cpp](../../../src/app/PublishDataEditor.cpp) | UI producer seçicisi yalnız set_variables bundle'ları; draft group/field/profile edit, validateDraft + capture, preview metadata. Preview network intent üretmez. V4 direct output/slot UX henüz yok. |
| [PlcRuntimeSocket.cpp](../../../src/application/PlcRuntimeSocket.cpp) | Ayrı legacy v2 application adapter; bağımsız listen/bind, session dedup, scalar measurement, intent/ACK events; v3 DeliveryStore exact result/hash akışıyla eş tutulmamalı. |
| [plc_simulator.cpp](../../../src/app/plc_simulator.cpp) | Mevcut `visionflow_plc_simulator` CLI kaynağı; v4 binary/robot handshake driver'ı olarak uygulanmış değildir. |

Build katmanları [CMakeLists.txt](../../../CMakeLists.txt): `visionflow_projects`
domain + ProjectRepository + CommunicationProfiles/PublicationService/codec'i
Qt Core ile toplar; `visionflow_acquisition` Runtime/endpoint/store/calibration,
Qt Gui/Concurrent/Multimedia/Network + projects/matching/hardware/OpenCV bağlar.
`visionflow_hardware` GenTL/OECS sınırıdır. Widgets yalnız `visionflow` ve UI test
hedeflerindedir. İsimleri idealize ederek yeni infrastructure dizini varsayma;
mevcut target ayrımı yeni kartta korunmalı.

## Doğrudan doğrulanan v3 trigger → terminal → ACK

1. Endpoint `receive` (162–173), session exact kontrolünden sonra TRIGGER'ı
   `trigger`'a (174–225) yollar. Secondary target `TRIGGER_SOURCE_DISABLED` döner.
   Duplicate RAM/disk identity önce kontrol edilir; aynı key farklı fingerprint
   conflict, kayıtlı sonuç replay; payload yoksa recovery gerekir.
2. `TriggerRouter::deliveryRequest` (28–33), key workcell/project/endpoint/peer/
   requestId için recipe/config/inputSnapshot/inputRevision/sequence fingerprint'i
   üretir. `admit` (34–66), Runtime mode/permission/input/busy kontrollerinden
   sonra callback'i ve `coordinator.begin`'i çalıştırır. V4 installation+epoch+
   station key'i bugünkü key değildir.
3. Endpoint callback (204–211), tüm publication target preflight'ından sonra
   **her hedef için `ledger.request` → `ledger.begin`** yazar; worker bundan sonra
   girer. Begin sonrası arızada ikinci acquisition yerine recovery bariyeri kalır.
   Secondary requestId immutable cycle UUID'sidir; ilk hedef PLC requestId taşır.
4. QtConcurrent worker (220–224) yalnız immutable ProductionCycle, catalog ve
   cancel yakalar; `RuntimeService::execute` → ExecutionService. Finished callback
   owner thread'e döner; pending result için `retryRecording` (234–248) çağrılır.
5. Coordinator finish → RuntimeService finish → production/multi finish terminali
   persist eder. Runtime constructor (34–37) callback'i ProductionStore.append'e
   bağlar; multi finish aynı arşiv yolunu kullanır. Endpoint committed records'tan
   cycle terminalini bulur ve ProductionResultCodec ile doğrular.
6. Her target `result` (226–233) ile kendi grubunu sealed RESULT'a çevirir;
   `DeliveryStore::commit` (115–122), terminal+payload hash/bundle manifest'ini
   `DELIVERY_PENDING` yazar. Replay (249–264) yalnız disk payload'ını kullanır;
   `attempt` kaydı socket send'den öncedir. Reconnect send yeni session ekler;
   sonuç içeriği tekrar hesaplanmaz.
7. Endpoint `acknowledge` (271–281) request namespace kaydını bulur, **resultId +
   payloadHash exact** ister; `acknowledge` → `commitAcknowledgement` (store124–125)
   `ACK_RECEIVED` → `ACK_COMMITTED` yazar. Ancak sonra retry/outstanding temizlenir
   ve restoreDelivery ilgili endpoint owner bariyerini günceller. Socket write
   ACK değildir; başka endpoint ACK'i diğer owner'ı kaldıramaz.

Bu kart zinciri kaynakla doğruladı; yeni bir network koşumu yapılmadı. Failure
injection/reconnect davranışının yeni test başarısı iddia edilmez.

## Uygulanmış v2/v3 ile COMM4 hedef farkı

| Alan | Mevcut kaynak davranışı | COMM4 hedefi / kart |
| --- | --- | --- |
| Listener ömrü | V2 initializeCommunication enabled+autoStart ile reçetesiz listen edebilir. V3 UI Run sırasında explicit listen; constructor/Load listen etmez. Endpoint stop listener'ı da kapatır. Runtime Stop tek başına endpoint.stop değildir; UI ayrıca stop kontrolü sunar. | Tek startup listener ve Run/Auto ayrımı, 05/16. |
| Protokol | V2 v=2 JSON session TRIGGER/BUSY/DONE/ACK; ACK requestId. V3 v=3 JSON+LF HELLO/STATUS/INPUT/TRIGGER/ACCEPTED/RESULT/ACK/ACKED; result/hash exact ACK. Ayrı adapter, auto-detect yok. | Explicit v4 fixed binary512/1024, 02/06/07. |
| Yürütme | Tek loaded recipe/active snapshot; multi-source aynı çevrim. V3 çok endpoint aynı Runtime, ilk hedef tetik, diğerleri fan-out teslim. | Station activation/control ve bağımsız cycles, 04/08/08a/09. |
| Publish | Set Variables committed bundle alanları; 16 grup/64 field, named typed JSON payload. | Machine template16 Number/32 Flag/4 Text/whole Pose; direct output, 11/12/17. |
| Pending/retry | Required offline/pending ortak coordinator admission'ını engeller; optional endpoint backlog128, tek in-flight, 2s/3 retry options varsayılanı. Timer50ms; send buffer iki max-line frame ile sınırlı. | Station başına pending, fair shared-socket scheduler, 13/14. |
| Kalıcılık | V2 session RAM entries + production event geçmişi; v3 durable namespace/fingerprint/journal/tombstone, explicit resume, acquisition tekrarı yok. | [Station+peer epoch ledger uygulandı](station-delivery-ledger.md); transaction migration15. |
| UI | V2, TCP v3 profiles ve Trigger/Delivery ayrı sekmeler; Publish Editor ayrı reçete yüzeyi. | Tek Communication station/calibration workspace,16/24. |

**Gerçek disk sınırı:** ProductionStore.cpp14,28–45,62–81: dosya≤20 MiB,
tüm alt gruplar toplamı≤512 MiB, toplam≤10.000 JSON kayıt; otomatik retention
yok. Gruplar snapshots/events/cycles/experimental_cycles/delivery/
delivery_namespaces. `requireCapacity` record+byte headroom denetler;
DeliveryStore admission (92–100) `32 + pending*8` kayıt ve
`32 MiB + pending*1 MiB` rezerv ister. Bu bir disk alanı ön kontrolüdür;
çok dosyalı atomik v4 WAL rezervi sayılmaz. Tek payload≤65.536 byte,
attempt<10.000; tombstone'lar bütçeye dahildir ve sessiz eviction yoktur.

## Kalibrasyonun gerçek servis sınırı

| Gerçek yol | Mevcut iş / COMM4 bağlantı sınırı |
| --- | --- |
| [CalibrationService.hpp](../../../src/application/CalibrationService.hpp), [cpp](../../../src/application/CalibrationService.cpp) | Append-only ID/version planar catalog; installation provider ile current/resolve/bind. Matematik kaydı üretim aktivasyon izni değildir. |
| [CalibrationWorkflow.cpp](../../../src/application/CalibrationWorkflow.cpp) | uniform_planar_scale/raw_planar_homography/manual_planar_scale; tek Image File/Webcam frame provenance + lens/working-plane kurulumu. Robot 6D solve portu sayılmaz. |
| [CalibrationCapture.cpp](../../../src/application/CalibrationCapture.cpp) | Legacy recipe Image File/Webcam adapter ile acquire/configuration/setup. Industrial capture'ı bu legacy kola ekleme varsayımı yapılmamalı. |
| [IndependentCalibrationCapture.hpp](../../../src/application/IndependentCalibrationCapture.hpp) | IndependentCalibrationSession; saved camera/setup request, prepareCamera, worker job/run, owner-thread accept/stop; yeni hedef eski taslağı retire eder. Industrial güncel giriş noktası adayı. |
| [CameraAcquisitionSessions.hpp](../../../src/application/CameraAcquisitionSessions.hpp), [cpp](../../../src/application/CameraAcquisitionSessions.cpp) | Context/generation, shared future prepare/acquire/close/shutdown; SDK hardware worker'da, UI await etmez. Experimental context production admission taşımaz. |
| [CameraLeaseArbiter.hpp](../../../src/application/CameraLeaseArbiter.hpp), [cpp](../../../src/application/CameraLeaseArbiter.cpp) | COMM4-10 ortak UUID/fiziksel anahtar rezervasyonu; atomik küme, owner/use/generation diagnostics ve quarantine. [Lease sözleşmesi](camera-leases.md). |
| [CameraWorkerSession.hpp](../../../src/application/CameraWorkerSession.hpp), [cpp](../../../src/application/CameraWorkerSession.cpp) | COMM4-10 CameraLeaseArbiter Ticket claim; UUID + physical alias exclusive, gerçek join sonrası release, cleanup hatası quarantine. |
| [CalibrationWizardSession.hpp](../../../src/application/CalibrationWizardSession.hpp), [ActiveCalibrationService.hpp](../../../src/application/ActiveCalibrationService.hpp), [ScopedActiveCalibration.hpp](../../../src/application/ScopedActiveCalibration.hpp) | Immutable scan/calculation jobs, planar model catalog ve scoped saveAndUse/availability API'leri. Robot holdout report/candidate/activation sözleşmesi bu API'lerin varlığından çıkarılamaz; 18'de içerik audit'i genişler. |
| [CalibrationStudyStore.cpp](../../../src/application/CalibrationStudyStore.cpp) | Legacy chessboard-only test study, decoded image SHA/provenance,≤4 MiB QSaveFile + lock. Durable robot sample ledger değildir. |
| [CalibrationWizard.hpp](../../../src/app/CalibrationWizard.hpp), [CalibrationOverview.cpp](../../../src/app/CalibrationOverview.cpp) | UI application session/workspace/save-use portlarını tüketir. Robot workspace adayı; mevcut çalışma plaka/mm akışı 6D hand-eye değildir. |

Native OECS cancel/drain/frame correlation, bağımsız fiziksel mm, lens ve robot
actual6DOF/controller/TCP/frame/plate/reference/tolerance engelleri korunur.
COMM4-18 araştırması,22–26 fixture/fiziksel kapı ayrımı halen gereklidir.

## Gerçek build ve CTest haritası

Otorite CMakeLists.txt'teki `add_executable/add_test` kayıtlarıdır; aşağıdakiler
uygulanmış **test adlarıdır**, bu kartta koşulmuş sonuçlar değildir.
Tam executable/source/arguments tablosu `build/comm4/COMM4-01/cmake-tests.json`.
Yeni COMM4 test/hedef adları ilgili kartın uygulamasında eklenir.

| Mevcut hedef / test kaynağı | CTest adı / argüman |
| --- | --- |
| runtime_coordinator_test / tests/runtime_coordinator_test.cpp | runtime_coordinator |
| runtime_test / tests/runtime_test.cpp | runtime_persistence |
| communication_wire_test / tests/communication_wire_test.cpp | communication_wire_contract |
| communication_v4_codec_test / tests/communication_v4_codec_test.cpp | communication_v4_codec (COMM4-06) |
| communication_values_test / tests/communication_values_test.cpp | communication_values_contract |
| spatial_values_test / tests/spatial_values_test.cpp | spatial_values_contract |
| publication_test / tests/publication_test.cpp | publication_contract |
| assignment_execution_test / tests/assignment_execution_test.cpp | assignment_execution_contract |
| trigger_router_test / tests/trigger_router_test.cpp | trigger_router_contract |
| communication_tcp_test / tests/communication_tcp_test.cpp | communication_tcp_runtime; tcp_measurement_acceptance (--measurement --measurement-type), tcp_measurement_inspection_acceptance (--measurement), tcp_measurement_robot_acceptance (--measurement --robot), tcp_control_measurement_acceptance (--measurement-type), tcp_control_robot_acceptance (--robot); delivery_retry_runtime (--retry-capacity), delivery_optional_runtime (--optional-backlog) |
| multi_endpoint_delivery_test / tests/multi_endpoint_delivery_test.cpp | multi_endpoint_delivery_contract |
| plc_socket_test / tests/plc_socket_test.cpp | plc_socket_runtime |
| delivery_store_test / tests/delivery_store_test.cpp | delivery_store_contract |
| station_delivery_ledger_test / tests/station_delivery_ledger_test.cpp | station_delivery_ledger_contract; COMM4-13 |
| station_delivery_scheduler_test / tests/station_delivery_scheduler_test.cpp | station_delivery_scheduler_contract; COMM4-14 |
| communication_migration_test / tests/communication_migration_test.cpp | communication_migration_contract; COMM4-15; 38 child-process staging/swap crash noktası |
| delivery_recovery_test / tests/delivery_recovery_test.cpp | delivery_recovery_contract |
| delivery_retry_test / tests/delivery_retry_test.cpp | delivery_retry_contract |
| production_recipe_test / tests/production_recipe_test.cpp | production_recipe_contract |
| multi_source_production_test / tests/multi_source_production_test.cpp | multi_source_production_contract |
| camera_acquisition_sessions_test / tests/camera_acquisition_sessions_test.cpp | camera_acquisition_sessions_contract (report path) |
| camera_lease_arbiter_test / tests/camera_lease_arbiter_test.cpp | camera_lease_arbiter_contract (report path); COMM4-10 |
| camera_worker_test / tests/camera_worker_test.cpp | camera_worker_lease_contract; camera_worker_stress (--stress) |
| runtime_camera_sessions_test / tests/runtime_camera_sessions_test.cpp | runtime_camera_sessions_contract (report path) |
| calibration_camera_sessions_test / tests/calibration_camera_sessions_test.cpp | calibration_camera_sessions_contract |
| independent_calibration_capture_test / tests/independent_calibration_capture_test.cpp | independent_calibration_capture |
| industrial_calibration_capture_test / tests/industrial_calibration_capture_test.cpp | industrial_calibration_capture |
| calibration_wizard_test / tests/calibration_wizard_test.cpp | calibration_wizard_contract |
| calibration_save_use_test / tests/calibration_save_use_test.cpp | calibration_save_use |
| active_calibration_test / tests/active_calibration_test.cpp | active_calibration_contract |
| scoped_active_calibration_test / tests/scoped_active_calibration_test.cpp | scoped_active_calibration |
| calibration_test / tests/calibration_test.cpp | calibration_persistence |
| endpoint_profile_ui_test / tests/endpoint_profile_ui_test.cpp | ui_endpoint_profiles (UI ON) |
| delivery_diagnostics_ui_test / tests/delivery_diagnostics_ui_test.cpp | ui_delivery_diagnostics (UI ON) |
| visionflow / src/app UI composition | ui_endpoint_screen (--endpoints-smoke), ui_publication_editor (--publication-smoke), ui_tcp_acceptance (--tcp-acceptance-smoke), ui_multi_endpoint_acceptance (--tcp-acceptance-smoke --multi-endpoint-smoke), ui_runtime_camera_sessions (--runtime-camera-sessions-smoke); UI ON |

`ui_publication_editor` kaydı `visionflow` executable'ını kullanır; ayrı publication
UI test hedefi varsayılmamalı. UI kalibrasyon hedefleri `calibration_ui_test` →
`ui_calibration_wizard`, `ui_independent_calibration_wizard`;
`calibration_overview_test` → `ui_calibration_overview`;
`calibration_activation_ui_test` → `ui_scoped_calibration_activation`;
`industrial_activation_ui_test` → `ui_industrial_calibration_activation`.
Core hedefler Widgets istemeden ON/OFF'ta kalır. Bu D profilli kartta build/CTest
koşulmadı, geçmiş test başarıları devralınmadı; yalnız belge/link/identity kontrolü.

## Kart başına sahiplik tablosu: gerçek port ve yeni aday

Bu tablo seçili kartların sahiplik bölümlerini somutlaştırır; gelecekteki dosya
adları **adaydır**, bugün var veya tamamlanmış sayılmaz. Tüm yollar yukarıdaki
gerçek haritaya göredir; yeni source/test/CMake sahipliği kart başlangıcında
daraltılır. Bir sonraki kart otomatik başlatılmaz.

| Kart | Gerçek başlangıç portu | Yeni aday / bu kartın sınırı |
| --- | --- | --- |
| 01 | Bu source-map, plan/kart01, progress/archive | Yalnız envanter; C++ yok |
| 02 | wire.md, verification.md | tests/fixtures/communication-v4/ golden byte/hex/PLC word tabloları; üretim codec yok |
| 03 | domain identity, persistence/ProjectRepository, application/CommunicationProfiles; CMake projects | StationIdentityRegistry ve store/tests; legacy otomatik rewrite yok |
| 04 tamam | RuntimeCoordinator/RuntimeService snapshot sınırı | StationDefinition/StationSnapshot, StationActivationService, StationRuntimeBoundary, FileStationActivationStore; station_activation_contract. [Port sözleşmesi](station-activation.md) |
| 05 | main startup composition, legacy listen guards, StationRuntimeBoundary | CommunicationServerLifecycle/profile, CommunicationListenerOwnership, StationRunController ve communication_server_lifecycle_test; [uygulanan sınır](server-lifecycle.md), UI sayfaları değişmedi |
| 06 | application/CommunicationV4Codec.hpp/cpp, tests/communication_v4_codec_test.cpp, CMake visionflow_projects | [Binary codec/framer/write queue](binary-codec.md); CTest communication_v4_codec; domain authority codec'e geçmez |
| 07 | CommunicationServerLifecycle, main process composition; v3 endpoint ayrı | CommunicationPeerSessionService + bounded socket adapter ve communication_peer_session_test; [uygulanan sınır](peer-session.md), station/ledger dispatch yok |
| 08 tamam | TriggerRouter.admit bariyerleri ve mevcut Coordinator.begin | StationAdmissionService + StationRuntimeAdmissionBoundary, session TRIGGER hook; station_admission_contract. [Port/sınırlar](station-admission.md); yürütme topolojisi09 ve gerçek durable adapter13'te |
| 08a | Coordinator start/stop/generation ve MainWindowRuntime local controls | StationControlService; UI/remote aynı komut portu |
| 09 tamam | RuntimeService.execute, RuntimeCoordinator.finish | StationExecutionRegistry.hpp/.cpp; station_execution_contract; [bounded worker ve isolation portu](station-execution.md); algoritma motoru kopyalanmaz |
| 10 | CameraAcquisitionSessions/CameraWorkerSession ortak lease | [CameraLeaseArbiter](camera-leases.md); UUID+physical all-or-none, provider Ticket, drain/diagnostics — tamam |
| 11 | PublicationConfig/SpatialTransform baseline; yeni PublicationTemplate.hpp/.cpp | Tamam: PublicationTemplate/SlotAssignment domain, publication_template_test / publication_template_contract; UI yok |
| 12 | PublicationService capture/dependency/requireCurrent/evaluate | Fixed-slot evaluator; aynı immutable provenance |
| 13 | ProductionResultCodec/ProductionStore/DeliveryStore journal | StationDeliveryLedger + admission/control adapter'ları ve station_delivery_ledger_contract; [uygulanan sınır](station-delivery-ledger.md) |
| 14 | StationDeliveryScheduler + PeerConnection | Uygulandı: shared-socket fair station scheduler; ledger otoritesi; main composition sonraki kartlarda |
| 15 | persistence/ProjectRepository, CommunicationProfiles, DeliveryStore, StationDeliveryLedger, ProductionStore | CommunicationMigrationPlanner/Transaction uygulandı; backup/manifest, unsupported/pending gate, CAS selection ve ledger'i koruyan rollback; communication_migration_contract |
| 16 | MainWindowCommunication/MainWindowRuntime, PublishDataEditor sınırı | Station table/view model + tek Communication; legacy data korunur |
| 17 | PublishDataEditor typed bundle seçicisi | Slot assignment/direct-output model; machine template vs recipe yetkisi |
| 18 | Yukarıdaki planar calibration portları, contract S6–S8 | calibration-readiness.md; optik/robot6D araştırması, daha derin audit |
| 19 | domain/SpatialTransform FrameRef ve communication value DTO portu | RobotActualPoseAdapter; controller convention fixture, robot client varsayılmaz |
| 20 | IndependentCalibrationSession, CalibrationWizardSession, ScopedActiveCalibration | RobotCalibrationSessionService;10 arbiter'ını tüketir |
| 21 | Independent capture, CameraAcquisitionSessions, legacy StudyStore sınırı | CalibrationSampleCoordinator;13 durable request/sample ledger |
| 22 |18'de seçilecek gerçek solver portu, Wizard/Active planar ayrımı | HandEyeCandidateSolver; iki geometri, uygunluk blokluysa fixture |
| 23 | ScopedActiveCalibration save/use portu;18 sonrası gerçek repository seçimi | HandEyeValidationReport/ActivationService; planar aktivasyonu robot izni yapmaz |
| 24 | CalibrationWizard/Overview ve16 Communication composition | Robot calibration view model; UI solve/SDK çağırmaz |
| 25 | src/app/plc_simulator.cpp, mevcut TCP test executable kayıtları | v4 simulator mode, robot handshake fixture driver; üretim producer yok |
| 26 | verification matrisi, kullanım README ilgili davranışları | Entegre kabul raporu ve yalnız dar gerçek açık düzeltmesi |

## Geçiş riskleri ve durma kapıları

- Tek Runtime'ı endpoint sayısı kadar bağlamak bağımsızlık sağlamaz. 04/08/09'un
  admission/activation/worker tasarımı,10 camera ownership ve13 ledger birlikte
  korunmalı;09'un multi-source kodunu kopyalaması çözüm değildir.
- V3 constructor trace generic eşleşmeler taşır; yeni kartlar direct source ve
  coverage olmadan çağrı/etki iddiası yapmamalı. Buradaki map baseline'dır.
- V3 canonical JSON hash ve namespace anahtarı v4 binary terminal digest/epoch/
  station anahtarıyla değiştirilemez.13/15 eski pending/tombstone'u korumalı.
- Listener, profile, recipe activation ve production permission ayrı ömürlerdir.
  V2 autoStart davranışını v3 uygulanmış startup listener diye sunma;05'te tek
  v4 lifecycle oluştur,16'da görünür durumu bağla.
- Mevcut field/text/payload kapasitesi v4 slot/text64 sınırlarına kayıpsız otomatik
  sığmaz.02 freeze öncesi gerçek byte fixture ve robot quaternion/Euler ön kontrolü
  zorunludur;15 unsupported mapping'i raporlamalı.
- Process-local physical lease v4 station arbiter sözleşmesini tek başına
  sağlamaz. Timeout/Stop gerçek drain'i bekler; quarantine/native engelleri korunur.
- Robot pose DTO varlığı hand-eye solver veya fiziksel gözlenebilirlik kanıtı
  değildir.18'in gerçek girdileri ve22–26 fiziksel kapısı atlanamaz.

Sonraki kart **COMM4-02**; bu envanter wire freeze veya uygulama başlangıcı değildir.
