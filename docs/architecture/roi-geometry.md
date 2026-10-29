# T2.1–T2.2 görüntü ve ROI sözleşmesi

9 Ekim 2026. Uygulandı; preprocessing algoritması/tool'u T2.3'te kalır.

## Görüntü sınırı

`vision/ImageAccess` bir ImageFrame referansını sahiplenir ve yalnız `span<const byte>` satırları verir; padding dahil değildir. Span ömrü access nesnesi kadardır. `ownedCopy()` ayrı, değiştirilebilir Mat üretir; raw piksel tamponuna mutable Mat header verilmez. Mevcut legacy adapter friend'ları korunur; yeni vision algoritmaları bu sınırı kullanır. UI ImageAccess/OpenCV çağırmaz.

`processed()` output'u kopyalar; boyut değişikliği ve format/type uyuşmazlığını reddeder. Aynı acquisition source/frame/cycle/raw coordinate kimlikleri korunur. Her adım input piksel SHA-256, method/version ve 32 byte parameters SHA-256 taşır; zincir en çok 32 adımdır. Raw `contentId()` semantiği değişmez; `processingId()` ayrı kimliktir ve EditorSession evaluation/cache scope'una katılır. Aynı piksel üreten farklı işlem de farklı scope olur. Image result trace yalnız boyut/format/processingId taşır; piksel bytes veya yeniden çalıştırma modeli arşivlenmez. Bu sınır crop/resize/rectification/lens düzeltmesi uygulamaz.

## Geometri

`domain/ToolRoi` OpenCV'siz closed variant ve exact schema 1 JSON kullanır. Şimdilik raw pixel, XRightYDown frame gerekir; mm veya pose-referenced ROI adapter'ı yoktur. Noktalar double/subpixel'dir. Pixel centre alanı `[0,w-1] × [0,h-1]`; bütün footprint alan içinde olmalıdır. Sınır dışı şekil kırpılmaz veya AABB'ye dönüştürülmez. Bounds yalnız kapsama denetimidir.

| Şekil | Parametreler ve anlam |
| --- | --- |
| ScanSegment | Başlangıç/bitiş ve tam dik şerit genişliği; ok başlangıçtan bitişe |
| RotatedRectangle | Merkez, tam width/height, +X'ten +Y'ye pozitif derece (raw'da saat yönü) |
| Annulus | Merkez; `0 < inner < outer`; disk/zero-inner bu sürümde yok |
| ArcRoi | Halka ve start/signed sweep derece; `0 < abs(sweep) ≤ 360` |
| PolygonRoi | En az üç vertex; kapanış implicit, sıfır alan ve self-intersection reddedilir |
| PathRoi | En az iki vertex; açık polyline, consecutive duplicate reddedilir; crossing henüz bead acceptance değildir |

Koordinat/boyut büyüklüğü en çok 1e8; polygon/path en çok 1024 vertex, JSON 128 KiB. NaN/Inf/unknown field/kind/future schema reddedilir. Şema mevcut editor recipe parametrelerine kendiliğinden eklenmez; legacy rectangle/model teaching aynen kalır.

## Viewer kullanımı ve ürün sınırı

Editor'da görüntü varken **ROI geometrisi…** geçici çalışma alanını açar. Sayısal alanlar ve mouse aynı ToolRoi codec/validasyon yoluna gider. **Çiz** sonrası ok/kutu/halka/yay sürüklenir; polygon/path noktaları tıklanır, Enter tamamlar, Backspace son vertex'i çıkarır. Tutamaçlar endpoint/width, centre/size/angle, radius/start/sweep veya vertex düzenler. Escape yarım çizim/drag'i atar; önceki geometri kalır. Geri al/Ctrl+Z her tamamlanmış mouse veya sayısal düzenlemeyi tek adım geri alır; geçmiş en çok 64 adım.

Şekiller **geçici** etiketi taşır; reçeteye kaydedilmez, tool output'u/ölçüm/karar üretmez ve gerçek yürütme sonuçlarını değiştirmez. İlgili yeni araçlara recipe binding sonraki tool paketlerindedir. Yeni görüntü/clear önceki geometri ve undo geçmişini siler. Zoom/pan koordinatları değiştirmez. Legacy rectangle çizimi ve kalibrasyon point callback'i korunur; point callback artık subpixel scene konumunu taşır.

Raw domain sınırı çizilir. `setValidDomain()` çağrısı varsa ek polygon outline gösterilebilir; bu API bu teslimde test fixture'ıyla doğrulandı, otomatik kalibrasyon alanı çözümlemesi yapılmaz. Çok küçük zoom'da yakın tutamaçlar örtüşebilir; zoom veya sayısal düzenleme gerekir. Toolbar küçük ekranda yatay, sayısal tablo dikey scroll kullanır. Native DPI/çoklu monitör kabulü yapılmadı.

## Kanıt

Yeni Release `build/tool-roi-ui` (ZXing OFF) CTest 31/31; `build/tool-roi-core` (ZXing ON, Widgets OFF) 28/28. `roi_geometry_contract` ownership/all six formats/stride/provenance ve analitik footprint; `roi_viewer_interaction` bütün şekiller/mouse/handles/numeric/Escape/undo/zoom/legacy rectangle; ana smoke gerçek Editor entry/no recipe mutation sınar. Normal/küçük PNG'ler `build/tool-roi-ui/t2-roi-*` ve `t2-editor-roi*` altında açılıp incelendi. Testler fiziksel doğruluk veya kurulum/dağıtım/CUDA kabulü değildir.

## EFR-03b — reçete ROI ve viewer rasterı

Geçici geometri çalışma alanı yukarıdaki sınırını korur. Reçete ROI'sinin
`cropImage` modu [ROI crop sözleşmesindeki](roi-crop-contract.md) owned child
rasterı kullanır. Seçili tüketicinin çizimi/önizlemesi bağlı Image girdisinin
local koordinatlarındadır; ROI'nin açık güncel sonucu child rasterdır ve parent
Region child üstünde düzenlenmez. Giriş görüntüsünde çiz/düzenle eylemi ayrı
sunulur. Görüntü sunum kimliği local coordinate ID içerir; aynı boyutta farklı
konum değişimi eski overlay/undo'yu taşımaz. Runtime ham görüntüsünde child
layer'ları source/frame/cycle/root scope doğrulamasından sonra rootOffset
farkıyla açıkça çevrilir; seçili child viewer local layer kullanır.
Aktif çizim aynı content/processing/geometri ve source/frame/cycle girdisiyle
yenilemede korunur; bu köken değişirse taslak iptal edilir. İşlenmiş görüntü
etiketi ve bağlı kaynak adı gerçek seçili Image rasterını anlatır.

## Tarihsel görüntü sınırı

Terminal replay saklanan sonuç ve kökenin okunmasıdır. Image trace raster bytes
taşımaz; crop için root/local coordinate ID, raw boyut, child boyut, rect zinciri,
offset ve doğrulanan geometri kimliği saklanır. Frame/cycle ilişkisi terminal
kaydıyla doğrulanır. Bu scope piksel veya yeniden çalıştırma izni üretmez.
Logs görüntünün mevcut olmadığını açıkça belirtir; güncel kare geçmiş sonuca
arka plan yapılmaz, geçmiş crop/overlay çizilmez. Scope taşımayan eski kayıt
aynı sınırlı tarihsel bilgiyle okunur; crop dönüşümü tahmin edilmez.
