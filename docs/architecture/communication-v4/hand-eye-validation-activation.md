# COMM4-23 — Bağımsız validation ve atomik seçim

Bu kartın `HandEyeValidationService` ve `HandEyeActivationService` portları,
22'deki açık sentetik fixture adapterını tamamlar. Fiziksel actual-pose/PnP,
native acquisition ve ölçübilim referansı uygun olmadığı için normal main/wire
composition açılmaz. `activateProduction` koşulsuz `PHYSICAL_GATE_BLOCKED`
döndürür. Planar `ScopedActiveCalibration` robot aktivasyon portu olarak
kullanılmaz; eski planar seçim ve izinleri değişmez. UI24 ayrı sorumluluktur.

## Immutable rapor

Solver her yeni adaya UUID ve ayrı immutable candidateRevision verir.
Validation, archive'dan exact candidate/revision/session kaydını okur; aday ve
training/dependency SHA256 bütünlüğünü denetler. Solve verisiyle sample veya
acquisition-frame UUID kesişimi, duplicate ve holdout olmayan rol reddedilir.
5–256 holdout gerekir; hiçbir örnek sessizce çıkarılmaz. Plate kalite politikası
18'in tek `FixturePolicyV1` kaynağından gelir.

Bağımsız referans her sample için açık base/flange_T_plate transformu taşır;
referans revision/source ve tolerance revision zorunludur. Fixture testinin
referansı candidate.fixedTransform yerine committed golden.fixedTruth'tur.
Fiziksel ölçümde bağımsızlık/izlenebilirlik, belirsizlik ve tolerans onayı için
ayrı authoritative producer gerekir; serbest DTO bunları kanıtlamaz.

`G=base_T_flange`, `C=camera_T_plate`, `X` dondurulmuş candidate olmak üzere
eye-in-hand `G X C`, eye-to-hand `inv(G) X C` hesaplanır. Holdout yeniden fit
edilmez. Her sonuç hem bağımsız referansla hem dondurulmuş training fixed Y/Z
ile ayrı karşılaştırılır. Translation Euclidean mm, rotation quaternion
geodesic `2*atan2(norm(vector(q_ref^-1*q_est)),abs(dot(q_ref,q_est)))` rad'dır;
bu trace/acos açısına eşdeğer olup sıfır yakınında hassasiyet kaybını azaltır.
Max, RMS ve nearest-rank p95 `ceil(0.95*n)-1` iki hata kümesi için raporlanır.
Bütün mm/rad max/RMS/p95 sınırları finite ve pozitif olmalı; eşitlik kabul
edilir. Training closure küçükken bağımsız referans hatası büyük olabilir.

Rapor; candidate digest/revision, training digest, full dependency snapshot,
holdout ve bağımsız referans satırları/digest, tolerance/digest, örnek hataları,
max/RMS/p95 ve açık sonucu taşır. Tolerans FAIL raporu da immutable archive'a
girer; aktivasyon kabulü sağlamaz. Yapısal retler reason ile döner. Archive
başarısızlığında accepted=false. `ProductionStore/hand_eye_validation` ortak
512MiB/10000-record kapasitesine dahildir; overwrite reddedilir.
Sentetik rapor `SOFTWARE_VERIFIED`, `physicalEligible=false`,
`productionPermitted=false` durumunu korur.

## Activation transaction

`HandEyeActivationBoundary::withExclusiveSnapshot` station admission, camera
drain ve configuration değişimlerini **callback boyunca disk commit dahil**
dışlayan application portudur. Tek bir snapshot okumak bu portun uygulanması
değildir. Kartın test boundary'si mutex kullanır; fiziksel normal composition
kurulmamıştır. Ürün bağlantısı kurulmadan gerçek station/lease authority ve
reference/tolerance registry bu portu sağlamalıdır.

Service ayrıca process'ler arası `QLockFile` ile CAS seçimini serileştirir.
Idle+drained, exact station/session/current candidate/revision, current full
dependency digest, validated report/candidate digest, reference/tolerance
revision ve expectedActiveRevision tek transaction içinde denetlenir. İlk
seçimde expectedActiveRevision null olmalıdır; sonraki seçim yeni UUID üretir.
Geometri/revision değişimi eski validation'ı reddeder; optik/mount değişikliği
camera/intrinsic/acquisition revision'ını değiştirmelidir. A→B→A aynı revision'ı
yeniden kullanamaz. Display name digest'te yoktur, rename geometriyi değiştirmez.

Tek mutable kayıt `hand_eye_active/<station>.json`'dır. Hash'li manifest exact
candidate/report kimliği ve digestlerini, yeni/önceki activation revision'ını
birlikte tutar. QSaveFile direct-write fallback kapalıdır; yarım/tam staging
yazımı görünür active'ı değiştirmez, rename commit eski veya yeni kaydı seçer.
Precommit hata eski seçimi korur; commit sonrası notification hatası gerçekleşmiş
seçimi başarısız rollback gibi bildirmez. Child-process crash testleri open,
partial write, complete write, precommit ve postcommit sınırlarında hem ilk
seçimi hem mevcut active'ı kapsar; dead PID lock recovery ayrıca sınanır.
Bu test elektrik kesintisi veya dosya sistemi/disk arızası kabulü değildir.

`remembered` hash ve immutable candidate/report linklerini doğrulayarak salt
okunur geçmiş getirir. Current dependency veya üretim izni çıkarsamaz; restart
Run/Auto/lease açmaz. Eski candidate/report kaydı atomik seçimden önce zaten
append-only olduğundan çok dosyalı mutable transaction yoktur.

## Kanıt sınırı

Gerçek CTest: `hand_eye_validation_activation_contract` (ON/OFF). İki geometri,
sample/frame overlap, stale candidate/dependency/reference/tolerance/active,
bağımsız sistematik bias, ayrı max/RMS/p95 ve rotation sınırları, NaN, kalite,
immutable overwrite, eşzamanlı CAS ve child crash/recovery sınanır. Kanıt
`build/comm4/COMM4-23/`; son kabul sayıları tarihli kapanış kaydındadır.
Robot/controller/TCP/frame, lens/intrinsics/mount/plate, bağımsız fiziksel
referans/tolerans ve native correlation/drain engelleri **BLOCKED** kalır.
