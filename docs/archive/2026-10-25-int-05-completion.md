# INT-05 — Kaynak doğrulama entegrasyonunun tamamlanması

25 Ekim 2026. Uygulayıcı: Codex.
Dal: `codex/camera-a0-e2-integration`; başlangıç `44840d2` (inceleme),
uygulama tabanı INT-04 `0993f3f`. Claude'un `2b580eb` uygulaması kaynak bazında
bu dala alındı; ayrı worktree/dalı değiştirilmedi. İnceleme kaydı ve geçmiş
korundu; tamamlanan INT-05 tek görev commit'inde kaydedilir. Push yapılmadı.

[Özgün INT-05 kartı](../architecture/camera-integration-a0-e2-plan.md),
[INT-02 politika matrisi](../architecture/camera-integration-int02-matrix.md),
[güncel sözleşme](../architecture/source-verification.md),
[ilk Claude uygulaması](2026-10-25-int-05.md),
[incelemede yeniden üretilen açıklar](2026-10-25-int-04-05-review.md).
INT-01 bundle ve INT-02/03/04 kararları korunur; bu kart INT-06 UI bağlantısını içermez.

## Uygulama ve kapatılan bulgular

- Owner plan → immutable worker capture → ticket/generation accept → gerçek close/
  drain → exact reauthorization → bütün-set admission yolu entegrasyondadır.
  Webcam yeni kareyle, normal OECS typed experimental yolla doğrulanır. Native
  backend/admission yoksa açık ret vardır. Ham kaynak model/probe miras almaz.
  Aynı uyumlu industrial kamera/setup/profili logical kaynaklar tek probe ile
  paylaşır; çelişen profiller hazırlıktan önce reddedilir. Dosya canlı izin yenilemez.
- **İnceleme P1/epoch kapandı:** ScopedActiveCalibration salt bellek içi kamera ve
  policy invalidation token'ı sunar. Verification başlangıç token'larını drain
  sonunda, reauthorization öncesi/sonrası ve Ready okunurken denetler. Frame sonrası
  inventory/disconnect, drain sırasındaki kopma ve policy A→B→A eski probe ile izin
  vermez. Yeni açık probe başlangıçtaki izinsizliği giderebilir. Kalıcı izin biti yok.
- **İnceleme P1/reçete kapandı:** Runtime begin/requireReady doğrulamayı çalıştırılacak
  immutable snapshot'ın özgün reçete/dizin kimliğiyle karşılaştırır. Tek kaynaklı
  systemRecipe'nin türettiği binding için kimlik mevcut bounded validation geçmişi
  ile bellekte eşlenir. B'nin Ready'si A'yı veya başka dizini çalıştıramaz;
  rollback da aynı kapıdan geçer. Çağrılmamış verification legacy yolu korur.
- **İnceleme P2/replay kapandı:** Plan gerçek dosyayı salt okunur decode eder;
  trusted metric çözünürlük/model/lens uyumu, experimental typed raster/köken ve
  readback denetlenir. Fingerprint piksel hash/geometri/format/experimental metadata
  ve varsa en fazla 1 MB sidecar hash'ini içerir. Aynı çözünürlükte piksel değişimi,
  sidecar ekleme/değiştirme ve iş sırasında dosya değişimi Ready/accept'i reddeder.
  Bozuk dosya ve yanlış metrik çözünürlük yeni Verify'da reddedilir.
- **Stop/drain:** RuntimeCoordinator doğrulama worker'ı ve gerçek close tamamlanana
  kadar Stopping kalır. Poll kalan verification drain'ini yürütür; proje/Load ve
  yeniden kamera sahipliği bu sırada kapalıdır. Tamamlanmış Ready normal Stop'ta
  korunur. Verification fit, production cycle, kayıt veya PASS üretmez.

## Değişen yollar

| Yollar | Sorumluluk |
| --- | --- |
| `src/application/SourceVerificationService.hpp` | Yeni immutable owner/worker, ticket, drain ve bütün-set servisi; epoch/admission denetimi |
| `src/application/MeasurementWorkspace.hpp` | Plan/paylaşım, replay doğrulaması/fingerprint, mutation ve operational kapıları |
| `src/application/ScopedActiveCalibration.hpp` | Salt owner-side invalidation token'ı; aktivasyon gövdesi kopyalanmadı |
| `src/application/RuntimeService.hpp/.cpp` | Opsiyonel verification bağlantısı, exact snapshot identity, prepare/begin/requireReady ve poll |
| `src/application/RuntimeCoordinator.cpp` | Stop iptali, Stopping/drain ve replaceable |
| `src/application/ProductionRecipeService.hpp` | Canonical recipe identity için iki application friend; kalıcı format değişmedi |
| `tests/source_verification_test.cpp`, `CMakeLists.txt` | Yeni sözleşme testi ve ek negatif regresyonlar |
| `docs/architecture/source-verification.md`, INT planı/matrisi, progress ve arşiv dizini | Güncel sözleşme/devir |

README, UI akışı, launcher, codec/kalıcı şema ve yerel config değiştirilmedi.
İlk Claude kapanışı tarihsel içerikle taşındı; eski inceleme kaydı değiştirilmedi.

## Yeni doğrulama ve kanıt

Kanıt kökü: `build/integration-a0-e2/int05-completion/`.

- Regresyonlar önce mevcut INT-05 uygulamasında çalıştırıldı: inventory invalidation
  sonrası eski probe kabulü yeni testte başarısız oldu (`regressions-before.log`).
  Önceki incelemenin üç yeniden üretim çıktısı kendi kapanışında korunur.
- Yeni source_verification testi, üç applicationType'ın her birinde 1/2/10 kaynak;
  raw-only/mixed, son kaynak hatası, bireysel izin/bütün-set ayrımı, ortak kamera,
  profil çakışması, gerçek Preview/Editor/Runtime lease sahipleri, Stop/geç/duplicate
  completion, iş/drain sırasında disconnect, policy epoch, exact Runtime/rollback/
  dizin kimliği, trusted/experimental dosya/sidecar ve yan etkisizlik senaryolarını kapsar.
- Önceki entegrasyon koşumu: Release UI ON tam CTest **121/131** geçti
  (`ui-tests.log/xml`). On hatanın dördü localhost HELLO/Communication timeout;
  kalan altısı `multi_source_production_contract` (Missing result),
  `multi_source_graph_contract` (pre-cancel), `backend_capabilities`,
  `measurement_dag` (SegFault), `industrial_runtime_admission_contract` ve
  `ui_pose_tracking_smoke`. Bu altı hata INT-04 ayrı derlemesinin tarihsel
  `baseline-head-ctest.log` kaydında da aynı mesajlarla vardır; yeni başarı veya
  bu görevde giderilmiş sorun olarak sunulmaz. Tarihsel kayıt ilk Claude
  worktree'sindeki `build/integration-a0-e2/int05/` altındadır.
- Önceki UI OFF ilgili CTest **18/20**; iki localhost testi sandbox dışında
  **2/2** geçti (`core-tests.log/xml`, `core-socket-tests.log/xml`).
  Bunlar önceki koşumlardır; son kapanış koşumu aşağıda ayrıca belirtilir.

## Son kapanış doğrulaması — 25 Ekim 2026

- Release **UI ON + ZXing** ve **UI OFF** build komutları yeniden çalıştırıldı:
  ikisi de exit 0, `ninja: no work to do`. Mevcut derlemeler güncel kaynakla
  eşleşir; bu koşum temizden derleme değildir. `final-ui-build.log`,
  `final-core-build.log`.
- UI OFF ilgili CTest: **9/9 geçti**, 115,04 saniye
  (`final-core-tests.log/xml`). Source verification, scoped calibration/runtime,
  industrial scoped calibration, experimental metric runtime, diagnostics,
  coordinator ve iki camera session sözleşmesi kapsandı.
- UI ON ilgili ilk koşum **6/10 geçti**; dört localhost testi sandbox içinde
  HELLO/Communication timeout verdi (`final-ui-tests.log/xml`).
- Aynı dört test sandbox dışında seri koşumda **4/4 geçti**, 94,01 saniye
  (`final-ui-socket-tests.log/xml`): multi_source_terminal_trace,
  plc_socket_runtime, industrial_oecs_experimental ve ui_shell_smoke.
  Böylece seçili UI ON on testin tamamı geçti; bu filtreli doğrulamadır.
- `source_verification` her iki Release yapılandırmasında **417 kontrol** geçti;
  inceleme düzeltmelerinin kalıcı negatif regresyonları bu testin içindedir.
- Tarihsel INT-04 baseline çıktısının kopyası:
  `historical-int04-baseline-tests.log`. Tam suite'in açık altı hatası yukarıda
  korunur; birleşik kabul veya tüm testlerin geçtiği iddia edilmez.
- `python scripts/check-context-docs.py` ve `git diff --check` başarılı.
  Son kaynak/binary hash'leri `final-source-binary-manifest.json` içindedir.
  Görev `INT-05: integrate source verification with camera sessions` başlıklı
  tek commit ile kapatılır; push yapılmaz.

Graph doğrulaması Tier 2'dir: mutlak `C:/Projects/Tvision` ile list_projects/
index_status, search → trace → snippet ve kapsam denetimi kullanıldı. Kısmi veya
eskimiş satırlar doğrudan kaynaktan okundu; graph tek başına tamlık kanıtı değildir.
UI kodu/akışı değişmediği için yeni görsel kabul iddiası yoktur.

## Sınırlar ve devam

Sonraki kart **INT-06**: MainWindow bağlantısı, kullanıcı komutu/readiness tablosu
ve restart UI regresyonları. Bu kartta başlatılmadı; ürün UI'ı henüz verification
komutunu çağırmaz. Fiziksel çekim yapılmadı; test adaptörleri ürün konfigürasyonuna
aktarılmaz. Native OECS pending-command, fiziksel mm/lens/PLC/robot, MUI-04,
DPI/çoklu monitör, CUDA, kurulum/dağıtım ve önceki baseline engelleri açıktır.
