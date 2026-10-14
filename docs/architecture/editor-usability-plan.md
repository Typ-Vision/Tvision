# Recipe Editor kullanım kolaylığı planı

12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](reference-production-acceptance.md). [R8 paket ve devir kanıtı](reference-release-acceptance.md).


11 Ekim güncel durum: U1–U4 kabulü korunur; [Ortak Referans R5a/R5b](reference-system-plan.md) uygulandı. Yedi tüketicide tek Referans, Basic’te uygun X/Y/açı, gelişmişte world öğretim/sampling/kimlik bilgileri ve salt okunur current Viewer vardır. R6 birleşik Editor kabulü tamamlandı; sıradaki **R7a**, ardından R7b/R8 ve T8.4.2b.3c. Alttaki U1–U4 kapsam/sırası tarihsel teslimdir.

11 Ekim 2026 — Sekiz talebin davranış sözleşmesi. **U1–U4 uygulandı ve birleşik yazılım kabulü tamamlandı.** Aşağıdaki proje değerlendirmesi plan oturumunun tarihsel görünümüdür.

U1/U2 teslimi: ortak yerel açılır bölümler, dört metrologi aracında Basic/Detailed ve değişmiş gelişmiş ayar bilgisi; tüm araçlarda Type/ID sadeleşmesi/kapalı Çıkışlar, viewer seçim koruması. Atomik varsayılan Image bağlantısı ProjectService’dedir; uygun bağlı Preprocess zinciri/son önceki dal, yoksa tek etkin acquisition. Kaynak olmayan/etkin olmayan kaynakta yeni araç bağlantısızdır; çoklu acquisition mevcut schema doğrulamasında atomik reddedilir. Eski araç/manüel seçim/bağlantı kaldırma yeniden yazılmaz. Yeni çizilebilir araç güncel toolImage ile hazırlanır; kaynak/cache eksikse Properties uyarısı ve seçili araç/reçeteye bağlı niyet tutulur. Stop/Escape/seçim/silme/reçete değişimi iptal eder; öğretim/kalibrasyon kapıları korunur. Pattern/Shape/Template model öğretimi bağlı rasterı kullanır. Algoritma/engine/schema sürümleri değişmedi.

Doğrulama kanıtı ve sınırlamalar ilerleme kaydındadır. Native DPI/çoklu monitör ve fiziksel kamera kabulü yapılmadı. U3 teslimi: ProjectService::moveTool atomik liste düzenlemesi; Flow sağ tık yukarı/aşağı, sınır/busy ve geç tetik kapıları. Seçili ID, parametre/ROI/bağlantılar ve aktif snapshot korunur; gerçek DAG bağımlılıkları listeden bağımsızdır. U4: Release core52/UI59 tam CTest, dört metrologi aracının Basic/Detailed/Elle gir/Çıkışlar, kaynak/çizim ve Flow menüsü normal/küçük ekran görsel kabulü tamamlandı. Menü popup’ı ayrı u3-menu görselinde, Flow listesi u3-flow görselindedir. Sonraki iş **T8.4.2b.3c**.

## 1. Proje değerlendirmesi ve öncelik

Hedef, kod yazmadan endüstriyel görüntü işleme reçetesi hazırlamak; Robot Guidance, Defect Inspection ve Measurement için aynı Editor/Runtime ve veri akışını kullanmaktır. Günlük temel akış görüntü edin → araç ekle → alanı çiz → temel ayarları yap → gerçek sonucu incele olmalıdır. Ayrıntılı mühendislik ayarlarına erişim korunmalıdır.

Mevcut altyapı bu hedefi destekler: C++20/Qt Widgets/OpenCV katmanları, typed port/DAG, owned frame ve kaynak bilgisi, worker/Stop/eski sonucu reddetme, kalibrasyon bağı, immutable Runtime snapshot ve kalıcı sonuç kayıtları bulunur. Measurement araçları ve kalibrasyon yazılımı en ilerideki ürün alanıdır. Pattern/Shape Pose ve ROI tracking editör/çekirdek kapsamı ilerlemiştir; robot koordinat dönüşümü ve robot guidance tamamlanmış değildir. Inspection tarafında Blob/Code vardır; gelişmiş kusur analizi ve ortak final karar hâlâ plan kapsamındadır.

Son teslim T8.4.2b.3b'dir. 10 Ekim kayıtlı Release loglarında core 50/50, UI açık paket 56/56 CTest geçmiştir (56 testin tamamı görsel UI testi değildir). Bu plan oturumunda testler yeniden çalıştırılmadı. T8.4.2b.3c pose tracking trusted completion/production Runtime/Logs/restart/CSV ve corpus kabulü açıktır. Endüstriyel kamera stream/grab/fiziksel kabul, K4, PLC restart kurtarma P7/6D.2, robot 6E, lens T15 ve ürünleştirme/dağıtım/CUDA kanıtları açık kalır. Yazılım test sayısı fiziksel doğruluk veya üretime hazır olma yüzdesi değildir.

Başlıca gelişim ihtiyacı, teknik yeteneği daha az kullanıcı adımıyla kullanılabilir kılmaktır. MetrologyEditor JSON alanlarını sırayla kontrol olarak üretir; alanların kullanım sıklığına göre sunum tanımı yoktur. MainWindowEditor araç türlerine göre büyüyen koşullar ve etkileşim bağlantıları içerir. Bu pakette ortak açılır bölüm ve araç bazlı açık alan grupları kullanılmalı; bütün mimariyi yeniden yazmaya dönüşmemelidir. README/mimari belgelerindeki tarihsel kayıtların tekrarı güncel durumun okunmasını zorlaştırır; ileride tek güncel durum özeti ve ayrı tarihçe düzeni önerilir. Çalışma ağacında çok sayıda önceki değişiklik vardır; uygulamada korunmalı ve bu paket ayrı, incelenebilir kapsamda tutulmalıdır.

Güncel kullanıcı yönlendirmesiyle önerilen sıra: **U1 → U2 → U3 → U4 → T8.4.2b.3c**, ardından mevcut tool yol haritası. Önceki tamamlanan işler tekrar açılmaz. Mevcut tema, ana paneller ve navigasyon korunur; Properties içeriği ve istenen Flow etkileşimleri bu talep kapsamında değişir. Bu, eski “GUI değişmeyecek” kuralının bu alanlar için güncel istisnasıdır.

## 2. Sekiz talebin davranış sözleşmesi

| No | Planlanan davranış | Kabul ölçütü |
| --- | --- | --- |
| 1 | Ok/kutu/halka çizim düğmesinin hemen altında varsayılan kapalı **Elle gir…** bölümü. Koordinatlar ve koordinat uygulama düğmesi burada. | Edge Point ax/ay/bx/by ilk açılışta görünmez; açınca aynı hassasiyet, birim ve doğrulamayla düzenlenir. Çizimden sonra sayısal değerler günceldir. |
| 2 | Image giriş etiketi **Görüntü kaynağı**. Yeni araç için uygun önceki Preprocess çıkışı, yoksa uygun acquisition görüntüsü otomatik bağlanır; mevcut seçici kalır. | Kullanıcı kaynak seçmeden Image File → Preprocess → Circle ekleyebilir; bağlantı kalıcıdır ve seçicide görünür. Manuel seçim sonradan ezilmez. |
| 3 | Statik output port listesi altta varsayılan kapalı **Çıkışlar** bölümünde. | Port adları/tipleri gerektiğinde erişilebilir; gerçek sonuçlar Results'ta kalır. Bölüm açmak yürütme yapmaz. |
| 4 | Ayrı Type ve ID satırları Properties'ten kaldırılır. Araç adı/durum kalır; tür Flow'daki araç etiketinden anlaşılır. | Kimlikler dosyada, bağlantıda ve tanıda korunur; ad değiştirme bağlantıları bozmaz. |
| 5 | Görüntü/overlay/boş alan tıklaması seçili aracı ve Properties'i değiştirmez. Mevcut araçlar arasında seçim Flow'dan yapılır. | ROI içi/dışı tıklama, sürükleme, zoom/pan ve çizim bitişi Image File'a geçmez. Yeni araç eklenince o aracın seçilmesi korunur. |
| 6 | Flow sağ tık menüsü: **Bir sıra yukarı**, **Bir sıra aşağı**. | Sınırdaki yön pasif; araç ID'si, ayarları, bağlantıları ve seçimi korunur; sıra Save/reopen sonrasında aynıdır. |
| 7 | Yeni eklenen çizilebilir araç seçildikten ve görüntü kaynağı bağlandıktan sonra birincil ROI çizim modu hazır olur. | Circle ekle → doğrudan görüntüde halka çiz; Edge Point/Pair ok, Line Fit döndürülmüş kutu, Blob/Code arama kutusu. |
| 8 | Karmaşık araçlarda **Basic / Detailed** birbirini dışlayan seçim; varsayılan Basic. | Detailed mevcut ayarların tamamına erişir. Mod değiştirmek ayarları/defaultları/sonuçları değiştirmez veya reçeteyi dirty yapmaz. |

Basic/Detailed için iki bağımsız checkbox yerine aynı anda yalnız biri seçilebilen iki radyo düğmesi veya segmentli seçim önerilir. Böylece ikisinin birden açık/kapalı olma belirsizliği yoktur. Kullanıcının istediği iki görünüm korunur.

### Properties sırası ve görünüm durumu

Araç adı/etkinlik → Görüntü kaynağı → Basic/Detailed → çizim düğmesi/Elle gir → temel ayarlar/tolerans → gerekiyorsa ROI pose referansı → gelişmiş gruplar → Çıkışlar. Açıklama ve uzun teknik yardım ikincil, kapalı bölümlerde tutulabilir; hata/eksik kaynak/yeniden öğretim gereksinimi görünür kalır. Çalıştırma kontrolleri erişilebilir kalır.

Elle gir ve Çıkışlar, Detailed görünümünde de bağımsız açılır; Detailed tüm kontrollere erişim sağlar, hepsini zorunlu olarak aynı anda açmaz. Bir grupta doğrulama hatası varsa grup açılır ve alan gösterilir. Basit görünümde gizlenen değiştirilmiş gelişmiş ayarlar için kısa “Gelişmiş ayarlar özelleştirilmiş” bilgisi bulunur.

Mod ve bölüm durumları yenileme/araç seçimi sırasında korunur; tercih araç türü bazında yerel UI ayarıdır, reçete parametresi değildir. Ürün ayarı config/local.ui.ini mekanizmasını kullanır; geliştirme/test oturumu kullanıcının dosyasını düzenlemez, test INI'leri build altında tutulur. Görünüm değişikliği Save/dirty/revision/cache invalidation üretmez.

### Basic alan grupları

| Araç | Basic | Detailed ekleri |
| --- | --- | --- |
| Edge Point | Ok çiz/ters yön, tarama genişliği, kenar yönü, ilk/son/en güçlü seçim, min kontrast | Boyuna/enine örnek adımı, Gaussian sigma, subpixel ve ayrıntılı açıklamalar |
| Edge Pair / Caliper | Ok çiz, tarama genişliği, açık/koyu çift, seçim, min kontrast, beklenen min/max çift genişliği, px/mm ve tolerans | Örnekleme, yumuşatma, subpixel ve diğer mevcut algoritma ayarları |
| Line Fit | Kutu çiz, kenar yönü/seçim, min kontrast, tarama sayısı, px/mm, uzunluk/açı toleransları | RANSAC/refit, inlier/residual/RMS, minimum destek/coverage ve örnekleme ayarları |
| Circle / Arc | Halka çiz, halka/yay ve yay açıları, içe/dışa tarama, kenar yönü/seçim, min kontrast, tarama sayısı, px/mm, çap/sapma toleransları | Sampling, robust fit, residual/RMS, coverage/gap/minimum gözlem kalite eşikleri |

Geometrik konum koordinatları Elle gir bölümündedir; sık kullanılan tarama genişliği ve yay kapsamı yalnız sayısal konumlarla birlikte saklanıp erişilemez yapılmaz. Aynı ayarın iki bağımsız kopyası üretilmez. Birimler raw px / nominal mm / synthetic sample ayrımına göre doğru etiketlenir. Basic'e dönünce gelişmiş tolerans/kalite kapıları çalışmaya devam eder. Pose öğretim gereksinimleri mod değişiminde gizlenmez.

## 3. Görüntü kaynağı varsayılanı

Bu bir araç oluşturma kolaylığıdır; çalışma anında gizli kaynak arama veya fallback değildir. Yalnız Image girişi olan yeni araçlara uygulanır; acquisition, sadece feature alan Measurement/Calculate gibi araçlar uygun olmayan bağlantı almaz.

1. Yeni aracın eklenme yerinden önceki etkin ve tip/birim/frame uyumlu üreticiler incelenir. Kendi çıkışı veya downstream bağlantısı seçilemez.
2. Geçerli acquisition zincirine bağlı uygun Preprocess araçları içinde Flow'da en yakın önceki çıkış tercih edilir. Preprocess zincirlerinde son uygun çıkış seçilir; yeni Preprocess de önceki uygun çıkışa bağlanabilir.
3. Uygun Preprocess yoksa mevcut engine'in desteklediği tek acquisition kaynağı kullanılır. Birden fazla acquisition kaynağı bulunan desteklenmeyen/belirsiz reçetede rastgele seçim yapılmaz; seçici ve kısa açıklama gösterilir. Etkin olmayan/bağlantısız Preprocess varsayılan aday değildir.
4. Varsayılan bağlantı araç ekleme ile aynı servis komutunda doğrulanıp kaydedilir. UI'da sadece seçilmiş görünmesi yeterli değildir. Eksik başka araç girişleri bulunan taslaklarda yeni hata eklememe ilkesi korunur.
5. Mevcut reçete açılırken, panel yenilenirken, Flow sırası değişirken, sonradan Preprocess eklenirken mevcut bağlantılar otomatik yeniden yazılmaz. Kaynak sonradan eklenen eski bağlantısız araçlar bu ilk kapsamda kullanıcı seçicisiyle bağlanır. Böylece bilerek kaldırılmış seçim tekrar oluşmaz.
6. Yapılandırmada Preprocess'e bağlanmak, onun geçerli görüntüsünün hazır olduğu anlamına gelmez. Run Tool mevcut güncel upstream cache'i ister; yoksa çalıştırma yönergesi sunulur. Otomatik full-flow/acquisition, eski cache veya sessiz raw görüntü fallback yoktur.

## 4. Viewer çizim ve Flow sırası

Mevcut hata kaynağı MainWindowEditor içindeki onImageClicked → selectTool(image_file) bağlantısıdır. Editor'daki bu seçim yan etkisi kaldırılmalı; Calibration nokta seçimi ve diğer viewer callback'leri korunmalıdır.

Çizim hazırlığı yalnız gerçek yeni araç ekleme olayında bir kez çalışır; Flow'dan eski aracı seçmek veya paneli yenilemek yeniden çizim başlatmaz. Başka tool/reçete seçmek, silmek, Stop veya Escape bekleyen çizimi iptal eder. Escape mevcut seçimi korumalı; yarım ROI reçeteye yazılmamalıdır. Çizim tamamlanınca normal tutamaç düzenleme moduna dönülür. Circle mevcut bas-sürükle-bırak hareketiyle merkezi/yarıçapları belirler; tek tıklamayla keyfi boyutta ROI kaydı oluşturulmaz.

Görüntü hazır değilse araç eklenebilir; görünür “Çizim için görüntü gerekli” durumu ve yalnız o yeni araçla ilişkili bekleyen çizim niyeti tutulur. Geçerli bağlı görüntü geldiğinde hâlâ aynı araç/reçete seçili ve oturum boşta ise çizim hazırlanır; seçim/iptal/oturum değişiminde geç hazırlık bastırılır. Preprocess için gösterilen raster ve çizimde kullanılan koordinat bağlamı aynı güncel bağlı görüntüye ait olmalıdır. Raw görüntüyü Preprocess sonucu diye sunmamak gerekir. World/pose bağlı salt okunur ROI'lerde mevcut öğretim ve kalibrasyon kapıları korunur.

Pattern/Shape/Template gibi hem model hem arama alanı çizen araçlarda ilk eylem **Model alanını çiz/öğret** olarak açıkça gösterilir; geçerli görüntü/model gereksinimleri korunur. Geometri/hesap araçlarında olmayan çizim işlevi uydurulmaz. Araç türünden birincil çizim eylemine ortak UI eşlemesi kullanılır.

Flow taşıma kalıcı liste sırasını değiştirir; bağlantıları ve ROI'leri değiştirmez. Gerçek yürütme bağımlılıkları mevcut DAG ile belirlenir: tüketici, listedeki konumundan bağımsız olarak üreticisinin sonucunu bekler. Bu ayrım Flow yardımında kısa açıklanır. Bağımsız araçların sırası mevcut deterministik DAG kuralına göre ele alınır; upstream/downstream bağları değişmez. Taşıma tek atomik ProjectService düzenlemesidir; seçili ID korunur ve reçete dirty olur. Aktif Runtime snapshot değişmez; çalışan Editor job sırasında taşıma engellenir veya mevcut generation/stale-result sözleşmesi üzerinden güvenle reddedilir. İlk uygulamada busy sırasında menüyü pasifleştirmek tercih edilir.

## 5. Uygulama paketleri ve doğrulama

### U1 — Properties ve seçim davranışı (1, 3, 4, 5, 8)

- Ortak açılır bölüm ve araç bazlı alan grupları; EdgePointEditor/MetrologyEditor ve MainWindowEditor entegrasyonu. Etiketler kullanıcı dilinde, teknik anahtarlar gerekirse tooltip'te.
- Type/ID satırları kalkar, Çıkışlar kapanır, manuel koordinatlar ayrılır. Basic/Detailed görünüm ve yerel tercih durumu korunur.
- Görüntü tıklaması araç seçmez; ROI iptal/tutamaç/Calibration seçimi gerilemez.
- Kabul: Basic↔Detailed ve bölüm aç/kapatta byte/hash/dirty/no-op Save korunması; hidden değer kaybı yok; gerçek ROI sürükleme ve resme tıklamayla aynı seçili ID; normal ve 850×600 görseller. Küçük ekranda bütün ayarların aynı anda görünmesi şart değildir; temel akış erişilebilir, etiketler sarılır ve gereksiz yatay scroll oluşmaz.

### U2 — Varsayılan görüntü ve çizim hazırlığı (2, 7)

- ProjectService/application düzeyinde atomik ekleme+bağlantı politikası; MainWindow sadece komut ve sunum. UI/OpenCV doğrudan çağrı eklenmez.
- Ortak birincil çizim eylemi ve seçili araç/generation ile sınırlı bekleyen çizim niyeti; bağlı güncel raster kontrolü.
- Kabul: Image File ve Webcam, Preprocess zinciri/dalları, etkin olmayan/eksik kaynak, kaynak yokken ekleme, elle override/bağlantıyı kaldırma, eski reçete/no-op reopen, üç applicationType, Escape/Stop/geç frame, raw/world/pose öğretim kısıtları. Otomatik seçimden sonra gerçek Run Flow; cache yokken Run Tool fallback yapmaz.

### U3 — Flow sıralama (6)

- ProjectService taşıma komutu ve Flow context menu; kimlik/bağlantı/parametre koruma, liste sınırları ve busy kapısı.
- Kabul: üst/alt uç ve tek araçta no-op; Save/reopen sıra; bağımlı/bağımsız araçlarda DAG sonucu ve veri kaynağı; taşıma ile mevcut manuel bağlantının değişmemesi; aktif snapshot korunması.

### U4 — Birleşik kabul ve belge kapanışı

- README yerel toolchain komutlarıyla Release core ve UI build; ilgili service/graph/viewer/editor/pose smoke CTest. Ortak ekleme ve UI akışı değiştiği için finalde mevcut tam core/UI paketleri bir kez çalıştırılır; sadece plan hazırlanırken derleme gerekmez.
- Normal 1400×900 ve küçük 850×600: Edge Point/Pair/Line/Circle Basic/Detailed, Elle gir/Çıkışlar, Flow sağ tık, kaynak seçimi ve otomatik çizim görselleri açılıp incelenir. Görseller/test INI/loglar build altında.
- Üç applicationType aynı Editor/Runtime'ı kullanır; eski reçeteler/typed bağlantılar, mm/px, kalite/tolerans, Stop/eski sonuç kapıları korunur. UI kolaylığı sahte sonuç/pose/PASS üretmez.
- README, gereksinimler, mimari ve progress her tamamlanan pakette gerçek duruma göre güncellenir. Bu dokümandaki işler uygulama ve kabul kanıtı olmadan tamamlandı işaretlenmez.

## 6. İlk plan oturumunun tarihsel sınırı

Kaynak ve belgeler incelendi; kayıtlı CTest log sonları ve mevcut circle-small/edge-editor-small görselleri okundu. Canlı uygulama etkileşimi, yeni build veya yeni test yapılmadı. Bu ilk değerlendirme sırasında yeni davranışlar uygulanmamıştı; U1/U2 teslimi belgenin başında kayıtlıdır. O oturumun sonraki işi **U1** idi; önceki T8.4.2b.3c işi korunur ve bu kullanım paketi sonrasında devam eder.
