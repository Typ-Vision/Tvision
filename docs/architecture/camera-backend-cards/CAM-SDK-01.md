# CAM-SDK-01 — Ortak runtime/yol yönetimi ve çoklu producer

Durum: Uygulandı; kalan tam ON testleri kullanıcı isteğiyle ertelendi.
[Kapanış](../../archive/2026-11-04-cam-sdk-01.md). Sonraki: [CAM-OECS-01](CAM-OECS-01.md).

## Oku

- [Ortak kabul](README.md), [ana plan](../camera-backend-activation-plan.md) §2–5.
- [CCUX sözleşmesi](../camera-setup/contract.md) C1/C2/C2a/C4.

## Davranış ve sahiplik

CameraLocalConfiguration, CameraService, GenTLTransport, CameraManager;
scripts/run-local.ps1, gerekli CMake ve ilgili testler. Gereken dar runtime
çözümleme yardımcısı application/hardware sınırında tutulur; UI SDK çağırmaz.

1. Otomatik tarama remembered eklemeler + süreç GenTL64 ortamını canonical
   birleştirir; çoklu CTI ekleme/çıkarma/devre dışı bırakma önceki markayı silmez.
   Explicit tek-producer tanısı otomatik listeyi değiştirmez.
2. Backend başına runtime DLL/bağımlılık dizini yerel kaydı; açık seçim → kayıtlı
   geçerli yol → vendor ortam/kurulum → bilinen konum. Belirsizlikte seçim;
   eski açık seçim bozulduysa görünür hata ve yeniden seçim, sessiz SDK değişimi yok.
3. SDK header yolu build'de; runtime/CTI makinede; proje/export taşınabilir kalır.
   x64 PE/dosya/yol kontrolleri ve süreçle sınırlı bağımlılık çözümlemesi.
   Adaptör yoksa yol bulunsa bile derlenmemiş durumunu göster.
4. Producer başına tamamlanma/hata kökeni taşınır. Başka producer hatası sağlıklı
   kamerayı engellemez; seçili producer kısmi/iptal/belirsizse kullanım reddedilir.
   Exact kimlik/çoklu rota/eksik seri kapıları korunur; global errors'ı silmek çözüm değil.
5. Derlenmemiş adaptör, eksik runtime, x86, export/bağımlılık yükleme hatası,
   offline, belirsiz rota ve busy ayrı nedenlerdir. Sadece mevcut bağlayıcının
   yükleme denetimi kullanılır; Basler C binding bu kartta geliştirilmez.
6. Runtime değişimi aktif işte uygulanmaz; temiz Stop/close sonrası yeniden açılır,
   loader gerektiriyorsa restart yönlendirilir. Program açılışı kamera açmaz.

## Kabul

Hedefli regex: `^(camera_transport|industrial_discovery_contract|industrial_provider_contract|camera_settings|ui_camera_manager|camera_probe_diagnostics)$`.
Yeni yerel runtime/producer fixture'larını bu gruba ekle. Testler: ortam/kayıt
birleşimi, canonical tekrar, yanlış x86, eksik DLL/export/dependency, eski ortam,
çoklu kurulum, bozuk A + sağlıklı B, seçili B'nin kısmi taraması, çift rota,
restart/Save As ve SDK yokken ürün başlangıcı. Gerçek kurulu producer yükleme
kanıtı ve OECS/Basler keşfi; sandbox 0 sonucu cihaz yokluğu sayılmaz.
Ortak tam ON/OFF ve UI iki boyut kabulü zorunlu. Kanıt: `build/camera/cam-sdk-01/`.

Kapsam dışı: SDK/driver kurulumu, global PATH/registry değişikliği, genel plugin
framework, Basler feature/grab ve üretim capability'si. Eksik runtime fiziksel
kabulü açık bırakır; yolun bulunması destek kabulü olmaz.
