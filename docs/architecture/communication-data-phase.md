# COMM-PLAN-01 — Haberleşme verileri araştırması ve faz önerisi

Tarih: 26 Ekim 2026. Durum: araştırma ve öneri; uygulama/saha kabulü değildir.
**Güncel uygulama planı [COMM-PLAN-03](communication/README.md) içindedir.**
Aşağıdaki sekiz geniş kart tarihsel öneridir; temel ve ihtiyaca bağlı alt fazlara ayrılan 38 kart bunların yerine
geçer. Araştırma bulguları korunur, hiçbir uygulama kartı henüz tamamlanmadı.
Kullanıcı yeni fazı ölçüm, yüzey kontrolü, robot pose aktarımı ve dış tetikleme
üzerine yönlendirdi. Bu belge aşağıdaki eski sıra ifadelerinin yerine yeni devam
noktasını önerir; kamera/native/fiziksel engelleri kapatmaz.

## 1. Başlangıç ve sınırlar

Başlangıç kaynakları: [progress](../progress.md),
[PLC sözleşmesi](plc-socket.md), UI gereksinimlerinin 26–27, 33–34 bölümleri.
Mevcut belgelerde vf.socket/2, tek peer/outstanding, SoftwareTrigger,
immutable snapshot, tek Measurement UUID eşlemesi ve aynı oturum ACK/replay vardır.
Kalıcı dedup/outbox/restart kurtarma, çoklu typed mapping, OPC UA ve robot
fiziksel kabulü açık. Bu tespit belge okumasıdır; güncel kaynak kodu denetimi veya
yeni test sonucu değildir. Mevcut 128 kabul/session sınırı sürekli üretim için
ayrıca ele alınmalıdır.

Bu oturumda codebase-memory becerisi ve MCP araçları mevcut katalogda bulunamadı;
bu durum boş arama sonucu sayılmadı. Yapısal kaynak incelemesi yapılmadı.
Uygulama kartının başlangıcında AGENTS'in istediği `C:/Projects/Tvision` kökü için
list_projects/index_status ve check_index_coverage çalıştırılmalı; bu çalışma
alanıyla (`C:/OpenCode/TVision`) ilişki doğrulanmalıdır. Bu önkoşul yeni inceleme
olmadan tamamlanmış sayılmaz.

## 2. Resmi kaynaklardan karşılaştırma

Kaynaklar 26 Ekim 2026 tarihinde incelendi. Ürün aileleri birbirine karıştırılmaz;
bir belgedeki özellik bütün sürüm/platformların garantisi değildir.

| Ürün | Belgelenen yaklaşım | VisionFlow için çıkarım (öneri) |
| --- | --- | --- |
| MVTec HALCON | Generic socket için send_data/receive_data; OPC_UA I/O interface bir istemcidir, node okuma/yazma, subscription ve method örnekleri vardır. | Algoritma çıktısı ile transport arasında açık bir adaptör ve veri sözleşmesi kur. |
| NI Vision Builder AI | Variable Manager, Set Variable ve LabVIEW shared/network variable eşlemeleri; ayrı Communication Device Manager ve EtherNet/IP assembly ayarları. | Kullanıcının anlamlı değişkenleri ile cihazın register/byte adreslerini ayrı düzenle. |
| Zebra Aurora Vision Studio | Modbus bağlantı/okuma/yazma filtreleri, OPC UA istemci örneği ve TCP araçları; bazı fieldbus seçenekleri ek donanım gerektirir. | Akışta görülebilen haberleşme adımları ve protokolden bağımsız üst model kullan. |
| Zebra Aurora Design Assistant | Ayrı flowchart ürünü; resmi ürün föyünde TCP/IP, Modbus, OPC UA, PROFINET, EtherNet/IP ve native robot arayüzleri listelenir. | Robot profilini genel sayısal veriye ek bağlam olarak ele al; Studio desteğiyle eşitleme. |

HALCON bir vision kütüphanesi/geliştirme ortamıdır; turnkey PLC iş akışı ile
eşdeğer sayılmaz. Aynı üreticinin MERLIC ürünü ayrıca OPC UA server plug-in
sunar; bu HALCON istemcisinden farklı bir ürün/roldür.

NI'nin resmi shared-variable örneği Set Variable ile ağ değişkenine veri
aktarımını açıkça gösterir. Bunun LabVIEW değişken altyapısı olması, OPC UA
olduğu anlamına gelmez. 2013 öğretici belge yalnız Set Variable/state diagram
yaklaşımı için tarihsel kanıttır; güncel protokol matrisi olarak kullanılmaz.
İncelenen kaynaklarla NI için yerleşik OPC UA desteği iddia edilmez.

Resmi kaynaklar:

- [HALCON OPC_UA arayüzü](https://www.mvtec.com/fileadmin/Redaktion/mvtec.com/06_downloads/interfaces/hioOPC_UA.html)
- [HALCON send_data](https://www.mvtec.com/doc/halcon/2311/en/send_data.html)
- [MERLIC OPC UA server kurulumu](https://www.mvtec.com/doc/merlic/26.03/manual/en-us/Content/RTE_plugins/Standard_plugins/OPC_UA_plugin/configuring_server.html)
- [NI shared variables ve Set Variable](https://knowledge.ni.com/KnowledgeArticleDetails?id=kA00Z0000019LjnSAE&l=en-US)
- [NI EtherNet/IP / Omron assembly yapılandırması](https://knowledge.ni.com/KnowledgeArticleDetails?id=kA03q000000x4qJCAQ&l=en-US%2F1000)
- [NI VBAI öğretici, Ağustos 2013](https://docs-be.ni.com/bundle/373379l/raw/resource/enus/373379l.pdf?save_local=true)
- [Aurora Vision haberleşme özeti](https://docs.adaptive-vision.com/studio/technical_issues/CommunicationOverview.html)
- [Aurora Modbus TCP uygulaması](https://docs.adaptive-vision.com/current/studio/application_notes/Using_ModbusTCP.html)
- [Aurora OPC UA uygulaması](https://docs.adaptive-vision.com/studio/application_notes/Using_OPCUA.html)
- [Aurora Design Assistant ürün föyü](https://www.zebra.com/us/en/software/fact-sheets/machine-vision-and-fixed-industrial-scanning-software/aurora-design-assistant.html)

## 3. Önerilen ürün modeli

Tek Recipe Editor/Runtime ve üç applicationType korunur. Ortak katmanlar:

1. **Variable Catalog:** isim/kalıcı UUID, tip, birim, koordinat sistemi,
   kapsam, erişim yönü, geçerlilik ve köken.
2. **Set Variables:** akışta çoklu atama; transport çağırmadan çevrim verisi hazırlar.
3. **Result / Decision:** gerçek araç sonuçlarından açık tolerans ve AND/OR
   kuralıyla genel parça kararı oluşturur. Atama genel kararı keyfi değiştiremez.
4. **Publish Data:** hazırlanmış paketi adlandırılmış çıkış grubuna bağlar;
   üretimde terminal kayıt sonrası outbox üzerinden teslim edilir.
5. **Communication Manager:** endpoint, rol, protokol, mapping, tetik, ACK,
   timeout, diagnostics ve bağlantı ömrünü yönetir.

UI yalnız application servisine gider. Önerilen bağımlılık:
UI → application (variable/trigger/publication/delivery orchestration) →
domain sözleşmeleri; protocol adaptörleri altyapı katmanından portları uygular.
Mevcut PlcRuntimeSocket'in taşınması otomatik kapsam değildir; önce uyarlama
sınırı kaynakta doğrulanır. Algoritmalar PLC veya OPC SDK'sı bilmez.

### 3.1 Değişken kapsamı ve geçerlilik

- Cycle-local: ölçüm, kontrol kararı ve hedef pose. Her çevrim başında unset;
  önceki parçanın sonucu yeni parçada varsayılan olmaz.
- Recipe parameter: kayıtlı sabit/tolerans/config girdisi; aktif immutable
  snapshot değişmez. Dışarıdan parametre değişimi kontrollü yeniden yükleme ister.
- Machine state: bağlantı, giriş sinyali, heartbeat ve sayaç. Kalıcılık açık
  seçilir; bağlantı kesildiğinde quality/timestamp bayatlığı görünür.
- System: cycleId, executionState ve gerçek decision gibi salt okunur alanlar.

İlk tipler Boolean, Int32/UInt32/Int64, Float64, String, Enum; ardından
Point/Pose3D ve gerektiğinde Array/Record. Wire tipi ayrıca seçilir; UInt16 veya
Float32'ye sığma/sonluluk/uzunluk denetimi yapılır. Sessiz truncation yoktur.
Her sonuç value + validity/reason + unit + source/cycle kimliği taşır.
Başlangıçta 0/false atanması sonuç geçerliliği sağlamaz.

### 3.2 Set Variables aracı

Properties içinde satır ekle/sil/taşı:

| Hedef | Kaynak | Denetim |
| --- | --- | --- |
| Part.Diameter | DiameterTool.Value | Geçerli, mm, aynı cycle |
| Part.SurfaceAccepted | SurfaceTool.Accepted | Gerçek Boolean tool sonucu |
| Part.Code | CodeTool.Text | Geçerli text, uzunluk |
| Pick.Target | RobotTransform.Pose | Geçerli Pose3D ve frame/calibration |

Sabit, tool portu, giriş snapshot'ı ve kısıtlı typed expression seçilebilir.
Serbest script ilk kapsam değildir. Bir tool'daki tüm satırlar önce eski
snapshot üzerinden değerlendirilir, hepsi uygunsa atomik yerel commit olur.
Örneğin B=A ifadesi aynı grubun yeni A'sını okumaz; bunun için sonraki Set
Variables tool'u gerekir. Hata halinde grubun hiçbiri atanmaz; hata nedeni
gösterilir. Aynı hedefe yinelenen satır reddedilir.

Gizli global değişken bağımlılıkları typed DAG'ı bozmaz: yazılan değerler
versioned bundle/port olarak çıkar; tüketici açıkça bu sürüme bağlanır.
Farklı dallardaki aynı hedef yazımları belirsizse validation reddeder;
sıralı yeniden atama yalnız açık dependency ile mümkündür. Rename UUID bağını
korur. Run Tool ve preview yalnız yerel değerlendirme yapar, fiziksel yayın yapmaz.

### 3.3 Yayın grupları ve fiziksel mapping

Bir ResultBundle birden fazla endpoint'e fan-out edilebilir. Örneğin aynı
çevrimde PLC'ye parça kararı/çap, OPC UA sunucusuna çap/kod, robota pose gider.
Her endpoint'in teslim durumu ayrıdır; dağıtık atomiklik iddia edilmez.
Zorunlu PLC teslimi Ready'yi engelleyebilir; opsiyonel arşiv hedefinin kesintisi
sınırlı kuyruk/alarm politikasıyla yönetilir. Sınırsız kuyruk yoktur.

Tool mantıksal alanı seçer; Communication mapping alanı nodeId/register/tag/
JSON alanına bağlar. Birim, ölçek, signedness, byte/word order, string encoding,
limit, erişim yönü ve şema sürümü görünür. Endpoint parolası reçeteye gömülmez;
yerel bağlantı profiline referans verilir. Reçetedeki mapping kimliği/sürümü
snapshot hash'ine katılır, aktif mapping sessizce değişmez.

Set Variables yerel atomikliği ağda çoklu node/register yazımının atomikliği
değildir. PLC/robot, payload'ı staging alana yazıp sequence/commit işaretiyle
tek parça kabul etmelidir; karşı tarafın uygulaması bu sözleşmeye uymalıdır.
OPC UA Write başarısı hedef uygulamanın veriyi kullandığının ACK'i değildir.

## 4. Tetik ve çevrim handshake önerisi

Akış başlamadan gelen dış tetik, akış içindeki bloklayan Wait aracına bağlı
olamaz. Ortak Trigger Router TCP komutu, Modbus register/sequence ve OPC UA
subscription olayını aynı typed TriggerRequest'e çevirir.

Örnek mantıksal akış:

```text
PLC/Robot Trigger(requestId, partId, recipe/config, inputRevision)
  → Ready/izin/kimlik ve input quality doğrulaması
  → giriş snapshot'ı + kalıcı request intent → Accepted / Busy
  → görüntü → ölçüm/kontrol → Result → Set Variables → Publish Data
  → terminal kayıt + kalıcı outbox → ResultAvailable(sequence, valid, decision)
  → tüketiciden ResultAck(sequence) → teslim kaydı → sonraki parça için Ready
```

Ready, Busy, ResultAvailable/Done, ResultValid, Decision, ErrorCode ve ACK ayrı
alanlardır. Decision: OK/NOK/NOT_EVALUATED; ERROR/CANCELED execution durumudur.
NOK geçerli kötü parça olabilir; ölçüm hatası otomatik NOK/OK olmaz. PLC'nin
ERROR/UNKNOWN için hattı durdurma veya ayrı kutuya yönlendirme politikası açıkça
tanımlanır. Süre dolması, robotun hareket etmeye yetkili olduğu anlamına gelmez.

Polling girişte kısa pulse kaybolmasını önlemek için tercih edilen model
request sequence + ACK'tir; bit protokolünde latched request/re-arm anlaşması
gerekir. Subscription ilk değeri/reconnect veya yüksek kalan bit yeni tetik
sayılmaz. Busy'de ilk varsayılan kapasite 1 ve açık ret; gelecekte sınırlı
kuyruk seçeneği. Çoklu tetik kaynakları arbitration ve tek cycle korelasyonu
kullanır. Kamera hardware trigger korelasyonu ayrıca mevcut kamera sözleşmesine
tabidir; software tetik native eski-kare engelini çözmez.

Kalıcı requestId/dedup, outbox ve ACK/recovery eski 6D.2/P7 işini kapsayacak
şekilde geliştirilir. Ağ kesintisinde aynı payload replay edilir, yeni ölçüm
başlatılmaz. Çökme anındaki intent/begin/terminal/ACK belirsizliği ayrı recovery
durumudur; fiziksel edinimin exactly-once garantisi verilmez. Yeni boot Ready
değildir ve robot komutu otomatik tekrar yürütülmez. Peer kalıcı dedup/ACK
uygulamıyorsa uçtan uca exactly-once iddia edilemez.

## 5. Robot ve hand-eye sınırı

Genel haberleşme altyapısı x,y,z,rx,ry,rz taşıyabilir; bu tek başına hand-eye
çözümü değildir. Calibration sürecindeki robot actual pose girişi ile üretim
pick target çıkışı ayrı mesaj tipleri/kanallarıdır. Sample ID, robot stationary
onayı ve görüntü/pose zaman eşleşmesi hand-eye kartında tanımlanmalıdır.

Pose3D; source/target frame, tool/TCP, uzunluk birimi, açı birimi, Euler sırası
veya rotation vector/quaternion temsili, handedness ve calibration kimliği
taşır. Rx/Ry/Rz etiketlerinden temsil tahmin edilmez. Robot profil adaptörü
tek tip iç dönüşümü üretici formatına çevirir. Geçersiz kalibrasyon veya eksik
gerçek pose için sıfır pose ya da eski hedef gönderilmez. Hedef veri teslimi,
robot hareket komutu ve robot görevi tamamlandı bilgisi ayrı durumlardır.
Hand-eye matematiği ve fiziksel robot hareket kabulü bu plan tesliminde yoktur.

## 6. Önerilen kart sırası ve kabul

Kartlar henüz uygulanmadı; bir oturumda seçili tek kart ilkesi korunur.

| Kart | Teslim | Ana kabul |
| --- | --- | --- |
| COMM-01 | Kaynak incelemesi, typed veri/variable/result/publish/trigger sözleşmesi, şema ve uyumluluk kararı | codebase-memory kapsaması; legacy vf.socket/2/no-op; DAG, scope, validity, snapshot ve recovery durumları kesinleşir |
| COMM-02 | Variable Catalog + çok satırlı Set Variables, editor persist/validation | Çoklu atama atomikliği, tip/birim hatası, eksik/skipped/stale çıktı, açık dependency, Save/reopen; preview fiziksel yazmaz |
| COMM-03 | Result kararı + Publish Data + çoklu scalar TCP mapping | Ölçüm ve yüzey OK/NOK/NOT_EVALUATED; yalnız terminal commit sonrası yayın; eski v2 peer korunur veya açık version negotiation |
| COMM-04 | Ortak Trigger Router + kalıcı dedup/outbox/recovery | Duplicate/reconnect/timeout/crash/Stop, aynı payload retry, bounded queue, session limit ve kapasite/retention; yeniden acquisition yok |
| COMM-05 | Modbus TCP adaptörü ve PLC handshake | İlk rol saha gereksinimiyle seçilir; register/byte-word order, staging/sequence, invalid sonuç, simulator ardından fiziksel PLC |
| COMM-06 | OPC UA adaptörü, ilk hedef client | Browse/read/write/subscription, namespace URI ile kimlik, quality/time, certificate trust, reconnect/re-arm ve uygulama ACK'i; server ayrı ihtiyaç kartı |
| COMM-07 | Pose3D ve robot profil adaptörü | Frame/unit/rotation doğrulaması, sample/target ayrımı, simulator; hand-eye üreticisi ve gerçek robotla ayrı kabul |
| COMM-08 | Birleşik operatör diagnostics ve saha kabulü | Normal/küçük ekran, yavaş/kopuk hedef, restart, kuyruk dolması, fiziksel PLC/robot ve gerçek parça korelasyonu |

İlk geliştirilecek protokol mevcut TCP'dir. Modbus TCP/OPC UA sırası bağlanılacak
PLC/robotun modeline ve OPC UA client/server rolüne göre değişebilir. OPC UA SDK
seçimi COMM-06'da güncel lisans/toolchain/deployment incelemesiyle yapılır;
bu araştırmada doğrulanmış SDK veya kurulum iddiası yoktur. PROFINET/EtherNet/IP
gelecekte adaptör olabilir; donanım/lisans gereksinimleri ayrıca doğrulanır.

Her UI kartında Release, ilgili CTest, normal/küçük ekran görsel kabulü gerekir.
Simülatör testleri saha kabulü değildir. Sonraki somut görev COMM-01'dir;
bu belge uygulama sözleşmesinin kesinleştiği anlamına gelmez.
