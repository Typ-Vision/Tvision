# ICM-04d — raster dışı metadata ve ready/epoch incelemesi

Tarih: 17 Ekim 2026. Uygulayıcı: Codex.
Durum: **araştırma alt adımı tamam; ICM-04d dış teknik kanıt bekliyor**.
Önceki [correlation kaydı](2026-10-17-icm-04d-correlation.md) değiştirilmedi.

## Kapsam ve bulgu

Yerel exact cihaz XML'i, OECS C API header'ları ve
[üretici COE-U User Manual V1.0](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf)
§3.10.2 incelendi; yeni native salt okunur sorgu alındı. Kılavuz görüntüye gömülü
metadata'yı açıklar. İncelenen exact XML'de Chunk/Event adlı node yok;
SFNC alternatifleri actual API'de `0x80000100` verdi. Bu kodlar 0 sayaç veya
cihaz genelinde hiçbir alternatif yokluğu diye yorumlanmadı.

Exact kimlik: Opto Engineering Srl / COE-050-M-USB-050-IR-C / 02A2220000627.
DeviceVersion `V1.4.8 190411 262291`; firmware `V1.4.8 190411,19012902`;
SDK raw version `0x03050200`. TriggerSelector yalnız 6; burst=1, TriggerMode Off,
cache=false, aktif FrameSpecInfo Timestamp false. Banka değiştirilmedi.

XML bağımlılıkları hazır kanıtı için yeni engelleri somutlaştırdı:

- TriggerSoftware erişim formülü yalnız SRC=7 ve MODE=1. WO ready anlamına gelmez.
- AcquisitionStatus line-scan türüne bağlı; selector Missing, status Unavailable.
- LineSource yalnız ExposureActive içerir; FrameTriggerWait alternatifi yok.
- TimestampLatch WO; DeviceTimestamp ve increment RO/0. Latch çalıştırılmadı;
  0 desteğin yokluğu sayılmaz. XML ns ve ns/tick birimleri SDK buffer timestamp
  olayı veya embedded timestamp formatı ile eşleştirme kanıtı değildir.
- AcquisitionStop açıklaması mevcut frame sonunda durmadır; API dönüşünün o sonu
  beklediği ve SDK StopGrabbing/Clear'ın eski exposure bariyeri kurduğu belgelenmedi.
  Clear API açıklaması acquisition sürerken buffer temizliğidir.

Native capability Unavailable kalır. Yeni trigger, stream, grab, reset, firmware,
UserSet, ham register veya ayar yazma yapılmadı. Üreticiyle iletişim kurulmadı.

## Değişen dosyalar

- `tests/industrial-camera-probe/main.cpp`: read-only inventory'ye firmware/kimlik,
  timestamp latch, Chunk/Event, acquisition/line ready adayları eklendi. IString
  değerleri bounded okunur. Önceki CLI ve writesiz davranış korunur.
- `docs/architecture/industrial-camera-trigger-correlation.md`: yeni semantik
  kanıtlar, yorum sınırları ve dış bağımlılık.
- `docs/architecture/industrial-camera-vendor-questions.md`: exact firmware ve
  5 somut teknik soruyla gönderime hazır İngilizce taslak; **gönderilmedi**.
- Progress ve arşiv dizini. Ürün backend/UI/schema/config değişmedi.

## Yeni doğrulama / kanıt

`build/icm/04d-semantics/`:

- `probe-build.log`: mevcut opt-in Release yapılandırmada industrial_backend_probe
  build exit 0. Yalnız tanı executable'ı değişti.
- `inventory-sandbox.json`: sandbox 0 cihaz; fiziksel yokluk kanıtı değil.
- `inventory-native.json`: ilk genişletilmiş sorgu, 1 cihaz.
- `inventory-final.json`: string/firmware dahil son binary, 1 exact cihaz;
  create/open/readback/XML/close/destroy status 0, exit 0; grabAttempted=false.
- `02A2220000627.xml`: actual XML; SHA256
  `8fc783046fe5cdc990d308bbe33ba0cd08761f39b77029235879baa7cb5101c1`.
- `validate.py` / `validated-summary.json`: exact kimlik/cleanup/exit, XML formülü,
  Chunk/Event yokluğu, LineSource alternatifi ve input hash assertion'ları geçti.
  SDK header'ları repoya kopyalanmadı; yerel kaynak yolu/hash kaydedildi.
- `cli-invalid.json`: hatalı CLI exit 1, grabAttempted=false.
- `context-check.json`: bağlam ve belge bağlantı kontrolü.

Ürün/UI değişmedi; yeni CTest, tam suite veya görsel kontrol koşulmadı. Önceki
core/UI 9/9 ve SDK 10/10 sonuçları bu koşumun başarısı olarak sunulmaz.

## Açık sınırlar / sonraki görev

Mevcut kaynaklarla temiz full-sensor raster + actual trigger/frame metadata +
ready/quiescence/epoch sağlayan yol kurulamadı. Sıradaki iş üretici taslağındaki
sorulara belgeli yanıt veya eşdeğer yeni teknik kanıt; ardından uygun yolun ayrı
uygulama/fixture/fiziksel kabulü. Aynı 100 tetik tanısı tekrar edilmedi; latch
sayısı karşılaştırılarak tazelik ilan edilmedi. Mask/crop sözleşmesi açılmadı.
04d tamamlanmadı; 04e/G1 ve G0–G4/H01–H08, sahne/ölçüm/lens, stress/unplug,
dağıtım/CUDA ve önceki tüm fiziksel engeller açık. Tek ICM-04d commit; push yok.
