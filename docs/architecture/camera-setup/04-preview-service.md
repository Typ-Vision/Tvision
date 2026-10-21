# CCUX-04 — Canlı önizleme servisi ve yaşam döngüsü

Durum: tamamlandı (21 Ekim 2026, yazılım fixture kabulü). Önkoşul: CCUX-02.
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

## Uygulanan application arayüzü

`CameraPreviewSession::start(opener, live, timeoutMs)` yeni, Disconnected,
PreviewLatest/capacity=1 worker ister. Provider opener'ı `CameraLeaseUse::Preview`
ve kapasite 1 ile açılır; açık deneysel OECS girişinin kendi politikası korunur.
Owner event loop `poll()` çağırır; Connect→Configure→Start→Grab sırası otomatik,
aynı anda yalnız bir komut vardır. `live=false` bir kareden sonra worker'ı kapatır.
`takeLatest()` owned raster, oturum generation'ı ve edinim türü döndürür; sonuç,
trusted completion veya üretim izni üretmez. Aktif UI bağlantısı CCUX-05'tedir.

Provider `continuous` bildirirse mevcut Start/Grab yolu native akışın sonraki
karesini alır; bildirmezse sıralı edinim etiketi kullanılır. OECS FrameBurstStart
profili değiştirilmez ve sıralı kalır. FPS ilk/son başarılı worker tamamlanmasının
monoton zamanından `(N-1)/süre` ile hesaplanır; ilk karede sıfırdır. UTC alanı
gerçek kare timestamp'idir. Son kare yuvası bir elemandır; worker'ın komut,
completion ve frame kuyruğu da sınırlıdır. Bu SDK/RSS sınırı iddiası değildir.

`stop`, `transportLost` ve `requestShutdown` generation'ı emekliye ayırır, son
kareyi siler ve yeni istek üretimini durdurur. Tek çekimin doğal kapanışı son
kareyi korur. Stopping/Error sırasında yalnız worker kapanışı poll edilir;
deadline aşımı Error olur ve lease'i bırakmaz. `drained()` olmadan kamera
değişimi veya yeniden Start reddedilir. Worker shutdown/destructor bitince lease
bırakılır; cleanup hatası lease'i karantinada tutar. Yeniden başlatma açık komut
ister. Proje/kamera sahibi kapanışta `requestShutdown` + `poll` ile drain etmeli;
drain öncesi destructor mevcut worker join'i nedeniyle bloklanabilir. SDK'yı
zorla kesme veya process öldürme çözümü eklenmedi.

