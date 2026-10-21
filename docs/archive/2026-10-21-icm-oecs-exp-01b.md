# ICM-OECS-EXP-01b — ışık açık gerçek 100 kare tekrarı

21 Ekim 2026. Uygulayıcı: Codex.
Durum: seri tekrarı tamam; EXP-01 fiziksel kabulü kısmi.

## Kapsam ve düzeltme

Kullanıcı önce sahne/ışığın sabit olduğunu belirtti, ardından ışıkları yanlışlıkla
kapattığını düzeltti. Işıklar açıkken aynı 20000 us / gain 0 ayarlarıyla 100 karelik
seri tekrarlandı. [Önceki kayıt](2026-10-21-icm-oecs-exp-01a.md) tarihsel haliyle
korunur; oradaki açıklanmamış kararma artık kullanıcı düzeltmesi ve bu tekrar
ile birlikte değerlendirilir. Yazılım hatası saptanmadı; ürün kodu değiştirilmedi.

Değişen belgeler: `docs/progress.md`, bu kayıt ve `docs/archive/README.md`.

## Yeni doğrulama ve kanıt

Mevcut Release `oecs_experimental_probe.exe`, sandbox dışında, exact
Opto Engineering Srl / COE-050-M-USB-050-IR-C / 02A2220000627 ile çalıştı.
`build/icm/oecs-exp-01-scene/series-lights-on-20000.json`:
**100/100 owned kare**, `EXPERIMENTAL_OWNED_FRAMES`, `shutdownReady=true`,
`stoppedQueueEmpty=true`. Mono8 2448×2048, origin(0,0), metadata kapalı;
profil/readback 20000 us, gain 0; FrameBurstStart/Software, burst=1,
cache=false, delay=0. Production admission ve tetik eşleşmesi false kalır.

`build/icm/oecs-exp-01-scene/series-lights-on-20000/` altında:

- `frame-0.pgm` ve `frame-99.pgm`, deneysel işaretli özgün raster ve JSON sidecar.
- `frame-0.pgm.png`, `frame-99.pgm.png`: kayıpsız görsel kontrol kopyaları;
  ikisi de açılıp incelendi. Noktalı plaka görünür, kenarlar yumuşak.
- `raster-stats.json`: ilk/son min–max **1–240**, ortalama **60.705 / 60.690**,
  stddev **55.282 / 55.270**, 255 değerinde piksel oranı **0**.

Yalnız ilk ve son raster kaydedilir; ara 98 kareye görsel kalite kabulü verilmez.
İlk/son görünürlüğü ve yakın parlaklığı, kullanıcının ışık açıklamasıyla uyumludur;
tetik tazeliği veya doğru parçayla eşleşme kanıtı değildir. Lens/odak ve bağımsız
referans doğrulanmadı; hassas ölçüm/kalibrasyon doğruluğu kabulü yoktur.

Ürün kodu değişmedi; yeni build/CTest koşulmadı. Belge boyut/bağlantı kontrolü ve
`git diff --check` bu kapanışta çalıştırıldı. Önceki test sonuçları yeni koşum değildir.

## Devam ve sınırlar

Fiziksel unplug/replug yapılmadı. Sonraki adım kullanıcıyla koordineli fiziksel
kopma/manuel yeniden açma gözlemi ve ölçüme uygun odak/sahne kabulüdür;
ardından EXP-02. EXP-01 bütünü, G1/production, MUI-04 gerçek ölçüm, PLC/robot
ve dağıtım kabulü kapanmaz. Eski pending-command karşı örneği geçerlidir.
