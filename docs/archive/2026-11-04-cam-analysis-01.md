# CAM-ANALYSIS-01 — Camera Manager ayar erişimi ve geometri analizi

4 Kasım 2026. Uygulayıcı: Codex.

## Kapsam ve durum

Kullanıcı K2 kontrollerini başarılı teyit etti. VF-08 başlamadan kamera ayar
erişimi ve optik/geometri alanları incelendi. Bu kayıt analizdir; uygulama planı,
ürün düzeltmesi veya fiziksel kabul değildir. Başlangıç HEAD: `6fb6934`;
çalışma ağacı temizdi. Alt ajan kullanılmadı.

## 1. Keşif var, ayarlar neden kapalı?

`CameraManager::controls` ayar erişimini kayıt, profil, exact keşif/rota,
boşta olma ve `CameraSettingsService::available` sonucuna bağlar. Kullanıcının
bildirdiği backend/runtime mesajı; kayıt, industrial kimlik, rota ve profil
kontrolleri geçildikten sonra settingsAvailable false olduğunda seçilir.
Eksik lens/düzlem veya 1×1 çözünürlük bu mesajın doğrudan nedeni değildir.

Normal ürün ayar yolu `CameraSettingsService::available` →
`OecsCameraSettings::available` şeklindedir. Test provider enjeksiyonu ayrıdır.
Üründe gerekenler:

- Kullanılabilir fiziksel kimlik; vendor tam `Opto Engineering Srl`, model tam
  `COE-050-M-USB-050-IR-C`.
- Derlemede `VISIONFLOW_WITH_OECS_SDK` tanımı.
- UI'da verilen mutlak runtime DLL yolunun mevcut dosya olması.

Yerel incelemede `build/reference-ui/CMakeCache.txt:363` include yolu boş,
runtime cache girdisi de boştu. `build.ninja` içindeki OecsCApi.cpp derleme
kuralında SDK tanımı ve SDK include yolu yok. CMakeLists.txt:64–74 bu tanımı
yalnız include yolu verildiğinde ekliyor. `oecsSdkBindingCompiled` tanım yokken
false döndürüyor. Günlük başlatıcı `VisionFlow.cmd` → `scripts/run-local.ps1`
varsayılan olarak bu build'deki visionflow.exe'yi çalıştırıyor.

Buna karşılık aşağıdaki dosyalar diskte mevcut:

- `C:/Program Files (x86)/OECS/Development/Includes/MvCameraControl.h`
- `C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll`

Dolayısıyla mevcut varsayılan build için somut neden SDK bağlayıcısının derlemeye
alınmamasıdır. DLL'in varlığı tek başına yeterli değildir. Runtime CMake girdisi
bağlayıcıyı açan anahtar değildir; yerel probe/test hedeflerini de etkiler.
GenTL keşfi ile OECS ayar erişimi farklı yollardır; kameranın keşfedilmesi
ayar SDK'sının derlenmiş/çalışır olduğunu kanıtlamaz.

İnceleme anında Get-Process sorgusunda VisionFlow süreci görülmedi.
Win32_Process CIM sorgusu erişim hatası verdi. Kullanıcının bildirdiği anın EXE
yolu, UI'daki DLL metni ve canlı kimlik bu oturumda doğrudan okunmadı; başka
EXE kullanıldıysa yukarıdaki build bulgusunun o EXE'ye uygulanması ayrıca doğrulanır.

`Uygula ve kaydet` ayrıca başarılı gerçek readback ister; başlangıçta pasif olması
tasarımdır. `Ayarları oku` erişimi düzelmeden scalar alanlar ve uygulama açılmaz.
Mevcut mesaj desteklenmeyen kimlik, SDK'sız build ve eksik/yanlış DLL nedenlerini
tek cümlede topladığı için kullanıcı uygulanabilir nedeni göremiyor. Availability
kontrolü DLL yükleme ve bağımlılık başarısını sınamaz; erişim açıldıktan sonraki
gerçek open/readback ayrıca başarısız olabilir.

## 2. Sensör genişliği/yüksekliği ne demek?

`CameraManager::editProfile` bu iki tam sayıyı doğrudan
`GeometryDescriptor.resolution` alanına ve sıfır başlangıçlı sensorRoi boyutuna
yazar. Birim **piksel (px)**; fiziksel sensör boyutu, görüş alanı (mm), kalibrasyon
katsayısı veya binning oranı değildir. `1 / 1` tek piksel genişlik/yükseklik
iddiasıdır. Formun sabitlediği binning/decimation 1×1 ise başka alanlardır.

Bu model için tarihsel fiziksel readback 2448×2048'dir (CCUX-03b kaydı);
mevcut OECS edinim backend'i de exact 2448×2048 Mono8 raster ister. Bugün canlı
readback yapılmadı. İlgili kamera ve mevcut tam raster yolu için beklenen giriş
2448 / 2048 px'dir; tüm kameralar için sabit değer değildir. 2448×2047 origin
(0,1) satır kırpma kararı bu tam sensör formuna yazılacak değer değildir.

Formda px suffix/açıklaması ve canlı çözünürlükle eşleşme denetimi yok.
Pozitif 1×1; domain'in genel pozitiflik, sınır ve ROI tutarlılığı doğrulamasından
geçebilir. Bu yüzden kayıt kabulü fiziksel doğruluk anlamına gelmez. Optik
formunu kaydetmek kameraya Width/Height yazmaz; istenen kurulum/profili kaydeder.
Edinim konfigurasyonu tam SensorWidth/SensorHeight kullanır ve sonunda
`verifyApplied` kayıtlı geometri ile gerçek readback'i karşılaştırır; 1×1 ile
2448×2048 uyuşmazlığı normal edinimin reddine neden olabilir. Scalar ayar
servisi geometri yazan bir SDK yolu değildir.

## 3. Yanlış kayıt ve readback etkileşimi

- Taslak/pending kurulumda başarılı readback formu gerçek çözünürlükle doldurur.
- Tamamlanmış kurulumda form, yeni readback bulunsa bile kayıtlı geometriyi
  öncelikli gösterir (`CameraManager.cpp:241`). Kaydedilmiş 1×1 burada kalabilir.
- Yalnız Ayarları oku disk kaydını değiştirmez; UI bunu açıkça bildirir.
- Uygula ve kaydet başarılı olursa settings servisi, pending olmayan kurulumun
  geometrisini gerçek readback ile günceller; değişiklik revision/invalidation
  kurallarından geçer. Pending lens/düzlem bilgilerini uydurarak tamamlamaz.
- Optik/geometri değişimi mevcut model/kaynak uyumunu etkileyebilir. Matematiksel
  modelin saklanması ile güncel kullanım izni ayrıdır; eski izin varsayılmaz.

## 4. Sonraki plan için etki sınırı

Plan henüz hazırlanmadı. Bulguların ulaştığı mevcut yüzeyler: CMake yerel SDK
yapılandırması, README/tanı rehberi; CameraManager availability açıklaması ve
geometri formu; OecsCameraSettings/CameraSettingsService; geometri sözleşmesi ve
edinimdeki verifyApplied; camera_manager ve oecs_camera_settings testleri.
Yeni SDK/backend veya yeni şema gerekliliği bu analizden çıkmıyor.

Mevcut UI testi SDK OFF durumunda ayar düğmesinin kapalı olmasını bekliyor;
ürün ayar/readback/taslak kontrolleri SDK derlenmişse çalışıyor. Bu nedenle SDK'sız
test başarısı gerçek ürün ayar yolunun çalıştığını göstermez. Testler kaynak
olarak incelendi; yeni koşum yapılmadı. 1×1 kayıt ve tamamlanmış kayıt/readback
önceliği sonraki çözümün kabulünde özellikle ele alınmalı.

## Doğrulama, sınırlar ve devam

Graph list_projects/index_status ile D:/TVision seçildi; availability çağrı zinciri
depth 1'de iki çağıran/iki çağrılanla doğrulandı. Depth 2 ilk sorgu sayfalandı;
tam etki iddiası yerine ilgili depth 1 ve doğrudan kaynak kullanıldı.
Kanıt alınan 13 kod/test/build/başlatıcı yolu check_index_coverage ile kontrol
edildi. CameraManager satır 29 parse_partial kaynakla okundu; CMake/başlatıcı
güncellik boşlukları doğrudan dosyalarla tamamlandı. Graph bağlantı hatası yoktu.

Değişen dosyalar: bu analiz, progress ve arşiv dizini. Ürün kodu, CMake cache,
yerel INI ve kullanıcı projesi değiştirilmedi. Yeni build/CTest/görsel/fiziksel
probe yapılmadı; mevcut tarihsel başarı yeni koşum sayılmadı. Native production,
pending-command/cancel ve bağımsız mm kabulü engelleri korunur.

Devam: bu analizi kullanıcıyla değerlendirip kamera çözüm planını hazırlamak;
ardından uygulama/kabul. K2 kullanıcı teyidi tamam; VF-08 bu çalışmada başlamadı.

Belge kabulü: `python scripts/check-context-docs.py` başarılı (300 belge,
1272 yerel bağlantı, progress 7990 / başlangıç toplamı 19852 bayt).
İlk kontrollerde boyut sınırı aşıldı; arşiv bağlantıları/engeller korunarak özet
kısaltıldı. `git diff --check` başarılı. Push yapılmadı.
