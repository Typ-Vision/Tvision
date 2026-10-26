# COMM-DATA-01 — Kalibrasyon için temel veri tipleri

26 Ekim 2026. Kısa araştırma notu; uygulama veya detaylı plan değildir.
Kullanıcı kapsamı yalnız kalibrasyonda gerekli veri tiplerini öğrenmeye daralttı.

| Veri | İçerik |
| --- | --- |
| Kamera modeli | İç parametreler, lens bozulması, görüntü boyutu ve kamera/kurulum kimliği. mm/px ölçeği ile aynı değildir. |
| Kalibrasyon hedefi | Plakanın nokta/desen geometrisi, gerçek ölçüsü ve birimi. |
| Görüntü gözlemi | Görüntü/frame kimliği, tespit edilen plaka noktaları ve kameraya göre plaka pozu. |
| Robot pozu | Gerçek konum ve yönelim; hangi base/user frame ve hangi flange/TCP için olduğu. |
| Pose / rijit dönüşüm | Öteleme + yönelim + hangi koordinat sisteminden hangisine dönüştürdüğü. |
| Eşleştirilmiş örnek | Aynı durağan konuma ait görüntü gözlemi ve robot pozu; sample ID/zaman bağı. |
| Kalibrasyon sonucu | Hesaplanan sabit dönüşümler, kullanılan model/örnek kimlikleri ve birimli hata değerleri. |

Float64 uygundur. mm/metre, degree/rad ve Euler dönme sırası açık olmalıdır.
Yönelim normalize quaternion veya rotasyon matrisiyle temsil edilebilir.
Quaternion dört sayı ile yalnız yönelimi taşır; ayrıca üç öteleme değeri gerekir.
HALCON Pose ise üç öteleme, üç rotasyon parametresi ve bir tamsayı temsil kodudur;
bu iki yedi sayılık biçim aynı değildir.

**Eye-in-Hand:** sabit ilişki kamera ile seçilen robot tool/flange frame'i arasındadır.
**Eye-to-Hand:** sabit ilişki kamera ile robot base arasındadır.
İki durumda da örnek girdileri robot pozu ve kameranın plaka gözlemidir;
koordinat dönüşümlerinin yönü korunmalıdır.
[HALCON](https://www.mvtec.com/doc/halcon/2511/en/calibrate_hand_eye.html),
[OpenCV](https://docs.opencv.org/4.13.0/d9/d0c/group__calib3d.html).

PDF yararlı bir giriş olsa da doğrudan sözleşme yapılmamalı: s.2'de HALCON
Pose temsil kodu, s.4'te dönüşüm yönleri karışmış; s.6–8'de otomatik senaryo seçimi,
15 poz altı kesin başarısızlık ve stochastic üstünlüğü genel kural sayılamaz.
[create_pose](https://www.mvtec.com/doc/halcon/2511/en/create_pose.html),
[calibrate_hand_eye](https://www.mvtec.com/doc/halcon/2511/en/calibrate_hand_eye.html).

Çıkarım: Number/Boolean yanında **birim ve frame taşıyan Pose/Transform** ile
**eşleştirilmiş CalibrationSample** düşünmek bu aşamada yeterli. Detaylı plan yok.

Kanıt: PDF'nin 14 sayfasının metin çıkarımı; 2/4/7. sayfaların görsel kontrolü;
render ve sentetik dönüşüm cebiri kontrolü `build/research/hand-eye/` altında.
Bu çalışma uygulama testi veya fiziksel kalibrasyon kabulü değildir.
PDF SHA-256: `9ea22d8d46823fd2dee1b7d82926c456fd76ef9c82054e3990e4db0da39ab5fe`.
