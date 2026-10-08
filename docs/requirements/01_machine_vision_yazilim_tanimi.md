# Endüstriyel Machine Vision Yazılımı — Genel Tanım

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Güncel kalibrasyon gereksinimleri ve kabul planı](../architecture/calibration-plan.md). Bu revizyon önceki geliştirme sırası ifadelerine göre önceliklidir.

## 7 Ekim 2026 onaylı geliştirme önceliği

Ürün vizyonu korunur. Genel incelemedeki dokuz mimari öneri [geliştirme planına](../architecture/development-plan.md) alınmıştır: ortak yürütme, referans invalidation, registry, session/controller, owned frame, kamera yaşam döngüsü, koordinat modeli, doğrulanmış üretim snapshot'ı ve persistence/build/test güçlendirmesi. Önce 6A, sonra gerçek kamera (6B), Measurement/kalibrasyon/tolerans/Runtime (6C), PLC (6D), robot guidance (6E) ve ürünleştirme (7) uygulanır. Temel servis yetkisi, hata kodları ve cycle logları ilk Runtime'a dahildir.

**Mevcut GUI görünümü, panel yerleşimi, tema, menüler ve navigasyon korunacaktır.** Bu onay iç mimari ve proje akış planına aittir. Aşağıdaki geniş ürün hedefleri tamamlanmış özellik listesi değildir; geçmiş geliştirme sıraları için güncel plan önceliklidir.


**Ürün:** VisionFlow Industrial Vision Suite  
**Doküman sürümü:** 2.0  
**Revizyon tarihi:** 7 Ekim 2026  
**İlgili dosyalar:** `02_machine_vision_ui_gereksinimleri.md`, `../../prototypes/visionflow_ui_prototype.html`

Bu doküman, önceki yazılım tanımını yeni reçete merkezli GUI kararına göre düzenler. İlk tanımın konuşma aktarımında eksik kalan başlangıcı, mevcut gereksinim kapsamından yeniden oluşturulmuştur. Bu dosya güncellenmiş gereksinim dokümanıdır; önceki metnin birebir transkripti değildir.

## 1. Proje Amacı

Kod yazmadan endüstriyel makine görüş uygulamaları oluşturulabilen, mühendislerin araç parametreleri ve veri akışını ayrıntılı biçimde yönetebildiği bir geliştirme ve çalışma ortamı oluşturulacaktır.

İlk kapsam üç uygulama türünü içerir:

| Uygulama türü | Amaç | Temel çıktı |
| --- | --- | --- |
| Robot Guidance | Parçanın konumunu ve yönelimini bulup robot koordinat sistemine dönüştürmek | Robot pozisyonu, yönelim, skor, geçerlilik |
| Defect Inspection | Eksik/fazla özellikleri ve bölgesel kusurları kontrol etmek | PASS / FAIL ve inceleme detayları |
| Measurement | Kenar, çizgi, daire ve noktalardan ölçüm yapmak | Ölçüm değeri, birim, tolerans sonucu |

Bu türler üç bağımsız program değildir. Aynı vision çekirdeğini, tool sistemini, referans mekanizmasını ve Recipe Editor arayüzünü kullanırlar. Uygulama türü yeni reçete oluştururken seçilir; başlangıç araçları, akış ve sonuç sunumu bu seçime göre hazırlanır.

## 2. Genel Mimari

Katmanlar ve sorumlulukları:

| Katman | Sorumluluk |
| --- | --- |
| UI | Reçete yönetimi, görüntü, overlay, özellikler, flow, kullanıcı etkileşimi |
| Application Services | Proje/reçete işlemleri, çalıştırma komutları, doğrulama ve yetki kontrolü |
| Vision Engine | Tool yürütme, bağımlılık çözümleme, sonuç ve süre yönetimi |
| Tool / Plugin System | Acquisition, filter, match, inspection, measurement, logic, output |
| Hardware / Communication | Kamera sürücüleri, robot/PLC protokolleri, bağlantı yönetimi |
| Persistence | Projeler, reçeteler, modeller, kalibrasyonlar, versiyonlar ve kayıtlar |

UI butonları kamera/robot API'lerini doğrudan çağırmamalıdır. Application Services üzerinden komut üretmeli, durum ve sonuç değişikliklerini ilgili servislerden almalıdır.

## 3. Ana Sayfalar ve Navigasyon

Sol menü:

- **Workspace:** Dashboard, Project Manager, Recipe Manager.
- **System:** Camera Manager, Calibration, Communication, Runtime, Diagnostics, Logs, Users, Settings.

Robot Calibration ve Camera Calibration, Calibration altında ayrı sekmeler veya alt sayfalar olarak sunulabilir. HTML prototipinde mevcut iki kalibrasyon bağlantısı korunur.

**Robot Guidance, Defect Inspection ve Measurement sol menüde bağımsız sayfa bağlantıları olarak yer almaz.** Recipe Manager'da uygulama türü sütunu ve tür filtresi bulunur. Bir reçete açıldığında ortak Recipe Editor çalışma alanına geçilir.

Recipe Editor, seçilen reçetenin bağlamsal düzenleme ekranıdır. Ayrı bir genel amaçlı uygulama türü olarak gösterilmez. Üst bölümde proje, reçete adı, uygulama türü, kamera, sürüm ve kaydedilmemiş değişiklik durumu görünür.

## 4. Recipe Teaching Page / Recipe Editor Akışı

Temel kullanıcı akışı:

1. Recipe Manager'ı aç.
2. New Recipe ile ad, kamera ve uygulama türü seç.
3. Ortak Recipe Editor'e geç.
4. Görüntü al veya dosyadan yükle.
5. Tool ekle, görüntü üzerine yerleştir ve parametreleri ayarla.
6. Tool girişlerini başka tool çıktılarıyla ilişkilendir.
7. Tek tool veya tüm akışı test et.
8. Sonuçları ve overlay'leri incele.
9. Reçeteyi versiyon açıklamasıyla kaydet.
10. Doğrulanmış reçeteyi Runtime'da seçerek çalıştır.

Uygulama türü reçetenin üst düzey metadatasıdır. Tür, oluşturma sırasında başlangıç şablonunu belirler; birbiriyle uyumlu ortak tool'ların kullanılmasını gereksiz biçimde engellemez. Sonradan tür değişimi, mevcut akışın giriş/çıkış ve kalibrasyon gereksinimleri doğrulanarak yapılmalıdır.

## 5. Ortak Recipe Editor

| Bölüm | İçerik |
| --- | --- |
| Sol panel | Toolbox: kategoriler, araç arama ve araca ait kısa açıklama |
| Orta panel | Image Viewer: canlı/kayıtlı görüntü, ROI ve sonuç overlay'leri |
| Sağ panel | Properties: seçilen tool için dinamik girişler ve parametreler |
| Alt panel | Flow, Results, Log, Variables, Communication ve Diagnostics sekmeleri |

Panel boyutları değiştirilebilir olmalıdır. Görüntü ve flow seçimi aynı tool seçimini paylaşır. Bir tool görüntüden veya flow'dan seçildiğinde Properties paneli o tool'a geçer.

Ortak prensip: **Tool ekle → görüntü üzerinde yerleştir → Properties üzerinden ayarla → sonucu görüntü üzerinde gör → başka tool çıktısına referans ver → test et → kaydet.**

Toolbox ve örnek başlangıç flow'u uygulama türüne göre öne çıkarılır. Yeni araç türleri aynı editör çatısı içinde sunulur.

## 6. Toolbox Yapısı

### 6.1 Acquisition Tools

- Camera / Image Acquisition.
- Image File: tek görüntü veya test görüntü dizisi.
- Trigger: software veya external trigger kaynağı.
- Exposure ve Gain: kamera desteklediği ölçüde ayarlanır.

Çıktılar: görüntü, frame ID, zaman damgası, çözünürlük, edinim durumu.

### 6.2 Image Processing Tools

Gray Scale, Threshold, Gaussian Filter, Median Filter, Sharpen, Normalize, Morphology, Erode, Dilate, Open, Close.

Girişler: kaynak görüntü ve isteğe bağlı ROI. Çıktılar: işlenmiş görüntü veya region/mask. Kaynak görüntü referansı korunur.

### 6.3 ROI Tools

Rectangle, Circle, Ellipse, Polygon, Free ROI, Rotated Rectangle.

Parametreler: merkez/konum, genişlik, yükseklik, yarıçap, nokta listesi, açı ve koordinat referansı. ROI mutlak görüntü koordinatında veya Match/Tool Output referansında tanımlanabilir. Rotasyon ve öteleme birlikte takip edilmelidir.

### 6.4 Matching Tools

Pattern Match, Shape Match, Template Match, Edge Match.

Parametreler: model, model ROI, arama ROI, minimum score, minimum/maksimum angle, scale aralığı, maksimum sonuç sayısı, overlap ve timeout.

Çıktılar: X, Y, Angle, Score, Center, Pose2D, contour ve match listesi. X/Y için görüntü koordinatı ve birim açıkça belirtilir.

Gösterim: kırmızı kontur, merkez işareti ve koordinat ekseni. Birden fazla sonuç varsa seçilen/aktif match görünür biçimde ayrılır.

Model teaching ve sonuç filtreleme ayrı işlemlerdir. Brush ile istenmeyen match bölgesini dışlamak bir **result filtering mask** oluşturur; modelin kendisini değiştirmez.

### 6.5 Blob Tools

White Blob, Black Blob.

Parametreler: threshold aralığı, Min Area, Max Area, Circularity, Width, Height ve ROI.

Çıktılar: True/False karar, blob sayısı, alan, merkez ve blob listesi. Alan birimi varsayılan olarak px²'dir; mm² gösterimi kalibrasyon gerektirir.

Blob adaylarını seçen filtreler ile PASS/FAIL kabul kriterleri ayrı alanlarda gösterilir. Örneğin aday alan filtresi 100–100000 px², kabul kriteri “en büyük kusur alanı ≤ 500 px²” olabilir.

### 6.6 Defect Tools

Missing Feature, Extra Feature, Blob Inspection, Scratch Detection, Contrast Inspection.

Girişler: görüntü, inspection ROI, gerekiyorsa referans görüntü/model ve parça poz referansı. Sonuçlar: PASS/FAIL (OK/NOK ile eşdeğer kullanıcı gösterimi), kusur detayları ve overlay.

Algoritma/edinim hatası bir parçanın FAIL sonucu ile karıştırılmamalıdır. ERROR veya SKIPPED yürütme durumu ayrıca raporlanır.

### 6.7 Measurement Tools

Edge Detection, Line Detection, Circle Detection, Corner Detection.

Çıktılar: Point, Line, Circle; ayrıca confidence/strength, found durumu ve kaynak koordinat sistemi.

### 6.8 Calculation Tools

Distance, Angle, X Distance, Y Distance, Diameter, Radius, Offset.

Girişler diğer tool'ların point/line/circle/pose çıktılarıdır. Çıktı değer, birim, geçerlilik ve isteğe bağlı tolerans kararını içerir.

Örnek: `Distance_1.P1 = Edge_1.Point`, `Distance_1.P2 = Edge_2.Point`. 523.42 px × 0.025 mm/px = 13.0855 mm; üç ondalık gösterimde 13.086 mm.

### 6.9 Logic Tools

IF, AND, OR, NOT, Compare, Range Check, Counter.

Koşullar sonuçlara referans verebilir. IF dalları ve yürütülmeyen tool'ların SKIPPED durumları açıkça gösterilir. Counter'ın reset ve reçete değişimi davranışı tanımlanır.

### 6.10 Data Tools

Variable, Array, String, Integer, Float; ayrıca Boolean ve yapılandırılmış point/pose nesneleri.

Global ve recipe-local scope desteklenir. Değişkenin tipi, birimi ve read/write kaynağı görünür olmalıdır.

### 6.11 Robot Tools

Robot Pick Position, TCP Offset, Tool Offset, Coordinate Transform, Robot Output.

Girişler: vision pose, geçerli kalibrasyon, hedef robot frame'i, pick referansı ve offset'ler. Çıktılar: X, Y, Z, Rx, Ry, Rz, frame ID ve valid durumu.

2D kameradan elde edilen X/Y/Angle, tek başına tam 3D pose değildir. Düzlemsel uygulamada Z/Rx/Ry reçete veya öğretimden alınabilir; UI bu bileşenlerin kaynağını gösterir.

## 7. Properties Panel

Her tool için ortak yapı:

| Grup | Alanlar |
| --- | --- |
| General | Name, Description, Enabled, Visible |
| Input | Referenced Tool, Input Image, ROI, Coordinate System |
| Parameters | Tool'a özel parametreler ve doğrulama sınırları |
| Output | Typed output port'ları, birimler, değer ve geçerlilik |
| Display | Color, Thickness, Show Result, contour/center/axis seçenekleri |

Parametre değişiklikleri reçeteyi dirty/draft yapar ve eski sonucun güncel olmadığı belirtilir. Hatalı değerler alan üzerinde gösterilir. Başka tool'a referans verilen alanlarda sabit değer ile bağlı değer ayırt edilir.

## 8. Referans Sistemi ve Data Flow

Örnek referanslar:

```text
Match_1.X
Match_1.Y
Match_1.Angle
Match_1.Pose2D
Blob_2.Result
Edge_1.Point
Distance_1.Value
ROI_1.Center
```

Bir tool başka bir tool'un çıktısını kullanabilir:

```text
ROI_Inspection.Reference = Match_1.Pose2D
Blob_1.Region = ROI_Inspection.Region
Distance_1.P1 = Edge_1.Point
Distance_1.P2 = Edge_2.Point
```

Ekranda anlaşılır tool adları kullanılır; dosyada bağlantılar kalıcı tool ID ve output port ID ile saklanır. Yeniden adlandırma bağlantıları bozmamalıdır. Port tip uyumu, eksik referans ve döngüsel bağımlılık çalıştırmadan önce kontrol edilir. Aynı çevrimde alınan görüntü ve sonuçların cycle ID'si ortak olmalıdır.

Parça takibi için yalnızca `ROI.Center = Match_1.Center` kullanmak yeterli değildir; ROI'nin açısı ve lokal offset'i de match pozuyla dönüşmelidir. UI bunu Reference/Pose binding olarak sunar.

## 9. Robot Guidance Modülü

Başlangıç flow'u:

1. Acquire Image.
2. İsteğe bağlı Filters.
3. Search ROI.
4. Pattern/Shape Match.
5. Pick Reference.
6. Robot Transform.
7. Offset.
8. Robot Output.

Kamera görüntüsü alınır, parça koordinatı bulunur, geçerli kalibrasyonla robot frame'ine dönüşür, ilgili frame'de offset uygulanır ve sonuç iletişim servisine gönderilir.

Çıktılar: X/Y/Z, Rx/Ry/Rz, score, frame, recipe ID, cycle ID ve result-valid. Hatalı/eksik match veya geçersiz kalibrasyonda güncel geçerli pozisyon üretilmez. Robot hareket komutu ile vision sonucunun gönderimi ayrı işlemler olarak gösterilir.

## 10. Kalibrasyon Modülü

Pixel to mm, Camera Calibration, Lens Distortion, Eye-to-Hand, Eye-in-Hand, Perspective Correction.

Çıktılar: Calibration Matrix, Transform Matrix; ayrıca model türü, kaynak/hedef frame, birimler, numune sayısı, hata metriği, kamera/lens/çözünürlük bağlantısı, sürüm ve geçerlilik.

Düzlemsel pixel→robot dönüşümü ile tam 3D eye-to-hand/eye-in-hand kalibrasyonları ayrı prosedürlerdir. Tek bir eşleştirilmiş vision/robot pozisyonu genel dönüşüm matrisini belirlemek için yeterli sayılmaz. Wizard seçilen modele uygun çoklu numune ve doğrulama ister.

## 11. Recipe Yapısı

Reçete en az şunları içerir:

- Recipe ID, Name, Description, Application Type.
- Schema Version ve Recipe Version.
- Camera Settings / camera reference.
- Tool List, parameters ve input/output bağlantıları.
- Model, ROI ve result-filtering mask verileri.
- Robot Offset ve frame bilgileri.
- Calibration Data veya versioned calibration reference.
- Variables ve sonuç kabul kriterleri.
- Runtime output mapping.
- Kullanıcı, tarih, versiyon açıklaması ve doğrulama durumu.

Uygulama türü kalıcı alan olarak `applicationType` ile temsil edilir:

| UI adı | Dosya değeri |
| --- | --- |
| Robot Guidance | `robot_guidance` |
| Defect Inspection | `defect_inspection` |
| Measurement | `measurement` |

Reçete durumu DRAFT/VALID/INVALID; yürütme durumu READY/BUSY/DONE/ERROR; parça kararı PASS/FAIL ayrı kavramlardır. Recipe Version, Schema Version ve engine/plugin sürümleri birbirinden ayrılır.

## 12. Runtime Ekranı

Live Image, Tool Overlay, Match Result, Measurement, Robot Position, Communication Status ve Cycle Time gösterilir. Runtime, aktif reçetenin uygulama türüne göre sonuç kartlarını değiştirir; ayrı runtime programları oluşturulmaz.

| Tür | Öncelikli sonuç sunumu |
| --- | --- |
| Robot Guidance | Pick target, frame, robot pose, score, output status |
| Defect Inspection | Büyük PASS/FAIL, kusur sayısı ve alanı, inspection listesi |
| Measurement | Ölçüm, birim, tolerans, PASS/FAIL ve ölçüm overlay'i |

Sonuç çevrim ID'si ve zamanı görünürdür. Trigger bekleme, busy, iletişim hatası ve eski sonuç açıkça ayrılır. Engineer parametre düzenleme ile Operator üretim görünümü yetkiye göre ayrılır.

## 13. Kullanıcı Seviyeleri

| Rol | Yetki |
| --- | --- |
| Operator | Doğrulanmış reçete seçme, Run/Stop/Reset, üretim sonuçlarını izleme |
| Engineer | Reçete oluşturma/düzenleme, tool ayarlama, kalibrasyon, haberleşme |
| Administrator | Kullanıcı/yetki yönetimi, sistem ve proje ayarları |

Yeni reçete oluşturma Engineer/Administrator yetkisi ister. Yetki denetimi yalnızca UI butonlarını gizlemekle sınırlı tutulmaz; servis katmanında da uygulanır.

## 14. Veri Kayıtları

Görüntü, sonuç, alarm, robot verileri, tarih ve kullanıcı kaydedilir. Ayrıca recipe/version, calibration/version, cycle ID, süre, hata kodu ve output durumu kayıtla ilişkilendirilir.

PASS/FAIL görüntü saklama politikaları ve retention ayrı ayarlanır. Export kayıtları filtreleri ve birimleri korur. Reçete düzenleme ve sistem ayarı değişiklikleri audit kaydına işlenir.

## 15. Gelecekte Eklenebilecek Fonksiyonlar

Deep Learning Classification, OCR, Barcode, 3D Vision, Point Cloud, AI Defect Detection, Segmentasyon, Multi Camera, GPU Acceleration, Script Editor ve gelişmiş Vision Flow Editor.

Yeni uygulama türleri New Recipe şablonlarına eklenir. Yeni tool'lar ortak Recipe Editor ve engine/plugin sistemi üzerinden sunulur; her özellik için sol menüde yeni uygulama sayfası açılması gerekmez.

## 16. Temel Tasarım Felsefesi

Yazılım Vision Builder kullanım kolaylığını, HALCON'un esnekliğini, Cognex'in endüstriyel reçete/runtime yaklaşımını ve Zebra Aurora'nın veri akış mantığını ortak platformda birleştirmeyi hedefler.

Operator doğrulanmış reçetelerle üretim yapar. Engineer ortak Recipe Editor içinde araçları, kalibrasyonu ve veri akışını yönetir. Robot Guidance, Defect Inspection ve Measurement uygulama şablonlarıdır; çekirdek ve editör ortaktır.

Bu doküman “uygulama iskeletini oluştur”, “sınıf yapısını tasarla”, “UI sayfalarını üret” veya “tool mimarisini oluştur” görevlerine temel sağlar. Ayrıntılı Tool Architecture & Data Model dokümanı ayrıca tool port tiplerini, yürütme kurallarını, plugin sözleşmesini ve üretim recipe şemasını tanımlamalıdır.

### HTML prototipinin kapsamı

HTML dosyası görsel ve etkileşimli UI prototipidir. Reçete oluşturma, tür seçimi, ortak editöre geçiş, arama/filtreleme, kopyalama, browser draft kaydı, JSON import/export ve tür bazlı runtime görünümü örneklenir. Vision algoritmaları, kamera/robot bağlantısı, gerçek kalibrasyon, sürükle-bırak flow, ROI çizimi, yetkilendirme ve üretim doğrulaması uygulanmış değildir. Run Demo önceden tanımlı örnek sonuçlar gösterir.
