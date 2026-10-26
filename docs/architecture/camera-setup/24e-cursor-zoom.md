# CCUX-24E — imleç merkezli ortak zoom

Durum: yazılım kabulü tamamlandı; [kapanış](../../archive/2026-10-26-ccux-24e.md).
Native wheel/DPI/çok monitör kabulü açık; Windows capture aracı yanıt vermedi. [Ortak plan](24-usability-plan.md) ve
[analiz](24-usability-analysis.md) 9 okunur. Sonraki: 24F.

## Hedef ve başlangıç

Wheel zoom, olay anındaki viewport konumunu esas alır. Çizim açıkken, pan sonrası
ve ilk wheel'de aynı kural geçerlidir. Düğme zoom'u viewport merkezini kullanır.
Başlangıç: `src/app/ImageViewer.cpp/.hpp`, `ImageViewerRoi.cpp` ve ilgili testler.
Çağıran ekranlarda Fit/setImage davranışı ayrıca izlenir; kaynağı değişen görüntü
ile aynı kaynaktan yeni kare ayrılır.

## Uygulama adımları

1. Mevcut sol-alt kayma bildirimini olay konumu, transform, scrollbar ve
   mapToScene ölçümleriyle yeniden üret. Çizim/ROI callback'leri temel event'i
   atlıyor mu doğrula. Yeniden üretilemiyorsa bunu kaydet; nedeni kesinleştirme.
2. Wheel'den önce imleç altındaki scene noktasını al; geçerli zoom sınırı içinde
   ölçekle, sonra o noktanın aynı viewport konumunda kalması için pan düzeltmesi yap.
   AnchorUnderMouse ile manuel telafinin iki kez uygulanmasını önle.
3. Boş görüntü, zoom alt/üst sınırı, sıfır wheel ve sınırda scrollbar durumunu
   tanımla. Geometri izin vermiyorsa en yakın geçerli konuma sınırla; ölçümleri
   veya görüntü koordinatını değiştirmek için rasterı kaydırma.
4. Fit/actualSize/resize açık komutlarını koru. Aynı kaynak ve geometride ardışık
   kareler manuel pan/zoom'u korusun; ilk görüntü veya kaynak/geometri değişimi
   güvenli Fit'e dönsün. Bu politika tüm ilgili çağıranlarla doğrulansın.
5. Overlay/ROI koordinatları ve hit-test aynı scene uzayında kalsın; wheel
   reçete edit'i veya yeni edinim tetiklemesin.

## Kabul

Merkez/dört köşe; çizim kapalı/açık/tutamaç üzerinde; middle pan sonrası; Fit'ten
ilk wheel; scrollbar var/yok; görüntü viewport'tan küçük/büyük; iki pencere
boyutu. Geometri elverdiğinde imleç altındaki nokta sapması en fazla 1 viewport
pikselidir. Tekrarlı in/out birikimli köşe kayması oluşturmaz. Sınırlandırılmış
durumlar ayrı raporlanır; başarısız ölçüm genel toleransa gizlenmez.

Editor, Kalibrasyon ve Runtime'da gerçek wheel etkileşimi kontrol edilir.
Camera Manager ortak viewer'a 24F'de geçerse aynı matris orada tekrar ilgili
kapsamda uygulanır. Native DPI/çok monitör yapılmadıysa açık kalır.

Testler: `roi_viewer_interaction`, `image_refresh_contract`; olay konumuna dayalı
ölçüm ve kare yenileme regresyonları eklenir. Release + normal/küçük görsel ve
etkileşim kanıtı `build/ccux/ccux-24e/`. Ortak kapanış uygulanır.
Commit: `CCUX-24E: anchor wheel zoom at the event position`.
