# Endüstriyel Machine Vision Yazılımı — UI Sayfa ve Ekran Gereksinimleri

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. Yeni modeller henüz uygulanmadı; sonraki uygulama adımı 6C-K1 model sözleşmesi, geçici Calibration görüntü import ve otomatik plaka tespitidir. [Güncel kalibrasyon gereksinimleri ve kabul planı](../architecture/calibration-plan.md). Bu revizyon önceki geliştirme sırası ifadelerine göre önceliklidir.

## 7 Ekim 2026 — Mevcut GUI'yi koruma kararı

Kullanıcı mevcut GUI'den memnundur. [Onaylanan mimari plan](../architecture/development-plan.md) uygulanırken tema, görünüm, panel yerleşimleri, menü adları/sırası ve navigasyon değiştirilmeyecektir. Yeni menü, node canvas veya genel görsel yeniden tasarım bu çalışmanın kapsamında değildir. Sonraki işlevler mevcut ayrılmış ekranlara ve kontrollere bağlanır; bugünkü hazırlık ekranları çalışan özellik sayılmaz.

Run Match/Run Flow davranış tutarlılığı ve referans invalidation mevcut kontroller üzerinden düzeltilecek; session/controller ayrımı UI iç yapısında yapılacaktır. UI bağlantısı değişen adımlarda Release, ilgili CTest ve mevcut görünümle görsel karşılaştırma gereklidir. Aşağıdaki eski geliştirme sıralarına göre güncel 6A–6E planı önceliklidir; diğer görsel genişletmeler gelecekte ayrı kapsam kararı gerektirir.


**Ürün:** VisionFlow Industrial Vision Suite  
**Doküman sürümü:** 2.0  
**Revizyon tarihi:** 7 Ekim 2026  
**İlgili genel tanım:** `01_machine_vision_yazilim_tanimi.md`  
**Görsel prototip:** `../../prototypes/visionflow_ui_prototype.html`

Önceki UI tanımının 46 bölümlük kapsamı korunmuş ve yeni GUI kararına uyarlanmıştır. **Vision Editor, Defect Detect ve Measurement bağımsız sol menü sayfaları olmaktan çıkarılmıştır.** Robot Guidance, Defect Inspection ve Measurement yeni reçete oluştururken seçilen uygulama türleridir. Üç tür ortak **Recipe Editor** ekranına açılır. Bu doküman hedef ürün gereksinimlerini tanımlar; HTML prototipinin uyguladığı etkileşimler en sonda ayrıca belirtilmiştir.

## 1. UI Genel Tasarım Mimarisi

Ana arayüz modern, koyu temalı, endüstriyel kullanıma uygun ve yüksek kontrastlı olmalıdır. Renk tek başına durum göstergesi olarak kullanılmaz; metin ve ikon eşlik eder.

| Bölge | İçerik |
| --- | --- |
| Sol navigasyon | Workspace ve System ekranları |
| Üst bar | Proje, aktif reçete, uygulama türü, bağlantı/durum ve kaydetme |
| Editor sol panel | Toolbox |
| Editor orta panel | Image Viewer / isteğe bağlı genişletilmiş Flow Editor |
| Editor sağ panel | Properties |
| Editor alt panel | Flow, Results, Log, Variables, Communication, Diagnostics |

Panel genişlikleri/yükseklikleri değiştirilebilir, düzen kullanıcı bazında saklanabilir olmalıdır. Küçük ekranda paneller sekmeli veya alt alta görünmeli; toolbar ve Properties erişilebilir kalmalıdır.

## 2. Ana Menü

### Sol navigasyon

| Grup | Sayfalar |
| --- | --- |
| Workspace | Dashboard, Project Manager, Recipe Manager |
| System | Camera Manager, Calibration, Communication, Runtime, Diagnostics, Logs, Users, Settings |

Calibration içinde Camera / Pixel / Robot alt ekranları bulunur. HTML prototipinde Robot Calibration ve Camera Calibration bağlantıları ayrı korunur.

Recipe Editor, Recipe Manager'da seçilen reçeteden açılır. Uygulama türleri için ayrı navigasyon butonları oluşturulmaz. Yeni uygulama türleri New Recipe seçimlerine eklenir.

### Komut menüleri

| Menü | Komutlar |
| --- | --- |
| File | New Project, Open Project, Save Project, Save As, Import, Export, Exit |
| Project | Project Settings, Recipe Manager, User Management |
| Camera | Camera Manager, Camera Settings, Trigger |
| Calibration | Camera Calibration, Pixel Calibration, Robot Calibration |
| Recipe | Open Editor, Tool Library, Variables, Test, Validate, Version History |
| Communication | Robot, PLC, Protocol Settings |
| System | General Settings, Logging, Diagnostics, About |

Menü komutları rol yetkisine ve geçerli sistem durumuna göre etkinleştirilir.

## 3. Home / Dashboard

Program açıldığında Dashboard gösterilir. Proje, reçete, uygulama türü, kamera ve robot bilgileri üst bölümde görünür.

Orta bölümde canlı kamera görüntüsü ve FPS, resolution, exposure, current recipe, trigger status bulunur. Sağda Camera/Robot/PLC connected durumları, calibration geçerliliği ve recipe ready durumu gösterilir.

Alt bölüm son çevrimi ve son işlemleri gösterir: Cycle Time, Vision Result, Match Score veya uygulama türüne uygun ana sonuç, Robot Response / Output Status.

Durumlar Connected/Warning/Error metni ve yeşil/sarı/kırmızı göstergelerle verilir. Dashboard'da aktif reçeteyi Recipe Editor'de açma veya Runtime'a geçme komutu bulunabilir.

## 4. Project Manager

Yeni proje alanları: Project Name, Project Location, Camera, Robot Type (gerekiyorsa), Communication Protocol ve Calibration. Create / Cancel butonları bulunur.

Proje ağacı Cameras, Recipes, Calibration, Communication, Variables ve Logs düğümlerini içerir. Reçeteler uygulama türü etiketiyle listelenir.

Project Settings'te açıklama, varsayılan donanım ve proje sağlığı gösterilir. Proje ayarları ile aktif reçete parametreleri ayrı yönetilir.

## 5. Recipe Manager ve New Recipe

Recipe Manager operatörün doğrulanmış reçete seçtiği, Engineer'ın reçete oluşturup düzenlediği ekrandır.

Tablo sütunları: Recipe Name, **Application Type**, Description, Camera, Version, Status ve Modified. Search ve Application Type filtresi bulunur.

Komutlar: New, Open Editor, Copy, Delete, Import, Export, Version History. Delete kimliği açıkça gösterir; aktif runtime reçetesi için ürünün kullanım durumuna göre kısıtlanır.

### New Recipe diyaloğu

Önce üç karşılaştırılabilir kart sunulur:

| Kart | Açıklama | Başlangıç akışı |
| --- | --- | --- |
| Robot Guidance | Parçayı bul ve pozisyon/yönelimini robot için hesapla | Image → Match → Pick Reference → Transform → Output |
| Defect Inspection | Bölgesel kusurları incele ve PASS/FAIL üret | Image → Reference → Inspection ROI → Defect → Result |
| Measurement | Mesafe, çap, açı ve özellikleri ölç | Image → Features → Calculate → Limits → Result |

Recipe Name ve Camera girilir. Gerekliyse Calibration Reference ayrıca seçilir. **Create & Open Editor** ortak Recipe Editor'e geçer. Reçete türü `applicationType` alanında saklanır.

İsim benzersizliği, gerekli alanlar ve donanım referansları doğrulanır. Yeni reçete DRAFT durumunda oluşur; yalnızca oluşturulmuş olması onu VALID yapmaz.

Mevcut satır açıldığında uygulama türü otomatik yüklenir. Tür değiştirme komutu başlangıç flow'unu sessizce sıfırlamaz; var olan akışın uyumluluğunu doğrular.

## 6. Recipe Teaching / Ortak Recipe Editor

Önceki “Vision Editor” ortak **Recipe Editor** adını alır. Robot Guidance, Defect Inspection ve Measurement aynı ekran yerleşimini ve aynı tool/data-flow altyapısını kullanır.

Üst bölüm: Recipes'e geri dönüş, Recipe Name, Application Type, Camera, Version, Dirty/Draft durumu, Test/Run, Save, Export ve Runtime.

Ana bölümler Toolbox, Image Viewer, Properties ve alt Flow/Result/Log'dur. Uygulama türü toolbox önceliğini, başlangıç flow'unu ve sonuç kartlarını belirler. Ortak araçlar uyumlu oldukları sürece tüm türlerde kullanılabilir.

Kaydedilmemiş değişiklikler görünür olmalıdır. Reçete değiştirme veya ekran kapatma durumunda değişiklikler kaybolmamalı; draft korunmalı veya açık Save/Discard seçimi sunulmalıdır.

## 7. Toolbox

| Kategori | Araçlar |
| --- | --- |
| Acquisition | Camera, Image, Trigger, Exposure, Gain |
| Image Processing | Gray Scale, Threshold, Gaussian Filter, Median Filter, Sharpen, Normalize, Morphology, Erode, Dilate, Open, Close |
| ROI | Rectangle, Circle, Ellipse, Polygon, Free ROI, Rotated Rectangle |
| Pattern | Pattern Match, Shape Match, Template Match, Edge Match |
| Inspection | Blob, Brightness, Contrast, Defect, Presence, Absence, Missing Feature, Extra Feature |
| Measurement | Edge, Line, Circle, Corner, Distance, Angle, Diameter |
| Logic | IF, AND, OR, NOT, Compare, Range Check, Counter |
| Data / Output | Variable, Result, Robot Output, PLC Output |
| Robot Guidance | Pick Reference, TCP Offset, Tool Offset, Coordinate Transform |

Her nesne drag & drop ile Image Viewer veya Flow Editor'e bırakılabilmelidir. Klavye veya Add Tool komutu alternatif olmalıdır. Araç arama, açıklama ve giriş/çıkış tipleri gösterilir.

## 8. Image Viewer

İşlemler: Zoom In/Out, Fit Image, Pan, Rotate View, Pixel Inspection, Histogram, Grid, Crosshair.

Üst toolbar: Live, Capture, Open, Save Image, Zoom +/−, Fit, Grid, Crosshair, Undo, Redo.

Viewer rotasyonu yalnızca görüntüleme dönüşümü olabilir; gerçek image processing rotasyonu ayrı tool'dur. Koordinat dönüşümleri ROI/overlay seçimini doğru görüntü pikseline eşlemelidir. Piksel koordinatı, görüntü çözünürlüğü ve zoom görünürdür.

## 9. Image Overlay

Match: kırmızı contour, merkez, X/Y ekseni, angle, score ve seçilen match ID. Inspection: ROI ve kusur bölgeleri. Measurement: edge/line/circle, ölçüm çizgileri, değer ve birim.

Örnek: `X: 124.52 px`, `Y: 235.14 px`, `Angle: −7.2°`, `Score: 96.4%`.

Görüntü piksel koordinatı ile kalibre edilmiş mm/robot koordinatı aynı etikette karıştırılmaz. Her overlay kaynak tool ve cycle ID ile ilişkilidir. Disabled/skipped sonuçlar aktif sonuçlardan ayrılır.

## 10. Tool Selection

Image Viewer veya Flow üzerinde tool seçildiğinde Properties otomatik değişir. Seçim görüntü, flow ve sonuç tablosunda senkronize edilir.

Pattern Match örneği: General (Name, Enabled), Search Region (X, Y, Width, Height), Matching (Min Score, Angle Min/Max, Scale Min/Max), Results (X/Y/Angle/Score).

Seçilen tool vurgulanır; Escape seçimi temizler. Overlay'lerin üst üste olduğu bölgede tool seçme listesi sunulabilir.

## 11. Properties Panel

Panel dinamik olmalıdır; yalnızca seçilen tool'un ilgili özellikleri gösterilir. Ortak alanlar Name, Description, Enabled, Visible, Input, Reference, ROI, Coordinate System'dir.

General/Input/Parameters/Output/Display grupları kullanılır. Bağlı alanlarda referans kaynağı gösterilir. Parametre tipi, aralık ve birim doğrulanır. Kaydedilmemiş parametre değişikliği reçeteyi dirty/draft yapar ve eski sonucu geçersizleştirir.

## 12. Match Properties

Alanlar: Model seçimi, Search ROI, Min Score, Max Results, Angle Min/Max, Scale Min/Max, Overlap, Timeout.

Display: Show Contour, Show Center, Show Coordinate. Model status ve son teach zamanı görünür.

Sonuç listesi: Match #, X px, Y px, Angle °, Score %, Filtered/Enabled. Örnek: X 125.4, Y 235.2, Angle 8.2°, Score 96.5%.

Score gösteriminde 0–1 veya 0–100 ölçeği tutarlı seçilir; UI yüzde kullanabilir. Minimum/maksimum açı ve ölçek sınırları kontrol edilir.

## 13. Match Model Teaching

Create Model → Model ROI / ROI şekli → Teach → Test → Update Model → Save.

Kullanıcı görüntü üzerinde model bölgesini seçer. Model preview, modelin kaynak görüntüsü ve ROI'si görünür. Update Model, sonucu filtreleme işleminden ayrıdır. Model kaydı reçete sürümüyle ilişkilendirilir.

## 14. Match Result Filter / Brush

Detected Matches sayısı ve her match contour'u görüntü üzerinde gösterilir. Brush Size px ve Delete Match / Keep Match / Add Region modları bulunur.

Fırça ile işaretlenen bölgeler **result filtering mask** oluşturur; match modelini değiştirmez. Mask referansı Absolute veya Part Reference olarak seçilir. Excluded match sayısı, Undo/Redo ve Clear Mask bulunur.

## 15. ROI Editor

Şekiller: Rectangle, Circle, Ellipse, Polygon, Rotated Rectangle, Freehand.

Properties: X, Y, Width, Height, Radius, Rotation, point list ve Reference. Reference seçenekleri Absolute, Match Pose, Tool Output.

Örnek `Center X = Match_1.X + 20`, `Center Y = Match_1.Y - 15` bağları gösterilebilir. Parçanın rotasyonunu da izlemek için pose binding kullanılır; lokal offset match açısıyla birlikte dönüşür. ROI referans çizgisi ve lokal eksenleri görüntüde gösterilir.

## 16. Defect Inspection Çalışma Düzeni

Defect Inspection ayrı menü sayfası değil, bu türde açılan reçetenin Recipe Editor düzenidir.

Başlangıç akışı: Image → Filter → Reference Match → Tracking Inspection ROI → Blob/Defect → Result. Sabit fikstürde reference match isteğe bağlı olabilir.

Toolbox inspection araçlarını öne çıkarır. Properties seçilen blob/defect tool'a geçer. Viewer kusurları ve tracking ROI'yi; Results inspection listesi ve final PASS/FAIL'i gösterir.

## 17. Defect / Blob Properties

Inspection Type: Black Area, White Area, Missing Area, Extra Area. Threshold Min/Max. Blob Filtering: Min/Max Area, Min/Max Width, Min/Max Height, Circularity.

Result Logic: Presence/Absence, permitted count, permitted defect area veya seçilen karşılaştırma. Filtreler tespit edilen adayları seçer; kabul kriterleri parçanın PASS/FAIL kararını belirler. Bu iki grup aynı anlama geliyormuş gibi gösterilmez.

Örnek: aday filtresi 100–100000 px², en büyük izin verilen kusur 500 px². 734 px² kusur tespit edilirse FAIL. px² → mm² dönüşümü uygun kalibrasyon gerektirir.

## 18. Defect Result

İki seviyeli parça kararı: PASS/FAIL; ayrıca Detected Area, Expected/Allowed Area, Blob Count ve kusur detayları.

Birden fazla incelemede Inspection #1 PASS, #2 PASS, #3 FAIL, Final Result FAIL örneği gösterilir. Final logic AND/OR veya explicit expression olarak yapılandırılır.

ERROR ve SKIPPED yürütme durumları PASS/FAIL'den ayrıdır. Hatalı çalışmış inceleme PASS olarak kabul edilmez. Kusur listesinden ilgili overlay'e odaklanılabilir.

## 19. Measurement Çalışma Düzeni

Measurement ayrı menü sayfası değil, bu türde açılan reçetenin Recipe Editor düzenidir.

Başlangıç akışı: Image → Filter → ROI → Optional Reference Match → Edge_1 / Edge_2 → Calculate → Calibration / Limits → Result.

İki edge aynı kaynaktan beslenen ayrı feature araçlarıdır; biri diğerinin yerine kullanılmaz. Çıktı değeri, birimi, tolerans ve referans kalibrasyon açıkça gösterilir.

## 20. Edge Tool

Image üzerine edge tool bırakıldığında search ROI, scan direction ve bulunan edge noktası çizilir.

Properties: Direction (Left→Right, Right→Left, Top→Bottom, Bottom→Top), Edge Type (Rising/Falling/Any), Threshold, Filter, Min Contrast, Search Length.

Sonuç: Point X/Y, Edge Strength, Found, Execution Time. Algoritma destekliyorsa subpixel seçenekleri ve profile görünümü sunulur. Found=false durumunda bağlı measurement tool geçerli sonuç üretmez.

## 21. Calculate Tool

İki veya daha fazla tool çıktısı seçilebilir. Örnek P1 = `Edge_1.Point`, P2 = `Edge_2.Point`.

Calculation: Euclidean Distance, X Distance, Y Distance; ayrıca Angle, Diameter, Radius, Offset.

Örnek sonuç: Pixel Distance 523.42 px, Scale 0.025 mm/px, Value 13.0855 mm; UI üç ondalıkla 13.086 mm gösterir. Tolerans karşılaştırması yuvarlanmış metin yerine ham değerle yapılır.

Perspective/lens distortion durumunda tek sabit scale yerine kalibrasyon modelinin dönüştürülmüş geometrisi kullanılır. UI kalibrasyon türünü ve birimi gösterir.

## 22. Dinamik Referans Sistemi

Örnek: `Match_1.Pose2D → ROI_Inspection.Reference`; `ROI_Inspection.Region → Blob_1.Region`; `Edge_1.Point → Distance_1.P1`.

Referans seçici tool/output port listesini, veri tipini ve birimini gösterir. Bağlantılar kalıcı ID'lerle saklanır; tool adı değiştirmek bağlantıyı bozmaz. Silinen tool'a kalan referanslar INVALID olarak işaretlenir.

Parça hareket edip döndüğünde tracking ROI ve bağlı inspection tool'ları aynı pose dönüşümünü kullanır. Döngüsel bağımlılık, uyumsuz tip ve eski çevrim sonucu çalıştırma öncesinde/çalıştırırken raporlanır.

## 23. Robot Guidance Teaching Wizard

Wizard, Robot Guidance reçetesinden açılır; bağımsız uygulama ekranı değildir.

1. Kamera seç; exposure/trigger ayarlarını kontrol et ve görüntü al.
2. Match modeli ve arama ROI'sini öğret; sonucu test et.
3. Düzlemsel/3D model ve Eye-to-Hand/Eye-in-Hand kurulumunu seç.
4. Mevcut geçerli kalibrasyonu seç veya Robot Calibration wizard'ına geç.
5. Pick Reference öğret: robotun bilinen pozunu ve ilgili vision match'i eşleştir; robot frame, TCP/tool ve lokal offset'i kaydet.
6. Kalibrasyon + vision pose + pick reference üzerinden hedef robot pozunu hesapla.
7. Önce hesaplanan pozisyonu görüntüde ve sayısal olarak doğrula; gerektiğinde ayrı robot test komutunu kullan.
8. Robot Guidance verilerini reçeteyle kaydet.

Tek eşleştirilmiş pozisyon pick offset/reference öğretmek için kullanılabilir; genel kamera–robot dönüşümünün kalibrasyonu yerine geçmez. “Send Robot To Pick Position” ilk kalibrasyondan önce otomatik hareket adımı olarak sunulmaz. Test komutlarında hedef frame/pose ve iletişim sonucu açıkça gösterilir.

2D match'ten türetilen X/Y/Rz ile öğretimden gelen Z/Rx/Ry kaynakları UI'da ayrı etiketlenir.

## 24. Robot Calibration

Camera, Robot, Calibration Type, model türü, camera/robot frame, TCP/tool ve sample listesi bulunur. Eye-to-Hand ve Eye-in-Hand prosedürleri aynı varsayımlarla hesaplanmış gibi sunulmaz.

Sample tablosunda Vision X/Y veya seçilen modelin camera pose'u; Robot X/Y/Z/Rx/Ry/Rz; frame; timestamp; valid/rejected ve residual gösterilir.

Komutlar: Capture/Add Sample, Remove Sample, Calculate, Validate, Accept Calibration, Save. Seçilen modelin gerektirdiği yeterli ve uygun dağılımlı numuneler olmadan hesap/kabul etkinleştirilmez.

Error alanı birimi ve türüyle gösterilir: örneğin düzlemsel holdout RMS = 0.18 mm. Calibration INVALID mesajı, nedeni ve önerilen işlemle sunulur. Matris, sürüm ve numuneler korunur.

## 25. Camera / Pixel Calibration

Calibration Target, Camera, Resolution, Lens, target ölçüleri, Acquire, Calculate ve Validate bulunur.

Sonuçlar: Pixel Size / planar scale, lens distortion model/coefficients, reprojection error, Camera Calibration Matrix ve Valid status. Tek “Distortion %” alanı model/ölçüm tanımı olmadan yeterli sayılmaz.

Kalibrasyon kamera/lens/çözünürlük ve gerekiyorsa çalışma düzlemiyle ilişkilidir. İlgili ayarlar değiştiğinde valid durumu yeniden değerlendirilir. Kamera kalibrasyonu ile robot dönüşümü ayrı fakat bağlı kayıtlar olarak sunulur.

## 26. Communication

Robot/PLC sekmeleri, Protocol (TCP Socket, Modbus TCP, OPC UA), IP Address, Port ve protokole özgü parametreler bulunur. Connect, Disconnect, Test Connection ve Save komutları servis katmanına gider.

Data mapping:

| Yön | Alanlar |
| --- | --- |
| Robot/PLC → Vision | Trigger, Recipe ID, Cycle/Request ID, Robot X/Y/Z/Rx/Ry/Rz |
| Vision → Robot/PLC | Ready, Busy, Done, Result Valid, PASS/FAIL, Vision/Robot X/Y/Z/Angle, Score, Error Code, Cycle ID |

Tip, signed/unsigned, ölçek, birim, register/variable adresi ve byte/word order gösterilir. Trigger/Busy/Done/Acknowledge handshake durumları ve timeout görünürdür. Sonuç pozisyonunun camera mı robot frame'inde mi olduğu açıkça yazılır.

## 27. Communication Diagnostic

Canlı monitor: Timestamp, Direction, Cycle ID, Payload/Data, Protocol ve Status. Örnek sıra RX TRIGGER, RX RECIPE, TX BUSY, TX X/Y, TX RESULT, TX DONE, RX ACK.

Filtreler RX/TX, device, error ve cycle. Pause, Clear, Export bulunur. Kayıtlar işlem zamanıyla ve milisaniye çözünürlüğüyle gösterilir; bağlantı hatası ile vision FAIL ayrı raporlanır.

## 28. Runtime

Üretim ekranı sade olmalıdır. Üstte Recipe, Application Type, Version ve System Ready/Busy/Error. Ortada kamera görüntüsü ve uygulama overlay'leri.

| Tür | Sonuç ve bilgi kartları |
| --- | --- |
| Robot Guidance | Vision validity, pick target, X/Y/Z/Rx/Ry/Rz, frame, score, output acknowledgement |
| Defect Inspection | PASS/FAIL, kusur sayısı, alan, inspection summary |
| Measurement | Value, unit, tolerance, PASS/FAIL, ölçüm summary |

Altta Cycle Time, Camera/Robot/PLC status, Trigger, Cycle ID ve result timestamp. Tek runtime, aktif reçetenin türüne uyarlanır. DRAFT/INVALID reçete üretime alınmadan gerekli doğrulama yapılır. Henüz çalışmamış reçetede sahte PASS veya eski reçetenin sayıları gösterilmez.

## 29. Manual Mode

Yetkiye göre Trigger Camera, Live, Run Vision, Send Result, Robot Request, Reset ve Clear Errors komutları bulunur. Vision test ile robot hareketi ayrı komutlardır.

Tool sonuç listesi: Match_1 PASS, Blob_1 PASS/FAIL, Edge_1 FOUND, Edge_2 FOUND, Distance_1 12.54 mm. Input image/cycle ve sonucu ne zaman ürettiği gösterilir.

## 30. Debug Mode

Engineer için Run All, Run Selected, Step, Stop ve dependency görünümü bulunur. Bağımlılığı eksik tool çalıştırılmaz veya gerekli upstream akış açıkça gösterilerek çalıştırılır.

Tool listesinde OK/FAIL/ERROR/SKIPPED/Disabled, execution time ve primary output görünür. Seçilen tool için input/output image, parameters, result, reference ve zaman gösterilir. Yürütme durumu ile parça kararı ayrıdır.

## 31. Flow Editor

Flow ortak Recipe Editor'ün parçasıdır; hem sıralı liste hem node görünümü sunulabilir. Tool'lar arasında gerçek veri bağlantıları ve typed input/output port'ları bulunur.

Örnek: Camera → Filter → Match. Match pose'u hem Tracking ROI/Blob'a hem Edge ROI'lerine referans olur; Edge_1 ve Edge_2 çıktıları Distance'a bağlanır. Bu dallanma tek sıra listede kaybolmamalıdır.

Add/Delete/Move Node, Connect/Disconnect, Zoom/Fit, Undo/Redo ve Validate bulunur. Grafik döngüsel bağımlılığı ve eksik port'u işaretler. Uygulama türü başlangıç şablonunu belirler; flow editörü ortaktır.

## 32. Tool Node Tasarımı

Node üzerinde icon, tool name, execution status, primary result ve execution time bulunur. Pattern Match örneği: Match_1, DONE, PASS, Score 96.5%, X/Y px.

Selected, Disabled, Running, Error ve Skipped görünümleri ayırt edilir. Seçildiğinde Properties ve görüntü overlay'i aynı tool'a odaklanır. Node üzerindeki kısa sonuç, ayrıntılı output listesinin yerine geçmez.

## 33. Tool Input / Output Portları

| Kaynak port | Hedef port |
| --- | --- |
| `Match_1.Pose2D` | `ROI_1.Reference` |
| `Match_1.X` | `Variable_MatchX.Value` |
| `Edge_1.Point` | `Distance_1.P1` |
| `Edge_2.Point` | `Distance_1.P2` |
| `Robot_Transform.Pose` | `Robot_Output.Pose` |

Portların tipi (Image, Region, Point, Line, Circle, Pose2D/Pose3D, Number, Boolean vb.), birimi ve koordinat sistemi görünür. Uyumsuz bağlantı engellenir veya açık dönüşüm tool'u istenir.

## 34. Variable System

Global ve Local variable desteği bulunur. Örnek Global: CameraScale, RecipeID, RobotX, RobotY. Local: MatchX, MatchY, BlobArea, Distance.

Properties içinden variable seçilebilir. Listede Name, Type, Scope, Unit, Value, Source, Read/Write ve Updated At bulunur. Recipe-local değişkenler başka reçeteye sessizce taşınmaz. İsim ve tip değişiklikleri bağlı alanlarda doğrulanır.

## 35. Alarm / Diagnostic

Alarm tablosu Time, Code, Description, Severity, Source, Cycle ID ve Acknowledged içerir. Örnek V001 Camera disconnected, V014 Match score too low, V031 Calibration invalid.

All/Warning/Error/Critical filtreleri, ayrıntı paneli ve ilgili tool/config ekranına gitme komutu bulunur. Parça FAIL üretim sonucu ile altyapı/algo hatası ayrı kategorilerdir.

## 36. Log

Filtreler Date, Recipe, Application Type, Version, Result, User, Error ve Cycle ID.

Kayıt: Timestamp, Recipe/Version, Image, Vision Result, Robot Position, Calibration Version, Cycle Time, Error, User. Görüntü ve overlay yeniden incelenebilir. Export filtreleri ve birimleri korur. Storage/retention ayarlarıyla ilişkilidir.

## 37. User Management

| Rol | UI yetkileri |
| --- | --- |
| Operator | Valid recipe select, Run, Stop, Reset, sonuç izleme |
| Engineer | Recipe create/edit, tools, calibration, communication, debug |
| Administrator | User/role management, system settings, project management |

Yetkisiz komutlar gizlenebilir veya nedenleriyle disabled olabilir. Servis katmanı aynı yetkileri doğrular. Aktif kullanıcı, rol ve oturum durumu görünür. Sistem ayarları için Engineer'a varsayılan Administrator yetkisi verilmez.

## 38. Settings

General: Language, Units, Theme, Date/Time. Vision: Default Score, Default Timeout, Processing Mode. Camera: Default Exposure, Trigger. Robot: Coordinate System, Unit, Orientation Convention, Timeout.

Logging: PASS/FAIL image storage, retention, output path. UI: panel layout, image/overlay defaults, decimal precision. Varsayılan değişikliği mevcut reçeteyi sessizce değiştirmez.

## 39. Save / Version Control

Kaydetme: Recipe, Application Type, Version, Description/Change Reason, User, Date. Eski versiyonlar korunabilir; production/validated versiyon ile draft ayrı tutulur.

Parametre, model, mask veya kalibrasyon referansı değişimi yeniden doğrulama gerektirir. Reçete sürümü, veri şeması sürümü ve plugin/engine sürümleri ayrı kaydedilir. Dirty göstergesi gerçek kaydetme sonucunda kapanır; hata olursa değişiklikler korunur.

## 40. Import / Export

Üretim recipe paketi örneği `Part_001.recipe`. İçerik: Application Type, tool config, parameters, models, ROI, masks, calibration reference/data, robot transformation, variables, output mappings ve version metadata.

Import sırasında schema/plugin sürümü, referanslar, gerekli modeller ve kalibrasyon uyumluluğu doğrulanır. İsim/ID çakışması açıkça yönetilir. Yeni import draft olarak alınır. JSON veya paket formatı engine mimarisi dokümanında kesinleştirilir; HTML demo JSON yalnızca prototip formatıdır.

## 41. Error Handling UI

Her hata Error Code, Description, Cause, Recommended Action, Source Tool/Device, Cycle ID içerir. İlgili ekrana gitme butonu bulunur.

Örnek V-102 MATCH FAILED: Score 42%, Minimum 70%; Possible Causes: Part missing, lighting problem, wrong recipe, camera moved, model incorrect; Open Match Tool.

Bağlantı/kalibrasyon/parametre hatası ile normal inspection FAIL ayrı mesajlar alır. Son geçerli sonucu yeni hatalı çevrimin sonucuymuş gibi gösterme.

## 42. Temel Kullanıcı Akışları

### Ortak başlangıç

Recipe Manager → New Recipe → Application Type + Name + Camera → Create → **Recipe Editor**.

### Robot Guidance

Capture → Exposure → Filter → Search ROI → Teach Match → Test Match → Filter Unwanted Results → Calibration → Pick Reference Teaching → Transform/Offset → Test Target → Save/Validate → Runtime.

### Defect Inspection

Capture → Filter → Optional Reference Match → Tracking Inspection ROI → Blob/Defect → Threshold + Candidate Filters → Acceptance Logic → Test PASS/FAIL → Save/Validate → Runtime.

### Measurement

Capture → Filter → ROI → Optional Match → Edge_1 / Edge_2 (veya Line/Circle) → Calculate → Calibration → Tolerances → Test → Save/Validate → Runtime.

Üç akışta da editör ve veri modeli ortaktır. Calibration veya Communication'a geçişte aktif reçete bağlamı korunur.

## 43. Kritik UI Kuralı

**Tool ekle → görüntü üzerinde yerleştir → Properties üzerinden ayarla → sonucu görüntü üzerinde gör → başka tool'ların çıktısına referans ver → test et → kaydet.**

Operator doğrulanmış uygulamayı kod yazmadan kullanır. Engineer aynı sistemin parametrelerini ve veri akışını yönetir. UI'da “Vision Editor” bir uygulama türü olarak gösterilmez; ortak çalışma ekranının adı Recipe Editor'dür.

## 44. AI Kod Geliştirme İçin Uygulama Kuralı

Bütün modüller tek seferde uygulanmaya çalışılmamalıdır. Geliştirme sırası:

1. Main Window ve navigasyon.
2. Project Manager.
3. Camera Manager / görüntü kaynağı abstraction.
4. Recipe Manager + Application Type seçimi.
5. Ortak Recipe Editor.
6. Image Viewer.
7. Toolbox.
8. Dinamik Properties System.
9. ROI System.
10. Match Tool / teaching.
11. Blob / Defect Tools.
12. Edge Tool.
13. Calculate Tool.
14. Flow Editor ve typed bağlantılar.
15. Calibration.
16. Robot / PLC Communication.
17. Tür bazlı Runtime.
18. Diagnostics.
19. User Management.
20. Logging / version history.

Her aşama sonunda uygulama compile edilebilir ve çalışabilir olmalıdır. Yeni modül önceki modülleri bozmamalıdır. Kritik akışlar için aşamaya uygun doğrulama yapılır.

## 45. UI Geliştirmede Temel Mimari Kural

UI vision algoritmalarına ve donanım API'lerine doğrudan bağlanmaz.

| Katman | Bileşenler |
| --- | --- |
| UI | Project Manager, Recipe Manager, Recipe Editor, Properties Editor, Runtime |
| Application Services | Recipe load/save, command handling, validation, authorization |
| Vision Engine | Acquisition pipeline, image processing, match, blob, edge, measurement, logic |
| Hardware / Communication | Camera adapters, robot adapters, PLC adapters, protocol services |

UI komut gönderir; sonuçları ve durumları servis üzerinden alır. Yeni kamera/robot protokolü aynı UI çatısını kullanabilir. Application Type bir recipe alanıdır; üç ayrı engine veya editor sınıf ailesi gerektirmez.

## 46. İlk Geliştirilecek UI ve Kabul Kriterleri

İlk ürün aşamasında Main Window, Project Manager, Recipe Manager, New Recipe ve ortak Recipe Editor çalışır hale getirilir. Editor Toolbox, Image Viewer, Properties, Flow ve Result alanlarını içerir.

İlk tool kapsamı Image, ROI, Pattern Match, Blob, Edge ve Calculate. Application Type seçimi aynı editöre farklı başlangıç şablonları yükler. Robot, calibration ve communication daha sonra eklenir.

Kabul kriterleri:

- Sol menüde Vision Editor / Defect Detect / Measurement ayrı uygulama sayfaları bulunmaz.
- Recipe Manager'da her reçetenin uygulama türü görünür ve filtrelenebilir.
- New Recipe üç tür arasından seçim yaptırır.
- Her tür aynı Recipe Editor yerleşimine açılır; başlıkta aktif reçete ve tür görünür.
- Tool seçimi Properties ve overlay'i senkronize eder.
- Tool/output referansları, birimler ve koordinat sistemleri kaydedilir.
- Parametre değişiklikleri dirty/draft yapar; eski sonuç güncel sonuç sayılmaz.
- Save / reopen reçete türünü, tool parametrelerini ve bağlantıları korur.
- Runtime aktif türün sonuçlarını gösterir; henüz çalışmamış reçete PASS görünmez.
- Yetkisiz kullanıcı recipe/system düzenlemesi yapamaz.

### Eşlik eden HTML prototipinin gerçek kapsamı

HTML modern koyu temalı tek dosyalık görsel/etkileşimli prototiptir. Mevcut Dashboard, Project, Calibration, Communication, Diagnostics ve Settings görselleri korunur. Recipe Manager, üç kartlı New Recipe, tek Recipe Editor ve tür bazlı Runtime yeni GUI kararına göre düzenlenmiştir.

Çalışan demo etkileşimleri: sayfa geçişi, reçete arama/tür filtresi, mevcut reçeteyi açma, yeni reçete oluşturma, kopyalama, örnek tool parametresi düzenleme, browser draft kaydı, JSON import/export, sekme değişimi ve Run Demo. Kalıcı draft saklama browser localStorage desteklediği ölçüde çalışır; JSON export bağımsız aktarım sağlar.

Demo kendi önceden tanımlı örnek sonuçlarını kullanır; parametrelerden gerçek vision sonucu hesaplamaz. Image Viewer/Flow/Properties düzenleri uygulama türüne göre örneklenir. Tool/node seçimi görsel olarak vurgulanır; tam dinamik tool editor, gerçek drag & drop, ROI çizimi, node bağlantısı, panel resize, vision engine, donanım bağlantısı, gerçek kalibrasyon ve kullanıcı yetkilendirme sonraki uygulama aşamalarıdır.
