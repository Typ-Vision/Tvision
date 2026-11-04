# ICM-BAS-01 — pylon C bağlantısı, exact kimlik ve G0

Durum: Uygulandı; native G0 geçti. [Kapanış ve kanıt](../../archive/2026-11-04-icm-bas-01.md).
Hedefli ON/OFF kabulü yapıldı; tam suite kullanıcı yönlendirmesiyle koşulmadı.
Bağlı: [CAM-OECS-01](CAM-OECS-01.md).
Sonraki: [ICM-BAS-02a](ICM-BAS-02a.md).

## Oku

- [Ortak kabul](README.md), [ana plan](../camera-backend-activation-plan.md) §1/3–6.
- [Eski backend kartı](../industrial-camera-tasks-backends.md) BAS-01;
  [kabul matrisi](../industrial-camera-acceptance-plan.md) G0/H01 ve SDK yokluğu.
- Kurulu pylon C header'ları ve üreticinin C API/runtime belgeleri; sürümü kanıtla.

## Davranış ve sahiplik

src/hardware altında dar Basler C binding/session; CMake optional SDK seçeneği,
manuel probe, fake SDK export fixture'ı ve binding testleri. CAM-SDK-01 yerel
runtime çözümlemesini kullan; mevcut GenTL keşfi/worker/lease yeniden yazılmaz.

1. Kullanıcı kararı **pylon C API**; C++ API/MSVC geçişi bu kartın alternatifi değil.
   MinGW ile kurulu header compile, calling convention/packing/struct boyutları,
   dinamik C export çözümlemesi ve hata metni/return code doğrulanır.
2. Basler acA1920-40um/23102780/U3V enumerate → exact open → kimlik/firmware
   readback → close; ilk index'e sessiz bağlanma yok. SDK init/terminate sahipliği,
   enumeration ömrü ve ortak fiziksel lease kurallarını belirle.
3. Salt okunur node envanteri: Width/Height/Offset, sensör sınırları, PixelFormat,
   exposure/gain/auto tür-birim-range-access, trigger selector/source ve ilgili
   ready/counter/recovery yetenekleri. Eksik node tahmin edilmez.
4. SDK/header yokken optional ürün başlangıcı/file/webcam/tarihçe bozulmaz.
   Kötü DLL/eksik export/kimlik mismatch/open hatasında handle/library temizlenir.
5. Viewer contention ve temiz reopen gözlenir. Viewer görüntüsü önceki kullanıcı
   beyanıdır; native G0 deneyinin yerine yazılmaz. NIC/PoE bu USB hedefe uygulanmaz.

## Kabul

Yeni fixture binding testi/header ABI self-check oluştur; adı CMake envanterinde
doğrulanır. Mevcut regex: `^(camera_transport|industrial_discovery_contract|industrial_provider_contract|camera_worker_lease_contract|backend_capabilities)$`.
Gerçek C API exact open/readback/close, yanlış seri, busy ve yeniden açma kanıtı;
SDK/runtime/header sürümü/hash/env/komut/exit raporu. Ortak tam ON/OFF regresyonu.
Kanıt: `build/icm/bas-01/`. Teknik feasibility başarısızsa G0 açık kalır;
çalışmayan binding ile sonraki karta geçilmez.

Kapsam dışı: feature yazma, görüntü/trigger, ürün ayar butonlarını açma, G1/G3/G4,
SDK binary/header dağıtımı. Geçmiş GenTL discovery yalnız kimlik başlangıç kanıtıdır.

## Uygulanan bağlantı ve yerel kullanım

`PylonSession` hardware katmanındadır. `VISIONFLOW_PYLON_INCLUDE_DIR` boşsa
bağlayıcı unavailable derlenir; runtime ürünün import bağımlılığı değildir.
Kurulu 10.1.4.1565 C header'ları MinGW 13.1 ile derlendi. `PylonHeaders.hpp`
yalnız stdint yeniden tanımı ve `_stdcall` yazımı için uyumluluk sağlar;
üretici dosyaları değiştirilmez. Pointer/return/bool/enum, packing ve kullanılan
`PylonDeviceInfo_t` boyut/offset değerleri compile-time doğrulanır.

Oturum aynı sahip thread'de açılıp kapanır. Tek pylon oturumu atomik SDK sahipliğini
init → enumerate → exact create/open → readback → close/destroy → terminate
boyunca tutar; ikinci oturum beklemek yerine busy alır. Enumeration index'i
yalnız exact vendor/model/serial + `BaslerUsb` eşleşmesinden sonra kullanılır;
open sonrası native ve remote kimlik yeniden doğrulanır. Seçili fiziksel
kameranın ortak `CameraLeaseArbiter` rezervasyonunu çağıran katman tutar;
manuel probe bunu yapar. Ürün worker/factory entegrasyonu 02a/02b'dedir.
Cleanup hatasında owner retry mümkündür; destructor da temizleyemezse SDK ve
handle pinlenir, yeni native oturum açılmaz; süreç yeniden başlatılmalıdır.

Node envanteri mevcut selector bağlamını okur; selector/auto/trigger dahil
hiçbir feature yazmaz. Eksik node `missing`, erişilemeyen node NI/NA/WO olarak
kalır. Komut capability'si korelasyon, cancel/drain veya üretim izni değildir.
Yol ve bağımlılıklar CAM-SDK-01 `CameraLocalConfiguration`/`RuntimeLibrary`
üzerinden çözülür; header/runtime repoya kopyalanmaz.

```powershell
cmake -S . -B build/reference-ui '-DVISIONFLOW_PYLON_INCLUDE_DIR=C:/Program Files/Basler/pylon/Development/include'
cmake --build build/reference-ui -j 6 --target visionflow_pylon_probe pylon_binding_test pylon_test_sdk
# README toolchain PATH'i ile; --runtime verilmezse yerel Basler runtime çözümlemesi kullanılır.
./build/reference-ui/visionflow_pylon_probe.exe --vendor Basler --model acA1920-40um --serial 23102780
```

Probe JSON/exit üretir; manuel fiziksel denemedir, CTest'e eklenmez.
`pylon_backend_binding` fake export + gerçek header ABI testidir;
SDK'sız yapılandırmada yerine `pylon_backend_unavailable` kaydedilir.
