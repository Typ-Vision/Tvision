# Ham plaka gözlemi — CCUX-15e API

23 Ekim 2026. 15e servis sınırı; çerçeveli algılama
[15f'de uygulandı](framed-plate-detection.md). Model/kayıt/replay 15g,
görünür sihirbaz akışı 15h kapsamındadır.

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
codec'idir; yeni model/gözlem kalıcılığı henüz uygulanmadı.

Kaynak envanteri: aggregate çağrıları Wizard, testler ve EditorSmoke'tadır;
hedef codec'ini CalibrationTargetLibrary ve PlaneMappingService kullanır.
PlaneMapping v1 chessboard, v2 generic circle olarak kalır. CalibrationService
ve CalibrationStudyStore legacy recipe/study yolunu chessboard ile sınırlar.
ActiveCalibrationService generic circle yönünün belirsizliğini korur.
CalibrationTargetLibrary çerçeveli hedefi okuma/yazmada açıkça reddeder; eski
UI bir v2 hedefi sessiz generic seçeneğe dönüştüremez. 15h bu kapıyı yeni UI
ile birlikte açacaktır. Model fit'i hem legacy gözlem
hem typed gözlem girişinde açık ret verir. Var olan codec'e kaçamaz.

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

WizardSession worker'ındaki ikinci fit de kaldırıldı. Worker/accept yalnız
güncel raw gözlemi sunar; ticket/iptal/eski completion koruması değişmedi.
`confirmPlate` grid incelemesidir, model kabulü değildir. `calculatePlate`
(ayrı `teachPlate` sembolü mevcut kaynakta yok) hâlâ PlaneMappingService::fit
ve CalibrationModelRecord::create çağırır. Fit reddi gözlemi korur, hesaplanmış
model bırakmaz. SaveUse'e yalnız gözlem/tanı verilemez.

PlaneMappingService'in model kapıları değiştirilmedi: tam sıra/sayı, duplicate,
konvekslik, hull, kararlı H ve varsayılan maksimum 1 px inverse hata; wrapper'ın
bağımsız kontrol koşulları da korunur. Typed raw overload aynı fit'e gider.
Çerçeveli model entegrasyonu tamamlanana dek açıkça reddedilir.

`calibration_plate_contract` bağımsız sinüzoidal sütun kaymalı raster ile
algılama → worker → accept → confirm başarısını, ardından H hesabının reddini
sınar. Bu lens solver testi veya fiziksel kamera/mm kabulü değildir.
