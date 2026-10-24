# MUI-ANALYSIS-01 — Ölçüm araçları ön analizi

19 Ekim 2026. Uygulayıcı: Codex.
Durum: ön analiz ve uygulama önerisi; ürün davranışı değiştirilmedi.

Kullanıcı temiz, telecentrik görüntüde Line Fit ve Circle ile sonuç alamadığını,
**pixel** modunda denediğini, Circle mesajının yaklaşık “insufficient point”
olduğunu bildirdi; Line hata metni hatırlanmıyor. Gerçek raster, reçete/ROI ve
tam hata metni bu incelemede mevcut değil. Aşağıdaki kod bulguları doğrulandı;
kullanıcının görüntüsündeki tek kök neden henüz doğrulanmadı.

İncelenen sözleşmeler: [T4–T7 kartları](recipe-tools-plan.md),
[Line](metrology.md), [Circle](circle-tool.md), [Edge Point](edge-point.md),
[UI hata gereksinimi §41](../requirements/02_machine_vision_ui_gereksinimleri.md).
Tamamlanmış T/EUI teslimleri yeniden açılmadı. Bu belge yeni iyileştirme önerisidir.

## 1. Sonuç

Sadeleştirme gerekli. Basic/Detailed görünümü zaten var; fakat gizlenen ayarlar
sonucu etkilemeye devam ediyor. Sorun yalnız alan sayısı değil: bağımlı ayarlar,
birim geçişi ve başarısızlığın hangi aşamada oluştuğunun açıklanmaması.

Öneri: günlük kullanımda ROI, tarama sayısı/yönü, kenar geçişi, kenar seçimi ve
kenar eşiği kalsın. Şerit genişliği ve gürültü ayarı kapalı gelişmiş bölümde;
fit kalitesi ayrı bölümde; ürün toleransları ayrı ve isteğe bağlı değerlendirmede
olsun. Sayısal kararlılık kontrolleri algoritmada kalmalı; kullanıcıdan solver
ayarlaması beklenmemeli. Başarısızlıkta otomatik tolerans gevşetilmemeli.

## 2. Kodda doğrulanan sorunlar ve olası nedenler

### A. “Nokta bulunamadı” ile “fit consensus yetersiz” şu an kolay karışıyor

`LineFitter.cpp:31` ve `CircleFitter.cpp:29`: bulunan nokta sayısı `minPoints`
altındaysa hemen hata. Circle varsayılanı en az **6 nokta**; bu ret henüz
RANSAC, RMS, çap veya sapma toleransına ulaşmadan olur.

`CircleFitter.cpp:77–79` ise nokta bulunmuş olsa bile yeterli nokta tek dairede
uzlaşmadığında “Insufficient circle [refit] consensus” verir. Kullanıcının
yaklaşık hatası iki sınıftan birine karşılık gelebilir. İlkinde fit toleransını
artırmak çözmez; ikincisinde fazla dar inlier mesafesi olası etkendir.

`EdgeFinder.cpp` reddedilen adayda `polarity`, `min_contrast`, `border_support`
nedenlerini zaten hesaplıyor. Ancak `LineFitter::observationsMapped` yalnız
seçilen noktayı taşıyor; `RadialScanner` başarısız taramayı `not_found` yapıyor.
Fit exception olduğunda `ExecutionService.cpp:280` çıktı/overlay'leri temizleyip
exception metnini gösteriyor. Sonuç güvenliği doğru; **başarısız denemenin
tanısı kayboluyor**. Nokta sayısı ve aday ret nedenleri hata yanında kalmalı.

### B. Circle mm → pixel geçişinde fit değerlerinin sayısı korunuyor

`MainWindowEditor.cpp:585`: yeni Circle mm modunda, `scanWidth=.1`,
`inlierDistance=.1`, `maxRms=.035`, `maxResidual=.1` ile oluşturuluyor.
`MetrologyEditor::parameters` mod değişiminde ROI/binding'i temizleyip
`unitsPerSample` değerini değiştiriyor; bu dört alanın sayılarını değiştirmiyor.

Sonuç: kullanıcı pixel seçtiğinde RMS **.035 px**, inlier/max residual **.1 px**,
şerit genişliği **.1 px** oluyor. Registry'nin pixel başlangıcı sırasıyla
**.35 px, 1 px, 1 px, 1 px**. Aynı pixel araca ulaşmanın iki yolu farklı
başlangıç davranışı veriyor. Bu statik kod bulgusudur; kullanıcının kaydedilmiş
değerlerinin bu şekilde olduğu henüz bilinmiyor.

Bu, özellikle consensus/residual hatasında öncelikli düzeltme adayıdır.
“Insufficient found circle points” tam metni doğrulanırsa dar fit eşikleri o
erken hatanın nedeni değildir. Şerit genişliği ise kenar örneklemesini etkiler.

Çözüm önerisi: yeni araç için birime uygun, açık başlangıç profili; mod geçişinde
boyutlu değerlerin ne olacağını görünür kılan atomik işlem. Mevcut reçetelerde
kullanıcının özel eşikleri sessizce sıfırlanmamalı veya kalibrasyonsuz çevrilmemeli.

### C. Tarama oku, polarity ve threshold ayrı kavramlar

Line ROI'sinin yerel X ekseni çizgi boyunca; oklar yerel −Y → +Y tarıyor.
Oklar kenarı kesmeli, kenara paralel ilerlememeli. Dikey kenarda kutunun açısı
bu yüzden önemlidir. Fit yönü taught eksenden 60°'den fazla saparsa ret var.

Circle içten dışa/dıştan içe yönünde arar. **Ok boyunca** beyazdan siyaha geçiş
`falling`, siyahtan beyaza `rising` demektir. Ok terslenince fiziksel aynı kenar
için polarity de değişir. UI'daki “Kenar yönü” etiketi bunu yeterince anlatmıyor.
Etiket “Kenar geçişi”; seçenekler “Beyaz → Siyah / Siyah → Beyaz / Her ikisi”
olmalı ve “ok yönüne göre” açıklaması bulunmalı. İlk teşhiste `any` yararlı;
üretimde istenen kenar açıkça seçilmeli.

`minContrast=20` bir siyah/beyaz görüntü threshold'u değildir: yumuşatılmış
profilin **0–255 normalize yoğunluk değişimi / örnek koordinat birimi** eşiğidir.
Raw modda gray8/px, world sampling'de synthetic px başınadır. First/last/strongest
yalnız eşiği ve polarity'yi geçen adaylar arasından seçim yapar. Hiç aday yoksa
selection değiştirmek çözüm değildir. Gray16 veride değerler 255/65535 ile
normalize edilir; etkin bit derinliği/display görünümü ayrıca incelenmelidir.

### D. Çok dar tarama alanı temiz kenarda da başarısız olabilir

Varsayılan `step=.5`, `sigma=1`. Line/Radial ön kontrolünde
`count=ceil(length/step)+1`, `radius=ceil(3*sigma/actualStep)` ve
`count >= 2*radius+4` gerekiyor. Örneğin **6 px** tarama: 13 sample, radius 6,
gereken 16 → ret. **8 px**: 17 sample, radius 6 → bu ön kontrol geçer.
Bunlar koddaki formülün aritmetik örnekleridir; görüntü deneyi değildir.

Kenar tarama ucuna çok yakınsa filtre/tepe komşuluk desteği eksilir;
`border_support` ile aday elenir. Line'da kutu yüksekliği, Circle'da dış−iç
yarıçap farkı arama uzunluğudur. `scanWidth` bundan farklı olarak kenar boyunca
ortalama alınan **şerit kalınlığıdır**. İki genişlik kullanıcıya ayrıştırılmalı.
Önce ROI kenarın iki tarafında destek bırakacak şekilde genişletilmeli; doğrudan
threshold düşürme veya sigma sıfırlama önerisi verilmemeli.

### E. Circle tarama sayısı ile kalite sınırları çelişebiliyor

Varsayılan: 72 ok, fit oranı ≥.60, gözlem oranı ≥.90, max gap ≤15°,
fit açısal desteği ≥90°. Bu dört kontrol farklı şeyleri ölçüyor.

Tam dairede 12 ok ve tüm noktalar bulunsa bile komşu aralık **30°** olur;
`maxGapDegrees=15` geçilemez. Fit başarılı olabilir ama `qualified=false`,
karar `NOT_EVALUATED` olur. Kullanıcı sadece ok sayısını azaltarak gizli bir
parametreyle çelişen konfigürasyon oluşturabilir. Tam dairede mevcut 15° şartı
en az 24 eşit aralıklı örnek gerektirir; bu sayı yalnız gap için gerekli koşuldur.

90°'den dar yay da tüm noktalar bulunsa bile mevcut minAngularSpan=90 ile
reddedilir. Dar yayı kabul ettirmek için conditioning denetimi kapatılmamalı.
ROI/sayı/kalite ayarı çelişkileri Run öncesinde açıkça anlatılmalı.

### F. Line'da pixel ve mm fit birbirine bağlı

Normal, referanssız `MetrologyService` calibrated yolunda önce raw fit,
sonra tüm gerçek noktaların mm'ye taşınması ve world refit var. Pixel kapısı
reddederse mm fit çalışmıyor. Kullanıcının **pixel** denemesinde mm eşikleri
devrede değildir; mm alanlarını değiştirmek o denemeyi çözmez.

Pixel varsayılanları: 21 tarama, ≥6 nokta, used/requested≥.6,
scan-ID yayılımı≥.5, uzamsal destek≥10 px, inlier≤1 px, RMS≤.35 px,
max residual≤1 px. Kısa çizgi için 10 px destek bile ayrı ret sebebidir.
Line alt/üst toleransı nesnenin toplam boyunu değil **ölçülen destek uzunluğunu**
değerlendiriyor. Genel “Alt tolerans” etiketi bunu gizliyor.

World/reference yolları `WorldTrackingService` üzerinden farklı sampling/refit
kullanıyor. Sadeleştirme bütün yolları aynı varsayarak uygulanmamalı; ayrı regresyon
gerekir. Raw kalitenin world metriği engellemesi ayrıca sözleşme değişikliği olarak
değerlendirilmeli; sadece UI alanını gizleyerek çözülmez.

### G. Piksel sayısı × ok sayısı sınırı görüntü kalitesinden bağımsız

Line/Radial kodunda görüntü≤16 Mi pixel ve pixel×scan≤512 Mi hash iş bütçesi var.
Örneğin 4096×3000 ve 72 ok ikinci kapıyı aşar. Threshold değiştirmek fayda etmez;
mesaj izin verilen ok sayısını göstermeli. Bu sınır ayrıca performans çalışmasının
konusudur; kullanıcıdaki görüntünün çözünürlüğü bilinmediği için kök neden sayılmaz.

## 3. Parametre kararı

| Alan | Önerilen sunum / karar |
| --- | --- |
| ROI, halka/yay, başlangıç ve yay açıklığı | Görüntü üzerinde; koordinatlar kapalı Elle gir. Arama uzunluğu açıkça görünür. |
| scanCount, direction | Temel. Line için yön ROI açısı + oku çevir etkileşimiyle anlatılır; Circle inward/outward kalır. Sayı değişiminde bağımlı kalite uygunluğu kontrol edilir. |
| polarity, selection, minContrast | Temel. Geçiş, İlk/Son/En güçlü, Kenar eşiği. Threshold birimi ve ok yönü açıklanır. |
| mode / calibration | Tek açık birim/kaynak durumu; algoritma ayarları arasında kaybolmaz. Kalibrasyon yoksa px tanısı kullanılabilir, mm sonucu üretilemez. |
| scanWidthPx / scanWidth | Gelişmiş “Ortalama şerit kalınlığı”; işe yarar, kaldırılmamalı. ROI arama uzunluğuyla karıştırılmamalı. |
| sigmaPx | Gelişmiş gürültü yumuşatma. Düşük/Standart/Yüksek önerileri gerçek sayısal ayarı görünür kılmalı; doğrulanmamış preset teslim edilmemeli. |
| stepPx, widthStepPx, unitsPerSample | Günlük panelden çıkar; uzman sampling ayarı. Önce birime/ölçeğe uygun politika doğrulanmalı. Eski değerler korunur. |
| subpixel | Normal kullanımda açık; uzman tanısı için override saklanabilir. Kullanıcıdan hassasiyet garantisi gibi seçim istenmez. |
| ransac / fitRansac | Normal kullanımda sağlam fit açık; algoritma adı günlük panelden çıkar. Mevcut override korunur. |
| inlierDistance*, fitInlierDistance | Gelişmiş “Fit'e katılma mesafesi”, açık birim. Yanlış kenarların kabul edilmesine yol açabileceğinden otomatik artırılmaz. |
| maxRms*, maxResidual*, fitMax* | Gelişmiş kalite; ölçülen değer ve sınır yan yana. İkisi aynı değildir: ortalama hata ile tek noktadaki en büyük hata. |
| minPoints / minFoundPoints / fitMinPoints | Günlük alandan çıkar; scanCount ile tutarlı politika/uzman ayarı. Minimum geometri desteği kaldırılmaz. Fit noktası ve bulunan nokta ayrımı tanıda korunur. |
| minFitCoverage / minCoverage | Tek anlamlı alanmış gibi birleştirilmez: kullanılan fit noktaları ile bulunan kontur noktaları farklıdır. Gelişmiş kalite politikasında, yüzde ve açıklamayla sunulur. |
| minSpanCoverage, minSpanPx/Mm, lineMinSpan | Gelişmiş destek kalitesi; birime/ROI'ye bağlı ön kontrol. Kısa çizgide hangi koşulun yetmediği gösterilir. |
| minAngularSpan, circleMinAngularSpan | Gelişmiş yay desteği; dar ROI ile imkânsız konfigürasyon Run öncesi açıklanır. |
| maxCondition, circleMaxCondition, lineMinVarianceRatio | Normal ayar listesinden çıkar; sayısal güvenlik/uzman politikası. Başarısız fit için “bunu artır” ilk öneri olmaz. |
| lower/upper, angleLower/Upper | Ayrı “Ölçümü değerlendir” bölümünde, Line için “Destek uzunluğu / yönlü açı sınırı”. |
| diameterLower/Upper, deviationLower/Upper | Ayrı çap ve yüzey sapması kontrolleri. Geometri bulmak için ürün toleransı girmek gerekmemeli. Kapatılabilir değerlendirme gelecekte açık config/karar semantiği ister. |
| maxGapDegrees | Gelişmiş kontur kapsaması; scanCount/yay ile birlikte doğrulanır. Seyrek örneklemenin kaçırabileceği kusur boyutu dikkate alınır. |

“Kaldır” burada öncelikle **normal kullanıcı panelinden çıkar** demektir.
Alanları şemadan topluca silmek, mevcut reçete davranışını ve replay'i bozar.
Basit görünüm bir görünüm tercihidir; eski özel değerleri sessizce resetlemez.
Yeni kullanıcı için sade profil ancak kaydedilen açık değerlerle uygulanmalıdır.

Diğer ölçüm araçları:

- **Edge Point:** aynı geçiş/seçim/eşik dili; mevcut aday/profil grafiği korunur.
  Sayısal sampling ayarları gelişmişte. Başarısız aday nedenleri burada zaten var.
- **Edge Pair/Caliper:** açık şerit/koyu şerit ayrımı gerekir; tek kenar polarity'si
  yerine geçirilmez. Min/max çift genişliği istenen çifti seçer, ölçüm toleransı
  değildir; ayrı “Aranan genişlik aralığı” bölümü. Ortak edge tanısı genişletilir.
- **Geometry Measurement:** işlem ve girdiler temel; yalnız o işlemin seçenekleri
  gösterilir. Refit ayarları uzman bölümünde kalır; downstream refit hatası,
  upstream çizgi/daire hiç bulunamadı hatasından ayrılır.
- **Measurement:** iki nokta ve mm alt/üst sınırı zaten yalın; gereksiz yeni fit
  ayarı eklenmez. Eksik nokta/kalibrasyon/gerçek tolerans aşımı ayrıştırılır.
- **Legacy Edge:** gray8 komşu farkı ile Edge Point gray8/px aynı eşik değildir;
  etiketler açıklanır, araçlar sessizce dönüştürülmez.

## 4. NOK alanında önerilecek tanı

Görsel olarak aynı sonuç alanında sunulabilir; execution ERROR, ölçüm
NOT_EVALUATED ve gerçekten ölçülmüş tolerans dışı sonuç birbirine çevrilmez.
Genel parça kararı ayrı kalır. Aşağıdaki mesajlar **tasarım örneğidir**, çalışmış
görüntü sonucu veya yeni uygulanmış UI değildir.

| Gerçek kanıt / aşama | Mesaj ve öneri |
| --- | --- |
| 0 veya <6 seçili Circle noktası | “72 taramadan 3 kenar bulundu; en az 6 gerekli.” ROI ve aday ret nedeni gösterilir. |
| Adayların eşiğin altında kalması | “Kenar adayları eşik altında. Kenar eşiğini azaltmayı deneyin.” Aday gücü/eşik ölçülmüşse sayıları göster. Hiç aday yoksa bunun kesin neden olduğu söylenmez. |
| Adayların ters polarity ile elenmesi | “Geçişler ok yönünde siyah → beyaz; seçili geçiş beyaz → siyah. Geçişi veya tarama yönünü kontrol edin.” |
| Tepe tarama ucunda / destek kısa | “Kenar arama sınırında. ROI'yi kenarın iki tarafında pay bırakacak şekilde genişletin.” Gerekli/mevcut destek ölçüsü gösterilir. |
| Çok aday / yanlış kenar | “Tarama birden fazla geçiş içeriyor. ROI'yi daraltın; İlk/Son/En güçlü seçimini adaylar üzerinde kontrol edin.” |
| Bulunan çok, fit'e katılan az | “Kenarlar bulundu, yeterli nokta aynı daireye/çizgiye uymuyor.” Önce noktaları incele; doğru konturdalarsa fit'e katılma mesafesini kontrollü artırmayı öner. |
| RMS veya max residual aşılmış | “Fit kalite sınırını aşıyor: ölçülen … px, sınır … px.” Yanlış kenar/gürültü/şekil kontrolünden sonra ilgili **fit kalite** sınırını incele. Çap toleransı önerilmez. |
| Dar yay / düşük geometrik destek | “Daireyi kararlı belirlemek için yay desteği yetersiz. Daha geniş yay seçin.” Condition sınırını gevşetmek otomatik öneri değildir. |
| Circle fit var, kapsam yetersiz | “Daire bulundu; kontur değerlendirmesi yapılamadı. Bulunan oran …, boşluk …°.” Eksik bölge/ok sayısı/ROI'yi göster. 12 ok–15° gibi imkânsız ayarı açıkla. |
| Kalibrasyon/frame/ROI alanı hatası | İlgili bağlama/yeniden öğretme/alan düzeltmesi. Eşik veya fit toleransı önerilmez. |
| İş bütçesi aşılmış | Çözünürlük, mevcut ok sayısı ve izin verilen sınır; tarama sayısını azaltma önerisi. |
| Gerçek ölçüm tolerans dışında | Ölçülen değer, birim, alt/üst sınır. Parçayı ve tanımlı şartnameyi kontrol et; “NOK'u geçirmek için toleransı artır” önerisi verilmez. |

Önerilen tanı modeli: sabit `code`, `stage`, `toolId`, frame/cycle/evaluation
kimliği, `actual/required/unit`, bulunan/kullanılan/elenen/eksik sayıları,
aday ret sayıları, ilgili parametre anahtarı, kısa açıklama ve öncelikli eylem.
İlk bloklayan neden önde, diğer nedenler ayrıntıda. Sayı bilinmiyorsa uydurulmaz.

UI exception metnindeki İngilizce kelimeleri arayarak karar vermemeli.
Vision katmanı yapılandırılmış tanı üretmeli, application anlam/eylem eşlemeli,
UI göstermeli. Fit başarısız olsa da o çalışmanın gerçek gözlemleri ayrı **tanı**
olarak gösterilebilir; geçerli line/circle portu, ölçüm/PASS üretilmez. Ayrı tanı
taşıyıcısı bounded olmalı; tüm profilleri her scan için tutmak yerine özet +
seçili scan profili kullanılmalı. Stop, kaynak/parametre değişimi eski tanıyı
temizlemeli. Runtime/Logs codec ve completion sınırları ayrıca ele alınmalı.

## 5. Önerilen uygulama sırası ve kabul

Bu kartlar öneridir; kullanıcı bu turda **ön analiz** istedi. Ürün uygulaması
başlatılmadı. Çalışma sıralı; paralel ajan yok.

1. **MUI-01 — Tanı ve pixel başlangıç davranışı.** İki “insufficient” sınıfını
   ayır; scan ret nedenlerini koru; Circle yeni araç/mm→px geçişini hedefleyen
   regresyon ekle. Kaydedilmiş özel ayarları bozmadan birim politikasını uygula.
2. **MUI-02 — Sade parametre ekranı ve ön kontrol.** Temel/gelişmiş/kalite/ürün
   sınırlarını ayır; px/mm etiketleri, ok-geçiş açıklaması, count/gap/span/support
   çelişkileri. Eski özel ayarları görünür kıl; değiştirme.
3. **MUI-03 — Ortak çözüm önerileri.** Line/Circle'dan başlayıp Edge Point,
   Caliper, Geometry Measurement'a aynı tanı dili; Editor/Runtime/Logs ve
   bounded serialization/replay. Hata ile tolerans sonucu ayrı kalır.
4. **MUI-04 — Kullanıcı rasteriyle kabul.** Temiz gerçek görüntü, mevcut reçete,
   ROI ve tam hata; siyah/beyaz ve ters yön; az ok/dar halka/kısa çizgi/dar yay;
   mm/px geçişi, reference/world yolları, yanlış kenar ve hasarlı kontur.

Kabulde yalnız analitik yumuşak kenarlar yeterli değil: anti-alias olmayan keskin
8-bit kenar, farklı subpixel faz/açı, Gray16 normalize aralık, gerçek telecentrik
raster ve yanlış kenar örnekleri de gerekli. Her ret doğru öneriyi üretmeli;
geçerli fit yokken ölçüm yok; gerçek kusur outlier diye toleranstan çıkarılmamalı.
Save/reopen/no-op, immutable Runtime, invalidation ve Stop korunmalı. UI
uygulandığında yeni Release build, ilgili CTest ve 1400×900/850×600 görsel kontrol
zorunlu. Eski test başarısı gerçek görüntü kabulü yerine kullanılamaz.

## 6. Bu analizde yapılan doğrulama

Mevcut 18 Ekim derlemeleriyle 19 Ekim'de **6/6** seçili CTest yeniden çalıştırıldı:
`line_fit_contract`, `circle_fit_contract`, `circle_metrics_contract`,
`radial_scan_contract`, `edge_point_contract`, `calibrated_circle_contract`.
Çıktı: `build/measurement-analysis/ctest.txt`, toplam 6.14 saniye.
Bu yeni koşum mevcut test corpus'unun geçtiğini gösterir; kullanıcının problemi
yeniden üretilmiş veya düzeltilmiş değildir. Kod/UI değişmedi; yeni Release
derleme ve görsel kabul yapılmadı. Formül örnekleri statik incelemedir.

Öncelikli devam: MUI-01. Kullanıcıdan tam hata/raster/reçete gelirse ilk iki
“insufficient” sınıfından hangisi olduğu daraltılır; bağımsız iyileştirme analizi
bu veri gelmedi diye bloke edilmez. ICM native/fiziksel kabul engelleri sürer.
