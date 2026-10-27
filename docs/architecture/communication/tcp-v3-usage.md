# TCP v3 yerel kullanım

Communication → TCP v3 profilleri: enabled `server` profili, açık adres/port,
100–60000 ms ACK timeout ve 0–10 retry kaydedin. Connect/Test yalnız bağlantı
denemesidir; Run öncesi Disconnect seçin. V2 dinlemesini durdurun.

Editor'da çoklu variable → Set Variables → Publish Data ile output
gruplarını hazırlayın; her grup ayrı enabled TCP server profiline bağlanır,
logical field mapping profilinde bulunmalıdır. Üç applicationType
aynı yolu kullanır. Communication v2 sekmesindeki reçete tetik modunu
`SoftwareTrigger` seçip Save yapın. Load Recipe → gerekirse Kaynağı doğrula.
Runtime'da **Run ile TCP v3 profilini aç** seçin ve peer ile mutabık kaldığınız
benzersiz namespace generation kimliğini yazın. **Run** yüklü sürümün profilini
açar; draft/selected recipe, Load veya Editor preview bağlantı açmaz.

Run tüm etkin grupların TCP sunucularını açar; her profile tek grup bağlanır.
Etkin Publish Data sırasındaki ilk grup tetik girişidir; diğer bağlantılar yalnız
RESULT/ACK teslimidir, TRIGGER için TRIGGER_SOURCE_DISABLED döner. Aynı peer
generation metni endpoint UUID ile ayrı namespace oluşturur. Required hedeflerin
tümü bağlı ve önceki teslimleri ACK_COMMITTED olmadan yeni çevrim başlamaz.
Optional bağlantı kesilse de en çok 128 pending bundle tutulur; limitte QUEUE_FULL
döner. Her hedef aynı cycle/bundle kimliğini, kendi request/hash/ACK durumunu
taşır; ikincil requestId cycle UUID'dir. Bir hedefin ACK'i diğerini tamamlamaz.
Restart yalnız açık Run/listen izniyle kalıcı pending veriyi tekrar yollar;
ACK_COMMITTED hedef yeniden çevrim üretmez.

UI dış typed alan gerektirmeyen INPUT snapshot'ı kullanır; istemci rolünü açmaz.
Genel Pose tek typed SpatialTransform record'udur: translation mm,
right-handed column vector T_target_source ve quaternion-wxyz; frame UUID ve
revision korunur. Mapping alanın tamamını taşır; x/y/z/rx/ry/rz veya örtük Euler
projection desteklenmez. Simulator Pose verisi yeni gerçek Runtime pose producer
veya robot hareket izni oluşturmaz. Peer namespace kimliği authentication iddiası
değildir. Güvenilen ve konfigüre ağda kullanın.

Örnek `input.json` (HELLO recipe/config/session kimliklerini simulator ekler):

```json
{"snapshotId":"part-input-1","revision":"1","fields":[],"sourceTime":"fixture","clockId":"fixture"}
```

README toolchain PATH'iyle, ayrı terminalde (kendi port/alan/değerinizi yazın):

```powershell
.\build\reference-ui\visionflow_plc_simulator.exe --v3 --port 5001 --input input.json --expect-decision OK --expect-field length --expect-value 1
```

Simulator INPUT → TRIGGER → RESULT → tam request/result/hash ACK gönderir.
Communication → Trigger / Delivery kalıcı request/cycle/result, payload hash,
attempt ve ACK_COMMITTED durumunu gösterir. Retry aynı kalıcı payload'ı gönderir;
ölçüm/edinim tekrarlanmaz. NOK geçerli false olabilir; ACKED parça kararı değildir.

Stop yeni tetik kabulünü kapatır, mevcut teslimi silmez; listener ACK/recovery için
kalır. Drained durumda Runtime **V3 kapat** eski listener'ı kapatır.
Bekleyen kayıt için Runtime **Kaydı tekrar yaz**, teslim için diagnostics Retry
kullanılır. Terminali olmayan RECOVERY_REQUIRED operatör uzlaştırması ister.
Restart v3 seçimini/Run iznini geri yüklemez. Eski peer generation/request key'lerini
yeniden kullanmayın; tombstone'lar kalıcıdır. Fiziksel PLC/robot/kamera/mm kabulü
bu simulator örneğiyle kapanmaz.
