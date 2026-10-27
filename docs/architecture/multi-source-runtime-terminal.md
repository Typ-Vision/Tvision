# KMC-10c2 / KMC-10d — Çok kaynaklı kalıcı Runtime

17 Ekim 2026. KMC-10c1'in immutable kaynak/model seti aynı worker ve dal replay kapıları korunarak kalıcı Runtime yoluna bağlandı. Doğrulama sayıları ve teslim durumu [ilerleme kaydında](../progress.md), birleşik kabul [B devir belgesinde](multi-source-release-acceptance.md) tutulur.

## Terminal schema7 ve tarihsel kanıt

`RuntimeService` explicit editor9 için trusted bütün-set provider alır. MainWindow provider'ı `MeasurementWorkspace::productionSources` üzerinden bağlar; UI algoritma veya kamera SDK'sı çağırmaz. Legacy tek-source `ProductionRecipeService` yolu schema9'u doğrudan kabul etmez; çok kaynaklı admission `MultiSourceRuntimeService` üzerinden bütün dalları denetler. Provider yoksa editor9 kapalı kalır.

Arşiv callback'i olan snapshot `multi_source_runtime/1`, yalnız bellek servisi `multi_source_in_memory/1` scope'u taşır. Her ikisi aynı immutable completion kapısını kullanır. Kalıcı terminal schema7 canonical configuration/content hash, tüm sourceSet/dependency hash, özgün editor9 definition, N kaynak-frame eşleşmesi ve dal başına mevcut schema3–6 kanıtını taşır. Dallar source UUID ile anahtarlanır; ad veya aynı pikseller kimlik yerine geçmez. Genel parça kararı eklenmedi.

Okuyucu schema1–6 davranışını korur. Schema7 yalnız schema3–6 dallarını kabul eder; recursive schema7 yoktur. Tam graph/source seti, missing/duplicate/foreign source/frame/tool, kaynak koordinatı, declared typed port, branch/top frame-tool eşitliği, calibration ve reference kanıtı yeniden doğrulanır. DONE tüm required kaynak ve tool setini ister. ERROR yalnız edinilen gerçek alt kümenin tanısını taşıyabilir; SKIPPED frame/sonuç taşımaz. Outer ProductionStore SHA256 ve atomik QSaveFile/QLockFile sınırları korunur. Hash bütünlük kanıtıdır; dijital imza veya kötü niyetli bütün dosyanın yeniden yazılmasına karşı kimlik doğrulama değildir.

Record failure ilk terminal nesnesini ve private trusted completion'ı bellekte dondurur. Yeni begin/prepare kapalıdır. Retry aynı ticket, snapshot capability ve completion digest'ini ister; aynı record ID/payload ile idempotent append yapar. Capture/provider/model çözümü veya algoritma replay'i tekrarlanmaz. Başarıdan önce kullanıcıya sonuç yayımlanmaz. Restart yalnız geçmiş okur; aktif snapshot, cihaz izni, pending in-process completion veya ACK oturumu geri yüklemez. Mevcut 16 MiB terminal / 20 MiB dosya / 512 MiB arşiv / 10000 dosya sınırları sürer; otomatik retention yoktur. Power-loss veya çok süreçli lease garantisi verilmez.

Logs dal başına model sürümü, etkinleştirme, kurulum ve sonuç kanıtını gösterir. CSV legacy kolonlarını korur; schema7 içeren export'a executionState, sourceId, sourceSet ve sourceFrames eklenir. Çıktısı olmayan ERROR/Stop çevrimi boş sonuç hücreleriyle ayrı kanıt satırı olarak korunur; scalar/değer uydurulmaz. sourceSet içindeki referenceBindings/systemCalibration/measurementSource tam tarihsel bağlamı taşır; canlı registry'ye ihtiyaç yoktur. Rasterlar terminal JSON'a gömülmez.

## Industrial terminal8 / terminal9

Industrial dal terminal8, industrial içeren whole-set terminal9 yazar. Eski terminal1–7
okuyucusu korunur; schema7 industrial alan kabul etmez. Terminal8 exact alan seti,
industrialSource/1 frozen admission ve ayrı industrialAcquisition/1 taşır. ACQUIRED,
actual readback, fiziksel/backend/route/profile/setup, özgün backend ve mapped execution
metadata ve raster content hash'ini bağlar. NOT_ACQUIRED yalnız source/reason/errorCode
ile kaydedilir; kare, piksel hash'i, scalar veya karar uydurulmaz. Partial ERROR'da
başarılı dalların gerçek kanıtı korunur; Stop bütün frame/sonuç/industrial evidence'i temizler.

Private completion digest industrial evidence'i de kapsar. Runtime worker dışında
caller completion yetkisi üretilemez; yanlış raster/request/readback/frame ve değişmiş
completion ret olur. Record retry frozen payload'ı yeniden yazar; discovery/capture/model
çözümü yoktur. Tarihsel okuma SDK, registry veya provider gerektirmez. CSV terminal9 için
industrialEvidence kolonu ve edinilemeyen her industrial kaynak için boş scalar kanıt
satırı taşır. JSON raster saklamaz; hash dijital imza değildir. Normal ürün factory'si
native G1 kanıtı olmadan açılmaz; test factory normal ürün yolundan seçilemez.

## Industrial Runtime / Logs sunumu

Industrial içeren editor9 reçetede Load, graph ve sourceReference yapısını kontrol
eden çevrimdışı beyandır; aktif snapshot veya edinim yetkisi üretmez. Eksik/stale
registry/model/backend ve bütün required set Run/prepare ile begin'de yeniden tam
kontrol edilir. Diğer kaynakların Load kontrolleri değişmez. Run görüntü almaz.

N-source tablosu beyan veya immutable snapshot'tan vendor/model/seri, kamera,
setup/profile revision ve kaynağa bağlı modeli gösterir. Gerçek kare yalnız o
çevrimin frame eşleşmesinden gelir. SourceStatusPresenter Runtime ve offline Logs
aynı kimlik/actual/backend/producer kanıtını sunar; SDK veya registry çözmez.
Unsupported, unavailable, busy ve timeout ayrı metinlerdir; özgün tanı detayda kalır.
Stop ve çalıştırılmayan dal ayrıdır. Raw / px, ölçüm doğrulaması ve araç bazında
parça kararı ayrı sunulur. Genel parça PASS üretilmez.

Native admission sağlanmadığında offline Load görüntüsüz kalır ve Run açıkça
kullanılamıyor döner. UI smoke 1/2/8/10 offline industrial kaynağı normal/küçük
pencerede, mevcut gerçek file/scoped fixture'ları üç applicationType'ta sınar.
Test fixture'ı normal uygulamaya industrial provider enjekte etmez.

## vf.socket/2

Mevcut seçili `measurementToolId` scalar mapping'i bütün çevrim DONE ve ilgili gerçek Measurement sonucu olduğunda yayınlanır. Seçilen araç ikinci veya başka bir kaynağa ait olabilir. Çok output/aggregate karar eklenmedi. Terminal commit → PLC_RESULT event → ACK düzeni, timeout/retry/dedup ve süreç/oturum sınırları değişmedi. İletişim context'i aynı schema7 terminalde saklanır. Localhost fixture gerçek PLC kabulü değildir.

## Runtime kullanımı ve N kaynak UX

Kaydedilmiş editor9 reçeteyi Recipe Manager → Load Recipe ile yükleyin; Run yalnız hazırlığı yapar. Tek çevrim gerçek sequential acquisition başlatır. Üç applicationType aynı Runtime'ı kullanır. Hazırlık tablosu yalnız required kaynakları listeler; kullanılmayan offline kamera hazırlığı bloke etmez. Tablo model/sürüm, hazırlık veya edinim hatası ve actual frame ID'yi ayrı gösterir. Hazırlık snapshot'ı gerçek kare/ölçüm kabulü değildir.

N>1 kaynak seçici ve klavyeyle gezilebilir tablo Viewer kaynağını seçer; araç seçici yalnız o dalı gösterir. Raw raster, işlenmiş preview, ROI/reference/overlay ve metrik resolver aynı source/frame/model üzerinde kalır. Aynı fiziksel CameraId veya aynı legacy webcam backend/index'in ikinci acquisition aracıyla talebi reddedilir; gizli ikinci capture veya çelişen profil reuse yoktur. Kaynak başına live preview açılmaz. Stop mevcut görüntüyü temizler ve erken/geç completion çıktısını bastırır. Logs / CSV düğmesi geçmiş ekranına gider; operator görünümünde de Runtime'a dön komutu Run durumunu değiştirmeden geri dönüş sağlar; fiziksel kabul veya production izni geri yüklemez.

## Kabul ve sınırlar

- `multi_source_terminal_trace`: üç applicationType × 1/2/8/10 kaynak, gerçek farklı dosya rasterı ve scoped model, lock/retry/changed completion, idempotent append, disk tamper, missing/duplicate/foreign frame/model, restart ve CSV. Ek 2 kaynak socket scalar/ACK timeout/dedup/record retry/Stop ve partial ERROR/model sürümü geçmişi.
- `ui_multi_source_runtime`: üç applicationType × 1/2/8/10, gerçek Viewer pikselleri ve ROI, kaynak/araç seçimi, preparation vs actual frame, busy/Stop, Logs/CSV, 1400×900 ve 850×600 PNG.
- Önceki graph/execution/immutable production, legacy typed terminal, scoped runtime, coordinator, socket ve kalibrasyon testleri birleşik kabulde korunur.

Edinim sıralıdır; synchronized trigger, ortak dünya veya robot pose birleşimi eklenmedi. Fiziksel cihaz/seri sürekliliği/transport/kopma/ölçüm doğruluğu/PLC/robot, native DPI/çok monitör, industrial SDK, lens distorsiyonu, CUDA ve kurulum/dağıtım kabulü ayrı açıktır.

## Ölçüm tanı özeti (MUI-03)

Yeni tool kayıtlarında isteğe bağlı `measurementDiagnostic` alt nesnesi schemaVersion=1
ile saklanır; terminal1–9 dış şemaları ve engine28 değişmez. Alan yoksa eski kayıt
olduğu gibi okunur. Sabit alan kümesi code/stage, requested/found/required/used,
polarity/contrast/border/noCandidate sayaçları ve toolId/sourceId/frameId/cycleId/
evaluationId taşır. Profil/raster veya scan başına aday dizisi gömülmez.
Kimlik metinleri en çok 256, code 64 karakter; scan ve nokta sayıları 0–4096,
aday retleri 0–16777216 tamsayıdır. found≤requested, used≤found ve
noCandidate≤requested koşulları uygulanır. Bilinmeyen alan/alt sürüm/aşama,
kesirli/negatif/aşırı sayaç, farklı cycle/evaluation/tool veya edinilmemiş frame
reddedilir. Whole-set kaynak graph'ı ve dal/top tool eşitliği ayrıca doğrulanır.

Özet private completion digest'ine dahildir; sonradan değiştirilen tanı completion
olarak kabul edilmez. Tanı karar vermez, NOT_EVALUATED kalır; Stop/SKIPPED içinde
taşınamaz. ERROR fit portlarını boş tutar; DONE no-found/qualified=false kendi
mevcut typed port semantiğini korur. Başarısızlıkta gerçek gözlem sayıları tutulur;
örnekleme ön kontrolü tarama yapılmadığını açıklar. Diğer exception türlerinden
metin eşlemesiyle tanı veya sayı üretilmez. UI application presenter'ını kullanır.

Logs aynı sınırlı nesneyi çevrimdışı doğrular/sunar. CSV mevcut kolonlarını korur;
çıktısı olmayan tool için de `measurementDiagnostic`, `diagnostic/1`, valid=false,
diagnostic=true ve reason içinde JSON özetiyle ayrı satır ekler. Whole-set CSV
sourceId/frameId bağını korur. Özgün scalar satırlar ve genel karar semantiği
aynıdır. Bu ek açıklama replay'i algoritmanın raster olmadan yeniden çalıştırılması
veya fiziksel kabul değildir; mevcut archive hash'i dijital imza değildir.


## Terminal10 / ResultBundle — COMM-13

Enabled Result or Publish Data writes terminal10 with `baseSchemaVersion` and
optional versioned `resultBundle` schema1. Other records retain their legacy
schema and bytes. Base evidence remains schema3–9, validated recursively;
whole-set publication dependency joins its frozen dependency hash.

Bundle stores cycleId, configHash, dependencyId, OK/NOK/NOT_EVALUATED decision,
groups and SHA256 bundleId over canonical compact content without bundleId.
Each group carries logical/profile contract, intent/profile dependency identity,
required/resultValid/reason and typed fields with origin. Missing scalar/Pose
fields have null values, never false/identity defaults. Invalid groups cannot
claim normal payload. Spatial assignment codec now accepts null transforms only
with non-valid metadata; existing valid encodings remain unchanged.

Terminal and bundle are one ProductionStore payload committed using the existing
QSaveFile path. Recording failure freezes the same ID/content; retry acquires no
new frame. Restart reads history only. Summary includes bundle identity/decision;
CSV continues to export the exact base scalar columns. No delivery or ACK state
is written into this immutable payload; outbox begins at COMM-14.
