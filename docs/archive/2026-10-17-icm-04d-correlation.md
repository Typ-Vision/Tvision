# ICM-04d — FrameSpecInfo, korelasyon kararı ve capability sınırı

Tarih: 17 Ekim 2026. Uygulayıcı: Codex.
Durum: **araştırma/erken ret alt adımı tamam; ICM-04d hâlâ kısmi**.
[Önceki kapalı metadata tanısı](2026-10-17-icm-04d-native-diagnostic.md) korunur;
oradaki sıfır sayaç gözlemi bu kayıtta daraltılır. Kalıcı karar
[tetik korelasyonu sözleşmesindedir](../architecture/industrial-camera-trigger-correlation.md).

## Bulgu ve karar

SFNC Chunk düğümleri dışında cihaz XML'inde **FrameSpecInfoSelector/FrameSpecInfo**
bulundu. Sandbox dışı read-only actual destek listesi 0/1/2/3/5/6/7/8.
Timestamp(0), Framecounter(5), ExtTriggerCount(6) bankaları false durumundaydı.
Bu bankaları snapshot/readback ile geçici açınca **iki ayrı oturumun her birinde
100 tetik/100 buffer; nFrameCounter ve nTriggerIndex 1..100** elde edildi.
Önceki kapalı metadata tanısındaki0, cihazda sayaç yokluğu kanıtı değildir.

Fakat son binary'nin 100 örneğinin tamamında raster byte **4..7 frameCounter**,
**8..11 triggerIndex** ile little-endian uint32 olarak birebir eşleşti. İlk64 byte
her örnekte kaydedildi. Örnek1 prefix `b98076000100000001000000`, örnek2
`bf0942010200000002000000`. Metadata sensör rasterinin içine yazılıyor; SDK bu
byte'ları temiz sensör pikseli olarak geri getirmiyor. Payload değişmemesi yeterli
değil. Bu görüntü mevcut full-sensor Mono8 ölçüm yoluna alınamaz. Sessiz kırpma,
sıfırlama veya maskeli ölçüm sözleşmesi eklenmedi.

Ready/quiescence ve ilk command öncesi actual baseline ayrıca açık. İki yeniden
açılışta sayaçların1'den başlaması bütün reset/cancel/stop-start durumlarına garanti
sayılmadı. Native backend'in kanıt stratejisi bu yüzden Unavailable kalır; sayaç
alanlarının varlığı native arm/command uygulaması anlamına gelmez. FrameStart
şartı gevşetilmedi; ICM-04e başlamadı.

## Değişen dosyalar

- `OecsCApi.hpp`: typed `OecsTriggerCorrelation {Unavailable,ActualCounterEpoch}`;
  default Unavailable. Backend uygulama yeteneği sorgusudur, SDK işlemi yapmaz ve
  per-request ready kanıtı değildir.
- `OecsSession.hpp/.cpp`: owner worker/idle-open capability sorgusu. Direct
  softwareFrame, implementation capability yoksa feature/pool/start/flush/command
  öncesi Unavailable/trigger-correlation/errorUnsupported verir. Idle session
  bu ret yüzünden Faulted olmaz; generation harcanmaz. Destek ilan edilmiş yolda
  eski arm/generation/exact modulo sayaç/evidence kontrolleri korunur.
- `IndustrialSoftwareCapture.cpp`: adapter unconditional softwareTrigger=true
  bildirmez; session'ın implementation capability'sini kullanır. Generic
  AcquisitionService unsupported ERROR üretir; frame/evidence yoktur.
- `oecs_trigger_test.cpp`: başarılı evidence fixture explicit opt-in; unavailable
  application/direct çağrısı için feature/stream/trigger/copy sıfır, tekrar ret,
  idle session ve closed-session sorgu kontrolü. Native/SDK'sız factory varsayılanı
  Unavailable. Önceki false-DONE/cancel/deadline corpus'u çalışmaya devam eder.
- `oecs_binding_test.cpp`: gerçek dynamic binding SDK export varlığını support'a
  çevirmiyor; default arm/command unsupported kalıyor.
- `tests/industrial-camera-probe/main.cpp`: FrameSpecInfoSelector/FrameSpecInfo
  salt okunur inventory'ye eklendi; önceki CLI davranışı korundu.
- Aynı dizindeki `trigger.cpp`: isteğe bağlı `--frame-spec`; üç bankanın enable
  snapshot'ı ve initial selector kaydedilir. Selector write/readback sonrası enable
  yazılır; her bank restore/readback ve selector restore yapılır. Hata yolunda da
  tüm restoration denenir. FrameSpecInfo payload boyutunu değiştirirse açık ret.
  Opt-in modda ilk64 raster byte tanı metadata'sına yazılır. Ürünle linklenmez;
  productionAdmission/readyProven/counterEpochProven=false kalır.
- Karar belgesi, contract-plan, README kullanım bağlantısı, progress ve arşiv dizini.
  UI, recipe/schema ve `config/local.ui.ini` değişmedi.

## Fiziksel kanıt ve kullanım

Exact: **Opto Engineering Srl / COE-050-M-USB-050-IR-C / 02A2220000627**.
`build/icm/04d-contract/metadata-native.json`: read-only actual destek.
`frame-spec-100.json` ve `frame-spec-final.json`: iki100 örnek koşumu. Her ikisinde
command→buffer33 ms; ilk/ara50 ms quiet pencerelerde buffer yok. nFrameNum0..99,
actual nFrameCounter/nTriggerIndex1..100. Metadata ve trigger ayarları restore=true;
tüm free/clear/stop/restore/close/destroy status0.
`metadata-after.json`: yeni read-only açılışta orijinal trigger/acquisition ve aktif
FrameSpecInfo Timestamp bankası/selector aynı; diğer iki bankanın restore readback'i
tanı içinde ayrı doğrulandı. `validated-summary.json`: JSON assertion kontrolü,
100 örnek için byte4/8 sayaç eşleşmeleri ve bağımsız readback sonucu.

Önceki kayıtla aynı opt-in Release probe build ve child PATH kullanılır. Yeni mod:

```powershell
& build/icm/02/abi/oecs_trigger_diagnostic.exe `
  'C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll' `
  'Opto Engineering Srl' 'COE-050-M-USB-050-IR-C' '02A2220000627' --frame-spec
```

Bu komut geçici cihaz ayarı yazar; varsayılan modsuz tanı eski davranışını korur.
UserSet/firmware/ROI/exposure/gain yazılmaz. Ani process kill/USB kopmasında restore
garantisi yok; böyle durumda actual ayar kontrolü gerekir. Native support sınırı
değişmediği için bu görevin ürün ret testi gerçek kameraya yeni tetik göndermez.

## Yeni doğrulama

Release build core `build/kmc9-core-off`, UI `build/reference-ui`, SDK
`build/icm/04a/sdk` ve opt-in probe exit0. İlgili CTest core **9/9**, UI **9/9**,
SDK **10/10**. Seçim:
`industrial_|owned_frame_contract|camera_lifecycle_contract|acquisition_contract|camera_transport|session_contract`.
Loglar `build/icm/04d-contract/{core,ui,sdk}-{build,tests}.log`; her build'de
`icm-04d-contract-tests.xml`. Probe logları `metadata-probe-build.log`,
`spec-probe-build.log`, `spec-final-build.log`. Bağlam kontrolü `context-check.json`.
Tam suite/UI görsel kontrol koşulmadı; UI değişmedi. Diagnostic restore-failure,
USB kopma ve metadata selector fault enjeksiyonu bu koşumda yapılmadı.

## Açık sınırlar ve sonraki iş

ICM-04d için **raster dışında actual frame/trigger metadata + ready/quiescence
semantiği** bulunmalı. Alternatif görüntü maskeleme/kırpma full-sensor geometri ve
kalibrasyon sözleşmesini değiştirir; bu teslim bunu kabul etmez. Actual counter
epoch kurulmadan native capability açılmaz. Stop/start/cancel/reset/wrap/reopen ve
metadata/baseline doğrulaması bu yolun bağımlılıklarıdır. Karar belgesi somut kabul
koşullarını ve üreticiye sorulacak semantiği içerir; mesaj gönderilmedi.
G0–G4/H01–H08, H02 sahne/tazelik, 1000 cycle/stress, unplug/replug, dağıtım,
lens/ölçüm ve önceki engeller açık. 04e'ye geçilmedi; push yapılmaz.

Alt adım ICM-04d görev ID'li tek commit ile kaydedilir.
