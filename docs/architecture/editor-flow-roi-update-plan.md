# Flow, combobox ve ROI güncelleme planı — EFR-PLAN-02

28 Ekim 2026. **Yalnız plan; uygulama başlamadı.** Kullanıcının güncel kapsamı:
1) Flow sağ tıkta Remove Tool, 2) tekerlekle combobox değerinin değişmemesi,
3) ROI'de isteğe bağlı gerçek görüntü kırpma. Template Match/θ ertelendi.
[Shape Pose hızlandırma notu](shape-pose-performance-note.md) bağımsız inceleme
girdisidir. Eski U1–U4 tamamlanmış işleri yeniden açılmaz.

Önerilen uygulama sırası: **EFR-01 → EFR-02 → EFR-03a-1 → EFR-03a-2 →
EFR-03a-3 → EFR-03a-4 → EFR-03b → EFR-04**.
Her kart ayrı görev ID'li commit/kapanış; ajanlar paralel çalışmaz. Sonraki
uygulama kartı EFR-01'dir. Bu plan uygulama yapılmış veya üretim kabulü verilmiş
anlamına gelmez. Üç applicationType aynı Editor/Runtime sözleşmesini kullanır.

## Kaynak başlangıcı ve korunacak sözleşmeler

| Konu | Mevcut kaynak / davranış |
| --- | --- |
| Flow menüsü | `src/app/MainWindowEditor.cpp`, `showFlowMenu`: yukarı/aşağı, tıklanan ID ve reçete kimliği yakalanıyor; busy/geç tetik kapısı var. |
| Silme | Aynı dosyada Properties Remove Tool; `src/application/ProjectService.cpp`, `removeTool` mevcut servis yolunu kullanıyor. Bağlantı temizleme/ret ve özel referans tüketicileri uygulamada ayrıca doğrulanacak. |
| ROI | `src/application/RoiService.hpp`: imageBinding ile kaynak/frame/boyut doğrulaması, yalnız giriş görüntüsünün koordinatlarında Region çıktısı. |
| Port/parametre | `src/domain/ToolRegistry.cpp`: roi sürüm 1, image girdisi, region çıktısı, searchRoi/imageBinding. |
| Görüntü | `src/vision/ImageFrame.hpp`, `ImageAccess.cpp`: owned piksel verisi ve processing kimliği; `processed()` aynı boyutu zorunlu tutuyor. |
| Mevcut geometri | [ROI/görüntü sözleşmesi](roi-geometry.md): mevcut processed yolu crop/resize uygulamaz. Yeni crop ayrı açık dönüşüm gerektirir. |

Kaynak incelemesi graph + dosya okumasıyla yapıldı. `ImageFrame.hpp` satır 30
kısmi indeks kapsamındaydı; imza ve dosya doğrudan okundu. Yeni incelemelerde
ilgili kapsam tekrar doğrulanmalı. UI algoritma/SDK çağırmaz; owned görüntü,
kaynak/frame/cycle kimliği, no-op/cache ve immutable Runtime korunur.

## EFR-01 — Flow sağ tıkta Remove Tool

### Davranış

- Menü: Bir sıra yukarı → Bir sıra aşağı → ayırıcı → **Remove Tool**.
- İşlem sağ tıklanan ID'ye uygulanır; Properties'te başka araç seçili olması
  hedefi değiştirmez. Boş Flow alanında araç menüsü açılmaz.
- Menü açılırken ve aksiyon çalışırken Editor idle, aynı reçete ve hedefin
  hâlâ var olduğu doğrulanır. Çalıştırma/öğretim busy sırasında silme pasif.
- Properties düğmesi ve menü ortak uygulama komutunu kullanır; UI'da ayrı
  bağlantı silme algoritması yazılmaz. Mevcut doğrudan silme akışına yeni onay
  penceresi eklenmez. Silme başarısızsa liste/seçim/dirty kısmi değişmez.
- Seçili olmayan araç silinirse mevcut seçim korunur. Seçili araç silinirse
  aynı sıradaki sonraki, yoksa önceki araç; liste boşsa boş Properties seçilir.
- Silinen aracın bekleyen çizim/öğretim niyeti ve Editor cache/overlay'i
  temizlenir. Aktif Runtime snapshot değiştirilmez.
- Normal bağlantılar mevcut servis sözleşmesiyle ele alınır; özel reference,
  Variables/Result/Publish kaynakları da taranır. Kalan tüketiciler geçerli
  kaynak varmış gibi sonuç üretmez; eksik/yeniden bağlanmalı durumu görünürdür.
  Mevcut politika atomik ret gerektiriyorsa önce kullanıcıya anlaşılır hata
  gösterilir; yetim referans üretmek için doğrulama gevşetilmez.

### Uygulama ve kabul

Başlıca yerler: MainWindowEditor ve gerekirse ortak MainWindow komutu;
ProjectService'in mevcut silme/setTools doğrulaması. Başka servis davranışı
değişecekse etkilenen tüketici testleri karta eklenir.

Kabul: sağ tık A/seçili B; seçili ilk/orta/son/tek araç; kaynak araç ve bağlı
tüketici; menü açıkken reçete değişimi/silme/Run; Properties ile eşdeğer sonuç;
Save/reopen ve cache/overlay temizliği. Normal ve küçük ekranda popup ve
silme sonrası seçim görselleri. İlgili `ui_editor_usability` ve silme/reference
regresyonları; UI Release derlemesi. Yeni undo özelliği bu kartta yoktur.

## EFR-02 — Combobox tekerlek politikası

### Davranış

- Uygulamanın kapalı tüm combobox'larında tekerlek seçimi değiştirmez;
  kontrol klavye odağında olsa da kural aynıdır. Dikey/yatay ve trackpad
  pixelDelta/angleDelta olayları kapsanır.
- ScrollArea içindeyse olay uygun üst kaydırma alanına iletilir; bir olay
  yalnız bir kez kaydırılır. ScrollArea yoksa değer değişmeden olay yok sayılır.
- Açık listede tekerlek listeyi kaydırabilir fakat tek başına değer kaydetmez;
  seçim tıklama veya Enter ile, iptal Escape ile olur. Klavye okları ve mevcut
  erişilebilir seçim davranışı korunur. Editable combo metin girişi korunur.
- Tekerlekten `currentIndexChanged` ile reçete/ayar yazımı, dirty/revision,
  cache invalidation veya donanım komutu oluşmaz. Gerçek kullanıcı seçimi aynı
  mevcut işlemi bir kez tetikler.

### Uygulama ve kabul

Başlangıçta MainWindow/Camera Manager/Setup/Calibration/Runtime ve dinamik
Properties diyaloglarında combobox envanteri çıkar. Merkezi Qt event filter
veya ortak kontrol politikası kullan; tek tek panellerde kopya filtre üretme.
Filtre yalnız combobox ve popup hedeflerine uygulanır; viewer zoom/pan ve diğer
kontrollerin wheel işleyişi değişmez. Spinbox politikası ayrı istektir, bu
kartta değer davranışı değiştirilmez.

Kabul matrisi: odaklı/odaksız/kapalı/açık/editable/disabled, iç içe scroll,
scroll sınırı, modal ve sonradan oluşturulmuş combo, Windows wheel ve trackpad.
Her durumda öncesi/sonrası index/text, sinyal/ayar yazma sayısı ve scrollbar
hareketi ölçülür. Açık listede scroll→Escape, scroll→tıklama ve klavye seçimi
ayrı doğrulanır. Kamera seçimi tekerlekle değişip bağlantı/readback başlatmaz.
UI Release, hedefli Qt etkileşim testi ve gerçek pencerede wheel kontrolü;
native kontrol yapılamazsa açık sınırlama olarak kaydedilir.

## EFR-03a — ROI kırpmanın görüntü/koordinat sözleşmesi

Bu kart checkbox'tan önce bitirilir. `processed()` boyut kontrolünü kaldırmak
ve eski raw coordinate kimliğini yeni rastera kopyalamak çözüm değildir.

### Parametre, port ve uyumluluk

- Yeni checkbox/parametre: **Görüntüyü ROI'ye kırp**, `cropImage`, varsayılan
  false. Yeni ROI tanımı image girdisi ile region ve image çıktıları taşır.
- Kapalıyken region mevcut anlamındadır; image çıktısı girdiyi değiştirmeden
  geçirir. Açıkken image gerçekten küçülür. Böylece çıkış portları mod değişince
  kaybolmaz; bağlı tüketici sessizce başka porta taşınmaz. Crop kapalıyken de
  ROI aracı ve Image çıktısı kullanılabilir; checkbox aracın etkinlik anahtarı değildir.
- Image girdisi alan bütün araçlar ROI/Image çıktısını iki modda da alabilmelidir.
  Kaynağın doğrudan kamera/dosya aracı olması şartı konmaz. Preprocess, ROI,
  Blob, Code Read, Template Match, pose ve metrologi dahil registry'deki tüm
  Image tüketicileri envanterlenir. Crop kökeni tek başına ret nedeni olamaz;
  aracın mevcut format, minimum boyut, eğitim ve kalibrasyon önkoşulları korunur.
- Region her iki modda **ROI girdisinin koordinatlarında** kalır. Kırpılmış
  görüntüyle aynı ROI'nin region çıktısı doğrudan uyumlu sayılmaz; koordinat
  uyuşmazlığı açık ret/dönüştürme gerektirir. Kırpılmış image kullanan araç tam
  yeni görüntüyü veya bu görüntüde çizilmiş kendi ROI'sini kullanabilir.
- İlk sürüm tam sayı, eksen hizalı dikdörtgen; dolgusuz, yeniden örneklemesiz ve
  formatı değiştirmeyen kopya. Siyah maske üretmek kırpma sayılmaz.
- Eksik/boş/sınır dışı ROI veya değişmiş imageBinding açık hata; tam görüntüye
  sessiz dönüş yok. Tam görüntü boyutunda ROI geçerlidir.
- Tek ROI davranışı vardır; eski/yeni ROI sürümlerini paralel yürütme veya
  eski port/hash davranışını aynen tutma gereksinimi yoktur. Mevcut kayıtta
  cropImage bulunmuyorsa false kabul edilir; çizili alan ve bağlantılar korunur.
  Gerekli codec/şema uyarlaması atomik yapılır, kayıt/açılış fixture'ıyla
  doğrulanır. Kullanıcıya ayrı ROI sürümü seçtirilmez. İlgisiz araçların
  no-op/hash sözleşmesi korunur; ROI değişikliği gerekli cache yenilemesini yapar.

### Raster ve köken

- Crop için ayrı owned image üretim API'si ve geometri köken sözleşmesi tanımla:
  giriş boyutu/coordinate kimliği, crop rect, çıktı boyutu/coordinate kimliği,
  child→parent ötelemesi ve edinim raw köküne bileşim. Aynı dönüşüm kimliği
  her karede sabit geometri için kararlı, frame/cycle ise güncel olmalı.
- Örnek: girişte `(300,200,100,80)` → çıktı 100×80; yerel `(10,20)`
  girişte `(310,220)`. Piksel merkezi convention korunur. İç içe crop'ta
  ötelemeler bileşir; overflow/footprint ve zincir bütçesi doğrulanır.
- source/frame/cycle/timestamp kökeni korunur; content kimliği gerçek pikselleri,
  processing/geometri kimliği rect ve dönüşüm zincirini ayırt eder. Aynı pikseli
  veren iki farklı crop aynı koordinat/geometri scope'u sayılmaz.
- Parametre/model/kaynak/boyut/geometri değişimi downstream cache ve teaching
  uygunluğunu yeniden değerlendirir. Eski eğitim sessizce yeni rastera taşınmaz.
- Crop açık→kapalı→açık ve aynı boyutta farklı konuma taşınma ayrı kabul
  senaryolarıdır. Geometriye bağlı eğitim geçersizleşir; yalnız eski rect'e
  dönmek eski eğitim iznini veya sonuç/cache'i canlandırmaz. Yeniden öğretim
  gerektirmeyen tüketiciler güncel girdiyle yeniden çalışabilir.

### Katmanlar arası zorunlu inceleme

ImageFrame/ImageAccess, Registry/codec/migration, RoiService/executor/cache,
ToolResultCodec, Viewer overlay ve kaynak seçimi, referans/pose resolver,
calibration resolver/sampler, immutable Runtime/terminal replay incelenir.
İsimleri ve test hedefleri uygulama başında graph/coverage ile kesinleştirilir.

Kalibre ölçümde crop koordinatı önce edinim raw koordinatına taşınır; mevcut
kalibrasyonun kaynak, ham çözünürlük, lens, çalışma düzlemi ve hull kapıları
aynı ham geometriyle denetlenir. Crop boyutu yeni kamera çözünürlüğü gibi
gösterilmez. İleri yol crop→ham→dünya, ters örnekleme yolu dünya→ham→crop
olarak uygulanır. Örnekleme tap'leri hem kalibrasyon hull'una hem crop raster
sınırına göre denetlenir; crop dışında kalan ham piksele gizlice erişilmez.
Homography için tek mm/px ötelemesi kullanılmaz. Birim veya pose uydurulmaz.

Tüm Image tüketicileri için kapalı/açık, ham/kalibre (desteklediği ölçüde),
referanslı/referanssız kullanım matrisi çıkarılır. Mevcut destekli çalışma
modunun crop girdisiyle çalışması zorunludur; genel bir “crop desteklenmiyor”
hatası kabulü karşılamaz. Hata senaryoları eksik eğitim/kalibrasyon, uyumsuz
frame/Region, geçersiz footprint ve aracın mevcut girdi sınırları içindir.
Yeni resize/rotation/rectification dönüşümleri bu kartların kapsamında değildir.

### Sıralı uygulama kartları

Bu bölüm tek büyük ajan görevi olarak verilmez. Sol6.1 medium alt ajanları
sırayla çalışır; her kart kendi testleri, kapanış kaydı ve commit'iyle teslim
edilir. Yönetici ajan sonucu incelemeden sonraki karta geçilmez.

| Kart | Sorumluluk ve çıkış kapısı |
| --- | --- |
| EFR-03a-1 | Tek ROI parametre/port ve eksik cropImage okuma sözleşmesi; tüm Image tüketicilerinin dosya/test matrisi; koordinat ve kalibrasyon API tasarımı. Uygulayıcıya belirsiz destek kapsamı bırakılmaz. |
| EFR-03a-2 | Owned crop API, raw kök/child dönüşümü, kararlı geometri kimliği; altı format, stride, ömür, sınır ve iç içe crop testleri. |
| EFR-03a-3 | ROI executor, registry/codec, Image portu, graph/cache/teaching invalidation; kapalı pass-through, kayıt açma ve mod geçişi testleri. |
| EFR-03a-4 | Matrisin bütün Image tüketicileri, referans/pose ve kalibrasyon/sampler entegrasyonu; ileri/ters dönüşüm, hull ve crop footprint testleri. Crop nedeniyle destek dışı bırakılmış consumer kalmaz. |

EFR-03a-2 altyapısı EFR-03a-3'ten önce, tüm core tüketiciler EFR-03b UI
kabulünden önce tamamlanır. Her kart yalnız kendi sorumluluk dosyalarını
değiştirir; ortak API değişikliği önce belgelenir ve sonraki karta aktarılır.

### Kabul

Owned kopya/ömür, desteklenen altı pixel formatı/stride, birebir piksel;
sol üst/kenar/tek piksel/tam görüntü/iç içe crop; taşma/sınır dışı;
aynı piksel-farklı rect kimliği; Save/reopen/eksik cropImage=false; frame/cycle/stale
cache; source/boyut değişimi; ham ve kalibre koordinat dönüşümü; hatalı
referans/kalibrasyon retleri. `roi_contract`, `roi_geometry_contract`,
`preprocessing_contract` ve yeni crop sözleşme testleri, ilgili codec/cache
regresyonları. Core Release. Bu kart kullanıcı özelliği tamamlandı sayılmaz.

## EFR-03b — ROI checkbox, bağlantılar ve görüntüleme

- ROI Properties'de çizim ve boyut özetinin yanında checkbox; kısa açıklama:
  “Açıkken Image çıktısı yalnız seçili alanı içerir.” Değer reçetede saklanır;
  yerel panel tercihi değildir. Busy sırasında değişiklik yapılamaz.
- Kapalı açıklaması mevcut Region kullanımını; açık açıklaması sonraki aracın
  Görüntü kaynağı olarak ROI/Image seçmesini anlatır. Kullanıcıya teknik hash
  veya migration ayrıntısı gösterilmez.
- ROI seçili ve güncel sonuç varken kapalı modda tam görüntü+ROI, açık modda
  kırpılmış çıktı görüntülenir. Düzenleme/yeniden çizim açık bir eylemle giriş
  görüntüsünde yapılır; kullanıcı kırpılmış raster üstüne giriş koordinatı çizmez.
- Çizim bitişi veya checkbox değişimi eski sonucu güncel diye göstermez.
  Gerçek yürütme sonrası görüntü/sonuç güncellenir. Kaynak yoksa net eksik durum.
- Mevcut bağlantılar otomatik yeniden bağlanmaz. Yeni araç varsayılan Image
  kaynağı seçiminde kullanıcı ROI/Image seçmişse korunur; ROI seçimini görünür
  kılmak için seçici listesi güncellenir. Gizli kaynak değiştirme eklenmez.
- Sonraki aracın ROI çizimi kendi görüntüsünde olur; Viewer orijinal kaynağa
  dönünce overlay açık köken dönüşümüyle yerleştirilir. Farklı frame'lerden
  point/region/pose karıştırılmaz. Crop değişiminde gerekli yeniden öğretim
  görünür olur. Match algoritması veya θ çıktısı değiştirilmez.

Kabul: Image File/Preprocess → ROI → bütün Image tüketicileri ve ham metrologi;
uygun kalibrasyonla ölçüm ve referans kullanan tüketici; aynı görüntüde ham/crop
dalları, iç içe crop, kaynak değişimi, Stop/eski sonuç, Save/reopen ve mode
toggle/no-op, açık→kapalı→açık ve aynı boyutta farklı konum. Geçersiz girdi ve
eksik önkoşulların ret mesajları ayrıca doğrulanır; crop tek başına ret değildir.
Image/Region bağlama yanlışlığı UI'da anlaşılır olmalı. UI Release,
`ui_roi_tool`, `ui_editor_usability`, ilgili consumer testleri; normal/küçük
ekranda çizim, checkbox açık/kapalı, kırpılmış çıktı ve overlay görsel kontrolü.

## EFR-04 — Birleşik kabul ve doküman kapanışı

Üç applicationType için aynı senaryolar; Editor Run Tool/Run Flow ve kaydedilmiş
reçeteyle Runtime davranışı. Crop zinciri immutable snapshot ve terminal
köken/replay/Logs semantiğini korur; yetkilendirilmeyen üretim yollarını açmaz.
Buradaki replay, saklanan sonuç ve köken bilgisinin tarihsel gösterimidir;
görüntüyü yeniden üretme veya algoritmayı yeniden çalıştırma değildir. Mevcut
arşiv piksel saklamaz; bu faz görüntü arşivleme eklemez. Crop/raw koordinat
kimliği, raw boyut, rect/dönüşüm ve frame/cycle ilişkisi geçmiş sonucu doğru
yorumlamaya yetecek şekilde codec'te saklanır ve round-trip test edilir.
Raster bulunmuyorsa geçmiş kırpılmış görüntü/overlay varmış gibi gösterilmez;
güncel kare geçmiş sonuca arka plan yapılamaz. Görüntünün mevcut olmadığı
açıkça belirtilir. Eski terminal kayıtları doğrulanmamış crop kökeniyle doldurulmaz.
Gerçek sonuç olmadan skor/PASS/robot pose üretmez. Fiziksel kamera/mm kabulü,
native DPI/çoklu monitör ve dağıtım kabulü yazılım testiyle kapatılmaz.

README'deki Release ON/OFF komutlarıyla derleme ve ilgili CTest, sonra bu
katmanlar arası değişim için tam iki yapılandırma regresyonu. Hedefli koşumlar
tam suite diye raporlanmaz. UI görselleri/test INI/loglar `build/efr/` altında;
kullanıcının `config/local.ui.ini` dosyası test için değiştirilmez.

Uygulama tesliminde ilgili UI gereksinimi, ROI/görüntü ve proje formatı
sözleşmelerini yeni davranışa göre güncelle; planı tamamlanmış ürün metni diye
kopyalama. Her kartta progress güncel özeti, yeni tarihli arşiv ve dizin bağlantısı;
`python scripts/check-context-docs.py`. Her tamamlanan kart tek görev ID'li
commit; push yalnız kullanıcı isteğiyle. EFR-PLAN-02 için yalnız belge kontrolü
yapılır; build/test/görsel veya performans kabulü iddiası yoktur.
