# Application edinim oturumları

CCUX-22b servis sözleşmesi; ekran bağlantıları CCUX-22c–e kapsamındadır.
`CameraAcquisitionSessions` uygulama açılışında cihaz açmaz. Yalnız açık
`prepare(context, inputs, requests, policy)` komutu keşif/resolve, Connect,
Configure/readback ve Start yapar; hazırlık Grab veya sonuç üretmez.

## Sahiplik ve komutlar

Servisin thread-safe cephesi immutable durum/kare değerleri taşıyan future döndürür.
UI future bekleyerek bloke olmaz; arka planda bekler veya hazır olmasını yoklar.
Tek sabit coordinator worker oluşturur, komut verir ve yok eder. SDK/backend
nesneleri mevcut donanım worker thread'inde kalır. Worker pointer'ı execution
işlerine taşınmaz. `IndustrialExecutionInputs.sessions/sessionContext/sessionGeneration`
yalnız bellek içi uygulama bağımlılığıdır; reçete veya admission alanı değildir.

Context ID işlem sahibini, generation hazırlanmış kaynak kümesini tanımlar.
Her prepare önceki generation'ı iptal eder. Kimlik kamera/setup/profile kayıtlarını
(isim hariç), revision/geometri, rota/backend, provider bileşimi ve edinim
politikasını kapsar. Sağlıklı exact eşleşme tekrar keşif, DLL hash/load, open veya
configure yapmaz. Profil/rota değişimi eski worker tamamen drain olduktan sonra
yeni worker açar. Kaynak kaldırma son kullanıcısı kalmayan kamerayı kapatır.

Durumlar kaynak bazında Opening → Configuring → Ready → Capturing → Ready;
çıkışta Closing → Closed, hatada Fault/drain'dir. Hazırlık farklı kameralarda
kısmen başarılı olabilir; `sources` hata ayrımını, `allReady()` bütün hedeflerin
hazır olmasını belirtir. Runtime bütünlük/rollback politikası CCUX-22d'de bağlanır.
Retry açık yeni prepare komutudur; otomatik yeniden bağlanma yoktur.

## Edinim ve çok kaynak

Mevcut global worker lease kaydı vendor/model/serial başına tek sahibi korur;
Camera Manager, preview ve başka context'ler aynı cihazı alamaz. Aynı context'te
aynı fiziksel kamera ve exact ayarları kullanan kaynaklar tek worker paylaşır.
Çelişen profiller hazırlıktan önce reddedilir. Her kamera için en fazla bir
bekleyen edinim vardır; her kaynak isteği ayrı Grab üretir, kare paylaşılmaz.

`acquireIndustrial` hazırlanmış generation varsa servise komut verir. Varsayılan
tek çekim yolu aynı kalır. Prepared başarıda stream cleanup ve mevcut ham kare/
readback doğrulamaları tamamlanınca bağlantıyı kapatmadan yayın yapılır. OECS
stream start/stop hâlâ kare başınadır. Ready native admission, trigger korelasyonu,
kalibrasyon/model geçerliliği veya üretim izni değildir; deneysel köken korunur.

Kaynak ID uygulama bağlamıdır: logical kaynağa bağlanan acquisition metadata'sı
`evidence.acquired`, değiştirilmemiş worker zarfı `evidence.workerAcquired` içindedir.
Donanımdan gelen frame/cycle/completion kimliği korunur; yayınlanan karede çağıranın
source/cycle/frame kimliği bulunur. Deneysel kare orijinal worker kökenini taşır ve
trusted evidence üretmez. Cache yeni çekim yerine kullanılmaz.

## Close/drain

Close, çağrıldığı anda generation'ı emekliye ayırır; geç open/kare yayınlanmaz.
Idle oturum da kapanır. `close` future'ı gerçek worker çıkışı ve join sonrasında
tamamlanır. `closeBudgetExceeded` yalnız bütçeyi, `drained` worker bitişini,
`clean` temiz SDK kapanışını bildirir. Bütçe aşımı lease bırakmaz; SDK close
hatasında drained olsa bile global lease karantinası ve Fault korunur.

`shutdown()` yeni komutları keser, tüm context'leri kapatır ve ortak future döndürür.
Facade destructor yalnız shutdown ister; UI'de join yapmaz. Kendini tutan coordinator
donanım worker'larının gerçek çıkışına kadar yaşamaya devam eder; donanım thread'i
detach/kill edilmez. Ekran/proje/pencere sahibi explicit shutdown/drain future'ını
asenkron tamamlatmalı, uygulama çıkışını bunun ardından sürdürmelidir. Süresiz SDK
çağrısı cihaz bırakılmış gibi gösterilemez; işletim sistemi kapanışı çözüm değildir.

Yazılım kabulü ve açık fiziksel sınırlar [22b kapanışında](../../archive/2026-10-23-ccux-22b.md).
