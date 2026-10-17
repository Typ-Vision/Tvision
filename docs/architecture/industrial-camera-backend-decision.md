# ICM-02 — koşullu acquisition backend kararı

17 Ekim 2026. **Seçilen tek yol: ortak application adapter arkasında OECS C API,
yerel x64 DLL'e dinamik bağlanma.** Bu offline karar tamamlandı; fiziksel open,
feature/unit/readback ve grab doğrulaması yapılmadı. G0–G4/H01–H08 açık kalır.
Ürün backend'i henüz uygulanmadı. [Görev kartı](industrial-camera-tasks-foundation.md#icm-02),
[kapanış](../archive/2026-10-17-icm-02.md).

## Deney ve seçim gerekçesi

`tests/industrial-camera-probe/` ana üründen ayrı, opt-in C++20/Qt Core Release
projesidir. Yerel SDK başlıklarını include eder; SDK başlığı/binary'si repoya
kopyalanmaz. MinGW 13.1.0 ile resmi `MvCameraControl.h` bildirimlerinden `decltype`
ile fonksiyon pointer'ları türetilir. MSVC import `.lib` dosyası link edilmez.

| Yol | Yeni gözlem | Karar |
| --- | --- | --- |
| GenTL | U3V CTI yükleniyor; remote port/XML URL, stream/buffer/event exportları mevcut. Mevcut probe discovery DONE, 0 cihaz | Keşif çalışıyor; cihaz olmadan remote XML okunmadı |
| GenApi consumer | OECS Includes ile `GenApi/GenApi.h` syntax-only derlemesi exit 1: başlık yok. Runtime GenApi DLL'i VC120 C++ bağımlılığı | Mevcut yerel dağıtımda MinGW nodemap consumer doğrulanamadı; XML/register elle yorumlanmayacak |
| OECS C API | Header derleniyor, DLL yükleniyor, SDK sürüm çağrısı ve USB enumeration çalışıyor; sonuç MV_OK, 0 cihaz. Gerekli feature/stream/event exportları mevcut | Koşullu seçildi; C sınırı vendor C++ nodemap ABI'sini adapter dışına çıkarmaz |

GenTL genel olarak uygunsuz ilan edilmez: ayrı MinGW GenApi consumer kurulumu bu
pakette yapılmadı. Mevcut yerel SDK ile doğrulanabilen dar C sınırı tercih edildi.
İki acquisition stack yazılmayacak. Generic discovery görünümü kalabilir; actual
open kimliği C API enumeration'daki vendor/model/seri/USB bilgisiyle ayrıca
doğrulanır. GenTL handle'ı vendor handle'ına cast edilmez.

## Exact binary ve ABI sınırı

Yerel runtime: `C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64`.

| Dosya | FileVersion | SHA-256 |
| --- | --- | --- |
| MvCameraControl.dll | 3, 5, 2, 0 | A5D161851573A3C2CED7B88603168C740E7BC0C2E4164621226ED1254DFEC81D |
| MvProducerU3V.cti | 1, 0, 0, 5 | 2C77ADE6EEB1AA6EDA8014A308455AA71728A997B3F4AAC55D286EEED52FEF3C |

`MV_CC_GetSDKVersion` raw `0x03050200`; OECS uygulama paketinin v1.2 bildirimiyle
aynı sürüm alanı değildir. Header `extern "C"`, `__stdcall` ve resmi struct'lar
kullanılır; struct tahmini/packing override yoktur. Bu x64 koşumunda pointer 8,
device info 572, list 2056/list pointer offset 8, frame info 256, frame out 328,
int value 96, float value 28 bayttır. Bunlar derleyici ölçümüdür; frame struct ABI'sinin
gerçek grab ile doğrulandığı anlamına gelmez. x86 veya başka SDK sürümü kabul edilmedi.

## Feature ve lifetime uygulama sınırı

ICM-04b'de isimler `MV_XML_GetNodeInterfaceType` ve
`MV_XML_GetNodeAccessMode` ile doğrulanıp typed C getter/setter'larına çevrilir.
Probe mevcutsa Width/Height/OffsetX/OffsetY/PayloadSize integer,
ExposureTime/Gain float, PixelFormat/TriggerMode/TriggerSource/ExposureAuto/GainAuto
enum readback kaydeder. XML boyut sorgusu hazırdır; **bu koşumda cihaz bulunmadığı
için bu çağrılar çalışmadı**. Probe XML/nodemap consumer değildir.

ExposureTime birimi veya Gain ölçeği isimden varsayılmaz. C float getter birim
taşımaz: gerçek remote XML'deki Unit/interface, üretici model sözleşmesi ve cihaz
readback'i doğrulanmadan ortak profile uygulanamaz. Integer alternatif/alias ancak
o model için kanıtla açılır; desteklenmeyen özellik açık unavailable verir.
Configure → tüm kritik alanları readback → applied profile kabulü atomik olmalıdır;
ara hata önceki üretim iznini geri getirmez. Bu deneye setter/trigger eklenmedi.

Yerel resmi `Samples/VC/VS/SimpleSamples/GrabImage/GrabImage.cpp` buffer çiftini
`GetImageBuffer`/`FreeImageBuffer` ile, `Events/Events.cpp` event callback kaydını
gösteriyor; örnekler çalıştırılmadı. Uygulama sırası:

1. DLL ve gerekli sembolleri doğrula; USB enumerate, exact kimliği seç, handle oluştur.
2. Open → feature/interface/unit doğrulama → configure/readback → StartGrabbing.
3. GetImageBuffer sonucunu owned frame'e kopyala; her başarılı buffer alışında
   FreeImageBuffer çağır. Timeout/kopma durumunda frame/başarı uydurma.
4. StopGrabbing; callback/event işi bitmeden context/DLL'i serbest bırakma;
   CloseDevice → DestroyHandle → DLL unload. Bütün handle'lar tek worker'a aittir.

Probe yalnız create/open/get/close/destroy dener; scope guard hata yolunda cleanup
sağlar, normal close/destroy kodlarını JSON'a yazar. Cihaz yokken handle yolu
doğrulanmış sayılmaz. SDK header'ına göre U3V access mode/key parametrelerini
yoksayar; C API open'ı GenTL read-only open kabulü yerine sunulamaz.

## Runtime ve dağıtım

DLL/SDK veya gerekli export yoksa açık `UNAVAILABLE`/API hatası; başka kamera,
webcam veya GenTL acquisition'a sessiz fallback yoktur. Ana ürünün normal startup,
image_file/webcam build/test yolu SDK'ya zorunlu link edilmez. DLL path yerel
konfigürasyondur; proje fiziksel kimliği değildir.

Doğrudan import envanteri `build/icm/02/oecs-objdump.txt`: GCBase/GenApi VC120,
MvRender ve Windows DLL'leri. GenApi'nin transitif MSVC runtime ve vendor USB TL,
driver bağımlılıkları yerel kurulumla çözülüyor; temiz makine dağıtımı doğrulanmadı.
Yalnız DLL kopyalamak yeterli kabul edilmez. Tam dependency closure ayrıca gerekir.

Yerel `OECS/License` ve `Common Files/OECS/Licenses` üçüncü taraf bildirimleri
incelendi; GenICam bildirimi mevcut. Bu bildirimler vendor SDK/runtime/driver için
genel yeniden dağıtım izni kanıtı değildir. Installer/EULA ve yeniden dağıtım paketi
doğrulanması açık; binary/başlık/lisans metinleri repoya taşınmadı. Hash envanteri
`build/icm/02/experiment.json` içindedir. Kurulum veya dağıtım kabulü yapılmadı.

## Yeniden üretim ve açık kapılar

README'deki yerel toolchain PATH'i ile:

```powershell
cmake -S tests/industrial-camera-probe -B build/icm/02/abi -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 '-DOECS_INCLUDE_DIR=C:/Program Files (x86)/OECS/Development/Includes'
cmake --build build/icm/02/abi
python scripts/probe-industrial-backend.py
```

Runner child PATH ve 20 saniye süreç timeout'u kullanır; komut/exit/hash/JSON/stderr
kaydeder. Eksik DLL ve yanlış API iki negatif kontroldür; fiziksel test değildir.
Compile/log, negatif kontroller ve ilgili CTest [kapanıştadır](../archive/2026-10-17-icm-02.md).
Sonraki bağımsız görev ICM-03a, ardından 03b. ICM-01b reboot sonrası exact cihaz,
OECS Mono8 raster ve viewer release → GenTL read-only open kanıtını bekler.
ICM-04a bu koşullu kararı kullanabilir; 04b/04c'de gerçek readback/grab kabulü ayrıca gerekir.
