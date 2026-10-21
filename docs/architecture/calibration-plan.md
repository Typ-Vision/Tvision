# Kamera ve robot kalibrasyonu — revize ürün planı

21 Ekim 2026: kamera merkezli sade Setup ve reçete referans alanlarını kaldırma
yönü kabul edildi; [güncel taslak](camera-calibration-ux-draft.md) robot ucunda
plaka ile Eye-to-Hand önerisini ve ölçüm/robot yöntem ayrımını içerir. Robot
adımları değerlendirme önerisidir; çalışan iç kalibrasyon/robot hesabı değildir.

15 Ekim — **KUI-5 tamamlandı**; [ortak resolver/Editor/Runtime/Logs ve açık legacy aktarımı](system-calibration.md). KUI-4 üç kart/iki sihirbaz akışı bütün metrik tüketicilere bağlandı; pixel tanısı korunur, eski world ilişkiler yeniden öğretim ister. Sıradaki KUI-6 tam Release kabulü. Fiziksel K4/lens/robot/CUDA/kurulum/dağıtım ayrı açık işlerdir. Alttaki kayıtlar tarihseldir.


15 Ekim — KUI-4 ana sayfa/iki sihirbaz/dosya-kurulum/robot şablonu tamamlandı; [UI kabulü](calibration-ui-acceptance.md). KUI-1–KUI-3 tekrar edilmedi. Sıradaki KUI-5 ortak resolver/Editor/immutable Runtime/Logs, ardından KUI-6. Fiziksel K4/robot/lens kabulü açık. Alttaki kayıtlar tarihseldir.


15 Ekim — **KUI-2/KUI-3 tamamlandı**: [servis kabulü](calibration-wizard-services.md). Bilinen ölçüde gerçek Edge/Caliper noktaları ve tek çift; chessboard/simetrik circle grid için ortak session, confirm ve raw H/strict replay mevcut. Circle ayrı tipli yeni kayıt yolundadır; legacy katalog/study ekranı circle kaydını reddeder. Yeni ana sayfa/sihirbazlar **KUI-4**; ortak Editor/Runtime davranışı KUI-5. Aşağıdaki ekran tarifleri tarihsel/mevcut eski UI'dır; lens/robot/fiziksel doğruluk kabulü değildir.


15 Ekim 2026 — [Kalibrasyon sihirbaz planının](calibration-wizard-plan.md) **KUI-1 application sözleşmesi uygulandı**; [kayıt/kurulum/etkinlik ve migration envanteri](active-calibration-contract.md). Yeni serviste tek çiftle hesaplama, optional bağımsız kontrol ve tek etkin seçim ayrıdır. Mevcut ekran ve recipe-scoped üretim resolver'ı henüz yeni politikaya geçmedi. Sıradaki KUI-2 gerçek nokta/session; yöntem kartları/sihirbazlar KUI-4, ortak metrik/Runtime KUI-5. Aşağıdaki eski yöntem/beş bölüm halen çalışan ekranı tarif eder; circle grid/lens/robot uygulanmış sayılmaz.

9 Ekim son kullanıcı kararı: lens bozulması düzeltmesi [reçete tool planının T15 aşamasına](recipe-tools-plan.md#t15--lens-bozulması-düzeltmesi-en-son), bütün tool paketlerinden sonraya ertelendi. T4–T9 ölçümleri mevcut manuel/uniform/raw-H modelleriyle geliştirilir; lens düzeltmesi zorunlu başlangıç işi değildir. Perspective circle/line fit world noktalarda, geçerli kalibrasyon alanında yapılacak; mm² için ayrı alan hesabı gerekir. Yeni metrologi ve lens modeli henüz uygulanmadı. K4 fiziksel doğruluk kabulü açık; sentetik kabul lens doğruluğu kanıtı değildir. Eski “gerektiğinde lens” sırası bu kararla daraltıldı.

9 Ekim güncel durum: **C1–C5 tamamlandı**. Setup → Calibration üç yöntem ve beş bölüm sunar: Manuel ölçek (px/mm), Bilinen uzunluktan ölçek ve Kalibrasyon plakası (perspektif). Manuel girdi boş başlar; px/mm ve salt okunur tersi, üretim sonucundan ayrı deneme, en az iki bağımsız doğrulama, katalog/recipe Save/reopen, gerçek Edge → Measurement → Runtime ve CSV bağlıdır. Güncel Release binary `build/manual-calibration-ui/visionflow.exe`; Measurement tool 2, execution/6. Sonraki P7/6D.2; K4 fiziksel kabul ertelenmiştir.


8 Ekim 2026. Kullanıcının iki kalibrasyon beklentisi üzerine kod/gereksinim incelemesi ve plan güncellemesidir; K1–K3 target/gözlem/test import/köşe/H/mm/tipli kayıt/Measurement/Runtime uygulandı. Intrinsics/robot modelleri ve K4 fiziksel kabul henüz yok. Güncel öncelik 6C-K kamera kalibrasyonunu PLC'den önce tamamlamaktır.

## Ürün amacı ve mevcut durum

VisionFlow; Measurement, Defect Inspection ve Robot Guidance için tek Recipe Editor, DAG ve Runtime sunar. Kalibrasyon ortak bir servistir: ölçüm fiziksel birim için kamera modelini, robot guidance ayrıca robot dönüşümünü kullanır. Piksel tabanlı kusur kontrolü kalibrasyonsuz çalışabilir; mm/mm² sonuçlar uygun model gerektirir.

Mevcut MeasurementService::teach bir elle girilmiş segmentten `mmPerPx = knownMm / pixelLength` hesaplar; distance piksel mesafesini bu katsayıyla çarpar. En az iki ayrı holdout segmentiyle hata kontrolü vardır. CalibrationService sürümlü kanıt, bağımsız current resolver ve binding; RuntimeService immutable snapshot ve sonuç kaydı sağlar. Bu altyapı korunur.

Plaka bulma ve raw çalışma düzlemine nokta dönüşümü uygulandı; eksik olan kamera iç parametreleri/lens düzeltmesi, fiziksel doğruluk kabulü ve robot kalibrasyonudur. Mevcut scale dünya orijini veya robot koordinatı tanımlamaz. CalibrationService ve execution context tipli PlanarCalibration kullanır; legacy scale alias/hash korunur, yeni modeller yalnız UI eklenerek desteklenemez. CMake core/imgproc/imgcodecs/objdetect/calib/geometry ister. Yerel OpenCV 5 SB API objdetect.hpp, homography/perspective API geometry.hpp altındadır; Release link ve sentetik köşe tespiti doğrulandı.

Genel mimari yönü doğru; ancak scalar prototipten PLC'ye geçiş ölçüm ürününün temel beklentisini erken tamamlanmış kabul ediyordu. Öncelik plaka → gerçek mm → bağımsız doğruluk kabulü olmalıdır. Robot guidance için açı/Pose çıktısı, pick reference ve robot dönüşümü de gerekir; mevcut sabit açıdaki Match bunu sağlamaz.

## Kullanıcıya iki kalibrasyon akışı

### 1. Planar Calibration / Homography — tek görüntüden X-Y mm

Kullanıcının son yönlendirmesiyle ilk hedef tek görüntüden planar homography'dir. Çok görüntülü intrinsic kalibrasyonu zorunlu başlangıç değildir; lens bozulmasının ihmal edilemediği kurulumlarda ayrıca gerekir. Sabit kamera ve sabit ölçüm düzlemi için H doğrudan hesaplanabilir; ayrı bir 3D plane pose çözmek şart değildir. Mevcut Calibration sayfası, genel tema/navigasyon/panel düzeni korunur; algoritmalar application/vision servislerinde worker üzerinde çalışır.

1. İlk hedef chessboard: iç köşe satır/sütun sayısı, gerçek kare kenarı mm, target kimliği ve dünya orijin/eksenleri girilir. Kare sayısı ile iç köşe sayısı ayrılır. Circle grid ve ChArUco sonraki genişletmedir. Genel Template Match skoru yerine desene özel köşeler bulunur.
2. **Kare al** veya **Test görüntüsü içe aktar (geçici)** ile görüntü edinilir. Tam çözünürlük kullanılır. Köşe tespiti, numaralandırılmış alt piksel overlay ve dünya orijin/eksen işaretleri gösterilir; kullanıcı eşleşmeyi inceleyebilir.
3. Bilinen mm koordinatları ile görüntü noktalarından image → world yönünde 3×3 H hesaplanır. En az dört dejenere olmayan eşleşme gerekir (dört noktalı durumda herhangi üçü aynı doğru üzerinde olmamalı); pratikte tüm uygun, alana yayılmış köşeler kullanılır. Yanlış sıra, yetersiz dağılım, tekil/kararsız matris ve geçersiz dönüşüm reddedilir.
4. Ölçüm noktaları H ile X-Y mm'ye dönüştürülür; mesafe/çap/genişlik/alan hesabı dünya düzleminde yapılır. Çap/merkez için kontur noktaları önce dönüştürülür, geometri sonra fit edilir; piksel çapı veya görüntü centroid'i körlemesine dönüştürülmez. İlk DAG entegrasyonu mevcut iki Edge noktası arasındaki mesafedir; diğer tool yetenekleri uygulanmış sayılmaz.
5. Fit RMS mm, ters projeksiyon hatası px ve bağımsız validation RMS/maksimum hata mm ayrı gösterilir. Aynı fit köşeleri bağımsız kabul sayılmaz. Tek görüntü hesaplamaya yeter; doğruluk için fit dışında tutulan örnekler/ayrı referanslar ve fiziksel kullanımda merkez/kenar/farklı yönlerde kontrol gerekir. Kabul sınırı uygulama hata bütçesiyle belirlenir.
6. H, target geometrisi/aralığı, dünya orijin/eksenleri, giriş koordinat türü, resolution, kamera/kurulum, doğrulanmış bölge, örnekler/hatalar ve varsa intrinsic dependency sürümlü kaydedilir. Reçete açıkça bağlanır; kabul edilmemiş aday otomatik etkinleşmez.

Lens politikası iki açık yol içerir:

- Bozulmanın hedef doğruluğa göre ihmal edilebilir olduğu doğrulanmış kurulum: raw pixel → H → world mm. İçe aktarılmış sentetik örnekte bu yalnız test varsayımıdır.
- Bozulmanın ihmal edilemediği kurulum: raw pixel → undistort → H → world mm. Kabul edilmiş intrinsics kullanılabilir veya ayrı çok görüntülü lens kalibrasyonu yapılır. Bu yol uygulanmadan ihtiyaç duyan veri üretim doğruluğu kabulü alamaz; düzeltme yapılmış gibi gösterilmez.

H öğretimi ve ölçüm aynı koordinat alanında çalışır. undistortPoints varsayılan normalize çıktısı ile düzeltilmiş piksel çıktısı karıştırılmaz; P/yeni kamera matrisi ve distortion modeli sürümlenir. Homography radyal distortion'u çözmez. Kamera, focus/zoom, crop/resize/orientation veya kamera–düzlem ilişkisi değişince ilgili kayıt yeniden doğrulanır. Plaka referans yüzeyi ile ölçülen özelliklerin yüksekliği aynı olmalıdır. Düzlem dışındaki noktalar genel 3D ölçüm değildir.

OpenCV planı: findChessboardCorners + cornerSubPix veya doğrudan alt piksel veren findChessboardCornersSB; findHomography; perspectiveTransform. cornerSubPix circle-grid merkezleri için ortak zorunlu aşama değildir. Robust fit kullanılırsa eşik hedef koordinat birimiyle (image → world için mm) açık tanımlanır ve reddedilen noktalar raporlanır. warpPerspective yalnız isteğe bağlı görüntü sunumudur; çıktı mm/pixel, orijin ve boyutu ayrıca tanımlanır. İlk ölçüm teslimi görüntüyü yeniden örneklemeyi gerektirmez. Yerel OpenCV 5 modül/API doğrulaması K1'de yapılır.

Uniform scale legacy/sınırlı mod olarak korunur; homography başarısızken sessiz fallback yapılmaz. Telecentrik/fisheye gibi optik modeller ayrıca değerlendirilir.

### Geçici test görüntüsü içe aktarma ve gözlemlenebilir kabul

Bu özellik ilk uygulama paketine dahildir; kullanıcı hazır chessboard görüntüsünü kamera olmadan deneyebilir. Calibration sayfasında geçici test komutu olur; Recipe Editor'ın mevcut Open Image özelliğinden bağımsız calibration çalışma oturumuna görüntü yükler. PNG/JPEG/BMP başlangıç kapsamıdır; mevcut image-file decode/owned-frame servisi yeniden kullanılır. UI doğrudan OpenCV çağırmaz.

- Orijinal boyut ve piksel koordinatları korunur; viewer ölçeği kalibrasyonu değiştirmez. Dosya adı/içerik kimliği, çözünürlük ve **Dosyadan test** kaynak etiketi gösterilir. İçe aktarma aktif reçetenin kamera kaynağını sessizce değiştirmez; kendine ait source/frame/generation kökeni taşır.
- Kullanıcı iç köşe sayısı ve kare aralığını mm olarak girer; program görselden gerçek fiziksel kare ölçüsünü tahmin etmez. İnternetten alınmış veya çizilmiş board'a girilen mm, test dünya ölçeğini tanımlar; kullanıcının fiziksel kamerasını kalibre etmiş sayılmaz.
- Akış: **İçe aktar → target bilgilerini gir → Köşeleri bul → H hesapla → iki nokta seçerek mm mesafesini incele → bağımsız doğrulama**. Köşe sayısı/sırası, H, fit hataları ve seçilen noktaların px/mm değerleri gözlemlenir. K1 sonunda tespit overlay'i, K2 sonunda H ve mm denemesi çalışır.
- İki nokta denemesi sonuç incelemesidir; ayrıca bilinen bağımsız referansla karşılaştırılmadıkça validation olmaz. H hesaplandı durumu ile doğrulandı durumu ayrıdır; eksik holdout varken H/deneme gösterilebilir ama üretim izni verilmez.
- Yeni dosya/target/kurulum/lens seçimi eski köşeleri, H'yi, validation ve ölçüm sonucunu temizler. İptal/okunamayan/bozuk dosya veya bulunamayan/yanlış desen önceki başarılı sonucu yeni sonuç gibi sunmaz; worker stale-result kontrolleri korunur.
- K3'te test kanıtı dosya kökeniyle kaydedilip yeniden açılabilir; fiziksel kamera binding'ine otomatik dönüştürülmez. Aynı gerçek kameradan alınmış offline görüntüyü üretimde kullanma ayrı, açık kurulum eşleştirmesi ve doğruluk kabulü gerektirir.
- Kabul testleri: düz ve perspektifli sentetik board, yanlış target ölçüsü/sayısı (yanlış mm değerinin görselden kesin tespit edilemeyeceği açık), bulunamayan/bozuk görsel, yeniden import invalidation, zoom bağımsızlığı ve bilinen homography ile mm sonuçları. Otomatik testler yalnız fit residual'ını değil bağımsız bilinen dönüşümü de sınar. UI Release/CTest ve görsel kontrol; INI/fixture/görseller build altında, config/local.ui.ini korunur.
- Geçici düğme kaldırıldığında kalibrasyon servisi ve test fixture yolu korunabilir. Bu aşamada otomatik kaldırma tarihi yok; kalıcı UI kararı daha sonra verilir.

### 2. Robot Calibration — kamera, robot ve dünya ilişkisi

Hand–eye genel problem adıdır; kullanıcı modları **Eye-to-Hand** (sabit kamera) ve **Eye-in-Hand** (robot üzerinde kamera) olur. Kamera iç kalibrasyonu bunların önkoşuludur; mm ölçeği tek başına robot dönüşümü değildir. Dünya/çalışma frame'i robot base ile otomatik eşit kabul edilmez.

`T_A_B`, B koordinatlarını A'ya dönüştürür; C=camera, G=gripper/flange, B=robot base, T=target:

- Eye-in-Hand: target dünyada sabit, kamera gripper'a sabittir. Eşleştirilmiş robot `T_B_G` ve vision `T_C_T` örnekleriyle `T_G_C` çözülür; `T_B_G * T_G_C * T_C_T` sabit target pose'u vermelidir.
- Eye-to-Hand: kamera base'e, target gripper'a sabittir. `T_B_C * T_C_T = T_B_G * T_G_T` tutarlılığı çözülür; hedef `T_B_C`'dir. Target montaj dönüşümü biliniyor/çözülüyor ayrımı açık tutulur. Eye-in-Hand solver girişleri yön dönüşümleri yapılmadan aynen kullanılamaz.

Çeşitli konum ve paralel olmayan dönme eksenlerini kapsayan robot pozları gerekir; yalnız öteleme veya tek örnek yeterli değildir. Görüntü ve robot pose'u aynı durağan örneğe ait olmalı; timestamp, frame, mm/açı birimi, Euler sırası veya quaternion konvansiyonu kayıtlı olmalıdır. Simetrik chessboard yön belirsizliği kontrol edilir; robot için işaretli/asimetrik hedef veya doğrulanmış köşe sırası gerekir.

Bağımsız robot pozlarıyla konum (mm) ve yönelim (derece) hatası kabul edilir. TCP/tool, base/work frame, target montajı ve kamera montajı sürümlenir. Pick reference/TCP offset ayrı öğretimdir. Kalibre edilmiş 2D kamera keyfi parçanın altı serbestlik dereceli pose'unu üretmez: ilk guidance hedefi bilinen düzlemde X/Y/Rz; Z/Rx/Ry öğretim/sabit varsayım kaynakları açık etiketlenir. Tam 3D için ayrıca uygun gözlem/model gerekir. Sonuç gönderimi ve robot hareketi ayrı komut kalır.

## Model, kayıt ve geçerlilik planı

UI'da iki akış; içeride planar homography, gerektiğinde bağlı kamera intrinsics ve robot extrinsics ayrı sürümlü varlıklardır. Önerilen servis sınırları: CameraCalibrationService, PlaneMappingService, RobotCalibrationService ve mevcut katalog/resolver. Kesin sınıf adları uygulama adımında seçilir.

- Yeni tipli model zarfı/modelVersion; legacy `uniform_planar_scale/1` kanıtı ve hash'i değiştirilmeden okunur. Otomatik olarak yeni model veya fiziksel kabul sayılmaz.
- Recipe'nin mevcut tek calibrationBinding'i kamera/düzlem/opsiyonel robot bağımlılıklarını açıkça taşıyan sürümlü yapıya taşınır; migration açılışta sessiz üretim izni vermez. Her varlık ID/version/content ve dependency zincirini taşır.
- Kamera kimliği, lens, focus/zoom, çözünürlük, crop/binning/resize/orientation ve koordinat alanı; düzlem pose/yükseklik/orijin; robot mount/TCP/frame değişimleri ilgili bağımlı modelleri geçersiz kılar. Kullanılan intrinsics değişimi ona bağlı plane/robot kabulünü düşürür; yalnız plane değişimi sabit camera–robot extrinsics'i zorunlu olarak yeniden çözmez, ancak plane kullanan reçete yeniden doğrulanır.
- Fiziksel değişimler otomatik algılanamıyorsa operatör beyanı ve sınırı görünür olur. Webcam index kalıcı fiziksel kimlik sayılmaz.
- Target tanımı, görüntü/robot örnekleri veya hash'li kanıt varlıkları, köşeler, rejection nedenleri, solver sürümü, fit ve holdout sonuçları saklanır. Mevcut 4 MiB katalog içine sınırsız görüntü gömülmez; sınırlı evidence dosyaları ve atomik manifest/eksik varlık reddi tasarlanır.
- Candidate, matematiksel validation, fiziksel doğruluk kabulü ve Runtime configuration activation ayrı durumlar olarak korunur. Geçersiz modelde değer/PASS/robot pose üretilmez; sessiz scale fallback yoktur.

## Uygulama sırası ve çıkış koşulları

| İş | Teslim ve kabul |
| --- | --- |
| 6C-K1 — Model, geçici import ve plaka tespiti (tamamlandı) | Yerel OpenCV API/modül doğrulaması; target tanımı, Calibration test import, tam çözünürlüklü frame, gerçek corner overlay ve stale-result/red kontrolleri; legacy regresyonu |
| 6C-K2 — Tek görüntüden homography ve mm denemesi (tamamlandı) | H hesaplama, iki nokta px/mm inceleme, bilinen perspektif geometri testleri, fit/holdout ayrımı, dejenere/alan dışı dönüşüm reddi; raw/undistorted giriş sözleşmesi |
| 6C-K3 — Kayıt, Measurement ve Runtime (tamamlandı) | Tipli dependency migration; Calibration akışı; save/reopen, eski binding/invalidation, immutable snapshot; aynı Edge → dönüştürülmüş noktalar → mm/tolerans; Release/core/UI CTest ve görsel kontrol |
| 6C-KL — Gerektiğinde lens desteği | Ayrı intrinsic oluşturma/kabul edilmiş model kullanma ve undistort yolu; bozulmanın hata bütçesini aştığı kurulumların K4 kabulünden önce tamamlanır; her planar testin önkoşulu değildir |
| 6C-K4 — Fiziksel ölçüm kabulü | Gerçek plaka/bağımsız referans, merkez/kenar/yön dağılımı, tekrarlanabilirlik ve tolerans hata bütçesi raporu; donanım yoksa açık kabul maddesi |
| 6D — PLC | Mevcut handshake/simülatör planı; K4 fiziksel olarak bloke ise bağımsız simülatör işi ilerleyebilir, üretim kabulü iddia edilmez |
| 6E — Robot | Kamera modeline bağlı Eye-to-Hand/Eye-in-Hand servisleri ve simüle bilinen dönüşüm testleri; açı/Pose/pick reference, bağımsız fiziksel robot kabulü ve sonuç teslimi |

K1–K3 tamamlandı. K4 kullanıcı isteğiyle fiziksel kurulum hazır olana kadar ertelendi; gerektiğinde KL lens gerekir. Kalibrasyon teslimi UI 19/19/core 17/17, yeni PLC teslimi UI 20/20/core 18/18. Kullanıcı TCP/IP socket önceliğiyle 6D'yi yetkilendirdi; [6D.1 yazılım teslimi](plc-socket.md) tamamlandı, fiziksel doğruluk kabulü yerine geçmez. Robot 6E sonraki aşamadır. Mevcut tamamlanmış altyapı yeniden yazılmayacak, modeller aşamalı genişletilecektir.

## Teknik kaynaklar

- [OpenCV kamera ve hand–eye referansı](https://docs.opencv.org/4.13.0/d9/d0c/group__calib3d.html): teknik model/solver referansı; yerel OpenCV 5 API/ABI doğrulamasının yerine geçmez.
- [HALCON image_points_to_world_plane](https://www.mvtec.com/doc/halcon/2605/en/image_points_to_world_plane.html): kamera parametreleri ve dünya düzlemi pose'u ile pikselden fiziksel düzleme dönüşüm. HALCON bağımlılığı veya plaka format uyumluluğu bu planın kapsamında değildir.

## K1 uygulanan servis sınırı

`ChessboardService` ImageFileService owned decode'u kullanır; decoded piksel hash'i ve bağımsız calibration-test source/fresh frame kimliği üretir. `ChessboardTarget` ID/iç köşe grid/mm; `ChessboardObservation` raw pixel centres, resolution/provenance ve target-local mm grid taşır. O=P1, +X sütun, +Y satır; simetri nedeniyle fiziksel yön iddiası yoktur. Bu gözlem H veya validated calibration değildir, mevcut legacy kataloğu/schema/hash'i değiştirmez. Intrinsic/lens düzeltmesi uygulanmaz.

MainWindow worker'a immutable frame/target gönderir; EditorSession cancellation, generation ve recipe signature geç sonucu engeller. UI OpenCV çağırmaz; ImageViewer numara/O/+X/+Y çizimini yapar. 16 MP detection / 512 corner sınırı vardır; Gray16 sabit 1/257 ile tespit için Gray8'e çevrilir, stored raw frame değişmez. Stop cooperative, OpenCV içinde hard interruption yoktur. Test görüntüsü disk kaydına veya fiziksel kamera binding'ine girmez; homography envelope/persistence migration K2/K3'te genişletilir.

## K3 uygulanan kayıt/Runtime ve kalan fiziksel kabul

Tipli PlanarCalibration/Dependency schema 2, raw homography evidence record schema 2; existing catalog/resolver/recipe schema 2 ve immutable Runtime aynı hash'li modeli kullanır. Execution version 4. Test kanıtı `.vfcalstudy` ayrı 4 MiB manifesttir; source image path/decoded hash korunmalı, resim gömülmez ve fiziksel camera binding'i oluşturulmaz. Kaydedilmiş acquired modelde H/fit noktaları/köken/holdout kanıtı tutulur; kamera görüntüsü embed edilmez. Atomic replay matematiksel tutarlılıktır, fiziksel kurulum/ölçüm kanıtı değildir.

H fit inverse hata sınırı UI'da 1 px; ölçüm domain'i observed inner-corner convex hull. Holdout RMS/max ve her sample measured/absolute residual mm ile CSV raporu çalışır. Bağımsız fiziksel bölge/yön/tekrar kabulü [K4 prosedüründe](calibration-acceptance.md); gerçek veri yok, KL ihtiyacı belirlenemedi. Lens correction uygulanmış gibi gösterilmez. Legacy scale başarısız H için sessiz fallback değildir; UI model seçimi açıktır.

## 9 Ekim — Manuel px/mm girişi uygulandı (C1–C5)

Kullanıcı doğrudan px/mm katsayısı girebilmelidir. Ayrı “Manuel ölçek” seçeneğinde pozitif/sonlu px/mm, açık birim ve karşılığı mm/px = 1 / (px/mm) gösterilir. Örneğin 20 px/mm → 0.05 mm/px. Uniform ölçek varsayımı açıklanır; perspektif/lens/robot dönüşümü sağlanmaz.

Katsayı tek başına doğrulanmış kabul edilmez. En az iki bağımsız bilinen segment ile mevcut holdout hata sınırları uygulanır; gerçek kanıt olmadan RMS/PASS üretilmez. Manuel köken/katsayı, kaynak/çözünürlük/lens/düzlem ve sürüm kalıcı kayıtta tutulur; replay/hash, resolver, Measurement/Runtime ve eski kayıt migration/no-op testleri gerekir. Katsayı/kurulum değişimi eski binding ve sonuçları geçersiz kılar. Mevcut teaching örneğini sahte veriyle doldurmak yerine servis sözleşmesi genişletilir.

Ekran sadeleştirme: model seç → görüntü/kurulum → ölçek veya plaka → bağımsız doğrulama → kaydet/bağla. Yalnız seçili modele ait kontroller gösterilir. C1–C5 ile bu akış uygulandı; yazılım kabulü progress dosyasındadır. Fiziksel K4 açıktır.
## 8 Ekim — Manuel ölçek uygulama planı hazır

[Manuel px/mm ve sade Calibration planı](manual-scale-calibration-plan.md) önceki kısa kapsam notunun ayrıntılı devamıdır. C1 matematik/codec → C2 katalog/Measurement/Runtime → C3 sade ekran → C4 UI kabul → C5 birleşik test/belgeler. C1–C5 tamamlandı: matematik/codec, katalog/reçete/Runtime/CSV, sade üç yöntem ekranı ve birleşik yazılım kabulü mevcut. Yeni manual_planar_scale/1, record/dependency schema 3, eski hash/encode koruması, beş bölümlü üç yöntem UI ve doğrulamasız deneme/üretim ayrımı tanımlandı. Belgenin 7. bölümü GPT-6.1 Sol medium için kopyalanabilir başlangıç talimatıdır. P7 bu kapsamdan sonra; K4 ertelenmiş kalır.
