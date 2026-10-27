# TCP v3 yerel kullanım

Communication → TCP v3 profilleri: enabled `server` profili, açık adres/port,
100–60000 ms ACK timeout ve 0–10 retry kaydedin. Connect/Test yalnız bağlantı
denemesidir; Run öncesi Disconnect seçin. V2 dinlemesini durdurun.

Editor'da çoklu variable → Set Variables → Publish Data ile tek etkin output
group hazırlayın; logical field mapping profilinde bulunmalıdır. Üç applicationType
aynı yolu kullanır. Communication v2 sekmesindeki reçete tetik modunu
`SoftwareTrigger` seçip Save yapın. Load Recipe → gerekirse Kaynağı doğrula.
Runtime'da **Run ile TCP v3 profilini aç** seçin ve peer ile mutabık kaldığınız
benzersiz namespace generation kimliğini yazın. **Run** yüklü sürümün profilini
açar; draft/selected recipe, Load veya Editor preview bağlantı açmaz.

Bu kapı tek TCP sunucu/grup ve dış typed alan gerektirmeyen INPUT snapshot'ı
kullanır. İstemci rolü ve çoklu hedef bu düğmeyle etkinleştirilmez; temel fazın
çoklu hedef kartı COMM-36'dır. Peer kimliği namespace kimliğidir, authentication
iddiası değildir. Güvenilen ve konfigüre ağda kullanın.

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
