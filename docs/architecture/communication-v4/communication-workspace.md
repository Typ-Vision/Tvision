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
