# 6C-K4 — Fiziksel planar kalibrasyon kabulü

8 Ekim 2026. K2/K3 yazılım ve sentetik test kabulü tamamlandı. Bu belge fiziksel kabul prosedürüdür; gerçek plaka, bağımsız referans ve kabul ölçümleri henüz sağlanmadığı için **fiziksel doğruluk kabul edilmedi**. Kullanıcı K4'ü kurulum hazır olana kadar erteledi ve TCP/IP socket önceliğiyle bağımsız 6D PLC yazılımını yetkilendirdi; bu fiziksel kabulü tamamlamaz.

## Önce belirlenmesi gerekenler

- Gerçek kamera/optik, lens/focus/zoom, sabit çözünürlük ve crop/resize/orientation; kamera–çalışma düzlemi montajı ve parça özelliğinin yüksekliği.
- Plaka ID, gerçek iç köşe sayısı ve kare kenarı mm. Baskı veya görselde belirtilen nominal boyut tek başına gerçek ölçü kanıtı değildir; gerçek referansla kontrol edilir.
- Ölçüm toleransı ve kalibrasyona ayrılan maksimum hata bütçesi mm. Referans ölçümün belirsizliği ve Edge/localization/tekrarlanabilirlik katkıları ayrıca değerlendirilir. UI varsayılan 0.01 mm fiziksel doğruluk garantisi değildir.
- H dışında tutulmuş bağımsız referans uzunlukları; referans cihazı/sertifika veya ölçüm yöntemi, ölçüm tarihi ve çevre koşulları. Fit köşeleri tekrar holdout olarak girilmez.

## Ölçüm sırası

1. Sabit kamerada gerçek plakayı ölçülecek özelliklerle aynı düzleme yerleştirin. Tüm üretim ölçüm noktaları görünen iç köşelerin convex hull alanında kalmalıdır; dışarıda extrapolation kapalıdır. Plaka alanı yeterli değilse görüntü/kurulum yeniden hazırlanır.
2. Webcam/Image File kaynağını gerçek kurulumla açıkça eşleştirin. Calibration → lens/kurulum/düzlem → Kurulumu uygula → Kare al. Geçici **Dosyadan test** yolu fiziksel kamera binding'i oluşturmaz.
3. Target bilgilerini girip Köşeleri bul. P1/orijin, +X sütun, +Y satır ve tüm köşeleri inceleyin. Simetrik chessboard fiziksel yönü otomatik tanımlamaz; bu model target-local mesafe içindir, robot frame'i değildir.
4. H hesapla. Fit RMS mm ve inverse RMS/max px yalnız fit tutarlılığıdır. 1 px inverse fit sınırını aşan, hatalı sıralı veya dejenere plaka reddedilir. Düşük fit RMS tek başına lens bozulmasının hedef bütçede olduğunu kanıtlamaz.
5. Merkez, köşelere/kenarlara yakın bölgeler ve en az X/Y/diagonal yönlerini kapsayan bağımsız segmentleri holdout tablosuna girin. Referansların gerçek mm değerini önceden kaydedin; H'den hesaplanmış mm'yi Known mm alanına geri yazmayın.
6. Doğrula. Her segmentin Measured mm/Absolute residual mm değerini ve toplu RMS/max mm'yi inceleyin. Yazılım en az iki farklı holdout ister; fiziksel bölge/yön kapsamı bundan daha fazla örnek gerektirebilir. Hata bütçesini aşan örnekleri silerek kabul üretmeyin; sebebi ve yeniden kurulum/lens ihtiyacını araştırın.
7. Bağımsız referansı birkaç kez yeniden yerleştirip yeni karelerle ölçün. Önerilen başlangıç: her bölge/yön için en az 10 çevrim; uygulamanın hız/ısı/aydınlatma/parça değişkenliği daha geniş kabul gerektirebilir. Bu sayı yazılımın otomatik kabul eşiği değildir.
8. Kanıt raporunu CSV dışa aktarın. Tek rapor matematiksel holdout özetidir; fiziksel referans kökenini kanıtlamaz. [Boş fiziksel gözlem tablosuna](../templates/calibration-physical-observations.csv) tekrarları, bölge/yönü, referans cihazını ve belirsizliği kaydedin. Maksimum mutlak hata, RMS ve tekrarlar arasındaki dağılımı hata bütçesiyle karşılaştırın.
9. Lens bozulması özellikle kenarlarda bütçeyi aşıyorsa **KL lens kalibrasyonu/undistort** gerekir. Mevcut raw H yolu distortion düzeltmesi yapmaz; bu kurulumda fiziksel kabul açık tutulur. Verisiz intrinsics veya sessiz scale fallback üretilmez.
10. Sonuç uygunsa sürümlü kalibrasyonu Kaydet ve bağla, sonra normal Save. Runtime Taslağı doğrula → Etkinleştir → Tek çevrim yalnız yapılandırma/yürütme kabulüdür; fiziksel rapor ayrı tutulur. Kamera/lens/focus/zoom/düzlem/yükseklik/crop/çözünürlük değişince ilgili kabul yenilenir.

## Kabul kaydı

Kayıt; calibration ID/version/contentId/model, source ve lens/düzlem kimlikleri, target ölçü kanıtı, hata bütçesi, referans/cihaz/belirsizlik, bölge/yön kapsamı, tekrar sayısı, ölçüm sonuçları, kabul eden kişi/tarih ve sınırlamaları içerir. Fiziksel kimliğin otomatik doğrulanmadığı beyan edilir. Yerel CSV/dosya matematiksel replay'i korumalı audit veya dış sertifikasyon değildir.

Bu oturumda gerçek plaka/bağımsız fiziksel ölçüm verisi yoktur. KL gerekliliği veriyle belirlenemedi; K4 açık kalır. Sentetik test/fixture sonuçları bu formun gerçek kabul satırları olarak doldurulmaz.
