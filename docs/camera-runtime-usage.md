# Kamera, kalibrasyon ve Runtime kullanımı

21 Ekim 2026 mevcut uygulama. [CCUX hedef akışı](architecture/camera-setup/README.md)
kartlar halinde uygulanır; güncel tamamlanma durumu [devir özetindedir](progress.md).


1. Project Manager'dan proje oluşturun/açın ve kaydedin. Recipe Manager'dan reçete
   oluşturun; kaynak ve araçları ortak Editor'da bağlayın. Save taslağı kaydeder.
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
   Native settings backend henüz Unavailable olduğundan fiziksel cihazda bu
   düğmeler destek varmış gibi açılmaz. Hatalı ayar başarı diye gösterilmez.
   **Ayrıntılar** yerel producer/rota/timeout ve optik/geometri taslağını taşır.
   Birden fazla uygun rota veya eski profil belirsizliğinde seçim görünür açılır;
   eski varsayılan profil ayrıca kaydedilir. Bilinmeyen lens/düzlem ölçüm izni vermez.
   Küçük pencerede alt işlemlere dikey kaydırma ile ulaşılır.
   **Setup → Calibration** aynı kamera kayıtlarını kullanır. Kalibrasyonda kayıtlı edinim
   profilini seçin; aday kaydı etkin model değildir. Aktivasyon ve kaynak→kurulum bağı ayrıdır.
   Editor’da Industrial Camera ekleyin, Properties üzerinden kayıtlı profili seçip
   **Profili bağla / güncelle** düğmesine basın. **Görüntü al** yeni kare ister; seçim
   tek başına edinim yapmaz. Profil düzenlenirse açıkça yeniden bağlayın.
   Mevcut COE-050-M-USB-050-IR-C için Camera Manager’da **Deneysel OECS edinimini
   kullan** seçimi ve kurulu x64 runtime ile **Önizleme** açılır. Mevcut **Önizleme** korunur; yeni tek/canlı kontrol akışı sonraki CCUX
   kartlarındadır. Tam sensör Mono8 ve tamamlanmış açık optik/geometri taslağı gerekir; SDK derlemesi [tanı rehberindedir](windows-camera-diagnostics.md).
   **Görünen kareyi kanıtıyla kaydet** yeni `.pgm` ve `.pgm.json` yazar.
   Tetik eşleşmesi doğrulanmamıştır; Stop kareyi temizler. Tercih restart’ta kapalıdır.
   Editor üstündeki **Deneysel OECS kamera / dosyayı Editor'da kullan** seçimi
   canlı FrameBurstStart edinimini ve PGM+JSON replay’i açar; restart’ta kapanır.
   Kullanım ve sınırlar [Editor rehberindedir](editor-usage.md).

4. Kaydedilmiş reçeteyi **Load Recipe → Run** ile Runtime'a hazırlayın.
   **Tek çevrim** gerçek edinim/yürütmeyi başlatır; Run tek başına sonuç üretmez.
   Stop eski sonucu bastırır; farklı sürüm için Stop → Load Recipe → Run kullanın.
   Deneysel OECS için Runtime’da **Deneysel OECS Runtime / replay (Raw / px)**
   seçimini Load öncesi açın; restart’ta kapalıdır. PLC/robot çıkışı engellidir.
   [Kapsam ve kayıt sözleşmesi](architecture/experimental-runtime.md).
5. Kalıcı sonuçları **Logs/CSV** üzerinden inceleyin. Kayıt hatasında tekrar yazma
   aynı sonucu kaydeder; yeni acquisition yapmaz. Restart geçmişi korur, aktif
   üretim iznini geri yüklemez. Deneysel geçmişin CSV düğmesi ayrıdır.

Editor komutları, ROI, nominal tarama kılavuzları ve küçük ekran kullanımı
[Editor kullanım rehberindedir](editor-usage.md).

Panel düzeni ve son seçimler `config/local.ui.ini` içindedir; bu proje autosave'i
değildir. Test INI, fixture, görüntü ve raporlar `build/` altında tutulur.
Sıfırlama: **View → Panel düzenini sıfırla**.

Workspace pin düğmesi veya **View → Workspace sabit** otomatik gizlemeyi açar.
Sol şerit hover/tıklama veya Tab ve Space ile açılır; içerik üzerinde yüzer.
Panelde klavye odağı varken açık kalır; Escape odağı şeride döndürür.
İlk kullanım ve panel sıfırlama sabit moddur.

