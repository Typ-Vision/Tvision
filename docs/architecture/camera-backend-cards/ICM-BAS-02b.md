# ICM-BAS-02b — Basler edinim, preview, trigger ve G1

Durum: Yazılım/önizleme uygulandı; fiziksel G1 açık.
[Kapanış ve kanıt](../../archive/2026-11-05-icm-bas-02b.md).
Bağlı: [ICM-BAS-02a](ICM-BAS-02a.md).
Sonraki: [ICM-BAS-03](ICM-BAS-03.md).

## Oku

- [Ortak kabul](README.md), BAS-01/02a kapanışı ve gerçek yetenek envanteri.
- [Eski backend kartı](../industrial-camera-tasks-backends.md) BAS-02;
  [kabul matrisi](../industrial-camera-acceptance-plan.md) G1/H02–H05.
- [Oturum sözleşmesi](../camera-setup/acquisition-sessions.md),
  [korelasyon sözleşmesi](../industrial-camera-trigger-correlation.md).

## Davranış ve sahiplik

Basler binding/session ve IndustrialCameraBackend/provider; ürün factory
composition, gerekli preview/acquisition entegrasyonu, binding/stream/trigger
fixture'ları ve manuel fiziksel probe. Ortak worker/lease/owned frame kullanılır.

1. Exact oturumda configure/readback, Mono8 owned frame, bounded preview ve tek
   bekleyen software trigger. Gerçek aktif ROI/geometri/stride/payload doğrulanır;
   OECS 2448×2048/FrameBurstStart sabiti veya notice Basler'e uygulanmaz.
2. SDK buffer iadesinden önce sahip olunan görüntü kopyası; incomplete/packed/
   yanlış kaynak-geometri ve overflow ret. Preview karesi production sonucu olamaz.
3. Trigger-ready, camera-side FIFO, BlockID reset/invalid/wrap, timeout/cancel ve
   Stop/reopen için belgeli korelasyon stratejisi yaz. Host generation, artan sayaç
   veya yalnız flush tazelik kanıtı değildir. Belirsiz oturum tekrar kullanılmaz.
4. Stop/kopma generation'ı emekliye ayırır, geç başarı yayınlanmaz; drain/close
   owner thread'de tamamlanır, budget aşılırsa lease karantinası sürer.
5. G1 ve ortak admission şartları sağlanmadan production capability açılmaz.
   Test adaptörü ve caller evidence güvenilir terminal sonuç üretemez.

## Kabul

Regex: `^(acquisition_contract|owned_frame_contract|camera_lifecycle_contract|camera_worker_lease_contract|session_contract|industrial_provider_contract|industrial_configuration|industrial_runtime_admission_contract)$`; yeni Basler stream/trigger testlerini ekle.
Fixture: eski/duplicate/late frame, aynı piksel farklı yeni frame, reset/wrap,
trigger failure, timeout/cancel her faz, Stop→Start, incomplete, buffer iadesi,
queue full ve cleanup fault. Gerçek tek/100 trigger, sahne/olay korelasyonu,
preview→trigger, Stop/reopen ve kullanıcıyla USB kopma/geri takma kanıtı.
Ortak tam ON/OFF; görünür UI değişirse iki boyut görsel kontrol.
Kanıt: `build/icm/bas-02b/`. Belirsiz korelasyon G1'i açık bırakır.

Kapsam dışı: hardware trigger/PTP, renk/packed dönüşüm, GigE, eşzamanlı kamera,
OECS engelini kapatma, G3/G4/robot/PLC fiziksel kabulü.

## Basler korelasyon kararı — 5 Kasım 2026

Exact exclusive acA1920-40um USB oturumunda tek kayıtlı buffer ve worker üzerinde
tek bekleyen komut kullanılır. FrameStart/Software, diğer tetik bankalarının Off
olması ve FrameTriggerWait readback'i denetlenir. Preview Continuous/Trigger Off
ayrı oturumdur; her kare untrusted provenance taşır. Etkin ROI/geometri zaten
setup ile eşleşmelidir; edinim yolu ROI değiştirmez. Settings-only giriş yalnız
exposure/gain/auto yazar.

USB BlockID 0 geçerlidir; UINT64_MAX, duplicate, geriye gidiş/reset ve wrap ret
verir. Artan ID yalnız bu negatifleri yakalar, command→frame kanıtı değildir.
Timeout, cancel, command/queue hatası veya unsolicited frame oturumu zehirler;
Stop başarılı olsa bile aynı hatalı nesnede Start/Grab yapılamaz. Ortak worker
generation'ı geç başarıyı eler, gerçek close/drain ve lease karantinasını taşır.
Host flush yalnız buffer temizliğidir; yeniden Start/reopen G1 sağlamaz.

[Üretici C rehberi](https://docs.baslerweb.com/pylonapi/c/programmingguide)
USB kamera FIFO'sunun eski kareleri önce verdiğini, stream restart'ta BlockID'nin
0'a döndüğünü ve flush sonrası ID'nin invalid olabildiğini açıklar. Bu nedenle
device-side eski exposure/command/FIFO bariyeri exact firmware için belgelenip
sahne/olay ve kopma kabulüyle doğrulanana kadar **softwareTrigger capability ve
Runtime production kapalıdır**. Manuel tek/100 tetik başarısı bu açığı kapatmaz;
ICM-BAS-03 önkoşulu henüz sağlanmadı.
