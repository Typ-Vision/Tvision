# CCUX-21 — PreProcess altında reçeteye bağlı ROI

Durum: 22 Ekim 2026 yazılım ve görsel kabulü tamamlandı.
[Kapanış](../../archive/2026-10-22-ccux-21.md). CCUX-20 gerçek kamera etkileşim
teyidi ve CCUX-15 fiziksel kabulü ayrı açık maddelerdir.

## Mevcut durum

MainWindowEditor'daki üst menü “Geçici geometri denemesi…” geçici subpixel geometriyi
düzenler; reçeteye bağlı değildir. Bu alan bir preprocessing işlemi değildir.
Araç Properties bölümündeki Model/Search ROI ve metrologi ROI çizimleri
reçeteye bağlıdır ve ilgili yürütücüde kullanılır.

## Oku

contract C4; ToolRegistry region portları, mevcut ROI/Preprocess yürütücüleri,
ImageAccess processing kökeni, ImageViewer/RoiEditor ve ilgili tool testleri.
Gereksinim belgesinde ROI / Preprocess bölümlerini başlık aramasıyla aç.

## Hedef ve kabul

- PreProcess kategorisinden eklenebilen, isimlendirilen ve kaydedilen ROI aracı;
  mevcut viewer geometrisi çizim altyapısı yeniden kullanılır.
- Image girdisinden aynı kaynak/koordinat sistemine ait Region çıktısı;
  region destekli downstream araçta bağlanan ROI gerçek işlem alanını sınırlar.
  Port desteği olmayan araçlar destekliyormuş gibi sunulmaz.
- Fiziksel raster kırpma/yeniden boyutlandırma ilk kapsam değildir. Görüntü
  origin/geometri/kalibrasyon bağı korunur; maskeyi siyaha boyamak otomatik
  olarak ölçüm alanını sınırlamak sayılmaz.
- Çiz → bağla → Run Tool/Flow → kaydet/aç → Runtime zincirinde ROI içi/dışı
  farklı fixture sonuçlarıyla işlem etkisini kanıtla. Source değişimi, bounds,
  stale cache, çok kaynak ve unsupported geometri negatifleri ekle.
- Geçici üst menü geometrisini işlevsel araçla karıştırmayan açık yönlendirme;
  mevcut tool ROI'leri ve tarifleri geriye uyumlu koru.
- Release, ilgili CTest ve normal/küçük ekran görsel kabulü.

Hand-eye, yeni kalibrasyon solver'ı ve production admission kapsam dışıdır.

## Uygulanan sözleşme

`roi` v1, editor schema 7: gerekli `image: Image/px/image` girdisi ve tek
`region: Region/px/image` çıktısı. Toolbox Ön işleme grubundadır. `searchRoi`
piksel hizalı dikdörtgendir; `imageBinding` source/coordinates/width/height taşır.
Boş taslak kaydedilebilir; çizilmemiş, sınır dışı veya farklı kaynak/koordinat/boyutta
çalıştırma hata verir ve downstream sonuç üretmez. Circle, polygon, rotated veya
subpixel Region bu sürümde yoktur; ek parametreler reddedilir. Mevcut viewer'ın
rectangle çizimi/taşıma/boyutlandırma/Escape/Undo altyapısı kullanılır.

Region görüntünün rasterini veya processing kökenini değiştirmez; orijinal
FrameMetadata'yı taşır. Blob, Template Match ve eski Edge gerçek işlem alanını
bu porttan alır. Diğer araçlara yeni Region portu eklenmedi. Kaynak geometri
bağının gözlemi mevcut metadata ve raster boyutuyla sınırlıdır; aynı kimlik/boyutta
fiziksel optik değişiklik kabulü bu aracın görevi değildir. Cache imzaları ROI ve
bağ değişimini downstream'e taşır; Runtime immutable snapshot'ı korur.
