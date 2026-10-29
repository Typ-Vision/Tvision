# COMM4-09 — Bağımsız istasyon yürütmesi

`StationExecutionRegistry` aynı `RuntimeService::execute` motorunu kullanır;
her station ayrı RuntimeService/RuntimeCoordinator ve immutable ProductionCycle
taşır. Üç applicationType için ikinci motor veya kopya ürün yoktur.

## Composition ve sınırlar

Registry, owner thread'de station UUID → Coordinator, catalog path, completion
sink ve isteğe bağlı acquisition factory bağlar. Bir Coordinator iki station'a
bağlanamaz; station sayısı en fazla32, worker limiti1–32'dir (varsayılan3).
Runtime/Coordinator registry'den uzun yaşar. Run/Auto/Stop mevcut
`StationRunController` portlarında kalır.

`StationRuntimeAdmissionBoundary` reserve callback'i registry `reserve` metoduna,
dispatch callback'i `dispatch` metoduna bağlanır. Kameralar için COMM4-10'da
aynı all-or-none rezervasyona fiziksel lease eklenecektir. Registry tek başına
kamera paylaşımı izni vermez; native production kapılarını değiştirmez.
Birleşik lease worker permit'ini ve kamera sahipliğini birlikte tutmalı; dispatch
permit adresini doğruladığı için `shared_ptr` aliasing ile bu adres korunabilir.

Reserve station başına bir permit ve ortak bounded worker kapasitesi ayırır.
Aynı station BUSY(2), dolu worker bütçesi RESOURCE_LIMIT(23), bağlanmamış context
NOT_READY(20) döner. İstek kabulünden önce kapasite yoksa Coordinator.begin veya
capture başlamaz. İptal edilen admission'ın RAII permit'i kapasiteyi bırakır.
Kalıcı intent commit'i mevcut StationAdmissionService'e aittir; dispatch bu
bariyerden sonra çağrılır. Thread pool'a kapasiteyi aşan gizli üretim kuyruğu yoktur.

Worker yalnız ProductionCycle, catalog, cancel token ve source factory kopyasını
yakalar. Recipe, publication/variable dependency, frame/cycle ve completion
authority mevcut immutable Runtime snapshot'ından gelir. Coordinator veya UI
worker'a taşınmaz. `poll` owner thread'de hazır future'ları istasyon bazında alır,
trusted cancellation suppression uygular ve aynı Coordinator.finish ile kaydeder.

## Completion, Stop ve hata

Completion sink exact admission intent, immutable cycle ve Runtime sonucunu alır;
station/request/activation kökenini korumalı ve retry için idempotent olmalıdır.
Bu port yeni v4 disk ledger veya publication evaluator değildir (COMM4-12/13).
Persist başarısızsa yalnız o station'ın frozen recording retry'ı ilerler; capture
yenilenmez. Sink başarısızsa commit edilmiş Runtime.finish tekrar çağrılmaz;
aynı sonuç sink'e yeniden verilir. Diğer station'lar poll edilmeye devam eder.
Permit ancak kayıt ve sink başarıyla bitince bırakılır; unresolved durumlarda
kapasitenin korunması bilinçli ve sınırlıdır. Accepted admission slotu ise mevcut
exact terminal/drain denetimiyle ayrıca release edilir; pending ACK bariyeri kalır.

Stop yalnız hedef Coordinator'ın cancel token'ını kapatır. Worker dönmeden Run
yeniden açılamaz. Geç frame başarılı karar/değer olarak yayımlanmaz; immutable
cycle'ın cancelled terminali eski request'e aittir. Registry callback'i event
queue'da `this` yakalamaz; emekli olmuş job tekrar poll edilmez. Runtime ticket ve
snapshot authority eski/başka completion'ı reddeder.

Worker authority üretmeden beklenmedik exception verirse sahte ERROR/PASS
üretilmez: station recovery hatası ve permit korunur. Normal shutdown önce
controller Stop, gerçek worker/camera drain ve owner poll ister; destructor cancel
edip worker'ları bekler, başarı terminali oluşturmaz. Bounded process checkpoint
ve durable recovery COMM4-05/13 composition sorumluluğudur.

Normal main manifest/ledger/event-loop composition henüz bağlanmaz. Fixture
factory fiziksel/native üretim capability'si açmaz. UI değişikliği yoktur.

## Kabul

`station_execution_contract`: aynı peer session'da üç ayrı fixture camera adapter'ı
kontrollü bariyere eşzamanlı girer; enter/leave nanosaniye timeline'ı test logunda
yer alır. Aynı station BUSY; dördüncü station bütçe doluyken RESOURCE_LIMIT, boşalınca
aynı kod yolundan yürür. Ayrı recipe/source/frame/variable/OK-NOK; Stop sırasında
geç frame, S1 acquisition error'ının S2/S3'ü durdurmaması; kayıt kilidi ve terminal
sink retry izolasyonu; duplicate, owner-thread, stale completion ve gerçek drain
sınanır. Fixture yazılım kabulüdür; fiziksel kamera/PLC/robot kabulü değildir.
