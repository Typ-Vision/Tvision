# V4 bağımsız PLC/robot simulator

COMM4-25, `00434634-0002-4000-8000-202610290001` frozen contract.
`scripts/communication_v4_simulator.py` yalnız Python standart kitaplığını kullanır;
üretim C++ codec'ini, fixture generator'ını veya Qt binding'ini çağırmaz. Request
512, response 1024 byte; big-endian uint64, RFC4122 UUID ve terminal SHA256
W7'den bağımsız uygulanır. Bu araç fiziksel robot hareketi göndermez.

## Gerçek listener'a bağlanma

Ürünün Communication → İstemciler → **Test profilini dışa aktar** çıktısı tercih edilen
yoldur (VF-05). Profil dinleme adresi/portu, installation, tek etkin peer/epoch ve
sözleşme revizyonunu taşır; credential değildir:

```powershell
python scripts/communication_v4_simulator.py --profile build/vf-05/client.json --log build/comm4/manual/status.jsonl
```

`--host/--port` ve `--peer/--epoch` verilmezse profil değerleri kullanılır; açık
değerler profil değerlerini geçersiz kılar (yalnız elle sınama). Profil yoksa yerel katalogda
önceden enrolled controlling peer/epoch ve enabled V4 listener gerekir; aşağıdaki
açık biçim bu elle sınama yoludur. Simulator kimlik/epoch oluşturmaz, discovery
üzerinden üretim yetkisi açmaz. Remote Enable operatör tarafından ayrıca açılır;
restart Auto başlatmaz.

```powershell
python scripts/communication_v4_simulator.py --host 127.0.0.1 --port 5000 --peer <enrolled-UUID> --epoch <enrolled-epoch-UUID> --log build/comm4/manual/status.jsonl
```

`--stations`, `--trigger` veya `--recover` verilmezse exact HELLO ve global STATUS
çalışır; acquisition yoktur. Eski
`visionflow_plc_simulator` v2/v3 executable'ı korunur; V4 açıkça bu Python CLI'dır.
Protocol auto-detect/downgrade yoktur. `--help` mevcut seçenekleri gösterir.

## VF-07 · Tek istasyonu tetikle ve teslimi oku

Communication'da istemci profilini dışa aktarın. Dashboard'da Yüzey A'yı yükleyip
**Otomatik üretimi başlat** eylemini kullanın. Normal tetik için Remote Enable
açılması gerekmez. Ayrı terminalde profili açıp gerçek istasyon adını/anahtarını görün:

```powershell
python scripts/communication_v4_simulator.py --profile client.json --stations --log stations.jsonl
python scripts/communication_v4_simulator.py --profile client.json --station "İstasyon 1" --trigger --expect-decision OK --log trigger.jsonl
```

`--station` katalogdaki tam ad veya gerçek wire anahtarıdır; görünen sıra numarası
varsayılmaz. İstemci exact katalog revision'ıyla station, recipe ve template
sayfalarını okur, STATUS hazır/busy/pending ve aktif seçimi denetler. Operatörün
her `--trigger` komutu en çok bir yeni üretim isteği gönderir. Önceden seçilmiş
örneğin beklenen kararı NOK ise `--expect-decision NOK` kullanın. Eşleşmeyen karar
veya resultValid=false kabulü başarısız yapar; geçerli terminal yine exact ACK'lenir.

Çıktıda ACCEPTED yalnız çevrimin kabulüdür; terminalde DONE/ERROR/CANCELED, karar,
**valid**, cameraKey, station/request, revision ve result/digest görünür. Teslim
önce AWAITING_ACK, yalnız exact ACKED yanıtıyla ACKED olur. Sayısal slotlar yalnız
geçerlilik biti açıksa ham signed32 wire değeri olarak gösterilir; bunlara örtük
mm/birim veya ölçek atanmaz. Ölçümün operatör kabulü VF-08 kapsamındadır.

Profilin yanındaki `.vf4-<peer>-<epoch>.json` dosyası kalıcı uint64 sayaç ve
çözülmemiş tetiği taşır. `--state yol.json` açık bir konum seçebilir; aynı
installation/peer/epoch için sonraki koşumlarda **aynı dosyayı koruyun**. İstek
ilk socket yazımından önce fsync/atomic replace ile kaydedilir; sayaç wrap yapmaz.
Eşzamanlı ikinci istemci state lock nedeniyle reddedilir. Crash sonrasında kalan
`.lock` dosyasını yalnız önceki sürecin kapandığı ve state'in korunduğu denetlendikten
sonra kaldırın; counter/intent dosyasını silerek belirsiz işi çözmeyin.

Timeout veya kopmada otomatik yeni tetik yoktur. Çözülmemiş isteği açıkça kurtarın:

```powershell
python scripts/communication_v4_simulator.py --profile client.json --recover --log recovery.jsonl
```

Kurtarma yeni HELLO session'ıyla **aynı station/request/body**'yi yeniden gönderir;
kalıcı sonuç aynı result UUID/digest ile gelir ve exact ACK edilir. Başarıyla ACKED
alınana kadar yeni `--trigger` kapalıdır. Bayat seçim reddinde katalog/operatör
seçimi gözden geçirilir; istemci kendi başına yeni üretim kimliği oluşturmaz.
Yanlış result/digest/selection taşıyan ACKED yerel intent'i kapatmaz.

Ürün kabulü, boş izole makinede gerçek MainWindow/Communication/Dashboard/Runtime
ve ayrı Python process'iyle çalışır; fixture server içermez:

```powershell
ctest --test-dir build/reference-ui --output-on-failure -R 'communication_v4_operator_contract|ui_station_tcp'
# Aynı kabulü normal başlatıcı üzerinden, kullanıcı verisine dokunmadan çalıştır:
$env:VISIONFLOW_TEST_ARTIFACTS='D:/TVision/build/vf-07/launcher'
.\VisionFlow.cmd -Vf07Acceptance
```

`-Vf07Acceptance` açık test seçeneğidir; normal başlatmayı değiştirmez. Test yeni
run dizininde istasyonu ürün dialoguyla oluşturur, yayımlanmış Yüzey A'yı Dashboard'da
yükler, panelden istemci kaydeder/profil dışa aktarır ve yerel Auto açar. Gerçek
dosya görüntüsü→Blob→Result ile beklenen OK/resultValid ve exact ACKED doğrulanır.
Bayat dört seçim alanı, yanlış ACK, timeout, kopma/aynı terminal kurtarma ve
restart'ta izinlerin kapalı olması da sınanır. `build/vf-07/ui/run-*/vf07-tcp/`
altında result.json/JSONL ve normal/küçük Dashboard görselleri saklanır. Bu yazılım
kanıtı fiziksel PLC/kamera veya robot hareketi kabulü oluşturmaz.

`--plan path.json` bir JSON işlem listesi okur. Her satır `kind`, beklenen
`expect_kind`, gerekiyorsa `station`, `recipe`, `template`, `activation` ve exact
384-byte `body_hex` taşır. `request` decimal string olarak explicit verilmelidir;
mutating komutlarda otomatik sıra atanmaz. Aynı peer/epoch altında kabul edilmiş
request kimliğini başka iş için kullanmayın; aynı request+body yalnız replay'dir.
Wrap yasaktır. `split` byte konumu ve `delay_ms` iki parça arasındaki süreyi seçer.
`ack: true` terminalin exact station/request/recipe/template/activation/result/hash
alanlarıyla ACK gönderir ve ACKED bekler. ACK alınmayan terminal pending kalır.

Sadece STATUS için örnek plan:

```json
[
  {"kind": 2, "request": "9007199254740993", "expect_kind": 102,
   "split": 127, "delay_ms": 10}
]
```

SELECT_RECIPE body offset0 recipeKey/offset4 templateKey; SET_AUTO body offset0=1;
TRIGGER body offset0 camera-set revision UUID'dir. Header her adımda güncel exact
activation/revision kullanır. SELECT terminali yeni activation döndürdüğünden
sonraki plan adımlarını eski activation ile kurmayın. Sabit plan formatı dynamic
değişken substitüsyonu yapmaz. Dinamik akış için Python `Peer` API'si ve
`tests/communication_v4_e2e.py` içindeki `catalogue`, `command`, `enable`, `trigger`
yardımcıları örnektir; `Peer.wait()` cevabın `raw`/`body` byte'larını döndürür.

Örneğin bir programda `Peer.connect()` sonrası kind3/body(category1,key) gönderip
CATALOG_REPLY body offset220 recipeKey,224 templateKey,232 activation,248 camera-set
revision alanlarını okuyabilirsiniz. Sayılar network order; UUID16 opaque byte.
Offsetlerin otoritesi [wire](wire.md), CLI planı wire şemasını genişletmez.

JSONL log TX kimliğini/byte uzunluğu/hash'ini, RX kind/station/request/reason,
result/digest, deadline/protocol hatası ve tamamlanma durumunu kaydeder. Hata exit
nonzero'dur; logdaki socket write ACK veya dış etkinin exactly-once kanıtı değildir.

## Otomatik fixture kabulü

```powershell
cmake --build build/reference-ui --target communication_v4_fixture_server communication_v4_robot_fixture -j 4
ctest --test-dir build/reference-ui --output-on-failure -R '^communication_v4_simulator_e2e$'
ctest --test-dir build/reference-ui --verbose -R 'station_execution_contract|station_delivery_ledger_contract|station_delivery_scheduler_contract|camera_lease_arbiter_contract|calibration_sample_coordinator_contract'
```

UI OFF için `build/kmc9-core-off` kullanılır. CMake BUILD_TESTING yalnız test
executable'larını üretir; Python3 Interpreter gerekir. Normal ürün main'inde
synthetic raster/dependency producer veya softwareFixture=true kurulmaz.
Calibration application callback'i ancak exact HELLO+station kimlik denetiminden
sonra çağrılır; normal main callback bağlamaz, calibration capability kapalıdır.

CTest yeni `comm4-25-e2e/run-*` dizini oluşturur. `result.json` artımlı 16 case
sonucunu, production/robot `peer.jsonl` hata/recovery akışlarını, `ready.json`
girdileri, machine manifest ve durable production/session kayıtlarını saklar.
Consumer subprocess effect dosyasını fsync ettikten sonra `os._exit(77)` ile
ACK'siz sonlanır; reconnect aynı terminali verir. Consumer dedup yoksa effect2
oluşması bilinçli olarak doğrulanır; exactly-once iddiası yoktur.

Production fixture dört station ve üç applicationType'ı gerçek
`CommunicationStationComposition`/Runtime/ledger üzerinden kurar; kaynaklar Image
File'dır. `pause-poll` ve `local-stop` yalnız fixture kontrol dosyalarıdır, wire
komutu veya normal ürün özelliği değildir. Üç işin terminalden önce kabulü burada,
gerçek acquisition overlap ise ayrı `station_execution_contract` bariyerinde aynı
TCP stream üzerinden ölçülür. Bu test ayrıca ortak kamera UUID/fiziksel lease
ile S2 CAMERA_BUSY/S3 ilerleme ve drain sonrası S2 retry davranışını doğrular.

Robot fixture iki mode için actual-pose SAMPLE/holdout, 100 duplicate başına
capture1, camera/sample/frame/origin echo, stale camera/frame/profile revision ve
ABORT başarısız drain→aynı komut retry→exact control ACK yolunu kullanır. BEGIN
mode'unun yerel dependency fixture'a seçimi, synthetic raster adapter ve
CALIB_ABORT'un kontrol ledger'ına bağlanması yalnız test executable'ındadır.
SOLVE/VALIDATE/ACTIVATE bu TCP fixture'da unsupported; mevcut iki-modlu solver ve
validation/activation CTestleri ayrı yazılım katmanı kanıtıdır.

`station_delivery_ledger_contract` her INTENT/BEGIN/TERMINAL/PENDING/ACK_COMMITTED
sınırının önce/sonrasında fault inject edip ledger'ı yeniden açar; yeni acquisition
yapmaz, terminal replay veya RECOVERY_REQUIRED denetler. Bu katman process kill
yerine write-boundary exception injection kullanır. Logdaki `FAULT_RECOVERY` JSON
satırları phase/record/replay bilgisini taşır. Gerçek process crash yalnız consumer
senaryosunda uygulanır. Codec her split konumu/coalesced/malicious sınırları,
scheduler gerçek yavaş-reader socket/backpressure/retry fairness'i ayrıca sınar.

## Sahiplik ve kapsam kanıtı

Mevcut kaynak `src/app/plc_simulator.cpp` v2/v3'tür. Yeni Python peer ve iki test
server'ı V4 aday simulator/fixture driver'dır. Ortak uygulama değişikliği yalnız
`CommunicationPeerSessionService` calibration application dispatch portudur;
auth/stale/unknown-station/throwing-adapter retleri peer testindedir. UI değişmez.

Graph D:/TVision generation2026-11-01T22:39:30Z: session/ledger/execution/session
sembolleri, `attachCommunicationPeer` inbound/outbound trace ve ilgili paths
coverage kontrol edildi. Watch kapalı; CMake ve değişmiş/new testler kaynak
fallback ile okundu. Composition line20 ve peer partial kapsamları doğrudan
kaynaktan tamamlandı. Graph'ın temiz sonucu eksiksizlik ispatı sayılmadı.

Fiziksel robot/controller actual6D, TCP/frame, lens/intrinsics, native OECS
cancel/drain/frame correlation ve bağımsız reference/tolerans kabulü **BLOCKED**.
Simulator/fixture gerçek accuracy, robot motion veya üretim izni vermez. Son
koşum sonuçlarının otoritesi [tarihli kapanış](../../archive/2026-11-02-comm4-25.md)
ve `build/comm4/COMM4-25/` kanıtıdır.
