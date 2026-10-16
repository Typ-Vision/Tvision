# KMC-1 — Kamera, kurulum ve kaynak kimliği sözleşmesi

16 Ekim 2026. Bu paket domain/application değer tipleri ve strict JSON codec sunar.
Yeni UI, registry dosyası, reçetesiz capture, aktivasyon veya çok kaynaklı Runtime
henüz uygulanmadı. KMC-0 baseline UI 70/70, OFF/core 61/61 geçti.

## Kimlik ve revizyon

`CameraId` proje içi mantıksal kimliktir; görünen ad, sıra, webcam index veya
ToolDefinition.id değildir. `SetupId` aynı kameraya bağlı ölçüm profilinin kimliğidir.
`MeasurementSetupDescriptor` optik profil, çalışma düzlemi, cihaz eşleme revizyonu
ve raw geometri taşır. `revised` isim değişiminde kimlik/revizyonu korur; optik,
düzlem, geometri veya cihaz revizyonu değişince revizyonu artırır. Başka CameraId'ye
aynı SetupId ile taşıma reddedilir. Geçmiş revizyonların kalıcı saklanması KMC-2'dir.

`DeviceBinding` adapter, provider'ın bildirdiği gerçek kimlik, kimlik güven düzeyi,
eşleme revizyonu ve yetenek beyanıdır. `unavailable` durumda providerId boştur;
eksik seri numarası oluşturulmaz. `provider_reported` işletim sistemi/USB portu
değişiminde kalıcılık garantisi değildir. `serial_reported` de yalnız adapter'ın
gerçekten bildirdiği seri numarası için kullanılabilir. `resolve` tam bir keşif
eşleşmesi ve aynı yetenek beyanını ister; sıfır/çoğul sonuç veya unavailable kimlik index fallback yapmaz.
`rebound` fiziksel kimlik/yetenek değişiminde revizyon üretir. Codec/discovery değeri
fiziksel doğrulama, bağlantı veya üretim izni değildir.

## Geometri ve cache

`GeometryDescriptor` actual raster çözünürlüğü, varsa sensör ROI/binning ve kritik
geometrinin bilindiği `complete` beyanını taşır. Null ROI/binning desteklenmeyen veya
bildirilmeyen alandır; 0 ya da sahte 1×1 binning yazılmaz. ROI sensör pikselidir;
ROI ve binning birlikte mevcutsa çıktı çözünürlüğüyle tam tutarlılık aranır.
Raw piksel merkezleri, +X sağ/+Y aşağı sözleşmesi korunur; rotasyon/flip/undistort
bu şemada desteklenmez. Adapter bunları dışlayamıyorsa complete=false kalır.

`geometryId` setup/camera/revision, optik/düzlem, deviceRevision ve geometriyi SHA256
ile bağlar; görünen adı dışlar. complete=false hash uyumu üretemez. `cacheId`
geometri kimliğine exposure/gain gibi **geometrik olmayan** actual edinim ayarlarını
ekler; bu ayarların değişimi geometrik kalibrasyon kimliğini değiştirmez. Geometri
hash'i tek başına aktiflik/actual kaynak uyumu kanıtı değildir. Bilinmeyen kritik
ayarları nonGeometricSettings'e koyarak uyumlu saymak yasaktır.

`MeasurementSetupContract` N kamera/kurulum için bağımsız codec'tir. N=0 geçerli;
varsayılan kapasite 128, çağıranın açık kapasitesi 1–4096 olabilir. Kamera ve setup
listelerinin her biri bu bütçeye tabidir. Taşma açık hatadır, truncation yoktur.
Duplicate camera/setup kimliği, dangling camera, aynı fiziksel kimliğin iki kartta
eşlenmesi ve eski deviceRevision reddedilir. Bir kamerada farklı SetupId profilleri
mümkündür; operasyonel seçim bu codec'e dahil değildir.

## Kaynak ve köken

Application `SetupReference` exact CameraId/SetupId/revision değeridir.
`MeasurementSourceEvidence` gerçek sourceId, frameId/cycleId/raw coordinates,
decoded pixel SHA256, işlem generation, explicit sourceKind (recipe_tool / calibration_session) ve ayrı hedef referansı taşır. sourceId
recipe acquisition tool'a veya calibration session kaynağına aittir; mantıksal
kamera kimliği olarak kullanılmaz. Stringler codec'te kimlik biçimi açısından
denetlenir; worker ticket, raster erişimi/hash hesaplama ve trusted completion
KMC-3/sonraki servislerin sorumluluğudur. Diskte yazılabilir evidence izin vermez.

| Origin | Hedef | Dosya / kanıt |
| --- | --- | --- |
| camera_acquired | Zorunlu | Dosya yolu boş; actual frame/cycle/hash/generation zorunlu |
| setup_file | Zorunlu | Mutlak dosya yolu ve gerçek raster kimlikleri; hedef açık kullanıcı beyanıdır |
| trial_file | Yok | Mutlak dosya yolu ve gerçek raster kimlikleri; aktivasyon anlamı taşımaz |
| legacy_record | Opsiyonel açık eşleme | generation=0; eksik frame/hash boş kalır, cycle/coordinates/path üretilmez |

Dosya yolu hedef kamera kimliğine girmez. Bu paket association kaydı veya legacy
evidence dönüştürücüsü yazmaz. Legacy raster kanıtı yokluğu korunur.

## Adapter ve mevcut servis sınırları

- Webcam mevcut `WebcamSettings` index/backend yoluyla çalışır. Yeni binding henüz
  Qt provider discovery'ye bağlanmadı; kalıcı fiziksel kimlik desteği iddia edilmez.
- GenTL mevcut discovery/read-only bağlantı kontrolüdür. Yeni codec industrial
  softwareCapture=true beyanını reddeder; grab/SDK uygulaması eklenmedi.
- `ToolDefinition` mevcut ID/parametre/tek kaynak sözleşmesini korur. Yeni source
  binding daha sonraki KMC-7/10 paketlerinde açık sürümlenecek.
- `CalibrationCapture` halen ToolDefinition kabul eder. KMC-3 application-owned
  kaynak isteği ve adapter çözümlemesiyle genişletecek; sahte recipe/tool oluşturulmaz.
- `MeasurementSetup` legacy schema1 ve configuration hash yolu aynen korunur.
  Yeni Descriptor bunun alanlarını yeniden yorumlamaz veya otomatik migrate etmez.

## Şema tahsisi ve dosyalar

| Sözleşme | Bu pakette durum |
| --- | --- |
| MeasurementSetupContract | Yeni format visionflow.measurement-setup-contract / schema1 |
| MeasurementCamera, DeviceBinding, GeometryDescriptor, MeasurementSetupDescriptor | Her nested nesne açık schema1; capabilities sabit strict alan kümesi |
| SetupReference, MeasurementSourceEvidence | Yeni bağımsız schema1 |
| Root project1, recipe1–2, editor6–8, execution23, terminal1–5 | Değişmedi |
| Legacy MeasurementSetup1, active catalog1, wrapper1–3, model/dependency/study | Değişmedi; eski alan/hash/no-op yolları korunur |
| Registry, selection, association, recipe source binding, multi-frame terminal | Yeni şema bu pakette tahsis edilmedi; ilgili sonraki pakette |

KMC-2 için registry yolu **<proje>.measurement-environment.json** olarak sabitlendi.
Immutable model kataloğu mevcut **<proje>.active-calibrations.json** yolunu koruyacak;
registry farklı setup'ların exact kayıt seçimlerini tutacak. Bunlar bu pakette
yazılmaz. Save As/kopya/import/export/recovery ve legacy migration KMC-2/8'dir.
İki dosyada atomik transaction iddiası yoktur.

Kabul testi `measurement_setup_contract`: N roundtrip, rename/revision, physical
remap/ambiguous/offline, geometri/cache ayrımı, strict nested schema/type/field,
duplicate/dangling/capacity ve dört source origin kabul/ret yollarını sınar. 100 kontrol; Release UI ve OFF/core ilgili 10/10 geçti (6.11 / 5.84 sn). build/kmc1-final-build-{ui,core}.log, build/kmc1-tests-{ui,core}.log ve iki build altında kmc1-tests.xml.


16 Ekim devamı: [KMC-2 registry/kalıcılık](measurement-environment.md) ve [KMC-3 reçetesiz edinim](independent-calibration-capture.md) ayrı application servisleri olarak uygulandı; UI/Runtime entegrasyonu sonraki paketlerdedir.
