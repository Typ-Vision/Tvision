# KUI-1 — Ortak kalibrasyon kayıt ve etkinlik sözleşmesi

15 Ekim — **KUI-4/KUI-5 entegrasyonu tamamlandı**; [ortak aktif resolver ve sürüm/migration açıklaması](system-calibration.md). Katalog yolu <proje>.active-calibrations.json; wrapper1/2/3, named/date ve explicit legacy-validated adoption desteklenir. Legacy raster kökeni eksikse üretilmez. SystemCalibration original modelden ayrı sealed transient binding kurar; eski validated() bağımsız kontrol olarak kalır. Editor/immutable Runtime/Logs bağlıdır. Alttaki KUI-1 teslim/sözleşme kaydı tarihseldir.


15 Ekim 2026. [Sihirbaz planının](calibration-wizard-plan.md) ilk paketi application katmanında uygulandı. Yeni ekran veya mevcut Editor/Runtime resolver değişikliği değildir. Yeni servis henüz MainWindow tarafından oluşturulmaz; ürün akışına bağlanması KUI-4/KUI-5 kapsamındadır.

## Model ve bağımsız kontrol

`CalibrationModelRecord::create` mevcut `MeasurementService` matematiğini encode/decode ile yeniden çalıştırır. Uniform tek çift, legacy manual ve chessboard raw homography modellerini kabul eder. Modelin hazır olması `validated()` ile eşitlenmez: 400 px gerçek girilmiş uzunluğa karşı 10 mm, 0,025 mm/px üretir; bağımsız kontrol yokken residual listesi boş, kabul durumu null kalır. Bu test programatik çift matematiğidir; rasterdan nokta bulma KUI-2'dir.

Yeni `model-ready/1` politikası ve record schema1 eski model nesnesini değişmeden sarar. `modelReady` diskte true olsa bile iç model, setup, kontrol ve SHA256 kimliği replay edilmeden okunmaz. Sahte eski `validated=true`/uydurulmuş kontrol kabulü reddedilir. Eski `PlanarCalibration` private durumu veya eski Measurement üretim kapısı gevşetilmez.

`CalibrationControl` sıfır–256 gerçek bağımsız segment ve isteğe bağlı mm sınırı taşır. Tek bağımsız segment kullanılabilir. Ölçülen mesafe/residual gerçek modelden yeniden hesaplanır; sınırsız kontrolde kabul/ret null kalır. Sınır tanımlanınca gerçek en büyük residual dahil sınırla karşılaştırılır. Başarısız kontrol kaydedilebilir fakat etkinleştirilemez. Öğretim segmenti/tersi, tekrarlanan segment/ID, dejenere uçlar, yanlış frame/alan ve H fit köşeleri kontrol olamaz. H mesafesi noktalar ayrı dönüştürüldükten sonra hesaplanır ve mevcut hull korumasından geçer. RMS veya fiziksel doğruluk değeri kontrol yokken oluşturulmaz.

## Ortak kurulum ve görüntü kökeni

`MeasurementSetup` schema1: ortak `id`, `adapter` (image_file/webcam), ilgili edinim ayarlarının SHA256 `configId`'si, lens/düzlem beyanı, çözünürlük ve source/target koordinatları. Acquisition tool ID bu kimliğe dahil değildir. İki ayrı reçete tool ID'si aynı açıkça ilişkilendirilmiş kurulumu paylaşabilir; boyut eşitliği tek başına yeterli olmaz. Bu alanların herhangi birinin değişmesi resolver uyumunu bozar.

`CalibrationImageEvidence`: sourceToolId, frameId, decoded raster SHA256 ve acquired/setup_file/trial_file kökeni. İç legacy modelin sourceToolId ve varsa manual/H frameId'si eşleşir. Test kaynağı acquired diye etiketlenemez. `setup_file` dosya kökenini koruyarak açık kurulum beyanıdır; `trial_file` etkinleştirilemez. Servis fingerprint biçimini doğrular; rasterı okuyup hash hesaplayan veya fiziksel kaynak kimliğini doğrulayan servis değildir. Gerçek frame/hash/config türetimi, kullanıcı beyanı ve kurulum seçimi KUI-2/KUI-4'te application session tarafından sağlanmalıdır. Webcam index fiziksel cihaz kimliği değildir.

## Katalog ve etkin seçim

`ActiveCalibrationService` açık proje için application tarafından sahiplenilecek tek instance'dır; owner thread komutları sunar. Constructor mutlak katalog yolu, proje kimliği ve zorunlu `ChangeGate` alır. Yeni `visionflow.active-calibration` schema1 katalog en çok 4 MiB/128 kayıt içerir; ID başına sürümler ardışık ve append-only'dir. QLockFile ve direct-write fallback kapalı QSaveFile ile atomik tek dosya commit kullanılır. Katalog ve son seçim aynı yeni dosyada tutulur; proje/legacy katalogla çok dosyalı transaction iddia edilmez. Ürün dosya konumu KUI-4 entegrasyonunda belirlenecek; testler yalnız build altında yazar.

- `save` yalnız aday ekler; mevcut tam ID/sürüm/contentId seçimi korunur. Yeni sürüm eski etkin kaydı otomatik geçersizleştirmez veya yerine geçmez.
- `activate` bağımsız current setup, matematik/kontrol/köken ve change gate'i denetler. Gate Runtime/worker/tamamlanmamış kayıt/ACK'i KUI-5'te gerçek uygulama durumundan çözmelidir; KUI-1 testi gate ret davranışını sınar.
- Başarılı kalıcı commit sonrası bellek pointer'ı değiştirilir. Lock/uyum/kanıt/commit hatası önceki etkinliği korur. Revision monoton artar; üst sınır 1 milyardır.
- `resolve` tam seçimi tekrar katalogdan denetler; uyumsuzluk veya yoklukta exception verir. En yeni modele, başka kayda veya recipe binding'e fallback yoktur.
- `ActiveCalibrationSnapshot` tam record ve seçim revision'ının değişmez değer kopyasıdır. Mevcut production snapshot/permission yerine geçmez. Cache ve Runtime hazırlığına bağlanması KUI-5'tir.
- Constructor son seçimi yalnız `remembered()` ile sunar; aktiflik sıfırdır. Proje kimliği uyuşmayan katalog reddedilir. Proje değişiminde instance yeniden oluşturulmalıdır. Restart/başka proje üretim izni kazanmaz.

## Sürümler, migration ve consumer envanteri

Yeni kayıt opt-in ayrı sözleşmedir. Eski katalog `.calibrations.json`, `.vfcalstudy`, proje ve terminal dosyalarına yazılmaz; engine execution/22, root1/recipe1–2/editor8/terminal1–4 ve socket2 değişmedi. Legacy uniform/manual/H record/dependency1–3 ve study1 yolları korunur.

Eski bir kayda açık geçiş: eski codec ile yükle/replay et; gerçek kurulum ve görüntü kökenini açıkça ilişkilendir; `CalibrationModelRecord::create` ile yeni kimlik oluştur; yeni kataloğa kaydet; ana sayfa üzerinden ayrıca etkinleştir. Aynı eski ID/sürüm korunacaksa o ID'nin 1'den başlayan bütün geçmişi sırayla taşınmalıdır. Eski kayıt veya hash yeniden yazılmaz. Eski holdout kanıtı iç legacy modelde korunur; yeni kontrol listesine uydurulmaz. Birden çok eski recipe binding'den otomatik etkin seçim yapılmaz. Otomatik importer/ana sayfa henüz yoktur.

| Consumer | Mevcut sözleşme | KUI-5 değişimi |
| --- | --- | --- |
| CalibrationWorkflow/CalibrationService | Recipe-scoped exact binding, latest catalog ve validated model | Ortak etkin resolver, açık migration politikası |
| MeasurementService/Measurement executor | İki gerçek raw point → validated exact model → mm/tolerans | Model-ready context; source tool ile setup eşleştirmesi |
| EdgePair/MetrologyService | Raw px tanısı veya kalibre mm genişlik; Line dünya noktalarından refit | Varsayılan ortak mm, eski px toleranslarının açık incelemesi |
| RadialScan/CircleToolService | Exact world ROI/model, alan kontrollü gözlemler ve refit | Ortak model, başka exact modelde yeniden öğretim |
| GeometryMeasurementService | Aynı frame/gerçek typed özelliklerden dünya geometri hesabı | Ortak context; legacy px port koruması |
| PoseRoi/WorldTracking/ReferenceService | Taught exact calibration, her tap alan kontrolü | Otomatik yeniden bağlama yok; öğretim geçerliliği denetimi |
| EditorSession/ExecutionService | Recipe binding/hash tabanlı cache ve resolver | Aktivasyon revision'ıyla mm sonuç/izin invalidation |
| ProductionRecipe/Runtime/terminal/Logs/CSV | Immutable recipe binding ve trusted completion | Tam ortak record/aktivasyon kimliği; tarihsel sonuç aynı modelle kalır |
| MainWindowCalibration/Properties | Eski üç yöntem/beş bölüm, per-recipe bağlantı | Ana sayfa/sihirbazlar KUI-4; tüm metrik consumer bağlantısı KUI-5 |
| Calculate/Code/Pattern/Shape/Blob | Raw px/score/pose/alan portları | Keyfi mm/mm² dönüşümü yok; yeni fiziksel port açık typed descriptor ister |

KUI-5 legacy/global politika ayrımını açık kaydetmeli; global politika sonrası eski recipe resolver'a sessiz dönüş olmamalı. KUI-1'de mevcut ürüne global politika açılmadı. KUI-3 circle target descriptor/replay tamamlandı; [session/plaka sözleşmesi](calibration-wizard-services.md). Chessboard nested schema1 korunur; circle nested schema2 ile ayrı türde saklanır.

## Kabul

Release OFF/core ve ON/UI derlendi. Her pakette aynı ilgili **11/11 CTest** geçti; core 5,07 sn, UI 7,72 sn. `active_calibration_contract` **77 kontrol**: tek çift/boş kontrol, gerçek optional kabul/ret, uniform/manual/H replay, hull/fit köşesi, setup ayrımı, explicit activation, aday kaydı, lock/busy retlerinde eski seçimin korunması, restart/proje ayrımı ve schema/kimlik/validation tamper retleri. Legacy measurement/calibration/manual/H/production/runtime/migration regresyonları aynı koşumdadır.

Kanıtlar build/reference-core ve build/reference-ui altında `kui1-final-build.log`, `kui1-final-ctest.log`, `kui1-final-tests.xml`. İlk core koşumundaki yeni test heap hatası `remembered()` içindeki geçici QJsonObject'e bağlı QJsonValueRef ömründen kaynaklandı; nesne/value ömrü düzeltilip final testler yeniden çalıştırıldı. İlk başarısız koşum kui1-ctest.log/kui1-tests.xml altında korunur. UI değişmediğinden yeni görsel kabul yapılmadı; tam R8/KUI-6 paket koşumu iddiası yoktur.

KUI-2/KUI-3 session ve plaka servisleri tamamlandı; [kanıt ve sınırlar](calibration-wizard-services.md). Sonraki paket **KUI-4 — ana sayfa, iki sihirbaz, dosya/kurulum ve robot şablonu**. Fiziksel doğruluk, lens/robot, industrial acquisition, CUDA, kurulum/dağıtım ve T8.4.2b.3c corpus açık kalır.
