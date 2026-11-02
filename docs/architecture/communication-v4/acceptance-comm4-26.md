# COMM4-26 — Entegre yazılım kabulü ve fiziksel devir

2 Kasım 2026. Uygulayıcı: Codex. Gerçek kamera bağlı olmadan Image File,
bağımsız TCP peer ve açık sentetik fixture girdileriyle çalışıldı.
Yazılım kapanışı fiziksel kamera/robot üretim izni oluşturmaz.

## Son kaynak ve kabul

Kaynak tabanı `d9e407ef2f10ed01aefd77ab01151777a6e76498` (COMM4-17a dahil).
Bu kart ürün/test davranışını değiştirmez; kabul ve kullanım/devir belgelerini
birleştirir. Kanıt kökü `build/comm4/COMM4-26/`.
`source-test-fixture-hashes.json` 845 kaynak/test/fixture/build-girdisi dosyasını,
`binary-hashes-on.json` ve `binary-hashes-off.json` çalıştırılan executable'ları,
`CMakeCache-on.txt` ve `CMakeCache-off.txt` gerçek seçenekleri kaydeder.
`final-source-match.json` kapanış belgeleri sonrası aynı kod/girdi hash'lerini denetler.

| Yeni koşum | Geçti | Başarısız | Skipped |
| --- | ---: | ---: | ---: |
| Release ON / ZXing ON hedefli | 34 | 0 | 0 |
| Release OFF / ZXing OFF hedefli | 29 | 0 | 0 |
| Release ON / ZXing ON tam CTest | 197 | 0 | 1 |
| Release OFF / ZXing OFF tam CTest | 156 | 0 | 1 |

Incremental iki Release build exit0. Hedefli kabulden sonra tam suite `-R`
olmadan çalıştırıldı. Komut/başlangıç/bitiş/exit bilgileri faz JSON'larında,
tam log/JUnit/LastTest dosyaları aynı köktedir. Önceki arşiv sonuçları bu
tabloda yeni koşum sayılmadı. `framed_calibration_plate_external` dış plaka
girdisi verilmediğinden skipped; kamera takılı olmaması testleri gizlemek için
kullanılmadı. COMM4-25'teki iki legacy UI hatası COMM4-17a düzeltmesiyle son
kaynakta yeniden sınandı; sonuç JUnit kayıtlarında görülebilir.

Her bağımsız hedefli/tam koşum öncesi build altındaki test INI'leri hashli
yedeklendi; fixture bağımlılıkları koşum sırasında korundu. TEMP/TMP her faza
ayrı verildi. Kullanıcı `config/local.ui.ini` hash'i değişmedi. Frozen wire
verifier yeni koşumda 1533 kontrol/82 frame/16 request kind/9 response kind/
13 metadata category geçti, skipped0; bu ayrı tasarım oracle'ıdır.
Kullanım/token sayaçlarına erişilemiyor; ücret/kota hesabı yapılmadı.

## Capability matrisi

`implemented` kod/port varlığını, `software-verified` aşağıdaki yeni koşumu,
`physical-blocked` gerçek kurulum kanıtının eksikliğini belirtir. `unsupported`
normal ürün yolunun bu işlevi sağlamadığını belirtir; fixture başarısı bunu açmaz.

| Capability | implemented | software-verified | physical-blocked | unsupported / kapsam sınırı |
| --- | --- | --- | --- | --- |
| Tek V4 listener, bağımsız station/Run/Auto/Stop | Evet | lifecycle/control/composition, dört station E2E | Gerçek PLC/hat | V2/V3 auto-detect ve paralel legacy listener yok |
| Aynı peer'da en az üç station, bağımsız worker/kamera | Evet | TCP overlap bariyeri, ortak lease collision, dördüncü station | Native kamera edinimi | Normal V4 production composition Image File; native/webcam provider yok |
| A→B→A aktivasyon, stale snapshot fence | Evet | E2E, activation/control | Gerçek hat reçete geçişi | Recipe seçimi Run/Auto açmaz |
| Number16/Flag32/Text4/whole Pose typed publication | Evet | evaluator/assignment/template/codec + Editor UI | Gerçek PLC ve fiziksel 6D kaynak | Point2D→6D yükseltme yok; normal main whole Pose üretmez |
| Kalıcı terminal, exact ACK/reconnect/recovery | Evet | ledger/scheduler/E2E consumer crash | Gerçek PLC ACK entegrasyonu | Exactly-once dış etki garantisi yok |
| Read-only migration, backup/manifest, explicit apply/rollback | Evet | migration ve workspace/composition | Kurulu eski binary/gerçek pending ACK ile saha devri | Fan-out/uzun text/uyumsuz mapping explicit unsupported; pending drain gerekir |
| Actual pose/frame/TCP dönüşüm portu | Evet | actual-pose adapter golden/negatif testleri | Controller actual6D/TCP/frame doğrulaması | Otomatik controller Euler convention yok |
| BEGIN/SAMPLE/ABORT/recovery ve pose–yeni frame eşleştirme | Evet | İki-mode TCP fixture + session/sample coordinator | Native acquisition/correlation/drain | Normal main calibration callback bağlı değil |
| İki mount PARK candidate solve | Fixture portu | İki mod ground truth/degeneracy/negatif testleri | Lens/intrinsics/plate/robot | Normal wire/main SOLVE açılmaz; telecentrik/fisheye ilk yolda unsupported |
| Bağımsız holdout validate ve atomik activate | Fixture portu | Golden reference, stale CAS, crash old/new | Fiziksel bağımsız reference/tolerans | Synthetic ACTIVE üretim izni değildir; normal wire/main validate/activate kapalı |
| Tek Communication, publish ve calibration UX | Evet | ON UI CTest + Windows Qt iki viewport | OS input/WGC/çoklu monitör saha kabulü açık | Bağlantı rengi üretim Ready veya parça OK değildir |
| Kurulum/dağıtım/CUDA | Bu kartın kapsamı değil | Bu kartta doğrulanmadı | Ayrı kurulum ve performans kanıtı gerekir | Hız/dağıtım başarısı iddia edilmez |

## Çapraz kanıt haritası

| Kabul davranışı | Yeni kanıt ve gerçek CTest |
| --- | --- |
| Tek bağlantı, dört station/üç applicationType, SELECT_RECIPE→SET_AUTO→READY→TRIGGER | `e2e-on/result.json`, `e2e-off/result.json`; `communication_v4_simulator_e2e` |
| Üç acquisition aralığının kesişmesi; S1 shared lease / S2 CAMERA_BUSY / S3 ilerleme | `station_execution_contract-on.log` / `-off.log` tam LastTest çıktısı; worker bariyeri fixture'dır, fiziksel kamera değildir |
| A/B'de socket/session korunması, A→B→A yeni activation, stale ret | E2E `recipe_A_B_A_no_socket_cut_stale_activation`; exact case adları result.json'dadır |
| Number15/Flag31, direct/Variable, false/0/empty geçerli, missing/overflow/UTF8/frame ret | E2E + `fixed_slot_publication_contract`, `publication_assignment_contract`, `publication_template_contract`, `communication_v4_codec`, `ui_publication_editor` |
| Consumer fsync sonrası process exit77, ACK'siz replay, dış effect duplicate | E2E crash/effect/peer JSONL dosyaları; consumer process kill ile ledger exception injection ayrı kanıttır |
| INTENT/BEGIN/TERMINAL/PENDING/ACK_COMMITTED önce/sonra 10 fault boundary | `journal-fault-recovery-on.json` / `-off.json`, tam LastTest `FAULT_RECOVERY` satırları; server power-cut kabulü değildir |
| Yavaş ACK, exact ACK, control reserve, reconnect/backpressure | `station_delivery_scheduler_contract`, E2E ve `station_control_contract` |
| İki-mode SAMPLE, holdout, 100 duplicate başına capture1, stale revision, timeout/drain | E2E robot fixture; `calibration_sample_coordinator_contract`, `robot_calibration_session_contract` |
| Candidate/validation/active ayrımı, bağımsız holdout, atomik activation | `hand_eye_candidate_solver_contract`, `hand_eye_validation_activation_contract`, solver/validation evidence kopyaları |
| Migration old/new crash ve ledger/authority korunması | `communication_migration_contract`, `communication_station_composition_contract`, `ui_communication_workspace` |

Robot TCP fixture SOLVE/VALIDATE/ACTIVATE yürütmez. Solver ve validation/activation
ayrı application-service fixture testleridir; bunlar tek fiziksel uçtan uca
kalibrasyon koşumu olarak birleştirilemez. Gerçek robot motion gönderilmedi.

## Görsel kabul

`native-ui/communication/` ve `native-ui/calibration/` Windows Qt platformunda
DPR1.25 ile 1400×900/850×600 mantıksal viewport görsellerini içerir (raster
1750×1125/1063×750). Workspace 41, iki-mode calibration 102 kontrol geçti.
Normal/küçük görüntüler açılarak üç station satırı, STOPPED/Auto/PENDING ayrımı,
PHYSICAL BLOCKED, missing actual pose, candidate/active ve drain/recovery durumları
kontrol edildi. Küçük ekranda tablo yatay, genel yüzey dikey kaydırılır; bütün
sütunların aynı anda görünmesi iddia edilmez. Scroll/focus testleri alt kontrollere
erişimi doğrular. Publish slot görüntüleri `ui-publication/` altındadır.
Qt event/keyboard ve QWidget render kontrolü OS SendInput/WGC/çoklu monitör kabulü
değildir; bu açık sınır korunur.

## Fiziksel devir — başlatılmadı

**COMM4-PHYS-01 — Native kamera ve PLC saha kabulü:** kullanıcı yönlendirmesiyle.
Önkoşullar: gerçek kamera/lens kimliği ve çalışma düzlemi; belgeli OECS
cancel/completion/drain/reset ve eski frame'i fence eden korelasyon kanıtı;
gerçek PLC peer/ACK programı ve beklenen sonuç fixture'ı. Kabul: aynı bağlantıda
station izolasyonu/A–B geçişi, gerçek disconnect/Stop/drain/restart sonrası yeni
tetik olmadan edinim0, exact frame/terminal/ACK, korunmuş kalıcı kayıtlar.
Normal V4 native provider composition eksiği ayrıca uygulanıp kabul edilmelidir;
yalnız kamera takmak üretim yolunu hazır hale getirmez.

**COMM4-PHYS-02 — İki mount bağımsız 6D kabulü:** kullanıcı yönlendirmesiyle.
Önkoşullar: robot/controller/firmware actual pose sözleşmesi; base/flange/TCP
ve offset revision; lens/intrinsics/distortion ve plaka ölçü/belirsizlik;
güvenli örnekleme planı, solve'dan bağımsız holdout/reference ve önceden onaylı
mm/rad toleransları. Native frame/capture kapısı ve normal calibration adapter
composition tamamlanmadan başlatılmaz. Kabul her iki modda exact SAMPLE pairing,
independent accuracy raporu, stale dependency retleri, atomik activation ve
restart'ta Run/Auto kapalı olmasıdır. Robot hareketini robot sistemi yürütür.

**COMM4-UI-NATIVE-01 — OS input/WGC/DPI saha kabulü:** erişilebilir masaüstü,
gerçek normal/küçük ekran ve hedef DPI/monitör düzeni ile kullanıcı yönlendirmesiyle.
Mevcut Windows Qt raster kabulü korunur; OS input başarısı varsayılmaz.

Bu ID'ler açık takip kapsamıdır; yeni faz/kart bu oturumda başlatılmadı.
Güncel kullanım ve migration akışı [workspace rehberindedir](communication-workspace.md).
