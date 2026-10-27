# vf.socket/3 — COMM-16 codec sözleşmesi

V3 ayrı explicit endpoint içindir; v2 parser/listener değişmez. Bu kart yalnız
codec/framing sağlar. Session eşleştirme, enum katalog üyeliği, exact input seçimi,
Runtime dispatch ve kalıcı ACK yetkisi adaptör/router sorumluluğudur (COMM-17).
Codec enum şekli/kod aralığını denetler; untrusted değer katalog oluşturmaz.

Sürümlü [v3 fixture](../../../tests/fixtures/communication/vf.socket-3.json) tüm
mesaj türlerini, Int64/scalar/null/Pose alanlarını içerir. [V2 fixture](../../../tests/fixtures/communication/vf.socket-2.json)
bilinen STATUS byte biçimini korur; mevcut v2 localhost testi parser uyumunu sınar.

## Framing ve türler

UTF-8 JSON nesnesi + LF; line üst sınırı LF hariç 65536 byte. CRLF, BOM, boş
satır, invalid UTF-8/Unicode, duplicate JSON key (escape eşdeğeri dahil), unknown/
missing alan, unknown sürüm/tip ve 32 seviyeden derin yapı reddedilir.
Framer socket paket sınırlarından bağımsızdır; read chunk en çok 65537 byte,
partial buffer en çok 65536 byte, dönen batch toplamı en çok iki line bütçesidir.
Adaptör `readAll()` yerine bounded read kullanmalı; framer hata verdiğinde peer'i
kapatmalı ve yalnız yeni bağlantıda reset etmelidir. Hatalı batch dispatch edilmez.

İlk partial byte'dan itibaren monoton 2000 ms deadline vardır; trickle veri süreyi
yenilemez. LF deadline anında/sonrasında gelirse reddedilir. Adaptör veri gelmese
de `tick(monotonicMs)` çağırır. Başarılı LF sonrası yeni partial yeni süre alır.

Her mesaj `v:3,type,sessionId` taşır. Kimlikler 1–128 ASCII `A-Za-z0-9_.:-`;
tablodaki boş izinleri hariç zorunludur. Sequence/revision UInt64 canonical decimal
**string** (0..18446744073709551615), baştaki sıfır/işaret/boşluk yoktur. Revision
semantik uygunluğu InputSnapshotStore'da ayrıca denetlenir. Hash lowercase 64 hex.

| type | Zarfa ek zorunlu alanlar |
| --- | --- |
| STATUS komut | Yok |
| HELLO / STATUS yanıt | state, ready, busy, recipeId, configId |
| TRIGGER | requestId, sequence, recipeId, configId, inputSnapshotId, inputRevision |
| INPUT | snapshotId, revision, recipeId, configId, fields, sourceTime, clockId |
| ACCEPTED / BUSY | requestId, sequence, cycleId |
| ACK / ACKED | requestId, resultId, payloadHash |
| INVALID | requestId, reason |
| ERROR | requestId, code |
| RESULT | requestId, sequence, recipeId, configId, inputSnapshotId, inputRevision, resultId, cycleId, execution, decision, resultValid, fields, error, payloadHash |

HELLO/STATUS recipeId/configId ve ERROR/INVALID requestId boş olabilir.
ready/busy Boolean'dır ve ikisi aynı anda true olamaz; state/reason/code kimlik
biçimindedir. INPUT sourceTime 1–256 UTF-8 byte provenance metnidir; clockId ile
birlikte kayıt edilir, uzak saat freshness yetkisi değildir. INPUT recipe/config
kimliği ve TRIGGER inputRevision exact snapshot bağını açık yapar.

RESULT execution `DONE|ERROR|SKIPPED`, decision `OK|NOK|NOT_EVALUATED`;
resultValid Boolean ve decision'ın evaluated olmasına eşittir. ERROR/SKIPPED
evaluated karar taşıyamaz. DONE error boş; diğer terminal error en çok 4096 UTF-8
byte metindir. Karar ile alan geçerliliği ayrı kalır: missing alan taşınabilir.
Codec gerçek çevrim/alan kökeninin otoritesini oluşturmaz.

## Typed field

fields en çok 64 elemanlı array; name benzersiz `A-Za-z` ile başlayıp en çok
64 ASCII `A-Za-z0-9_.-` karakteridir. Her elemanın exact alanları:
`name,type,unit,validity,value,reason,producerId,producerPort,cycleId,frameId,configId,dependencyIds`.

Scalar type `Boolean|Int32|UInt32|Int64|Float64|String|Enum`; mevcut
CommunicationValues sözleşmesindeki range/finite/string/unit/validity/provenance
kuralları uygulanır. Int64 decimal string, Enum `{definitionId,code}`;
null missing/stale/skipped/invalid/error için reason zorunlu, valid için değer
zorunlu ve reason boştur. false/0 geçerli değerdir. Bu zarf schemaVersion taşımaz;
wire sürümü v=3'tür, mevcut domain envelope'a internal schemaVersion=1 eklenir.

Spatial type `Pose`, unit `mm`; value mevcut SpatialTransform schemaVersion=1
nesnesidir: source/target FrameRef, right-handed-column-vector, mm translation,
quaternion-wxyz rotation. Pose tek record; alanlara sessiz parçalanmaz. Aynı
null/validity/provenance kuralları uygulanır. Üretici/katalog ve çevrim eşleşmesi
sonraki katmanın işidir; bu taşıma robot hareket/kalibrasyon kabulü değildir.

## Canonical persisted payload ve hash

Biçim adı **vf.canonical-json/1**: Qt 6 compact JSON; object key'leri her seviyede
UTF-16 lexical sıralı, array sırası korunur, UTF-8, whitespace/LF yoktur. Sayılar
Qt compact JSON gösterimidir; bu yerel versioned format RFC 8785 iddiası taşımaz.
Int64/UInt64 string olduğu için 53-bit sayı yuvarlaması yoktur. Field array sırası
payload'ın parçasıdır; object alan sırası hash'i etkilemez.

RESULT'tan `v,type,sessionId,payloadHash` çıkarılarak canonical payload üretilir.
`SHA256(payload)` lowercase hex hash'tir. Oturumdan bağımsız payload result/request/
cycle/config/input kimliklerini ve tüm terminal/field verisini içerir. sealResult
yalnız ilk hazırlıkta hash üretir; resultPayload hash'i yeniden doğrulayarak byte
döndürür. Adaptör bu byte/hash'i kalıcı outbox'a bir kez yazmalı; retry/recovery
kayıtlı byte'ları kullanmalı, canlı sonuçtan yeniden hesaplamamalıdır. Transport
zarfına yeni sessionId eklenmesi payload hash'ini değiştirmez. Socket write
başarısı consumer ACK değildir; ACK'in exact resultId/hash eşleşmesi ve kalıcı
commit'i DeliveryStore/adaptör tarafından yapılır.
