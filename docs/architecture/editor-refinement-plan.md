# Recipe Editor iyileştirme planı — EUI

18 Ekim 2026 tarihli plan; planlama kaydı EUI-PLAN-01'dir.
**EUI-01 uygulandı ve bağımsız kabul edildi** ([kapanış](../archive/2026-10-18-eui-01.md)).
**EUI-02 tamamlandı** ([kapanış](../archive/2026-10-18-eui-02.md)).
**EUI-03 tamamlandı** ([kapanış](../archive/2026-10-18-eui-03.md)).
**EUI-04 tamamlandı** ([kapanış](../archive/2026-10-18-eui-04.md)).
**EUI-05 tamamlandı** ([kapanış](../archive/2026-10-18-eui-05.md)).
EUI-06–08 açık. Sıradaki EUI-06.
Canlı devam noktası [progress](../progress.md)'tir.
Kullanıcının beş talebi, ICM fiziksel kabulünden önce önceliklidir.
[Devir mesajı](editor-refinement-handoff.md), [güncel durum](../progress.md).

## Başlangıç ve kapsam

Kullanıcı önceki manuel deneme listesi için “testler ok” bildirdi. Bu kullanıcı
beyanıdır; adım bazlı kayıt, görüntü, ölçüm veya yeni otomatik koşum yoktur.
ICM-09a yazılım kabulü mevcut; ICM-09b/G0/G1 ve fiziksel ölçüm engelleri açık kalır.
İlk plan teslimi yalnız belgeydi. Aşağıdaki kartlar hedef kapsamdır; gerçekleşen
teslim ve doğrulama için güncel progress ve ID kapanış kayıtları esas alınır.

18 Ekim isteği, 7 Ekim GUI'yi koruma kararını Editor görünümü, Toolbox ve ortak sol
Workspace paneli için değiştirir. Diğer ekranların işlevleri ve genel ürün kimliği
korunur. Üç applicationType tek Editor/Runtime kullanır. Yeni node canvas,
otomatik Edge dönüşümü, yeni algoritma ailesi veya kamera capability açma kapsam dışıdır.

Planlama sırasındaki kaynak incelemesi (uygulama öncesi):

- `src/app/ImageViewer.cpp`: Blob `searchRoi` yolu `drawRegion/onRegion` ile tek
  çizimdir; `setRegions` statik dikdörtgenler çizer. `ImageViewerRoi.cpp` içindeki
  tutamaçlı ToolRoi yolu ayrıdır. Var olan altyapı, iki yolu karıştırmadan kullanılmalı.
- `src/application/ToolExecutors.cpp`: Blob threshold + connected components +
  alan filtresi uygular; dönen maske, count ve en büyük bölge aynı kabul kümesini kullanır.
- `src/domain/ToolRegistry.cpp`: Edge ve Edge Point farklı türlerdir. Edge strength
  `gray8`, Edge Point strength `gray8/px`; parametre/port ve yürütücüler eşdeğer değildir.
- `ImageViewer.cpp`: açıklamalar `ItemIgnoresTransformations` ile sabit ekran
  boyutundadır. Uzaklaşınca resme göre büyümüş görünmeleri bu davranışla uyumludur;
  burada önerilen ölçekleme uygulanmadan sorun giderilmiş sayılmaz.
- `src/app/MainWindow.cpp`: sol panel 215 px sabit, içerik kenarları 20/16 px;
  `MainWindowEditor.cpp` birden fazla komut/bilgi satırı ve düz Toolbox listesi kullanır.
- `src/app/MetrologyEditor.cpp`: start/sweep, koordinatları uygula yoluyla geometriye
  dönüşür. `RadialScanner.cpp` ve `LineFitter.cpp` gerçek tarama konumlarını üretir;
  UI bunlara benzer ama farklı bir ikinci örnekleme algoritması yazmamalı.

## Ortak davranış ve veri kuralları

1. ROI ve Blob filtre değişimi reçete parametresidir; revision/dirty, cache ve
   validation invalidation mevcut application servisinden yürür. Sürükleme sırasında
   geçici önizleme, bırakmada tek geçerli değişiklik; Escape/başarısızlıkta geri dönüş.
   Mouse hareketi başına Save/Run veya yeni revision üretilmez.
2. Pin, Toolbox grup durumu, açıklama görünürlüğü ve panel boyutları yerel görünüm
   tercihidir: `config/local.ui.ini`. Reçete hash'i, aktif immutable Runtime ve
   hesap sonucu değişmez. Testler ayrı `build/` INI kullanır.
3. Kaynak seçimi, raw/processed raster, frame/coordinate kimliği, pose/world dönüşümü
   ve busy kilitleri korunur. Referansa bağlı ROI sürüklenip sessizce bağdan koparılamaz;
   mevcut salt okunur/yeniden öğretim yolu açıklanır. Yeni gesture bunu aşmaz.
4. Eski reçete aç/no-op Save byte/hash davranışı korunur. Yeni Blob alanı eksikken
   eski davranış sürer. Sürüm numarası tahmin edilmez: güncel codec/tool/engine kimliği
   koddan doğrulanır; yeni alanın eski okuyucuda sessizce yok sayılmasını önleyen
   sürüm/capability kuralı uygulanır. Gerçek değişim ve yeni parametre kimliğe katılır.
5. Nominal ROI/tarama kılavuzu sonuç değildir. Parametre değişince eski fit/ölçüm
   güncel sonuç gibi kalmaz. UI algoritma veya donanım SDK'sını doğrudan çağırmaz.

## Görev sırası

Tek ajan, sıralı çalışma; her ID bağımsız kabul ve tek commit. Bir kart tamamlanmadan
sonrakine geçilmez. Kullanıcının sıralı devam talimatıyla EUI-04 ve EUI-05 tamamlandı. Sıradaki EUI-06.

| ID | Paket | Önkoşul | Talep |
| --- | --- | --- | --- |
| EUI-01 | Blob arama ROI taşıma/boyutlandırma | Plan | 1 |
| EUI-02 | ROI sınırına temas eden Blob'ları eleme | 01 | 1 |
| EUI-03 | Görüntü açıklamaları ve zoom | 02 | 4 |
| EUI-04 | Tarama okları ve canlı halka/yay geometrisi | 03 | 5 |
| EUI-05 | Toolbox grupları ve Edge uyumluluğu | 04 | 2, 3 |
| EUI-06 | Workspace pin/otomatik gizleme | 05 | 2 |
| EUI-07 | Kompakt, modern Editor düzeni | 06 | 2 |
| EUI-08 | Birleşik yazılım/görsel kabul ve kullanıcı devri | 07 | Tümü |

### EUI-01 — Blob search ROI düzenleme

- Seçili Blob'un mevcut arama dikdörtgeni gövdesinden sürüklenir; dört köşe ve
  dört kenar tutamacıyla büyür/küçülür. İmleç taşıma/yatay/dikey/çapraz işlemi anlatır.
- Yeni çizim bir kere tamamlandıktan sonra düzenleme açık kalır. Eski aracı seçmek
  yeni ROI çizimini başlatmaz. ROI dışından sürükleme pan; orta tuşla pan ROI içinde
  de erişilebilir olsun. Viewer tıklaması seçili aracı değiştirmez.
- Görüntü sınırını aşan taşıma sınırda durur; resize en az 2×2 px olur. Mevcut
  piksel dikdörtgeninin kapsayıcı uç/yuvarlama semantiği korunur; bir piksellik kayma yok.
- Escape sürüklemeyi iptal eder; Ctrl+Z son tamamlanmış ROI düzenlemesini geri alır.
  Başka araç/kaynak/reçete, Stop ve busy geçişi yarım düzenlemeyi commit etmez.
- Region girişi veya tracking tarafından belirlenen alan kullanıcıya açıklanır;
  düzenlenemeyen alan için yanıltıcı tutamaç gösterilmez. İlk kapsam Blob'dur;
  Template model ROI ve diğer araçlar aynı anda yeniden tasarlanmaz.
- Dokunulacak yerler: ImageViewer, ImageViewerRoi, MainWindowEditor/Matching/Tracking,
  ilgili application tool update yolu. Kalıcı `searchRoi` biçimi gerekmedikçe değişmez.
- Kabul: taşıma, sekiz tutamaç, zoom/pan altında koordinat, sınır/2×2, Escape/Undo,
  Save/reopen, çok kaynakta doğru raster, reference kilidi ve stale sonuç temizliği.
  İlgili testler: `roi_viewer_interaction`, `ui_editor_usability`, `editor_services`,
  `editor_target_only`, `project_persistence`; eksik etkileşim senaryoları eklenir.

### EUI-02 — “ROI sınırına temas edenleri reddet”

- Blob Properties'e bu etiketle checkbox; tooltip: “Arama alanının sınırına değen
  bağlı bölgeler sayım ve ölçümden çıkarılır.” Birbirine değen nesneleri ayırma isteği
  değildir. Önerilen alan `rejectBorderTouching`, varsayılan **false**.
- Önce etkili arama alanında segmentasyon/bileşenler, sonra sınır teması ve alan
  filtreleri. Bir piksel bile temas ederse bileşenin tamamı elenir; yalnız kenar
  piksellerini silip kalan kısmı nesne saymak yanlıştır.
- Etkili alan: search ROI + varsa upstream region, mevcut reference/pose çözümü ve
  görüntü sınırı kurallarıyla belirlenen gerçek maske. Dikdörtgende dört kenar/köşe;
  dönük maskede bounding box yerine maske sınırı kullanılır. Sınır pikseli, 8-komşusundan
  en az biri etkili alan dışında olan geçerli pikseldir; komponent bağlantısı 4/8 ayarı
  ayrı kalır. ROI yoksa tüm görüntünün dış kenarı sınırdır. Var olan geçersiz ROI ret
  davranışı yeni bir sessiz clipping davranışına dönüştürülmez.
- `candidateCount` filtre öncesi aday sayısı kalır. `count`, `found`, en büyük
  `area/point/region` ve kırmızı maske yalnız kalan bileşenlerden oluşur. Hepsi
  elenirse found=false, count/area=0; point/region geçersizdir. Sahte PASS yoktur.
- Domain defaults/validation, codec migration, ToolExecutors ve Properties birlikte
  değişir; Editor/Run Tool/Runtime aynı uygulamayı kullanır. Kimlik/engine sürüm
  bağımlılığı ve eski okuyucu ret kuralı kart kapanışında belgelenir.
- Kabul: her kenar/köşe, içeride nesne, tam alanı dolduran nesne, delikli nesne,
  bright/dark, connectivity 4/8, alan filtresiyle birleşim, dönük maskenin eğik kenarı,
  tümü elenme, off eski sonuçlarla eşitlik, eski dosya no-op ve on Save/reopen.
  `blob_contract`, tracking/pose ilgili testleri, `tool_migration_contract`,
  `tool_execution`, `multi_source_execution`, `multi_source_target_only`, Runtime testi.

### EUI-03 — Görüntü açıklamaları

- Run Flow/Run Tool yakınında **“Açıklamaları göster”** checkbox'ı; ilk kullanımda
  kapalı, yerel tercih kalıcı. Geçiş yeni execution/acquisition başlatmaz.
- Sonuç açıklamaları (support/angle/RMS, değer etiketleri, P1/P2, outlier ve maske
  açıklaması) bu tercihe uyar. Geometri, ölçüm çizgileri, noktalar, maskeler, ROI
  tutamaçları ve tarama okları görünür kalır. Hata/geçersizlik uyarıları ve kaynak/
  koordinat bilgisi kapatılmaz; Results tablosundaki sayısal bilgi her zaman erişilir.
- Yazı ve arka plan birlikte ölçeklenir. Başlangıç tasarım kuralı: 1:1'de 11 pt,
  görsel ölçek `min(zoom, 1.5)`; uzaklaşırken yazı da küçülür, yakınlaşırken en fazla
  16.5 pt görünür. Böylece sabit ekran metni uzak görüntüyü kaplamaz; okunmak için
  zoom kullanılabilir. DPI için Qt mantıksal pikseli kullanılır; dev label kutusu olmaz.
- Bu tercih ilk etapta Recipe Editor viewer'ına aittir; Calibration/Runtime viewer
  davranışı global flag ile değiştirilmez. Overlay yeniden kurulduğunda tercih korunur.
- Kabul: Line Fit, Circle, iki nokta ölçümü ve Blob; %10/%25/%100/%400 zoom,
  tekrar Run, tool/source değişimi, restart; görünürlük değişimi recipe hash/dirty'yi
  etkilemez. `roi_viewer_interaction`, `ui_geometry_smoke`, `ui_circle_smoke`,
  `ui_editor_usability` ve normal/küçük ekran görselleri.

### EUI-04 — Tarama okları ve halka/yay

- Line Fit ve Circle/Arc için scanCount=N olduğunda **N tarama oku** çizilir.
  10 tarama=10 ok; oklar başarılı bulunan noktaların sayısına göre eksilmez.
  Seçili aracın kılavuzu Run öncesi de görünür; henüz tarama sonucu olduğu ima edilmez.
- Circle outward: iç yarıçaptan dış yarıçapa, inward: tersine. Line Fit:
  döndürülmüş kutunun gerçek tarama başlangıç→bitiş yönü; inward/outward Line Fit'e
  eklenmez. Edge Point/Pair mevcut tek tarama oku aynı görsel dili kullanır.
- Geometri application katmanından UI'ye iletilir; varsa saf scan-layout üretimi
  ortaklaştırılır. Detector çağrısı gerekmeden kılavuz üretilebilmeli; UI ayrı örnekleme
  matematiği yazmamalı. World/mm taramada raw görüntüye mevcut dönüşüm uygulanır;
  haritalanmayan alan için uydurma ok veya sonuç yoktur.
- Tam dairede N eşit aralık ve yinelenen 0/360 oku yok; yayda iki uç dahil N örnek.
  Signed sweep, görüntünün aşağı artan Y ekseni ve geçerli açı aralığı mevcut domain
  semantiğiyle uyumludur. Mevcut minimum scanCount/minPoints sınırları gevşetilmez.
- Start ve Sweep, Circle Basic içinde erişilebilir olur. Geçerli değer değişince
  bekletmeden nominal yay + oklar güncellenir; 360° halka, kısmi sweep halka sektörü
  olarak görünür. Spin sırasında önizleme, editingFinished'te atomik commit; invalid
  geçici sayı eski geçerli geometriyi bozmaz. “Koordinatları uygula” da aynı doğrulama
  yolunu kullanır. Fit sonucunun yayı ancak yeni Run ile güncellenir.
- Oklar ince, düşük kontrastlı; uçlar görünür ekran boyutuyla sınırlı. ROI, kılavuz
  ve gerçek fit ayrı çizim stilleridir. N=128'de keyfi seyrekleştirme yapılmaz.
- Kabul: N=10/üst sınır, inward/outward uç eşleşmesi, döndürülmüş Line Fit,
  start/sweep=360/180/90 ve desteklenen negatif sweep, uçların tekrarsızlığı,
  px/world dönüşümü, Run öncesi/sonrası, Save/reopen ve stale temizliği.
  `radial_scan_contract`, `line_fit_contract`, `calibrated_circle_contract`,
  `metrology_runtime_contract`, `ui_circle_smoke`, `roi_viewer_interaction`.

### EUI-05 — Gruplu Toolbox ve eski Edge

Mini başlıklar açılır/kapanır; başlık araç değildir, Add Tool başlıkta pasiftir.
Arama grup içeriğine uygulanır, eşleşen gruplar geçici açılır; arama temizlenince
önceki açık/kapalı hali geri gelir. Klavye, çift tık, Add Tool ve mevcut yeni araç
çizim hazırlığı korunur. Liste sırasına gömülü kind tahmini kullanılmaz.

| Grup | Mevcut araçlar |
| --- | --- |
| Acquisition / Görüntü edinimi | Image File, Webcam, Industrial Camera |
| Image Processing / Ön işleme | Preprocess |
| Detection / Tespit | Blob, Code Read |
| Matching / Eşleştirme | Template Match, Pattern Pose, Shape Pose |
| Measurement / Ölçüm | Edge Point, Edge Pair / Caliper, Line Fit, Circle / Arc, Measurement, Geometry Measurement |
| Calculation / Hesaplama | Calculate |

Edge yeni ekleme kataloğundan çıkar; registry/codec/executor ve eski Flow/Properties
kalır. Eski reçetede “Edge (eski)” sunum etiketi olabilir; kaydedilmiş araç adı ve
kind değiştirilmez. Otomatik Edge→Edge Point migration yapılmaz. Diğer yeni araç
ekleme girişleri denetlenir; eski türü deserialize etmek veya mevcut aracı düzenlemek
yasaklanmaz. Future plugin türleri kaybolmamalı; metadata yoksa Other/Diğer grubuna.
Industrial Camera listede görünmesi native edinimin açıldığı anlamına gelmez.

Kabul: her eklenebilir tür bir grupta tam bir kez, Edge yeni listede yok/eski dosyada
çalışır, strength birimleri korunur, no-op hash aynı. Registry tür sayısıyla görünür
toolbox öğe sayısı ayrılır; 17 sabit satır bekleyen smoke testleri semantik kontrolle
güncellenir. `backend_capabilities`, `tool_creation_policy`, `tool_migration_contract`,
`ui_shell_smoke`, `ui_editor_usability`, `measurement_dag`.

### EUI-06 — Workspace sabitleme / otomatik gizleme

Kapsam, soldaki WORKSPACE/SYSTEM navigasyonunu taşıyan ortak paneldir; Toolbox ayrı
kalır. Küçük pin simgesi ve “Sabitle / Otomatik gizle” tooltip/erişilebilir adı olur.
İlk kullanım sabit; tercih yereldir. Otomatik gizlemede 24–32 mantıksal px sol şerit
kalır; hover veya tıklama açar. Açılan panel içerik üzerinde yüzer, viewer'ı her hover'da
yeniden boyutlandırmaz. Sabitlemek yeniden layout'a dahil eder.

Başlangıç hover gecikmesi yaklaşık 150 ms, çıkışta kapanma 350 ms; panelde klavye
odağı, açık menü, modal işlem veya fare sürüklemesi varken kapanmaz. Escape uygun
odak dönüşüyle kapatabilir; Tab/klavye ile şeritten erişim vardır. Panel açılıp
kapanırken aktif sayfa/araç/zoom değişmez. View menüsünden erişim ve panel düzenini
sıfırlamada sabit moda dönüş vardır. Diğer sayfalar bu navigasyonu kullanmayı sürdürür.

Kabul: hover sınırında titreme yok, pin/unpin, odak/menü/dialog, restart, Reset,
850×600 ve 1400×900, ekranlar arası geçiş; `ui_shell_smoke`, `ui_editor_usability`,
ilgili Runtime/Calibration UI regresyonları. Yerel INI korunarak test INI kullanılır.

### EUI-07 — Daha geniş ve sakin Editor

- Recipe Editor büyük başlığı yerine kompakt başlık/komut satırı; reçete adı,
  dirty/draft ve önemli durum görünür. Save, Run Flow, Run Tool, Stop, açıklama
  checkbox'ı erişilebilir; ikincil reçete ayrıntıları overflow/katlanır bölümdedir.
- Sürekli uzun açıklamalar yardım/tooltip veya açılır bilgiye taşınır. Kritik hata,
  çalışıyor/iptal ve kalibrasyon eksikliği gizlenmez. Kaynak seçici ve görüntü toolbar'ı
  gereksiz satır tüketmez; küçük ekranda kontrollü sarma/overflow olur.
- Grafit/lacivert yüzey, ince ayırıcı, sınırlı turkuaz vurgu; daha az kutu/kart ve
  boşluk, tutarlı yazı ve kontrol boyları. Kapsamlı global stylesheet değişikliğiyle
  diğer ekranların renk/yerleşimi bozulmaz. İkonların tooltip ve klavye erişimi vardır.
- Viewer yatay/dikey stretch önceliği alır; Toolbox/Properties ve alt Flow/Results
  kullanıcı tarafından boyutlandırılabilir. Reset yeni makul varsayılanları yükler;
  mevcut kayıtlı boyutlar taşma yapmayacak şekilde sınırlandırılır.
- Aynı fixture ve pencere boyutunda önce/sonra viewer viewport ölçüsü ve görsel alınır.
  Sabit Workspace modunda viewport alanı küçülmemeli, yüksekliği artmalı; auto-hide
  modunda buna ek yatay alan kazanımı gösterilmeli. Sadece farklı zoom ile daha geniş
  görünmüş görüntü kanıt sayılmaz. 850×600'de temel komutlar ve Properties erişilebilir.
- İlk iş mevcut ekranın görsel baseline'ını almak; sonra Qt uygulamasını hazırlayıp
  normal/küçük ekran incelemesi. Görsel belirsizlik rutin tasarım seçimiyle çözülür;
  kullanıcıya onay için boş taslak bırakılmaz.
- Kabul: üç applicationType, tek/çok kaynak, uzun reçete adı, hata/busy/boş viewer,
  yeni ve eski INI; `ui_editor_usability`, `ui_shell_smoke`, `ui_multi_source_editor`,
  `ui_multi_source_runtime` ve ilgili araç smoke testleri.

### EUI-08 — Birleşik kabul ve sonraki sıra

Son kaynak Release UI ON/ZXing ON ve core OFF/ZXing OFF tam CTest. Test adetleri
değişebilir; eski 105/89 sayısı yeni sonucun yerine yazılmaz. Önceki socket aralıklı
başarısızlığı çıkarsa ilk hata/log ve tekrar ayrılır; kör tekrar başarı kanıtı değildir.

Görsel kontrol: 1400×900, 850×600; Blob ROI + sınır filtresi, açıklama açık/kapalı
zoom serisi, 10 ok + iç/dış yön, Line Fit, üç farklı yay, gruplu Toolbox, Workspace
iki modu, kompakt Editor. Otomatik mouse/klavye etkileşimi gerçek giriş olaylarıyla
sınanmalı; sadece widget setter testi gesture kabulü sayılmaz. Desteklenen yüksek DPI
fixture'ı eklenebilir; native çoklu monitör kabulü kanıtsız kapanmaz.

Kullanıcı devri: doğru EXE/run-local komutu ve ExecutionPolicy engelinde yalnız alt
süreç için `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
"C:\Projects\Tvision\scripts\run-local.ps1"`; kullanıcıya beş talebin kısa tekrar test
listesi. EUI-08 yazılım kabulü kullanıcının yeni testini yapılmış saymaz.

Sonraki sıra: kullanıcı geri bildirimi/düzeltme → çok kaynak/kalibrasyon manuel
kontrollerinin eksiklerini netleştirme → seçili backend G0/G1 → ICM-09b fiziksel
kabul → ICM-10 bağımsız ölçüm/lens. OECS gecikmiş tetik engeli veya Basler/Daheng
model/SDK kanıtı çözülmeden fiziksel testlerin tamamlandığı iddia edilmez.

## Kapanış standardı

Her kart: kapsam, gerçek değişen dosyalar, komut/çıkış kodu ve kanıt yolları,
bilinen sınırlamalar, sıradaki ID; `docs/archive/YYYY-MM-DD-eui-XX.md`,
`Uygulayıcı: Codex` (veya gerçek uygulayıcı Claude Code). Progress güncel özetini
ve arşiv dizinini güncelle; tek görev ID'li commit, push yok. Kanıtlar
`build/eui/XX/` altında; mevcut test/INI/kanıt üzerine yazma.
`python scripts/check-context-docs.py` ve `git diff --check` çalıştır.
README yalnız kullanım değiştiğinde, gereksinim/mimari ilgili sözleşme değiştiğinde
güncellenir. Plan ve gerçek teslim durumu birbirine karıştırılmaz.
