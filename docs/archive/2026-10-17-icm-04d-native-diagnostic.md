# ICM-04d — native trigger tanısı ve korelasyon sınırı

Tarih: 17 Ekim 2026. Uygulayıcı: Codex.
Durum: **tanı alt adımı tamam; ICM-04d production admission hâlâ kısmi**.
Önceki [çekirdek/fixture kaydı](2026-10-17-icm-04d.md) değiştirilmedi.

## Kapsam ve değişen dosyalar

- `tests/industrial-camera-probe/main.cpp`: 28 aday düğüm için önce gerçek access
  ve interface type, sonra türüne uygun getter. Integer değer/range string olarak
  kayıpsız; enum supported raw değerleri ayrı. Missing/unavailable/hatalı getter
  sıfır actual değer üretmez. Bu araç salt okunur kalır.
- Aynı dizinde `trigger.cpp` ve `CMakeLists.txt`: ürünün dışında, otomatik CTest'e
  eklenmeyen `oecs_trigger_diagnostic`. Exact vendor/model/serial tek eşleşme ister;
  owned cihaz kaydıyla açar. Yalnız sole FrameBurstStart bankası, başlangıç Off,
  Continuous, bounded tight Mono8 kabul eder. Source/mode/burst/cache snapshot alır;
  Software/On/burst=1/cache=false geçici yazar, actual readback doğrular.
- Dört SDK buffer; ilk clear, 50 ms tetiksiz başlangıç; 100 komutun her birinde
  monoton 1000 ms komut+wait bütçesi, bounded boyut doğrulaması ve raw metadata/hash.
  Her kare free sonrası 50 ms ek-frame kontrolü. Quiet timeout bir ready oracle'ı
  değildir. Geç veya ekstra buffer açık hatadır; borrowed buffer hata yolunda da
  free edilmeye çalışılır. Free → Clear → Stop ve bütün restore adımları denenir;
  restore readback/close/destroy hatası başarı diye sunulmaz.
- Kalıcı UserSet, firmware, ROI/exposure/gain, reçete/schema veya yerel INI değişmez.
  SDK image-node sayısı bu tanı handle'ına aittir. Raster kaydedilmez; hash yalnız
  tanı metadata'sıdır. Varsayılan çıktı her durumda productionAdmission/readyProven/
  counterEpochProven=false. Bu kod native ürün command/arm binding'i değildir.
- README kullanım bağlantısı, güncel progress, arşiv dizini ve acquisition sözleşme
  sınırı güncellendi. UI değişmedi.

## Kaynak ve gerçek düğüm kanıtı

Yerel cihaz XML'i `build/icm/04d/02A2220000627.xml`:
TriggerSelector=6 FrameBurstStart; CounterEventSource 0/11/13=Off/Line0/Line2.
XML'de başka event adları bulunması actual destek demek değildir.
AcquisitionStatus, `IsLineScan_Ava` koşuluna bağlıdır; unsupported register okuma
veya bu koşulu atlama yapılmadı. Yerel CameraParams.h nFrameNum, nFrameCounter,
nTriggerIndex alanlarını içerir; header'da bulunmaları cihazın bunları doldurduğunu
kanıtlamaz. SDK/vendor dosyaları Git'e eklenmedi.

[Üretici COE-U kılavuzu](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf),
s.22–24: burst sayısı tek tetik başına kare sayısını belirler; Counter bölümü harici
hatların frekans bölmesini anlatır. Bu açıklama ready veya counter epoch garantisi
sağlamaz. Burst=1 eşdeğerliği burada yalnız tanı gözlemidir; ürün FrameStart şartını
sessiz alias ile değiştirmez.

## Yeni fiziksel koşumlar

Exact: **Opto Engineering Srl / COE-050-M-USB-050-IR-C / 02A2220000627**.
`build/icm/04d-native/contract-sandbox.json`: sandbox 0 cihaz.
`contract-native.json`: sandbox dışı 1 exact cihaz, open/close/destroy 0.

Actual: AcquisitionMode Continuous(2); AcquisitionBurstFrameCount=1 (1..1023);
TriggerSelector yalnız 6; TriggerMode Off(0), Source Line0(0), Cache=false,
Delay=0, ExposureMode Timed(0). CounterSelector yalnız0, CounterEventSource yalnız
0/11/13; CounterCurrentValue okunabilir0 fakat software/frame event kaynağı yok.
TriggerCounter/FrameCounter/Chunk/Event sorguları 0x80000100. DeviceTimestamp ve
Increment okunabilir0; latch komutu çalıştırılmadı, zaman epoch'u çıkarılmadı.

İki ayrı sandbox dışı oturum:

| Kanıt (`build/icm/04d-native/`) | Sonuç |
| --- | --- |
| `trigger-100-native.json` | 100 komut/100 buffer; command→buffer 33–37 ms |
| `trigger-100-final.json` | Son binary: 100 komut/100 buffer; 33 ms |
| `contract-after.json` | Yeni açılışta orijinal mode/source/selector/burst/cache/AcquisitionMode aynı |
| `validated-summary.json` | JSON assertion kontrolü: sayılar, metadata, restore, cleanup ve bağımsız readback |

İki koşumda da ilk ve ara tetiksiz 50 ms pencerelerde buffer gelmedi. nFrameNum her
açılışta **0..99**, nFrameCounter ve nTriggerIndex **100 örneğin tümünde 0**.
Device ticks monoton arttı; hash kaydı tazelik kanıtı yapılmadı. Final koşumda stream
açık/On/Software iken access: AcquisitionStatusSelector=AM_NI(0),
AcquisitionStatus=AM_NA(1), TriggerSoftware=AM_WO(2). Command yazılabilirliği hazır
olma garantisi değildir. İki koşumda da tüm cleanup/restore kodları0 ve
settingsRestored=true; sonraki bağımsız readback de doğruladı. Başlangıç geometrisi
2448×2048 Mono8, exposure5000/gain0 korundu.

**Bu 2×100 gözlem H02 sahne/tazelik veya ICM-04d admission kabulü değildir.**
Sahne değişimi/raster görsel kontrolü yapılmadı. Stream-start/flush/quiet bekleme
eski exposure quiescence veya cihaz hazır kanıtı yerine geçirilmedi.

## Tekrar çalıştırma

Mevcut yerel opt-in Release proje: `build/icm/02/abi`; kaynak
`tests/industrial-camera-probe`. README'deki Qt/MinGW/CMake PATH'e eklenir.
Fiziksel erişimde sandbox 0 dönerse dış erişim gerekir. Runtime DLL dizini child
PATH'te olmalı; önce başka camera session/viewer kapalı olmalı.

```powershell
cmake --build build/icm/02/abi -j 6
$env:PATH = 'C:\Program Files (x86)\Common Files\OECS\Runtime\Win64_x64;' + $env:PATH
& build/icm/02/abi/oecs_trigger_diagnostic.exe `
  'C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll' `
  'Opto Engineering Srl' 'COE-050-M-USB-050-IR-C' '02A2220000627'
```

Başarı exit0 yalnız bounded tanı gözlemidir; eksik exact cihaz, argüman, ayar,
buffer, deadline veya cleanup/restore hatası exit1. Ani process kill/USB kopması
restore garantisi vermez; böyle hata sonrası ayar actual readback gerektirir.

## Yeni yazılım doğrulaması ve sınırlar

Probe Release build ve SDK Release build exit0. İlgili SDK CTest **10/10**:
`industrial_|owned_frame_contract|camera_lifecycle_contract|acquisition_contract|camera_transport|session_contract`.
`build/icm/04d-native/{probe-build,trigger-build,trigger-final-build,sdk-build,sdk-tests}.log`;
JUnit `build/icm/04a/sdk/icm-04d-native-tests.xml`.
`invalid-arguments.json` ve sandbox `absent-exact-device.json`: exit1, komut0/kare0.
Gerçek kamera command/timeout/cancel/restore-failure enjeksiyonu yapılmadı; bunlar
bu executable için kabul iddiası değildir. Ürün çekirdeğinin fixture testleri ayrı.
Core/UI suite ve görsel kontrol yeniden koşulmadı; UI/ürün kodu değişmedi.
Bağlam kontrolü `build/icm/04d-native/context-check.json`.

## Engel ve sonraki görev

**ICM-04d açık**: actual trigger counter ve stream-ready/epoch garantisi bu cihazda
bulunamadı. Aynı 100/100 denemesini tekrarlamak bu açığı kapatmaz. Sonraki iş vendor
ready/quiescence/metadata garantisini bulmak veya mevcut actual counter şartına
alternatif, hata/late/reset/cancel/reopen yolları kanıtlanabilir acquisition
sözleşmesini tanımlayıp doğrulamaktır. Kanıt yoksa backend açık unsupported kalır;
FrameBurstStart otomatik FrameStart alias'ı olmaz. 04e başlamaz.
G0/H01 OECS sahne/release, G1/04e, H02 sahne, 1000-cycle/teardown, unplug/replug,
dağıtım/lens/ölçüm ve önceki fiziksel kapılar açık kalır.

Alt adım ICM-04d ID'li tek commit ile kaydedilir; push yapılmaz.
