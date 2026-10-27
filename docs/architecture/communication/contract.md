# Haberleşme verileri ortak sözleşmesi

COMM-PLAN-03 hedef sözleşmesi; uygulama tamamlandı anlamına gelmez.
Yalnız seçili kartın işaret ettiği bölümleri okuyun. Uygulama adları/sürüm
numaraları COMM-01 envanterinde doğrulanır; buradaki davranışlar korunur.

C0–C7 temel fazın sözleşmesidir. C8 yalnız seçili Modbus, C9 yalnız seçili
OPC UA alt fazında; C10 kalibrasyon örnekleri/FANUC ihtiyacı seçilirse uygulanır.
C1 genel mekânsal veri sözleşmesi temel fazda kalır; C10 temel teslim şartı değildir.

## C0 — Katmanlar ve kapsam

C++20/Qt Widgets/OpenCV; UI application servisine gider. Domain değer/config/
doğrulama ağ SDK'sı bilmez. Application, snapshot/atama/karar/yayın/tetik/teslimi
yönetir; transport adaptörü yalnız ortak portları uygular. Mevcut servisler
yeniden yazılmaz. Üç applicationType aynı tool'ları ve Runtime'ı kullanır.
Eski vf.socket/2 ve no-op Save byte/hash davranışı korunur. Yeni alanlar yalnız
açık edit ile şema yükseltir. Şema numarasını güncel koddan seçin.

Bu faz için küçük başlangıç: recipe-cycle local variable ve salt okunur system
değerleri. Global writable machine variable, dışarıdan reçete/tolerans değişimi,
serbest expression/script ve sınırsız nested record sonraki kapsama bırakılır.
Input snapshot ve endpoint health sistem verisidir; hidden mutable global değildir.

## C1 — Değerler, kimlik, birim ve mekânsal veri

Scalar tipler Boolean, Int32, UInt32, Int64, Float64, UTF-8 String ve tanımlı Enum.
Float NaN/Inf reddedilir; daraltma/signedness değişimi açık converter ister.
Int64 JSON'da decimal string; wire tipinin doğruluğu tag ile korunur. İlk metin
sınırı 4096 UTF-8 byte. Enum tanımı ID ve izinli kod kümesi taşır.

ValueEnvelope: typed value veya yokluk, validity/reason, unit, producer ID/port,
cycleId/frameId (uygunsa), configId ve dependency kimlikleri. Missing, stale,
skipped, invalid ve error nedenleri ayrılır. Yok değer null'dır; 0/false/identity
başarı varsayılanı değildir. Salt constant kendi declared unit'ini taşır.

Unit boyutu: unitless, length, angle. İlk birimler mm/m, rad/degree; dönüşüm
açık ve boyut uyumlu. Mevcut px sonuçları world length'e otomatik yükselmez.
Rename UUID binding'ini korur; silme/tip/birim değişimi referans doğrulaması ister.

SpatialValue: `T_A_B` B'den A'ya; right-handed, column vector. FrameRef cihaz/
robot/group kimliği, frame UUID/revision içerir. Rotation: normalize quaternion,
**w,x,y,z**; translation **Float64 mm**. Euler yalnız açık giriş/çıkış codec'i,
angle hesapları rad. Zero norm reddedilir; q ve -q aynı rotasyondur. Validasyon
norm toleransı ve normalize politikasını açık tanımlar; girdi bozulması örtülmez.
Raw matrix import rigid şartlarını denetler. Farklı frame/revision compose edilmez.

Point/Rotation/Transform ayrı anlam taşır; pixel Point ve düzlemsel Pose2D,
Pose3D'ye sessiz dönüştürülmez. HALCON 7-tuple ile quaternion+translation 7'li
aynı tip değildir. İlk fazda sadece typed transport/domain; yeni 3D producer yok.

## C2 — Variable tanımı ve atama

Recipe variable: UUID, benzersiz görünen ad, type/unit, opsiyonel frame kısıtı,
yorum. En çok 256 variable/reçete, tool başına 64 atama. Değer her cycle başında
unset; sonuçlar reçete tanımına kalıcı yazılmaz. System alanları read-only.

Set Variables kaynakları ilk sürümde **typed constant, açık tool output veya
açık upstream assignment bundle alanı**. Input snapshot tool portu da olabilir.
Genel expression ve string eval yoktur. Mevcut Calculate ayrı araç olarak kalır.
Tüm RHS aynı immutable giriş görünümünden değerlendirilir; sonra tek atomik
commit. A ve B aynı grupta ise B=A yeni A'yı okumaz. Sıralı işlem ikinci tool ve
açık dependency ister. Aynı hedef UUID'ye iki satır reddedilir.

Çıkış bundle'ı producer/tool ve revision taşır. Variable adı üzerinden gizli
okuma/yazma yapılmaz. Aynı hedefe farklı dalların belirsiz yazımı validation
hatasıdır; açık sıralı sürüm olabilir. Cycle/stale/missing/type/unit hatasında
grubun tamamı uncommitted; eski cycle değeri geri gelmez.

Run Tool, Run Flow/Editor preview yalnız hesaplar. Gerçek yayın sadece aktif
immutable Runtime'ın terminal commit yolundan gelir; Editor Test publish etmez.

## C3 — Karar

ExecutionState, recipe status ve part decision ayrı. İlk genel decision aracı
Result'tır: seçilmiş açık Boolean accepted portları için ALL veya ANY.
İlk sürümde yalnız bir etkin Result bulunur; yeni araç boş ve disabled taslaktır.
Boş etkin liste invalid; required girdilerden biri missing/skipped/error/stale ise
**NOT_EVALUATED**, diğer girdiler kısa devreyle bunu OK/NOK'a çeviremez.
Tüm required girdiler valid olduğunda ALL/ANY sonucu OK/NOK üretir.
Execution ERROR/CANCELED ise export NOT_EVALUATED/resultValid=false.
Tool Boolean toleransı kendi başına genel karar değildir; Match found/score
otomatik kabul değildir. Kullanıcı bilinçli rule kaynağı seçer.

System Decision salt okunur; Set Variables gerçek genel kararı değiştiremez.
Yeni wire OK/NOK/NOT_EVALUATED kullanır; eski v2 PASS/FAIL anlamı değişmez.
Kararı olmayan geçerli ölçüm verisi yayımlanabilir; decision NOT_EVALUATED kalır.
ResultValid yayın grubundaki required alanların geçerliliğidir; fiziksel doğruluk
izni veya robot hareket izni değildir. Grubun required alanı invalid ise normal
payload yayımlanmaz; aynı request'e terminal invalid/error zarfı iletilir.

## C4 — Profil, yayın intent'i ve terminal bundle

Endpoint profile: UUID/revision, protocol/role, connection ayarları, enabled,
timeout/retry ve credentialRef. Machine-local; secret reçetede/logda olmaz.
Reçete output group: UUID, required/optional endpoint, logical field mapping,
profile contract revision ve payload schema. Fiziksel adresler profile mapping'de;
reçete kaynak UUID/port ve mapping contract kimliğine bağlanır. Aktif bağlantı
ayar/dependency hash'i immutable snapshot'a katılır; edit Stop/reload ister.
Dosya taşıma eşdeğer endpoint'i kendi kendine seçmez.

COMM-36 TCP fan-out aynı terminal bundle'ı hedef başına ayrı request/ACK
journal'ına bağlar. İlk etkin grup tetik girişidir; ikincil sunucular delivery-only
çalışır ve requestId olarak cycle UUID kullanır. Her hedef ayrı TCP server
profili ister; mapping whole typed record taşır. SpatialTransform → x/y/z/rx/ry/rz
ve örtük Euler projection yoktur. Runtime teslim bariyerleri endpoint sahibine
göre birleşir; bir ACK başka hedefin bariyerini temizlemez. Çoklu journal yazımı
dağıtık atomik commit değildir; kısmi yazımda recording/recovery engeli korunur.

Publish Data tool'u network yapmaz; cycle-bound PublicationIntent üretir.
En çok 16 output group/cycle, 64 field/group, ilk payload sınırı 64 KiB.
TerminalCycle + intent/bundle commit tamamlanmadan network yazımı yok.
Akış ortasında gönderim ve dış yanıt bekleyerek aynı akışı sürdürme yoktur;
bu ihtiyaç ayrı etkileşim/handshake kapsamı gerektirir.
Disk kaydı retry aynı cycle/intent kimliğini kullanır; tekrar acquisition yok.
Terminal immutable; delivery/ACK ayrı ledger olayıdır. Stop yeni admission'ı
kapatır, mevcut kalıcı pending çıktıyı silmez. Runtime restart kendi kendine Run
ve yeni frame/robot command başlatmaz.

## C5 — Request ledger, outbox ve recovery

Kimlikler: workcell/project, endpoint/peer identity, requestId; recipe/configId
istek fingerprint'indedir. Aynı request key + farklı içerik CONFLICT. Aynı key
aynı içerik duplicate ise mevcut BUSY/terminal/ACK state döner; cycle yinelenmez.
requestId ve sequence farklıdır; v3 sequence UInt64 decimal string, wrap yok.
Modbus için aşağıda ayrı bounded unsigned sequence kullanılır.

Kalıcı sıra: REQUEST_INTENT → CYCLE_BEGIN → TERMINAL+BUNDLE → DELIVERY_PENDING →
ATTEMPT/ACK_RECEIVED → ACK_COMMITTED. ACK commit olmadan Ready/ACKED yok.
Bundle ve terminal iki ayrı fiziksel dosyadaysa commit manifest/journal ile
restart tutarlılığı çözülür; sadece iki başarılı write'a atomiklik denmez.
Store/kayıt bozuk veya kapasite doluysa admission kapanır, kuyruk silinmez.

Restart durumları: terminal+bundle varsa aynı payload replay edilebilir;
intent/begin olup terminal yoksa **RECOVERY_REQUIRED**, otomatik yeni cycle yok.
ACK commit olmuşsa replay yeni effect üretmez. Kimliği belirsiz/foreign config
operator reconciliation ister; UI sahte ACK veya başarılı parça oluşturamaz.
Eski v2 kayıtları v3 talebiymiş gibi yorumlanmaz.

İlk politika endpoint başına 1 in-flight; required pending yeni cycle'ı engeller.
Optional backlog en çok 128 bundle/endpoint ve ortak mevcut storage budget'ı
aşamaz. Limitte admission kapalı ve açık QUEUE_FULL; silent eviction yok.
Ack tombstone/dedup key'leri retention sınırına kadar kalıcı; key silme aktif
request namespace'ini sessiz yeniden kullanılabilir yapmaz. Sürekli üretim için
operatör kontrollü namespace rollover ve peer mutabakatı; eski namespace replay
reddi. Tombstone'lar otomatik süre/ACK eviction olmadan mevcut archive bütçesine
kadar saklanır: toplam 10.000 JSON kayıt / 512 MiB, tek dosya 20 MiB. Admission
32 kayıt / 32 MiB ve endpoint'teki her unresolved request için ek 8 kayıt / 1 MiB
baş boşluğu ister; limitte QUEUE_FULL, retention/delete yoktur. Bu preflight
disk ayırma garantisi değildir; sonraki yazma hatası recording/recovery bariyerini korur.

`DeliveryStore::admission/request(required, online)` required offline/pending ve
optional 128 sınırını uygular. `request` preflight'ı delivery lock altında tekrar
denetler. Peer kimliğine operatör/peer tarafından anlaşılmış generation eklenir.
`rolloverNamespace(from,to,peerAgreement)` yalnız drained eski namespace ve hiç
kullanılmamış yeni peer generation için kalıcı retirement yazar. Eski key/namespace
inspect/replay/request/listen ile yeniden silahlanamaz; yeni peer generation açık
listen ister. Retirement ve eski tombstone'lar aynı bütçede kalır; dolu archive
yerine geçmek/taşımak ayrıca kontrollü arşiv yönetimi gerektirir, bu API silmez.

Retry default 2 s, en çok 3 otomatik deneme, monoton deadline, aynı byte payload.
Geç ACK kabul edilir; retry bitince pending/alarm kalır. Raw write/flush/OPC Write
başarısı consumer ACK değildir. Uçtan uca exactly-once garantisi verilmez.
V3 endpoint API aralığı 100–60000 ms / 0–10 retry'dır (default 2000/3);
ilk gönderim retry sayılmaz. QElapsedTimer deadline'ı owner-thread QTimer ile
poll edilir; offline iken attempt tüketilmez. Yeni bağlantı bütçeyi yeniden
başlatır. Required ACK_TIMEOUT kabulü kapalı tutar; optional pending veri 128'e
kadar saklanır. Endpoint başına tek RESULT in-flight, ACK commit sonrası sonraki
pending seçilir. Persisted canonical payload/resultId/hash korunur; yalnız yeni
bağlantının session zarfı değişir. ACK_COMMITTED satırları RAM session kotasına
sayılmaz. V2'nin 128 accepted request oturum sınırı değişmez.

## C6 — Input snapshot ve Trigger Router

Tüm adapter'lar TriggerRequest üretir: endpoint/peer, requestId, sequence,
recipeId/configId, inputSnapshotId. Admission tek application servisi üzerinden;
ready/mode/camera/recording/delivery izinleri korunur. Aynı anda bir cycle;
çoklu tetik source arbitration ilk geçerli kabul + diğerine BUSY, sessiz queue yok.
Input snapshot tüm required typed girdilerin quality/time/revision kontrolünü
yapar. Parçalı robot pose bir snapshot sayılamaz. Sistem girişlerini remote taraf
set edebilir ama recipe değişkenlerini/izinleri doğrudan yazamaz.

Subscription ilk değeri/reconnect ve yüksek kalmış trigger bit yeni istek değildir.
Modbus/OPC tercih sequence + latched request/ACK'tir. Freshness maxAge profile'da
zorunlu; farklı clock'ların timestamp eşitliği synchronization kanıtı sayılmaz.
Software tetik kamera native frame-correlation engelini kaldırmaz.

## C7 — TCP v3 sınırı

V2 listener/mesajları korunur. Yeni **ayrı explicit v3 endpoint/port profili**;
v2'ye yeni alan ekleyip eski peer'i bozma, sessiz downgrade yapma.
UTF-8 JSON-LF; max line 64 KiB, partial deadline 2 s, bounded buffers.
Temel zarf `v:3,type,sessionId`; HELLO sunucu sessionId ve status/config'i verir.
Komutlar TRIGGER/STATUS/ACK; çıktı ACCEPTED/BUSY/RESULT/ACKED/INVALID/ERROR.
TRIGGER için requestId, sequence, recipeId, configId, inputSnapshotId zorunlu;
ACK için requestId/resultId/payloadHash zorunlu. RESULT aynı kimlikleri, execution,
decision, resultValid, field envelope'ları ve terminal error taşır.

Typed field `name,type,unit,validity,value,reason` + origin metadata; Pose tek
record'dur. Input snapshot güncelleme tipi `INPUT`: snapshotId, revision, fields,
sourceTime/clockId; TRIGGER kaydedilmiş exact snapshot'ı seçer. HELLO/STATUS
Ready/Busy gösterir, parça sonucu değildir. Session stale rejection korunur;
kalıcı dedup request kimliği session'dan bağımsız C5'tedir.

COMM-16 JSON örnekleri/strict alan şemasını codec fixture'ları olarak kaydeder;
bilinmeyen alan/tip/sürüm, eksik kimlik, overflow ve nonfinite reddedilir.
ACK/RESULT hash canonical persisted payload'dan; retry yeniden hesaplamaz.
V3 generic TCP ilk hedef güvenilen/konfigüre yerel ağdır; kimlik doğrulanmış
uzak servis iddiası yoktur. TLS/principal gereksinimi ayrı karttır; OPC güvenliği
aşağıda tanımlıdır.

## C8 — Modbus TCP

İlk rol VisionFlow client, PLC/simulator server; 0-based register adresi ve
explicit unitId. İlk implementation COMM-24 toolchain probe'uyla seçilir;
paket/library mevcut diye varsayılmaz. Transport thread UI'ı bloklamaz.
Adres mapping'de register türü/count, wire numeric türü, byte/word order, scale,
range, string encoding/length. Sınır aşımı ve overlap validation hatasıdır.

Handshaking profile'ı iki staging alanı kullanır: input(request payload + request
sequence), output(result payload + commit sequence); ayrı ACK sequence.
32-bit sequence iki register'dır; PLC yazı sırası ve read-before/read-after
kararlılık kontrolü zorunlu. Wrap'ta session/namespace re-arm; eski sequence
kabul edilmez. Payload önce, commit en son; consumer sequence tutarlılığı
görmeden kullanmaz. Çoklu register write'ın uygulama atomikliği varsayılmaz.
PLC örnek register tablosu COMM-25'te fixture olarak tanımlanır; canlı cihazın
adreslerini uydurmaz. Short pulse desteği iddia edilmez.

## C9 — OPC UA client

İlk rol client; server ayrı kapsam. SDK COMM-29'da lisans, MinGW/Qt ABI,
Release ve paketleme gereksinimiyle seçilir, sessiz zorunlu dependency yok.
Node kimliği namespace **URI** + identifier; reconnect'te namespace index yeniden
çözülür. Scalar type/rank/access ve unit mapping doğrulanır. İlk destek scalar
ve Pose'un açık field mapping'i; genel arbitrary ExtensionObject/PubSub yok.

DataValue status/source/server timestamp ayrı; bad/uncertain/stale veri request
başlatmaz. Subscription generation/Stop fence, reconnect initial-notification
re-arm ve sequence duplicate denetimi C6'yı uygular. Result field'ları önce,
commit sequence en son; consumer ACK node/sequence ile C5 teslimi kapatılır.

Certificate trust, endpoint security policy/mode ve credentialRef profildedir.
Unknown certificate otomatik güvenilir yapılmaz, plaintext/None'a sessiz fallback
yoktur. Test simulator için açık ayrı profil; üretim trust kabulü ayrı kanıt.
SDK callback thread'i UI/domain'a doğrudan girmez; bounded application queue.

## C10 — Kalibrasyon ve robot veri temeli

Kamera modeli/hedef/gözlem mevcut servis varlıklarına ID/revision ile referanstır;
yeni intrinsics solver veya ikinci calibration catalogue kurulmaz.
RobotActualPose: spatial transform, device/group, base/user-frame, flange/TCP
kimliği, sample sequence, source clock/time, received monotonic time, stationary
durumu. Commanded hedef actual sample yerine geçmez.

CalibrationSample: sampleId/sessionId, image/frame/source, robot actual pose,
target/model/intrinsics kimlikleri ve pairing/validity/rejection nedeni.
Aynı durağan capture'a ait örnek şart; clock domain veya pairing belirsizse
sample invalid. İlk teslim DTO/codec ve doğrulama; kamera sihirbazına otomatik
örnek toplama/hareket eklenmez. Invalid sample kalibrasyon sonucu üretmez.

FANUC profile için XYZ mm, WPR degree ve açık matris tanımı kullanılır;
bilinen controller/endpoint profili doğrulanmadan native destek ilan edilmez.
WPR→quaternion codec, frame/TCP metadata ve golden fixture ile sınırlıdır.
Configuration/turn/external axes endpoint gerektiriyorsa zarf korunur; pose'dan
uydurulmaz. Joint→Cartesian/IK, hareket programı ve hand-eye çözümü yoktur.
Gerçek pose producer yoksa test fixture yalnız testte kalır; normal Runtime
sıfır/sentetik hedef yayımlamaz.
