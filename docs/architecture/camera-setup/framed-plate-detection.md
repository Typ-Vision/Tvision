# Çerçeveli plaka algılaması — CCUX-15f

23 Ekim 2026. Application servisi `FramedCircleGridService::observe`;
`CircleGridService::observe` açık FramedCircles hedefini buraya yönlendirir.
Generic `detect`, chessboard ve eski codec anlamları korunur. Görünür profil/
overlay sihirbazı 15h, model ve gözlem kayıt/replay entegrasyonu 15g'dedir.

## Girdi ve ölçüm uzayı

7×7 koyu daire/açık zemin, kapalı koyu çerçeve ve iki komşu iç kenara bağlı
üçgen kesik profili desteklenir. Descriptor'daki bilinmeyen fiziksel boyutlar
algoritma tarafından doldurulmaz. Aralık, piksel algılamasının ölçeği değildir.

Gray8/16, RGB/RGBA ve BGR/BGRA kabul edilir; Gray16 çalışma parlaklığı /257'dir.
Raw köken, en çok 16 MP ve processing zincirinin boşluğu denetlenir.
İsteğe bağlı `QRect searchRoi` özgün piksel koordinatındadır; çerçeveyi keserse
ret verir. Raster/origin/hash değişmez. Geçici ölçek ve dörtgen dönüşümü yalnız
arama/topoloji içindir; merkezler özgün rasterde ölçülür. Lens/remap yoktur.

## Sınırlı hat

1. Raw/provenance ve ROI doğrulaması; immutable görüntünün çalışma kopyası.
2. En çok 1200 piksel arama ve yarım ölçeğinde 15 parlaklık eşiği; kontur
   hiyerarşisinde kapalı dış çerçeve ve iç açık alan. Dış sınır dörtgenle,
   üçgenli iç sınır beşgenle yaklaşıklandırılır. Küçük eğrilik bu yaklaşıkların
   toleransıyla sınırlıdır; her eğri çerçeve veya bozulma desteklenmez.
3. İç kesikte tek köşeye yakın bir diyagonal ve iki kenara bağlantı aranır;
   ayrık sahte üçgen ve çift köşe işareti ret. Aynı fiziksel çerçeve/işaretin
   eşik/ölçek tekrarları raw konumla tekilleşir. Farklı yorumlar korunur.
4. Bölgenin özgün rasterinde 15 yerel aralık eşiğiyle elips konturları;
   alan/eksen oranı, elips artığı, kenar gradyanı ve kontrast denetlenir.
   Yinelenen eşikler aynı merkeze birleştirilir. Bütün karede 49 blob aranmaz.
5. Adayların medyan boyutu ve normalize komşuluk kümeleriyle 7 satır/sütun;
   her hücrede tek gerçek aday gerekir. Küçük kir elenebilir. Aynı hücrede
   eşdeğer iki aday, eksik nokta veya birden çok plaka ret; nokta türetilmez.
6. Kontur eşik geçişleri bilinear gradyanla raw rasterde subpixel düzeltilir,
   elips yeniden fit edilir. En güçlü geçerli kenar seçilir; merkez ID'si
   değiştirilmez. Kalibre edilmiş belirsizlik tahmini sunulmaz (optional boş).
7. Tam gözlem, P1 ve raster eksen yönü factory denetiminden geçer. H fit'i bu
   hatta çağrılmaz; mevcut model 1 px inverse kapısı değiştirilmez.

Her eşikte en çok 20.000 kontur işlenir; en çok 16 çerçeve hipotezi ve 512
tekil aday vardır. Aşama, eşik ve kontur döngülerinde iptal noktaları vardır;
OpenCV'nin tek çağrısı ortasında kesme garantisi yoktur. Bütçe aşımı ret verir.

## Yönün kanıt sınırı

P1 işaretli köşeye en yakın merkezdir. +X o köşeden raw görüntüde saat yönündeki
kenara, +Y diğer kenara gider; dört 90° dönüş aynı hücre kimliklerini korur.
`quarterTurns` arama dörtgenindeki işaretli köşedir; ara açılarda en yakın
görüntü köşesine geçebilir. `mirrored=false` pozitif raw hücre winding'ini
anlatır; fiziksel görüntünün aynalanmadığına dair donanım kanıtı değildir.

Kare simetrik grid ve eş kenarlı köşe işareti fiziksel X/Y yansımasını tek
başına ayıramaz. Algılayıcı aynalamayı otomatik düzeltmez veya doğrulanmış diye
etiketlemez; bu sınır diagnostics `mirrorEvidence` içinde de açıktır. Fiziksel
eksen kabulü/aynalama için ek asimetrik işaret veya güvenilir edinim bilgisi
gerekir; 15j'de operatör/kurulum kabulü yerine raster winding kullanılamaz.

## Tanı ve fixture sözleşmesi

`PlateDetectionDiagnostics` sürüm, kullanılan parametreler, raw adaylar,
atanan/elenen adaylar, çerçeve sınırı ve input/frame-marker/raw-candidates/
topology-orientation/toplam sürelerini taşır. Ret noktaları model girdisi
olamaz. İptal/ret `Ready` veya model üretmez. UI bağlantısı henüz açılmadı.

Sentetik fixture bağımsız analitik disk/çerçeve koşullarından, 2×2 piksel
kaplamasıyla çizilir; beklenen merkezler önceden tanımlı QTransform'dan gelir.
0,8 px tolerans raster kaplaması, iso-kenar/elips ve perspektif merkez farkını
kapsar; ölçülmüş maksimum sonuç bu sınırı yeniden belirlemez. 2 px gerçek
fixture toleransı elle görsel kimlik işaretlemesi + ayrı Pillow alan-centroid
segmentasyonunun bulanık/asimetrik kenar belirsizliğini içerir. Bunlar mm veya
metrologik merkez doğruluğu iddiası değildir.

`tests/fixtures/framed-plate-external.json` iki eski raw/sidecar SHA-256'sını,
bağımsız oracle dosyalarını ve tarihsel yanlış 49 tanısını bağlar.
`prepare-framed-plate-baseline.py` hash denetimiyle build altında immutable
kopyalar/inceleme görselleri üretir. Yeni beklenen cevabı algılayıcıdan çıkarmaz.
Raw dosyaları depoya alınmamıştır: harici CTest eksik rasterde exit 77/Skipped,
değişmiş byte/sidecar veya yanlış sonuçta Failure verir. Sentetik test ayrı ve
zorunludur. Gerçek raster başarı kapsamı skip ile birleştirilemez.

56000 µs eski karede 50 adaydan 49 merkez seçilmiştir. İlk karanlık raster
raw kenar/kontrast kapısında ret kalır. Saklanmamış 58-aday olayı yeniden
üretilmemiştir; 58→49 fixture sentetiktir. Son koşum/kanıt ve fiziksel açıklar
[kapanıştadır](../../archive/2026-10-23-ccux-15f.md).
