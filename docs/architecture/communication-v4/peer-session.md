# COMM4-07 — HELLO ve connection session otoritesi

`CommunicationPeerSessionService` Qt owner thread'inde çalışan state portudur.
`CommunicationServerLifecycle` isteğe bağlı bu portu alır; normal process
composition'u `main.cpp` içinde her zaman bağlar. Testlerin eski raw transport
fixture'ı port verilmediğinde korunur; üretim composition'unda raw/v3 fallback yoktur.
UI, domain/store şeması ve frozen wire revision değişmedi.

## Kimlik ve izin

Makine dizinindeki `communication-identities-v4.json`, mevcut
`JsonStationIdentityStore` formatıdır. Enabled listener dosya yokken de listen
edebilir; HELLO `RECORDING_ERROR` döndürür. Başlangıç, HELLO veya reconnect
installation/peer/epoch yaratmaz, registry'yi yazmaz. Enrollment yerel explicit
registry portuyla yapılır. Tek controlling peer politikası için tam bir canlı
peer/epoch kaydı gerekir; birden fazla canlı kayıt `CONTRACT_MISMATCH` ile
fail-closed olur. Retired epoch `RETIRED_EPOCH` olarak kalır. UUID authentication
değildir; güvenilen yerel ağ politikası sürer, TLS/safety iddiası yoktur.

İlk TCP connection tek otorite slotunu tutar. Başarılı HELLO yeni rastgele
session UUID döndürür; contract0 yalnız discovery, frozen exact revision
`00434634-0002-4000-8000-202610290001` negotiation'ı session mutation iznini açar.
Yeni HELLO eski session'ı önce fence eder; yanlış revision/enrollment veya
store hatasında eski exact izin korunmaz. İkinci connection bir frame'lik bounded
ret yolunda `PEER_BUSY` alıp kapanır; owner/session değişmez. En çok bir böyle
ret socket'i tutulur, fazlası hemen kapanır; sessiz ret socket'i2s'de kapanır.

Her operasyon current connection/session/peer/epoch'u denetler ve kalıcı
registry'yi yeniden okur. Dışarıdan epoch retirement, ambiguous enrollment,
installation değişimi veya bozulma mevcut session'a izin taşımaz. Reconnect ve
service restart yeni session üretir. `requestIdentity()` installation+peer+epoch+
station UUID+uint64 request döndürür; session anahtara girmez. >2^53 ve uint64 max
değerleri double/JSON sayı dönüşümünden geçmez. Fingerprint codec'in frozen
session hariç hash'idir. Bu port durable intent/ACK ledger oluşturmaz.

`authorize()` yalnız session katmanının sonucudur; station admission değildir.
`handle()` bugün mutating komutları dispatch etmez: izinli olanlara `NOT_READY`,
stale/wrong/retired olanlara ilgili ret döner. Tam katalog snapshot'ı, active
recipe/activation, admission ve production gate kontrolleri08'in application
router'ında ayrıca zorunludur. ACK/RECOVER/rollover'ın gerçek ledger işlemleri
13/14'tedir. Remote Run/recipe/Auto/Stop bu kartta uygulanmadı.

## Salt okunur sorgu portları

Discovery session'ı STATUS/CATALOG_GET okuyabilir. Global STATUS sıfır snapshot,
slotlar ve Ready=false döndürür; station STATUS station application portu hazır
olmadığından `NOT_READY` döner. Bağlantı kurulması üretim hazır anlamına gelmez.

CATALOG_GET registry'nin canlı entry'lerini kategori/key sırasıyla okur;
key0 ilk kayıt, nextKey sonraki aynı kategori kaydıdır. Retired entry dönmez.
Beklenen catalogue revision farklıysa `STALE_CONFIG`; bilinmeyen key
`CATALOG_REPLY/UNKNOWN_KEY` ile exact category/key/revision echo eder.

Başarılı entry için state portunun `Metadata` callback'i, aynı registry snapshot
ve entry'den256 byte frozen metadata verir. Callback immutable yerel manifest'in
saklı content/dependency digest'lerini ve gerçek snapshot alanlarını sağlamalıdır.
Reply codec'le yeniden doğrulanır. Eksik metadata `NOT_READY`, bozuk provider/store
`RECORDING_ERROR` döndürür. Registry kimlikleri metadata/digest yerine geçirilmez.
Normal main henüz manifest provider'ı bağlamaz: listede kayıt varsa metadata hazır
değildir. Handshake testi açık sentetik metadata provider'ı kullanır; bu gerçek
istasyon veya fiziksel kabul iddiası değildir.08/11/12 ilgili sahiplikte provider
bağlarını tamamlar; wire formatını değiştirmez.

## Transport ve doğrulama sınırı

Socket başına `CommunicationV4Framer` ve `CommunicationV4WriteQueue` kullanılır.
Read buffer4096, read2048 byte, batch4, queue64 frame, socket write high-water64KiB;
partial deadline frozen2000ms'dir.25ms timer sessiz partial deadline'ı ve geride
kalan batch'leri ilerletir. Byte'lar frame sınırında sıralı yazılır; saturation,
bozuk frame veya socket hata yolu bağlantıyı kapatır. Bu kart accepted iş
üretmediğinden queue taşması accepted cycle kaybetmez; durable replay13/14'tedir.
Profildeki eski parser/ACK/bütçe alanları frozen codec limitlerini büyütmez;
bu port frozen limitleri kullanır. Flush/write teslim veya ACK kanıtı değildir.

Release C kabulü: ON/ZXing ON ve OFF/ZXing OFF incremental build; yeni
`communication_peer_session`, listener/registry/station activation/codec/wire ve
legacy TCP/socket komşu CTestleri. Frozen sözleşme, codec, domain/store ve UI
değişmediğinden tam F suite veya U görsel kabulü bu kart için uygulanmaz.
Yeni koşum ve açık sınırlar [kapanış kaydındadır](../../archive/2026-10-29-comm4-07.md).
