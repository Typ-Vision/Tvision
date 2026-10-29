# vf.socket/4 binary wire — COMM4-02 frozen baseline

**Contract revision `00434634-0002-4000-8000-202610290001`, 29 Ekim 2026.**
[Freeze raporu](freeze-comm4-02.md) ve
[golden manifest](../../../tests/fixtures/communication-v4/manifest.json) bu
baseline'ı sabitler. Bu bir tasarım/fixture kabulüdür; çalışan v4 codec/server
veya fiziksel PLC/robot uyumluluğu iddiası değildir. Değişen byte, enum, digest,
metadata veya anlam yeni contract revision ve yeniden freeze gerektirir.
Önceki 64-byte request / 128-byte response önerisi bu veri kümesine sığmaz.
Bu baseline **her request 512, her response 1024 byte** kullanır; ek paket veya
değişken uzunluklu frame yoktur. Text ve pose sayısı kapasite sınırıyla açıktır.
Pickit uyumluluğu iddiası yoktur. Pickit'in fixed integer socket yaklaşımı yalnız
esin kaynağıdır: [socket dokümanı](https://docs.pickit3d.com/en/latest/robots/robot-brands/socket_communication.html),
[arayüz dokümanı](https://docs.pickit3d.com/en/latest/robots/pickit-interface/pickit_interface.html).
Farklı boyut/field/signature mevcut robot protokolüne drop-in uymaz.

## W0 — Byte kuralları ve sınırlar

Offset'ler sıfır tabanlı ve son dahil aralıklardır. Tüm uint32/int32 ve uint64
big-endian/network byte order; int32 two's complement. uint64 iki uint32 word
(önce yüksek, sonra düşük) olarak PLC'de taşınabilir; signed64'e zorla çevrilmez,
JSON/export'ta decimal string olur. UUID/revision/session 16 opaque byte'dır;
RFC4122 canonical hex byte sırası, GUID mixed-endian memory dump yasaktır.
PLC dört uint32 word'ü byte sırasını koruyarak taşır; Float64'e çevrilmez.
Revision alanları uint64 sayaç yerine immutable 128-bit revision identity'dir.
IEEE754 binary64 alanları network byte order, finite zorunludur. Bunlar robot
Float32 register'ı gibi yorumlanamaz. Header int alanları **uint32**'tür;
request sequence ve açık numeric payload dışında int signedness değişmez.

Unused/reserved byte sıfır; parser bilinmeyen bit, mesaj türü veya reserved!=0
reddeder. Gelecek extension aynı sürümde sessiz eklenmez. Magic ASCII `VF04`
(56 46 30 34), version uint32=4. Header/body toplamı exact size'a eşittir.
TCP packet sınırı yoktur. Direction'a göre sabit512/1024 bilinir; ilk128 header
doğrulanır; size/version beklenenle uyuşmazsa bounded error ve close, tarayarak
magic yeniden bulma yoktur. Bir read<=2048, receive buffer<=4096, dispatch batch
<=4 request; tam frame sonraki batch'e kalabilir, taşma disconnect olur.
İlk partial byte'tan itibaren 2000ms monoton deadline; trickle süreyi yenilemez.
Veri gelmese tick çalışır; deadline'da tamamlanan frame de reddedilir. Socket
write partial olabilir; tek scheduler frame offset'ini korur, diğer frame aynı
frame bitmeden yazılmaz. High-water64KiB; sınırda read/admission backpressure.

## W1 — Ortak header, her iki yönde 128 byte

| Offset | Byte | Alan |
| --- | ---: | --- |
| 0–3 | 4 | magic VF04 |
| 4–7 | 4 | version=4 |
| 8–11 | 4 | message kind |
| 12–15 | 4 | frame size; request512/response1024 |
| 16–19 | 4 | flags=0; henüz hiçbir bit atanmadı |
| 20–23 | 4 | stationKey; global komutta0 |
| 24–31 | 8 | requestId, uint64; response exact echo |
| 32–47 | 16 | connection session UUID |
| 48–63 | 16 | peer UUID |
| 64–79 | 16 | persistent peer epoch UUID |
| 80–83 | 4 | recipeRevisionKey; immutable katalog girişi |
| 84–87 | 4 | templateRevisionKey; immutable katalog girişi |
| 88–103 | 16 | station activation UUID |
| 104–127 | 24 | reserved0 |

Toplam 4×6+8+16×3+4×2+16+24=128. Identity alanlarında tüm sıfır yalnız aşağıda
izin verilen handshake/global sentinel'dir. TRIGGER için station/recipe/template/
activation hepsi nonzero ve exact active snapshot olmalıdır; stale STALE_CONFIG
ile reddedilir. Response bu alanları isteğin/saklı terminalin kimliğiyle döndürür;
mevcut UI seçimiyle değiştirilmez.

## W2 — Mesaj kataloğu

| Request kind | Kod | Response kind/kod |
| --- | ---: | --- |
| HELLO | 1 | HELLO_REPLY101 |
| STATUS | 2 | STATUS_REPLY102 |
| CATALOG_GET | 3 | CATALOG_REPLY103 |
| SELECT_RECIPE | 4 | ACCEPTED110, TERMINAL111 |
| SET_AUTO | 5 | ACCEPTED110, TERMINAL111 |
| STOP | 6 | ACCEPTED110, TERMINAL111 |
| TRIGGER | 10 | ACCEPTED110, TERMINAL111 |
| ACK | 11 | ACKED112 |
| RECOVER | 12 | TERMINAL111 veya STATE113 |
| CALIB_BEGIN | 20 | ACCEPTED110, TERMINAL111 |
| CALIB_SAMPLE | 21 | ACCEPTED110, TERMINAL111 |
| CALIB_SOLVE | 22 | ACCEPTED110, TERMINAL111 |
| CALIB_VALIDATE | 23 | ACCEPTED110, TERMINAL111 |
| CALIB_ACTIVATE | 24 | ACCEPTED110, TERMINAL111 |
| CALIB_ABORT | 25 | ACCEPTED110, TERMINAL111 |
| EPOCH_ROLLOVER | 30 | STATE113 |
| Tüm komutlar | — | REJECT190 veya PROTOCOL_ERROR191 |

SELECT_RECIPE/SET_AUTO/STOP kayıtlı controlling peer ve yerel Remote Enable
politikası altında UI ile aynı application komutlarına gider. Motion veya robot
güvenlik komutu yoktur. STATUS stationKey0 ise global sayaçları;
stationKey>0 ise yalnız o station'ı verir; tek response içine sınırsız liste
sığdırmaya çalışma yoktur. Katalog entry başına request/response ile okunur.
ACCEPTED geçici kabul bildirimidir, sonuç veya capture-complete değildir.
Terminal üreten her mutating komut exact ACK ister; önceki calibration terminali
ACK olmadan sonraki mutating komut kabul edilmez. ABORT güvenli yeni capture
admission'ını hemen kapatabilir; tamamlanması önceki durable state'e bağlıdır,
eski unacked terminali silmez. Her mutating komutun ayrı requestId'si vardır.

## W3 — Request body 128–511 (384 byte)

Her tür yalnız tabloda atanmış alanı kullanır; kalan tüm body0. PoseBlock aşağıda
128 byte'dır. Request read-only türlerde requestId nonzero korelasyon sayacı,
mutating türlerde kalıcı dedup ID'sidir. HELLO hariç doğru connection session
zorunlu; HELLO dışındaki sıfır session STALE_SESSION'dır.

| Tür | Alanlar (offset, byte) |
| --- | --- |
| HELLO | 128 clientContractRevision UUID16; 144 capabilities uint32=0; 148–511 reserved364. Header session0, station/recipe/template/activation0; peer/epoch nonzero. |
| STATUS | Body384 reserved0. Recipe/template/activation header0; station0 veya bilinen key. |
| SELECT_RECIPE | 128 desiredRecipeRevisionKey u32; 132 desiredTemplateRevisionKey u32; 136–511 reserved376. Header mevcut activation/recipe/template (ilk yüklemede0), hedef station nonzero. Idle/drained durumda compare-and-swap; sonuç yeni activation bildirir. |
| SET_AUTO | 128 requestedAuto u32 (0 kapalı,1 açık); 132–511 reserved380. Header exact active snapshot. Açma kaynak hazırlığını da yapar; terminal READY ancak gerçek preparation tamamlanınca. |
| STOP | Body384 reserved0; header exact station snapshot. Admission hemen kapanır, terminal gerçek drain sonunda gelir; eski pending sonuç silinmez. |
| CATALOG_GET | 128 category u32; 132 numericKey u32; 136 expectedCatalogueRevision UUID16 (ilk sorgu0); 152–511 reserved360. Header station/recipe/template/activation0. |
| TRIGGER | 128 expectedCameraSetRevision UUID16; 144 inputSnapshot UUID16 (girdi yoksa0); 160 inputRevision UUID16 (yoksa0); 176–511 reserved336. Header exact station snapshot. |
| ACK | 128 resultUUID16; 144 digest32; 176–511 reserved336. Header station/recipe/template/activation saklı terminalle exact; requestId terminal requestId. |
| RECOVER | 128 resultUUID16 (bilinmiyorsa0); 144–511 reserved368. Header station ve eski recipe/template/activation exact, requestId aranan accepted key. |
| CALIB_BEGIN | 128 calibrationSession UUID16; 144 mode u32 (1 eye-to-hand,2 eye-in-hand); 148 cameraKey u32; 152 cameraRevision UUID16; 168 intrinsicRevisionKey u32; 172 plateRevisionKey u32; 176 flangeTcpRevisionKey u32 (flange actual için0); 180 robotAdapterRevisionKey u32; 184 poseContract PoseBlock128; 312–511 reserved200. PoseContract translation/quaternion tüm0, frame identity/revision alanları zorunlu; type=2 convention. |
| CALIB_SAMPLE | 128 calibrationSession UUID16; 144 sampleUUID16; 160 cameraKey u32; 164 stationary u32=1; 168 cameraRevision UUID16; 184 robotSampleSequence u64; 192 robotSourceTime u64; 200 robotClockKey u32; 204 actualPoseKind u32 (1 base_T_flange,2 base_T_tcp); 208 actualPose PoseBlock128; 336 acquisitionProfileRevisionKey u32; 340 sampleRole u32 (1 solve,2 holdout); 344–511 reserved168. |
| CALIB_SOLVE | 128 calibrationSession UUID16; 144 sampleSetDigest32; 176 solverPolicyRevisionKey u32; 180–511 reserved332. |
| CALIB_VALIDATE | 128 calibrationSession UUID16; 144 candidateUUID16; 160 candidateRevision UUID16; 176 holdoutSetDigest32; 208 referenceSetRevisionKey u32; 212 toleranceRevisionKey u32; 216–511 reserved296. |
| CALIB_ACTIVATE | 128 calibrationSession UUID16; 144 candidateUUID16; 160 candidateRevision UUID16; 176 validationReportUUID16; 192 expectedActiveRevision UUID16 (ilk active yoksa0); 208 dependenciesDigest32; 240–511 reserved272. |
| CALIB_ABORT | 128 calibrationSession UUID16; 144 reasonCode u32; 148–511 reserved364. |
| EPOCH_ROLLOVER | 128 proposedEpochUUID16; 144 currentCatalogueRevisionUUID16; 160–511 reserved352. Header station/recipe/template/activation0, current epoch exact; drained koşulu. |

CALIB body kimlikleri nonzero; kaynak zamanı tek başına freshness/pairing yetkisi
değildir. CALIB commands header station, recipe/template ve activation lease
başlangıcında dondurulan değerlerdir; üretim Run gerekli değildir. RobotAdapter
revision frame convention, pose source ve TCP offset sözleşmesini sabitler.
TRIGGER input snapshot girdi sözleşmesi UI/application tarafından yaratılır;
bu sürüm arbitrary remote input set veya büyük input stream tanımlamaz. Pose
gereken calibration kendi SAMPLE body'sinde atomik taşır.

## W4 — PoseBlock (128 byte, göreli offset)

| Göreli offset | Byte | Alan |
| --- | ---: | --- |
| 0–23 | 24 | x,y,z Float64 mm (8'er byte) |
| 24–55 | 32 | quaternion w,x,y,z Float64 (8'er byte) |
| 56–71 | 16 | targetFrame UUID |
| 72–87 | 16 | targetFrameRevision UUID |
| 88–103 | 16 | sourceFrame UUID |
| 104–119 | 16 | sourceFrameRevision UUID |
| 120–123 | 4 | convention1 = right-handed/column/mm/quaternion-wxyz; contract-only2 |
| 124–127 | 4 | reserved0 |

Norm `n=sqrt(((w*w+x*x)+y*y)+z*z)`; `abs(n-1)<=1e-6` kabul, n=0 veya finite
olmayan ara değer reddedilir. Float64 round-to-nearest/ties-to-even, gösterilen
sıra; FMA/reassociation yok. Kabul edilen q bileşenleri n'ye bölünür; ilk nonzero
bileşen negatifse dördü de ters işaret alır. Tüm numeric signed zero +0 olur.
Gönderen terminali canonical değerlerle seal eder; alıcı digest doğrularken
byte'ları tekrar normalize etmez. SAMPLE fingerprint alınan ham request üzerindedir;
aynı ID ile q/-q byte değişikliği de CONFLICT olur. Contract-only2 yalnız BEGIN'de, numeric alanları0, frame alanları
zorunlu; SAMPLE/RESULT'ta yasaktır. Invalid whole pose tüm128 byte0 ve poseValid0.
Geçerli identity pose açık validity1 ve frame kimlikleri taşır; missing'den ayrılır.

## W5 — Ortak operational response body (896 byte)

ACCEPTED/TERMINAL/ACKED/STATE/STATUS_REPLY/REJECT/PROTOCOL_ERROR aynı yerleşimi kullanır.
Unused alan0; terminalin Number/Flag/Text/Pose alanları fixed template'dir.

| Offset | Byte | Alan |
| --- | ---: | --- |
| 128–143 | 16 | result UUID; TERMINAL/ACKED dışı0 |
| 144–159 | 16 | cycle UUID; üretim dışı0 |
| 160–191 | 32 | terminal SHA-256 digest; TERMINAL/ACKED dışı0 |
| 192–195 | 4 | execution:0 unknown,1 done,2 error,3 canceled |
| 196–199 | 4 | decision:0 notEvaluated,1 OK,2 NOK |
| 200–203 | 4 | resultValid0/1 |
| 204–207 | 4 | reason/status enum |
| 208–211 | 4 | numberValid bitmap low16, upper16=0 |
| 212–215 | 4 | flagValid bitmap32 |
| 216–219 | 4 | flagValue bitmap32; invalid bit value0 |
| 220–223 | 4 | textValid bitmap low4, upper28=0 |
| 224–227 | 4 | poseValid0/1 |
| 228–231 | 4 | commandState/phase enum |
| 232–247 | 16 | calibration session UUID; üretimde0 |
| 248–263 | 16 | sample UUID; sample dışı0 |
| 264–279 | 16 | acquired frame UUID; yoksa0 |
| 280–343 | 64 | Number[16], signed int32, slot i offset280+4i |
| 344–615 | 272 | Text[4], her68 byte: length u32 +64 byte UTF-8 |
| 616–743 | 128 | whole PoseBlock |
| 744–747 | 4 | detail UTF-8 byte count0..128 |
| 748–875 | 128 | detail metni; artan alan0 |
| 876–883 | 8 | yerel capture monotonic nanosecond tick u64; yoksa0 |
| 884–887 | 4 | stationReady0/1 |
| 888–891 | 4 | stationBusy0/1 |
| 892–895 | 4 | pending delivery count u32 |
| 896–899 | 4 | camera owner stationKey; yoksa0 |
| 900–915 | 16 | calibration candidate/model UUID; yoksa0 |
| 916–931 | 16 | candidate/model revision UUID; yoksa0 |
| 932–947 | 16 | validation report UUID; yoksa0 |
| 948–951 | 4 | cameraKey; calibration/tek kameralı üretim için exact, multi-camera üretimde0 |
| 952–955 | 4 | originating request kind |
| 956–1023 | 68 | reserved0 |

Sayım:16+16+32+10×4+16+16+16+64+272+128+4+128+8+4×4+16×3+2×4+68=896.
Byte offset tablosu otoritedir; fixture sayımı aritmetiği otomatik denetler.
SELECT_RECIPE terminalinde yeni active recipe/template/activation header'da;
isteğin eski snapshot fingerprint'i ledger'da ayrıca korunur. ACK yeni terminal
header'ıyla yapılır. STATUS_REPLY de sorgu header'ındaki0 yerine gerçek active
snapshot kimliklerini verir; STOPPED ilk yükleme yoksa0. Tek istasyon için normal
pending terminal yanında en çok bir STOP/ABORT control terminal rezervi vardır;
bunlar pending ACK yüzünden admission kapatma eylemini geciktirmez. Ayrı exact
ACK kayıtları gerekir; global64 response limiti aşılmaz.
Normal1 + control1 station sınırıdır, sınırsız STOP kuyruğu yoktur. Aynı key
tekrarı mevcut kaydı döndürür; ikinci farklı control isteğinde rezerv doluysa
RESOURCE_LIMIT, yeni admission yine hemen kapanır fakat ACCEPTED/durable terminal
uydurulmaz. Üretim/örnek kontrol rezervini kullanamaz. Storage headroom normal
kabul öncesi iki sabit terminal ve journal kaydına ayrılır; mevcut512MiB /
10.000 kayıt /20MiB dosya sınırlarını büyütmez. Disk yazılamıyorsa RECORDING_ERROR
ve admission kapalı; Stop/drain eylemi güvenli uygulama portunda sürer.
SAMPLE terminalinde commandState CAPTURED veya CAPTURE_FAILED robotun capture
beklemesini sonlandırabileceğini ifade eder; accepted/timeout/reject böyle izin
oluşturmaz. SourceTime yerine local capture tick başka clock'la eşitlenmez.
Calibration terminal resultValid başarılı komut çıktısıdır, fiziksel hand-eye
üretim izni değildir; decision notEvaluated kalır. Terminal error/canceled
decision0 ve resultValid0 gerektirir. STATUS decision/resultValid ve slotları0;
hazır olma parça sonucu değildir. İlgili ayrıntı wire'a sığmıyorsa sabit reason ve
kalıcı diagnostics referansı kullanılır; detail metni kesilmez.

## W6 — HELLO_REPLY ve CATALOG_REPLY body

HELLO_REPLY:128 serverInstallationUUID16;144 serverContractRevisionUUID16;
160 catalogueRevisionUUID16;176 serverState u32;180 maxStations u32=32;
184 maxTextBytes u32=64;188 maxWriteQueueBytes u32=65536;
192 partialDeadlineMs u32=2000;196 requestBytes u32=512;
200 responseBytes u32=1024;204 reason u32;208–1023 reserved816.
Yeni session UUID header'dadır. İstek contract revision0 ise yalnız keşif;
mutating komutlar exact kabul edilmiş revision olmadan kapalıdır. Peer/epoch
kurulu namespace ile eşleşir; otomatik yeni epoch yaratılmaz. İlk enrollment
yerel explicit kayıtla yapılır. Tek server bir production controlling peer kabul
eder; ikinci bağlantı PEER_BUSY ile close. Çok station aynı bağlantıda çalışır.

CATALOG_REPLY:128 catalogueRevisionUUID16;144 category u32;148 numericKey u32;
152 entityUUID16;168 immutableRevisionUUID16;184 parentUUID16 (yoksa0);
200 nameBytes u32;204 nameUTF8[64];268 metadataBytes u32;
272 metadata[640];912 nextKey u32 (son0);916 reason u32;
920–1023 reserved104. Toplam16+4+4+16+16+16+4+64+4+640+4+4+104=896.
Category:1 station,2 recipeRevision,3 templateRevision,4 camera,
5 intrinsicRevision,6 plateRevision,7 frameRevision,8 adapterRevision,
9 acquisitionProfileRevision,10 solverPolicyRevision,11 referenceSetRevision,
12 toleranceRevision,13 robotClock. Metadata'nın dondurulmuş sabit şeması W6a'dadır.
640 byte üstü intrinsic matrix/plate/image verisi bu yolla taşınmaz; local model
catalogue otoritesidir, wire metadata yalnız sözleşme özeti/digest taşır. Name64
üstü görünen ad için local ayrı display label gerekir; wire adı sessiz kısalmaz.

Numeric key uint32,0 sentinel; kalıcı registry UUID+immutable revision'a birebir
bağlar. Revisions yeni key alır; rename kimliği bozmaz. Silinmiş key tombstone
olur, asla başka UUID/revision için kullanılmaz. Key alanı tükenince admission
bloklanır, silent wrap/reuse yoktur. Katalog snapshot revision değişince peer
yeniden exact revision okur; partial listeyle trigger kabul edilmez. Çakışma
oluşturulması repository validation/transaction testinde reddedilir.

## W6a — Sabit catalogue metadata (schemaVersion1)

CATALOG_REPLY `metadataBytes=256`; metadata'nın göreli0–255 byte'ı kullanılır,
256–639 sıfırdır. Aşağıdaki offset'ler **metadata başlangıcı272'ye görelidir**.
Her kategori aynı ortak başlığı kullanır:0–3 schemaVersion u32=1,
4–7 schemaBytes u32=256,8–39 contentDigest SHA256,40–71 dependenciesDigest SHA256.
Digest'ler wire metadata'nın hash'i değildir: yerel katalogdaki immutable içerik
ve dondurulmuş dependency manifest'inin saklı exact digest'idir. Görünür ad hash
girdisi değildir; algoritma/şema sürümü içerik manifest'inde kayıtlıdır. Yerel
byte manifest'i olmayan giriş mutating kullanım için uygun değildir. Yalnız
özet taşınır; tam template ölçekleri, matrisler, plaka noktaları ve fiziksel
toleranslar digest ile bağlanan yerel kayıtta kalır, peer'a gizlice türetilmez.

Alanlar aksi yazılmadıkça u32 BE. `UUID`16 byte; tabloda yazılmayan her göreli
byte (alanlar arasındaki boşluklar dahil) sıfırdır; böylece her kategori256 byte,
ortak72 + atanmış alanlar + kalan reserved0, overlap olmadan tamamlanır.

| Category | Göreli offset: alanlar |
| --- | --- |
|1 station|72 stationState;76 activeRecipeKey;80 activeTemplateKey;84 cameraCount;88 activationUUID;104 cameraSetRevisionUUID;120 remoteEnabled0/1;124–255 reserved132|
|2 recipeRevision|72 applicationType (1 measurement,2 inspection,3 robotGuidance);76 requiredCameraCount;80 publicationTemplateKey;84–255 reserved172|
|3 templateRevision|72 numberCount=16;76 flagCount=32;80 textCount=4;84 poseCount=1;88 textMaxBytes=64;92 numberEncoding=1 signed32;96 rounding=1 tiesToEven;100 poseConvention=1;104–255 reserved152|
|4 camera|72 backend (0 unavailable,1 imageFile,2 webcam,3 industrial);76 acquisitionProfileKey;80 intrinsicKey;84 frameKey;88 capabilityBits (bit0 acquire,bit1 productionPermitted,bit2 calibrationPermitted; diğerleri0);92–255 reserved164|
|5 intrinsicRevision|72 cameraKey;76 widthPixels;80 heightPixels;84 projection (1 pinhole,2 fisheye);88 distortion (0 none,1 BrownConrady,2 fisheye);92 calibrationEvidence0/1;96–255 reserved160|
|6 plateRevision|72 geometry (1 planarGrid,2 explicit3DPoints);76 pointCount;80 unit=1 mm;84 physicalEvidence0/1;88–255 reserved168|
|7 frameRevision|72 parentFrameKey (root0);76 convention=1;80 role (1 base,2 flange,3 tcp,4 camera,5 plate,6 other);84–255 reserved172|
|8 adapterRevision|72 representation=1 genericQuaternionWXYZ;76 actualPoseKind (1 base_T_flange,2 base_T_tcp);80 convention=1;84 flangeTcpFrameKey (flange actual0);88 controllerVerified0/1;92–255 reserved164|
|9 acquisitionProfileRevision|72 cameraKey;76 pixelFormat (1 Mono8);80 triggerMode (1 software,2 hardware);84 nativeProductionAllowed0/1;88–255 reserved168|
|10 solverPolicyRevision|72 algorithm (0 unavailable,1 researchApprovedHandEye);76 modeMask (bit0 eyeToHand,bit1 eyeInHand, diğerleri0);80 softwareEvidence0/1;84–255 reserved172|
|11 referenceSetRevision|72 referenceType (1 synthetic,2 physicalIndependent);76 unit=1 mm;80 independentEvidence0/1;84–255 reserved172|
|12 toleranceRevision|72 translationUnit=1 mm;76 rotationUnit=2 radian;80 physicalApproval0/1;84–255 reserved172|
|13 robotClock|72 unit=3 nanosecond;76 monotonic0/1;80 clockEpochUUID;96 synchronisedEvidence0/1;100–255 reserved156|

Evidence/approval/capability bool'ları ölçüm sonucu değildir ve birbirinin yerine
geçmez. Generic fixture tüm fiziksel kanıtları0 taşır. Algorithm1 ancak COMM4-18
araştırması tamamlandığında yerel yetenek olabilir; bu freeze solver seçmez.
Backend3 mevcut native engeli kaldırmaz. CameraCount1..32; active yüklenmemiş
station'da recipe/template/activation/cameraSet0 ve cameraCount0; diğer durumlarda
exact snapshot. Bir station kamera kümesi en çok32 kamera içerir; fazlası RESOURCE_LIMIT.
Non-root frame parentUUID kendi parentFrameKey UUID'siyle exact; diğer kategorilerde
parentUUID yerel sahip entity (recipe/template root ise0). Entry UUID/revision/key
nonzero; istekte key0 o kategorinin ilk canlı kaydıdır, sonuç nextKey0 sonu belirtir.
İlk expectedCatalogueRevision0 anlık revision keşfeder; takip sorguları exact revision
ister. Liste ortasında değişirse STALE_CONFIG; peer listeyi yeniden okur. Bilinmeyen
category INVALID_VALUE; bilinmeyen key UNKNOWN_KEY: echo category/key+catalogue
revision+reason, entity/revision/name/metadata/nextKey0. Retired key geri dönmez.

HELLO contract0 yalnız discovery; exact contract HELLO yenilenmeden mutation yok.
Yeni HELLO yeni session üretir, durable identity değişmez. Enrollment controlling
peer/epoch'u yerel kayıttan doğrular. Peer tam exact katalog snapshot'ını almadan
mutating key göndermez; server her key/revision/dependency'yi ayrıca doğrular.
RequestId0 bütün türlerde INVALID_VALUE; uint64 MAX geçerlidir, sonraki0 yasaktır.
Global STATUS sayaç döndürür; stationReady/busy ve slotlar0; catalogue/HELLO dışında
production-ready listesi yoktur. Operational STATUS_REPLY sorgu recipe alanları0
olmasına rağmen mevcut snapshot'ı döndürür (W5); global snapshot0 kalır.
CALIB_ABORT reasonCode W7 reason enum'udur. Global read-only/HELLO sentinel'leri
dışında peer/epoch/session nonzero; calibration station snapshot hiç yüklenmemişse
recipe/template/activation üçü birlikte0 olabilir, lease bu snapshot'ı dondurur.
FlangeTcpRevisionKey0 yalnız actualPoseKind1; kind2 exact offset revision ister.
sample robotClockKey, sequence ve camera/profile kimlikleri nonzero; robotSourceTime
clock epoch içinde0 olabilir. Kaynak timestamp'i acquisition korelasyonu yapmaz.

## W7 — Digest, idempotency ve hatalar

Terminal digest SHA256 girdisi exact:
`ASCII("vf.socket4.terminal/1") || 00 || frame[20:32] || frame[48:104] || body896`.
Burada slice sonu hariçtir; sıralama stationKey, requestId, peerUUID, epochUUID,
recipeKey, templateKey, activationUUID'dir. Body frame[128:1024]; frame160–191
hesap sırasında32 sıfır byte'dır. Prefix22 byte, header68 byte, body896 byte:
preimage986 byte. Server installation ledger namespace'idir; wire digest alanı değildir.
Magic/version/kind/size/session/reserved header digest'e dahil değildir.
Digest girdisi versioned prefix ASCII `vf.socket4.terminal/1` + NUL ile başlar.
Terminal header kind111 olarak kalıcı seal edilir; reconnect yalnız session
zarfını değiştirir. ACKED digest'i yeniden üretmez, saklı exact result digest'i
echo eder. ACKED body'si yalnız resultUUID/digest/reason/state taşır; slotlar0.
Duplicate terminal byte-for-byte saklı body ve stable header alanlarını döndürür.
Fingerprint exact `ASCII("vf.socket4.request/1") || 00 || frame[8:12] ||
frame[20:32] || frame[48:104] || frame[128:512]`: prefix21 + header72 + body384
=477 byte. İki digest'te de session hariç; key'in command kind'i fingerprint'tedir.
ACK kendi mutating intent'ini oluşturmaz, mevcut terminal key'ini exact kapatır.
RECOVER de yeni fingerprint admission'ı yapmaz; saklı key'i okur. HELLO/STATUS/
CATALOG_GET requestId yalnız korelasyondur, durable mutation dedup kaydı değildir.
EPOCH_ROLLOVER drained durumda namespace değişimini atomik kaydeder; STATE reason0
eski request/epoch'u echo eder. Aynı rollover key/body eski namespace tombstone'unda
idempotent kalır, yeni epoch'un ilk request'i değildir; sıra0'a wrap yapılmaz.

Reason frozen enum0 NONE,1 NOT_AUTO,2 STATION_BUSY,3 CAMERA_BUSY,4 STALE_CONFIG,
5 STALE_SESSION,6 CONFLICT,7 QUEUE_FULL,8 RECORDING_ERROR,9 RECOVERY_REQUIRED,
10 UNKNOWN_KEY,11 INVALID_VALUE,12 UNSUPPORTED_VERSION,13 BAD_FRAME,
14 ACK_MISMATCH,15 ACK_TIMEOUT,16 LEASE_MISMATCH,17 PHYSICAL_GATE_BLOCKED,
18 PEER_BUSY,19 RETIRED_EPOCH,20 NOT_READY,21 CAPTURE_TIMEOUT,
22 RESULT_INVALID,23 RESOURCE_LIMIT,24 CONTRACT_MISMATCH,25 REMOTE_DISABLED. commandState frozen0
NONE,1 ACCEPTED,2 RUNNING,3 TERMINAL,4 ACK_COMMITTED,5 COLLECTING,6 CAPTURED,
7 CAPTURE_FAILED,8 CANDIDATE,9 VALIDATED,10 ACTIVE,11 ABORTED,12 RECOVERY_REQUIRED.
READY ayrı reason değildir: başarılı SET_AUTO terminali execution1/resultValid1,
reason0, stationReady1 ve commandState3 taşır. Enum kodları sabittir; etiketten
numeric hash türetilmez. ServerState0 NOT_CONFIGURED,1 LISTENING,2 PEER_CONNECTED,
3 FAULT. StationState0 STOPPED,1 PREPARING,2 ARMED_MANUAL,3 ARMED_AUTO,4 RUNNING,
5 DRAINING,6 BLOCKED. CalibrationState0 CREATED,1 COLLECTING,2 SOLVING,3 CANDIDATE,
4 VALIDATING,5 VALIDATED,6 ACTIVE,7 FAILED,8 ABORTED. Bunlar commandState ile
karıştırılmaz; operational body phase yalnız commandState'tir.

ACK geç/duplicate gelebilir; exact kayıt varsa ACK commit idempotent'tir.
Başka result/digest/epoch uyuşmaz ACK_MISMATCH, durumu değiştirmez. Request max
uint64 sonrası yeni epoch için drained kontrollü rollover gerekir. Process restart
server installation/registry/epoch/ledger'i korur; connection session değişir.
Reconnect eski session komutu reject; RECOVER aynı persistent request key'i bulur.
Retired epoch'tan tetik reddedilir, tombstone inspect read-only diagnostics'tedir.
Parser semantic reject'i yeni cycle saymaz. Bozuk magic/size/version ve taşma
öncesi bounded protocol error gönderilebiliyorsa gönderilir, sonra close; raw
payload loglanmaz. Busy ret kuyruk değildir. Hardware capture kabul edildikten
sonra sonuç belirsizse retry yeni capture oluşturmaz, recovery gerektirir.

## W8 — Freeze için zorunlu byte fixture kümesi

[Fixture README](../../../tests/fixtures/communication-v4/README.md) binary/hex,
bağımsız PLC word tablosu ve encode/decode expected değerlerinin girişidir.
Fixture kabulü çalışan codec/framer, ledger veya fiziksel robot testi değildir.
Fixture: tüm kinds; HELLO keşif/exact contract; station1/2/3; Number INT_MIN/MAX,
negatif scale, ties-to-even/overflow; bit0/31; boş/geçerli64byte Unicode Text ve
65byte reject; Pose identity/nontrivial/inverse/revision mismatch; SAMPLE full
identity; zero/large uint64>2^53/MAX; UUID leading0; text reserved/nonzero padding;
unknown flags/types; signed/word reversal; SHA digest+ACK vectors; partial/coalesced
frames; stale session/activation; max-limits. Her tür body sum ve reserved alan
check'i, metadata category şeması, enum/state kodları freeze kabulüne dahildir.
