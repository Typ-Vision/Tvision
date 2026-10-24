# CCUX-02 — Kamera ayarı ve readback application servisi

Durum: uygulandı; [kapanış ve kanıt](../../archive/2026-10-21-ccux-02.md). Önkoşul: CCUX-01. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C1–C2; [entegrasyon kartları](../industrial-camera-tasks-integration.md) ICM-05b.
Kaynak: `src/application/IndustrialCameraConfiguration.hpp/.cpp`, `src/application/IndustrialCameraProvider.hpp/.cpp`, `src/application/CameraLocalConfiguration.hpp/.cpp`, `src/application/MeasurementWorkspace.hpp`; `src/app/CameraManager.cpp` içindeki apply/editProfile akışını sembolle bul.

## Uygula

1. Kamera ID'sinden varsayılan kayıt ve uygun yerel rotayı çözen application girişini ekle; UI'nın profil JSON'u kurmasına gerek kalmasın.
2. Desteklenen exposure/gain aralıkları, birimleri ve başlangıç/readback değerlerini DTO olarak sun. Mevcut backend okuma desteği yetersizse hata/unsupported sun, sabit başarılı değer uydurma.
3. Ayar uygulama → gerçek readback → kayıt güncellemesini mevcut worker/lease/gate ile bağla. Donanım/disk arasında atomiklik iddia etme; rollback başarısızlığını açık unusable durumla ele al.
4. Auto/manual exposure/gain davranışını cihaz desteğine göre belirle. Profil değişimi ile kritik geometri değişimini ayrı bildir.
5. Exact hatırlanan/tek rotayı otomatik seç; çoklu belirsiz rotayı kullanıcı çözümüne bırak. Backend admission kapıları değişmesin.

## Kabul

Aralık dışı/NaN, unsupported gain, readback uyuşmazlığı, disconnect/apply failure, rollback failure, stale registry ve Runtime busy negatifleri. Aynı ayarda no-op; değişen kamera dışındaki kayıtlar korunur.

**CTest:** industrial_configuration, industrial_provider_contract, camera_transport, industrial_oecs_experimental

## Sınır ve kapanış

Sade ekran CCUX-03'te; kalibrasyon geçerliliği CCUX-06'da. Mevcut invalidation henüz gevşetilmez.
Kanıt: `build/ccux/ccux-02/`. AGENTS ve ortak sözleşme C6 ile kapat.

## Uygulanan servis sözleşmesi

`MeasurementWorkspace::cameraSettings(CameraId, expectedRevision, local, factory)`
reçetesiz, kaydedilmiş projeden varsayılan setup/profile ve exact fiziksel rotayı
çözer. Exact hatırlanan rota önceliklidir; artık eşleşmeyen ipucunda yalnız tek
exact rota otomatik seçilir, çoklu rota açık seçim ister. Yerel yol projeye yazılmaz.

Dönen `CameraSettingsSession`: `connect()` → owner thread'de `poll()` →
`Connected` sonrasında `snapshot()` ile typed exposure/gain/auto feature DTO'ları
(erişim, gerçek aralık, increment, birim, başlangıç değeri) ve acquisition readback;
`apply({exposureUs, gainDb})` → `poll()` → `Configured`/`Fault`.
İstenen optional değerler `snapshot.requested`, uygulananlar `snapshot.actual`
içindedir; profil istenen değeri korur, cihazın float gösterimi ayrı kalır.
Başarısız işlemde `error()` esas alınır; DTO'nun eski değeri başarı kanıtı değildir.
Tek oturum tek apply içindir; tekrar ayarlamak için kapatıp yeniden okuyun.

Provider'ın worker-only `createSettings(camera, device)` / `setting` /
`writeSetting` desteği acquisition `create(request)` admission yolundan ayrıdır.
Eski backend varsayılanı açık unsupported hatasıdır. Ürün edinim factory'si native
Unavailable kalır; test enjeksiyonu ürün ayarı değildir. CCUX-03b ile desteklenen
COE-050-M-USB-050-IR-C için `OecsCameraSettings` ayrı ürün ayar backend'idir;
`CameraSettingsService::available` kurulu runtime/kimlik desteğini denetler.
UI aynı workspace/worker/lease/CAS işlemini kullanır; SDK UI'ya taşınmaz.
Kamera ayarı oturumu
Start/Grab sunmaz. Oturum workspace'ten önce kapanır; destructor worker'ı join eder.

Mevcut profil sözleşmesi manual-only kalır. Auto kapalı olduğu readback ile
kanıtlanır veya backend'in desteklediği `Off` yazılıp tekrar okunur; destek yoksa
işlem reddedilir. Sayısal aralık/access her yazımda yeniden kontrol edilir.
Tam readback ve kimlik doğrulanmadan repository CAS commit yapılmaz. Disk/stale/
cihaz hatası önceki cihaz değerlerine rollback ve tam readback kontrolü ister;
doğrulanamıyorsa `unusable` + Fault, yeni oturumda yeniden kontrol gerekir.
Donanım ve disk arasında atomiklik veya SDK çağrısını zorla kesme garantisi yoktur.

`profileChanged`, `geometryChanged`, `deviceChanged` ayrı bildirilir. Gerçek no-op
donanım yazımı, revision ve invalidation üretmez. Diğer değişiklik/hatalarda mevcut
conservative kamera invalidation korunur; uyum gevşetmesi CCUX-06'dadır.
Pending setup lens/düzlem iddiası üretmez ve pending kalır; gerçek geometri DTO'da
bulunur, profil gerçek readback sonrası doldurulabilir. Camera Manager bu okunan
çözünürlüğü optik taslağa başlangıç olarak taşır; lens/düzlemi uydurmaz.
Hazır setup'ın değişen
geometrisi yeni revision/history ile kaydedilir. Matematik etkinleştirilmez.

OECS tetik modu açıkken auto node'ları unavailable olabilir. Ayar backend'i yalnız
bu durumda TriggerMode'u geçici Off yaparak auto node'unu okur/yazar; önceki modu
başarıda ve hatada geri yükleyip readback ile doğrular. Restore doğrulanamazsa
session Fault olur. Start/Grab/SoftwareTrigger çağrısı yapılmaz; bu işlem bekleyen
tetik komutu için cancel/drain garantisi veya production admission oluşturmaz.

