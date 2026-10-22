# CCUX-21 — PreProcess altında reçeteye bağlı ROI

Durum: planlandı, uygulanmadı. Kullanıcı isteği: 22 Ekim 2026.
Önce CCUX-20 performans düzeltmesi ve gerçek kamera etkileşim teyidi.

## Mevcut durum

MainWindowEditor'daki üst menü “ROI geometrisi…” geçici subpixel geometriyi
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
