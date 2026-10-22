# Kamera merkezli kalibrasyon ekranı — CCUX-09 / CCUX-18

Setup → Calibration aynı `MeasurementWorkspace` kamera registry'sini kullanır.
Kaydedilmiş proje yeterlidir; reçete gerekmez. Kamera listesi ve altındaki seçili
kamera özeti yöntemi, kayıt tarihini, kullanım durumunu ve yalnız bu oturumda
uyumlu etkin modelin temel değerlerini gösterir. Durumlar: Kalibrasyon yok,
Kayıtlı / bağlantı ve uyum denetimi bekleniyor, Geçerli, Yeniden kalibrasyon gerekli.
Restart kayıtlı seçimi korur, kullanım izni oluşturmaz.
Uyumsuz modelin eski değerleri ana ekranda etkin değer olarak sunulmaz.

0 kamerada dosyadan deneme açılabilir; 1 kamerada kart şeridi gizlenir, 5/10
kamerada kartlar genişliğe göre sarılır ve dikey kaydırılır. Kart/klavye seçimi
keşif veya edinim yapmaz. Uzun kimliğin tamamı seçili kamera özetinde kalır.

**Kalibre Et** → **Bilinen ölçü** veya **Kalibrasyon plakası**. Robot seçeneği
"Henüz desteklenmiyor" olarak kapalıdır. Application katmanının Ready ve tek
anlamlı varsayılan kamera/setup/profile kopyası sihirbaz hedefidir. Pending veya
belirsiz varsayılan tamamlanmadan normal edinim açılmaz. Profil/kurulum normal
akışta tekrar sorulmaz. Görüntü → gerçek kenar/plaka → fiziksel mesafe/aralık →
hesapla/kontrol → **Kaydet ve kullan** mevcut matematiği kullanır.

Son düğme [tek application komutunu](measurement-setup-migration.md) çağırır.
Ready varsayılan kurulumun operasyonel seçimi, adayın etkin seçimiyle aynı son
registry commit'inde yapılır. Hata bunu veya önceki modeli değiştirmez. Sonuç
"Kaydedildi ve bu oturumda kullanıma hazır", "Aday kaydedildi; kullanıma alınamadı"
veya ret olarak görünür. Kısmi kayıt tekrar denenebilir; aynı aday/hash korunur.
Başarıdan sonra Bitir kapanışı vardır. Bağımsız fiziksel kontrol yokluğu açık kalır.

Dosya adayı kökenini korur; Kaydet ve kullan ayrıca gerçek canlı uyum probe'u
ister. Dosya tek başına fiziksel edinim veya aktivasyon probe'u değildir.
OECS edinimini normal komutların application politikası seçer; ayrıca mod/onay
seçimi yoktur. Typed edinim kanıtı kayıtta korunur. Tetik ilişkilendirmesi, fiziksel
mm kabulü veya üretim yetkisi verilmez.

**Ayrıntılar** geçmiş, dosya denemesi, legacy ilişkilendirme/aktivasyon incelemesi,
çok kurulumlu eski projelerin açık seçimi, import/export/Save As, arşivleme ve
**Optik / kamera konumu değişti** bildirimini taşır. Sihirbazın dosya ayrıntıları
trial veya açık kurulum beyanını korur; trial yalnız aday kaydeder. Kalibrasyon
yalnız bu kamera merkezli ana sayfayı üretir; eski ana sayfa, kanıt ekranı, stack
ve gezinme widget'ları yoktur. Kamera/proje/politika değişimi
ve pencere kapanışı işi iptal eder; geç capture/model sonucu başka hedefe taşınmaz.
UI algoritma veya SDK çağırmaz. Runtime/worker/pending kayıt/ACK gate'i korunur.

Kabul suite'leri ve yeni offscreen görseller `build/ccux/ccux-09/` altındadır.
1400×900 ve 850×600 görüntüleri native DPI, fiziksel kamera veya mm kabulü değildir.
