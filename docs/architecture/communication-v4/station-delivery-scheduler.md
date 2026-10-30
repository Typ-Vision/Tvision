# Station delivery scheduler

COMM4-14 application portu: `StationDeliveryScheduler.hpp/.cpp`.

Ledger tek pending otoritesidir. Scheduler yalnız owner thread üzerinde sealed
terminali okur; intent/capture/seal/ACK üretmez. Yetki predicate'i her taramada
PeerSessionService üzerinden session/peer/epoch/contract kontrolü yapar.
ACK/RECOVER application routing'i `setDeliveryLedger` ile bağlanır. Ledger ve
session service bağlı socket'lerden uzun yaşamalıdır; ledger bağlantıdan önce
atanır. Normal main station/worker/manifest composition'ı sonraki kartlardadır.

StationKey başına FIFO, station'lar arasında frame sınırında round-robin vardır.
Global kuyruk64 response/65536 byte; socket bytesToWrite dahil65536 byte bütçe
korunur. En çok dört request'in yanıtına yer ayrılmadan read/admission yapılmaz.
Partial frame seçildikten sonra offset bitene kadar başka frame seçilmez.
Yazılamayan terminal disk üzerinde kalır; kuyruk doluluğu silent eviction yapmaz.
Aktif partial frame sırasında yeni HELLO splice yerine disconnect gerektirir.

Retry saati bütün frame socket'e kabul edildiğinde başlar; bu teslim/ACK değildir.
2000ms arayla ilk gönderime ek üç tekrar vardır. Son tekrarın2000ms ACK süresi de
biterse `timedOut(key)` alarmı kalır; ACK pending'i kapatır. RAM'de bekleyen/yavaş
socket'e yazılamayan frame retry bütçesini tüketmez. Alarm diğer station'ların
frame'lerini engellemez. Alarm scheduler diagnostics portudur; UI alarm sunumu
COMM4-16 composition kapsamındadır.

Reconnect yeni scheduler/session ve yeni deneme bütçesi açar; saklı body/digest
ve stable header aynıdır. Eski session ve retired epoch yetki kazanmaz. Yeni
HELLO eski transport kuyruğunu fence eder; unresolved işler diskten recovery
ister. Scheduler process içi retry sayacını kalıcılaştırmaz; pending ve exact ACK
journal'ı kalıcıdır. Exactly-once consumer effect iddiası yoktur.

`station_delivery_scheduler_contract`: byte başına partial write/fairness,
S1 ACK beklerken S2/S3 ve control terminali,2s/3retry/sticky alarm,32 station64
normal/control kayıt, socket+RAM highwater, partial disconnect/replay; gerçek
localhost HELLO/reconnect/stale/retired fence ve160 STATUS'lu yavaş okuyucu.
[Kabul kaydı](../../archive/2026-10-30-comm4-14.md).
