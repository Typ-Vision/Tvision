# Kalibrasyon ana sayfası, sihirbazlar ve tek aktif kalibrasyon

14 Ekim 2026 — Kullanıcı isteğiyle hazırlanan uygulama planı. **15 Ekim: KUI-1 application sözleşmesi tamamlandı**; [teslim ve migration envanteri](active-calibration-contract.md). KUI-2 session ve KUI-3 plaka servisleri de tamamlandı; [servis kanıtı](calibration-wizard-services.md). KUI-4 UI tamamlandı; [arayüz kabulü](calibration-ui-acceptance.md). KUI-5 ortak Editor/Runtime resolver da tamamlandı; [sözleşme](system-calibration.md). Sıradaki KUI-6 tam Release kabulüdür. Mevcut C1–C5, K1–K3 ve R1–R8 teslimleri korunur. Sabit bir 0,05 mm hedefi veya varsayılan fiziksel hata garantisi bu kapsamda yoktur.

## 1. Ürün kararı ve kapsam

Amaç: kullanıcı koordinat tabloları, kimlikler ve model ayrıntılarıyla uğraşmadan görüntüden kalibrasyon oluşturabilmeli; hangi kalibrasyonun ölçümlerde kullanıldığını tek yerde görebilmelidir.

- Ana sayfada tam üç yöntem kartı: **Bilinen ölçü ile kalibrasyon**, **Kalibrasyon plakası ile kalibrasyon**, **Robot kalibrasyonu**.
- İlk iki kart ayrı, yeniden boyutlandırılabilir adım adım QDialog açar. Üçüncü kart yalnız Eye-in-Hand / Eye-to-Hand açıklamalı şablon sayfasını açar.
- İlk yöntem kullanıcının “bilinen nokta” tarifidir: görüntüde araçla bulunan iki nokta ve aralarındaki gerçek mesafe. Başlık, gerekli girdiyi açıklamak için “Bilinen ölçü” olur.
- Manuel px/mm girişi ana sayfada dördüncü yöntem olmaz. Eski kayıtlar okunur; yeni manuel katsayı oluşturma ilk teslimin kullanıcı akışına dahil değildir.
- Birçok kalibrasyon kaydı saklanabilir; **aynı anda sıfır veya bir aktif kalibrasyon** olabilir. Hesaplanmış kayıt ile aktif kayıt farklıdır.
- Etkinleştirme/değiştirme/pasifleştirme yalnız Kalibrasyon ana sayfasından yapılır. Sihirbaz bitişi kayıt oluşturur, aktif kalibrasyonu değiştirmez.
- Kalibrasyon destekleyen bütün ölçüm araçları tek aktif modeli ortak servis üzerinden kullanır. Araçlarda kalibrasyon ID/model seçimi bulunmaz.
- Lens intrinsics/undistort ve robot hesaplama/hareketi bu teslimde yoktur. Plaka yöntemi düzlemsel perspektif dönüşümüdür. Kullanıcıya lens düzeltmesi yapılmış gibi sunulmaz.

**Tek aktif kapsamının plan varsayımı:** çalışan uygulamanın açık proje/ölçüm kurulumu için tek etkin kayıt. Aynı projede reçete seçmek kalibrasyonu değiştirmez. Başka projeye geçiş veya yeniden başlatma etkinliği kaldırır; son kullanılan kayıt ana sayfada hatırlatılır, yeniden etkinleştirme buradan yapılır. Böylece farklı bir projenin/kameranın kalibrasyonu gizlice devralınmaz. Çoklu kamera ve ayrı uygulama süreçleri arasında makine genelinde etkinlik koordinasyonu bu kapsamın dışındadır.

## 2. Kalibrasyon ana sayfası

Üstte tek durum kartı:

- Etkin kayıt varsa adı, yöntem, kaynak/kurulum, çözünürlük, durum ve **Ayrıntılar / Devre dışı bırak**.
- Etkin kayıt yoksa **Aktif kalibrasyon yok** ve kısa sonraki işlem.
- Bağımsız kontrol bilgisi ayrı etiket: **Yapılmadı**, **Kontrol sonuçları mevcut**, kullanıcı sınır tanımlamışsa gerçek kabul/ret sonucu.
- Geçersizleşen etkin kayıt ölçümde kullanılmaz; neden ve düzeltme işlemi görünür. Sessizce başka kayıt seçilmez.

Ortada üç kart:

| Kart | Kısa açıklama | İşlem |
| --- | --- | --- |
| Bilinen ölçü | Parçada iki kenar/nokta bulun, gerçek mesafeyi girerek piksel–mm ölçeğini hesaplayın. | Başlat |
| Kalibrasyon plakası | Chessboard veya daireli plakanın bilinen geometrisiyle ölçüm düzlemini kalibre edin. | Başlat |
| Robot kalibrasyonu | Kamera ile robot arasındaki koordinat ilişkisi. Eye-in-Hand / Eye-to-Hand. Henüz kullanıma hazır değil. | Şablonu aç |

Altta kısa kayıt listesi: **Ad / Yöntem / Tarih / Durum / Etkinleştir**. Kimlik, sürüm, matris, hash ve teknik rapor ayrıntı açılımındadır. Etkin satır işaretlidir. Yeni bir kayıt kaydetmek veya kayıt listesini seçmek etkinliği değiştirmez. Eski kaydı etkinleştirmek aynı ana sayfa komutundan geçer.

## 3. Ortak sihirbaz davranışı

- Üstte adım göstergesi ve tek cümlelik talimat; ortada geniş görüntü ve yalnız o adımın kontrolleri; altta sabit **Geri / İleri / İptal**.
- Adıma göre birincil işlem **Görüntüyü onayla**, **Noktaları onayla**, **Hesapla**, **Kaydet ve bitir** olur. Hesaplanmamış sonuca ilerlenmez; neden düğmenin yanında açıklanır.
- İşlem sırasında yalnız ilgili kontroller kilitlenir. İptal/Stop ve geç worker sonucu koruması mevcut EditorSession yaklaşımıyla sürer.
- Önceki adıma dönmek veriyi tek başına silmez. Görüntü, ROI, seçilmiş nokta, gerçek mesafe veya plaka tanımı değişirse yalnız bağımlı sonuçlar geçersizleşir.
- Sihirbaz kendi taslak oturumunda çalışır. Reçete DAG'ına geçici tool eklemez; üretim sonuçlarına/aktif kalibrasyona dokunmaz. Kaydetmeden kapanışta taslak atılır.
- Kayıt adı varsayılan üretilir ve son adımda değiştirilebilir. ID/sürüm sistem tarafından yönetilir; hesaplama için başka adıma gidip ID doldurmak gerekmez.
- Sayısal görüntü koordinatları ayrıntılardadır. Kullanıcı işlemi görüntü üstünden yapar. Yakınlaştırma/pan desteklenir; algoritma orijinal raster koordinatlarını kullanır.
- 1400×900 ve 850×600 hedeflenir. Küçük ekranda iletişim kutusu kullanılabilir alanı doldurur; alt eylemler kaybolmaz. Ana nokta/mesafe tablosu yatay kaydırma gerektirmez.

### Ortak ilk adım: Görüntü al / çağır

**Kameradan al** veya **Dosyadan aç**. Çalışan acquisition adapter'ları kullanılır; industrial grab desteği bu ekranla varmış gibi gösterilmez. Görüntü geldikten sonra kaynak, çözünürlük ve **Bu görüntüyü kullan** görünür.

Dosya yolunda iki açık kullanım vardır: **Bu kurulumdan kaydedilmiş görüntü** ve **Sadece deneme**. İlkinde hedef ölçüm kurulumu bir kez seçilir/onaylanır, kaynak dosya kökeni korunur; çözünürlük/koordinat alanı uyuşmazsa bağlanamaz. Kullanıcının aynı kurulum beyanı fiziksel cihazın otomatik doğrulandığı anlamına gelmez. Deneme kaydı gerçek kamera için etkinleştirilemez. Image File reçeteleri de açıkça tanımlanan ortak dosya kurulumuyla çalışabilir.

Bu, mevcut “geçici dosyadan test hiçbir reçeteye bağlanamaz” yolundan farklı, **yeni ve tipli bir kurulum ilişkilendirme özelliğidir**; dosya görüntüsü acquired frame olarak yeniden etiketlenmez.

## 4. Yöntem 1 — Bilinen ölçü sihirbazı

### Adım 1 — Görüntüyü seç

Ortak görüntü adımı. Ölçülecek referans aynı çalışma düzleminde olmalıdır; kısa bir açıklama yeterlidir.

### Adım 2 — Kenarları/noktaları bul

İki basit araç:

- **Tek kenar bul:** kullanıcı kenarı kesecek bir ok/tarama şeridi çizer; EdgePointService gerçek kenarı bulur ve P1, P2… olarak ekler.
- **İki kenar bul (Kaliper):** kullanıcı ölçülecek genişliği kesecek tarama şeridi çizer; EdgePairService gerçek iki ucu bulur. İki ayrı nokta olarak kaydedilir ve aralarındaki çift ilişkisi korunur.

Birden çok tarama yapılabilir; ekle, sil, yeniden bul, yeniden adlandır işlemleri vardır. Çizim uçları ölçüm noktası değildir. Bulunmayan kenar yerine ROI ucu veya eski sonuç kullanılmaz.

Varsayılan subpixel açıktır; başarılı inceltme/refined bilgisi sonuçtan gelir. Basit görünümde tarama yönü, açık/koyu kenar ve duyarlılık bulunur; sigma/step/first-last-strongest gibi ayrıntılar kapalı bölümde kalır. Birden çok aday varsa overlay üzerinden gerçek aday seçilir; belirsiz seçim kullanıcıya gösterilir.

Onaylanan nokta; koordinat, kaynak kare/hash, ROI, araç parametreleri ve algılama sonucu kimliğini taşır. Ham X/Y'yi elle yazmak bu akışın varsayılanı değildir. Aynı noktayı kopyalamak iki ayrı ölçüm ucu sayılmaz.

### Adım 3 — İki noktayı seç ve gerçek mesafeyi gir

Kompakt nokta listesi: **Nokta / Nasıl bulundu / Görüntüde göster**. İki satır seçilince görüntüde bağlayan çizgi vurgulanır. Kaliperin çifti hazır seçili gelir, kullanıcı değiştirebilir.

Tek ana alan: **Bu iki noktanın gerçek mesafesi [mm]**. Yanında hesaplanan piksel mesafesi salt okunur gösterilir. **Hesapla** gerçek pozitif mesafe ve iki farklı geçerli nokta gerektirir.

İlk teslimde hesaplama bir seçilmiş çiftten uniform scale üretir: `mmPerPx = knownMm / distancePx`. Birden çok nokta toplanabilir; hesaplamaya hangi iki noktanın girdiği açıktır. Kullanıcı istemeden tüm noktaları fit'e katma veya iki uzaklıktan perspektif/lens modeli çıkarma yoktur. Çoklu mesafe ile ortak ölçek optimizasyonu ayrı gelecek genişletmedir.

### Adım 4 — Sonuç ve kayıt

**Hesaplandı — kaydetmeye hazır**, kullanılan çift/gerçek mesafe ve ölçek gösterilir. Tek çiftle hesaplanan fit'ten “sıfır fiziksel hata” ya da “doğrulandı” çıkarılmaz.

İsteğe bağlı **Başka bir mesafeyle kontrol et** aynı nokta bulma/seçme etkileşimini kullanır; tekrar ham X/Y tablosu doldurtmaz. Öğretim çifti bağımsız kontrol sayılamaz. Kullanıcı hata sınırı girmezse yalnız gerçek farklar gösterilir; uydurma varsayılan sınır/kabul yoktur.

**Kaydet ve bitir** ana sayfaya döner, yeni kayıt seçilir. **Etkinleştir** yalnız ana sayfadadır.

## 5. Yöntem 2 — Kalibrasyon plakası sihirbazı

### Adım 1 — Görüntüyü seç

Ortak görüntü adımı.

### Adım 2 — Plakayı tanımla

İlk teslim iki desen destekler:

| Desen | Kullanıcının girdiği bilgiler | Açıklama |
| --- | --- | --- |
| Chessboard | Yatay/dikey iç köşe sayısı, gerçek kare kenarı [mm] | Küçük şema kare ile iç köşe farkını gösterir. |
| Simetrik circle grid | Yatay/dikey daire sayısı, komşu merkezler arası mesafe [mm] | Küçük şema mesafenin çap değil merkezler arası olduğunu gösterir; ilk teslim eşit X/Y aralıklıdır. |

Boyut alanı boş başlar; fotoğraftan fiziksel mm tahmin edilmez. Plaka tanımı isimle kaydedilip yeniden kullanılabilir. Hazır örnek grid sayıları kullanılsa bile mm değeri kullanıcı tarafından gerçek plakasına göre girilir.

Karar: ilk teslimde evrensel “standart plaka” veya vendor preset listesi sunulmaz. Ticari plakaların desen ve descriptor'ları farklı olabilir. Asimetrik circle grid, ChArUco ve vendor descriptor importu gelecekte aynı yöntem altında genişletilebilir; ilk teslimde destekleniyormuş gibi seçenek gösterilmez.

### Adım 3 — Noktaları bul ve onayla

**Plakayı bul**: chessboard için mevcut ChessboardService, circle grid için yeni detector adapter'ı. Algılanan tüm noktalar numaralı overlay ile, bulunan/beklenen adet ve geçerli ölçüm bölgesiyle gösterilir. Kullanıcı **Noktaları onayla** der.

Eksik, yanlış sıralı veya geometrik olarak yetersiz gözlemde neden açıklanır. İlk teslim kısmi plaka kurtarma yapmaz. Yanlış fiziksel aralığın görüntüden doğrulanamayacağı kısa açıklanır. Simetrik desen orijini/ekseni fiziksel robot referansı sayılmaz.

Circle grid yeni world point tanımı, blob/merkez tespiti, perspektif altında merkez yanlılığına yönelik bağımsız raster testleri ve kayıt/replay gerektirir; yalnız combobox'a eklemek yeterli değildir. KUI-3 yerel OpenCV 5 features/objdetect build/link ve bağımsız düz/perspektif raster kabulü tamamlandı; yeni ekran henüz yoktur. Güçlü perspektif/clutter/eksik grid reddi ve merkez yanlılığı sınırları servis kabul belgesindedir.

### Adım 4 — Hesapla, incele ve kaydet

**Hesapla** tüm uygun eşleşmelerden raw image→plane homography üretir. Geçerli bölge görüntüde boyanır; mevcut hull dışı ölçüm reddi korunur. Fit istatistikleri “plaka uyumu” olarak etiketlenir; bağımsız fiziksel hata değildir. Lens düzeltmesi uygulanmadığı açıkça belirtilir.

İsteğe bağlı **Başka bir mesafeyle kontrol et** bulunur. Model kurarken kullanılan plaka noktaları bağımsız kontrol diye gösterilmez. **Kaydet ve bitir** yalnız kaydeder; ana sayfadan etkinleştirme gerekir.

## 6. Yöntem 3 — Robot şablonu

Ayrı şablon sayfasında Eye-in-Hand (kameranın robotta olması) ve Eye-to-Hand (kameranın sabit olması) açıklamaları/yerleşim şeması bulunur. **Henüz uygulanmadı** görünürdür. Capture, Calculate, Etkinleştir ve robot hareket komutu yoktur. Şablon açmak mevcut ölçüm kalibrasyonunu değiştirmez.

## 7. Hesaplama, etkinlik ve kontrol durumları

| Durum | Anlamı | Ana sayfadan etkinleştirme |
| --- | --- | --- |
| Taslak | Girdi veya algılama/hesaplama eksik | Hayır |
| Hesaplandı | Gerçek girdilerden matematiksel model ve köken kontrolleri başarılı | Uygun kurulumda evet |
| Aktif | Ana sayfadan seçilmiş tek model | Zaten etkin |
| Geçersiz / uyumsuz | Kaynak, kurulum, geometri veya kanıt doğrulaması başarısız | Hayır |

**Bağımsız kontrol** bunlardan ayrı kanıttır. Kullanıcının istediği tek bilinen mesafe → Hesapla akışını iki zorunlu holdout ile engellemeyeceğiz. Bu karar, mevcut modeldeki `validated()`/en az iki holdout önkoşulunu etkiler ve bilinçli bir ürün davranışı değişikliğidir.

Yeni sürümlü sözleşmede `modelReady`, `verificationEvidence` ve `activeSelection` ayrı tutulur. İsimler uygulama taslağıdır. Sahte holdout eklemek, mevcut `validated` Boolean'ını zorla true yapmak veya tek fit'i bağımsız doğrulama olarak kaydetmek yasaktır. Resolver/Runtime yeni matematiksel geçerlilik politikasını açıkça destekler; model türü, kaynak, çözünürlük, hash, alan ve gerçek sonuç kontrolleri korunur. Kullanıcının tanımladığı bağımsız kontrol başarısızsa etkinleştirme bloke edilir; kontrol yapılmadıysa bu bilgi görünür kalır.

Kalibrasyon etkinliği reçetenin VALID durumu veya parça PASS kararı değildir. Ölçüm tolerans kararı gerçek ölçümden gelebilir; fiziksel doğruluk sertifikası anlamına gelmez.

## 8. Tek aktif modelin bütün araçlara uygulanması

Yeni application-owned etkin kalibrasyon servisi tek kaydı ID + sürüm + contentId + kurulum kimliği + etkinleştirme revision'ı ile çözer. “Katalogdaki en yeni sürüm otomatik aktiftir” yaklaşımı yeni yolda kullanılmaz. Yeni kayıt oluşturmak eski etkin kaydı bozmaz.

### Kurulum kimliği ve kaynak uyumu

Mevcut `CalibrationDependency.cameraId` edinim tool ID'sine bağlıdır. Farklı reçetelerde aynı fiziksel kurulumun tool ID'leri farklı olabilir; ortak kalibrasyon için kaynak tool kimliğiyle kurulum kimliği ayrılmalıdır. Paylaşılan `measurementSetupId` kaynak adapter/config/çözünürlük/koordinat alanı ve lens/düzlem beyanına bağlanır. Aynı boyuttaki iki görüntü tek başına aynı kurulum sayılmaz. Webcam index kalıcı fiziksel kimlik olarak sunulmaz.

Bu kurulum bir kez Kalibrasyon tarafında tanımlanır; uyumlu reçeteler ortak modeli otomatik tüketir. Başka kaynak kullanılırsa araç kalibrasyon seçtirmek yerine **Aktif kalibrasyon bu görüntü kaynağına uygun değil** der ve ana sayfaya yönlendirir.

### Birimler ve mevcut reçeteler

- Yeni metrik araçlar varsayılan olarak **sistem kalibrasyonunu kullanır**. Araç içinde kalibrasyon seçici veya zorunlu “calibrated seç” adımı olmaz.
- Measurement, Caliper, Line Fit, Circle/Arc ve Geometry Measurement ilgili fiziksel sonuçları ortak modelle üretir. Edge Point raw typed noktasını korur; fiziksel nokta gösterimi aynı dönüşümü kullanır.
- Code/Pattern/Shape/Blob gibi araçların konum, piksel alanı ve score portları keyfi şekilde mm'ye çevrilmez. Fiziksel ölçüm çıktısı sunan her yol gerçek dönüşüm/geometri hesabını kullanır; desteklenmeyen mm² vb. sahte sonuç üretmez.
- Raw px portları, debug ve görüntü çizim koordinatları korunur; mm fiziksel sonuç olarak açık etiketlenir. Calculate'ın legacy px portu sessizce mm diye yeniden adlandırılmaz. Yeni fiziksel çıktı ihtiyacı typed descriptor ile eklenir.
- Aktif model olmadan mm isteyen yol açık eksik-kalibrasyon durumu verir. Piksel tanısı çalışabilir; mm toleransına px değeri uygulanmaz.
- Eski araçların px tolerans/ROI parametreleri sayısal olarak aynen bırakılıp mm diye yorumlanmaz. Ana sayfadaki etkinleştirme öncesi ortak etki incelemesi hangi reçetelerin uyumlu olduğunu, hangilerinin birim/ROI düzenlemesi istediğini gösterir. Bu kalibrasyon seçim ekranı değildir.
- Uniform ölçekle mümkün olan dönüşüm açıkça hesaplanabilir; homography için tek katsayıyla px eşik/yarıçap dönüştürülmez. Mevcut exact-calibration'a bağlı nominal world ROI/Referans öğretimleri başka modele otomatik bağlanmaz. Bunlar yeniden öğretim bekler; diğer uyumlu ölçümler ortak modeli kullanır.

### Etkinleştirme ve Runtime

1. Kullanıcı yalnız ana sayfada Etkinleştir'e basar; aday/kurulum/kanıt ve etkilenen reçeteler denetlenir.
2. Runtime çalışırken, worker veya tamamlanmamış kayıt/ACK varken değiştirme reddedilir; mevcut Stop/tamamlama yolu kullanılır. Çalışan çevrimin modeli değişmez.
3. Başarılı kalıcı kayıt ardından tek etkin pointer atomik olarak değiştirilir. Başarısızlıkta önceki etkinlik korunur.
4. Eski modele bağlı mm sonuçlar/overlay/cache ve configuration validation izinleri temizlenir; ham görüntü sonucu gereksiz yere yeniden üretilmez.
5. Yeni Editor/Run Flow ve Runtime hazırlığı aynı ortak resolver'ı kullanır. Immutable snapshot modelin tam kimliğini ve activation revision'ını taşır; değişimde önceki hazır snapshot yeni çevrim başlatamaz, yeniden hazırlanır.
6. Logs/CSV eski ölçümü o çevrimde kullanılan eski modelle korur; geçmiş sonuçlar yeni modele göre tekrar yorumlanmaz. Restart kayıtları gösterir, etkinlik/üretim izni otomatik açılmaz.

## 9. Uygulama sınırları, kayıt ve geçiş

- Qt Widgets yalnız sunum/etkileşim; EdgePoint/EdgePair/Chessboard/PlaneMapping ve yeni circle detector application/vision servislerinde çalışır. Kamera SDK veya OpenCV UI'dan doğrudan çağrılmaz.
- Ortak wizard session görüntü/nokta/plaka/mesafe/model/kontrol taslağı ve generation/cancellation sahipliğini taşır. Aynı görüntüye farklı araçlar eklenebilir; farklı frame'den noktalar karıştırılmaz.
- Yeni target descriptor chessboard/circle grid türünü, sıra/gerçek world koordinatlarını ve kaynak kanıtını sürümler. Circle grid'i chessboard kaydı diye saklama yoktur.
- Yeni active-selection, setup ve model-ready/control evidence alanları strict sürümlü codec ile eklenir; future/unknown/eksik alanlar reddedilir. Domain validation, persistence, execution ve completion aynı sözleşmeyi kullanır.
- Eski uniform/manual/H katalogları, `.vfcalstudy` ve proje/reçete binding'leri okunur; yeni etkinliğe kendiliğinden dönüşmez. Legacy no-op aç/Save hash davranışı korunur. Kullanıcı ana sayfada kaydı seçip açık etkinleştirme yapınca yeni modele geçiş kaydedilir.
- Eski projede farklı reçete kalibrasyonları varsa sistem hangisinin etkin olacağını tahmin etmez. Adaylar listelenir, ana sayfadan tek seçim yapılır. Yeni global politika etkin olduktan sonra recipe binding'e sessiz fallback yoktur.
- Katalog kaydı önce atomik ve içerik doğrulamalı yazılır; etkin seçim referansı ardından atomik kaydedilir. Çok dosyalı tek transaction iddia edilmez. İlk yazı başarılı/ikincisi başarısızsa yeni aday kalabilir, eski etkin seçim korunur; kullanıcıya sonuç açıkça bildirilir.
- Runtime terminal/configuration codec'leri gerekli kimlikleri taşıyacak biçimde sürümlenir; tarihsel replay korunur. Eski production izinleri yeni policy'ye taşınmaz.

## 10. Sıralı uygulama paketleri

Her paketin sonunda progress güncellenir. İlk plan oturumu yalnız belgeydi; kullanıcı 15 Ekim'de sıradaki işe başlamayı istedi ve KUI-1 uygulandı.

| Paket | Teslim | Bitiş kanıtı |
| --- | --- | --- |
| KUI-1 — tamamlandı | Tek aktif model, kurulum, model-ready/kontrol ayrımı ve migration sözleşmesi; sürüm/consumer envanteri | 77 yeni kontrol; Release OFF/core ve ON/UI ilgili 11/11 CTest. Ürün entegrasyonu KUI-4/KUI-5; [kanıt](active-calibration-contract.md) |
| KUI-2 — tamamlandı | Ortak wizard session + bilinen ölçü algılama/hesaplama servisleri | Release core/UI 12/12; 52 kontrol, KUI-3 sahiplik regresyonlarıyla 55. Gerçek Edge/Caliper, tek çift, frame/hash, iptal/geç sonuç; [kanıt](calibration-wizard-services.md) |
| KUI-3 — tamamlandı | Chessboard/circle target ve plaka servisleri | 473 kontrol; Release core/UI ilgili 23/23. Bağımsız düz/perspektif raster/H oracle, merkez yanlılığı, hull/strict replay ve güçlü perspektif ret; [kanıt](calibration-wizard-services.md) |
| KUI-4 — tamamlandı | Ana sayfa, iki sihirbaz, dosya/kurulum ilişkilendirmesi ve robot şablonu | Uçtan uca gerçek tıklama/çizim, geri/ileri/iptal, ID gerektirmeyen akış, 1400×900 ve 850×600 görsel kontrol |
| KUI-5 — tamamlandı | Ortak etkin modelin bütün metrik araçlara/Editor/Runtime/Logs'a bağlanması | Ana sayfadan A→B değişimi, reçeteler/üç applicationType, default mm ve px port koruması, eski tolerans/ROI retleri, snapshot/cache/Stop/ACK/restart/CSV |
| KUI-6 | Birleşik Release kabulü ve kullanıcı akışı denemesi | OFF/core ve ON/UI Release + ilgili/tam CTest, son ekran görselleri, README/gereksinim/mimari güncellemesi ve ilk kullanım denemesi |

KUI-4 tek başına ürün teslimi değildir; KUI-5 tamamlanmadan ekrandaki etkinlik tüm ölçüm araçlarında uygulanmış sayılmaz. UI değişen her pakette Release derleme, ilgili CTest ve görsel kontrol yapılır. INI/fixture/görsel/loglar build altında; config/local.ui.ini korunur.

Dokunulması beklenen alanlar: MainWindowCalibration ve yeni CalibrationHome/Wizard sunumları; EditorSession yanında calibration session; CalibrationWorkflow/Service/Report/StudyStore, MeasurementService; Chessboard/PlaneMapping ve yeni circle adapter; Project/Coordinates/codec; Execution/ProductionRecipe/Runtime resolver ve completion; metrik tool descriptor/executor/Properties; migration ve UI kabul testleri. Sınıf adları kesin uygulama isimleri değildir.

## 11. Kullanıcı kabul senaryoları

1. Kullanıcı boş ana sayfadan başlayıp kaliperle iki kenar bulur, gerçek mesafeyi girer, hesaplar ve kaydeder; tek bir raw X/Y veya ID yazmaz. Ana sayfadan etkinleştirir; iki farklı ölçüm aracı seçim yapmadan aynı modeli kullanır.
2. Birden fazla nokta bulur; yanlış çift seçimini görüntüde fark edip değiştirir; seçilmeyen noktalar ölçeği etkilemez.
3. Chessboard ve circle grid ayrı gerçek rasterlardan bulunur, ölçü girilip model kaydedilir. Algılama başarısızken sonraki adımda eski sonuç görünmez.
4. Etkin A varken B hesaplanıp kaydedilir; A değişmez. Ana sayfadan B etkinleştirilince A kullanım dışı kalır. Aynı anda iki etkin model olamaz.
5. Farklı reçetelerde aynı kurulum ortak modeli kullanır; farklı kaynak/çözünürlükte mm/karar verilmez. Eski mm ROI/px toleranslar yanlış birimle çalışmaz.
6. Robot şablonu yalnız açıklama gösterir; etkinliği değiştirmez, pose/hareket üretmez.
7. İşlem iptali, disk kayıt hatası, proje değişimi ve restart; eski sonucun yeniymiş gibi gösterilmesine veya otomatik üretim iznine yol açmaz.
8. Bağımsız kontrol yapılmamış model “Hesaplandı/Aktif; bağımsız kontrol yapılmadı” olarak görünür. Veri olmadan RMS, maksimum fiziksel hata veya doğruluk garantisi yazılmaz.

## 12. Kaynaklar ve sınırlar

- [OpenCV calibration pattern türleri](https://docs.opencv.org/4.13.0/da/d0d/tutorial_camera_calibration_pattern.html): chessboard, circle grid ve diğer desenler; fiziksel boyut görselden otomatik bilinmez.
- [OpenCV 5 calibration örneği](https://github.com/opencv/opencv/blob/5.x/samples/cpp/calibration.cpp): desen enum'ları ve grid parametreleri. Yerel ABI/link kabulünün yerine geçmez.
- [MVTec calibration plates](https://www.mvtec.com/products/calibration-plates): ticari plaka aileleri. Bu plan HALCON descriptor uyumluluğu veya vendor plate desteği vaat etmez.
- [Mevcut kalibrasyon mimarisi](calibration-plan.md), [fiziksel kabul](calibration-acceptance.md), [manuel eski yol](manual-scale-calibration-plan.md), [proje formatı](project-format.md), [üretim sözleşmesi](production-recipes.md).

Bu plan 0,05 mm veya başka fiziksel doğruluk, lens düzeltmesi, industrial acquisition, robot, CUDA, kurulum/dağıtım kabulü değildir. Önceki T8.4.2b.3c corpus işi ve diğer açık maddeler kapanmış sayılmaz. KUI-5 sonrası sıradaki paket KUI-6'dır.
