# COMM4-06 — Binary codec ve bounded taşıma portları

`src/application/CommunicationV4Codec.*`, Qt Core üzerinde network/Widgets,
Runtime veya registry bağımlılığı olmayan wire shape katmanıdır. CMake hedefi
`visionflow_projects`; test hedefi `communication_v4_codec_test`, CTest adı
`communication_v4_codec`. Listener'a bağlama ve session authority COMM4-07'dedir.

`CommunicationV4Frame` typed header ve exact384/896 byte body taşır. Checked
uint32/int32/uint64/Float64 erişimleri BE kullanır; UUID `to/fromRfc4122` kullanır.
Uint64 JSON/Float64/signed64 üzerinden geçmez. Body oluşturucu sıfır tamponda
atanmış alanları yazar; encode da decode ile aynı strict shape denetimini yapar.
Header magic/version/direction/kind/size/flags/reserved, komut body tail'leri,
closed enum/mask alanları, metadata category1..13/schemaVersion1 ve tüm padding
denetlenir. UTF-8 decoder stateless strict kullanılır; yarım multibyte, surrogate,
overlong ve geçersiz byte dizileri kabul edilmez. Number signed32 tüm bit
desenlerini kayıpsız taşır; missing slot kanonik sıfırdır. Pose finite/norm/frame
kimlikleri/convention/reserved denetlenir. Terminal digest doğrulanırken alınan
pose tekrar normalize edilmez; publication canonical pose üretme sorumluluğu
sonraki typed publication katmanındadır. `putFloat64` nonfinite reddeder ve -0'ı
+0'a yazar. Gelen sealed byte'lar decode/encode sırasında korunur.

W7 fingerprint ve digest fonksiyonları prefix NUL dahil exact slice'ları kullanır;
transport session hariçtir. `terminalDigest` sealing için sıfırlanmış digest
alanıyla hesaplar; ACKED saklı digest'i echo eder, yeniden seal edilmez.

## Framing ve write sözleşmesi

`CommunicationV4Framer` direction explicit512/1024 bekler. `feed` read<=2048,
receive<=4096, dönüş<=4 frame ile sınırlıdır. İlk128 byte hazır olunca header
denetlenir; untrusted size allocation büyütmez, magic taraması/resync yoktur.
Hatalar terminaldir; peer kapatıldıktan sonra `reset` yeni bağlantıya başlar.
Adapter sessizlikte de monoton `tick` çağırır. İlk partial byte zamanı sonraki
trickle ile yenilenmez; 2000ms'de gelen final byte da reddedilir. Tam frame'in
zamanı final byte gelişinde sabitlenir; dispatch bekleme süresi partial sayılmaz.
Tam frame arkasındaki partial tail kendi ilk geliş zamanını korur.

`CommunicationV4WriteQueue` response'ları64 frame/65536 byte ile sınırlar.
`enqueue=false` açık saturation'dır; accepted iş veya durable kayıt düşürme izni
değildir. Tek socket sahibi `pendingBytes(socket.bytesToWrite())` ile yalnız
baş frame'in credit kadar kalan byte'ını alır; `write` dönüşünün pozitif sayısını
`consumeWritten` ile ilerletir. Sıfır write ilerletmez, negatif write disconnect
gerektirir. Bir frame bitmeden diğer frame sunulmaz. Socket high-water ve queue
backpressure portları vardır; adapter bunları read/admission kapısına bağlar.
Durable retry, fair station scheduling ve reconnect ledger bu kartta yoktur.

## Fixture sınırları

Frozen manifest'in82 frame'i byte-exact korunur:55 valid frame,17 shape-negative,
10 authority/değer yorumlama vektörü. Stale session/activation, yanlış ACK,
bilinmeyen peer ve pose revision değerleri biçim olarak geçerlidir; codec registry
veya ledger otoritesi yerine fixture kimliklerini hardcode etmez.

`bad_signed_word_order` fixture generator'ı değişmiş Number için digest'i yeniden
seal eder ve frozen outcome açıkça `decoded 32768, not INT_MIN` der. Bu byte dizisi
geçerli signed32 `32768`'dir; codec bunu INT_MIN olarak yorumlamaz. Keyfi geçerli
Number byte değişimini, değiştirilmiş digest de geçerliyken, sender niyetini
bilmeden reddetmek mümkün değildir. Header endian reversal reddedilir;
seal edilmemiş Number değişimi digest hatasıyla reddedilir. UUID mixed-endian
değişimi de başka opaque UUID'dir, bilinen peer eşleşmesini COMM4-07 denetler.
Bu ayrım wire freeze'i değiştirmez veya domain yetkisini codec'e taşımaz.

Testler her valid golden frame için byte-byte stream, trigger'ın511 split'i,
coalesced4request, deadline/tick/clock/fatal reset, header/read cap, partial write,
64 frame saturation ve20000 deterministik fuzz girdisini kapsar. Yazılım fixture
kabulü fiziksel PLC/robot uyumluluğu değildir. Son koşum/kanıt
[kapanış kaydındadır](../../archive/2026-10-29-comm4-06.md).
