# ICM-04d — tetik korelasyonu kararı

17 Ekim 2026. **Karar: mevcut actual counter epoch sözleşmesini koru; native
backend capability Unavailable. FrameSpecInfo araştırma yolu açık.**

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

1. Raster dışında actual metadata yolu veya üretici tarafından doğrulanmış temiz
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
