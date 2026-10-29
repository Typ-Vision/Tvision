# COMM4-03 — Identity registry schema 1

Uygulanan port: `StationIdentityRegistry` / `StationIdentityStore`;
JSON adapter: `persistence/StationIdentityStore.*`. Bu kart listener veya station
Runtime bağlamaz. Enrollment yerel/açık bir komuttur; load eksik dosyayı yaratmaz,
bozuk dosyayı boş registry ile değiştirmez. Legacy proje şeması değişmez.

## Kimlik ve transaction

Tek installation UUID namespace'inde W6 category 1–13 ortak uint32 allocator
kullanır. Key 0 yalnız sentinel'dir, registry kaydı olarak tahsis edilemez. Station/camera UUID
kimliği revision=0 taşır; diğer kategoriler nonzero immutable revision UUID ister.
`(category, entity UUID, revision UUID)` tek key'e karşılık gelir. Parent UUID
kimlikle birlikte sabittir; ad değişebilir. Ad boş/NUL olamaz, UTF-8 en fazla
64 byte; kesilmez. Opaque16 import/export RFC4122 byte sırasıdır; GUID mixed-endian
memory dump kullanılmaz. Null UUID yalnız optional revision/parent alanındadır.

Bind aynı kimlik/ad için idempotent; farklı parent/ad conflict'tir (rename ayrı
komut). Yeni revision yeni key alır. Retire kaydı silmez ve yeniden bind/revive
edilemez. `nextKey` monoton uint64 sayaçtır, `4294967296` exhaustion sentinel'idir;
UINT32_MAX tahsisinden sonra yeni bind açık hata verir. Aradaki kullanılmamış
key'ler de `nextKey` gerisinde tahsis edilemez. Import geçmişi tahminle doldurmaz.

Her değişim yeni catalogue revision UUID üretir; snapshot ve mutation exact
expected revision ister. No-op revision'ı değiştirmez. Store compareExchange da
installation, immutable identity, tombstone, epoch geçmişi ve monoton key'i
doğrular; history silme/çakışmayı sessiz onarmaz. Her peer için en fazla bir active
epoch vardır; retired epoch yeniden açılamaz. Retire/enroll yalnız kayıt portudur;
drained/controlling peer admission denetimi gelecek application kartlarındadır.
Bu servis production izni vermez.

## Dosya

Envelope exact iki alan taşır: `payload`, `sha256`. SHA256, payload'ın Qt
QJsonDocument Compact UTF-8 serialization byte'larından hesaplanır. Bu corruption
denetimidir; authentication veya rollback protection değildir. Payload:

| Alan | Şema |
| --- | --- |
| schemaVersion | integer 1 |
| installation, catalogueRevision | lowercase 32 hex digit / opaque16, nonzero |
| nextKey | canonical decimal string, 1–4294967296 |
| entries | en fazla 65536 kayıt; key uint32, category 1–13, entity/revision/parent hex UUID, name string, retired bool |
| epochs | en fazla 65536 kayıt; peer/epoch nonzero hex UUID, retired bool |

Dosya en fazla 32 MiB'dır. Unknown alan, invalid sayı/UUID, duplicate key/identity,
checksum/schema uyuşmazlığı veya eksik dosya hatadır. Schema fixture:
[`identity-registry-v1.json`](../../../tests/fixtures/communication-v4/identity-registry-v1.json).
Metadata W6a'nın encoder'ı bu kartta uygulanmaz; registry yalnız kimliği taşır.

Yazar aynı path için QLockFile tutar, kilit altında disk revision'ı yeniden okur;
eski yazar CAS hatası alır. Tek snapshot QSaveFile ile direct-write fallback kapalı
yazılır. Okuyucu atomik replacement öncesi veya sonrası tutarlı dosyayı görür.
Write/open/commit hatası admission'a üst katmanda açık blok olarak aktarılmalıdır.
PREPARED/HALF_WRITTEN/WRITTEN/COMMITTED hook'ları test failure injection içindir.
Process-kill testleri güç kesilmesi veya disk/controller fsync garantisi değildir.
Silinen dosya veya eski backup restore'u otomatik enrollment için gerekçe değildir;
kurulum/backup/rollback lifecycle'ı bu portun dışındadır.
