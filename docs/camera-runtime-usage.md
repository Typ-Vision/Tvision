# Kamera, kalibrasyon ve Runtime kullanımı

23 Ekim 2026 mevcut uygulama. [Güncel durum ve açık fiziksel kabul](progress.md).

## Kamera → Kalibrasyon → Editor → Runtime

1. Projeyi oluşturun/açın ve kaydedin; kamera ve kalibrasyon için reçete gerekmez.
2. **Setup → Camera Manager → Cihazları ara → seç → Ekle** ile kamerayı kaydedin.
   Kamera ID boşsa CAM_01 gibi benzersiz bir kısa ID atanır. Kayıt ve varsayılan
   kurulum/profil birlikte oluşturulur; aynı fiziksel kamera tekrar eklenmez.
   **ID / adı kaydet** metadata değiştirir, kamera/model bağlarını korur.
3. **Ayarları oku** desteklenen cihazın aralık, birim ve gerçek değerlerini getirir.
   Exposure/gain için **Uygula ve kaydet** gerçek readback sonrası kaydeder.
   Opto/OECS ayar yolu görüntü ediniminden bağımsızdır. Başarısız readback başarı
   sayılmaz. **Ayrıntılar → Optik / geometri taslağı** içinde gerçek lens/düzlemi
   tamamlayın; bilinmeyen değerler ölçüm izni vermez. Tam sensör Mono8 gerekir.
4. **Tek görüntü** bir kare alıp cihazı kapatır; **Canlı görüntü** Durdur'a kadar
   önizleme yapar. Normal komut desteklenen kamera için uygun edinim yolunu seçer;
   ayrıca kamera modu/onay kutusu yoktur. Gerçek FPS ve continuous/sıralı türü
   gösterilir; bu video dosyası kaydı değildir. Kamera/ayar değişimi canlıyken
   kilitlidir. **Görünen kareyi kanıtıyla kaydet** yeni `.pgm` + `.pgm.json` yazar.
   Stop/kopma/kamera değişimi kareyi emekliye ayırır ve bekleyen kaydı iptal eder.
5. **Setup → Calibration → kamera → Kalibre Et** ile **Bilinen ölçü** veya
   **Kalibrasyon plakası** seçin. Hazır varsayılan hedef otomatik aktarılır.
   Liste kamerayı açmaz; canlı sihirbaz açılışı hedefi arka planda hazırlar.
   Çekimler aynı bağlantıyı kullanır. Dosyaya geçiş/Stop/İptal ve başarılı
   Kaydet ve kullan bağlantıyı bırakır; çıkış kapanış tamamlanana kadar bekler.
   Hesap/kayıt hatasında yeniden denemek için bağlantı korunur.
   Görüntü alın, gerçek kenar/plakayı seçin, bilinen mesafe/aralığı girip hesaplayın.
   Üçgen işaretli koyu çerçeveli plaka için açık **Çerçeveli daireli plaka (7×7)**
   profilini seçin; merkez aralığını girip teyit edin. **Plakayı bul** ardından
   49 merkezi, P1 ve +X/+Y'yi inceleyin. Gerekirse **Arama alanı çiz** ile tüm
   çerçeve/işareti kapsayan alan seçin; zoom/arama rasteri kırpmaz. Algılama ve
   model hesabı ayrıdır: 49 merkez bulunup model reddedilebilir; bu durumda kayıt
   adımı açılmaz. Fit uyumu ve aralık teyidi bağımsız fiziksel doğruluk kabulü değildir.
   **Kaydet ve kullan** aday kaydı ve uygunluk/etkin seçimi tek komutta yapar;
   başarısızlık önceki modeli korur, kısmi kayıt tekrar denenebilir. Dosya kaynağı
   sihirbaz ayrıntılarındadır; hedefli dosya adayını kullanmak canlı uyum karesi
   ister. Dosya fiziksel edinim olarak yeniden etiketlenmez.
6. Kalibrasyon listesinde **Kalibrasyon yok**, **Kayıtlı / bağlantı ve uyum denetimi
   bekleniyor**, **Geçerli**, **Yeniden kalibrasyon gerekli** durumlarını okuyun.
   Restart kayıtları korur; etkin mm değerini/iznini kendiliğinden geri getirmez.
   Bağımsız fiziksel kontrol yapılmadıysa bu durum görünür. Dosya denemesi, geçmiş,
   legacy ilişkilendirme ve **Optik / kamera konumu değişti** Ayrıntılar'dadır;
   eski kalibrasyon ekranlarına ayrı geçiş yoktur. Robot yöntemi desteklenmez.
7. Yeni reçeteyi üç uygulama türünden biriyle açın. Boş reçete DRAFT'tır.
   Ortak Editor'da kaynak ekleyip **Properties → Kamera ID / ad** seçin; varsayılan
   kurulum/profil atomik bağlanır, seçim kare almaz. Açık reçete düzenleme komutu
   bağlı kameraları arka planda hazırlar. Sağlıklı bağlantı açık düzenleme boyunca
   korunur; **Save**, araç/ROI ve Properties sekmesi bağlantıyı kesmez.
   Kamera hazır değilse dosya düzenleme sürer; diğer hazır kaynaklar çekilebilir.
   **Görüntü al** başarısız hedefi açıkça yeniden dener; otomatik tekrar yoktur.
   **Stop** boşta da bağlantıyı bırakır; sonraki edinim yeniden hazırlayabilir.
   Kaynakları ve araçları bağlayın.
   **Görüntü al / Run Flow** normal komutları canlı veya PGM+JSON kökenini korur.
   Raw/px model istemez; mm aynı kaynağın uygun etkin modelini ister. Başka/global
   modele fallback yoktur. Kamera/seri değişimi açık yeniden seçim ister.
   Aynı kayıtlı setup/profile revision değişimi Editor bağlantısını yeniden hazırlar;
   eski kalibrasyon/model izni geri gelmez. Yeni ayar snapshot'ıyla reçete
   DRAFT/dirty olur; otomatik kaydedilmez. **Save** ile taslağı kaydedin.
8. **Load Recipe → Run → Tek çevrim** ile kaydedilmiş immutable sürümü yürütün.
   Load bağlantı açmaz; Run bütün kamera kümesini arka planda hazırlar.
   Hazırlık sonuç üretmez; tek çevrimler hazır bağlantıları kullanır.
   Stop gerçek kamera kapanışını bekler; disk kayıt retry kamerayı tutmaz.
   Kamera/model/ayar değişiminde **Stop → yeniden
   Load Recipe → Run** gerekir. Kaynak tablosu kamera, kalibrasyon ve birimi;
   ipuçları profil/köken ayrıntısını gösterir. Stop geç sonucu ve görüntüyü bastırır.
   OECS sonuçlarında parça kararı NOT_EVALUATED, PLC/robot Kullanılamıyor kalır.
9. **Logs**'ta normal ve eski/yeni OECS kayıtlarını seçin. Tek **Seçili kayıt türünü
   dışa aktar (CSV)** komutu seçilen türün exporter'ını kullanır; model ID/content/
   version, kaynak, birim ve köken korunur. Hata/Stop sayısal sonuç üretmez. Kayıt
   hatasında **Tekrar yaz** aynı donmuş sonucu kaydeder, yeniden edinim yapmaz.

Editor çıkışı önce mevcut Kaydet/Vazgeç/İptal kararını çözer; İptal bağlantıyı
korur. Kabul edilen sayfa/reçete/proje değişimi ve pencere kapanışı drain bekler; geç
sonuç başka hedefe taşınmaz. Restart/proje açılışı capture veya Run başlatmaz.
Eski çok profilli projede belirsiz varsayılan bir defa açıkça seçilip kaydedilir;
çevrimdışı kamera listede kalır. Webcam ayarları/önizlemesi Camera Manager'da
sunulmaz; Editor/Kalibrasyon yolunu kullanın. Küçük pencerede alt komutlara dikey
kaydırma ile erişilir.

OECS runtime/SDK yapılandırması ve donanım engelleri [tanı rehberindedir](windows-camera-diagnostics.md).
Yazılım fixture'ları bağımsız fiziksel mm doğruluğu, native üretim, lens/düzlem,
ışık/odak, tolerans, robot/PLC veya dağıtım kabulü değildir. Kullanıcı fiziksel
kabulü [CCUX-15](architecture/camera-setup/15-physical-acceptance.md) kapsamında açıktır.
Editor komutları ve ROI kullanımı [Editor rehberindedir](editor-usage.md).

Panel düzeni ve son seçimler `config/local.ui.ini` içindedir; bu proje autosave'i
değildir. Test INI, fixture, görüntü ve raporlar `build/` altında tutulur.
Sıfırlama: **View → Panel düzenini sıfırla**.

Workspace pin düğmesi veya **View → Workspace sabit** otomatik gizlemeyi açar.
Sol şerit hover/tıklama veya Tab ve Space ile açılır; içerik üzerinde yüzer.
Panelde klavye odağı varken açık kalır; Escape odağı şeride döndürür.
İlk kullanım ve panel sıfırlama sabit moddur.

