# COMM4-13 — Kalıcı station terminal/ACK ledger

`StationDeliveryLedger`, mevcut `ProductionStore` içinde ayrı
`station_delivery` grubunu kullanır. Eski `cycles`, v3 `delivery` ve namespace
kayıtları değişmez/silinmez. Yeni format `visionflow.station-delivery/1`;
bilinmeyen format, bozuk envelope/hash veya eksik zincir admission'ı kapatır.
Bu adapter migration işlemi yapmaz; transactional migration COMM4-15'tedir.

## Kimlik ve atomik kayıt

Anahtar installation/peer/epoch/station UUID + decimal string uint64 request'tir.
Session anahtar veya fingerprint değildir; sayısal ID JSON double'a dönüşmez.
Original request512 byte, fingerprint ve response1024 byte ayrı saklanır.
SELECT_RECIPE terminalinin yeni activation/header'ı eski isteği değiştirmez.

Hash zincirli immutable kayıtlar `INTENT → BEGIN → TERMINAL → PENDING →
ACK_COMMITTED` sırasındadır. Her event deterministic UUID, previous hash ve
exact request/response içerir; mevcut checksum envelope ve QSaveFile atomic
commit kullanılır. INTENT ve BEGIN kabul/uygulama etkisinden önce yazılır.
Terminal+sealed bytes tek event'te atomiktir. PENDING yazısından önce kesinti
olsa da saklı TERMINAL replay/ACK edilebilir. Ayrı payload dosyası yoktur.

Her station normal1 ve yalnız STOP6/CALIB_ABORT25 için control1 rezerv taşır;
ACTIVATE24 normaldir. ACK olmadan farklı normal request BUSY, ikinci control
RESOURCE_LIMIT döner. Farklı command aynı key'i paylaşamaz. Unacked kayıtlar
epoch/peer değişse de slotu tutar; otomatik capture/retention/eviction yoktur.
32 unresolved station sınırı ve arşivin ortak512MiB/10000 record/20MiB file
bütçesi korunur. Admission kalan event'ler ve control terminal için konservatif
headroom denetler. Gerçek disk hatasında kalıcı intent kalır; başarı uydurulmaz.
İşlemler aynı lock ile sıralanır; callback/event pump/SDK/worker çağrılmaz.
Fault hook yalnız deterministic test injection içindir.

## Port composition ve recovery

`Admission` mevcut `StationAdmissionStore` portudur; admission intent artık
original request'i de taşır. `Control` mevcut `StationControlStore` portudur.
Kontrol servisi exact ACK frame'ini store'a geçirir; yeni adapter key-only ACK'i
reddeder. Diğer store'ların eski overload'ı kaynak uyumluluğu için korunur.

Bu process'in başarılı BEGIN'leri exact retry'da ACCEPTED replay edebilir.
Yeni ledger instance'ı terminali olmayan kaydı `RECOVERY_REQUIRED` reason9 /
phase12 döndürür. Admission/control bunu tanır ve işi yeniden başlatmaz.
Restart'ta eski BEGIN için `seal` da reddedilir; reconciliation açıkça gerekir.
Terminali olan kayıtta yeniden seal yalnız exact byte eşitliğiyle idempotent'tir.
ACK yalnız stationKey/request/peer/epoch/recipe/template/activation + result UUID
ve digest exact eşleşirse commit edilir. Duplicate exact ACK restart sonrası da
idempotent'tir; başka station veya normal/control slotunu serbest bırakmaz.

Peer session authorization'dan sonra control handler'ın `nullopt` döndürdüğü
ACK/RECOVER için `handle` bağlanabilir. ACKED saklı UUID/digest'i echo eder;
RECOVER terminali veya recovery STATE'i döndürür, intent yaratmaz. `replay`
yalnız session zarfını değiştirir; original sealed byte'lar disk üzerinde kalır.
Caller station barriers'a `pending(station)` bilgisini bağlar ve mevcut
StationExecutionRegistry completion sink'inde authoritative terminali `seal` eder.
Terminal verisini üretmek, Runtime/activation history reconciliation ve normal
main manifest/station/worker composition bu adapter'ın görevi değildir.
Main'de tüm v4 production akışı henüz etkin değildir; retry/fair scheduling14,
UI/composition sonraki kartlardadır. Yazılım fixture fiziksel kabul değildir.

`station_delivery_ledger_contract`: gerçek dosya store'u,100 duplicate admission,
restart capture0, her write öncesi/sonrası fault, torn journal, yazılamayan
terminal hedefi, uint64 sınırı, SELECT yeni activation, ayrı STOP/ABORT rezervi,
yanlış/exact ACK ve byte-stable replay. Yeni kabul kanıtı ve sınırlar
[kapanış kaydındadır](../../archive/2026-10-30-comm4-13.md).
