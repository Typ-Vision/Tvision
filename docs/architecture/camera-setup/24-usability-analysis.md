# CCUX-24A — on kullanıcı talebinin analiz ve plan girdileri

26 Ekim 2026. Uygulayıcı: Codex.
Durum: kaynak incelemesi tamam; uygulama yapılmadı. Başlangıç commit'i 4cfb93d,
dal CameraIntegration; başlangıç çalışma ağacı temizdi.

## Kanıt kapsamı ve sınırlar

AGENTS, progress, README, CCUX indeksi/contract, ilgili UI gereksinimleri,
15e–j planının fiziksel kabul bölümü incelendi. codebase-memory Tier 2 ile
C:/Projects/Tvision seçildi; list_projects/index_status, sembol arama, çağrı
izleme ve snippet okuma kullanıldı. İlk indeks generation 2026-10-25T20:53:17Z; belge yazımı sonrası ek varsayılan incelemesinde 2026-10-25T21:42:16Z.
27 kanıt dosyası check_index_coverage ile kontrol edildi; metadata_match.
ImageViewer.hpp 26/32/46, CameraManager.cpp 28 ve CircleToolService.cpp 37
kısmi indeks satırları doğrudan okundu. Diğer dosyalarda kayıtlı kapsam sorunu yok;
bu sonuç eksiksiz çağrı grafiği garantisi değildir. Qt callback ve genel isimler
üzerindeki graf ilişkileri doğrudan kaynakla doğrulandı.

Bu çalışmada uygulama açılmadı, gerçek kamera/reçete/model değiştirilmedi,
Mouse davranışı canlı yeniden üretilmedi. Kesin kaynak bulguları ile neden
adayları aşağıda ayrıdır. Kullanıcı Circle hata bildirimini geri çekti; güncel
isteği yalnız yeni Circle araçlarında calibrated varsayılanıdır.
`ctest --test-dir build/reference-ui -N` exit 0: 135 kayıtlı test.
Bu bir test koşumu veya yeni başarı kanıtı değildir.

## 1. Sağdaki Flow kalıcı olsun

**Kesin neden:** MainWindowEditor.cpp:136–156 içinde flowTools_ ve DAG özeti
aynı Flow sekmesinde. MainWindowExecution.cpp:219 ve 260 çalıştırma başlangıcında
editorTabs'ı resultsScroll'a geçiriyor. Dolayısıyla yalnız otomatik sekme geçişini
kaldırmak, kullanıcı Results'ı kendisi açınca Flow'un kaybolmasını çözmez.

**Öneri:** Alt alanı yatay splitter yap: solda sonuç/kenar profili/DAG sekmeleri,
sağda her zaman görünür tek Flow listesi. Üstteki viewer/Properties alanını
daraltmadan mevcut sağ alt listenin yerini koru. Sol Flow sekmesini “Bağlantılar”
olarak yeniden adlandır; DAG özeti ve Validate DAG burada kalsın. Run Flow/Run Tool
sol Results'ı açabilir; sağdaki araç listesi, seçili araç ve scroll konumu değişmez.
Liste kopyalama yok: tek selection modeli, mevcut sağ tık sıralama korunur.

MainWindow.cpp:187–188,230 splitter durumunu local INI'ye kaydediyor.
Yeni alt splitter için ayrı/sürümlü ayar gerekir; eski restore kalıcı Flow'u
sıfır genişliğe düşürmemeli. Properties, ROI ve klavye seçimi aynı tool ID'yi izler.

Kabul: Run Flow, Run Tool, hata/Stop, Results/Profil seçimi, reçete yeniden açma
ve eski INI ile Flow görünür; 1400×900 ve 850×600'de kullanılabilir.

## 2. Bağımsız kontrol ve fiziksel kabul ifadeleri

Üç farklı durum var:

| Veri | Anlamı | Kaynak |
| --- | --- | --- |
| Model fit/RMS | Kalibrasyon plakasına matematiksel uyum | Plaka modeli |
| controlAccepted | Kullanıcının bağımsız mesafesinin girilen mm sınırını karşılaması | ActiveCalibrationService.cpp:48–78 |
| physicalAcceptance=false | Deneysel edinim kanıtının fiziksel/üretim kabulüne yükseltilmemesi | ExperimentalCalibrationEvidence.cpp:43–48 |

Kontrol fit noktasını tekrar kullanamaz; bilinen mm, farklı noktalar ve tolerans
üzerinden residual hesaplanır. CalibrationWizard.cpp:154 ve Overview.cpp:108
controlAccepted okur. Buna karşılık MainWindowExecution.cpp:56 experimental
durumunda sabit “bağımsız fiziksel kabul yok” döndürür. ExperimentalRuntime.cpp:226
de controlAccepted okumadan sabit “yapılmadı” yazar. Kullanıcının gördüğü iki
ifade bu yüzden aynı anda çıkabilir; başarılı kontrolün kaybolduğunu tek başına
göstermez. Başka model/snapshot seçilmesi ihtimali gerçek kayıtla ayrıca elenmeli.

**Öneri:** Kalibrasyon sayfasında modelin kendi kontrol durumunu ortak formatter
ile göster: yapılmadı / ölçümler mevcut fakat sınır yok / sınır içinde / sınır dışı.
Editor ve Runtime'da rutin sabit fiziksel kabul cümlesini kaldır; gerçek kalibrasyon
uyumsuzluğu/hatası kalır. Eski schema physicalAcceptance bayrağını true yapma;
kontrol başarısı productionAdmission veya PLC izni üretmez. Saha kabulü teknik
kanıt ve ilgili kabul kaydında ayrı kalır. Tarihsel Logs/CSV anlamını değiştirme;
formatter kullanılıyorsa kaydedilmiş modelden türet, güncel modelden değil.

Kabul: kontrol yok/geçti/reddedildi/sınır yok ve eski kayıt matrisi; aynı model
Editor/Runtime'da çelişkili “kontrol yapılmadı” üretmez. Yeni model etkinleşince
çalışan immutable Runtime sessizce değişmez.

## 3–4. Sonuç kimlikleri ve Measurement kalibrasyon bölümü

MainWindowExecution.cpp:161,168 Cycle/Frame/Kaynak satırları;
MainWindowRuntime.cpp:356,366,420 çevrim/frame ve kaynak özetleri üretir.
ExperimentalRuntime::sourceSummary de kaynak/model/content kimlikleri ekler.
Sonuç sunumundan Kaynak, Frame, Cycle teknik kimlikleri çıkarılmalı. Çok kameralı
Runtime kaynak seçicisi korunur: hangi resmin görüldüğünü seçmek için gereklidir.
Metadata, kalıcı kayıt, korelasyon, Logs/CSV ve hata kanıtı silinmez.
Sonuç alanında değer/birim, yürütme durumu, süre ve gerçek hata kalır.

Measurement bölümünde MainWindowEditor.cpp:446–447 Kalibrasyon özeti ve
Calibration aç düğmesi var; ikisi kaldırılır. Tolerans alanları, input bağlantıları
ve application katmanındaki zorunlu model denetimi korunur. geometry_measurement
ayrı tool türüdür; bu panelde aynı başlık bulunmadığı için gereksiz yeniden yazım yok.

Kabul: aynı gerçek sonuçlar ve çıktı tercihleri korunur; seçili tool görünür
çıktısı değişmeden gereksiz kimlik satırları kalkar. Model yoksa sahte mm çıkmaz.

## 5. Circle varsayılanı — güncel kullanıcı düzeltmesi

Kullanıcı hata olmadığını açıkladı; hata araştırması ve yeni birim geçiş davranışı
kapsamdan çıkarıldı. İstek: yeni Circle için pixel yerine calibrated varsayılanı.
Kayıtlı pixel araçlar, kopyalanan araçlar ve kullanıcı seçimi değiştirilmeyecek.

CircleConfig.cpp:8–16 temel varsayılanı pixel yapıyor. ProjectService::addTool
registry varsayılanını kullanıyor. MainWindowEditor.cpp:727 yeni araçlarda
edge_pair/line_fit/geometry_measurement için calibrated uygular; içerideki
circle_fit başlangıç bloğu dış listedeki eksiklik yüzünden ulaşılamaz durumdadır.

Öneri: yeni Circle oluşturma politikasını calibrated yap; mevcut hazır mm
başlangıçları (scanWidth/inlierDistance/maxResidual/unitsPerSample=0.1,
maxRms=0.035) tutarlı uygula. Bu yalnız combo etiketini değiştirmek olmamalı.
Global varsayılan değiştirilecekse eski sürüm migration/default tüketicileri
korunmalı; tercih yeni-tool oluşturma politikasıdır. Pixel seçeneği kalır.
Ürün toleransları fiziksel kabul sayılmaz; kullanıcı toleransını değiştirme.

MainWindowEditor.cpp:391–392 ortak metrologi Kalibrasyon başlığı/düğmesi
Circle, Line ve Edge Pair'de aynı yerden ekleniyor; tutarlı sadeleştirme için
bu ortak blok kaldırılmalı. Circle matematiği, birim değişiminde yeniden öğretim
ve model geçerlilik kontrolleri değişmez.

Kabul: yeni Circle calibrated ve mm başlangıçlarıyla açılır; mevcut pixel
reçete aç/kaydet/kopyala sırasında pixel kalır; pixel elle seçilebilir;
kalibrasyon yoksa model uydurulmaz; Save/reopen ve çizim/Run Tool doğrulanır.

## 6. mm/px girişlerinde üç basamak

Envanter: MetrologyEditor (parametre ve ROI), EdgePointEditor, RoiEditor,
PoseMatchEditor (px konumları), MainWindowEditor (scan/geometry/measurement),
MainWindowTracking (world ROI/örnek ölçeği), CalibrationWizard (mm alanları).
Kodda dokuz basamak yanında altı basamaklı mm girişleri de var.

Hedef: uzunluk/koordinat girişleri 0.001 adım ve üç ondalık; adet/tam sayı
alanları tam sayı kalır. Derece, yüzde, skor, exposure/gain ve birimsiz alanları
toplu regex ile değiştirme. Her alanın birim semantiğini listele.
Görüntüleme hassasiyeti solver/model/JSON hassasiyeti değildir.

QDoubleSpinBox değeri de yuvarlar. MetrologyEditor initialDisplayValue ile
değişmeyen parametreyi koruyor; diğer editörler aynı güvenceye sahip varsayılmamalı.
Ortak edit-state yaklaşımıyla untouched değer özgün double kalmalı; kullanıcı
gerçekten düzenlediğinde üç basamak kaydedilmeli. Pozitif 1e-9 minimumlar üç
basamakta sıfıra yuvarlanabilir: yeni girişlerin minimumu alan bazında 0.001,
mevcut daha küçük değerler dokunulmadığında korunmalı. Çözünürlüğün altındaki
eski pozitif değerin ekranda 0.000 görünmesi tooltip/orijinal değer bilgisiyle
açıklanabilir; modele sıfır yazılmaz. Doğrudan gösterilen mm/px koordinatı ile
mm/örnek dönüşüm katsayısını aynı kayıt kuralıyla körlemesine değiştirme.

Kabul: 1.23456789 yüklendiğinde başka alanı değiştirmek bu değeri değiştirmez;
no-op Save/hash korunur; açık değişiklik 1.235 olur; virgüllü Türkçe giriş,
negatif koordinatlar, küçük pozitif minimumlar ve adet alanları sınanır.

## 7. Kalibrasyon Ayrıntılar envanteri ve öneri

CalibrationOverview.cpp:42–71 düğmeleri gerçek callback'lere bağlıdır;
yalnız Robot kalibrasyonu açıkça devre dışıdır. Hepsini “eski/boş” saymak yanlış.

| Mevcut öğe | İşlev / önerilen işlem |
| --- | --- |
| Kamera / kurulum ekle, Kurulumu düzenle, Yeni ölçüm kurulumu | Yönetim işlevi; Kalibrasyon'dan kaldır, Camera Manager'da tekleştir |
| Cihazları keşfet | Camera Manager'a yönlendiriyor; tekrar düğmeyi kaldır |
| Kurulum ve profil seçicileri | Çoklu/legacy seçim için gerekli; tek varsayılanda gizle, belirsizlikte açık seçim koru |
| Operasyonel seç | Gerçek çalışma kurulumu seçer; çok kurulum yönetimine taşı |
| Devre dışı bırak | Gerçek etkin modeli kapatır; Kalibrasyon'da koru, anlaşılır ad kullan |
| Bilinen ölçü / Kalibrasyon plakası | Kalibre Et ile aynı wizard; tekrarlanan düğmeleri kaldır |
| Dosyadan deneme başlat | Kaynak seçimi mevcut wizard'da sunulur; ayrı giriş kalkar |
| Etkinleştir | Eski aday + gerçek probe akışı; ana komut yerine yalnız geçmişte seçili kaydı tekrar kullanma ihtiyacına indir |
| Robot kalibrasyonu | Uygulanmamış; kaldır |
| Eski kayıtları kurulumla eşle | Gerçek migration; normal sayfadan kaldır, ihtiyaç halinde eski proje açılışında koşullu uyumluluk akışı |
| Denemeyi bu kurulumla ilişkilendir | Deneme/model ilişkilendirme; normal sayfadan kaldır, gerekirse dosya sihirbazındaki açık ilişkilendirmeye taşı |
| Kayıt/sürüm/seçim/kontrol tablosu | Salt okunur geçmiş ve izlenebilirlik; koru |
| Kamerayı arşivle / Kurulumu arşivle | Yönetim eylemi; Camera Manager'a taşı |
| Optik / kamera konumu değişti | Gerçek invalidation; Camera Manager görünür kurulum alanına taşı, işlevi koru |
| Dışa aktar / boş projeye al / bağımlılıklarıyla farklı kaydet | Gerçek proje transferi; proje yönetimine taşı |

Yeni ana akış: kamera → Kalibre Et → çerçeveli 7×7 plaka → hesapla → isteğe bağlı
bağımsız kontrol → Kaydet ve kullan. Bilinen ölçü, bağımsız kontrol adımının
kullandığı edge/mesafe altyapısından ayrılmalıdır; eski yöntemi UI'dan kaldırmak
bu kontrol servislerini silme gerekçesi değildir.

Önerilen ürün kararı: yeni kalibrasyon oluşturmada çerçeveli 7×7 tek varsayılan
akış olsun; eski manuel ölçek, generic circle grid/chessboard ve bilinen ölçü
oluşturma girişleri normal üründen çıkarılsın. Dosya seçimi aynı sihirbaz içinde
korunsun. Eski kayıtları okuma/hesaplama, no-op encode ve migration gerekleri
korunsun. Sonraki uygulama kartında eski UI/solver ayrımı, CMake ve test çağrıları
tam taranarak yalnız gerçekten kullanılmayan UI kodu silinsin. Bu analiz bütün
eski solver'ların kullanılmadığına dair tam dead-code denetimi değildir.
Mevcut kaynakta Kalibrasyon ana sayfası tek CalibrationOverview'dur
(MainWindowCalibration.cpp:6); tarihsel belge başlıkları ek canlı ekran sayılmaz.

## 8. Camera Manager düzeni

CameraManager.cpp:67 optik/geometri komutunu advanced içine koyuyor; :138 ve
:257 eksik kurulumda kullanıcıyı bu gizli yere gönderiyor. :89 önizleme minimum
100 px; :326 görüntüyü sabit 640×280'e ölçekliyor. Bütün sayfa dikey scroll'da.

Öneri: kamera listesi ve temel ayarlar solda, esneyen büyük önizleme sağda;
dar pencerede splitter ile dengeli paylaşım ve ayarlara scroll. Optik/Geometri
seçili kameranın temel ayarlarının yanında görünür kart olsun; tamamlandı/eksik
durumu, düzenle ve optik değişikliği bildir eylemleri birlikte yer alsın.
“Taslak” sözcüğünü sadece pending durumunda kullan. Preview ortak ImageViewer
ile Fit/pan/zoom sağlayabilir; yeni karede kullanıcının zoom'unu gereksiz sıfırlama.
Camera Manager test hedefi bugün viewer kütüphanesine bağlı değil; yeni widget
kullanılırsa CMake bağlantısı da güncellenir. En-boy oranı ve gerçek raw frame
saklama korunur. Yeni kareyi ölçekli pixmap'ten yeniden üretme.

Kabul: normal/küçük pencere, boş/kayıtlı/eksik kurulum, tek/canlı görüntü,
Stop/çıkış/device error; preview büyürken settings readback ve lease/drain bozulmaz.

## 9. İmleç merkezli zoom

ImageViewer.cpp:18 AnchorUnderMouse kullanıyor; wheelEvent :153–156 yalnız
zoomBy çağırıyor, olay konumunu zoom merkezine açıkça vermiyor. mouseMoveEvent
:176–180 searchMove/roiMove true ise temel QGraphicsView metoduna ulaşmıyor.
ImageViewerRoi.cpp:178–188 çizim modunda sürükleme yokken bile true dönebiliyor.
Bu, Qt'nin son imleç konumunun eskimesi için somut neden adayıdır; kullanıcının
sol-alt davranışı henüz olay kaydı/görsel deneyle doğrulanmadı.

Öneri: wheel olayındaki viewport noktasını zoom öncesi scene'e eşle;
ölçeklemeden sonra aynı scene noktası aynı viewport noktasında kalacak şekilde
pan telafisi uygula. Zoom düğmeleri viewport merkezini esas alsın. Ortak viewer
düzeltmesi Editor, Kalibrasyon ve Runtime'a yansır; Camera Manager yeni viewer
kullanırsa aynı davranışı alır. Fit, actualSize, resize, scroll sınırları ve boş
kenar alanı politikası ayrı test edilir. setImage'in fitImage çağrısı canlı
karelerde manuel zoom'u sıfırlayabildiği için frame refresh politikası da incelenir.

Kabul: görüntü içi dört köşe/merkez, çizim açık/kapalı, tutamaç, middle pan sonrası,
fitten ilk wheel, scroll bar var/yok, küçük/büyük görüntü, iki pencere boyutu.
Scroll geometrisi izin verdiğinde imleç altı noktanın sapması en fazla 1 viewport
pikseli; tekrarlı zoom-in/out birikimli köşe kayması oluşturmaz. Native DPI
kontrolü ayrıca yapılmadan doğrulandı denmez.

## 10. Runtime'da bütün araç overlay'leri

MainWindowRuntime.cpp:397 sonuçları yalnız runtimeTools_ seçimi için çiziyor;
:401–417 referenceBinding ROI de seçili tool ile sınırlı. ImageViewer::setToolOverlay
önce önceki overlay'i siliyor; döngüye almak tek başına çözüm olmaz. Tek roi_
de bütün tarama bölgelerini aynı anda taşıyamaz.

Öneri: görünür “Tüm araçları göster” checkbox (varsayılan açık), isteğe bağlı
Tarama alanları / Bulunan noktalar / Sonuç şekilleri / Etiketler filtreleri.
Kapalı durumda mevcut seçili araç odağı korunur. Tool seçici ayrıntı/kenar profilini
kontrol etmeye devam eder. Checkbox yerel UI tercihidir; reçete hash'ini değiştirmez.

Tool ID, source ID, frame/cycle ve geometri katmanıyla toplu, salt okunur overlay
sunumu oluştur. Görüntüde yalnız seçili kaynağın güncel çevrimine ait öğeler
birleşir; başka kameranın koordinatları karışmaz. Raw/işlenmiş görüntü seçimi
deterministik olmalı: tüm araçlar modunda ortak raw geometri; seçili araç
modunda mevcut işlenmiş preview davranışı korunabilir. Her tool için bağımsız
renk/etiket ve layer; iki farklı tool'un tek noktası otomatik mesafe çizgisi
olarak birleştirilmemeli. Maske, code köşeleri, fit çizgi/çember, kabul/reddedilen
noktalar ve ROI tarama yönleri ayrı aktarılmalı.

Nominal ROI yerine aktif snapshot ve aynı çevrimin gerçek referans/pose çözümünü
kullan; referenceBinding ve poseTracking yollarını birlikte kapsa. Çözülmeyen
referansta eski/nominal bölge çizilmez. Failed/skipped tool için bulunmuş sonuç
uydurulmaz; geçerli statik tarama alanı gösteriliyorsa sonucu olmayan ayar
geometrisi olarak ayırt edilir. Stop, yeni kaynak, yeni çevrim ve hata eski
katmanları temizler. Viewer düzenleme/recipe callback'leri Runtime'da kapalıdır.

Kabul: tek çekim ve normal çevrim aynı sunumu kullanır; en az Circle + Line +
Edge/Measurement + Blob/Code karışımı, iki kaynak, iki ardışık çevrim, referans
başarısızlığı, checkbox toggle ve çok sayıda nokta. Toggle yeni acquisition veya
execution başlatmaz. Güncel karede sahte PASS/pose görünmez.

## Sıralı uygulama kartları ve doğrulama

Bu belge uygulama başlatmaz. Kartlar sıralı yürütülür, her biri tek commit;
tamamlanmış CCUX/INT görevleri yeniden açılmaz.

| Kart | Kapsam | İlgili mevcut CTest hedefleri |
| --- | --- | --- |
| CCUX-24B | 1,3,4,5'in panel sadeleşmesi: kalıcı Flow ve sonuç sunumu | ui_editor_usability, ui_shell_smoke, ui_runtime_visibility, output_visibility_policy |
| CCUX-24C | 2 ve 5: kontrol metni ve yeni Circle calibrated varsayılanı | calibration_save_use, scoped_active_calibration, calibrated_circle_contract, ui_circle_smoke, experimental_metric_runtime_contract, ui_multi_source_editor, ui_multi_source_runtime |
| CCUX-24D | 6: üç basamak ve untouched hassasiyet | ui_geometry_smoke, ui_roi_tool, ui_pose_tracking_smoke, ui_circle_smoke; yeni no-op precision senaryoları |
| CCUX-24E | 9: ortak zoom düzeltmesi | roi_viewer_interaction, image_refresh_contract; yeni imleç sabitleme senaryoları |
| CCUX-24F | 8: görünür kurulum ve büyük preview | ui_camera_manager, camera_settings, camera_acquisition_sessions_contract |
| CCUX-24G | 7: kalibrasyon sadeleşmesi ve yönetim komutlarını taşıma | ui_calibration_overview, ui_calibration_wizard, ui_scoped_calibration_activation, measurement_setup_migration, framed_calibration_plate_contract, calibration_camera_sessions_contract |
| CCUX-24H | 10: çoklu Runtime overlay | ui_multi_source_runtime, ui_runtime_visibility, ui_runtime_failures, reference_world_contract, world_tracking_contract |
| CCUX-24I | Birleşik kabul | Değişen alanların birleşik regresyonları; Release ON/OFF uygun kapsam |

Her UI kartında Release build, ilgili test ve normal/küçük pencere görsel kontrolü.
Test isimleri bu analizde ctest -N ile doğrulandı. Yeni davranışları mevcut
smoke'lara anlamlı senaryolarla ekle; yalnız eski testin geçmesi yeni talebin
kabulü sayılmaz. Kanıt dizinleri build/ccux/ccux-24<harf>/.
Circle varsayılanı ve gerçek kullanıcı etkileşimiyle zoom kabulü kaydedilmeli.
24H öncesi çoklu overlay veri modeli ve pose/reference kapsamı dar teknik tasarımla
netleştirilmeli. UI görünüm değişiklikleri için güncel gereksinim ve C3 sözleşmesi
uygulama sırasında güncellenir; eski tarihli teslim kayıtları değiştirilmez.

## Açık veri ve kararlar

- Circle hata bildirimi kullanıcı tarafından geri çekildi; hata metni beklenmiyor.
  Yeni calibrated varsayılanı kesin kullanıcı kararıdır.
- Zoom nedeni kuvvetli kaynak bulgusu; gerçek etkileşim yeniden üretimi 24E'nin
  ilk işi. Ekranlar arası ortak widget kullanımı doğrulandı; native DPI kabulü yok.
- Eski yöntemler için yukarıdaki öneri UI oluşturma akışını sadeleştirir,
  kayıt uyumluluğunu korur. Tüm eski matematik servislerini fiziksel silmek ayrı
  bağımlılık/uyumluluk denetimi ister; bu analiz böyle bir silmeyi onaylamaz.
- Saha kabul bayrağını değiştiren yeni bir ürün süreci bu UX paketinin konusu
  değildir. Kullanıcının yaptığı bağımsız kontrolün doğru görünmesi bu pakettedir.
