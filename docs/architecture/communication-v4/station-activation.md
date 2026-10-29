# COMM4-04 — Station aktivasyon portu

Kalıcı seçim `StationDefinition`, `StationRecipeSelection`, `StationSnapshot`
değerleriyle tutulur. Station UUID/key, recipe UUID/revision/key ve camera UUID/key
mevcut installation identity registry'den doğrulanır; retired veya yanlış kategori
kabul edilmez. Kaynak binding listesi enabled acquisition tool kümesiyle tam eşleşir;
industrial binding reçetedeki camera UUID ile de eşleşir. File/webcam fixture binding'i
fiziksel cihaz veya native üretim uygunluğu kanıtı değildir.

`StationActivationService::activate` owner thread'de son station activation UUID'sini
beklenen UUID ile karşılaştırır. İlk seçim null expected ister. Trusted
`StationActivationBoundary` idle/drained denetimi ve configuration-only doğrulama
yapar; sonra yeni UUID ve catalogue CAS ile tek append commit edilir. Başarısız
CAS/configuration/persistence eski seçimi korur. Farklı station son seçimi değişmez;
A→B→A daima üç ayrı activation taşır. Aynı recipe revision'ın içeriği yeniden
yazılamaz. Editor seçimi bu servisi kendiliğinden çağırmaz; snapshot değer kopyası
ve `shared_ptr<const ...>` kullanır.

`FileStationActivationStore` schema1: SHA-256(32 byte) + Qt6.0 sürümüne sabitlenmiş,
big-endian QDataStream payload. Magic `visionflow.station-activations`; installation,
catalogue revision ve append-only snapshot history içerir. Reçetenin bütün mevcut
alanları, calibrationBinding, kaynak kümesi ve absolute project directory saklanır.
16 MiB payload,32 station ve4096 activation sınırı açık hata verir; history silinmez.
QLockFile altında revision CAS ve QSaveFile atomic replacement vardır. Eksik/bozuk
dosya restore sırasında yaratılmaz/onarılmaz. SHA checksum kimlik doğrulama veya
eski backup rollback tespiti değildir; filesystem güç kesilmesi garantisi verilmez.

`StationRuntimeBoundary` mevcut RuntimeService/RuntimeCoordinator çiftini station'a
bağlar; aynı Runtime iki station'a atanamaz ve yanlış coordinator/service çifti
reddedilir. Çalışan cycle, preparation, camera drain, source verification veya kayıt
retry varken seçim kapalıdır. Pending delivery seçim kapısı değildir; eski terminal
ve coordinator delivery owner bariyeri korunur. `load(snapshot)` mevcut offline
Coordinator Load yoludur, Run/Auto açmaz; legacy Load pending delivery varken yine
reddeder. Seçim ve Runtime Load iki açık adımdır; seçim commit'i eski loaded Runtime'ı
değiştirmez. Bu köprü UI'ye bağlanmış yeni ürün akışı değildir.

Mode/health görünümü Stopped/Preparing/ArmedManual/Running/Draining/Blocked durumlarını
mevcut Runtime'dan salt okunur türetir. ArmedAuto domain'de tanımlıdır; v4 Auto
denetleyicisi sonraki karttadır. Restore yalnız seçim geçmişini okur; yeni Runtime
Stopped kalır, production snapshot/Run/Auto izni geri yüklenmez. Pending terminalin
exact ACK ledger'ı COMM4-13'ün sorumluluğudur; burada ikinci ledger oluşturulmaz.

Kabul `station_activation_contract`: dört farklı station/recipe/camera; gerçek
Runtime active cycle/Stop drain; A/B/A ve stale CAS; gerçek kalıcı RESULT_PENDING
terminal byte/metadata koruması; configuration/lock/corruption/history/limit ve
restart/owner thread sınırları. UI, listener, socket, kamera arbitrajı ve eşzamanlı
worker dispatch bu kartta uygulanmaz.
