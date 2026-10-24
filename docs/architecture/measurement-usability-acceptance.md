# MUI-04 — Ölçüm kullanılabilirliği kabulü

19 Ekim 2026. Uygulayıcı: Codex.
Durum: yazılım corpus'u ve gerçek kullanıcı görüntüsü kabulü ayrı izlenir.
Kullanıcı bu oturumda gerçek raster/reçete/ROI'nin **hazır olmadığını** bildirdi.
Gerçek telecentrik görüntüde bildirilen problem yeniden üretilmiş sayılmaz.

## Tekrarlanabilir yazılım corpus'u

`measurement_raster_acceptance_test.cpp`: analitik ikili sınıflandırmayla üretilen,
anti-alias içermeyen 192×192 Gray8 rasterlar. Circle merkezi için üç subpixel faz,
iki kontrast yönü ve iki tarama yönü; Line için üç açı, üç faz ve iki kontrast yönü.
Gray16 eşdeğeri aynı Gray8 değerin 257 ile çarpılmasıyla üretilir; normalize
detektör eşitliği sınanır. Bu fiziksel sensör bit aralığı veya hassasiyet kabulü değildir.

Bağımsız analitik oracle kapıları: radius hatası <0.6 px, Line açı hatası <0.8°.
Bunlar sentetik keskin raster regresyon sınırlarıdır; telecentrik doğruluk veya
subpixel performans garantisi değildir. Gerçek koşum sayıları/hataları kapanış
kaydında ve build raporundadır.

Negatif örnekler:

- Ters polarity: ölçülmüş polarity retleri, geçerli fit/ölçüm üretmeme.
- 12 ok / 15° boşluk: fit bulunabilir, kontur tanısı ve accepted invalid kalır.
- 6 px halka araması: örnekleme desteği, eşik düşürme önerisinden ayrılır.
- Kısa çizgi: bulunan noktalar varken uzamsal destek reddi ayrı kalır.
- 45° yay: geometrik destek/conditioning reddi korunur.
- Gray16 düşük etkin aralık: tam 65535 normalizasyonunda eşik altı aday tanısı.
- Hasarlı kontur: fit outlier'ları gerçek sapmadan çıkarılmaz; tolerans ret olur.
- İki kenarlı halka: İlk/Son ayrı gerçek yarıçap seçer; doğru kenar otomatik
  varsayılmaz. ROI ve seçim kullanıcı tarafından doğrulanmalıdır.

Mevcut calibrated_circle/metrology_runtime/world_tracking/reference_world ve
geometry_measurement_service testleri mm/px, non-affine world/refit, geçersiz
kalibrasyon, no-op/reopen/TargetOnly ve immutable completion sözleşmelerini korur.
Her koşumun kapsamı kapanışta ayrı kaydedilir; tarihsel geçiş yeni test sayılmaz.

## Gerçek kullanıcı rasteriyle bekleyen kabul

Gerekli girdi: özgün görüntü dosyası, proje ve reçete, Line/Circle ROI'leri,
tam hata metni, beklenen kenar/ölçü ve px/mm modu. Dosya yolları geldiğinde:

1. Kopyalanmış proje ve test INI'sini build altında aç; özgün dosyaları koru.
2. Değişiklik yapmadan mevcut reçeteyi çalıştır, tam hata/tanı ve kimlikleri kaydet.
3. Ham bit aralığı, çözünürlük, ROI desteği, ok/geçiş ve aday retlerini karşılaştır.
4. Normal/ters kontrast ve yön, az ok, dar halka, kısa çizgi, dar yay, mm/px,
   reference/world ve yanlış kenar/hasarlı kontur senaryolarını ayrı kaydet.
5. Geçersiz geometride ölçüm/PASS olmadığını, gerçek kusurun sapmada kaldığını,
   Save/reopen/no-op, Runtime/Logs/restart ve Stop/invalidation'ı doğrula.
6. Değişen her ayarın önce/sonra değerini ve gerekçesini kaydet; kaliteyi geçmek
   için otomatik tolerans gevşetme yapma. Fiziksel boyut oracle'ı ve kalibrasyon
   kanıtı yoksa yalnız raster tanısı olarak raporla.

Bu bölüm tamamlanana kadar **MUI-04 gerçek görüntü kabulü açık** kalır.
Native kamera G0/G1, lens/ölçüm belirsizliği, PLC/robot, native DPI/çoklu monitör,
CUDA ve kurulum/dağıtım kabulü bu corpus ile kapanmaz.
