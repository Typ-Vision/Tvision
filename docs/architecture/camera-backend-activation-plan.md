# CAM-PLAN-02 — OECS, Basler ve sonraki markaların devreye alınması

4 Kasım 2026. Durum: analiz ve sıralı uygulama planı; uygulanmış ürün desteği değil.
Önceki [kamera analizi](../archive/2026-11-04-cam-analysis-01.md),
[backend kartları](industrial-camera-tasks-backends.md),
[CCUX sözleşmesi](camera-setup/contract.md) ve
[çalışma protokolü](../working-protocol.md) geçerlidir.

## 1. Doğrulanmış başlangıç

Kullanıcı pylon Viewer'da Basler USB kameradan görüntü aldığını bildirdi.
Mevcut `visionflow_camera_probe.exe` ile explicit x64 Basler producer keşfi:
**Basler / acA1920-40um / 23102780 / U3V**. Keşif exit 0, errors boş.
Kamera açılmadı, feature okunmadı/yazılmadı, grab yapılmadı. Sandbox içi ilk
koşum sıfır cihaz, sandbox dışı koşum bir cihaz buldu; ilki cihaz yokluğu değildir.

Kanıt: `build/icm/camera-plan-02-external-20261104-202203/` altında
`basler-discovery.json`, `result.json`, `installed-files.json`,
`camera-environment.json`. İlk koşum: `build/icm/camera-plan-02-20261104-202141/`.
Dosya sürümleri birbirinden farklıdır; bunlardan tek bir SDK paket sürümü türetilmez.

| Rol | Makinede bulunan yol | Dosya sürümü |
| --- | --- | --- |
| USB3 GenTL producer | `C:/Program Files/Basler/pylon/Runtime/x64/ProducerU3V.cti` | 10.0.3.1565 |
| GigE GenTL producer | `C:/Program Files/Basler/pylon/Runtime/x64/ProducerGEV.cti` | 10.6.0.1565 |
| Basler C API | `C:/Program Files/Basler/pylon/Runtime/x64/PylonC_v10.dll` | 10.1.4.1565 |
| SDK include kökü | `C:/Program Files/Basler/pylon/Development/include` | PylonC.h mevcut |

Bu USB cihazın rotası ProducerU3V'dir; ProducerGEV'nin kurulu olması GigE kabulü
değildir. Win32 dizinleri de mevcut; ürünümüz x64 olduğu için seçilmez.

Süreç `GENICAM_GENTL64_PATH` yalnız Daheng'i görüyor. Güncel Machine değeri
OECS + Daheng + Basler x64 dizinlerini içeriyor; Machine PYLON_DEV_DIR dolu,
süreçte boş. Kurulum öncesi açılan host/terminal eski ortamı miras bırakmış olabilir.
Ürün normal olarak sürecinin ortamını okur; registry değişikliği çalışan sürecin
ortamını yenilemez. Yeni oturum veya explicit yerel seçim gerekir.

## 2. Mevcut kodun sınırları

- `GenTLTransport` discovery ve readonly open/close sunuyor. Remote node map,
  feature ayarı, stream/event/buffer edinimi bu sınıfta yok.
- `genTLProducerPaths` verilen dizinler + süreç GENICAM_GENTL64_PATH içindeki
  CTI'leri listeler; recursive disk taraması yapmaz.
- `CameraService::discover`: explicit producer listesi doluysa ortamı kullanmaz.
  Camera Manager tek dosya seçiminde `setProducers({f})` yapar; önceki marka
  listesi değişir. Boş metin alanı remembered listeye gider; ortamla birleşme yok.
- `CameraLocalConfiguration` producer listesi ve fiziksel kimlik başına rota
  saklar. SDK include/runtime kaydı yok. Camera Manager OECS DLL alanı mevcut
  kaynakta varsayılan yoldan açılır; kalıcı backend runtime yazımı yok.
- Ayar servisi normal üründe yalnız OecsCameraSettings'e bağlıdır. Native ürün
  factory'si default durumda unavailable; test enjeksiyonunu ürüne açmak çözüm olmaz.
- `CameraService::resolveIndustrial` ve UI route kapısı herhangi bir inventory
  error varsa tüm rotaları reddeder. Bir eski producer hatası diğer markayı
  engelleyebilir; izolasyon için seçili rotanın keşif bütünlüğü ayrıca taşınmalıdır.
- OECS exact model/2448×2048/FrameBurstStart kabulleri Basler'e genellenemez.
  AcquisitionPolicy aynı Runtime reçetesinde OECS ile native kamera karışımını
  bugün reddeder; iki markayı ayrı ayrı desteklemek bu sınırı kendiliğinden kaldırmaz.

## 3. Bağlantı kararı

**Kullanıcı kararı: ortak GenTL keşfi + Basler pylon C API adaptörü.**
Uygulama C++20/Qt/MinGW kalır. Kurulu C header'ı extern C,
calling convention ve packing tanımlarını sunuyor. Mevcut Qt/MinGW ürününe
MSVC C++ nesne/exception ABI'si taşınmaz; QLibrary ile C export çözümlemesi,
mevcut OECS yaklaşımı gibi ayrı hardware katmanında denenir.

Bu bir MinGW uyumluluk kabulü değildir: ICM-BAS-01'de gerçek header compile,
export/signature/struct boyutu ve çalışma zamanı deneyi geçmelidir. Başarısızsa
karar yeniden açılır; bütün ürünü MSVC'ye taşımak varsayılan çözüm değildir.
GenTL üzerinden tam genel edinim alternatifi remote XML/GenApi ve stream/event/
buffer tüketicisi gerektirir; yalnız CTI ekleme işi değildir. İlk Basler desteği
için iki edinim yolunu birden geliştirmeyiz.

Keşifte görülen vendor/model/seri, C API enumerate/open/readback'te exact yeniden
doğrulanır. USB sırası/Device ID ile ilk kameraya bağlanılmaz. İki API yolunun
aynı fiziksel kamera lease'ini kullanması gerekir. pylon Viewer cihazı tutuyorsa
işlem anlaşılır busy hatası verir; otomatik başka kameraya geçmez.

## 4. Yollar nasıl bulunacak ve saklanacak?

Üç ayrı öğe korunur:

| Öğe | Ne zaman gerekir? | Saklama |
| --- | --- | --- |
| SDK header/include kökü | Geliştirici derlemesi | Makineye özel CMake cache/preset; proje/reçeteye yazılmaz |
| Runtime DLL + bağımlılık dizinleri | Adaptörü çalıştırma | Backend başına makineye özel CameraLocalConfiguration |
| CTI listesi ve kamera rotası | GenTL keşfi / seçili bağlantı | Mevcut yerel yapılandırma genişletilir |

Önerilen otomatik aday sırası: açık kullanıcı seçimi → kaydedilmiş geçerli yerel
kayıt → ilgili vendor ortam/kurulum konumu → bilinen standart kurulum dizini.
GenTL otomatik taraması remembered producer eklemeleri ile süreç
GENICAM_GENTL64_PATH adaylarının canonical birleşimini kullanır; mükerrer dosya
elenir. Explicit tek-producer tanı taraması ayrı kalır, otomatik listeyi silmez.
İstenmeyen/eski producer yerel listeden devre dışı bırakılabilir; bozuk kayıt sessiz
başarıya dönüştürülmez. Bütün disk veya PATH üzerinde rastgele DLL taranmaz.

Basler için PYLON_DEV_DIR include adayı verir; bunun runtime DLL yolu olduğu
varsayılmaz. Aynı kurulumun Runtime/x64 dizini ayrıca doğrulanır. OECS için
mevcut kurulum adayı korunur; bulunamadığında elle seçim vardır. Gelecek marka
için yalnız o adaptörün bildiği SDK konumları eklenir. Sürümlü DLL adı belirsizse
alfabetik/en yüksek sürüm sessiz seçilmez; desteklenen adaylar denetlenir, birden
fazla uyumlu kurulum için mevcut seçim korunur veya kullanıcıya seçim sunulur.

Yerel kayıt backend kimliği, canonical runtime yolu ve gereken bağımlılık
dizinlerini tutar; kullanılan sürüm/hash tanı raporuna yazılır. Projede yalnız
fiziksel kamera/setup/profile kimlikleri kalır. Başka PC'ye kopyalanan proje o
PC'de runtime çözümlemesini yeniden yapar; eski C: yolu proje kimliği değildir.

Mutlak DLL yolu ana dosyayı seçer, bağımlılıklarını tek başına çözmez. Win64 PE,
gerekli export, yükleme hatası, bağımlı DLL ve runtime sürümü ayrı kontrol edilir.
Üreticinin tek kurulumundan tutarlı dosya takımı kullanılır. Bilinen runtime
dizinleri için süreçle sınırlı Windows loader/başlatıcı düzeni seçilir ve farklı
markalar birlikte sınanır; global PATH/registry değiştirmek ürün komutu olmaz.
Tek DLL'i exe yanına kopyalamak dağıtım stratejisi sayılmaz. Aynı isimli uyumsuz
bağımlılık çatışması varsa raporlanır; process izolasyonu ancak kanıt gerekirse
ayrı tasarlanır. Çalışan lease varken DLL değiştirme/unload yok; değişiklikte
Stop ve yeniden açma, gerekiyorsa uygulamayı yeniden başlatma gerekir.

## 5. Nasıl devreye alınacak?

Sıra: derlenmiş adaptör → runtime çözümleme/yükleme → keşif → exact open →
readback → desteklenen ayarı uygulama/readback → owned görüntü → tetik/Stop
doğrulaması → ilgili ürün kabulü. Yol seçimi bu basamakları atlatmaz.

UI nedeni ayrı gösterir: adaptör derlenmemiş, runtime bulunamadı, yanlış mimari,
bağımlılık/export yüklenemedi, cihaz bulunamadı, rota belirsiz, cihaz meşgul,
feature desteklenmiyor veya readback uyuşmuyor. Ayar capability'si edinimden
bağımsızdır. SDK'sız bilgisayarda dosya/webcam/tarihçe çalışmaya devam eder.

Bir producer'ın yükleme/keşif hatası diğer sağlıklı producer sonuçlarını
listelemeden düşürmemeli. Kullanım kararı seçili producer'ın tamamlanmış taraması,
exact kimlik ve rota üzerinden verilir; aynı producer içindeki kısmi/bozuk tarama
reddedilir. Mevcut global hata kapısını yalnız kaldırmak yeterli değildir.
Çapraz producer çift rota ve eksik seri belirsizliği korunur.

Yeni marka: önce kurulu CTI ile discovery desteklenebilir. Ayar/grab için hazır
uyumlu ürün adaptörü veya gerçekten uygulanmış genel GenTL tüketicisi gerekir.
DLL yolunu girerek bilinmeyen marka SDK'sı çalıştırılamaz. Genel plugin yükleyici,
script adaptörü veya bütün markalar için baştan yeni framework yapılmayacak.

## 6. Basler geometri ve feature farkları

[Model belgesi](https://docs.baslerweb.com/aca1920-40um): default 1920×1200,
full 1936×1216. Canlı Width/Height/Offset readback henüz yok. İlk uygulama aktif
ROI'yi okuyup gösterecek; otomatik full-sensor değişikliği yapmayacak. İleride
ROI düzenleme gerekiyorsa ayrıca açık uygulama/readback ve model invalidation ister.

Geometri alanları px etiketi, gerçek/istenen ayrımı ve uyuşmazlık denetimi ister.
Sensör maksimumu ile aktif görüntü ayrı sunulur. Formda 1×1'in genel domain
pozitiflik denetiminden geçmesi fiziksel doğruluk değildir. Tamamlanmış kurulumda
eski geometriyi yeni readback önünde gösterme sorunu önceki analizin kapsamındadır.

Exposure/gain/auto/trigger node, tür, birim, range ve yazılabilirlik cihazdan okunur.
Özellikle GainRaw ile dB doğrudan aynı kabul edilmez; eski/yeni feature adları
yalnız belgeli eşleme ve gerçek readback ile ele alınır. Başlangıç hedef Mono8 ve
tek software trigger'dır; FrameStart/FrameBurstStart/Off ayrı semantik taşır.
Basler'e OECS FrameBurstStart veya 2448×2048 sabiti uygulanmaz.

USB kamera tamponunda eski kare kalabilmesi ve BlockID reset/invalid durumları
[pylon C rehberinde](https://docs.baslerweb.com/pylonapi/c/programmingguide)
belgelidir. Trigger-ready veya artan BlockID tek başına çevrim-kare eşleşmesi
kanıtı sayılmaz. Basler G1 stratejisi kendi timeout/cancel/Stop ve donanım kuyruk
semantiğiyle kanıtlanır; OECS engeli Basler'e aktarılmaz veya kapanmış sayılmaz.

## 7. Sıralı uygulama kartları

Her satır ayrı görev/commit; ajanlar paralel çalışmaz. Uygulama henüz başlamadı.
**[Altı ayrı uygulama kartı ve ortak kabul](camera-backend-cards/README.md)**
CAM-PLAN-03 ile yazıldı; uygulamada seçili kart esas alınır.

| Sıra / ID | Teslim ve sahiplik | Kabul / çıkış |
| --- | --- | --- |
| 1 — CAM-SDK-01 | CameraLocalConfiguration, CameraService, GenTLTransport, CameraManager, başlatıcı ve ilgili testler: çoklu CTI çözümleme, backend yerel runtime kaydı, nedenli tanı, seçili producer hata izolasyonu | Eski ortam, yanlış x86/eksik DLL/export, bozuk başka producer, çift rota, no-SDK startup/file/webcam, restart ve proje taşınabilirliği; runtime bağımlılığı için gerçek yükleme kanıtı |
| 2 — CAM-OECS-01 | CMake/yerel build ve mevcut OECS ayar/readback yolu; CameraManager px/aktif geometri açıklaması ve 1×1/readback önceliği düzeltmesi | Doğru SDK'lı Release, gerçek exact readback; mevcut scalar no-op apply/save; yanlış geometri ve eski model izin denetimi; yeni canlı crop yok, native OECS engeli korunur |
| 3 — ICM-BAS-01 | Mevcut backend kartının USB hedefi: dar pylon C binding/feasibility, vendor runtime entegrasyonu, salt okunur kimlik/node envanteri; hardware katmanı/CMake/probe | acA1920-40um/23102780 exact native identity, firmware/runtime/hash/ABI ve exclusive open-close; Viewer contention; G0 kararı. Keşif kanıtı devralınır, G0 tamam denmez |
| 4 — ICM-BAS-02a | Basler settings adaptörü, CameraSettingsService ve ürün factory composition; mevcut worker/lease/rollback kullanılır | Pending kurulumda gerçek ayar/ROI/range readback, desteklenen scalar apply/save, doğrulanmış rollback ve unsupported node; acquisition yetkisi üretilmez |
| 5 — ICM-BAS-02b | Basler native owned Mono8 capture, preview, software trigger ve Stop/recovery; mevcut ICM-BAS-02/G1 kapsamı | Belgeli korelasyon, gerçek tek/100 trigger, aynı görüntülü yeni kare, pending/timeout/Stop/reopen/kopma/buffer ömrü negatifleri; G1+ortak admission şartı |
| 6 — ICM-BAS-03 | Camera Manager, Calibration, Editor, Runtime ürün zinciri ve eski kart H01–H08/G3 | Gerçek 1000 cycle, teardown, history/Logs/CSV, üç applicationType, UI; yalnız Basler tek kamera kapsamı |

02a/02b mevcut ICM-BAS-02'nin alt teslimleridir; ayrı yeni paralel backend planı
değildir. Genel G0/G1/G3/G4 eşikleri eski kartlarda korunur. Basler/OECS aynı PC'de
discovery ve ayrı oturum kullanımı CAM-SDK-01/Basler kabulünde sınanır. Aynı Runtime
reçetesinde OECS+Basler edinimi, AcquisitionPolicy sınırlaması nedeniyle ayrı sonraki
kapsamdır; burada sessiz açılmaz. İki markanın desteklenmesi, eşzamanlı çok kamera
kapasitesi veya fiziksel mm doğruluğu anlamına gelmez.

Kod kartları hedefli davranış/negatif/sınır kabulünden sonra son kaynakta tam
Release ON/OFF CTest ister. UI değişiklikleri normal/küçük görsel kontrol de ister.
Gerçek donanım kanıtı fixture'dan ayrılır; kalibrasyon için lens/düzlem ve bağımsız
referans ayrı kabul edilir. Runtime/driver dağıtımı lisans ve üretici paket koşulları
doğrulanmadan yapılmaz. Sayaç erişilemezse tüketim ölçümü uydurulmaz.

## Resmî kaynaklar ve açık kararlar

- [pylon dağıtım rehberi](https://docs.baslerweb.com/pylonapi/pylon-deployment-guide):
  CTI/runtime/dependency ayrımı, x64 dosyalar ve driver/runtime dağıtımı.
- [pylon API kapsamı](https://docs.baslerweb.com/pylonapi/): C API destek yüzeyi.
- [pylon C rehberi](https://docs.baslerweb.com/pylonapi/c/programmingguide):
  feature erişimi, USB buffer ve BlockID semantiği.
- [acA1920-40um](https://docs.baslerweb.com/aca1920-40um): full/default geometri.

Açık: MinGW header/C ABI feasibility, exact firmware ve aktif readback, runtime
paket sürümü/lisans dağıtımı, modelin gerçek node/trigger/recovery yetenekleri.
Kurulum güncelleme veya SDK yükleme yapılmadı; mevcut dosyaların envanteri çıkarıldı.
