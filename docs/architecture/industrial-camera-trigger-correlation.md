# ICM-04d — tetik korelasyonu kararı

17 Ekim 2026. **Uygulanan yol ActualCounterEpoch; native capability Unavailable.
Bu strateji tek olası çözüm değildir. Yeni timestamp deneyi doğrudan saat
kıyaslamasını çürüttü; alternatif stratejilerin kanıt koşulları aşağıdadır.**

## Kanıt ve kararın sınırı

[Önceki tanıda](../archive/2026-10-17-icm-04d-native-diagnostic.md) nFrameCounter ve
nTriggerIndex, metadata kapalıyken 0 kaldı. Bu sonuç cihazın sayaç sağlayamadığı
anlamına gelmez. Cihaz XML'i SFNC Chunk dışında FrameSpecInfoSelector/FrameSpecInfo
sunuyor; actual destek listesi 0/1/2/3/5/6/7/8. XML karşılıkları Timestamp(0),
Framecounter(5), ExtTriggerCount(6). Üç enable bankası başlangıçta false.

[Yeni tanı](../archive/2026-10-17-icm-04d-correlation.md) bu üç bankayı geçici açınca
iki ayrı 100 tetik/100 buffer'da sayaçların 1–100 ilerlediğini gösterdi. Bu, actual
metadata yoludur; default native ürün binding'i bunu henüz kullanmaz. nFrameNum
ile nFrameCounter farklı alanlardır; mevcut native hardwareFrameId eşlemesini
kanıtsız biçimde actual frame counter diye yeniden etiketlemeyin.

Final koşumun 100 örneğinde raster byte 4–7 little-endian uint32=nFrameCounter,
byte 8–11=nTriggerIndex. Bu metadata rasterin içindedir; aynı payload uzunluğu
temiz sensör pikseli anlamına gelmez. Bu nedenle mevcut full-sensor/owned Mono8
ölçüm sözleşmesi altında FrameSpecInfo etkinleştirilmiş görüntü kabul edilmez.
Metadata'yı okuyup bu byte'ları sıfırlamak da kayıp sensör değerlerini geri getirmez.

## Aday yollar

| Yol | Kanıt / eksik | Karar |
| --- | --- | --- |
| FrameStart + actual counters + verified ready/quiescence | Çekirdek fixture var, cihaz sole FrameBurstStart | Genel sözleşme korunur, cihaz yolu açık |
| FrameBurstStart, burst=1, cache=false + FrameSpecInfo | Actual sayaçlar var; raster byte 4–11 metadata | Mevcut full-sensor sözleşmesinde admission yok |
| Quiet timeout + sonraki buffer | Yalnız belli sürede buffer gelmediği gözlemi | Ready/quiescence kanıtı kabul edilmez |
| Start/Stop/flush/reopen + ilk buffer | İncelenen SDK header'ı acquisition start/stop anlatır; eski exposure bariyeri garantisi sunmaz | Host generation hardware epoch yerine geçmez |
| Device ticks veya hash | Artış/değişiklik görüldü; command ilişkisi/baseline yok | Tek başına yeterli değil |
| CounterCurrentValue | Kaynaklar yalnız Off/Line0/Line2 | FrameSpecInfo'dan ayrı, software oracle'ı değil |

Kamera buffer alabiliyor; ürün backend'inin korelasyon yeteneği bundan ayrı bir
sözleşmedir. `OecsCApi::softwareTriggerCorrelation()` cihazı sorgulamaz, backend'in
uyguladığı kanıt stratejisini bildirir. Varsayılan Unavailable; SDK'sız ve native
dynamic binding de bu durumdadır. Yalnız ActualCounterEpoch uygulayan backend
software trigger capability ilan edebilir. Bu ilan actual ready kanıtı değildir;
prepareSoftwareTrigger ve frame doğrulaması her istekte ayrıca başarılı olmalıdır.
Adapter ve doğrudan session çağrısı unsupported'ı feature/pool/start/flush/command
öncesi reddeder. Idle oturum bu ret yüzünden Faulted olmaz.

## FrameSpecInfo yolunu açmadan gerekenler

1. **Önceki full-sensor şartı; aşağıdaki yeni kullanıcı kararıyla daraltıldı.**
   Raster dışında actual metadata yolu veya üretici tarafından doğrulanmış temiz
   sensör rasteri bulunmalı. Gösterilmiş metadata byte'larını sessizce kabul etme,
   sıfırlama veya kırpma yok. Maskeli görüntü farklı bir geometri/kalibrasyon ürün
   sözleşmesi ister; bu görev bunu açmaz. FrameSpecInfo enabled bankaları/selector
   diagnostic snapshot/rollback kapsamında kalır; inactive bank state kaybedilmez.
2. İlk accepted command öncesi actual frame/trigger baseline ve eski exposure
   quiescence kanıtı kurulmalı. Sonraki buffer'a beklenen generation atamak yasak.
   İlk buffer'da 1 görmek her açılış için baseline0 garantisi vermez.
3. Ready eksikliği için cihaz/SDK semantiğiyle doğrulanabilir bariyer bulunmalı.
   DeviceTimestampLatch adayının command/metadata zaman birimi ve exposure ilişkisi
   kanıtlanmadan tick karşılaştırması eklenmez. Ham register ile access koşulu atlanmaz.
4. Trigger index'in software tetikleri, rejected command, cache, burst ve reset
   davranışı sınanmalı; reset/wrap/late/cancel/reopen sahte DONE vermemeli.
5. FrameBurstStart/burst1 için ayrı explicit profil/validation kararı ve fixture
   gerekir; FrameStart string alias'ı yapılmaz. Native metadata mapping ve raster
   kabulü birlikte doğrulanmalı. Mevcut private cycle/frame, monoton bütçe, tek
   in-flight, owned-copy ve cleanup kuralları değişmez.

Sonraki iş raster dışında sayaç/trigger metadata yolu ve hazır/quiescence semantiği
araştırmasıdır. İki reopen koşumunda sayaçların 1'den başlaması her durumda baseline0
garantisi değildir; stop/start/cancel/reset davranışı henüz sınanmadı. Aynı 100/100
tanısını tekrarlamak bu engelleri kapatmaz. Üreticiye sorulacak somut konular:
Stop/Clear eski exposure bariyeri, ready olmadan software command davranışı,
FrameSpecInfo frame/trigger reset kapsamı ve raster dışı eşdeğer metadata yolu. Bu karar
üreticiyle mesajlaşma yetkisi veya üretici onayı içermez.

[Üretici COE-U kılavuzu](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf)
tek tetik/burst davranışını anlatır; incelenen bu açıklama ready/counter epoch
garantisi sağlamaz. Yerel SDK header ve cihaz XML'i destek/semantik için ayrı
kaynaklardır. ICM-04d tamamlanmadı; ICM-04e henüz başlamaz.
## Raster dışı yol incelemesi — 17 Ekim 2026

[Yeni salt okunur tanı](../archive/2026-10-17-icm-04d-semantics.md) firmware
`V1.4.8 190411,19012902` üzerinde alternatif Chunk/Event/ready düğümü bulamadı.
Bu, başka firmware veya üreticiye özel API bulunmadığı iddiası değildir.
SDK'da `MV_FRAME_OUT_INFO_EX` içindeki sayaç ve unparsed chunk alanlarının
bulunması cihazın raster dışı metadata gönderdiğini kanıtlamaz.

- XML `TriggerSoftwareAvailInq_Int` formülü yalnız `SRC=7 && MODE=1` kontrol eder;
  command'ın WriteOnly olması exposure-ready veya kabul edilmiş tetik kanıtı değildir.
- `AcquisitionStatus` erişimi `IsLineScan_Ava` ile line-scan cihaz türüne bağlıdır.
  Area-scan cihazdaki unavailable durumu ham register erişimiyle aşılmaz.
  `LineSource` yalnız ExposureActive içerir; FrameTriggerWait output alternatifi yoktur.
- `DeviceTimestampLatch` WriteOnly, `DeviceTimestamp` ve increment ReadOnly'dir;
  latch çalıştırılmadan iki sayının 0 okunması timestamp desteği yokluğu değildir.
  XML timestamp'i ns, increment'i ns/tick olarak tanımlar; bu bilgi buffer tick'inin
  hangi exposure/transport olayına ait olduğunu veya sayaç epoch'unu açıklamaz.
  Increment düğümündeki ImposedAccessMode RW, alttaki register RO'yu yazılabilir yapmaz.
- XML AcquisitionStop'u mevcut frame sonunda durma olarak tanımlar. Komut dönüşünün
  bu sonu beklediği, SDK StopGrabbing'in bununla aynı bariyeri kurduğu veya Clear'ın
  daha sonra gelecek eski exposure'ı engellediği belgelenmemiştir. SDK Clear açıklaması
  acquisition sürerken buffer temizlemeyi anlatır; exposure iptal garantisi vermez.

[Üretici kılavuzu §3.10.2, s.35–36](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf)
metadata'nın görüntüye gömülmesini açıklar ve önceki byte bulgusunu destekler.
Buradaki 32-bit embedded timestamp formatı ile DeviceTimestamp ns değeri veya
SDK buffer timestamp'i eşdeğer varsayılmaz. İncelenen kaynaklarla mevcut full-sensor
sözleşmesini sağlayan native yol kurulamadı; Unavailable korunur.

Sonraki bağımlılık [hazır üretici soru taslağına](industrial-camera-vendor-questions.md)
yanıt veya bu koşulları sağlayan yeni teknik kanıttır. Taslak gönderilmedi.
Yeni kanıt gelmeden aynı 100-trigger koşumunu veya latch/tick karşılaştırmasını
tekrarlamak admission engelini kapatmaz; 04e hâlâ başlamaz.

## Yeniden değerlendirme ve timestamp karşı örneği — 17 Ekim 2026

[Yeni fiziksel deney](../archive/2026-10-17-icm-04d-timestamp.md) önceki sıfır latch
okumasını daralttı: DeviceTimestampLatch çalışıyor, değer pozitif ve artıyor.
Metadata'nın desteklenen sekiz bankası false iken 24 tam Mono8 buffer alındı.
Ancak latch ile SDK nDevTimeStampHigh/Low aynı sayısal ölçek/başlangıç değildir:
bu koşumda host monoton saniyesine yaklaşık 148.502 milyon ve 1.250014 milyar
birim ilerlediler. DeviceTimestampIncrement hâlâ 0. Bu oranlar gözlemdir;
dönüşüm sabiti, saat senkronizasyonu veya firmware hatası kanıtı değildir.

12 karede command sonrası 150 ms beklenip, **yeni command göndermeden** latch
alındı ve önceki command'ın buffer'ı okundu. 12/12 eski-command buffer'ı için
`frameTicks > latchBeforeReceive` doğru çıktı; 24/24 frameTicks, alımdan SONRAKİ
latch'ten de büyüktü. Dolayısıyla ham sayısal `>` testi yanlış tazelik üretir.
Başlangıç offset'i çıkarmak veya ölçülen oranla düzeltmek de belgeli olay/clock
domain/reset ilişkisi sağlamaz. Rapor timestamp yoluna admission vermez.

### Güvenlik hedefi ile uygulama stratejisini ayır

Zorunlu hedef: döndürülen temiz kareyi tam olarak bu isteğin tek tetiğine bağlamak;
belirsizlikte ERROR. İki sayaç ve ayrı bir ready düğümü bu hedefin tek ispatı
değildir. Mevcut ActualCounterEpoch kodu ve fixture sözleşmesi değiştirilmedi.
Başka bir strateji ayrı typed capability/arm/evidence ve kabul testleri ister.

Alternatif **bariyer + tek bekleyen tetik** için yeterli koşullar:

1. Belgeli ve sınanmış başlangıç bariyeri eski exposure, gecikmiş command ve
   device/transport/SDK kuyruklarından sonraki epoch'a kare geçmesini engeller.
   Sadece host flush veya sessiz bekleme bu koşul değildir.
2. Exclusive sahiplik, On/Software, explicit FrameBurstStart/burst=1/cache=false,
   diğer tetik yollarının kapalı olması ve trigger delay politikası doğrulanır.
   Etkin epoch'ta tek command bekler; kaynaksız/duplicate teslim reddedilir.
3. Device/transport tesliminin bir command'dan en fazla bir tamamlanmış frame
   verdiği ve eski buffer'ı yeniden vermediği belgelenir ve sınanır.
4. Yeni istek ancak öncekinin tamamlanmış frame'i tüketilince açılır. Timeout,
   cancel, command failure veya bağlantı belirsizliği epoch'u zehirler; yeni
   başlangıç bariyeri kurulmadan retry/reopen ile admission yapılmaz.

Bu koşullarda ready olmadığı için **düşürülen** command timeout üretir, başka
çevrime ait kare üretmez. Ready sinyali kullanılabilirliği iyileştirir; tek başına
bir güvenlik şartı değildir. Tersine, ertelenmiş command veya eski exposure
kalabiliyorsa bu çıkarım geçersizdir. Exact firmware için bariyer ve teslim
semantiği henüz kanıtlanmadı; bu bir uygulanmış ürün yolu değildir.

Alternatif timestamp stratejisi ise belgeli ortak clock, exposure olayı, wrap/reset
ve sıralama gerektirir; bu koşumdaki iki ham alan bu stratejiyi sağlamaz.
FrameSpecInfo + açık geçerli alan seçeneği aşağıdaki kullanıcı kararıyla açıldı;
tek başına epoch sorununu çözmez. Sessiz crop/mask yoktur.

USB3 görüntü lideri chunk olmadan timestamp/frame ID taşıyabilir; örneğin
[Aravis'in U3V kaynak tanımı](https://raw.githubusercontent.com/AravisProject/aravis/main/src/arvuvspprivate.h)
bunu gösterir. **Chunk node yokluğu, transport metadata yokluğu değildir.** Bu
kaynak exact OECS alan mapping'ini veya exposure semantiğini kanıtlamaz.
[2023 üretici kılavuzu](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual_1.2.0_20230428.pdf)
§3.6/3.11.2 tek/burst tetik ve embedded bilgiyi açıklar; tüm fonksiyonların her
cihazda mevcut olduğunu garanti etmez. Başka kamera belgeleri exact kanıt sayılmaz.

Sıradaki karar tek bir doğrulanabilir stratejidir: eski beş sorunun tamamının
aynı anda sağlanması gerekmez. [Soru taslağı](industrial-camera-vendor-questions.md)
bu ayrımı ve yeni sayısal karşı örneği içerir. Native capability ve 04e kapısı
kanıt sağlanana kadar değişmez.

### Kullanıcı kararı: ilk sensör satırını ölçümden çıkar

Kullanıcı bu incelemede **ilk satırın ölçüm dışında bırakılmasına izin verdi**.
Bu karar önceki her full-sensor pikselini koruma şartını bu profil için değiştirir;
counter/epoch garantisini gevşetmez. Yeni exact-device tanıda metadata açık 12
karenin byte 4–7/8–11 alanları SDK sayaçlarıyla eşleşti. İlk kareden row 0 açıkça
çıkarılarak 2448×2047 owned Mono8 PGM üretildi; image hash ve origin kaydedildi.

- Sensör edinimi hâlâ 2448×2048, OffsetX/Y=0; donanım ROI yazılmaz.
- Ölçüm rasteri 2448×2047, sensör başlangıcı `(0,1)`; dönüşüm
  `sensorX=imageX`, `sensorY=imageY+1`. Metadata satırı doldurulmaz/interpole edilmez.
- Yalnız Timestamp/Framecounter/ExtTriggerCount açık, diğer desteklenen bankalar
  kapalı ve layout doğrulanmış olmalı; değişen layout açık ret verir.
- Ürün entegrasyonunda bu çıktı politikası geometri/kalibrasyon kimliğine dahil
  edilmeli; 2448×2048 için hazırlanmış model/kalibrasyon sessizce yeniden kullanılamaz.
  Sensör offset'i ile yazılım raster başlangıcı ayrı alanlardır.
- **Şu an yalnız opt-in tanı çıktısıdır.** Native adapter, persistent geometri
  şeması, kalibrasyon ve UI henüz bu politikayı uygulamaz; capability Unavailable.

Dolayısıyla raster engeli için yetkilendirilmiş ve fiziksel örneği alınmış yol var;
kalan öncelik ready/epoch başlangıcı ve hata sonrası recovery bariyeridir. Üretici
soru taslağında raster dışı metadata artık zorunlu değil, alternatif olarak geçer.
