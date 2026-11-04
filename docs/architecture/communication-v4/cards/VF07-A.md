# VF07-A · K2 operatör akışı: Result, V4 Publish Data ve Dashboard Stop

Durum: **A–D uygulandı; hedefli yazılım kabulü kayıtta, K2 gerçek operatör teyidi açık**.
Tarih: 4 Kasım 2026. Planı hazırlayan: Codex.
Çalışma kökü: `D:/TVision`. İncelenen HEAD: `4505106` (VF-07).
Önkoşul: VF-01–07 yazılım zinciri. Sonraki: **bu kart kapanışı ve K2 operatör tekrarı → VF-08/K3**.
Kimlik: kullanıcının istediği **VF07-A**; VF-07'nin düzeltme/operatör kullanılabilirliği kartıdır.

## Güncel teslim ve doğrulama kararı · 4 Kasım 2026

Uygulayıcı: Codex. [Uygulama/kabul kaydı](../../../archive/2026-11-04-vf07-a.md).
Kullanıcının bu oturumdaki açık talimatı: tüm ON/OFF testleri yapılmayacak.
Bu talimat aşağıdaki planın tam ON/OFF yükümlülüğünü bu teslim için değiştirir;
hedefli Release davranış/negatif/UI testleri ve iki boyutta görsel kontrol uygulanır.
Geçmiş suite başarıları yeni koşum olarak sunulmaz. Native/fiziksel ve gerçek
K2 operatör teyidi ayrıca açıktır; VF-08 otomatik başlatılmadı.

Result seçimi/refresh ve target-only düzeltildi. Publish Data V4-only oldu;
legacy veri roundtrip ve görünmeyen enabled korundu. Dashboard Stop tüm sahipleri
kapsar, pending'i silmez. Revizyon seçici tarih/son/yüklü bilgisini taşır.
Eski TCP NOK, aynı recipe11'de farklı dosyalardan tekrar üretildi (rice.jpg OK,
P1.bmp NOK); kullanıcı makine kaydı değiştirilmedi.

## 1. Amaç ve kapsam

Operatör, Recipe Editor'da gerçek kararı kaybetmeden akışı doğrular; Publish Data'da yalnız V4 şablon/slotlarını görür; Dashboard'dan Stop verir, doğru yayımlanmış revizyonu seçer ve durdurma/teslim koşulları sağlandığında reçeteyi değiştirir. Bu kart, VF-08'in iki istasyonlu kabulünü başlatmaz.

Teslim edilecek dört bağlı değişiklik:

1. Result/çevrim araçlarının kaynak seçimi ve sonuç önbelleği tutarsızlığının düzeltmesi.
2. Publish Data'daki V4 öncesi düzenleme kontrollerinin kaldırılması; V4 doğrulamasının ayrılması.
3. Communication'daki Stop/drain işleminin Dashboard'a **Stop** adıyla, tüm istasyonları kapsayan tek işlem olarak taşınması.
4. Aynı operatör akışındaki revizyon seçimi ve yüklemeye hazır mesajının anlaşılır hale getirilmesi.

Kapsam dışı: wire revizyonu, yeni protokol, katalog/ledger temizleme, geçmiş revizyon silme, yeni ölçüm aracı, robot/PLC fiziksel kabulü, VF-08'in aynı sokette ölçüm/A→B/kurtarma matrisi. Genel legacy motorun tamamen silinmesi bu kartın işi değildir.

## 2. Gözlem ile teknik çıkarımı ayır

### 2.1 Operatör kanıtı

- Kullanıcı ekranında Image_File_1 DONE (512×512), Preprocess_1 DONE, Blob_1 DONE; count=88 ve found=true.
- Result_1 Enabled açık, ALL ve Blob_1/found seçili. Buna rağmen Flow'da READY; Results listesinde Result yok ve genel karar NOT_EVALUATED.
- Result seçiliyken Viewer boş; Run Tool, `Acquire this source first` bildiriyor.
- TCP örneğinde DONE/NOK/valid=true ve exact ACKED var. Son okunan `build/k2-trigger.jsonl` request=10, recipeKey=11 için de bu sonucu taşıyor. Bu, editördeki NOT_EVALUATED ile aynı hata olarak kabul edilmez.
- Kullanıcı aynı adlı üç yayın görüyor; UUID'lerden yenisini ayırt edemiyor. Önizleme hazır derken yükleme STATION_NOT_DRAINED ile reddediliyor.

### 2.2 Kaynaktan doğrulanan hata zinciri

| Yer | Mevcut davranış ve etkisi |
| --- | --- |
| `src/domain/ToolGraph.cpp`, validateGraph/source çözümlemesi | Result, Set Variables ve Publish Data çevrim düzeyindedir; tekil toolSources kaydı atanmıyor. Bu tasarım çok kaynaklı çevrimlerde gereklidir; bu araçlara rastgele kamera atamak çözüm değildir. |
| `src/app/MainWindowEditor.cpp`, refresh akışı | Seçilen araç için sourceForTool çağrılıp selectSource uygulanıyor. Çevrim aracında kaynak boş olduğundan görüntü seçimi temizlenebiliyor. |
| `src/application/EditorSession.cpp`, selectSource/evaluationScope/cacheResult/reconcile | Boş kaynağın scope'u seçili frame'e dayanıyor. Viewer seçimi frame'i temizleyince scope değişiyor; önbellek girdisi ve cycle.tools içindeki Result kaybolabiliyor. Görsel seçim, çevrim kanıtını değiştirmemeli. |
| `EditorSession::evaluation`, `MainWindowExecution.cpp::runExecution` | Bütün target-only araçlar ImageFrame taşıyan değerlendirmeye yönlendiriliyor. Result'ın kendi resmi olmaması görüntü edinilmedi hatasına dönüşebiliyor. |
| `ExecutionService.cpp`, cycle decision | Eksik/geçersiz Result'ta NOT_EVALUATED koruması doğru; karar zorla OK yapılmamalı. |

Bu zincir güçlü kaynak bulgusudur. Uygulamanın ilk adımında tekrar üretme ve başarısız regresyon testiyle doğrulanacak; henüz bu kart için yeni test koşumu yapılmış değildir. TCP NOK'un kökü ayrıca istasyonun immutable yayın içeriği, kamera dosyası, Blob/Result gerçek çıktısı karşılaştırılarak bulunacak. Beklentiyi NOK'a çevirmek hata çözümü sayılmaz.

## 3. A — Result ve çevrim araçları

### Davranış sözleşmesi

- Tam Run Flow sonrası Result'ın DONE ve geçerli OK/NOK sonucu, araç seçimi/sayfa çizimi/Viewer seçimiyle silinmez. Gerçek girdi veya ayar değişikliği ise eski kararı geçersiz kılar.
- Görüntü aracı bağlamı ile çevrim bağlamı ayrılır. Result/Set Variables/V4 slot önizlemesi tek bir fiziksel kaynağa zorla bağlanmaz.
- Viewer'da son açık görüntü korunabilir; çevrim aracının kendi görüntüsü yoksa açıklama gösterilir. Bu seçim sourceFrames/cache/cycle kanıtını değiştirmez. İki kaynaktan rastgele biri karar kaynağı gibi gösterilmez.
- Result Run Tool, aynı geçerli çevrimin Boolean üreticilerini kullanarak yalnız Result'ı yeniden değerlendirir; yeni edinim veya gizli Run Flow yapmaz. Gerekli güncel kanıt yoksa `Önce Run Flow ile gerekli kaynakları ve araçları çalıştırın` ve eksik üretici adı gösterilir.
- Target-only bağlamı birden fazla kaynaklı Result için de doğru cycle/config/provenance taşır. Yeni sahte frame, cycle UUID veya yeniden etiketlenmiş eski port üretilmez. Aynı sözleşme Set Variables ve slot önizlemesinde denetlenir.
- Önbellek geçerliliği tüm gerçek bağımlılıkların frame/cycle/config/dependency kimliklerine dayanır; seçili Viewer'a dayanmaz. Geniş bir yeni cache framework yerine mevcut değerlendirme yapısındaki en küçük ortak düzeltme uygulanır.
- Eksik/disabled/ERROR/SKIPPED Boolean veya eski port revizyonu → NOT_EVALUATED ve neden; found=false olan geçerli ALL girdisi → NOK. found=true → OK yalnız bütün gerekli girdiler gerçekten geçerliyse.
- Editor dışarı paket göndermez. Genel karar, execution durumu ve tek aracın ölçüm PASS/FAIL'i ayrı tutulur.

### Sıralı uygulama

1. İzole dosya girdisiyle Image File → Preprocess → Blob → Result kur. Run Flow, Result seçimi, tekrarlı refresh ve Run Tool sırasını hatalı kaynakta test et.
2. Değerlendirme/scope ayrımını EditorSession ve ExecutionService sınırında düzelt; bütün çağıranları tara. UI yalnız bu portları kullansın.
3. Result seçimi Viewer'ı değiştirse bile sonucun korunmasını ve gerçek değişiklikte silinmesini test et.
4. Aynı image ve aynı kayıtlı revizyonla Editor/Runtime/TCP kararı karşılaştır. İstasyon farklı dosya kullanıyorsa bunu açıkça kaydet; eşit sonuç varsayma.

## 4. B — Publish Data yalnız V4

### Kaldırılacak arayüz

V4 Publish Data Properties içinden aşağıdakiler ve bunların boşluk/layout/sinyal bağları kaldırılacak; katlanmış bir legacy panel olarak bırakılmayacak:

- Eski mantıksal paket/Set Variables bundle/Setup Communication JSON eşlemesi açıklaması.
- Output group UUID (`publishGroup`), Kaynak bundle (`publishProducer`), Endpoint UUID (`publishEndpoint`).
- Required (ACK beklenir) (`publishRequired`), Logical field/Bundle field tablosu ve eski alan açıklaması.
- + Grup/− Grup/+ Alan/− Alan, Eşlemeyi çöz / önizle, Revision bağını yenile; eski durum/hata alanları.

### Kalacak ve netleşecek arayüz

- Makine şablonu seçimi; şablon revizyonu ve sözleşmesi salt okunur.
- Number/Flag/Text/Pose slotları, filtre, typed kaynak seçimi, Slot önizle, Atamayı temizle, zorunlu slot ve tip/birim/frame/ölçek açıklamaları.
- `Paketi uygula` adı **Atamaları uygula** olur; Taslağı geri al kalır. Şablon veya slot seçimi ancak başarılı atomik Apply ile reçeteye yazılır.
- Kısa açıklama: `Genel karar ve sonuç geçerliliği terminale otomatik eklenir. Slotlar ek veriler içindir. Önizleme veri göndermez.` Yüzeyde ek slot zorunlu değildir; Ölçümde Number[0] zorunludur.
- V4 yapılandırması için eski Publish Data yürütme Enabled kutusunu açmak veya Run Tool yapmak gerekmez. Bu kontroller Publish Data Properties'ten kaldırılır; mevcut serialized enabled değeri görünmezken sessizce değiştirilmez. V4'ün gerçek çalışırlığı şablon/assignment doğrulamasıdır.

### Veri uyumluluğu ve çağıranlar

`PublishDataEditor::parameters/groups/validateDraft`, `MainWindowEditor::readToolProperties/commitToolProperties` ve ProjectService::updatePublication birlikte ele alınacak. Yalnız widget gizlemek yeterli değildir: bugün Apply eski groups/endpoint doğrulamasından da geçiyor.

- Yeni V4 reçetesi eski group/endpoint/profile bağı üretmez veya istemez.
- Var olan legacy groups, tool.parameters ve publication geçmişi açma/kaydetme/slot-only Apply sırasında korunur. Silinmiş UI'nin varsayılan boş değeriyle eski veri ezilmez; değişmemiş aç/kaydet no-op hash korunur.
- Eski gönderim yolu V4 sanılıp otomatik etkinleştirilmez. Aktif legacy bağ nedeniyle V4'e güvenli geçiş yapılamıyorsa kayıt korunur, açık geçiş/uyumsuzluk nedeni gösterilir; toplu otomatik temizleme yapılmaz.
- Legacy parser/codec/history desteği yalnız bu UI temizliği nedeniyle kaldırılmaz. Eski kontrolleri kullanan UI testleri gerçek V4 davranışını sınayacak şekilde güncellenir; eski veri roundtrip testleri korunur.

## 5. C — Dashboard'da Stop

### Kapsam kararı

Dashboard'da Otomatik üretimi başlat düğmesinin yanında tek **Stop** bulunur. **Tüm kayıtlı ve Runtime'a bağlı istasyonları durdurur; satır seçimine bağlı değildir.** Tooltip/erişilebilir açıklama: `Tüm istasyonları durdurur; bekleyen sonuçları silmez.` Reçete yükleme/değiştirme seçili istasyona uygulanmaya devam eder.

Communication'daki `Stop / drain` düğmesi kaldırılır. Bu kart diğer Communication Run/Auto kontrollerinin genel yeniden tasarımı değildir. Editor Stop, kamera preview Stop ve protokoldeki Stop semantiği korunur.

### Teknik davranış

1. UI tek bir application/workspace stop-all portunu çağırır; UI içinde Runtime/SDK çağrısı veya ayrı state makinesi olmaz.
2. Aktif toplu başlatma iptal edilir. Mevcut rollback yalnız o başlatmanın açtığı istasyonları durduruyor; global Stop **önceden çalışanlar dahil bütün bağlı istasyonlara** Stop iletir. İptal tek başına global Stop sayılmaz.
3. Her istasyonda Auto kapanır, yeni tetiğe admission engellenir, devam eden iş mevcut coordinator cancellation ve camera drain yoluyla sonlandırılır. Kuyruktaki geç hazırlık cevabı Run/Auto'yu yeniden açamaz.
4. Tek istasyonda hata diğerlerine Stop iletimini engellemez. Hatalar istasyon adı ve nedeni ile biriktirilir; kısmi durum gerçek satırlardan izlenir.
5. Geçiş sırasında `Durduruluyor`, başarıda `Durduruldu` gösterilir. Yalnız gerekli idle/drained bariyeri sağlandıktan sonra tamamlandı mesajı verilir; click anında bütün satırlar Stopped boyanmaz. UI thread bloklanmaz.
6. Hata/lease/recording bitmediyse tamamlandı denmez; engel görünür ve güvenli tekrar Stop mümkün olur. Art arda Stop yeni çevrim/activation oluşturmaz.
7. Stop; listener TCP bağlantısını, yüklenmiş revizyonu, activation'ı ve terminal UUID/digest'ini silmez/değiştirmez. Run/Auto yeniden başlamak için açık kullanıcı işlemi gerekir. Remote Enable'ı değiştirmez.
8. Pending ACK ile Runtime durmuş olabilir. Satırda `Durduruldu · ACK bekliyor` ayrı gösterilir. Stop teslimi ACK'lenmiş saymaz ve pending'i silmez; exact ACK/reconcile tamamlanana kadar reçete değişimi kapalıdır.
9. İstasyon yoksa Stop devre dışıdır. Bağlanamamış istasyon varsa neden görünür; tüm makine başarı mesajı verilmez.

### Reçete değişimi

Running/Preparing/Draining durumunda Reçete değiştir pasif ve düzeltme `Önce Dashboard → Stop` olur. Backend seçim, drained, pending ve activation/CAS kontrollerini yükleme anında yeniden yapar; UI kontrolü backend bariyerinin yerine geçmez.

Uyumluluğu denetle salt yapılandırma kontrolü olarak kalır. Başarılıysa çalışan istasyonda `Reçete uyumlu; yüklemek için önce Stop` der. Pending varsa `Reçete uyumlu; bekleyen teslimi tamamlayın` der. `Yüklemeye hazır` yalnız gerçek yükleme önkoşulları o anda sağlanmışsa kullanılır. Yarışta oluşan ret ham STATION_NOT_DRAINED yerine anlaşılır neden ve düzeltme yoluyla gösterilir; önceki seçim korunur.

## 6. D — Yayımlanmış revizyonu ayırt et

Bu, bildirilen aynı reçete değişim akışının tamamlayıcı düzeltmesidir; geçmiş silme özelliği değildir.

- Seçicide `Ad · yayın tarihi/saat · #recipeKey · kısa UUID` gösterilir; tam UUID ve zaman dilimi tooltip/ayrıntıda bulunur. Saat yerel gösteriliyorsa zaman dilimi belirtilir, katalogdaki UTC değiştirilmez.
- Aynı recipeId'nin en yeni yayını **Son yayımlanan**, seçili istasyonun kullandığı **Yüklü** olarak işaretlenir; aynı kayıtta ikisi de bulunabilir. Gruplama yalnız ada göre yapılmaz.
- Kaynakta PublicationPublisher kaydı publishedAt içeriyor, StationSavedRecipe listesi bugün bu bilgiyi taşımıyor. Mevcut liste DTO'suna gerekli metadata eklenir; yeni persistence şeması veya ikinci kayıt deposu açılmaz.
- Aynı reçete grubu içinde tarihe göre yeni önce; eşit tarihte deterministik anahtar sırası. Eksik legacy tarihte `Tarih bilinmiyor`; en yeni iddiası uydurulmaz. Farklı recipeId'lerin aynı adı olması test edilir.
- Liste yenilenirken seçilen recipeKey korunur. Yeni yayın otomatik istasyona yüklenmez veya activation değiştirmez. No-op tekrar publish aynı kaydı korur; tarihi yeniden yazmaz.
- Önceki immutable revizyonlar, activation ve terminal geçmişi korunur. Sadece son kaydı göstermek veya dosya silerek listeyi sadeleştirmek yasaktır.

## 7. Uygulama sahipliği ve sıra

| Sıra | Sorumluluk / muhtemel dosyalar | Tamamlanma kanıtı |
| --- | --- | --- |
| A | `EditorSession.*`, `MainWindowEditor.cpp`, `MainWindowExecution.cpp`, gerekirse `ExecutionService.*`; `ToolGraph.*` sözleşmesi | Seçim/refresh ve target-only testleri; gerçek girdi değişiminde invalidation |
| B | `PublishDataEditor.*`, MainWindow Properties commit, ProjectService yayın/assignment sınırı | V4-only UI, legacy no-op, atomic Apply ve typed slot testleri |
| C | `CommunicationWorkspace.*`, `MainWindow.cpp/.hpp`, `MainWindowCommunication.cpp`, `CommunicationPanel.*`; mevcut StationRunController/RuntimeCoordinator portları | İki istasyon, batch iptal, drain/pending/ret yarış testleri |
| D | `PublicationPublisher.*` mevcut metadata, `StationSavedRecipe`, composition liste portu ve Dashboard seçici | Tarih/son/yüklü etiketleri, stabil seçim, eski revizyon korunumu |
| Kabul | `tests/multi_source_execution_test.cpp`, `tests/editor_publication_ui.cpp`, `tests/communication_workspace_ui_test.cpp`, `tests/dashboard_station_ui.cpp`, ilgili UI smoke'lar ve `CMakeLists.txt` | Hedefli Release sonuçları, iki viewport görselleri; tam ON/OFF istisnası ve K2 operatör kapısı kayıtta |

Tek uygulayıcıyla sırayla yürüt. Önce hatayı testle sabitle; sonra A, B, C, D; ardından birleşik kabul. Dosya listesi ilk kodlama keşfinde bütün çağıranlarla kesinleştirilir. Yeni bağımlılık veya paralel uygulayıcı gerekmez.

## 8. Kabul matrisi

| ID | Senaryo | Geçiş koşulu |
| --- | --- | --- |
| A1 | Tek kaynak → preprocess → blob(found=true) → Result | DONE/OK; Result seçimi, 20 refresh, sekme/satır geçişi sonucu düşürmez |
| A2 | found=false ve ALL/ANY kuralları | Doğru NOK/OK; execution ve validity ayrı, önceki OK taşınmaz |
| A3 | Result seçiliyken Run Flow, sonra Result Run Tool | Acquire hatası yok; target-only yeni acquisition yapmaz; üreticiler değişmez |
| A4 | İki kaynaklı Boolean kuralı ve Viewer değişimi | Karar aynı current cycle kanıtıyla sabit; kaynaklar karışmaz |
| A5 | Kaynak/ROI/parametre/revizyon değişimi; disabled/error/skipped/missing | Eski sonuç kullanılmaz; NOT_EVALUATED ve somut neden; A→B→A eski kanıtı canlandırmaz |
| A6 | Geç güncelleme/Stop/cancel, Set Variables ve slot preview | Sahte geçerli çıktı yok; geç sonuç geri dönmez; tek kaynak varsayımı sızmaz |
| B1 | Boş makine, hazır Yüzey ve Ölçüm | Yalnız V4 UI; Yüzey ek slotsuz, Ölçüm Number[0] eksikken ret |
| B2 | Yanlış tip/birim/ölçek/frame, overflow ve iptal | Önceki assignments/hash korunur; ret alan adıyla; önizleme dışarı göndermez |
| B3 | Legacy groups bulunan kayıt aç/kaydet/V4-only Apply | Legacy veriler/kimlikler korunur; group/profile zorunluluğu yeni V4 akışına karışmaz |
| B4 | 1400×900 ve 850×600, klavye | Eski kontroller yok; şablon/slot/Apply ulaşılabilir; clipping/boş legacy alan yok |
| C1 | İki istasyon Auto açık, Dashboard Stop; satır seçili/seçili değil | İkisi de gerçek Stopped ve Auto kapalı; listener ve seçimler korunur |
| C2 | Preparing/busy/draining, önceden çalışan + yeni başlayan batch | Hepsine Stop; geç cevap yeniden başlatmaz; gerçek geçiş gösterilir |
| C3 | Bir istasyonda hata; art arda Stop | Diğer istasyon da durur; hata adıyla; sahte toplu başarı yok |
| C4 | Pending üretim/kontrol terminali + Stop | Durduruldu ile ACK bekliyor ayrı; aynı result/digest korunur; yanlış ACK kapatmaz |
| C5 | Stop bitmeden/sonra veya pending varken reçete değişimi | Uygun açıklama; backend yeniden kontrol; ret eski activation'ı korur; başarı Auto açmaz |
| D1 | Aynı reçetede 3 yayın; aynı adlı farklı recipeId | Son/yüklü/tarih ayrılır; eski kayıtlar durur; explicit seçim korunur |
| D2 | No-op publish ve restart | Kayıt/tarih çoğalmaz; son seçim geri gelir, Run/Auto/Remote kapalı |
| E1 | Aynı kayıtlı içerik + aynı görüntü ile Editor → yayın → yükleme → TCP | Önceden belirlenen karar eşleşir; DONE/valid ve exact ACKED kanıtlanır |
| E2 | Farklı Editor/istasyon dosyası | Kaynak/revizyon/Blob found ve karar ayrı kaydedilir; fark yanlışlıkla cache hatasına bağlanmaz |

## 9. Test ve kanıt planı

Plan aşamasında build/test yapılmadı; uygulama kanıtı kapanış kaydındadır. Bu teslimde güncel kullanıcı talimatıyla tam Release ON/OFF çalıştırılmaz; son kaynakta etkilenen hedefli kontroller uygulanır. Mevcut test isimlerini `ctest -N` ile doğrula; regex'in gerçekten test seçtiğini kontrol et.

İlk hedefler: `result_execution_contract`, `multi_source_execution`, `multi_source_target_only`, `multi_source_graph_contract`, `editor_target_only`, `ui_result_editor`, `ui_multi_source_editor`, `ui_assignment_editor`, `ui_publication_editor`, `ui_publication_v4`, `fixed_slot_publication_contract`, `publication_assignment_contract`, `publication_publish_contract`, `station_control_contract`, `station_recipe_selection_contract`, `station_recipe_selection_editor9_contract`, `ui_station_dashboard`, `ui_station_tcp`. Değişen çağıranların ek hedefleri de dahil edilir; eski legacy testler sessizce kapatılmaz.

```powershell
cmake --build build/reference-ui --target visionflow result_execution_test communication_workspace_test -j 6
# Mevcut hedefleri listele; göreve özel izole CTest girdilerini kullan:
ctest --test-dir build/reference-ui -N -R 'result_execution|publication|station|multi_source'
# Gerçek koşum dizinleri ve hedef/kanıt eşlemesi: build/vf07-a/acceptance.json
# Tam ON/OFF ve OFF build bu teslimde kullanıcı talimatıyla atlandı.
```

Yeni test INI, örnekler, hash manifesti, JSONL, ekran görüntüleri ve otomatik sonuç/checkpoint `build/vf07-a/` altında koşuma özel tutulur. Kullanıcının `config/local.ui.ini`, gerçek projeleri, makine kataloğu ve peer/state dosyaları değiştirilmez. Kullanıcı ekranındaki rice/88 örneği yalnız önceden tanımlanmış girdi/hash ile tekrarlandığında kabul kanıtıdır; yalnız ekran sayısı beklenen oracle sayılmaz.

### Normal operatör tekrarı

1. İzole normal launcher, ürün UI'siyle makine/istasyon/reçete/istemci oluştur; elle manifest veya servis fixture'ıyla eksik adımı doldurma.
2. Bilinen aynı dosyayla Run Flow → Result seç → slot önizle; beklenen karar korunmalı.
3. Save → İstasyonda yayımla; seçicide son yayın görünür, yüklenen eski revision değişmez.
4. İki hazır istasyonda Auto → Dashboard Stop; ikisi de durmalı. Stop devam ederken yükleme açık olmamalı.
5. Yeni yayını seç → uyumluluğu denetle → yükle; Auto kapalı kalmalı. Auto'yu açıkça başlat.
6. V4 Python CLI ile önceden belirlenen OK/NOK'u doğrula; station/request/revision/activation/camera/result/digest ve ACKED kaydet.
7. Restart; seçim ve geçmiş korunmalı, çalışma izinleri kapalı olmalı. Operatörün gerçek tekrar sonucu ayrıca kaydedilmeli.

## 10. Kapanış ve VF-08 kapısı

**Yazılım teslimi**, güncel talimata göre hedefli kabul ve normal/küçük ekran kontrolüyle kaydedilir; tam ON/OFF istisnası yukarıdadır. K2 operatör kapısı ayrı açık tutulur. Offscreen yazılım kanıtı native operatör veya fiziksel cihaz kabulü olarak sunulmaz. Bu kartın kapanışı ve K2 operatör teyidi olmadan VF-08'e geçilmez; VF-07'nin geçmiş yazılım kanıtı silinmez veya yeni koşum diye sunulmaz.

Uygulama kapanışında ayrı ayrı `uygulandı / hedefli test geçti / normal kullanıcı akışında doğrulandı` raporlanır. Kart, ana plan, güncel workspace kullanım sözleşmesi, simulator rehberi ve gerekiyorsa README kullanım adımları final davranışa göre güncellenir. Tarihli arşiv, progress ve arşiv indeksi; `python scripts/check-context-docs.py`, diff/link kontrolü ve tek `VF07-A: …` uygulama commit'i; push yok.

### Analiz sınırları

Graph D:/TVision, generation `2026-11-03T23:16:47Z`; watch bu oturumda aktif değil. Verify sorguları, Result reconcile ve workspace command çift yön trace, ilgili 16 yol için coverage ve doğrudan kaynak okuması kullanıldı. PublishDataEditor.hpp ve CommunicationPanel/Composition kısmi parse aralıkları kaynakla tamamlandı. Graph trace'in eksik dinamik Qt çağrılarını kapsadığı varsayılmadı.

Bu plan sırasında uygulama çalıştırılmadı, yeni regresyon kanıtı üretilmedi. Kaynak zincirinin kesin hata testi ve TCP NOK'un bağımsız nedeni A'nın başlangıç kabulüdür. Planlama teslimi, VF07-A uygulamasının tamamlandığı anlamına gelmez.
