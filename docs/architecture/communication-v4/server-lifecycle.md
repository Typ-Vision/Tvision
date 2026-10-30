# COMM4-05 — Process listener ve station Run sınırı

Uygulanan C profili. `CommunicationServerLifecycle` process composition'unda
bir kez oluşturulur; Qt application thread'inde çalışır. `src/app/main.cpp`
başlangıçta bu composition'u oluşturur. Güncel UI ve station owner bağlantısı
[COMM4-16 workspace](communication-workspace.md) içindedir.

## Profil ve ömür

Makine dosyası `QStandardPaths::AppLocalDataLocation` altında
`communication-server-v4.json`'dır. Smoke testleri kendi build fixture kökündeki
`v4-machine/` dizinini kullanır. Eksik profil NOT_CONFIGURED; dosya oluşturulmaz.
Profil `CommunicationServerProfiles::save` ile açıkça yazılır; mevcut v2/v3 INI
ve JSON dosyalarından türetilmez, otomatik yükseltilmez. Örnek:

```json
{"schema":"visionflow.communication-server.v4/1","enabled":true,"address":"127.0.0.1","port":5000,"peerPolicy":"single-trusted-local-network","parserDeadlineMs":1000,"ackTimeoutMs":2000,"retries":3,"maxStations":32,"maxQueuedResponses":64,"writeHighWaterBytes":65536,"maxReadBatch":4}
```

JSON en çok16 KiB; alan türleri/aralıkları doğrulanır. Port1–65535 ve numeric
IP zorunludur; port0/fallback yoktur. Enabled geçerli profil constructor'da
listen eder; disabled profil ağ açmaz. Bozuk profil veya bind conflict açık
Fault/error'dur; normal süreç başlangıcında `qCritical` tanısı yazılır. Profil
değişimi için süreç kapatılır; Run/Stop/reçete seçimi profili yeniden yüklemez.

`CommunicationListenerOwnership` process'te tek v4 composition'u seçer.
Seçilmiş v4 varken legacy v2/v3 listen açık hata verir; v4 bind hatası da sessiz
legacy fallback açmaz. Önceden dinleyen legacy varsa v4 seçimi reddedilir,
legacy dinleme değişmez. V4 seçilmediğinde eski v2/v3 fleet/ACK davranışı korunur.
Dosya migration veya eski pending ledger temizleme yapılmaz.

Bir peer socket'i ve bir pending connection sınırı vardır; ikinci peer mevcut
peer'i değiştiremez. Socket read buffer64 KiB'dır. Listener generation bir kez
oluşturulur, station Run/Auto/Stop veya activation A/B ile değişmez.
`peer()` codec/session katmanının bağlanma portudur;05 tek başına byte okumaz,
wire cevap üretmez.07 [session portunu](peer-session.md) normal main'de bağlar.
Profilde parser/ACK ve
kuyruk bütçelerinin bulunması bu mekanizmaların uygulanmış olduğu anlamına gelmez.

## Station ve kapanış

`StationRunController`, COMM4-04'ün bağımsız Runtime/coordinator çiftine bağlanır.
Başlangıç yalnız stopped/drained çift kabul eder; Auto process-local false'dur.
Run immutable snapshot'ı offline Load edip mevcut kamera hazırlığını ister;
hazırlık tamamlanınca ARMED_MANUAL olur. Auto ayrı açık komuttur. Stop Auto'yu
önce kapatır, mevcut coordinator'ın generation fence/cancel/drain yoluna gider.
Controller ve listener birbirini çağırmaz. Aktif çalışma, kayıt retry ve pending
delivery mevcut Runtime bariyerlerini korur. Station admission/remote expected
activation yarışı ve worker dispatch08/08a/09'dadır; burada TRIGGER endpoint'i yoktur.

`addStation` en çok profil limitine kadar bağımsız controller kaydeder; duplicate
ve çalışan çift reddedilir. Composition sahibi explicit shutdown tamamlanmadan
controller'ları yok etmez. `beginShutdown(0..60000 ms)` tüm station'larda admission'ı
kapatıp Stop ister; `pollShutdown` nonblocking drain kontrolüdür. Bu arada listener
ve peer açık kalır. Drained veya deadline'da atomic shutdown checkpoint denenir,
sonra socket/listener kapanır. Timeout unresolved işi tamamlanmış göstermez;
recoveryRequired kaydı station/activation/mode/pending/drained bilgisini taşır.
Checkpoint yazma hatası görünür error/recovery'dir; kapanışı sonsuza kadar tutmaz.

Normal main kapanışı2s bütçeyle bu portları sürer. Runtime terminal/begin ve
delivery depoları otoritedir; shutdown dosyası ikinci ledger değildir. Restart
checkpoint'ten Run/Auto/production snapshot restore etmez; yalnız enabled
profille yeni listener generation açar. Hard crash ve güç kesilmesinde yeni
checkpoint garanti edilmez; mevcut Runtime recovery kayıtları korunur.

V4 station Runtime proje/worker portu09, wire STATUS/ACK portları06/07/13'tedir;
normal main bunları16'da bağlar. Bozuk migration seçimi veya legacy rollback
startup error ile listener'ı Fault'ta tutar; profil/pointer dosyaları karışmaz.
Bu karttaki localhost kontrol erişimi
transport ve application-state kabulüdür; PLC wire/fiziksel kabulü değildir.
