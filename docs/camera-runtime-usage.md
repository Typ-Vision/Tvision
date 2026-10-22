# Kamera, kalibrasyon ve Runtime kullanımı

22 Ekim 2026 mevcut uygulama. [CCUX hedef akışı](architecture/camera-setup/README.md)
kartlar halinde uygulanır; güncel tamamlanma durumu [devir özetindedir](progress.md).

## Kısa devreye alma

1. Projeyi oluşturup kaydedin; önce reçete oluşturmanız gerekmez.
2. Setup → Camera Manager → Cihazları ara → kamerayı seç → Ekle. Kamera ID'sini
   kontrol edin; destek varsa exposure/gain readback'ini okuyun. Tek/Canlı görüntü
   ile sahneyi kontrol edin, Durdur ile edinimi kapatın.
3. Setup → Calibration → aynı kamera → Kalibre Et. Bilinen ölçü veya plaka
   yöntemiyle gerçek raster üzerinde hesaplayın; Kaydet ve kullan sonucunu okuyun.
   Deneysel ve bağımsız fiziksel kontrol durumları ayrı görünür.
4. Yeni reçeteyi üç uygulama türünden biriyle açın. Editor'da kaynak ekleyip
   Kamera ID / ad seçin; kaynak ve ölçüm araçlarını bağlayıp Run Flow çalıştırın.
   Raw/px kalibrasyon istemez; mm aynı kaynağın uygun modelini gerektirir. Kaydedin.
5. Load Recipe → Run → Tek çevrim; gerçek sonucu Logs ve uygun CSV düğmesinden
   inceleyin. Model/ayar değişiminde Stop → yeniden Load Recipe gerekir. Deneysel
   onay oturumluktur; deneysel sonuç PLC/robota gönderilmez.

Kayıtlı / bağlantı bekleniyor, yeniden kalibrasyon gerekli veya eksik kurulum
durumunda etkin mm değeri beklemeyin. Yazılım fixture kabulü bağımsız gerçek mm
doğruluğu kabulü değildir; fiziksel devreye alma ayrıca tamamlanır.

## Ayrıntılı kullanım

1. Project Manager'dan proje oluşturun/açın ve kaydedin. Recipe Manager'dan reçete
   ad/açıklama ve uygulama türüyle oluşturun. Kamera seçmeden boş DRAFT açılır;
   kaynak ve araçları ortak Editor'da bağlayın. Bilgileri düzenle yalnız ad/açıklamayı
   değiştirir; mevcut kamera/kalibrasyon bağını korur. Projedeki eski genel referans
   metinleri Ayrıntılar'dadır ve yeni reçeteye aktarılmaz. Save taslağı kaydeder.
2. Image File için gerçek görüntü açın; kamera kaynağında edinim yapın. **Run Flow**
   akışı yürütür. **Run Tool** güncel girdilerle seçili aracı çalıştırır; eksik
   upstream sonucu veya görüntü yerine değer üretmez.
3. **Setup → Camera Manager** içinde **Cihazları ara → seç → Ekle** ile kamerayı
   kaydedin. Kamera ID boşsa CAM_01 gibi benzersiz bir ID atanır; açıklama/ad
   isteğe bağlıdır. Kayıt ve varsayılan kurulum/profil birlikte oluşturulur; reçete
   gerekmez. **ID / adı kaydet** bağları koruyarak metadata günceller. Kayıtlı kamera
   taramada tekrar eklenmez, çevrimdışıyken listede kalır.
   **Ayarları oku** desteklenen backend'den başlangıç, aralık ve birimleri getirir;
   exposure/gain için **Uygula ve kaydet** gerçek readback doğrulanınca kaydeder.
   COE-050-M-USB-050-IR-C ve kurulu OECS runtime için ayar okuma/yazma native
   görüntü ediniminden bağımsızdır; deneysel görüntü onayı gerekmez. Kayıtlı
   satırda Ekle yerine **Kamera kayıtlı** görünür. Webcam ayarları ve bu sayfada
   webcam önizlemesi desteklenmez; Editor/Kalibrasyon edinim yolu kullanılır.
   Hatalı ayar başarı diye gösterilmez; erişim/aralıklar gerçek cihazdan gelir.
   **Ayrıntılar** yerel producer/rota/timeout ve optik/geometri taslağını taşır.
   Birden fazla uygun rota veya eski profil belirsizliğinde seçim görünür açılır;
   eski varsayılan profil ayrıca kaydedilir. Bilinmeyen lens/düzlem ölçüm izni vermez.
   Yeni Opto kaydında önce ayarları okuyup kaydedin, ardından **Optik / geometri
   taslağı** içindeki gerçek lens ve çalışma düzlemini tamamlayın. Okunan çözünürlük
   taslağa taşınır; taslak kaydı fiziksel doğrulama değildir. Varsayılan işlem bütçesi
   5 saniyedir. Native üretim edinimi kapalı kalır.
   Küçük pencerede alt işlemlere dikey kaydırma ile ulaşılır.
   **Setup → Calibration** aynı kamera kayıtlarını kullanır. Kamera → **Kalibre Et**
   → **Bilinen ölçü** veya **Kalibrasyon plakası** seçin. Hazır varsayılan hedef
   otomatik geçer; görüntü alıp gerçek kenar/plakayı seçin, fiziksel mesafe/aralığı
   girin, hesaplayıp **Kaydet ve kullan** komutunu uygulayın. Kısmi aday kaydı ve
   ret açık görünür; önceki seçim korunur. Dosya kullanımı Ayrıntılar'dadır ve
   etkinleştirmek ayrıca canlı uyum karesi ister. Deneysel OECS için bu ekranda
   açık oturum onayı gerekir; sonuçta Deneysel ve bağımsız kontrol yokluğu görünür.
   Geçmiş/legacy eşleme ve **Optik / kamera konumu değişti** Ayrıntılar'dadır.
   Kayıtlı / bağlantı bekleniyor durumu kalibrasyon yok anlamına gelmez. Kaynak bağı
   ve Runtime hazırlığı ayrı kalır.
   Editor’da Industrial Camera ekleyin, Properties → **Kamera ID / ad** seçicisinden
   kamerayı seçin. Varsayılan kurulum/profil birlikte bağlanır; seçim kare almaz.
   Aynı kameranın exposure/gain snapshot’ı Run Flow hazırlığında yenilenir ve
   reçete DRAFT/dirty olur; diske kendiliğinden yazılmaz. Geometri/seri veya
   varsayılan profil kimliği değişirse açık yeniden seçim gerekir. Dosya/Webcam
   kaynaklarında aynı seçici vardır; eski açık kurulum bağı Ayrıntılar’dadır.
   Raw/px model istemez; mm yalnız ilgili kaynağın uygun etkin modelini kullanır.
   Deneysel mm ayrıca kalibrasyonda etkin oturum izni ve Editor onayı ister;
   canlı veya PGM+JSON kökeni sonuç/cache üzerinde korunur.
   Mevcut COE-050-M-USB-050-IR-C için Camera Manager’da **Deneysel OECS edinimini
   kullan** seçimi ve kurulu x64 runtime ile **Tek görüntü** veya **Canlı görüntü** açılır.
   Tek görüntü bir kareden sonra cihazı kapatır; canlı görüntü Durdur’a kadar sürer.
   Durumda gerçek tamamlanma FPS’si ve continuous/sıralı edinim türü gösterilir;
   bu bir test önizlemesidir, video dosyası kaydı değildir. Kamera/ayar değişimi
   canlıyken kilitlidir. Sayfa/proje değişimi ve pencere kapanışı önce session’ı
   durdurup bekleyen çağrının kapanmasını bekler; UI bu sırada çalışmaya devam eder.
   Tam sensör Mono8 ve tamamlanmış açık optik/geometri taslağı gerekir; SDK derlemesi [tanı rehberindedir](windows-camera-diagnostics.md).
   **Görünen kareyi kanıtıyla kaydet** yeni `.pgm` ve `.pgm.json` yazar.
   Dosya seçimi sırasında görünen kare sabit kalır; Stop/kopma/kamera değişimi
   kareyi emekliye ayırır ve kaydı iptal eder. Tetik eşleşmesi doğrulanmamıştır;
   Stop kareyi temizler. Tercih restart’ta kapalıdır.
   Editor üstündeki **Deneysel OECS kamera / dosyayı Editor'da kullan** seçimi
   canlı FrameBurstStart edinimini ve PGM+JSON replay’i açar; restart’ta kapanır.
   Kullanım ve sınırlar [Editor rehberindedir](editor-usage.md).

4. Kaydedilmiş reçeteyi **Load Recipe → Run** ile Runtime'a hazırlayın.
   **Tek çevrim** gerçek edinim/yürütmeyi başlatır; Run tek başına sonuç üretmez.
   Stop eski sonucu bastırır; farklı sürüm için Stop → Load Recipe → Run kullanın.
   Deneysel OECS için Runtime’da **Deneysel OECS Runtime / replay (Raw / px +
   kaynak başına mm)** seçimini Load öncesi açın; restart’ta kapalıdır.
   Kaynak tablosu kamera ID/adını, kalibrasyon durumunu ve birimini gösterir;
   profil ve köken ayrıntıları hücre ipuçlarındadır. mm yalnız aynı kaynağın uygun
   deneysel modeliyle çalışır. Model/ayar değişiminde Stop → yeniden Load Recipe
   → Run kullanın. Parça NOT_EVALUATED kalır; PLC/robot çıkışı engellidir.
   [Kapsam ve kayıt sözleşmesi](architecture/experimental-runtime.md).
5. Kalıcı sonuçları **Logs/CSV** üzerinden inceleyin. Kayıt hatasında tekrar yazma
   aynı sonucu kaydeder; yeni acquisition yapmaz. Restart geçmişi korur, aktif
   üretim iznini geri yüklemez. Deneysel geçmişin CSV düğmesi ayrıdır; model
   kimliği/content/version, kaynaklı gerçek sonuç/birim ve köken kanıtı içerir.
   Hata ve Stop satırları sayısal sonuç üretmez.

Editor komutları, ROI, nominal tarama kılavuzları ve küçük ekran kullanımı
[Editor kullanım rehberindedir](editor-usage.md).

Panel düzeni ve son seçimler `config/local.ui.ini` içindedir; bu proje autosave'i
değildir. Test INI, fixture, görüntü ve raporlar `build/` altında tutulur.
Sıfırlama: **View → Panel düzenini sıfırla**.

Workspace pin düğmesi veya **View → Workspace sabit** otomatik gizlemeyi açar.
Sol şerit hover/tıklama veya Tab ve Space ile açılır; içerik üzerinde yüzer.
Panelde klavye odağı varken açık kalır; Escape odağı şeride döndürür.
İlk kullanım ve panel sıfırlama sabit moddur.

