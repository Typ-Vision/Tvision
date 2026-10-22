# CCUX-20 — büyük görüntüde Editor tepkiselliği

22 Ekim 2026 kullanıcı önceliği; CCUX-15 fiziksel kabulünden önce.

## Oku

README, progress, bu indeks ve contract C2/C4/C6; ImageFrame, ImageViewer,
EditorSession evaluationScope/reconcile ve MainWindowExecution yenileme yolu.

## Kapsam ve kabul

- Küçük raster ve gerçek kamera boyutunda 2448×2048 Mono8 ile aynı ölçümü
  düzeltme öncesi/sonrası Release üzerinde yap. Decode/preview oluşturma,
  tekrar contentId ve tekrar QPixmap yüklemesini ayır.
- Immutable raster kimliği bir kez hesaplanır; metadata kopyaları aynı kimliği
  paylaşır. Mevcut hash formatı, stride/piksel sahipliği, processing kökeni ve
  cache invalidation korunur; worker/UI arasında mutable lazy cache kullanılmaz.
- Değişmeyen preview yeniden QPixmap oluşturmaz; aynı boyutta farklı kare,
  clear/reload, zoom ve ROI davranışı korunur. Ölçüm rasteri küçültülmez.
- Release build; image_refresh_contract, owned_frame_contract, session/cache,
  preprocess ve ilgili Editor/ROI CTest; normal/küçük görsel kontrol.
- Fiziksel kamera ile kullanıcı etkileşim gecikmesi ayrıca kabul edilir;
  sentetik raster mikro ölçümü tüm uygulamanın uçtan uca süresi değildir.

## Sonraki iş

[CCUX-21](21-preprocess-roi.md): PreProcess altında işlevsel ROI aracı.
CCUX-15 fiziksel mm/kurulum ve native edinim engelleri açık kalır.
