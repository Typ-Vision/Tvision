# P3 — Bağımsız TCP/IP socket ve vf.socket/2

8 Ekim 2026 P3/P4: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. Sonraki paket P5 Blob filtreleri/kırmızı gerçek bölge; K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026. Kullanıcı K4 fiziksel kalibrasyon kabulünü erteledi ve TCP/IP socket önceliğiyle PLC geliştirmesini yetkilendirdi. Bu teslim gerçek localhost socket → etkin Runtime → kalıcı sonuç → ACK yazılım kabulüdür. Fiziksel PLC, üretim ölçüm doğruluğu ve restart sonrası teslim kurtarma kabulü değildir.

## Rol ve sınırlar

VisionFlow TCP sunucusu, PLC/simülatör istemcidir. Communication mevcut ekranında yerel IP/port, ACK süresi ve etkin snapshot'taki Measurement sonuç kaynağı seçilir. Varsayılan `127.0.0.1:5000`; port 0 geçici port seçer. IP sayısal IPv4/IPv6 olmalıdır. Tek peer ve tek outstanding istek vardır; ikinci bağlı peer reddedilir. Socket asenkron Qt Network üzerinde owner thread'dedir; edinim/algoritma immutable Runtime snapshot ile worker'da çalışır. UI socket veya donanım API'si çağırmaz.

Dinleme proje/reçete gerektirmez. Sonradan RuntimeService/coordinator bind edilir; proje değişimi pending cycle/ACK yokken unbind eder ve bağlantı/session korunur. Load/Run/Stop salt listening tarafından engellenmez. Stop cooperative token’ı iptal eder ve yeni kabulü kapatır; pending terminal kayıt/DONE/ACK silinmez. Editor/kamera worker’ı acquisition kaynağını kullanırken yeni socket kabulü kapanır.

Communication enabled/autoStart/bind address/port/ACK timeout/retries yerel settings INI’sinde kalır. Yoksa autoStart kapalıdır; açılış asla otomatik Run yapmaz. Hatalı IP/port dolu açık hata verir, kullanıcı tekrar dinlemeyi başlatır. Reçete definition.runtime {trigger: Manual|SoftwareTrigger, measurementToolId: UUID veya boş} normal Save/DRAFT/sürüm yolunu kullanır; eski kayıt varsayılan Manual ve mapping yoktur. Ekranda düzenlenen draft, loaded snapshot mapping’ini değiştirmez. TLS/remote principal/vendor wire/Modbus/OPC UA/robot uygulanmadı.

## Wire sözleşmesi — vf.socket/2

UTF-8 JSON nesnesi + LF. TCP paket sınırları mesaj sınırı değildir; parçalı ve birleşik mesajlar ayrıştırılır. Her mesajda tam sayı `v:2`, `type`, `sessionId` vardır. Bağlantıda VisionFlow `HELLO` gönderir. Session ID her açık dinleme oturumuna ait UUID'dir; reconnect aynı ID'yi korur, stop/start veya process restart yeni ID üretir. Eski session komutları reddedilir.

| Yön / tip | Alan ve davranış |
| --- | --- |
| Vision → HELLO / STATUS | state, ready, busy, outstanding requestId, etkin recipeId/configId, measurementToolId; bunlar parça sonucu değildir |
| PLC → STATUS | Yalnız v, type, sessionId; acquisition yapmaz |
| PLC → TRIGGER | v, type, sessionId, requestId, recipeId, configId; etkin immutable snapshot kimliği birebir uyuşmalıdır |
| Vision → BUSY | requestId; duplicate çalışırken recordingPending de gelebilir |
| Vision → DONE | requestId, cycleId, recipeId/configId, executionState, error, measurementToolId, resultValid, decision, valueMm |
| PLC → ACK | v, type, sessionId, requestId; aynı isteğin terminal sonucunu kabul eder |
| Vision → ACKED | requestId; ACK kalıcı yazıldı, outstanding engeli kaldırıldı |
| Vision → INVALID | requestId, reason, resultValid=false, decision=NOT_EVALUATED, valueMm=null; cycle/frame üretilmez |
| Vision → ERROR | requestId (varsa), code; ERROR bir PASS/FAIL parça kararı değildir |

PLC komutlarında eksik/ekstra alan ve yanlış tür reddedilir. requestId ASCII harf, sayı veya `_.:-`, 1–64 karakterdir; recipeId/configId aynı token biçimini veya boş metni kabul eder; configId etkin SHA-256 hex kimliğidir. HELLO'dan etkin kimlikler okunabilir; production PLC bunları beklenen yapılandırmayla eşleştirmelidir.

HELLO/STATUS recipeId yüklü reçeteyi gösterir; yoksa boş metindir. configId yalnız Running aktif snapshot varsa doludur, aksi halde boş metindir. Geçerli yeni istek: NO_RECIPE_LOADED, NOT_RUNNING, NOT_READY (Starting/Stopping/shared acquisition), TRIGGER_SOURCE_DISABLED, RECORDING_PENDING, DELIVERY_PENDING, RUNTIME_FAULT, DEPENDENCY_INVALID, MAPPING_INVALID veya SESSION_FULL için INVALID. Worker sürerken BUSY; yeni acquisition yoktur. Schema/yanlış session/config/conflict ERROR; v1/future sürüm UNSUPPORTED_VERSION. Accepted duplicate kontrolü bu yeni kabul koşullarından önce çalışır.

Örnek TRIGGER (kimlikler HELLO'dan gelir):

```json
{"v":2,"type":"TRIGGER","sessionId":"<oturum-uuid>","requestId":"part-0001","recipeId":"<etkin-recete-uuid>","configId":"<64-hex-config>"}
```

DONE, execution terminal mesajıdır; `executionState=DONE` tek başına PASS değildir. Seçilmiş enabled Measurement gerçek DONE kararı ve mm sonucu taşıyorsa resultValid=true, decision=PASS/FAIL, valueMm ham mm'dir. Tolerans ham değerden mevcut engine tarafından hesaplanır. Mapping yoksa veya cycle ERROR/SKIPPED ise resultValid=false, decision=NOT_EVALUATED, valueMm=null. Genel recipe AND/OR, Match skorundan yapay parça kararı, robot pose veya register/byte-order mapping yoktur. `resultValid` veri geçerliliğidir; K4 fiziksel kabulü ifade etmez.

## Tekrar istek, ACK ve yeniden bağlantı

- Dedup anahtarı `(sessionId, requestId)`; aynı ID'nin farklı recipe/config içeriği REQUEST_CONFLICT üretir. Busy duplicate mevcut BUSY; terminal duplicate aynı DONE döndürür. ACK edilmiş istek de yeniden edinim yapmaz. ACKED cevabı kaybolursa ACK tekrarlanabilir.
- Aynı anda yeni requestId, çalışan worker varsa BUSY; kayıt/ACK varken INVALID / RECORDING_PENDING veya DELIVERY_PENDING olur. Eski isteğin ACK'i başka outstanding isteği temizlemez.
- ACK timeout varsayılan 2000 ms; UI 100–60000 ms. Servis retryCount varsayılan 3 (API 0–10). Yalnız aynı DONE yeniden gönderilir; görüntü veya algoritma yeniden çalışmaz. Bütçe dolunca ACK_TIMEOUT engeli korunur; geç ACK veya reconnect kabul edilir.
- Bağlantı kopması running işi durdurmaz. Sonuç kaydedilir, aynı dinleme oturumuna yeniden bağlanınca HELLO ve pending DONE döner. Yeniden bağlantı ACK retry bütçesini başlatır. Bağlantı kapandı diye request otomatik yeni cycle'a çevrilmez.
- Oturum en çok **128 accepted request ID** tutar; eviction yoktur. Limitte SESSION_FULL / INVALID ve yalnız eski sonuç replay/ACK çalışır. Tüm pending tamamlandıktan sonra operatör açıkça stop/start yaparak yeni oturum başlatır. Bu ilk yazılım sınırıdır; sürekli üretim/kalıcı dedup sonraki iştir.
- Giriş satırı en çok 2048 byte; partial frame 2000 ms içinde tamamlanmazsa peer kapatılır. Socket read buffer 8192 byte, çıkış kuyruğu/mesaj 65536 byte, diagnostics son 128 satırdır. Timeout algoritma/SDK hard-interrupt garantisi değildir.

## Kalıcı kanıt ve hata politikası

`ProductionStore events` içinde hash/atomik/append-only PLC_REQUEST intent, PLC_RESULT ve PLC_ACK tutulur. Önce request intent, ardından CYCLE_BEGIN yazılır; başarısız begin sonuçsuz intent bırakabilir. Terminal cycle schema 2'ye optional communication nesnesi (protocol/session/request/mapping) eklenir; iletişim istendiyse immutable cycle `communicationState=RESULT_PENDING`, manuel cycle NOT_REQUESTED kalır. CYCLE_BEGIN de aynı ilişkiyi taşır. Recipe/root/engine şeması değişmedi.

Runtime.finish terminal cycle'ı yazmadan ve PLC_RESULT event commit olmadan DONE dışarı çıkmaz. Hata RECORDING_PENDING olarak tutulur; UI Kaydı tekrar yaz aynı cycle/result/event ID ile retry yapar, yeniden acquisition yoktur. PLC_ACK event commit olmadan ACKED/Ready verilmez; başarısız ACK aynı komutla tekrar denenir. Socket write/flush teslim kanıtı değildir; mantıksal teslim kanıtı kalıcı ACK'tir. Logs son 128 cycle ve son 256 event'i ilişkilendirir; ACK event bu pencerede yoksa ACK alınmadığını kesin iddia etmez. Diskte immutable cycle ACK durumuyla overwrite edilmez.

Arşiv önceki 512 MiB/10000 kayıt toplam sınırını paylaşır; otomatik retention yoktur. Oturum dedup/result cache RAM'dedir; process restart kanıtı korur fakat ayar varsa dinleme geri gelir; izin/aktif snapshot/unfinished delivery otomatik geri gelmez. Çökme sonrası belirsiz isteği yeni session'a yeni TRIGGER olarak otomatik taşımak desteklenmez. Kalıcı dedup/outbox/recovery ve fiziksel PLC wire eşleştirmesi **6D.2** kapsamıdır. Restart dahil exactly-once garantisi verilmez.

## Simülatör ve kabul

Önce güncel `build/k1-ui/visionflow.exe` açın; Communication’da enabled/endpoint seçip dinlemeyi başlatın. Reçetede SoftwareTrigger/Measurement seçip Save → Load Recipe → Run kullanın. Simülatör sahte vision sonucu üretmez; gerçek etkin Runtime çevrimini ister.

```powershell
$env:PATH = 'C:\Qt\6.12.0\mingw_64\bin;C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
.\build\k1-ui\visionflow_plc_simulator.exe --port 5000
.\build\k1-ui\visionflow_plc_simulator.exe --port 5000 --duplicate --reconnect --ack-delay 3000
.\build\k1-ui\visionflow_plc_simulator.exe --port 5000 --drop-ack
.\build\k1-ui\visionflow_plc_simulator.exe --port 5000 --resume
```

Port 0 seçildiğinde ekrandaki actual port kullanılır. `--status-only` acquisition yapmaz; `--request` belirli request ID verir. `--drop-ack` 10 saniye gözlemleyip outstanding sonucu bırakır; `--resume` yeni TRIGGER göndermeden pending DONE'u ACK eder. `--reconnect` BUSY sonrası aynı session/request ile reconnect eder; session değişmişse otomatik tekrar çalıştırmayı reddeder. CLI deadline 20 saniye; exit 0 tamamlandı, 2 argüman/protokol, 3 sunucu INVALID/ERROR/session, 4 socket, 5 deadline. Normal 0 vision PASS demek değildir; actual FAIL veya ERROR sonucu da ACK edilebilir.

Release UI 20/20, Widgets OFF 18/18 CTest. TCP test: actual PNG → Edge → 9.975 mm PASS ve ham sınırla DONE/FAIL; duplicate/reconnect/ACK timeout, kayıt/ACK lock retry, cancel, source error, kurulum invalidation, framing/schema/session reddi. UI smoke ayrı simülatör process'iyle duplicate+reconnect+geç ACK ve tek cycle, Logs ACKED, close/manual guard ve 850×600 görüntülerini doğrular. Localhost connect sandbox tarafından engellendiği için ağ testleri yalnız yerel testler için izinli sandbox dışı çalıştırıldı. Fiziksel PLC/kamera/üretim kabulü, kurulum/dağıtım/CUDA kanıtı yoktur.

P3/P4 güncel yazılım kabulü: Release UI 22/22, Widgets OFF 20/20; P3 bağımsız autoStart/Load/Run/Stop/INVALID/rebind, port collision, settings persistence, mode Save/reopen ve UUID mapping de sınandı. Önceki 6D.1 sayıları tarihsel kayıttır; fiziksel/restart teslim kurtarma kapsamı değişmedi.


## COMM-17 — explicit TCP v3 development servisi

`CommunicationTcpEndpoint` ayrı Qt Network listener'ıdır; normal Communication
UI/v2 listener'ı v3'e geçirmez. Konstruktor/Editor/Load ağ açmaz; geliştirme
entegrasyonu `listen(CommunicationTcpOptions)` çağırır. Runtime, coordinator,
ortak TriggerRouter/InputSnapshotStore ve DeliveryStore aynı owner thread'de
yaşar; yalnız immutable Runtime lease QtConcurrent worker'ına taşınır. Endpoint
ve peer kimliği açık machine-local konfigürasyondur, authentication iddiası yoktur.
Port çakışması hata; sessiz fallback/downgrade yoktur.

INPUT yalnız konfigüre input adlarını ve yetkili enum kataloğunu kabul eder;
exact snapshot/revision/config, type/unit/frame/quality ve local monotonic age
kontrolü router'dadır. Geçerli admission'ın son aşamasında REQUEST_INTENT ve
ayrılmış cycle UUID ile CYCLE_BEGIN kalıcılaşır; ardından coordinator.begin ve
worker başlar. Terminal communication origin input kimliğini/revision'ı taşır.
V3 kayıtları Result/Publish olmasa da boş, NOT_EVALUATED terminal10 bundle alır.

RESULT, commit edilmiş terminal bundle'ın seçili logical output group'undan
hazırlanıp canonical payload/hash olarak outbox'a bir kez kaydedilir. Terminal
veya DELIVERY_PENDING commit'i yoksa socket RESULT yoktur. `retryRecording()`
aynı completion/terminal/payload'ı yazar; acquisition çalıştırmaz. Duplicate ve
reconnect ve restart, kalıcı request anahtarından kayıtlı payload'ı okur. Her bağlantının
sessionId'si yenidir; eski session reddedilir, kalıcı request anahtarı değişmez.
ACK exact request/resultId/hash eşleşir; ACK_RECEIVED + ACK_COMMITTED olmadan
ACKED veya teslim bariyerinin kalkması yoktur. Stop admission'ı kapatıp cooperative
cancel ister; pending kayıt/delivery silinmez. Disconnect/queued write Ready
oluşturmaz. Bounded read ve 50 ms framer tick codec deadline'ını uygular.

Local simulator yeni seçenekleri:

```powershell
.\build\kmc9-core-off\visionflow_plc_simulator.exe --v3 --port 5001 --input snapshot.json --expect-decision OK --expect-field count --expect-value 1
```

INPUT dosyası `snapshotId,revision,fields,sourceTime,clockId` taşır; fields strict
v3 typed envelope'dur. Simulator v/session/type/recipe/config'i HELLO'dan doldurur,
`--sequence` (varsayılan 1) ve `--request` seçilebilir. Beklenen karar/alan/JSON
literal değeri uyuşmazsa exit 3; gerçek Runtime sonucu üretir, sahte vision sonucu
oluşturmaz. `--duplicate`, `--reconnect`, `--drop-ack`, `--ack-delay` kullanılabilir.
V3 simulator `--resume` kapalıdır; uygulama servisinde explicit recovery aşağıdadır.
Timeout/retry politikası aşağıdaki COMM-19 sözleşmesindedir; normal kullanıcı aktivasyonu COMM-23 kabulündedir. Mevcut v2
komutları ve wire alanları korunur. Test `communication_tcp_runtime` gerçek
localhost Runtime/bundle/outbox/ACK ve ayrı v2 profilini sınar; fiziksel kabul değildir.

## COMM-18 — Explicit v3 restart/recovery

`CommunicationTcpEndpoint::listen()` v3 ledger/outbox'ı namespace
(workcell/project/endpoint/peer) için tarar; constructor/Load ağ veya edinim açmaz.
Startup Running/Ready izni oluşturmaz. Pending delivery coordinator bariyerini
kurar; terminali olmayan intent/begin veya bozuk journal RECOVERY_REQUIRED tutar.
Yeni session ile aynı stable request/fingerprint duplicate'i INPUT veya yeni
cycle olmadan aynı canonical payload/resultId/hash'i döndürür. Farklı fingerprint
REQUEST_CONFLICT; v2 geçmişi v3 ledger olarak yorumlanmaz.

Owner-thread `inspectRecovery()` kayıtları salt okunur sunar.
`resumeDelivery(exactTriggerRequest)` açık servis komutudur: terminal10 + bundle
commit edilmiş ama DELIVERY_PENDING eksikse cycle ve communication/config/input
kimliklerini doğrular, frozen terminalden payload'ı üretip outbox'a commit eder.
Yeni acquisition/prepare/Run yoktur. Terminali olmayan, unknown veya foreign
request reconciliation ister; sentetik terminal veya ACK oluşturulmaz.
ACK_RECEIVED kalıntısı yalnız mevcut durable receipt üzerinden ACK_COMMITTED'a
ilerletilebilir. ACK_COMMITTED replay acquisition/consumer effect yaratmaz;
exact tekrar ACK idempotent ACKED verir. Bir ACK diğer unresolved request'in
bariyerini kaldırmaz. Bu API normal UI'ya bağlı değildir; repair UI COMM-22,
automatic timeout/retry aşağıdaki COMM-19 kapsamındadır; fiziksel kabul COMM-38'dir.

`delivery_recovery_contract` ayrı child process'i intent, begin, terminal/outbox
arası, pending, ACK_RECEIVED ve ACK_COMMITTED sınırlarında gerçekten sonlandırır;
yeni Runtime ile edinimsiz recovery ve localhost byte replay'i doğrular.
Genel crash dayanıklılığı veya uçtan uca exactly-once garantisi değildir.

## COMM-19 — Bounded v3 retry ve namespace

Explicit `CommunicationTcpOptions` required/default=true, ackTimeoutMs/default=2000
(100–60000), retryCount/default=3 (0–10) taşır. Monoton QElapsedTimer deadline'ı
50 ms owner-thread poll ile denetlenir. İlk RESULT + en çok retryCount otomatik
tekrar; retry aynı persisted canonical bytes/resultId/hash'i kullanır. Tükenince
ACK_TIMEOUT/pending korunur; geç ACK commit kabul edilir. Offline attempt
tüketmez; yeni bağlantı fresh session + yeni retry bütçesiyle pending RESULT'u
gönderir. Yeni cycle/acquisition başlatmaz. Optional backlog bir seferde tek
RESULT yollar; ACK commit sonrası sıradaki pending'e geçer.

Required offline/unresolved admission kapalıdır; optional endpoint başına 128
unresolved request sınırında QUEUE_FULL verir. ACK tombstone'ları RAM kotasını
tüketmez: v3 sürekli ACK'li istekler 128 session sınırına takılmaz. V2'nin yukarıdaki
128/session davranışı aynıdır. Shared disk 10.000 kayıt/512 MiB/20 MiB tek dosya;
admission 32 kayıt/32 MiB + her unresolved request için 8 kayıt/1 MiB baş boşluğu
ister. Bu preflight'tır; sonraki disk hatası veri silmez, kayıt/recovery kapısını korur.

Tombstone/retirement otomatik silinmez. `DeliveryStore::rolloverNamespace` drained
eski namespace, yeni/hiç kullanılmamış peer generation ve kayıtlı peer agreement
ister; eski namespace restart sonrası da NAMESPACE_RETIRED olur. Yeni generation
açık listen gerektirir. Rollover bütçeyi sıfırlamaz veya kayıt silmez; archive taşıma/
retention operasyonu bu API'de yoktur. Normal profil UI'sı COMM-20, diagnostics
COMM-22, birleşik TCP kabulü COMM-23 ve fiziksel kabul COMM-38'dir.
