# Kamera ve kalibrasyon sadeleştirme — taslak

21 Ekim 2026 — **CCUX-PLAN-01**. Genel kullanıcı akışı ve yeni reçeteden iki
referans alanının kaldırılması kullanıcı yönlendirmesidir. Robot yöntemi ve
aşağıdaki uygulama sırası değerlendirme önerisidir; detaylı görev planı veya
çalışan özellik değildir. Önceki geliştirme sıraları yerine güncel devam noktası
[progress](../progress.md) üzerinden izlenir.

## Kabul edilen ürün yönü

- Genel makine görüş ürünü; defect, robot guidance ve ölçüm aynı Editor/Runtime'ı
  kullanır. Bir projede tek veya çok kamera olabilir; beş kamera üst sınır değildir.
- Setup → Kameralar: tara, fiziksel kamerayı seç, kullanıcı ID/adı ve desteklenen
  başlangıç exposure/gain değerleriyle ekle. Tek görüntü / canlı görüntü / durdur
  isteğe bağlı testtir. Fiziksel kimlik tarama sırasından bağımsız tutulur.
- Ana ekranda kurulum/profil taslağı oluşturma ve profil seçme zorunluluğu kalkar.
  Sabit makine kurulumu varsayılır; gerekli setup/profile kayıtları içeride
  oluşturulur. Teknik bağlantı ayrıntıları gerektiğinde açılır.
- Setup → Kalibrasyon aynı kamera listesini kullanır. Seçili kameranın aktif
  değerleri veya “Kalibrasyon yok” gösterilir. Kalibre Et: bilinen ölçü, plaka,
  robot kalibrasyonu. Kaynak seçili kameradır; reçete/profil tekrar sorulmaz.
- Kaydet ve kullan, kontrol/aktivasyon işlemlerini tek kullanıcı akışında toplar;
  başarısız kontrol önceki modeli silmez. İçeride aday/aktif/kabul ayrımı korunur.
- Editor kamera ID'sini seçer; ilgili ayarlar/model otomatik çözülür. Çok kaynaklı
  reçetede her kaynak kendi kamera bağına sahiptir. Raw/px işi kalibrasyonsuz
  çalışabilir; mm ve robot sonuçları uygun bağımlılıkları gerektirir.
- Restart kayıtları hatırlar, uygunluğu/bağlantıyı kontrol eder; üretim kendiliğinden
  başlamaz. Exposure/gain değişimiyle optik/geometri değişimi ayrıştırılacaktır;
  mevcut activation kuralları bugün değişmiş değildir.

## Yeni reçete alanlarının kaldırılması

Yeni ve düzenlenen reçetenin genel bilgi penceresinden “Görüntü kaynağı referansı”
ve “Kalibrasyon referansı” kaldırılır. Ad, açıklama ve uygulama türü kalır.
Kamera bağlantısı Editor kaynak düğümünde yapılır; kalibrasyon o kaynaktan çözülür.
Kaynak henüz eklenmemiş reçete oluşturulabilir ve DRAFT kalır.

Bu yalnız iki widget silme işi değildir: `MainWindowWorkspace.cpp` içindeki
create/edit çağrıları, ProjectService varsayılanları, proje düzeyinden referans
mirası ve legacy ProductionRecipeService kontrolü birlikte incelenir. Eski
reçetelerin alanları aç/kaydet sırasında silinmez; uyumluluk/migration açıkça
tanımlanır. Aktif Runtime snapshot'ı arka planda değişmez. Yeni reçetede eski
global metin referansları gizlice geri doldurulmaz. Kopyala/import/düzenle,
çok-kamera, kalibrasyonsuz px ve kaynaktan mm çözümleme kabul kapsamındadır.

## Plaka ile ölçüm ve robot kalibrasyonunun ayrımı

| Seçenek | Amaç | Sonuç |
| --- | --- | --- |
| Bilinen ölçü | Uygun düzlemde ölçek | Ölçüm modeli |
| Kalibrasyon plakası ile | Seçilen çalışma düzleminde metrik ölçüm | Düzlem eşlemesi; gerektiğinde bağlı kamera modeli |
| Robot kalibrasyonu | Kameranın robotla koordinat ilişkisi | Eye-to-Hand veya Eye-in-Hand dönüşümü |

Aynı plaka bulma, kamera iç kalibrasyonu ve görüntü edinim servisleri paylaşılır;
kullanıcı iki ayrı sihirbazı tekrar etmek zorunda bırakılmaz. Kamera iç modeli,
ölçüm düzlemi ve robot dönüşümü birbirini ezmeyen, bağlı kayıtlardır.
Robot sihirbazındaki başlangıç plakası çalışma düzlemiyle aynı değilse mevcut
ölçüm kalibrasyonu yerine sessizce etkinleştirilmez.

## Robot için önerilen kaba akış

1. **Yerleşim:** sabit kamera + robot ucunda rijit plaka = Eye-to-Hand;
   robot üzerinde kamera + ortamda sabit plaka = Eye-in-Hand. Seçili kamera
   taşınır, robot ve poz biçimi belirlenir. İlk uygulama önerisi Eye-to-Hand.
2. **Başlangıç görüntüsü:** plaka görünür ve yaklaşık merkezde; netlik, pozlama,
   desen kimliği ve fiziksel boyut kontrol edilir. Tam merkez/paralellik şart
   değildir. İlk robot pozu ve görüntü kaydedilir; bu tek kare tam kamera iç
   kalibrasyonu veya robot dönüşümü kabulü değildir.
3. **Kamera modeli:** uyumlu iç kalibrasyon varsa kullanılır. Standart perspektif
   2D kamera için yoksa farklı eğim/konumlarla aynı sihirbazda hesaplanır. Uygun
   görüntüler sonraki robot örnekleriyle ortak toplanabilir; hesap sırası kamera
   modeli → plaka pozları → hand-eye olur. Tek düzlem H/px-mm modeli bu önkoşulu
   karşılamaz. Telecentrik lens için pinhole modeli varsayılmaz; ayrı model/
   gözlemlenebilirlik incelemesi gerekir. Bu öneri geçmiş lens ertelemesini
   genel ölçüm işleri için kaldırmaz; robot yöntemi önkoşulu olarak değerlendirilir.
4. **Örnek toplama:** robot durur, eşleşen gerçek robot pozu ve yeni görüntü
   kaydedilir. Farklı konum, mesafe ve paralel olmayan eksenlerde dönüş gerekir.
   Başlangıç tasarım hedefi yaklaşık 15–20 çeşitli poz olabilir; sayı tek başına
   yeterlilik değildir. Düşük serbestlikli robotlar ayrı yöntem değerlendirmesi ister.
   Tekrarlı, bulanık, belirsiz yönlü veya eşleştirmesi kanıtlanamayan örnek reddedilir.
5. **Hesapla:** modun doğru yönlü dönüşümleriyle çözüm; tutarlı robot base,
   flange/TCP, uzunluk/açı birimleri ve rotasyon tanımı. Plaka–flange montajı
   sabit tutulur, bilinmiyorsa çözümde ele alınır; robot TCP'si plaka merkezi
   varsayılmaz. Kullanıcıya solver seçimi yüklemek yerine servis/test sorumluluğu.
6. **Bağımsız kontrol:** çözüme alınmamış pozlarla konum/yönelim hataları gösterilir;
   kabul eşikleri uygulamanın toleransından gelir. Ardından Kaydet ve kullan.
   Pick reference, çalışma düzlemi/yüksekliği ve TCP offset ayrı öğretimdir.

İlk sürümde robotu kullanıcı/robot programı konumlandırır, TVision örnek toplar;
otomatik poz gezdirme robot bağlantısı ve hareket prosedürüyle ayrı genişletmedir.
Görüntü/poz eşleşmesinde sabit bekleme süresi kanıt sayılmaz. Mevcut OECS eski
pending-frame engeli robot fiziksel kabulünü de engeller; EXP-03 bunun çözümü değildir.
2D kamera + hand-eye keyfi parçanın tam 3D pose'unu sağlamaz; ilk picking hedefi
bilinen düzlemde X/Y/Rz, kalan bileşenlerin açık öğretim/varsayımıdır.

Plaka önerisi: yön/nokta kimliği belirgin ChArUco veya uygun kodlu hedef;
mevcut simetrik desenlerin yön belirsizliği ayrıca giderilmelidir. ChArUco henüz
uygulanmış değildir; yerel OpenCV 5 modül/API ve hedef doğruluğu ayrıca incelenir.

## Sonraki planlama adımı

Önce ekran akışlarını ve bu robot önerisini birlikte değerlendir; sonra kamera
ekleme/canlı test → kalibrasyon ve EXP-02 mm → reçete referanslarını kaldırma ve
Editor/Runtime bağlantısı → robot iç model/örnek toplama/çözüm/doğrulama görevlerini
ayır. İlk iki ekranla reçete alanlarının sadeleştirilmesi aynı ürün hedefidir;
robot hesabı ayrı teslim olur. Mevcut KMC/ICM altyapısı yeniden başlatılmaz.

Açık detaylar: robot marka/poz protokolü ve serbestlik derecesi, lens modeli,
çalışma düzlemi/yükseklik aralığı, tolerans, kullanılacak hedef. Bunlar taslak
değerlendirmesini engellemez; donanım uygulamasından önce netleştirilir.

## Kaynaklar ve mevcut sözleşmeler

- [Mevcut kamera/robot planı](calibration-plan.md), [kaynak bağı](scoped-calibration-integration.md).
- [OpenCV hand-eye ve kamera modeli](https://docs.opencv.org/4.13.0/d9/d0c/group__calib3d.html): dönüşüm yönleri ve paralel olmayan dönme eksenleri.
- [OpenCV ChArUco kalibrasyonu](https://docs.opencv.org/4.13.0/da/d13/tutorial_aruco_calibration.html): çok bakışlı kamera modeli ve nokta kimlikleri.
- [Zivid yerleşim ve veri toplama rehberi](https://support.zivid.com/en/latest/camera/academy/applications/hand-eye/cautions-and-recommendations.html): rijit montaj, farklı bakışlar, durarak edinim. Zivid'e özgü 3D/iç model önerileri mevcut 2D kameraya taşınmaz.
