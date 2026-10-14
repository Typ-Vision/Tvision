# Ön işleme ve kod okuma — uygulanan sözleşme

10 Ekim T8.4.2b.1: `code_read` v2 programatik explicit poseTracking/reference DAG ile raw dönen dikdörtgen maskesi kullanır. Dış crop pikselleri beyazlanır ve decoded polygon'un bütün köşeleri ROI içinde olmalıdır; raw frame değişmez. Engine execution/17, adapter method zxing-reader/v1 ve pinned backend korunur. Teaching UI ve production pose whitelist/trusted completion tracking kabulü açıktır; aşağıdaki T3.3 Runtime kabulü tracking içermeyen kod reçeteleri içindir. [Güncel sözleşme](pose-roi.md).

9 Ekim 2026: T2.3, T3.1, T3.2 ve T3.3 uygulanmıştır. Kod okuma etkileşimi/overlay ve birleşik UI kabulü tamamlandı. Üç applicationType aynı registry, executor, Recipe Editor ve Runtime yolunu kullanır.

## Preprocess

Toolbox → Preprocess → Image bağlantısı. Kanalı seçin; **Ekle / Sil / ↑ / ↓** ile filtreleri sıralayın. **Run Flow** yeni kare edinir; **Run Tool** yalnız seçili aracı güncel upstream cache üzerinde çalıştırır. Seçili Preprocess'in gerçek çıktısı görüntü alanında **İşlenmiş Gray8** olarak gösterilir. Başka araca geçince ham görüntüye dönülür. Runtime'da da seçili Preprocess çıktısı ayrı etiketlenir. Sayısal piksel bilgisi gösterilen görüntüye aittir.

`preprocess` v1, editor schema 7: `{channel, steps}`. Kanal `gray/red/green/blue/alpha`; her adım exact `{op,size,value}`. En çok 16 adım, tam görüntü en çok 16 Mi piksel. Çıktı owned Gray8; aynı boyut, raw pixel-centre geometri ve acquisition/frame/cycle metadata. Gray16 sabit `255/65535` ölçeğiyle Gray8'e yuvarlanır; renk kanalları semantik RGB adlarıyla seçilir, eksik kanal ERROR verir. Gray dönüşümü OpenCV luminance kullanır.

| op | size | value |
| --- | --- | --- |
| gaussian | 3–31 tek sayı | sigma 0–100; 0 otomatik |
| median | 3–31 tek sayı | 0 |
| threshold | sabit kaydedilen tek sayı; hesapta kullanılmaz | 0–255, `pixel > value` → 255 |
| adaptive_mean / adaptive_gaussian | 3–31 tek sayı | çıkarılan C, −255–255 |
| erode / dilate / open / close | 3–31 tek sayı, dikdörtgen kernel | 0; bir iterasyon |

Gaussian ve morphology `BORDER_REPLICATE`; median/adaptive OpenCV replicate davranışı. Tam kare işlenir: maske, ROI dışı koruma veya mask hole politikası uygulanmaz. Binary delikler seçilen morphology işleminin gerçek sonucuna göre değişir. Crop/resize/lens dönüşümü yoktur. Her tool çağrısı `ImageAccess::processed` üzerinden parametre SHA-256, yöntem/sürüm ve input content hash ekler; toplam processing chain 32 ile sınırlıdır. Girdi değişmez. EditorSession yalnız acquisition source çıktısını ham kare olarak tutar. Kalibrasyon resolver ham kaynak kareyi kullanır; downstream aynı raw koordinatlarında ölçer. Zoom/pan ve ROI geometrisi aynı boyutlu önizlemede korunur.

## Code Read servis çekirdeği

`vision/codes/CodeReader` ZXing 2.3.0 adapter'ıdır. `code_read` v1 registry/executor ve Production whitelist'tedir; editor schema 7 ve explicit `zxing-reader` version/archive SHA-256 dependency gerekir. ProjectService yeni servis aracını eklerken mevcut gerçek backend kimliğini pinler; OFF build eklemeyi atomik reddeder. Açık/kapalı bütün konfigürasyonlar strict doğrulanır. Toolbox Code Read ve Properties format/içerik/adet/seçim/limit ayarlarını sunar. ZXing OFF girdisi devre dışıdır; servis de eklemeyi atomik reddeder.

Format allow-list: QRCode, DataMatrix, Code128, Code39, EAN-8, EAN-13, UPC-A, UPC-E. Typed sonuç adları mevcut codec ile aynı: QR, DataMatrix, Code128, Code39, EAN8, EAN13, UPCA, UPCE. Integer raw rectangle ROI sınır dışındaysa ret; boş nesne tam karedir. Full input en çok 16 Mi piksel. Gray8 hazırlığı aynı owned erişim sınırını kullanır; crop stride korunur, köşelere ROI offset eklenir. Raster döndürülmez/ölçeklenmez. ZXing native rotation/inversion açıktır; no-read durumunda bütün raster ters çevrilerek en fazla ikinci native deneme yapılır. Karışık polariteli çoklu 1D kodların eksiksiz bulunması kabul edilmedi.

`maxCount` 1–128; reader `maxCount+1` ister ve sınırın üstünde **ERROR** verir; sessiz kesilmiş liste kabul edilmez. Native reader'ın bulamadığı semboller için tamlık garantisi yoktur. `budgetMs` 1–60000; dönüşüm, native çağrı ve bounded codec denetiminden sonra kontrol edilir. Stop/deadline cooperative'dir: sürmekte olan ZXing/OpenCV çağrısını zorla kesmez; geç çıktı atılır. Bir kodda text/raw bytes 64 KiB; mevcut typed codec 2 MiB toplam payload sınırı geçerlidir.

Sonuçlar ilk raw köşenin Y, X'i, semboloji ve raw bytes ile kararlı sıralanır; aynı metinli farklı konumlar birleştirilmez. Text Plain UTF-8/ECI, raw bytes ve GS `0x1d` korunur; otomatik GS1 alan ayrıştırma/normalizasyon yoktur. Confidence üretilmez. Gerçek Unicode ECI, Data Matrix/Code128 GS ve aynı metinli iki QR bağımsız Zint fixture'larıyla sınanır. Bu GS separator kabulüdür; GS1 AI/FNC1 parser kabulü değildir.

## İçerik ve kayıt

Portlar: `codes:CodeResults(px,image)`, `count:Number(count,image)`, `found:Boolean`, `accepted:Boolean`, `checks:InspectionResults`. Yöntem `zxing-reader`, adapter sürümü 1; gerçek backend sürüm/hash snapshot dependency'dedir. NO_READ, geçerli boş `codes`/count=0/found=false verir; backend/config/timeout ERROR ve iptal SKIPPED çıktı bırakmaz. Read durumu içerik kabulünden ayrıdır. `accepted`, adet ve içerik kurallarının Boolean birleşimidir; genel parça PASS/FAIL değildir.

`minCount/maxExpected` dahil adet; rule `none/exact/prefix/length/regex`. Exact/prefix case-sensitive ve normalize edilmez. Length UTF-16 code unit sayısıdır, 0–4096 dahil sınırlar. Seçim `all/any/selected`; selectedIndex sıfırdan başlar, eksik index false. Boş listede yalnız `none + all + minCount=0` içerik kontrolünü geçebilir; metin kuralı boş listeyle geçmez. Checks ayrı count/content adı ve mismatch reason taşır.

Regex bütün metni eşleştirir; pattern en çok 256, input en çok 4096 UTF-16 birimi. Qt Core güvenli altkümesi: group/alternation/backreference yok, character class dışında toplam en çok bir quantifier. İç içe tekrar ve çoklu tekrar ret; arbitrary script yok. Native regex çağrısında hard timeout iddiası yoktur.

Execution kimliği `visionflow.execution/8`; root/recipe/terminal/PLC şemaları değişmedi (terminal 3, vf.socket/2). Typed code/check sonuçları mevcut completion/replay/Logs/CSV codec'lerini kullanır. Save/reopen/no-op, immutable Runtime, dependency tamper, processed upstream değişiminde TargetOnly invalidation sınanır. Görüntü bytes arşive yazılmaz. Kodla reçete değiştirme, URL açma ve komut yürütme yoktur.

## Kod Oku kullanıcı akışı (T3.3)

Image File/Webcam → isteğe bağlı Preprocess → **Code Read** ekleyin, Properties Image girişini bağlayın. Format allow-list en az bir öğe ister. **Search ROI çiz** integer raw px dikdörtgenini reçeteye kaydeder; **Tam kare ara** temizler. Bu alan geçici “ROI geometrisi…” çalışma alanından ayrıdır. Adet aralığı dahil; exact/prefix/length/regex ve all/any/seçili sıra (0 tabanlı) ortak servis sözleşmesine bağlıdır. Uyumsuz aralık/boş format/tehlikeli regex kaydedilmez; hata gösterilir, son geçerli konfigürasyon korunur.

**Run Flow** yeni görüntü edinir; **Run Tool** geçerli upstream cache üzerinden yalnız kod aracını çalıştırır. Results ve Runtime sıra/format/kaçışlı metin/raw byte sayısını, adet ve içerik kontrollerini gösterir. Görüntüde gerçek raw köşeleri birleştirilir, #0/#1 etiketi listedeki sırayla aynıdır. Metin plain text ve JSON escaped sunulur; NUL/GS kaybolmaz, HTML/URL çalıştırılmaz. NO_READ boş liste, ERROR/SKIPPED çıktısızdır; parametre/Stop eski liste ve polygon'u temizler. Adet/içerik “Uygun” sonucu genel PASS değildir. Runtime yalnız terminal kayıt sonrası gösterir; Logs aynı okunur listeyi ve tam typed kanıtı korur.

Length **UTF-16 birimi** olarak açık gösterilir: A + emoji 3 birimdir (2 code point / 5 UTF-8 byte). Mevcut v1 kararlarını değiştirmemek için algoritma planındaki code point varsayılanından bu sapma bilinçli kaydedildi; gelecekte farklı birim sürümlü olacaktır. Karışık açık/koyu çoklu 1D tamlığı kabul edilmedi; UI uyarısı adet kontrolünde tek polarite kullanmayı söyler. Native decoder'ın sembol bulamaması yokluğu kanıtlamaz.

Release UI smoke sekiz bağımsız encoder formatını ortak editor üzerinden çalıştırır; rectangle/full-frame, exact ayar Save/reopen/no-op, gerçek dört kenar+etiket, TargetOnly/raw/zoom/0 acquisition, Stop ve immutable Runtime/Logs/CSV sınanır. Normal/küçük Editor ve Runtime/Logs PNG'leri `build/preprocess-code-ui/code-*.png`; küçük ekran Properties dikey scroll kullanır. Offscreen kontrol native DPI kabulü değildir.

## Kabul sınırları

Yazılım testleri sentetik bağımsız encoder görselleri üzerindedir. Hasarlı/checksum/DPM/baskı/bulanıklık/gerçek corpus, native DPI ve fiziksel K4 ayrıca açıktır. ISO baskı kalitesi, kurulum/dağıtım/CUDA veya üretim doğruluğu kabulü yoktur. Sonraki adım **T4.1 — Sampler/profil**.
