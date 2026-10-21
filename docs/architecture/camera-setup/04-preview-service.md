# CCUX-04 — Canlı önizleme servisi ve yaşam döngüsü

Durum: planlandı. Önkoşul: CCUX-02. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C2,C5; [deneysel plan](../industrial-camera-oecs-experimental-plan.md) “Uygulama sözleşmesi”.
Kaynak: `src/application/CameraWorkerSession.hpp/.cpp`, `src/application/CameraLifecycle.hpp/.cpp`, `src/application/ExperimentalOecsAcquisition.hpp/.cpp`, `src/application/IndustrialCameraProvider.hpp/.cpp`.

## Uygula

1. Tek görüntü ve canlı önizleme için ortak application session oluştur veya mevcut session'ı genişlet; Qt widget bağımlılığı yok.
2. Backend gerçek continuous destekliyorsa o yolu, aksi halde en fazla bir bekleyen sıralı edinimi kullan. Edinim türünü sonuçta bildir; FrameBurstStart semantiğini koru.
3. Idle/Starting/Streaming/Stopping/Error durumları, latest-frame tamponu, gerçek timestamp/FPS, deadline ve generation davranışını tanımla.
4. Stop/kamera değişimi/kopma/kapanışta yeni istek üretimini kes; geç frame'i bastır ve lease'i gerçek worker bitişinde bırak. Timeout'ta worker meşgulse yeni open yapma; process öldürmeyi normal Stop çözümü yapma.
5. Kalibrasyon/Editor/Runtime ile aynı cihaz lease'ini paylaş; preview doğrudan sonuç veya trusted completion üretmesin.

## Kabul

Yavaş/hung fake backend, geç callback, ardışık Start/Stop, A→B kamera geçişi, kopma ve shutdown; en fazla bir pending request ve sınırlı framebuffer; Stop sonrasında kare yayımlanmaması. SDK/RSS hard sınırı iddia etme.

**CTest:** camera_worker_lease_contract, camera_worker_stress, industrial_oecs_experimental

## Sınır ve kapanış

UI sonraki kart. Gerçek cihaz yoksa fixture lifecycle kabulüyle bitir; gerçek uzun video testi CCUX-15'tedir.
Kanıt: `build/ccux/ccux-04/`. AGENTS ve ortak sözleşme C6 ile kapat.

