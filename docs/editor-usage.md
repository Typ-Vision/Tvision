# Recipe Editor kullanımı

Üç applicationType ortak Editor'ü kullanır. Reçete adı üst satırda kısaltılabilir;
tam adı tooltip ve erişilebilir metinde bulunur. Draft/dirty, tür ve sürüm ayrı
görünür. Save, Run Flow, Run Tool/Görüntü al, Stop ve Açıklamaları göster ana komut
satırındadır. Sağdaki **...** düğmesi Reçete bilgileri ve Çok kaynaklı Editor
geçişini açar. Geçiş bir reçete işlemidir; paneli açmak reçeteyi değiştirmez.

Viewer'da Open, kaynak seçici (çok kaynaklı reçetede) ve Fit vardır. Viewer **...**
menüsünde Reload, Zoom −/+, 1:1 ve Geçici geometri denemesi bulunur. Menü düğmelerine
Tab ve Space ile açılır; ok tuşları/Enter komut seçer, Escape menüyü kapatır. Görüntü/kaynak/frame kimliği
tek satırda kısaltılır; tooltip ve erişilebilir metinde tam bilgi korunur.
Sonuç/hata ayrıntıları Results'ta, kritik workspace hataları üstte görünür.
Run Tool da yürütme durumunu göstermek için Results'ı açar.

Toolbox/Properties arasındaki ve alt Flow/Results üzerindeki ayırıcılar
sürüklenerek boyutlandırılır. **View → Panel düzenini sıfırla** varsayılan
viewer öncelikli boyutlara ve sabit Workspace moduna döner. Yerel görünüm
tercihleri `config/local.ui.ini` içindedir; reçete hash'i veya Runtime snapshot'ı
değişmez. 850×600'de uzun adlar viewer alanını daraltmaz; Properties kaydırılabilir.

**Reçeteye bağlı ROI:** Toolbox → Ön işleme (PreProcess) → ROI ekleyin.
Görüntü kaynağını bağlayın, kaynağı/Preprocess'i çalıştırın ve **Dikdörtgen ROI çiz**
ile alanı sürükleyin. Alanı tutamaçlarla düzenleyebilirsiniz. Araç adını değiştirip
kaydedin. Blob veya Template Match'in Properties → `region` girdisinde bu aracın
`region` çıktısını seçin; tüketicinin Image girdisi aynı kaynağa ait olmalıdır.
Run Tool için önce güncel kaynak ve ROI çıktısı gerekir; Run Flow bağımlılıkları
çalıştırır. Kaydet/aç ve Runtime aynı alanı kullanır. Görüntü kırpılmaz, piksel
koordinatları değişmez. Kaynak/koordinat/boyut değişiminde yeniden çizim gerekir.
İlk sürüm yalnız piksel hizalı dikdörtgen üretir. Region girişi olmayan araçlarda
kendi ROI çizimini kullanın. Viewer menüsündeki geçici geometri reçeteye kaydedilmez.

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

## Line / Circle birim başlangıcı ve hata tanısı

Yeni Circle pixel modunda açılır. Circle birimi değişince eski ROI temizlenir;
özel sayısal şerit/fit/ürün sınırları otomatik çevrilmez. **Birim başlangıcını uygula** düğmesi scanWidth, inlierDistance, maxRms, maxResidual
ve unitsPerSample değerlerini açıkça px (1 / 1 / .35 / 1 / 1) veya mm
(.1 / .1 / .035 / .1 / .1) başlangıcına getirir. Bunlar fiziksel doğruluk
kabulü değildir; ürün toleranslarını seçili birimde ayrıca kontrol edin.

Line/Circle fit başarısızlığında `LINE_POINTS` / `CIRCLE_POINTS` bulunan kenar
sayısının yetersizliğini; `LINE_CONSENSUS` / `CIRCLE_CONSENSUS` bulunan
kenarların aynı geometriye yeterince uymadığını belirtir. Canlı hata mesajında
istenen/bulunan/gerekli ve (consensus için) katılan nokta sayıları, adayların
geçiş/eşik/sınır desteği ret sayıları ve adaysız tarama sayısı gösterilir.
Aday ret sayıları tarama sayısı değildir; bir taramada birden fazla aday olabilir.
Geçerli fit olmadığında ölçüm portu, PASS veya parça kararı üretilmez.
Editor ve Runtime aynı aşamaya göre çözüm önerisini gösterir. Edge Point'te
kenar geçişi/eşik/sınır desteği; Caliper'da komşu kenar, açık/koyu şerit ve aranan
genişlik; Geometry'de downstream refit ayrı açıklanır. Fit destek/kalite hatası ve
Circle kontur değerlendirmesinin yapılamaması ürün toleransı aşımından ayrıdır.
Öneri hiçbir ayarı otomatik değiştirmez. Kalibrasyon/bağlama gibi diğer hatalar
özgün mesajıyla korunur; bu hatalarda eşik gevşetme önerilmez.

Yeni Runtime kayıtları sınırlı tanı özetini gerçek kaynak/frame/cycle/evaluation
kimliğiyle saklar. Logs yeniden açıldığında aynı açıklama gösterilir; CSV'de
`measurementDiagnostic` satırı geçersiz scalar ve JSON neden hücresiyle taşınır.
Bu satır ölçüm veya PASS değildir. Stop ve parametre/kaynak değişimi canlı eski
tanıyı temizler; geçmiş kayıtları silmez. Eski kayıtlar okunur, eksik tanı türetilmez.

## Line / Circle sade ayarlar ve ön kontrol

Temel kullanımda birim, tarama yönü/sayısı, **Kenar geçişi**, **Kenar seçimi**
ve **Kenar eşiği** görünür. Geçiş ok yönünde Siyah → Beyaz / Beyaz → Siyah /
Her ikisi; seçim İlk / Son / En güçlüdür. Eşik siyah-beyaz görüntü eşiği değildir;
0–255 normalize profil değişimi / örnek koordinatı birimidir.

- **Gelişmiş tarama:** ortalama şerit kalınlığı ve gürültü yumuşatma. Şerit,
  kenar boyunca ortalamadır; ROI arama uzunluğu kenarı kestiği mesafedir.
- **Fit / kontur kalitesi:** fit'e katılma mesafesi, RMS, en büyük residual,
  nokta sayıları, ayrı fit/gözlem/destek yüzdeleri ve yay/boşluk sınırları.
- **Ürün sınırları:** Line destek uzunluğu ve yönlü açı; Circle çap ve yüzey
  sapması. Bu kontroller etkin kalır; paneli kapatmak değerlendirmeyi kapatmaz.
- **Detailed → Uzman örnekleme / solver:** örnek adımları, subpixel, sağlam fit
  override ve koşul sınırı. Basic/Detailed ve bölüm açılması reçeteyi değiştirmez.

Bölümler başlangıçta kapalıdır; tercihler yerel INI'dedir. Basic görünümde özel
ayar özeti korunmuş değerleri gösterir. Yüzdeler dosyada eski 0–1 oranlarıyla
saklanır. Alanları gizlemek veya birimleri etiketlemek eski ayarları sıfırlamaz.

Ön kontrol; tarama/nokta sayısı, nominal boşluk/yay desteği, Line nominal desteği,
şerit/ROI genişliği ve filtre/tepe için kısa arama desteğini açıklar. Uyarı ölçüm
sonucu değildir; çalıştırmayı veya kalite sınırlarını otomatik değiştirmez.
World/reference dönüşümünde nominal açıdan gerçek current world kalite kararı
çıkarılmaz. Görüntü alanı, kalibrasyon, iş bütçesi ve gerçek fit kapıları yürütmede
ayrıca denetlenir. Bu kontrolün uyarı vermemesi başarılı fit garantisi değildir.


## Kamera ve PGM+JSON edinimi

Editor'da kaynak için **Kamera ID / ad** seçin. Kayıtlı varsayılan kurulum/profil
bağlanır; seçim edinim başlatmaz. **Görüntü al / Run Flow** desteklenen Opto/OECS
kaynağının edinim yolunu application politikasıyla seçer. Ek bir mod veya oturum
onayı yoktur. FrameBurstStart / Software profil gerekir; eski FrameStart profili
sessiz çevrilmez. Profil değişiminden sonra kaynağı yeniden hazırlayın.

Camera Manager'ın kaydettiği `.pgm` + `.pgm.json` çiftini **Open** ile açın.
JSON, raster hash, kamera ve geometri uyuşmazlığı reddedilir. Yeniden adlandırmada
JSON'u yeni `.pgm.json` adına taşıyın. Save/reopen yolu korur; restart görüntü veya
Run başlatmaz. Kaynak değişimi eski kare/sonuç/cache'i temizler. **Run Tool** güncel
upstream sonucunu kullanır; TargetOnly yeni edinim yapmaz.

Raw/px kalibrasyonsuz çalışır; mm için aynı kaynağın Kalibrasyon'da kullanıma
alınmış uygun modeli ve güncel bağlantı/uyum gerekir. Canlı/PGM kökeni ve model
kimliği sonuçta korunur. Yeni model/ayar Runtime'a sessiz uygulanmaz: Stop ve
Load Recipe → Run gerekir. [Tam devreye alma akışı](camera-runtime-usage.md).

Bağımsız referans olmadan fiziksel doğruluk/PASS çıkarılmaz. Kenar/fit ayarını
sahneye göre inceleyin; toleransı sonucu geçirmek için gevşetmeyin. OECS native
üretim ve fiziksel kabul engelleri [progress'te](progress.md) açıktır.
