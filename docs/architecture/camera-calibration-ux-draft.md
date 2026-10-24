# Ertelenmiş robot kalibrasyonu taslağı

21 Ekim 2026. CCUX-PLAN-01 görüşmesinin robot önerisi korunmuştur.
Kullanıcı robot işini güncel uygulama paketinden çıkardı. Kamera/kalibrasyon ve
reçete sadeleştirmesinin tek güncel planı [CCUX](camera-setup/README.md) ve ortak
sözleşmesidir; önceki genel taslağın tekrarı burada tutulmaz.

Bu dosya CCUX başlangıç okuması değildir. Aşağıdaki robot akışı uygulanmış
özellik veya onaylanmış ayrıntılı görev planı değildir. Plaka ile ölçüm ile robot
koordinat dönüşümü farklı amaç ve bağlı kayıtlar olarak kalır.

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

## Kaynaklar ve mevcut sözleşmeler

- [Mevcut kamera/robot planı](calibration-plan.md), [kaynak bağı](scoped-calibration-integration.md).
- [OpenCV hand-eye ve kamera modeli](https://docs.opencv.org/4.13.0/d9/d0c/group__calib3d.html): dönüşüm yönleri ve paralel olmayan dönme eksenleri.
- [OpenCV ChArUco kalibrasyonu](https://docs.opencv.org/4.13.0/da/d13/tutorial_aruco_calibration.html): çok bakışlı kamera modeli ve nokta kimlikleri.
- [Zivid yerleşim ve veri toplama rehberi](https://support.zivid.com/en/latest/camera/academy/applications/hand-eye/cautions-and-recommendations.html): rijit montaj, farklı bakışlar, durarak edinim. Zivid'e özgü 3D/iç model önerileri mevcut 2D kameraya taşınmaz.
