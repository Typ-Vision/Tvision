# Recipe Editor kullanımı

Üç applicationType ortak Editor'ü kullanır. Reçete adı üst satırda kısaltılabilir;
tam adı tooltip ve erişilebilir metinde bulunur. Draft/dirty, tür ve sürüm ayrı
görünür. Save, Run Flow, Run Tool/Görüntü al, Stop ve Açıklamaları göster ana komut
satırındadır. Sağdaki **...** düğmesi Reçete bilgileri ve Çok kaynaklı Editor
geçişini açar. Geçiş bir reçete işlemidir; paneli açmak reçeteyi değiştirmez.

Viewer'da Open, kaynak seçici (çok kaynaklı reçetede) ve Fit vardır. Viewer **...**
menüsünde Reload, Zoom −/+, 1:1 ve geçici ROI geometrisi bulunur. Menü düğmelerine
Tab ve Space ile açılır; ok tuşları/Enter komut seçer, Escape menüyü kapatır. Görüntü/kaynak/frame kimliği
tek satırda kısaltılır; tooltip ve erişilebilir metinde tam bilgi korunur.
Sonuç/hata ayrıntıları Results'ta, kritik workspace hataları üstte görünür.
Run Tool da yürütme durumunu göstermek için Results'ı açar.

Toolbox/Properties arasındaki ve alt Flow/Results üzerindeki ayırıcılar
sürüklenerek boyutlandırılır. **View → Panel düzenini sıfırla** varsayılan
viewer öncelikli boyutlara ve sabit Workspace moduna döner. Yerel görünüm
tercihleri `config/local.ui.ini` içindedir; reçete hash'i veya Runtime snapshot'ı
değişmez. 850×600'de uzun adlar viewer alanını daraltmaz; Properties kaydırılabilir.

Blob Search ROI'yi gövdeden taşıyın, sekiz tutamaçla boyutlandırın. Escape iptal,
Ctrl+Z geri alır; ROI dışı sol veya orta tuş pan yapar. Referans/region bağı
düzenlemeyi kilitler. “ROI sınırına temas edenleri reddet” kapalı başlar; açıldığında
sınıra değen bağlı bölgeler sayım ve ölçümden çıkarılır.

**Açıklamaları göster** kapalı başlar ve yerel olarak hatırlanır. Sonuç yazıları
zoom ile küçülür; büyütme 1,5× ile sınırlıdır. Geometri, maskeler ve Results bilgisi
görünür kalır. Runtime ve Calibration viewer'ları bu tercihten bağımsızdır.

Toolbox başlıklarını tıklayarak veya Enter/Space ve sağ/sol oklarla açıp kapatın.
Arama eşleşen grubu geçici açar; temizlenince yerel grup tercihi geri gelir.
Başlık araç eklemez. Yeni kenar aracı Edge Point'tir; eski Edge reçeteleri korunur.
Dar panelde kısaltılan araç adının tamamı tooltip'te bulunur.

Line Fit ve Circle/Arc seçildiğinde Run öncesinde de tarama sayısı kadar nominal
ok çizilir. Circle Basic'teki Start/Sweep geçerli değişimde yayı önizler; düzenleme
bitince ROI kaydedilir. Yeni fit sonucu için Run gerekir. Nominal kılavuz sonuç
değildir; gerçek sonuç olmadan PASS, skor veya robot pose gösterilmez.
