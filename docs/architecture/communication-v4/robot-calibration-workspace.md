# COMM4-24 — Robot kalibrasyon operatör workspace'i

Tek Communication sayfasındaki `RobotCalibrationPanel`, application katmanındaki
`RobotCalibrationWorkspace` immutable projection/command portunu tüketir.
Planar `CalibrationWizard/Overview` kendi mm/plaka işini korur; hand-eye oturumu
ve active seçimi bu planar model seçimiyle birleştirilmez.

## Application sınırı

Projection station UUID, mount mode ve monoton revision taşır. Provider, sample,
readiness, dependency veya model değişiminde revision'ı ilerletmelidir. BEGIN,
SAMPLE solve/holdout, SOLVE, VALIDATE, ACTIVATE, ABORT ve explicit Recovery aynı
owner-thread komut yolundadır. Komut anında application portu snapshot'ı yeniden
okur; session, candidate UUID/revision, validation UUID ve active revision
uyuşmazlığında handler çağrılmadan STALE_CONFIG verir. Handler servislerin kendi
kalıcı dedup/lease/dependency/physical gate denetimlerini aynen tüketmelidir.
Projection bir production izni veya donanım yetkisi değildir.

`Read` ve `Execute` application binding portlarıdır; widget bunların içine
solver, SDK veya pose oluşturucu yerleştirmez. Application composition bu portları
COMM4-20 session/lease, COMM4-21 capture coordinator ve COMM4-22/23 uygun
solver/validation/activation adapterlarına bağlar. Geçerli physical adapter henüz
olmadığından normal main binding'i unavailable projection verir; BEGIN/SAMPLE/
SOLVE/VALIDATE/ACTIVATE üretimde açılmaz. Fixture binding sadece kabul testindedir.
UI'da fixture/physical-ready anahtarı, robot hareket komutu veya pose editörü yoktur.

Actual pose eksikken iki SAMPLE rolü de açık nedenle kapalıdır. Reconnect/recovery
ve gerçek drain devam ederken yeni işler kapanır; uygulamanın sunduğu ABORT ve
Recovery kullanılabilir. ABORT ilerlemesi application snapshot'ından gelir;
widget süreye göre tamamlanma veya lease release uydurmaz. Mode, açık session
sırasında salt okunur; terminal ABORT/FAILED sonrasında tekrar seçilebilir.

## Operatör görünümü

İki mount, frame/TCP/lens readiness, actual-pose durumu, lease sahibi, exact
session/candidate/validation/active kimlikleri ayrı gösterilir. Sample tablosu
UUID, solve/holdout rolü, frame UUID, durum, reject nedeni ve pairing/token
metnini taşır. Uzun hücrelerde tooltip exact metni korur. Disabled nedeni tooltip,
accessible description ve her zaman okunabilen metinde vardır. Dikey scroll,
klavye Tab ve küçük viewport kabulü mevcuttur. Communication station tablosu
korunur; farklı station seçimi production komutu/lease oluşturmaz.

## Doğrulama ve sınırlar

`robot_calibration_workspace_contract` UI OFF/ON'da fiziksel gate, actual pose,
recovery/drain, exact stale identity, adapter yokluğu ve owner-thread sınırını
sınar. `ui_robot_calibration_workflow`, iki committed golden mount fixture'ında
gerçek BEGIN, PARK candidate solve, independent holdout/reference validation,
fixture CAS activate ve gerçek session ABORT/drain servislerini çalıştırır.
Candidate/validation active seçimi değiştirmez; physical activation reddedilir.
Golden sample/frame çiftleri live capture gibi sunulmaz; token yokluğu açık yazılır.
SAMPLE'ın gerçek capture/pairing kabulü `calibration_sample_coordinator_contract`
hedefinde korunur; UI actual pose yokken capture dispatch etmediğini ayrıca sınar.

Yeni yazılım ve görsel kabul [kapanış kaydında](../../archive/2026-11-02-comm4-24.md).
Windows Qt native 1400×900 ve 850×600 logical viewport, DPR1.25 PNG'leri gerçek
native rendering kanıtıdır; WGC helper app-approval timeout'u nedeniyle OS input
injection/WGC capture kabulü açık kalır. Fiziksel 6D/native acquisition/lens/frame/
TCP/reference gate BLOCKED'dur; fixture ACTIVE production izni oluşturmaz.
