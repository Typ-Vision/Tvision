# Reçeteden bağımsız ve çok kameralı kalibrasyon — ürün taslağı

16 Ekim 2026. Durum: **birlikte değerlendirme için öneri; uygulanmış ürün özelliği değildir.** Kullanıcı bu aşamada kaba plan ve ekran tasarımları istedi. Kod mimarisi, şemalar ve geliştirme iş paketleri henüz kesinleşmedi. KUI-1–KUI-5 mevcut uygulama olarak korunur; KUI-6 fiziksel/kullanıcı akışı borçları sürer. 16 Ekim KMC-0 dört UI regresyonunu kapattı (UI 70/70, core 61/61); KMC-1 [kimlik codec'i](measurement-setup-contract.md) tamamlandı, ürün ekranları henüz uygulanmadı.

**16 Ekim değerlendirme sonucu:** kullanıcı **Sistem özeti** düzenini seçti. Kamera sayısı sabit değil; çoğu makine tek, bazıları 8–10 kamera kullanacak. Beş kamera bu taslaktaki örnek veridir. Güncel uygulama sırası ve politikalar [ayrıntılı geliştirme planında](independent-calibration-implementation-plan.md); aşağıdaki ilk taslak notları tarihsel değerlendirme kapsamıdır. KMC-0/1 altyapı teslimleri güncel planda kayıtlıdır; bu belgedeki ekranlar hâlâ taslaktır.

## Amaç ve başlangıç varsayımları

Kalibrasyon sayfası reçete oluşturmadan veya açmadan kullanılabilir. Kamera/ölçüm kurulumu seçilir, kameradan kare alınır veya dosya açılır; mevcut bilinen ölçü/plaka adımları devam eder. Kayıt oluşturmak ile kullanım için etkinleştirmek ayrı kalır.

İlk taslakta kayıtların sahibi açık makine projesidir. Reçetesiz çalışma, projesiz veya bütün projeler arasında ortak kayıt anlamına gelmez. Makine genelinde katalog gereksinimi ayrıca kararlaştırılacak.

Kamera kalıcı donanım kimliğidir; ölçüm kurulumu kamera + optik/geometrik koşullar + çalışma düzlemidir. Kalibrasyon, bu kuruluma ait sürümlü bir kayıttır. Reçete ilgili kurulumu kullanır. Dosya bir görüntü kökenidir; dosya yolu fiziksel kurulum kimliği olmaz.

## Tasarım ilkeleri

- Proje genelinde tek etkin model yerine kurulum başına sıfır veya bir etkin kayıt. Farklı kameraların kayıtları aynı anda etkin olabilir.
- Aynı kameranın farklı düzlem/optik profilleri ayrı tanımlanabilir; gerçek ayarla uyumsuz profil kullanılabilir gösterilmez. Profil seçimi fiziksel kameranın ayarlarını değiştirmiş sayılmaz.
- Görüntü kaynağı ile hedef kurulum ayrı gösterilir. Dosya açınca reçete istenmez. Hedefi henüz bilinmeyen çalışma taslak/deneme olarak saklanabilir; hedef ilişkilendirmesi sürümlü ve açık bir işlemdir, eski kanıt yeniden etiketlenmez.
- Hesaplandı, bağımsız kontrol, etkin seçim ve mevcut kaynakla kullanılabilirlik ayrı durumlar. Bağlantının kesilmesi geçmiş kaydı silmez; kullanım durumunu etkiler.
- Sihirbaz Kaydet ile biter; etkinleştirme ana sayfada seçilen kurulum için yapılır. Runtime çalışırken değişiklik engeli ilk taslakta korunur.
- Recipe/Execution/PartDecision ayrımı, gerçek sonuç olmadan mm/PASS/pose üretmeme, eski sonuç ve geç completion reddi korunur.
- Yeni etkin sürüm eski world ROI/referanslarını sessizce dönüştürmez; etki incelemesinde yeniden öğretim ihtiyacı görünür.

## Ekranlar

| Ekran | Yeni/düzenleme | Kullanıcı işlemi |
| --- | --- | --- |
| Kamera ve kurulumlar | Mevcut kamera ekranının genişlemesi | Kamera seç, fiziksel kimliği gör, ölçüm kurulumu oluştur/düzenle, kalibrasyona geç |
| Kalibrasyon ana sayfası | Düzenleme | Beş kameranın etkin kayıtlarını ve kullanılabilirliğini gör; kurulum seç, yöntem başlat, kayıt geçmişini aç |
| Görüntü adımı | Düzenleme | Kameradan çek / dosyadan aç, hedef kurulum veya yalnız deneme, kaynak/çözünürlük ve aynı-kurulum beyanı |
| Bilinen ölçü / plaka sihirbazı | Mevcut etkileşimleri koruyan düzenleme | Nokta/plaka seçimi, gerçek ölçü, hesap, isteğe bağlı bağımsız kontrol, adlandır ve kaydet |
| Etkinleştirme incelemesi | Düzenleme | Hedef kurulum, eski/yeni kayıt, etkilenen reçeteler/ROI, uyum ve meşgul durumunu gör |
| Recipe Editor kaynak paneli | Düzenleme | Reçete görüntü kaynağını kurulumla ilişkilendir; kullanılan etkin kalibrasyonu salt okunur gör |
| Runtime kaynak hazırlığı | Düzenleme | Yalnız seçili işin gerektirdiği kameralar ve modeller için hazır/eksik durumunu gör |

İki ana sayfa alternatifi incelendi: **Kurulum odaklı** ve **Sistem özeti**. Kullanıcı **Sistem özeti** düzenini seçti. Üründe tek kamera varsa doğrudan kurulum ayrıntısı; birden çok kamera varsa N kart ve seçilenin ayrıntısı gösterilecek. 8–10 kamera ekran genişliğine göre birden çok satıra yerleşir. Kurulum odaklı alternatif ayrı ürün modu olarak geliştirilmez.

## Temel akışlar

1. **Kamera:** Kalibrasyon → kurulum seç/oluştur → yöntem → Resim çek → görüntüyü onayla → mevcut hesap adımları → Kaydet → ana sayfada Etkinleştir.
2. **Dosya:** Kalibrasyon → yöntem → Görüntü aç → hedef kurulumu seç veya yalnız deneme → aynı-kurulum bilgisi → mevcut hesap/kayıt. Kamera o anda erişilemiyorsa hesap/kayıt mümkündür; etkin kullanımın doğrulama politikası ayrıca netleştirilecek.
3. **Beş kamera:** Her kurulum için ayrı kayıt ve etkin seçim; bir kameradaki değişiklik diğerlerinin seçimini değiştirmez. Runtime her frame'in kaynağından modeli çözer ve çevrim boyunca sürümleri sabit tutar.
4. **Uyumsuzluk:** Yanlış kamera/geometri/düzlem → açık gerekçe, ilgili mm yolu kullanılamaz. Başka kalibrasyona otomatik geçiş yok.
5. **Deneme:** Kaynak belirsiz görüntü kaydı korunur; etkinleştirme yerine Kurulumla ilişkilendir işlemi önerilir. İlişkilendirme yeni sürüm ve gerekli yeniden denetimi gerektirir.

## İlk değerlendirmede açık bırakılan kararlar

- Katalog yalnız makine projesine mi ait, uygulama genelinde paylaşılabilir mi?
- Açılışta son seçim otomatik uyum denetiminden sonra kullanılabilir mi, yoksa açık etkinleştirme mi gerekir? Mevcut ürünün yeniden etkinleştirme davranışı henüz değişmez.
- Kamera başına tek güncel profil mi, aynı fiziksel kurulumda birden fazla ölçüm düzlemi mi? Arayüzde gereksiz seçim yükü oluşturmadan ayrım nasıl sunulmalı?
- Bağımsız kontrol opsiyonel mi kalacak, belirli üretim profilleri bunu zorunlu mu tutacak? Hesaplama başarısı fiziksel doğrulama sayılmaz.
- Etkin kalibrasyonu takip eden reçeteler yeterli mi, ayrıca belirli sürüme sabitleme gerekli mi?
- Kaynak çevrimdışıyken etkin seçim saklanıp kullanım mı engellenecek, yoksa etkinleştirme tamamen mi beklenecek?
- Ortak makine koordinatı gereken işler hangileri? Bağımsız mm ölçümlerinden ayrı bir ikinci aşama olarak planlanmalı; stereo/3D/robot yeteneği bu taslakla eklenmez.

## Kaba iş sırası

1. Ekran alternatiflerini ve terimleri birlikte değerlendir; tek kamera ve beş kamera senaryolarını kesinleştir.
2. Kamera/kurulum sahipliği, dosya kökeni, etkinlik ve yeniden başlatma politikasını yazılı ürün kararına dönüştür.
3. Reçeteden bağımsız görüntü/kayıt akışını ve kurulum başına etkin seçimi tasarla; eski verinin geçişini ayrıca belirle.
4. Editor ve Runtime kaynak eşlemesini, sürüm kaydını ve etki incelemesini ayrıntılandır.
5. Uygulama öncesi test/kabul planı çıkar: tek/beş kamera, dosya, yanlış kaynak, profil değişimi, Stop/çevrim, restart ve eski proje. Qt UI değişikliklerinde Release + ilgili CTest + görsel kabul zorunlu.

## İncelenebilir tasarım ve sınırlar

Bu sohbetteki etkileşimli taslakta kamera/kurulum seçimi, iki ana sayfa düzeni, iki yöntemli dört adımlı sihirbaz, dosya/deneme yolu, kayıt ve etkinleştirme incelemesi, Editor ve Runtime kaynak görünümü bulunur. Bütün cihazlar, görüntü çizimleri ve durumlar **örnek senaryodur**. Donanım çağrısı, gerçek hesaplama, reçete/katalog mutasyonu veya üretim sonucu yoktur. Etkinleştirme yalnız taslağın yerel görünümünü değiştirir. Şema ve SDK seçimi yapılmadı.

Görselleştirme kaynağı bu sohbete ait kalıcı görselleştirme dizinindeki `calibration-workspace.html` dosyasıdır. Önizleme ve kontrol çıktıları `build/design-calibration/` altındadır. C++ UI değiştirilmediği için bu taslak tesliminde Release/CTest koşulmaz; ürün kabulü iddia edilmez.
