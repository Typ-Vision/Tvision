# COMM4 hedef sistem sözleşmesi

**Hedef sistem.** Wire alt sözleşmesi [COMM4-02 ile donduruldu](freeze-comm4-02.md);
uygulama kartları sürer. Bu belge yeni davranışı tarif eder; v2/v3'ün bugün değiştiğini veya
robot üretim desteği bulunduğunu söylemez. Binary ayrıntılar [wire](wire.md),
kanıt kuralları [verification](verification.md) içindedir.

## S0 — Baseline, katman ve kapsam sınırı

Parent keşfinin kaynak çapaları: `src/application/RuntimeCoordinator.hpp`
tek loaded recipe ve RuntimeService bağı içerir;
`CommunicationTcpEndpoint.hpp` gerçek yolu
COMM4-01'de doğrulanmalıdır; mevcut endpoint Runtime/Coordinator/router/input/
delivery referansları ve outputGroup profiline bağlıdır. Aynı Runtime'ı hedeflere
bağlamak istasyon bağımsızlığı değildir. `PublicationService.hpp` capture,
dependency/current kontrolü ve evaluate yolu yeniden kullanım adayıdır.
Bu plan tam kaynak audit'i sayılmaz; parent'ın graph constructor trace'i generic
edge verdiği için çağrı zinciri kanıtı olarak kullanılamaz. COMM4-01 gerçek
dosyaları, graph coverage ve gerekli kaynak fallback'ini kaydeder.

Mevcut UI çapaları `src/app/MainWindowCommunication.cpp`, `PublishDataEditor.*`;
kalibrasyon adayları `CalibrationService/Workflow/Capture/StudyStore.*`,
`CalibrationWizard/Overview.*` dosyalarıdır. İçeriği COMM4-01 öncesi doğrulanmış
sayılmaz. Yeni domain uygulama servisine, UI yalnız bu servislere gider; UI
socket parser, OpenCV solve veya kamera SDK'sını doğrudan çağırmaz. Üç
applicationType tek Editor/Runtime altyapısını paylaşır; istasyon için kopya
ürün veya ayrı applicationType yaratılmaz.

## S1 — Sunucu yaşam döngüsü

Tek machine-local v4 server profili adres/port, enabled, peer policy, parser/ACK
süreleri ve bütçe taşır. Açılış, kayıtlı enabled ve geçerli profili listen eder;
ilk kurulumda eksik profil açık NOT_CONFIGURED'dır. Bind hatası alarmdır; port
değiştirip sessiz fallback yapılmaz. Process/project restore listen edebilir,
**Run/Auto/üretim izni restore etmez**. Sunucu LISTENING/PEER_CONNECTED olabilirken
tüm istasyonlar STOPPED olabilir. STATUS/ACK/recovery/calibration kontrol kanalı
üretim Run'ından bağımsızdır; her komut ayrıca kendi yetkisini denetler.

Run istasyonun immutable reçete ve kaynak kümesini hazırlar; Auto dış tetik
admission'ını açar. Durumlar STOPPED → PREPARING → ARMED_MANUAL veya
ARMED_AUTO; çalışma RUNNING, durma DRAINING, hata BLOCKED. Numeric durumlar wire W7'de sabittir. Manuel tek cycle aynı admission portunu kullanır. Auto kapalı PLC
TRIGGER NOT_AUTO döner; listen ve ACK sürer. Stop yeni admission'ı atomik kapatır,
çalışan acquisition/worker'ı belgeli drain/cancel ile bitirir, terminal ve pending
teslimi korur. Process kapanışı bounded drain, disk checkpoint ve socket close
yapar; süre aşımı recovery kaydıdır, sahte terminal başarı değildir.

Reçete A/B seçimi yalnız o istasyonun aktivasyonunu değiştirir, socket veya peer
session'ını değiştirmez. Aktif cycle eski snapshot ile tamamlanır; aktivasyon
ancak o istasyon idle/drained iken atomik yapılır. Pending eski sonuç exact ACK
ile teslim edilebilir; yeni reçete hazır olsa da o istasyonun unresolved delivery
bariyeri korunur. A→B→A eski activation revision/üretim iznini canlandırmaz.

Yerel Run/Auto/Stop ve PLC SELECT_RECIPE/SET_AUTO/STOP aynı application
portlarını kullanır. Remote Enable bir defalık makine ayarıdır; controlling
peer yetkisi gerekir. Yerel Stop/disable devam eden remote hazırlığı fence eder;
geç callback tekrar Auto açamaz. SELECT_RECIPE hedef istasyonu idle/drained iken
değiştirir, yeni activation döndürür ve Auto kapalı kalır. SET_AUTO(1) kamera ve
reçete hazırlığını tamamlayınca READY döndürür; tetik/çevrim başlatmaz. SET_AUTO(0)
admission kapatır, STOP gerçek drain ister. Bunlar robot motion/safety komutu
değildir. UI ve remote komut yarışı expected activation ile compare-and-swap;
başka istasyon etkilenmez. Pending sonuç sırasında STOP/ABORT engellenmez, yalnız
kontrol terminali için sınırlı ayrı kayıt rezervi kullanılır.

## S2 — Station sahipliği ve eşzamanlılık

Station kalıcı UUID ve never-reused numeric wire key, görünen ad, active recipe
UUID/revision, activation UUID, camera bindings, mode/health ve station ledger
taşır. En az üç, konfigüre limit dahil daha fazla station desteklenir. İlk sınır
32 station; frozen wire limiti32'dir. Wire stationKey=0 yalnız
global komutlardır. Başka station state'i UI seçiminden veya tek global loaded
recipe'den türetilmez. Editor selection active recipe yerine geçmez.

Bir station aynı anda bir üretim cycle kabul eder. Başka station bağımsız kamera,
worker ve bütçe varsa aynı bağlantıdan eşzamanlı çalışır. Router kimlik, generation,
mode, exact activation, pending delivery, kayıt bütçesi ve kaynak iznini tek atomik
admission adımında denetler. Accepted'dan önce kalıcı request intent gerekir.
Başka station BUSY/ACK_TIMEOUT veya kalibrasyon lease yüzünden global durmaz;
gerçek paylaşılan storage/worker sınırının etkisi açık RESOURCE_LIMIT'dir.

Kamera UUID'si sahiplik anahtarıdır. Bağımsız kameralar eşzamanlıdır. Paylaşılan
kamerada üretim cycle veya calibration capture için exclusive acquisition lease;
gizli sıraya alma ilk fazda yok, CAMERA_BUSY terminal olmayan ret döner. Multi-
camera acquisition tüm kaynakları UUID sırasıyla atomik reserve eder; kısmi
sahipliği bırakır, deadlock/hold-and-wait yoktur. Preview/edit/calibration da aynı
arbiter'a gider. Kaynak lease generation'ı geç callback'i fence eder. Queue
eklenirse yeni sözleşme ister; starvation izlenir ve UI kaynak sahibini gösterir.

Native OECS korelasyon/cancel/drain kanıtı yokken üretim izni kapalı kalır.
Software trigger veya lease bu engeli kaldırmaz. Donanım incapable ise explicit
UNAVAILABLE; fixture/test API normal capability açmaz. Stop→Start eski kareyi yeni
cycle'a bağlayamaz; kaynak frame sequence/session eşleşmesi zorunludur.

## S3 — Reçete, sabit template ve publish

PLC hedef station, beklenen recipe revision ve activation kimliğiyle tetikler.
TRIGGER reçete yüklemez/değiştirmez. Ölçüm/kontrol → isteğe bağlı Set Variables →
Publish Data sonucu hazırlar. Publish kaynak seçimi (a) cycle-local variable veya
(b) açık tool UUID+typed output portudur. Doğrudan kaynak, aynı provenance ve DAG
dependency denetimlerini alır; isim/string expression veya gizli global okuma yok.
Set Variables kullanılmadan typed output yayınlanabilir.

Machine template revision; **16 Number, 32 Flag, 4 Text ve 1 whole Pose** slotunu
sabitler. İsimler, gerekli/opsiyonel bayrağı, Number türü Int32, birim, rational
scale/offset, aralık, rounding; Text byte sınırı; Pose target/source frame ve
revision/adapter contract tanımlıdır. Özel adlandırılmış slot, mevcut indekslerden
birini kullanır; ilave alan veya key/value payload açmaz. Template metadata
reçeteden bağımsızdır; reçete slotlara kaynak bağlar. A/B aynı template revision
ile aynı wire düzenini üretir. Template değiştirmek açık revision/activation
işlemidir, bağlı peer onayı ve drained eski contract ister.

Tip/birim/frame uyumsuzluğu validation hatasıdır. Number int32 signed, açık
rational conversion ve ties-to-even rounding; overflow/signedness kaybı invalid,
saturate/wrap yok. Bool truthiness dönüşümü yok. Text UTF-8 max64 byte/slot;
daha uzun veri kesilmez, required ise result invalid olur. Missing optional slot
validity mask'i kapalı ve kanonik sıfır byte taşır; 0/false/empty text geçerli
değer olabilir. Bir required slot geçersizse terminal sonuç RESULT_INVALID ve
normal resultValid=false; geçerli kalan slotlar diagnostics'te tutulabilir fakat
PLC kullanma izni çıkaramaz. Execution, Result karar ve resultValid ayrıdır.

Pose tek typed SpatialTransform'dur; planar Point/Pose2D'yi Z=0 ve identity
quaternion ekleyerek 6D'ye yükseltmek yasaktır. Bileşenlere gizli projection yok;
XYZ/Euler gerekirse açık typed adapter ve kontratlı slot eşlemesi gerekir. Pose
üretilmemişse geçersizdir; UI sıfır pose/sahte PASS/score göstermez. Editor Test,
Run Tool/Flow yalnız preview; network intent veya gerçek terminal üretmez.

## S4 — Kalıcılık, ACK ve flow control

İstek anahtarı server installation UUID + peer UUID + peer epoch UUID + station
UUID + request uint64'tür; bağlantı session UUID anahtara dahil değildir.
Canonical fingerprint komut/body/recipe/activation içerir, yalnız transport session
hariçtir. Aynı key+aynı içerik mevcut accepted/terminal durumunu döndürür; yeni
frame/cycle yok. Aynı key+farklı içerik CONFLICT. Global CALIB komutları session
kimliğiyle ilgili station ledger'ına bağlıdır. Reddedilmiş transient BUSY yeni iş
sayılmaz; kabul edilmemiş key tekrar denenebilir. Accepted key sonsuza dek aynı
anlamda kalır. ACK tüketici uygulamasının exact sonucu işlediğini bildirir;
socket write/flush teslim kanıtı değildir.

Kalıcı intent → cycle begin → terminal+sealed bytes/result UUID/hash → pending →
attempt → ACK committed; çok dosyalı yazım manifest/WAL ile recovery-safe olur.
ACK exact station/peer epoch/request/result UUID/payload digest ister. Bir
station ACK'i diğerini veya kalibrasyon sample'ını temizlemez. Disk hata/full veya
bozuk journal admission'ı kapatır; unresolved kayıt silinmez. Restart terminali
olmayan begin'i RECOVERY_REQUIRED yapar; otomatik capture/re-run yoktur.

İlk politika station başına bir pending terminal; aynı station yeni trigger için
ACK bekler. Paylaşılan socket adil round-robin scheduler ile station mesajlarını
çerçeve sınırında interleave eder; response byte'ları birbirine karışmaz. Başlangıç
32 station, global 64 queued response, socket 64 KiB write high-water, okuma batch
en çok dört request. Pending sonuçlar disk otoritesidir; RAM limitte disconnect
ve replay mümkündür, silent drop yoktur. Global disk bütçesi mevcut güvenli
arşiv sınırından büyütülmez; COMM4-01 gerçek ölçüyü doğrular. Kuyruk doluysa
QUEUE_FULL; accepted isteğe kaynak ayırmadan önce terminal kaydı için headroom
ayrılır. Pending terminal retry 2s, üç tekrar; timeout alarmı kalır, yeniden
acquisition yok. Reconnect yeni session zarfı, aynı sealed payload/digest kullanır.

STOP/ABORT için bir ek control-terminal rezervi vardır (wire W5); normal üretim
ve kalibrasyon örnekleri için tek pending kuralı değişmez. Kontrol terminalinin
ACK'i bekleyen üretim sonucunu temizlemez.

Tombstone/retired epoch/numeric ID geçmişi sessiz eviction görmez. Request uint64
wrap yasaktır. Yeni peer epoch açık drained rollover ister; eski epoch retired
kalır. Outstanding eski sonuçlar varken peer kimliği/epoch değişimi otomatik olmaz.
Peer UUID authentication değildir; ilk sürüm güvenilen yerel ağ, üretim güvenlik
konfigürasyonu açıkça belgelenir; TLS/robot safety desteği iddia edilmez.

## S5 — Tek yeni UX ve migration

Communication'da tek yeni yüzey: server health/profil, station listesi, active
recipe/activation, Run/Auto/Stop, camera ownership, publication template, delivery/
ACK diagnostics ve calibration workspace. V2/V3 ayrı kalıcı sayfa/sekme bırakılmaz.
Bağlantı yeşil olması üretim Ready veya parça OK değildir. Üç station'ın durumları
aynı anda görünür; seçili satır detay gösterir. Port ve global listener ayarı
station seçimiyle değişmez. Reçete assignment Editor içinde, machine template
Communication altında; özel slot adları mapping'i kolaylaştırır.

Legacy v2/v3 dosyaları okununca otomatik rewrite edilmez. Explicit migration
önce read-only analiz+backup+manifest üretir: supported mapping, unsupported
özellik, UUID/numeric mapping, eski pending ACK listesi. Kullanıcı açık uygula
eylemiyle staging kopyayı doğrular; tek atomik pointer/swap ile v4'e geçer.
Başarısızlıkta eski byte/hash ve config aktif kalır. Eski çok endpoint fan-out,
64 KiB alan, string>64, birden çok whole Pose, v2 scalar conversion gibi
uyumsuzluklar sessiz kayıp görmez; unsupported raporu migration'ı bloklar veya
kullanıcının ayrı düzeltme taslağı gerekir.

Aynı linkte protocol auto-detect/downgrade yoktur; v4 explicit profil/versiyondur.
Eski pending ACK varsa mevcut kurulu v3 sürümü/profiliyle drain ve kanıt alınır;
v4 UI altında gizli v3 parser çalıştırılmaz. Alternatif belgeli rollback: uygulama
durdur, immutable backup sürüm/config+aynı ledger ile eski binary'yi aç ve drain,
v4 staging'i yeniden değerlendir. İki binary aynı ledger/socket'i eşzamanlı
yazamaz. v4'te kabul edilmiş yeni işler varken kör config restore yasaktır;
v4 pending drain/reconcile sonrası revision-checked rollback gerekir. Arşiv ve
dedup kayıtları migration/rollback ile silinmez veya anahtarları yeniden kullanılmaz.

## S6 — Robot ve optik araştırma kapısı

İki kurulum kapsamda: eye-to-hand sabit kamera/hareketli plaka; eye-in-hand
hareketli kamera/sabit plaka. Kamera intrinsic/distortion modeli, lens modeli,
distortion correction/pixel koordinatı, hedef geometrisi ve plate pose'un 6D
gözlenebilirliği araştırılır. Mevcut düzlemsel px→mm kalibrasyonu 6D hand-eye
değildir. Düzlemsel plate görüntüsünden solvePnP gibi pose kestirimi ancak
kalibre intrinsics, gerçek geometrik boyut, belirsizlik ve cheirality/ambiguity
denetimiyle adaydır; algoritma/SDK seçimi COMM4-18 kararıdır.

Gerekli gerçek girdiler: robot marka/controller/firmware, erişilebilir actual
6DOF pose, robot frame ve TCP tanımı, gripper/plate montajı, kamera/lens/distortion,
plaka ölçü/belirsizlik, hareket çeşitliliği, bağımsız kabul referansı/tolerans.
Eksikse yazılım DTO/fixture/oturum planı sürer; fiziksel solve/validation/activation
uygunluğu BLOCKED olarak korunur. Robot programı veya hareket komutu üretilmez;
operatör/robot sistemi güvenli örnek pozlarını kendi kontrol eder.

## S7 — Koordinat ve örnek alma sözleşmesi

`T_A_B`, B koordinatından A'ya dönüşüm; right-handed, column vector. İç temsil
Float64 translation mm + normalize quaternion **w,x,y,z**; frame UUID/revision
zorunlu. Robot gerçek örneği `base_T_flange` tercih edilir. TCP gerçek pose
gelirse açık `flange_T_tcp` offset/revision ile
`base_T_flange = base_T_tcp * inverse(flange_T_tcp)`; offset eksikse reddet.
Commanded pose actual yerine geçmez. Euler yalnız açık adapter; eksen sırası,
intrinsic/extrinsic, derece/radyan, handedness ve controller convention golden
fixture ile belgelenir. Başka frame'i base diye yeniden adlandırma veya ters
transform'u sessiz düzeltme yoktur.

Eye-in-hand bilinmeyen `flange_T_camera`, `base_T_plate` sabittir:
`base_T_flange[i] * flange_T_camera * camera_T_plate[i] = base_T_plate`.
Eye-to-hand bilinmeyen `base_T_camera`, `flange_T_plate` sabittir:
`base_T_camera * camera_T_plate[i] = base_T_flange[i] * flange_T_plate`.
Bunlar yön referansıdır; solver'a verilecek relatif A/B denklemleri COMM4-22'de
bu eşitliklerden türetilip bağımsız sentetik ground-truth ile sınanır. İlk modun
sonucunu ikinci modda ters çevirip kullanmak doğru genelleme değildir.

CALIB_BEGIN mode, station/camera, frame/revision ve model/plate dependency'lerini
dondurur. Production'dan ayrı calibration lease hedef kamerayı tutar; ilgili
station admission kapanır, bağımsız station devam eder. SAMPLE tek mesajda
session UUID, sample UUID, camera key, robot actual pose, robot sample/time
kimliği ve stationary beyanı taşır. Kabulden sonra yalnız **yeni** acquisition
başlatılır; eski preview kare kullanılmaz. Robot capture terminal cevabını alana
kadar durur; ACCEPTED robotu serbest bırakmaz. Cevap durable sample/frame UUID,
capture success/failure ve robot-can-release anlamı taşır. Pose–frame eşleşmesi
monoton yerel acquisition token + camera session + sample UUID ile yapılır;
uzak timestamp eşitliği tek başına kanıt değildir. Stationary beyanı fiziksel
ölçüm kanıtı değildir; robot handshake ve fixture dışında fiziksel doğrulama ister.

Duplicate SAMPLE aynı kayıtlı capture cevabını verir, tekrar çekmez. Capture
timeout sonrası pending hardware tamamlanması belgeli fence/drain ister; yeni
sample'a eski kare bağlanamaz. Reconnect BEGIN'i veya SAMPLE'ı kendiliğinden
yenilemez. ABORT yeni capture kapatır, lease'i gerçek drain'den sonra bırakır;
önceki sample/ledger kalır. Robot bağlantı kaybı güvenli hareket izni değildir;
operatör kontrolünde kalır, yazılım motion komutu göndermez.

SAMPLE role=solve veya holdout ile kaydedilir. Holdout örnekleri solve dataset'ine
giremez; aday sonrasında aynı lease içinde alınabilir. Capture cevabı cameraKey,
sample/frame UUID ve originating command içerir. Tek kameralı üretim cevabı da
cameraKey taşır; çok kameralı reçete kaynak kümesini activation kataloğundan okur.

İlk wire PoseBlock yalnız generic quaternion-wxyz taşır. Marka/controller bilgisi
yoktur; COMM4-02 [dar ön kontrolü](freeze-comm4-02.md) bu kapsamla freeze etti.
Controller RX/RY/RZ/Euler/axis-angle dönüşümü COMM4-19'un açık robot entegrasyon
adapter'ında, wire'a girmeden yapılır. Controller dönüşümü henüz kanıtlanmadı;
generic peer bu dönüşümü yapamıyorsa o controller unsupported kalır. Euler'ı
wire'a eklemek yeni contract revision/freeze ister. Otomatik convention yoktur.

## S8 — Solve, validate, activate

Session: CREATED/COLLECTING → SOLVING → CANDIDATE → VALIDATING → VALIDATED →
ACTIVE veya FAILED/ABORTED. SOLVE yalnız candidate üretir; üretim modeli değişmez.
En az örnek sayısı, yön/translation çeşitliliği, singularity/condition metriği ve
plate reprojection hatası araştırma kapısında sayısal olarak karara bağlanır;
üç/çok sayıda benzer pose yeterli kabul edilmez. Reject edilen örnek nedeni
saklanır; sessiz örnek çıkarma yoktur.

VALIDATE, solve eğitim kümesinden ayrı holdout örnek ve bağımsız fiziksel referans
kullanır. Translation/rotation hata, max/RMS/percentile, tolerans, dataset digest
ve model/dependency revision raporu üretir. Sentetik fixture yazılım doğrular;
fiziksel accuracy kabulü değildir. ACTIVATE exact validated candidate UUID/revision
ve dependency hash ile compare-and-swap, idle station+drained camera koşulunda
atomik olur. Eski geçerli model başarısız işlemde korunur. Lens, intrinsics,
mounting, TCP/frame revision değişimi validation iznini düşürür; rename düşürmez.
Restart ACTIVE kayıtlarını okur ama üretim Run/Auto ve lease'i yeniden açmaz.
