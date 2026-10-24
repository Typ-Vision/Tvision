# Industrial kamera — birleşik Release kabulü

18 Ekim 2026. Uygulayıcı: Codex.
[Kabul planı](industrial-camera-acceptance-plan.md),
[görev kartları](industrial-camera-tasks-integration.md).

## ICM-09a / G2

**G2 kapalı:** son kaynakla iki zorunlu tam Release koşumu geçti; hata/atlama yok.
Bu karar ortak ürün yazılımı/fixture kapsamındadır; native G0/G1/G3/G4 açık kalır.
Kaynak `adde81d8ad8eed4dec3e61687fea6c82c7d4b8d5`; başlangıç çalışma ağacı temiz.
İlk koşum bu revision'dır. Son kaynakta
`tests/backend_capabilities_test.cpp` ve `src/app/EditorSmoke.cpp` içindeki
eski araç sayısı beklentileri düzeltildi; kullanıcı ekran davranışı değişmedi.
Son iki dosyalık `final-source.diff` SHA256:
`24B5A5749D1AE85159024C78222BA8D1AA5DA2CAC979F64AB6F3CA0A93AA35E3`.
İlk registry düzeltmesinin `test-fix.diff` kanıtı da ayrıca korunur.
Kanıt kökü `build/icm/09a/`; `source.json` kaynak ve ortam kapsamını,
`commands.ps1` exact configure/build/CTest komutlarını taşır.
Qt 6.12.0 / MinGW 13.1.0 / OpenCV 5.0.0 kullanıldı.

| Yapılandırma | Configure/build | Tam CTest |
| --- | --- | --- |
| Release Widgets ON / ZXing ON / OECS SDK OFF | exit 0 / 0 | Son kaynak 105/105; 600.93 sn, exit 0 |
| Release Widgets OFF / ZXing OFF / OECS SDK OFF | exit 0 / 0 | Son kaynak 89/89; 346.11 sn, exit 0 |

Her yapılandırma için `configure-{ui,core}.log`, `build-{ui,core}.log`,
`inventory-{ui,core}.log`, `tests-{ui,core}.log` ve `tests-{ui,core}.xml`
ayrı tutulur. Önceki görevlerin test başarıları bu koşum yerine kullanılmaz.
İlk tekrarlar `build-{ui,core}-final.log`, `tests-{ui,core}-final.log/.xml`;
ikinci UI düzeltmesinden sonraki son kaynak tam koşumları
`tests-{ui,core}-complete.log/.xml` dosyalarındadır. Son build'ler
`build-ui-shell-fix.log` ve `build-core-complete.log`; ilk koşumlar üzerine yazılmaz.

## İlk hatalar ve tekrar politikası

İlk core koşumu 86/89: `multi_source_terminal_trace` (17.12 sn, check161,
`expected server HELLO`), `backend_capabilities` (`capability/legacy registry
mismatch`) ve `plc_socket_runtime` (10.30 sn, `expected server HELLO`) başarısız.
İlk log/XML dosyaları korunur. Socket sunucusunun LISTEN tanısı bulunması veya
birkaç bağlantının geçmesi bütün localhost bağlantılarının çalıştığını kanıtlamaz.
Sandbox dışı tanı tekrarı `socket-diagnostic-core.log/.xml` **1/1** (35.96 sn)
geçti; bu tek test tam kabul yerine kullanılmadı.

İlk UI koşumu **101/105**, 573.08 sn: aynı üç hata ve `ui_shell_smoke`
(31.07 sn, `Communication UI timeout`). Bu smoke'un Communication bölümü gerçek
localhost TCP fixture'ı kullanır. İlk UI log/XML de korunur.

Registry testi industrial kaynak eklenmeden önceki sabit 16 sayısını bekliyordu.
Legacy 16 türün exact kümesi + `industrial_camera`, duplicate ret ve yeni kaynağın
acquisitionSource/editor9/backend bağımsızlığı kontrol edilir hale getirildi.
Capability beklentisi gevşetilmedi; legacy/industrial bütünlüğü ayrı doğrulanıyor.
Son iki tam koşum localhost erişimi için sandbox dışında yapılır. Aralıklı HELLO
hatasının kesin kök nedeni bu tekrar başarısından çıkarılmaz; gerçek PLC kabulü açık.

Registry düzeltmesi sonrası sandbox dışı core **89/89**, 300.78 sn geçti;
UI **104/105**, 508.08 sn oldu. `ui_shell_smoke` bu kez Communication bölümünü
geçip eski `sixteen Toolbox entries including pose tools` beklentisinde durdu.
UI toolbox kontrolü exact 17 sayısını ve Industrial Camera'nın varlığını ayrıca
denetler hale getirildi. İlgili tekrar `tests-ui-shell-fix.log/.xml` **1/1**,
39.42 sn, exit 0. Son kaynakla iki tam koşum yeniden yapıldı; seçki tam kabul değildir.

## Yazılım matrisi ve oracle

Aşağıdaki eşleme [S01–S30](industrial-camera-acceptance-plan.md) için bu
koşumun CTest adlarını belirtir. Bunlar fixture/disk/contract kanıtlarıdır;
gerçek kamera, webcam, PLC, robot veya fiziksel kalibrasyon kabulü değildir.

| ID | Test / bağımsız kontrol |
| --- | --- |
| S01 | camera_transport, camera_probe_diagnostics: eksik/invalid producer, export, 0 cihaz ve hata izolasyonu |
| S02 | industrial_backend_lifetime, session_contract, camera_transport: partial open/cleanup/busy |
| S03 | industrial_discovery_contract, measurement_setup_contract, measurement_environment_persistence: exact kimlik, ambiguity/rebind |
| S04 | tool_migration_contract, project_persistence, industrial_source_tool_contract: legacy/new no-op bayt/schema/hash ve unknown alan ret |
| S05 | measurement_setup_contract, industrial_scoped_calibration: origin/offset/binning/mirror/geometri |
| S06–S07 | industrial_configuration: typed range/unit/readback, transactional rollback ve Fault |
| S08–S09 | industrial_owned_frame, owned_frame_contract, industrial_backend_lifetime: padding/payload/incomplete/overflow ve owned ömür |
| S10–S11 | industrial_software_trigger, industrial_execution_contract, multi_source_execution: eski/geç/duplicate, aynı piksel farklı frame/cycle |
| S12–S14 | acquisition_contract, camera_lifecycle_contract, camera_worker_lease_contract, camera_worker_stress, session_contract: cancel/late, FIFO/drop, lease/shutdown |
| S15 | industrial_calibration_capture, independent_calibration_capture, calibration_wizard_contract: reçetesiz edinim ve gerçek math |
| S16 | industrial_scoped_calibration, scoped_active_calibration, system_calibration_contract: model/setup/device invalidation ve restart izni |
| S17 | industrial_execution_contract, multi_source_execution, multi_source_target_only, editor_target_only: mixed DAG/source/model/ROI ve capture0 |
| S18 | multi_source_graph_contract, multi_source_production_contract: aynı fiziksel kamera, bütçe ve partial error |
| S19–S20 | industrial_runtime_admission_contract, production_recipe_contract, runtime_coordinator, multi_source_production_contract: exact actual ve frozen admission |
| S21–S22 | multi_source_terminal_trace, typed_terminal_trace, runtime_persistence: forged evidence ret, private completion, record retry capture0 |
| S23 | runtime_persistence, typed_terminal_trace, ui_multi_source_runtime: registry/SDK bağımsız tarihçe/Logs/CSV |
| S24 | plc_socket_runtime, multi_source_terminal_trace: Stop/error boş scalar ve gerçek seçili scalar/ACK; localhost fixture |
| S25 | measurement_setup_migration, measurement_environment_persistence, project_persistence: Save As/transfer/import profil korur, route/readback/izin taşımaz |
| S26 | ui_camera_manager, ui_industrial_calibration_activation, ui_calibration_overview, ui_multi_source_editor/runtime: 0/1/8/10, küçük ekran, busy/error ve üç applicationType |
| S27 | SDK OFF iki derleme; industrial_backend_lifetime, camera_transport, industrial_provider_contract, webcam_contract, tool_execution, ui_shell_smoke/editor_services |
| S28 | Yukarıdaki regexsiz tam UI/core CTest envanterleri |
| S29–S30 | industrial_provider_contract, industrial_discovery_contract, industrial_runtime_admission_contract, ui_camera_manager: test factory izolasyonu, native unavailable, diğer kaynakların bağımsızlığı |

SDK'sız kapsam: iki cache'de `VISIONFLOW_OECS_INCLUDE_DIR` ve
`VISIONFLOW_OECS_RUNTIME_DLL` boş; vendor include/import linki yok.
`ui-imports.txt` uygulamanın PE DLL import listesidir. Kamera transport testi
boş GenTL yolu ile 0 producer sonucunu, lifetime testi SDK'sız binding ret yolunu,
normal factory testleri sentetik native capability reddini sınar.
Dosya edinimi gerçek disk rasteriyle, webcam ise capture fixture'ıyla sınanır.
Bu iş istasyonundaki vendor yazılımı kaldırılmadı; ayrı temiz makine/redistributable
ve native DPI/çoklu monitör/dağıtım kabulü yapılmadı. Optional SDK binding kontrolü
ayrıca aşağıda kaydedilir; gerçek cihaz açma testi bu kabul koşumuna dahil değildir.

Optional SDK: ayrı `build/icm/09a/sdk` Release core/ZXing OFF cache'inde yerel
OECS header'ı ile yalnız `oecs_test_sdk`, `oecs_binding_test`, `oecs_session_test`
hedefleri derlendi. Runtime DLL cache girdisi boş; fiziksel `oecs_backend_local`
envanterde yok. `configure-sdk.log` ve `build-sdk-final.log` exit 0;
`tests-sdk-final.log/.xml` binding/lifetime **2/2**, exit 0.
Bu seçki üçüncü bir tam suite olarak sunulmaz. İlk kısmi build komutu fixture DLL
hedefini atladığından `tests-sdk.log/.xml` 1/2 ve `load fixture inspection exports`
hatası verdi; hedef eklenerek düzeltildi. İlk build/test kanıtı korunur.

## Görsel kontrol

Son tam UI koşumunun görsellerinden yedisi açılıp okundu; kopyalar
`build/icm/09a/visuals/` altında: `camera-small-error.png` (620×600),
`calibration-small.png` (850×600), `editor-{normal,small}.png` ve
`runtime-{normal,small}.png` (1400×900 / 850×600), `logs-normal.png` (850×774).
Camera Manager SDK eksikliğini gösterir ve edinimi kapatır; kalibrasyonda
kamera/setup/profile ve bağımsız edinim uyarısı okunur, içerik kaydırılır.
Editor'da explicit profil bağlama ve boş viewer, Runtime'da sentetik kimlik,
ölçüm doğrulanmadı/unavailable/kare yok ve boş sonuç alanı korundu.
Küçük Runtime ilk tam kaynak satırını ve kaydırma çubuklarını gösterir.
Logs fixture'ında DONE/SKIPPED, ACKED ve CSV eylemi görünür; bu TCP/file fixture'ı
gerçek PLC veya industrial native çevrim kabulü değildir.
Runtime/kalibrasyon/Camera Manager son görselleri ilk incelenenlerle hash olarak
aynıydı; farklı çıkan iki Editor görseli yeniden açılıp incelendi ve kopyalandı.
İlk tekrar karşılaştırması `visuals-final-comparison.json`, son kaynak karşılaştırması
`visuals-complete-comparison.json`, son hash'ler `visuals-final-hashes.json`.
Offscreen/sentetik görüntüler fiziksel kabul yerine geçmez.
`config/local.ui.ini` başlangıçta yoktu; test ayarları ve fixture'lar build altındadır.

## Fiziksel kabul matrisi

Bu bölüm G2'den bağımsızdır. Yeni fiziksel deney yapılmadı.

| Backend / model | G0 | G1 | G3 / H01–H08 | G4 |
| --- | --- | --- | --- | --- |
| OECS / COE-050-M-USB-050-IR-C / 02A2220000627 | 01b açık; geçmiş exact C API open/grab tanısı tam H01 değil | Native 04d/04e açık/ertelenmiş | Açık; geçmiş tanı H02–H08 ürün kabulü değil | Açık |
| Basler / model-seri-firmware seçilmedi | BAS-01 açık | BAS-02 açık | BAS-03 / 09b açık | Açık |
| Daheng / model-seri-firmware seçilmedi | DAH-01 açık | DAH-02 açık | DAH-03 / 09b açık | Açık |

İkinci fiziksel cihaz, N-camera kapasitesi, ölçüm toleransı/bağımsız referans,
lens, gerçek PLC/robot, CUDA ve kurulum/dağıtım kabulü açık kalır.
G2 başarısı production capability açmaz; ilgili native G1 ve trusted completion
gerekir. G3 yalnız sınanan backend/model/firmware, G4 yalnız sınanan kurulum içindir.

## ICM-09b — başlatıldı, fiziksel kabul açık

09a commit `1cfa72a` sonrasında önkoşul incelemesi yapıldı. G2 hazır;
hiçbir seçili native backend için G0/G1 tamam değil. Normal
`CameraProviderFactory::capabilities` bütün edinim yeteneklerini false döndürüyor;
open/configure test enjeksiyonunu zorunlu tutuyor. Kapı gevşetilmedi.
`build/icm/09b/preflight.json`, `native-gate.txt` ve
`preflight-source-hashes.json` yeni statik inceleme kanıtıdır.
Bu adımda fiziksel test/command/kare **0/0/0**; yeni build/CTest yapılmadı.
09a'nın test sonuçları 09b fiziksel sonucu olarak kullanılmaz.

| Satır | Güncel durum / gereken kanıt |
| --- | --- |
| H01 | Açık: seçili model/seri/firmware, vendor viewer gerçek sahne + release ve exact open. OECS geçmiş open/grab tam H01 değil. |
| H02 | G1 önkoşulunda bekliyor: native 100 software trigger ve bağımsız olay/kare tazeliği. Eski OECS gecikme karşı örneği geçerli. |
| H03 | G1 önkoşulunda bekliyor: gerçek 20 lifecycle ve preview→production geçişi. Fixture stress fiziksel kabul değil. |
| H04 | G1 önkoşulunda bekliyor: kontrollü unplug/replug ve 10 Stop; explicit recovery/activation/prepare. Şimdilik kablo adımı istenmedi. |
| H05 | G1 önkoşulunda bekliyor: viewer contention ve serbest bırakma sonrası exact tekrar open. |
| H06 | G1 önkoşulunda bekliyor: desteklenen actual profil/geometri değişimi ve restart invalidation. |
| H07 | G1 önkoşulunda bekliyor: tek/mixed-source, üç applicationType gerçek ürün çevrimi ve kalıcı Logs/CSV. |
| H08 | G1 önkoşulunda bekliyor: gerçek 30 dk preview/1000 cycle, süre/RSS/pool/native kapanış. |

Devam girdisi: erişilebilir Basler/Daheng exact model/seri/firmware ve SDK ile
ilgili BAS/DAH-01→02; ya da OECS exact firmware için belgeli pending-command
cancel/completion/drain/reset bariyeri ve buna yönelik yeni karşı deney.
Aynı Stop→Start/sessizlik deneyi tekrarlanmaz. Backend G0/G1 sonrası H01–H08
uygulanır; G3/G4 ve ICM-10 açık kalır.
