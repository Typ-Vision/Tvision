# Kamera, kalibrasyon ve Runtime kullanımı

21 Ekim 2026 mevcut uygulama. [CCUX hedef akışı](architecture/camera-setup/README.md)
henüz uygulanmadı; bu rehber her ilgili UI tesliminde güncellenir.


1. Project Manager'dan proje oluşturun/açın ve kaydedin. Recipe Manager'dan reçete
   oluşturun; kaynak ve araçları ortak Editor'da bağlayın. Save taslağı kaydeder.
2. Image File için gerçek görüntü açın; kamera kaynağında edinim yapın. **Run Flow**
   akışı yürütür. **Run Tool** güncel girdilerle seçili aracı çalıştırır; eksik
   upstream sonucu veya görüntü yerine değer üretmez.
3. **Camera Manager** ile kamerayı projeye kaydedin; yerel producer hatırlanır.
   **Setup → Calibration** aynı kamera kayıtlarını kullanır. Kalibrasyonda kayıtlı edinim
   profilini seçin; aday kaydı etkin model değildir. Aktivasyon ve kaynak→kurulum bağı ayrıdır.
   Editor’da Industrial Camera ekleyin, Properties üzerinden kayıtlı profili seçip
   **Profili bağla / güncelle** düğmesine basın. **Görüntü al** yeni kare ister; seçim
   tek başına edinim yapmaz. Profil düzenlenirse açıkça yeniden bağlayın.
   Mevcut COE-050-M-USB-050-IR-C için Camera Manager’da **Deneysel OECS edinimini
   kullan** seçimi ve kurulu x64 runtime ile **Önizleme** açılır. Tam sensör
   Mono8 gerekir; SDK derlemesi [tanı rehberindedir](windows-camera-diagnostics.md).
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

