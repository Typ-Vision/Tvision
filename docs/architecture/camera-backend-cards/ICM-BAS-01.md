# ICM-BAS-01 — pylon C bağlantısı, exact kimlik ve G0

Durum: Planlandı. Bağlı: [CAM-OECS-01](CAM-OECS-01.md).
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
