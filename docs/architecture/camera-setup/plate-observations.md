# Ham plaka gözlemi — CCUX-15e API

23 Ekim 2026. 15e servis sınırı; çerçeveli algılama
[15f'de uygulandı](framed-plate-detection.md). Model/kayıt/replay 15g ile uygulandı;
görünür sihirbaz akışı 15h ile uygulanmıştır.

## Hedef ve uyumluluk

`CalibrationGridTarget` aggregate alanları ve `ChessboardTarget` /
`ChessboardObservation` alias'ları korunur. Sona eklenen optional `framed`
alana yalnız `FramedCircles` için izin verilir. İlk profil 7×7, koyu daire/açık
zemin, üçgen köşe yönü ve eşit pozitif merkez aralığıdır. Çap, çerçeve kalınlığı
ve iç genişlik/yükseklik isteğe bağlı pozitif sonlu mm değerleridir; bilinmeyen
alan yazılmaz. Aralık kullanıcı beyanıdır, fiziksel doğrulama değildir.

Eski chessboard/generic hedef JSON'u v1 olarak aynı alanlarla yazılır.
Çerçeveli descriptor v2, `framed_circle_grid`, `marked_centre_1` ve açık
`geometry` alanıyla ayrılır; bilinmeyen alan/sürüm reddedilir. Bu descriptor
codec'idir; yeni model/gözlem kalıcılığı aşağıdaki v3 yolundadır.

Kaynak envanteri: aggregate çağrıları Wizard, testler ve EditorSmoke'tadır;
hedef codec'ini CalibrationTargetLibrary ve PlaneMappingService kullanır.
PlaneMapping v1 chessboard, v2 generic circle olarak kalır. CalibrationService
ve CalibrationStudyStore legacy recipe/study yolunu chessboard ile sınırlar.
ActiveCalibrationService generic circle yönünün belirsizliğini korur.
CalibrationTargetLibrary, 15h ile çerçeveli descriptor okuma/yazımını destekler;
sihirbaz açık üçüncü profil olarak geri yükler. Bilinmeyen fiziksel alanlar boş
kalır; merkez aralığı yeni kare/hedef/aralıkta yeniden teyit edilir. Legacy recipe/study yolu çerçeveli hedefi reddeder; yeni model v3 codec'ine gider.

## Gözlem ve tanı

`CalibrationGridObservation::validateRaw()` yalnız tamlık, raw köken, 16 MP /
512 nokta sınırı, 64 küçük hex raster hash, sonlu/sınır içi merkezler,
0,5 px üstü merkez ayrımı, hedefin satır/sütun dünya dizisi ve hücrelerin
katlanmayan tutarlı konveks topolojisini doğrular. H hesabı çağırmaz.

`RawPlateObservation::create` bunlara ek olarak her merkez için birebir
satır/sütun kimliğini, raw konum eşitliğini, varsa 0–100 px sonlu belirsizliği,
algoritma sürümünü ve `raw_pixel_centres/1` uzayını doğrular. Nesne yalnız const
alan erişimi sağlar. Hedefin ID/içeriği/sürümü descriptor içinde; frame/cycle,
kaynak, koordinat ID, çözünürlük ve piksel hash'i grid içinde taşınır.
Raster origin/edinim ayrıntısı FrameMetadata içeriğinde korunur; gözlem
hiçbir crop/warp veya lens düzeltmesi yapmaz.

Çerçeveli gözlemde üçgen raw konumu zorunlu, quarter-turn 0–3 ve mirror alanı
açıktır. İşaret, dört grid köşesi içinde P1’e tekil olarak yakın olmalı;
mirror bayrağı raw hücrenin yön işaretiyle tutarlı olmalıdır. Kanonik P1=(satır 0,sütun 0), +X sütun, +Y satırdır. Generic/chessboard
`Unmarked` taşır; üçgen/dönüş/mirror kanıtı uydurulmaz. Bu factory yapısal bir
sözleşmedir; üçgenin rasterde gerçekten varlığını kanıtlayan algoritma değildir.
O denetim ve dönmüş kimliklerin doğruluğu 15f algılayıcısında uygulanır;
fiziksel aynalama kanıtı ile raster hücre yönü ayrımı ilgili sınır belgesindedir.

`PlateDetectionResult` private kuruculu Ready+immutable gözlem veya
Unsupported/InvalidInput/NotFound/Cancelled tanısıdır. Tanı kısmi adayları model
girdisi değildir; tanıyı Ready ilan ederek gözlem üretilemez.
`CircleGridService::observe` generic algılayıcıyı bu sözleşmeye uyarlar;
çerçeveli hedefi `FramedCircleGridService` servisine yollar. Eski `detect`
exception API'si yalnız generic anlamını korur.
Generic algılayıcı belirsizlik tahmin etmez; değer boş kalır.

## Model ve oturum sınırı

CircleGridService'in iki çıkışındaki H fit, `validateRaw` ile değiştirildi.
OpenCV tekrar eden/katlanan atamaları hâlâ reddedilir. Generic algoritmanın
önceki tam blob sayısı ve dar eksen hizalı fallback politikası korunur;
yeni çerçeve/topoloji algoritması bu karta dahil değildir.

WizardSession worker'ındaki ikinci fit de kaldırıldı. Worker/accept typed yön kanıtıyla güncel raw gözlemi sunar; ticket/iptal/eski completion koruması değişmedi.
`confirmPlate` grid incelemesidir, model kabulü değildir. `calculatePlate`
(ayrı `teachPlate` sembolü mevcut kaynakta yok) hâlâ PlaneMappingService::fit
ve CalibrationModelRecord::create çağırır. Fit reddi gözlemi korur, hesaplanmış
model bırakmaz. SaveUse'e yalnız gözlem/tanı verilemez.

PlaneMappingService'in model kapıları değiştirilmedi: tam sıra/sayı, duplicate,
konvekslik, hull, kararlı H ve varsayılan maksimum 1 px inverse hata; wrapper'ın
bağımsız kontrol koşulları da korunur. Typed raw overload aynı fit'e gider.
Çerçeveli hedef yalnız typed gözlem overload'ıyla ve en fazla 1 px sınırla hesaplanır; çıplak grid overload'ı reddedilir.

`calibration_plate_contract` bağımsız sinüzoidal sütun kaymalı raster ile
algılama → worker → accept → confirm başarısını, ardından H hesabının reddini
sınar. Bu lens solver testi veya fiziksel kamera/mm kabulü değildir.

## CCUX-15g model ve replay

PlaneMapping v3 (`raw_planar_homography/3`) descriptor v2, raster hash,
49 row/column/raw merkez, isteğe bağlı belirsizlik, algoritma/uzay sürümü,
üçgen konumu/dönüş/mirror ve acquisition sidecar nesnesini saklar.
`observationId`, solver çıktıları hariç hedef/kaynak/raster/gözlem alanlarının
canonical compact JSON SHA-256 bağıdır. PlaneMapping::rawPlate() ve Wizard
result/session rawPlate() bu typed kanıtı görünüm katmanına açar.
V1/v2 yazımı ve hash anlamı korunur; yeni profil eski codec'e indirgenmez.

Decode yalnız H veya başarı bayrağına güvenmez: tam şema/alanlar, hedef,
merkez kimlikleri, topoloji/yön factory denetimi, raw bounds ve mevcut solver
kapıları yeniden çalışır; yeniden encode tüm kayıtla birebir eşleşir.
CalibrationModelRecord ayrıca raster hash ve varsa exact acquisition sidecar
bağını doğrular. ExperimentalCalibrationEvidence mevcut kamera/geometri,
origin/crop, readback ve raster kapılarını korur. SaveUse mevcut atomik katalog/
revision/generation/iptal/ret sözleşmesini kullanır; bağımsız kontrol, hull ve
1 px inverse kapıları gevşetilmez. Yeni scan, invalidatePlate veya kare değişimi
typed gözlemi/onayı/modeli birlikte siler.

Bu replay rasteri yeniden algılamaz; yön factory'si işaretin grid köşesine
tekil yakınlığını ve raw hücre yönünü denetler. Üçgenin rasterde varlığı ilk
algılayıcının kanıtıdır; kayıtlı algoritma adı veya yerel hash, imza/sertifika
veya düşmanca yeniden yazıma karşı fiziksel kanıt değildir. Fiziksel mirror,
12 mm aralık doğruluğu ve bağımsız mm kabulü açık kalır. Rapor yeni profilde
üçgen yapısal kanıtını söyler, eski kayıtlara yön bulundu anlamı eklemez.

## CCUX-15h sihirbaz

Çerçeveli 7×7 seçimi eski iki profili dönüştürmez. Aralık için açık teyit gerekir;
7×7 adet kilitlidir. Arama alanı özgün piksel QRect olarak worker işine taşınır,
raster kırpılmaz; zoom/pan yalnız görünümü değiştirir. ROI yalnız bu sihirbaz
karesine aittir ve yeni karede temizlenir. Algılama worker'ı tanıyı/typed gözlemi
generation ticket ile kabul eder; hedef/aralık/ROI değişimi ve Stop eski işi
iptal ederek gözlem/onay/adayı birlikte temizler. Dosya/çıkış/SaveUse, 22e drain
semantiğini kullanır.

Tanı aday sayısı ile eşleşen merkezi ayırır; numaralar, P1/+X/+Y, çerçeve ve
reddedilmiş/kısmi aday etiketleri raw koordinatlarda gösterilir. Hull yalnız
hesaplanmış modelde gösterilir. 49 merkez gözlemi fit reddinde korunur; kayıt
adımı açılmaz. Fiziksel kontrol ayrı bilgidir; aralık teyidi fiziksel kabul değildir.

UI regresyonu sırasında görülen generic OpenCV graph atamasının beyaz arka plan
konumu gerçek merkez kabul edilmez; mevcut 127 eşikli measured-component fallback
korunur. Yeni algoritma/fit gevşetmesi yoktur; eski generic tamlık ve model kapıları
sürer. Chessboard/generic UI adayları da yeniden sınanmıştır.
