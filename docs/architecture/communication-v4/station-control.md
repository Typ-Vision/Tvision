# COMM4-08a — Station kontrol portu

`StationControlService`, SELECT_RECIPE/SET_AUTO/STOP ile yerel select/setAuto/stop
komutlarını aynı `StationActivationService` ve `StationRunController` üzerinden
yürütür. Controller gerçek `RuntimeCoordinator` hazırlık/Stop/drain yolunu kullanır;
UI, socket veya donanım SDK'sı içermez. Peer session handler'ı kind4/5/6 ve kontrol
ACK11'i buraya yönlendirir; üretim/kalibrasyon ACK'i ilgili ledger adapter'ına
bırakılır. Handler yoksa veya manifest/durable port yoksa üretim izni açılmaz.

## Kimlik ve immutable seçim

Remote Enable yalnız yerel porttan gelir; başlangıçta kapalıdır. Session exact
HELLO yetkisini denetler; servis ayrıca installation ve tek kayıtlı controlling
peer/epoch'u doğrular. Station binding numeric key/UUID ile exact'tır. Header'daki
recipe/template/activation mevcut snapshot'a eşleşir; ilk seçimde üçü de0'dır.
Hatalı activation/revision4, Remote Enable kapalı25 döner.

`SavedRecipes` yalnız kayıtlı immutable recipe revision + template manifest'inden
beslenmelidir. Editor draft veya request body reçete içeriği kaynağı değildir.
Aktif manifest'in revision ve canonical recipe bytes'ı yeniden denetlenir.
`StationActivationService` kayıtlı recipe/camera kimliklerini, configuration ve
idle/drained CAS'ı doğrular. SELECT_RECIPE yeni activation üretir; Auto kapalıdır.
Socket/session değişmez. A→B→A üç farklı activation'dır.

## Hazırlık ve yerel yarış

SET_AUTO(1) durable intent'ten sonra kaynak hazırlığını başlatır; ACCEPTED READY
değildir. Owner event loop `poll()` çağırır; gerçek Coordinator ancak hazırsa
terminal execution1/resultValid1/reason0/commandState3/stationReady1 mühürlenir.
Auto bundan sonra açılır. Eksik kaynak/hazırlık hatası READY vermez. SET_AUTO(0)
admission'ı kapatır; kamera drain başlatmaz. STOP admission'ı hemen kapatır ve
terminali gerçek drain'e kadar bekletir. Eski üretim sonucu/ACK bariyeri korunur.

Controller'ın `controlGeneration` değeri Run/Auto/Stop'ta değişir. Yerel Stop,
Auto disable veya Remote Enable disable geç hazırlık promotion'ını fence eder;
gecikmiş poll Auto'yu yeniden açmaz. Yerel Auto da aynı hazırlık portunu ve
generation kontrolünü kullanır. Local Stop durable store yokken de kullanılabilir.
Kontrol state'i owner thread'dedir; en çok32 station bind edilir.

## Durable port ve normal/control rezervi

`StationControlStore` zorunlu porttur; bu kart gerçek dosya ledger'ı oluşturmaz.
COMM4-13 adapter'ı aşağıdaki koşulları aynı station ledger'ında sağlamalıdır:

- `find`: installation+peer+epoch+station+request anahtarını bütün command kind'leri
  boyunca arar. Original request ve fingerprint ayrı kalır; SELECT terminalinin
  yeni activation header'ı eski fingerprint'in yerine geçmez.
- `reserve`: ACCEPTED'dan ve application etkisinden önce intent+terminal headroom
  atomik kalıcıdır. Normal üretim/örnek/kontrol komutları ortak normal1 rezervini,
  yalnız STOP/ABORT control1 rezervini kullanır. Ret kayıt yaratmaz. İkinci farklı
  STOP rezerv doluyken23 döner; admission yine kapanır, ACCEPTED uydurulmaz.
- `seal`: result UUID, canonical terminal digest ve exact response header/body
  bir kez atomik kaydedilir. Başarılı seal olmadan remote Auto açılmaz.
- `acknowledge`: yalnız doğrulanmış exact station/peer/epoch/request/recipe/
  template/activation/resultUUID/digest kaydını idempotent kapatır. Control ACK
  üretim rezervini veya başka sonucu temizlemez.

Store işlemleri event pump veya application callback çalıştırmaz. Read/write/
ambiguous commit/seal/ACK hatası station'ı fence eder ve Stop'a gider; kayıt
silinmez, acquisition/reprepare otomatik tekrarlanmaz. Recovery önce durable
intent/activation history tutarlılığını çözmelidir. Restart servisinin eski intent'i
yeniden yürütmesi yasaktır. Exact geç ACK storage fence sırasında da işlenebilir.

Aynı komutun retry'ı güncel Auto/activation'dan önce durable kaydı arar; yalnız
session zarfı yenilenir. SELECT terminal ACK'i yeni header'a eşleşir. Aynı key
farklı içerik6'dır. Sealed terminal exact command retry ile porttan okunabilir;
normal main event-loop/manifest/worker composition ve proaktif terminal send/retry
scheduler'ı burada bağlanmaz. Bunlar09/13/14/16 entegrasyonlarının sorumluluğudur.

`station_control_contract`, fake shared ledger ve gerçek Image File Runtime ile
bu portları sınar. Bu fixture disk/crash dayanıklılığı veya fiziksel PLC/kamera
üretim kabulü değildir. Native OECS correlation/drain kapısı değişmez.
