# A8 — Taslak, doğrulama ve aktif snapshot sözleşmesi

7 Ekim 2026. `application/ProductionRecipeService` donanımdan bağımsız, owner-thread üzerinde çalışan bellek içi çekirdektir. Mevcut ProjectService taslakları, editor Run Flow ve `.vfproject` formatı değişmedi. Runtime ekranı bu servise henüz bağlı değildir; üretim kabulü 6C'de tamamlanır.

## Üç ayrı nesne

- `Recipe`: ProjectService tarafından düzenlenen ve kaydedilen taslak. Diskteki DRAFT/VALID/INVALID alanı bir aktivasyon izni değildir.
- `RecipeValidation`: servis tarafından oluşturulmuş UUID, principal, UTC zaman, içerik ve bağımlılık SHA-256 kimliği. Kapsamı `execution_configuration` olarak açıkça kayıtlıdır. Yapısal kabul, gerçek kamera/kalibrasyon doğruluğu, tolerans veya parça kararı kabulü değildir.
- `ProductionSnapshot`: const shared pointer ile paylaşılan Recipe kopyası, proje klasörü, doğrulama ve bağımlılık manifesti. Çalışan worker taslağı değil bu snapshot'ı yakalar. Sonraki taslak düzenleme/Save/discard kopyayı değiştirmez. Reçetenin status alanı kopyalanır fakat servisin durum geçişlerini yönetmez.

İçerik kimliği recipe ID/revizyon/şema, isim/açıklama, applicationType, kamera/kalibrasyon referansı, bütün definition ve normalize edilmiş mutlak proje klasörünü kapsar. Status ve oluşturma/değişiklik tarihleri içerik izni değildir. Aynı recipeVersion altında yapılan taslak değişiklikleri de hash'i değiştirir. Gömülü model PNG ve ROI definition içinde kalır.

Bağımlılık manifesti `ExecutionService::version()`, kullanılan tool ID/kind/registry version, Match model/geometri kimliği, güvenilir resolver'dan gelen dış varlık SHA-256 kimlikleri ve varsa CalibrationDependency schema 1 kaydını taşır. Engine davranışı değiştiğinde execution sürümü, tool davranışı değiştiğinde descriptor sürümü artırılmalıdır. Bunlar binary imzası veya otomatik kaynak hash'i değildir.

Kalibrasyon referansı varsa resolver bağımlılığı sağlamalıdır; kimlik eşleşmezse veya sürüm/kamera/lens/çözünürlük/düzlem/frame/birim değişirse eski doğrulama kullanılamaz. A8 bağımlılık kaydını bağlar; katsayıların ve fiziksel ölçüm doğruluğunun kabulünü yapmaz. Kalibrasyon hesapları ve mm/pose çıktıları 6C/6E'de uygulanır.

## Doğrulama ve yetki

Servis Recipe/editor formatı, typed DAG, required input, enabled upstream, tek enabled dosya kaynağı ve boş olmayan kaynak yolunu kontrol eder. Enabled Match için mevcut MatchService ile aynı decoder Gray8 PNG/base64, boyut/ROI eşleşmesi ve doku kontrolünü yapar. Referans Edge kutusu/anchor ve model kimliği güncel olmalıdır. Mevcut beş built-in tool kapsamı desteklenir; compiled extension üretim kabulü henüz yoktur. Üç applicationType aynı servis yolunu kullanır.

`Authorize(principal, action)` güvenilir application policy'dir; Validate/Activate/Rollback/Run ayrı eylemlerdir. Policy olmadan veya principal boşken varsayılan ret uygulanır. UI'dan rol boolean'ı ve dışarıdan doğrulama nesnesi alınmaz; aktivasyon yalnız bu servis örneğinin ürettiği validation ID'yi kabul eder. Kullanıcı kimliğinin doğrulanması, kalıcı rol yönetimi ve Runtime komut bağlantısı 6C işidir. Bu aşama tam Users uygulaması değildir.

Kaynak image dosyası değişken edinim girdisidir: piksel içeriği doğrulama bağımlılığı olarak dondurulmaz ve varlığı/decode'u Validate sırasında sınanmaz. Gerçek her cycle'da AcquisitionService hata/timeout ve typed sonuç kontrollerini yapar. Model/kalibrasyon gibi değişmez varlıkların resolver kimlikleri ayrı tutulur. Eksik source dosyası ERROR üretir; yapısal doğrulama PASS/FAIL üretemez.

## Aktivasyon, rollback ve çevrim sınırı

Activate güncel taslak içerik/yol kimliğini ve resolver bağımlılıklarını yeniden kontrol eder. Eski doğrulama veya yetkisiz principal ret alır; başarısız işlem aktif snapshot'ı değiştirmez. Rollback yalnız aynı servis ömründe daha önce etkinleşmiş snapshot'lara yapılır; güncel bağımlılıklar tekrar sınanır. Değişmiş kalibrasyon/varlıklarla geçmiş sürüm sessizce açılmaz.

`beginCycle` Run yetkisini ve bağımlılıkları kontrol eder; tek in-flight ticket ve immutable snapshot döndürür. Aktif cycle varken yeni cycle, Activate ve Rollback Busy hatası alır. Yeni taslağın bağımsız Validate işlemi sürebilir. Worker bu snapshot'ın definition/path/recipe ID/content ID'sini ExecutionService'e verir. Snapshot, lease bitene kadar sabittir; sonuç kabulünden sonra yeni sürüm etkinleştirilebilir.

`finishCycle` owner-thread üzerinde terminal execution sonucu, recipe/config kimliği, frame/output cycle kökeni ve ticket kontrolünü yapar. Yanlış/stale/duplicate callback durum değiştirmez; aynı cycle ID son 128 kayıtta yeniden kabul edilmez. Ret sonrasında gerçek worker sonucu ile tamamlamak veya worker artık bittiyse `abandonCycle` ile ERROR kaydetmek gerekir. Abandon SDK/worker iptal komutu değildir; worker sonlanmadan kullanılması kaynak güvenliği sağlamaz. Future/cancel/close sahipliği mevcut JobSession ve 6C Runtime controller'ında kalır.

## Geçmiş, loglar ve persistence sınırı

Snapshot geçmişi varsayılan 16, yapılandırılabilir 1–256 kayıtla sınırlıdır. Limit dolunca Validate HistoryFull verir; aktif/rollback verisi otomatik silinmez. Her definition en fazla 16 MB, dış varlık manifesti en fazla 256 kimliktir. Ayrı toplam byte bütçesi/kalıcı arşivleme A9/6C/7'de değerlendirilir.

Cycle trace schema 1, son 128 kaydı bellekte tutar; eski trace otomatik düşer. Principal, ticket, recipe/revision, validation/config/dependency ID, engine/tool/model/calibration manifesti, UTC başlangıç/bitiş, gerçek cycle/frame/source/koordinat kökeni, tool execution state ve mevcut sayısal/boolean/nokta/region çıktıları kayıtlıdır. Ham image tamponları loga kopyalanmaz. İletişim durumu `NOT_REQUESTED` olur; PLC gönderimi yapılmaz. PartDecision veya robot pose icat edilmez; ERROR/SKIPPED ayrı execution durumlarıdır.

Bu geçici kayıtlar kalıcı cycle logları/audit/retention değildir. Restart sonrasında aktif sürüm ve doğrulama yoktur; disk VALID veya başka servis UUID'si otomatik güvenilmez. Atomic snapshot manifest/geçmiş persistence ve yeniden kabul politikası A9/6C/7'de uygulanacak; mevcut root/recipe schema 1, editor schema 5 ve atomik kayıt korunur.

`production_recipe_contract`: gerçek PNG acquisition/DAG sonuç/trace, taslak izolasyonu, version boundary/rollback, servis yetkisi, bağımlılık ve model invalidation, corrupt/missing model, DAG/disabled kaynak, owner-thread, yabancı/stale/duplicate result, restart ve bounded history/log kabulü. Release ve CTest 11/11 geçti. Sentetik fixture fiziksel kamera veya kalibrasyon kabulü değildir.
