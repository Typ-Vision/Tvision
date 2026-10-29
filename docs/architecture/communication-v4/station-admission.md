# COMM4-08 — Station admission portu

`StationAdmissionService`, owner thread üzerinde station başına tek atomic
admission uygular. HELLO/exact session otoritesinden sonra
`CommunicationPeerSessionService::setTriggerHandler` station-aware portuna
`V4RequestIdentity` ile TRIGGER yönlendirir. Handler bağlanmamışsa NOT_READY;
normal main henüz station manifest/ledger/worker composition'ını bağlamaz.

## Snapshot ve bariyerler

Application manifest provider'ı immutable `StationSnapshot`, template numeric
key, camera-set revision ve varsa input snapshot/revision + immutable typed
`InputSnapshot` değerlerini sağlar. Bunlar ağ isteğinden türetilmez. Input shape,
quality ve freshness provider'ın sorumluluğudur; kimlikleri tek başına değer
üretmez. Girdi yoksa iki UUID ve değer pointer'ı boş olmalıdır. V4 için yeni
remote input yazma protokolü açılmaz.

Request station UUID/key, recipe key, activation, template key ve üç body UUID'si
exact karşılaştırılır. Eksik manifest NOT_READY, uyuşmazlık STALE_CONFIG'dir.
Stopped/Auto kapalı PLC için NOT_AUTO; Preparing/Draining/Blocked veya hazır
olmayan kaynak için NOT_READY. Manual origin ArmedManual'da aynı portu kullanır.
Recording pending `RECORDING_ERROR/PENDING_STORAGE`, pending delivery
`STATION_BUSY/PENDING_DELIVERY` döner. Bunlar frozen enum'a yeni değer eklemez;
detail alanı neden ayrımını korur. Fiziksel/SDK kapısı resource portunda kalır.

`reserve` worker/kamera/terminal headroom için all-or-none lease verir; callback
sonrasında snapshot ve bariyerler yeniden okunur. Stop veya revision değişimi
kabulü iptal eder, lease bırakılır. Station admitting guard'ı callback içinden
yeniden girişte ikinci tetik kabulünü önler. Farklı station guard/aktif kayıtları
ayrıdır; global 32 aktif station sınırında RESOURCE_LIMIT vardır. Queue yoktur.

## Durable intent ve yürütme sınırı

`StationAdmissionStore::find/commit` zorunlu application portudur. `find` mutable
snapshot/mode'dan önce çalışır: accepted key aynı fingerprint ise durable ACCEPTED
veya sealed TERMINAL tekrar edilir; farklı içerik CONFLICT. Session yalnız yeni
cevap zarfına yazılır. Transient ret intent yazmaz; aynı key sonra denenebilir.

`commit` intent + terminal headroom'u kalıcı olarak ayırmadan dönemez; event
pumping/control callback yapamaz. Ancak bundan sonra `begin` çağrılır. Commit
hatasında acquisition0, station PENDING_STORAGE ile fence edilir; belirsiz write
otomatik yeniden capture edilmez. Begin/dispatch hatasında RECOVERY_REQUIRED,
aktif lease ve accepted identity korunur. Durable kayıt mevcutsa duplicate
accepted durumunu okur; yeni iş oluşturmaz. Recovery, saklı intent durumundan
hareket etmelidir. Bu kart kalıcı journal/ACK ledger uygulamaz; COMM4-13 portun
disk adapter'ını sağlayacaktır. Test store'u yalnız port davranışı fixture'ıdır.

`release` yalnız exact identity/fingerprint/header'a ait doğrulanmış sealed
terminal ve gerçek drain varsa station lease'ini bırakır. ACK/delivery bariyeri
ayrıdır; bu çağrı pending terminali silmez. Stale/başka completion, yalnız
accepted kayıt veya drain sürerken release etkisizdir.

`StationRuntimeAdmissionBoundary` mevcut `StationRuntimeBoundary` binding'ini
ve `StationRunController` mode/activation'ını kullanır; recording ve delivery
durumunu gerçek Coordinator'den okur. İzinli begin aynı Coordinator.begin
yolundadır; worker scheduling, kaynak arbitrajı ve manifest sağlayıcıları explicit
callback'tir. Bağımsız motor kopyası veya yeni hardware driver yoktur. Eksik
callback'ler hazır kabul edilmez. Provider/adapter/service ömrünü composition
sahibi yönetir; session callback'i servis yok edilmeden kaldırılmalıdır.

`station_admission_contract` deterministik storage/resource/reentrancy/Stop/revision
yarışları ve gerçek Runtime begin/Stop/drain fixture'ını kapsar. Bu yazılım sınırı
normal main v4 production, fiziksel kamera/PLC/robot veya tam concurrent worker
kabulü değildir. Sonraki kart COMM4-08a'dır.
