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

## EUI kullanıcı tekrar testi

Uygulamayı repo kökünden `.\scripts\run-local.ps1` ile başlatın. EXE:
`C:\Projects\Tvision\build\reference-ui\visionflow.exe`. ExecutionPolicy engeli
varsa yalnız başlatılan alt süreç için şu komutu kullanın:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "C:\Projects\Tvision\scripts\run-local.ps1"
```

1400×900 ve 850×600 pencere boyutlarında gerçek bir görüntüyle deneyin:

1. Blob ROI'yi taşıyın, köşe/kenarlardan boyutlandırın; Escape ve Ctrl+Z'yi deneyin.
   Sınır filtresini açıp Run Tool yapın: sınıra değen nesne elenmeli; kaydet/aç
   sonrası ROI ve checkbox korunmalı.
2. Workspace pin/otomatik gizleme, hover, Escape ve View → Panel düzenini sıfırla
   yollarını deneyin. Komutlara, Properties'e ve viewer'a küçük ekranda erişin.
3. Toolbox gruplarını kapatıp arama yapın, temizleyin ve araç ekleyin. Yeni listede
   Edge Point bulunmalı; eski Edge içeren reçete açılıp çalışabilmeli.
4. Line Fit, Circle, iki nokta ölçümü ve Blob sonuçlarında açıklamaları açıp
   kapatın; %10/%25/%100/%400 zoom deneyin. Geometri ve Results görünür kalmalı.
5. Circle'da tarama sayısını 10 yapın; iç/dış yönü ve 360°/180°/90° yayları deneyin.
   Line Fit yönünü de kontrol edin. Oklar Run öncesinde görünmeli; değişiklikten
   sonra eski fit kaybolmalı, yeni sonuç yalnız Run ile oluşmalı.

18 Ekim 2026: kullanıcı bu tekrar testlerini gerçekleştirdiğini ve hepsinin
başarılı olduğunu bildirdi. [Kabul kaydı](archive/2026-10-18-eui-accept-01.md).
Native DPI/çoklu monitör ve fiziksel kamera/kalibrasyon kabulü ayrı kontrollerdir.
