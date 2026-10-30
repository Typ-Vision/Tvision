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
| `publication-templates-v4/<revision>.json` | Açıkça hazırlanmış template manifest'leri; slot editorü COMM4-17'de |
| `migration/active.json` | Tek transactional migration selection pointer'ı |
| `production/` | Migration seçimi yokken ortak production archive/ledger |

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
