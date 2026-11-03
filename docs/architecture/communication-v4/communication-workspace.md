# Tek Communication workspace — COMM4-16

`CommunicationPanel` yalnız `CommunicationWorkspace` application portuna bağlıdır.
Listener, Runtime, ledger ve migration işlemleri UI'da kopyalanmaz. Bir satırı
seçmek yalnız detayı değiştirir; port/listener generation/aktif reçete değişmez.
Connection, Run/Auto ve delivery ayrı durumlardır. Connected+STOPPED bir üretim
Ready veya parça sonucu değildir. Tablo yatay, sayfa dikey kaydırılabilir;
UUID/revision/owner ayrıntısı seçili satırda ve tooltip'te korunur.

## Process ve istasyon sahipliği

`main.cpp`, Qt AppLocalDataLocation altında machine directory seçer. Smoke
koşumları yalnız kendi `VISIONFLOW_TEST_ARTIFACTS/v4-machine` dizinini kullanır.

| Kayıt | Sahiplik |
| --- | --- |
| `communication-server-v4.json` | Makine listener profili; UI save yalnız sonraki açılışı değiştirir |
| `communication-identities-v4.json` | Güncel installation/key/retired/epoch catalogue authority |
| `communication-stations-v4.json` | Checksummed, append-only immutable station seçim geçmişi |
| `publication-templates-v4/<revision>.json` | Açıkça hazırlanmış template manifest'leri; Editor revision'ı seçer, manifest'i yazmaz (COMM4-17) |
| `migration/active.json` | Tek transactional migration selection pointer'ı |
| `production/` | Migration seçimi yokken ortak production archive/ledger |

### VF-01 · Açık makine kurulumu

Communication → Makine kurulumu → Kurulumu başlat, boş veya yalnız listener
profili bulunan makinede kalıcı installation kataloğunu ve Yüzey/Ölçüm
manifestlerini oluşturur. Publish Data → Slot ataması (v4), aynı manifestleri
ve katalogdaki görünen adları okur. Kullanıcı JSON/UUID/migration hazırlamaz.
Yüzey terminal karar/geçerlilik alanlarını kullanır; ek slot zorunlu değildir.
Ölçüm Number[0] zorunludur, varsayılan birimi unitless ve ölçeği 1/1'dir;
kalibrasyonsuz piksel çıktısı mm sayılmaz. Çıktı/birim bağlama VF-03 kapsamıdır.

`MachineSetup`, processler arası QLockFile ile yalnız kurulum işlemlerini
serileştirir. Atomik identity create → atomik manifest → identity CAS sırasını
kullanır. Hazır şablon UUID/revizyonları installation altında sürümlü UUIDv5
ile belirlenir; kesinti sonrası aynı kayıtlar tamamlanır. Tamamlanmış kuruluma
tekrar basmak katalog revizyonunu veya manifest baytlarını değiştirmez.
Mevcut identity bozuksa ya da manifest/anahtar çelişkisi varsa veri korunur ve
onarım nedeni gösterilir. Kimlik kataloğu kayıpken geçmiş/seçim/şablon verisi
bulunan dizin temiz kurulum sayılmaz. Kullanıcı şablonları ve retired anahtarlar
yeniden yazılmaz. Kurulum listener profiline, reçeteye veya geçmişe yazmaz;
Run/Auto/Remote Enable açmaz, istasyon oluşturmaz.

İlk boş kurulumda normal composition aynı süreçte yeniden oluşturulur; kayıtlı
istasyon varsa mevcut Runtime sahipleri korunarak şablonlar yenilenir.
Başlangıçta kurulum durumu salt okunur denetlenir, eylem olmadan enrollment
yapılmaz. Normal uygulamayı izole veriyle çalıştırmak için yalnız test sürecinde
`VISIONFLOW_USER_DATA_DIRECTORY` mutlak bir test dizinine ayarlanabilir; bu yol
makine kayıtları ile UI INI'sini birlikte ayırır. Smoke veya fixture gerektirmez.

Enrollment kendiliğinden oluşturulmaz. Geçerli identity authority'den sonra
eksik seçim deposu boş oluşturulabilir; bu işlem reçete/Run/Auto seçmez.
`CommunicationStationComposition`, normal main ve integration testlerinin ortak
owner'ıdır. Her station ayrı Runtime/Coordinator/controller taşır; listener
bunlardan uzun yaşamaz. Owner, shutdown sırasında station Stop/drain'i izler.
Eksik/bozuk/retired manifest production izni oluşturmaz. Seçili snapshot olsa
bile uygun immutable recipe/template/provider yoksa Run/Auto unavailable kalır.

Yerel reçete seçimi ile PLC SELECT_RECIPE aynı `StationControlService::select`
portuna gider; yeni activation compare-and-swap ile kaydedilir, Run/Auto açılmaz.
Yerel Auto ve PLC SET_AUTO aynı hazırlık/control-generation bariyerini kullanır.
Remote Enable ayrı, process-local operatör iznidir; her başlangıçta false olur.
Remote kapatmak ortak Stop fence'ini uygular. Yerel Stop her zaman açık kalır;
eski ekrandaki activation nedeniyle güvenli Stop reddedilmez.

`CommunicationPeerSessionService` exact negotiated session'ı dispatch/control ve
gerçek station STATUS'a bağlar. STATUS parça sonucu üretmez; kayıtlı snapshot,
pending ve gerçek controller hazırlığı kullanılır. W6 metadata yerel versioned
manifest içerik/dependency digest'lerinden oluşturulur; olmayan fiziksel
capability veya robot kaydı uydurulmaz. İlgisiz category manifest'i unavailable
kalır. Camera-set revision aynı immutable kaynak kümesinden elde edilir.

`StationExecutionRegistry` normal Runtime yürütmesini ve bounded worker iznini
kullanır. Durable intent olmadan worker başlamaz. Completion gerçek Runtime
commit'inden sonra immutable recipe bindings ve `FixedSlotPublication` üzerinden
seale edilir. Mevcut TERMINAL sonrası retry aynı result UUID/bytes kullanır;
acquisition/Runtime finish tekrarlanmaz. Ledger exact ACK, restart/recovery ve
pending için tek otoritedir; scheduler mevcut fair session delivery yoludur.
Pending Run/Auto'yu engeller, Stop/control ACK'yi engellemez. Integer request ID
JSON diagnostics'te de string kalır; double'a çevrilmez.

## Sağlanan ve açık kalan capability

Normal composition'ın doğrulanan production yolu Image File kaynaklarıdır.
Scalar direct output ve tek yazarlı Set Variables kaynakları aynı typed slot
evaluator'una gider. Desteklenmeyen unit/type, ambiguous bundle/çok yazar ve
whole Pose binding fail closed kalır. Pose frame/adapter entegrasyonu COMM4-19,
assignment/template editorü COMM4-17 kapsamındadır. Point/Pose2D whole Pose olmaz.
Hata/Stop terminalinde execution error/canceled, decision NotEvaluated ve invalid
slotlar korunur; eski OK/değerler taşınmaz.

## Publish slot assignment editorü (COMM4-17)

Reçete Editor'ünde `publish_data` paneli iki ayrı yetki taşır. v3 logical bundle
alanları (`publication`/OutputGroup) eskisi gibi reçetede düzenlenir. v4 sabit slot
tarafında makine `publication-templates-v4/*.json` manifest'leri **salt okunur**
listelenir; UI yalnız reçete kaynağını (`publicationV4` assignment referansları)
bağlar. Template adı, required bayrağı, birim, ölçek/offset, aralık ve Pose
sözleşmesi manifest otoritesidir; Editor bunları değiştirmez. Revision değişimi
hâlâ açık migration/activation işidir.

Kaynak listesi cycle-local Variable veya doğrudan typed tool output portudur.
Enum ve whole Pose aday olarak görünür fakat açık adapter eksiği nedeniyle
`blockReason` ile kapalıdır; Point2D/Pose2D whole Pose'a yükseltilmez. Birim
eşlemesi `publicationUnit` üzerinden tek yerden yapılır; bilinmeyen birim
(ör. `px`, `gray8/px`) sessizce dönüştürülmez, kaynak reddedilir.

Satır bazında durum sözleşme denetimini gösterir: uyumsuz port, eksik required
slot, kaybolan kaynak. Değer/wire önizlemesi yalnız bu tanımla eşleşen kabul
edilmiş çevrim varsa ve aynı `FixedSlotPublication` evaluator'ı üzerinden yapılır;
aksi halde panel "değer önizlemesi yok" der ve sıfır pose/sahte PASS üretmez.
Runtime yolu ve Editor yolu aynı `publicationBindings` çözümlemesini kullanır, bu
yüzden preview ile üretim aynı tip/birim/revision otoritesine sahiptir.

Apply atomiktir: bundle ve assignment birlikte tek `ProjectService` adımında
yazılır; değişiklik yoksa byte no-op olur, hata durumunda önceki tanım korunur ve
Taslağı geri al diski/tanımı değiştirmez. Editor önizlemesi ağ, ledger veya
terminal üretmez.

Native/webcam physical identity/provider composition bu owner'da üretilmez.
Bu kaynaklar için uygun normal production provider/manifest olmadığı açıkça
gösterilir. OECS correlation/drain ve bağımsız fiziksel mm/robot kabulü açılmaz.
Sentetik/file fixture veya UI'da Auto göstermek fiziksel kabul değildir.

## Migration ve catalogue authority

UI read-only analiz için bir migration request JSON dosyasını okur. İşlemler
ayrı explicit düğmelerdir: analiz → backup+manifest → seçim uygula; rollback
için ayrıca pointer/ledger review gerekir. İstek en fazla1MiB'dir:

```json
{
  "schema": "visionflow.communication-migration-request/1",
  "project": "D:/machine/project.vfproject",
  "profiles": "D:/machine/communication-profiles.json",
  "binary": "D:/installed-legacy/visionflow.exe",
  "registry": "<machine-directory>/communication-identities-v4.json",
  "archive": "D:/machine/project.vfproject.production",
  "station": "<explicit station UUID>",
  "publication": { "...": "tam PublicationTemplate.toJson nesnesi gerekir" },
  "serverProfile": "<machine-directory>/communication-server-v4.json",
  "workspace": "<machine-directory>/migration",
  "package": "<yeni canonical UUID>"
}
```

Bu şema örneği doğrudan çalıştırılabilir fixture değildir. Publication için tam
strict şema, gerçek yollar ve disabled profil gerekir. Source/profile/binary
dosyaları normal ürünün bağlı olduğu installation ile eşleşmelidir. Main
arbitrary workspace veya yedek registry'yi authority olarak seçmez. Analiz
unsupported/pending nedenleri ve UUID/numeric mapping'i salt okunur raporlar.
Yerel/runtime/kamera işleri ve listener açıkken mutating bakım reddedilir.
Başka legacy binary'nin kapalı olması ayrıca operasyonel önkoşuldur.

Explicit apply tek `migration/active.json` pointer'ını yazar. O process yeniden
başlayana kadar Run/Auto ve yeni selection engellenir. Başlangıç listener'dan
önce pointer/manifest/staged bytes hash'lerini doğrular; bozuk seçim aktif
listener'a sessiz fallback yapmaz. Kullanıcı root server profilini kaydederken
selection pointer'ı yazılmaz. Migration'ın disabled profili ağ izni oluşturmaz;
root machine listener profili ayrıca explicit operatör ayarıdır.

Source registry tam güncel live authority olmalıdır. İlk restart'ta live
catalogue immutable baseline ile eşitse staged catalogue CAS ile benimsenir;
yarıda kesilirse sonraki restart aynı seçimi tamamlar. Live catalogue zaten
seed veya geçerli monotonik devamıysa korunur. Installation, eski key kimlikleri,
retired ve epoch geçmişini kaybeden geçiş reddedilir. Eski yedek üzerine
kopyalanmaz. Migrated recipes orijinal project directory'den asset çözer;
paket klasörü yanlışlıkla görüntü kökü yapılmaz.

Rollback source project/profile/binary kimliğini ve current pointer/ledger'i
yeniden doğrular. Live registry staged seed'in geçerli devamı olabilir; yeni
key/retired/epoch kayıtları geri alınmaz. Live archive ve yeni v4 ACK/tombstone
yerinde kalır. Legacy selection sonrası normal main v4 listener açmaz; eski
kurulu binary'yle kontrollü dönüş ayrı operatör işlemidir.

## Kanıt sınırı

`communication_workspace_contract`, `communication_station_composition_contract`
ve `ui_communication_workspace` gerçek CTest adlarıdır. İkinci test normal main
owner'ını kullanır: üç Runtime, negotiated session/catalogue, TRIGGER → terminal
→ exact ACK, Number15/Flag31, local/remote Auto ve Stop sonrası canceled terminal.
Migration testi ayrıca startup adoption/monotonik authority/rollback'i sınar.
UI testi normal/küçük offscreen görüntü, seçim/listener izolasyonu, pending,
camera owner ve klavye/scroll kontrolü üretir.

Windows native/DPI etkileşim kabulü **açıktır**. Kilit ekranı nedeniyle native
computer-use durduruldu; kullanıcı uzak bağlantıdan kilidi açamayacağını belirtti.
Offscreen veya QWidget grab bu kabulün yerine geçmez. Tamamlanan yazılım
doğrulaması bu sınırla birlikte [devir/arşivde](../../progress.md) raporlanır.


## Operatör akışı ve migration devri

COMM4-26 [kabul raporu](acceptance-comm4-26.md) son yazılım koşumunu ve normal
ürün/fixture/fiziksel sınırlarını ayırır. Tek Communication ekranında:

1. Makine listener profilinde adres/port ve enabled durumunu açıkça kaydet;
   değişiklik sonraki açılışta kullanılır. Açılışta listen, station Run/Auto açmaz.
2. Hedef station'ı seçip kaydedilmiş reçete revision'ını yükle. Bu seçim yalnız
   ilgili station aktivasyonunu değiştirir; listener/session korunur. Bekleyen
   sonuç varsa exact ACK/recovery tamamlanmadan yeni iş kabul edilmez.
3. Run ile kaynakları hazırla; Auto'yu ayrıca aç. PLC kontrolü için açılışta kapalı
   Remote Enable ve controlling peer yetkisi gerekir. PLC sırası exact catalogue
   ve HELLO sonrası SELECT_RECIPE → SET_AUTO → READY → TRIGGER'dır.
4. Publisher'ın reçete assignment'larını Editor'da machine template slotlarına
   bağla; Variable veya direct typed output seç ve atomik Apply yap. Editor
   önizlemesi ağa göndermez. Execution/Result/resultValid ayrımını koru.
5. Pending ACK, station çalışma durumu ve kamera sahibini ayrı kontrol et. Stop
   admission'ı kapatıp drain ister; pending terminali silmez. Reconnect/recovery
   aynı sonucu döndürür; consumer kendi dış etkisini idempotent işlemelidir.

Normal V4 production composition bu kabulde Image File'dır. Native/webcam
provider ve gerçek robot calibration adapter composition açık iştir. Calibration
ekranında PHYSICAL BLOCKED/disabled nedenlerini gideren gerçek kanıt olmadan
fixture ACTIVE veya ekrandaki düğmeler üretim izni oluşturmaz.

Legacy v2/v3 geçişinde önce read-only analiz yap; unsupported mapping ve eski
pending ACK listesini değerlendir. Pending sonuçları kurulu eski binary/profille
drain et. Yeni binary ve eski binary aynı socket/ledger'i eşzamanlı kullanmaz.
Explicit backup+manifest oluştur; hash/UUID mapping ve disabled staged profili
inceledikten sonra Apply yap ve yeniden başlat. Yeni listener profilini ayrıca
operatör etkinleştirir; migration kendiliğinden Run/Auto veya ağ izni açmaz.
Rollback için current pointer/ledger revision kontrolü ve yeni V4 pending
drain/reconcile gerekir; eski dosyaları live registry üstüne kopyalama.
Monotonik numeric key, epoch, dedup/tombstone ve ACK geçmişi korunur.
Uyumsuz text/fan-out/Pose mapping'i sessizce küçültülmez; analiz raporundaki
unsupported nedenleri çözülmeden apply edilmez.


## VF-02 · İstasyonun kalıcı dosya kaynağı

Onaylı [V4 kullanıcı uygulama planı](../visionflow-v4-uygulama-plani.md),
VF-02/K1: Communication → İstasyon ekle → ad → mantıksal kamera → test
görüntüsü → Kaydet. Kaynak ayarları / adlandır seçili satırı düzenler;
dosya kaybolduğunda aynı eylemle yeni dosya seçilir. Kaydet görüntüyü
OpenCV ile gerçekten decode eder; okunamayan/geçersiz veya 64 MiB üzeri
dosya mevcut eşleşmeyi değiştirmez. Önizleme piksel görüntüsüdür, araç sonucu
veya fiziksel kamera kabulü değildir.

StationFileSource (station, camera, revision, absolute path) makinenin
IdentityCatalogue.stationSources alanına aittir; recipe tool UUID'si veya
StationRecipeSelection dosya yolu değildir. İstasyon/kamera anahtarları, ad ve
tam kaynak eşleşmesi aynı checksummed identity snapshot'ında tek QLockFile +
revision CAS + QSaveFile commit ile kaydedilir. Kısmi yazımda önceki kayıt
korunur; ilk kayıtta kimlik-only istasyon bırakılmaz. Anahtarlar yeniden
kullanılmaz. Ad değişikliği kamera/kaynak kimliğini değiştirmez; farklı dosya
yolu yeni kaynak revizyonu üretir. Aynı dosya/ad tekrar kaydı no-op'tur.

İlk açık kaynak kaydı JSON payload schemaVersion=2 üretir. Eski schemaVersion=1
boş stationSources ile okunur ve kaynak eklenmedikçe aynı biçimde yazılır.
SchemaVersion=2 eksik/çift kaynak, bilinmeyen alan, hatalı kimlik veya checksum
ile reddedilir. VF-02’de StationActivationCatalogue biçimi değişmedi; VF-04 aşağıdaki sürümlü
çözülmüş kaynak kaydını ekler ve eski manifest baytlarını korur.
Eski activation'lardan kamera otomatik çıkarılmaz; Kaynak ayarları ile bir
kez açık eşleme yapılır. İlk ürün sürümünde kaydedilmiş kamera kimliği korunur
ve başka istasyona sessizce atanmaz.

Kaydet, owner thread üzerinde bütün Runtime'ların drained olmasını, Remote
Enable'ın kapalı olmasını, hedef istasyonda pending terminal bulunmamasını ve
Editor/Calibration/Runtime bakım bariyerini denetler. Yeni katalog satırı hemen
görünür; mevcut server/run owner'ları yerinde değiştirilmez. Panel açıkça
uygulamanın yeniden başlatılmasını ister; o süreçte Run/Auto/Remote Enable ve
reçete seçimi engellenir. Sonraki normal composition istasyon kimliklerini
yeniden bağlar, başlangıç Run/Auto/Remote Enable kapalı kalır.

VF-02 kaynak kaydı üretim reçetesi seçmez. VF-04 makine kaynağını reçetenin
tool kimliklerine çözümler; admission ve execution aynı çözülmüş snapshot’ı
kullanır. Eski reçete dosyasına sessiz dönüş yoktur. Gerçek TCP üzerinden A/B
kabulü VF-08’e aittir.

Kalıcılık, bozuk görüntü, stale CAS, kilit, yarım yazım ve legacy/restart
korumasının hedefli testi `station_source_setup_contract`'tır. K1 ürün kabulü
normal başlatıcı ve izole kullanıcı verisiyle, elle katalog üretmeden yapılır.
Güncel sonuç ve kanıt yolları [devir özetindedir](../../progress.md).

## VF-03 · Değişmez reçete yayımlama

Recipe Editor → reçeteyi aç → Publish Data → hazır Yüzey/Ölçüm şablonunu seç →
zorunlu çıktıyı bağla → **İstasyonda yayımla**. Düğme Save'den ayrıdır: taslak
kaydedilmez, yalnız diskteki kayıtlı proje yayımlanır. Kaydedilmemiş değişiklik
açık hata verir. Yayımlama istasyonun yüklü reçetesini, activation'ını,
stationSources kaydını veya sonuç geçmişini değiştirmez.

Yayın, `publication-catalogue-v4.json` içinde checksum'lı tek katalogdur;
kayıt alanları recipeKey, revision, templateKey, recipeId, proje dizini,
publishedAt, tam reçete içeriği ve içerik SHA256'sıdır. İçerik = reçete
baytları + bağlı şablon id/revision; şablon değişimi yeni revizyon üretir.
Aynı içerik tekrar yayımlandığında baytlar ve anahtar değişmez (no-op).
Değişen içerik yeni RecipeRevision katalog anahtarı ve yeni revision alır;
önceki kayıtlar salt okunur kalır.

Sıra kesintiden sürdürülebilir: identity CAS ile önce değişmez revision kimliği
ve katalog anahtarı bağlanır, sonra QLockFile + QSaveFile ile katalog atomik
yazılır. Yarım kalan dosya hiçbir zaman yüklenebilir sayılmaz; aynı içerikle
yeniden deneme aynı revision/anahtarı üretir. Bozuk katalog sessizce
sıfırlanmaz; mevcut dosya korunur ve yayımlama reddedilir.

Yayımlamadan önce aynı `FixedSlotPublication` doğrulaması çalışır: yanlış
tip/birim, eksik zorunlu slot, çözülemeyen kaynak veya DAG döngüsü hiçbir dosya
yazmadan reddedilir. Ölçüm şablonunun Number[0] slotu zorunludur; birim/ölçek
sözleşmesi (`Int32 • mm • scale n/d • offset n/d`) editörde görünür. Eksik veya
geçersiz ölçüm geçerli sıfır olarak gösterilmez; editör satırı `✗ Gerekli slot
atanmadı` / `✗ Değer üretilmedi` der. Yüzey şablonunda ek slot gerekmez.

Composition yayın kataloğunu normal açılışta ve dosya değişiminde migration
paketi olmadan okur; Communication reçete seçicisinde `• yayım` etiketiyle
listeler. Katalog makine kurulum kimliğiyle eşleşmeli, tüm anahtarlar canlı
identity kataloğunda ve şablon manifesti mevcut olmalıdır; aksi kayıt listeye
girmez. Liste keşfi activation üretmez; açık Dashboard/Communication yüklemesi
VF-04 çözümlemesini kullanır. Kalıcı istasyon kaynağı olmadan yeni yayın
yüklenemez; başarısız çözümleme activation veya Run/Auto üretmez.

Hedefli testler `publication_publish_contract` (tip/birim/eksik slot/unsaved/
kesinti/no-op/yeni revizyon/bozuk katalog/activation korunumu) ve
`ui_publication_v4` (gerçek editör düğmesi + Communication listesi) olarak
çalışır.

## VF-04 · Dashboard’dan reçete yükleme ve istasyon kaynağı

Dashboard → İstasyonlar → satır → yayımlanmış revision → **Uyumluluğu denetle**
→ **Reçete yükle**; sonraki seçimde **Reçete değiştir**. Satır istasyon, bağlı
kamera, yüklü reçete/revision ve çalışma durumunu gösterir. Önizleme salt okunur;
yükleme aynı uyumluluk denetimini tekrar yapar. Çalışan istasyon önce Stop/drain
olmalıdır; bekleyen üretim veya kontrol terminali exact ACK/reconcile bekler.
Başarı yeni activation üretir, Auto kapalı kalır. Ret mevcut seçimi, kamera
eşlemesini ve activation’ı korur. Editor açma/kaydetme/yayımlama seçimi değiştirmez.

`StationRecipeSelection.recipe` yayımlanmış içeriğin tamamını aynen tutar.
Ayrı `resolvedSource`, makineye ait station/camera/source revision/absolute path
kaydını dondurur; `cameras` yeni reçetenin source tool UUID’sini aynı makine
kamerasına bağlar. İlk dosya kaynağı sürümünde tam bir etkin Image File gerekir;
çoklu kaynak, Webcam/Industrial Camera veya eksik setup/kalibrasyon reddedilir.
İstasyonun dosyası seçimde gerçekten decode edilir. Kaynak yapılandırması ve
reçete içeriği aynı şey değildir: `stationPublicationBytes` reçetenin bütün
alanlarını, revision/key ve proje dizinini karşılaştırır; `stationRecipeBytes`
çözülmüş kamera/kaynak kaydını da içerir. Yeni yol eski manifest denetimini
devre dışı bırakmaz. Legacy seçimler tam eski kamera/manifest eşitliğini korur.

Activation kataloğu schema 1’i okur; yalnız çözülmüş seçim varsa schema 2 yazar.
Schema 2 her seçimde kaynak varlığı ve varsa dondurulmuş kaynak alanlarını taşır.
Eski activation ve canonical manifest baytları geçişte korunur; geçmiş kayıtlar
yeniden seçilmiş sayılmaz. Yeni kod hem sürümü okur; eski binary schema 2’yi
reddeder. Geçişte aynı dosyayı eski/yeni sürümle eşzamanlı kullanmayın.
Checksummed QSaveFile/QLockFile ve append-only revision CAS aynı kalır.

Yerel ve yetkili uzak SELECT_RECIPE aynı resolver, configuration ve drain
kapısından geçer. Kaynak identity revision’ı configuration sonrasında yeniden
denetlenir; reentrant seçimde yalnız CAS kazananı kalır. Admission manifest’i
güncel makine kaydıyla exact resolved snapshot eşitliği ister. Runtime yükleme
ve configuration için `stationExecutionRecipe` yalnız kopyadaki Image File
yolunu çözer; executor aynı kopyayı ticket snapshot’ıyla karşılaştırır. Sonuç
yayını aynı admission snapshot’ına bağlıdır. Kamera-set revision’ı kaynak
revision/yolunu da kapsar. Recipe metadata v2 istasyona özgü kamera listesi
yerine değişmez içeriği ve gerekli kaynak sayısını taşır; legacy metadata v1’dir.

Restart son başarılı seçimi/kaynak snapshot’ını geri getirir; Run, Auto ve
Remote Enable açılmaz. Kaynak kaydı sonradan değişmişse eski seçime üretim izni
verilmez; durdurulmuş istasyonda açık yükleme gerekir. Fiziksel kamera/PLC/robot
ve kalibrasyon doğruluğu bu dosya kaynağı kabulüyle kapanmaz.

`station_recipe_selection_contract`: farklı kaynak UUID’li A/B, beyaz istasyon
ve siyah reçete görüntüsü, gerçek worker pikseli ve composition terminal Flag0,
uyumsuz tür/sayı/kalibrasyon, eski activation, missing file, pending ACK,
iki istasyon izolasyonu, source/selection CAS yarışları ve schema 1→2 geçişi.
`ui_station_dashboard`: normal executable, ürün kurulum/kaynak dialogu, gerçek
Editor yayımlama düğmesi, Dashboard A→B/uyumsuz ret, iki executable restart ve
1400×900/850×600 görselleri. Her koşum yeni artifact dizinindedir. Bağımsız
simülatörle TCP uçtan uca B görüntüsü kabulü VF-08’de yapılır.

### VF-04a · Tek dosyada editor9 kaynak kümesi

Editor schema 9, yalnız bir Image File olduğunda da whole-source Runtime yolunu
kullanır. Station composition configuration-only ve operational provider’larını
ayrı bağlar. `fileSources` tam grafiği ve tek dosya kaynağını doğrular; schema 9
korunur, yayımlanmış içerik değiştirilmez. Kurulum/kalibrasyon isteyen reçeteler
gerçek bağlı provider olmadan boş evidence ile kabul edilmez. Bu piksel yolu
kalibrasyonlu mm veya native kamera kabulü oluşturmaz.

İstasyon/kaynak kaydından sonra restart beklenirken preview de açık yeniden
başlatma mesajı verir; `Station runtime not bound` genel hatasına düşmez.
`station_recipe_selection_editor9_contract`, eski biçimin yanı sıra modern tek
Image File ile gerçek admission/worker/terminal ve kaynak değişmezliğini sınar.
Dashboard UI kabulü artık modern schema 9 yayını kullanır.

## VF-05 · İstemci kaydı ve test profili

Communication → **İstemciler** bölümü kayıtlı kontrol kimliklerini listeler. **Yetkili
istemci kaydet** yeni rastgele peer ve epoch üretir; kullanıcı UUID yazmaz. Tek etkin
kimlik kuralı katalogda zorlanır: etkin bir epoch varken ikinci kayıt reddedilir ve
mesaj önce `Epoch yenile` eylemini söyler. Kayıt ve yenileme bütün station
Runtime'larının drained olmasını ve Remote Enable'ın kapalı olmasını ister; yenileme
ayrıca bekleyen normal/kontrol teslimini (exact ACK/reconcile tamamlanmadan) reddeder.

`replaceControllingEpoch` eski etkin epoch'u aynı katalog revision'ında emekliye
ayırır ve yeni epoch'u ekler; tarih silinmez, retired epoch yeniden açılamaz. Peer
kimliği yenilemede korunur, yalnız epoch döner. Bu bir kimlik geçmişi işlemidir;
admission, Run/Auto veya üretim izni üretmez. `CommunicationPeerSessionService`
zaten tek etkin epoch dışında HELLO'yu `CONTRACT_MISMATCH` ile reddettiğinden eski
profil sessizce çalışmaya devam etmez.

**Test profilini dışa aktar** 64 KiB sınırlı, strict şemalı
`visionflow.communication-client-profile/1` JSON'u yazar: schema, label, installation,
peer, epoch, listener adresi/portu ve sözleşme revizyonu. Profil credential veya
üretim izni değildir; içe/doğrula yolu kurulum kimliği, aktif epoch, emekli epoch ve
sözleşme revizyonu eşleşmesini denetler. Başka makine kurulumuna ait profil, bilinmeyen
epoch, emekli epoch, eksik/fazla alan ve farklı sözleşme kapalı reddedilir. Profil
yazımı QSaveFile ile atomiktir.

`scripts/communication_v4_simulator.py --profile <json> --log <jsonl>` ürünün
dışa aktardığı profili okur; `--peer/--epoch` verilmezse profil kimliği, `--host/--port`
verilmezse profil bağlantısı kullanılır. Açık `--peer/--epoch` override'ı yalnız elle
sınamada kalır. Profil okuyucu strict'tir (bilinmeyen/eksik alan, sözleşme, sıfır UUID
ve port sınırı); discovery veya üretim yetkisi üretmez. Gerçek PLC'ye aktarım ayrı
operatör işidir: adres/port, peer ve epoch değerleri PLC yapılandırmasına elle girilir.
Gerçek TCP tetik/terminal/ACK akışı VF-07'ye aittir.

Hedefli testler `communication_clients_contract` (kayıt, ikinci etkin ret, yenileme,
pending ret, restart kalıcılığı, profil şema/uyum redleri ve bağımsız Python
HELLO) ve genişletilmiş `ui_communication_workspace` (panel düğmeleri, export/verify
dosya akışı, emekli profil reddi, normal/küçük ekran görselleri).
