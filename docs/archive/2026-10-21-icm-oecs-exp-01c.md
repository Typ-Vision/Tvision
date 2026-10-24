# ICM-OECS-EXP-01c — fiziksel kopma ve manuel yeniden açma

21 Ekim 2026. Uygulayıcı: Codex.
Durum: fiziksel USB kopma/manuel yeniden açma gözlemi tamam;
EXP-01 odak ve ölçüme uygun sahne kabulü açık.

## Kapsam ve değişen dosyalar

Kullanıcı test için hazır olduğunu bildirdi; ilk gerçek kare alındıktan sonra
USB kablosunu çıkardığını, kopma kaydı sonrasında yeniden taktığını doğruladı.
`tests/oecs_experimental_probe.cpp` içine elle kullanılan `--disconnect`
seçeneği eklendi. İlk raster/kanıt kaydı ardından readiness dosyası ve stderr
işareti üretir; 180 saniyelik pencerede, tek istek ve 100 ms aralıkla gerçek SDK
fault'unu gözler. Yapay transportLost olayı veya otomatik reconnect üretmez.
Fault sonrası kare sızıntısı ve kapanış kontrol edilir; başlangıç kare kanıtı
fault/cleanup hata raporlarında da korunur. Ürün/UI kodu değişmedi.

Tanı rehberi, progress ve arşiv dizini güncellendi. Yeni kanıtların kökü:
`build/icm/oecs-exp-01-disconnect/`.

## Yeni doğrulama ve kanıt

- Release `oecs_experimental_probe` hedefi derlendi: `build.log`.
- İlgili CTest **3/3**: `camera_worker_lease_contract`, `camera_worker_stress`,
  `industrial_oecs_experimental`; `tests.log`, `tests.xml`. Tam suite veya yeni
  UI görsel kabulü beyan edilmez; UI değişmedi.
- Sandbox dışı gerçek exact cihaz: Opto Engineering Srl /
  COE-050-M-USB-050-IR-C / **02A2220000627**. Profil 20000 us / gain 0,
  2448×2048 Mono8 origin(0,0), metadata kapalı; FrameBurstStart/Software,
  burst=1, cache=false, TriggerDelay=0. JSON'da gerçek readback korunur.
- `unplug.json`: ilk owned kare ardından kullanıcı USB'yi çıkardı;
  gerçek hata **OECS feature read failed**, acquisitionFaultObserved=true,
  frameAvailableAfterFault=false, stoppedQueueEmpty=true, shutdownReady=true;
  exit 0. Fault readiness sonrası 165 ek başarılı kareden sonra gözlendi.
  Bu sayı fiziksel çıkarma anından itibaren sayılmaz; unplug zaman damgası ve
  donanım komut aşaması ölçülmedi, algılama gecikmesi çıkarılamaz.
- `disconnected-open.json`: kullanıcı henüz kabloyu takmamışken ayrı oturum
  **OECS kamera bulunamadı**, exit 1; beklenen negatif, raster üretilmedi.
- `manual-reopen.json`: kullanıcı geri taktığını bildirdikten sonra ayrı, açık
  istekle aynı seri açıldı; **1/1 owned kare**, shutdownReady=true ve
  stoppedQueueEmpty=true, exit 0. Otomatik retry/reconnect yapılmadı.
- `unplug/frame-0.pgm` ve `manual-reopen/frame-0.pgm`: deneysel işaretli özgün
  rasterler + JSON sidecar. `.pgm.png` kayıpsız görsel kopyaları açılıp incelendi;
  iki görüntüde de plaka görünür, kenarlar yumuşak. `raster-stats.json`:
  min/max 0–240, ortalama 54.582 / 54.070, stddev 47.763 / 47.421;
  255 piksel oranı 0. Yalnız bu iki raster kaydedildi.

Bağlam boyut/bağlantı kontrolü `context-check.json`, diff whitespace kontrolü
`diff-check.log` altında kaydedilir. Önceki 100 seri/test sonuçları yeni koşum
olarak sunulmaz. Bu görev `ICM-OECS-EXP-01c` kimlikli tek commit ile kaydedilir;
push yapılmaz.

## Sınırlar ve devam

Bu tek fiziksel kopma çevrimi edinim hatası, çıkış bastırma, kablo yokken ret ve
manuel yeni oturum gözlemidir; native iptal/boşaltma bariyeri veya tazelik garantisi
değildir. Production admission ve triggerAssociationVerified false kalır.
Eski pending-command karşı örneği, native Unavailable ve G0/G1/G3/G4 sınırları
korunur. UI kopma etkileşimi bu probe koşumunda ayrıca test edilmedi.

Sonraki adım odak/ölçüme uygun sahne kontrolü, ardından EXP-02 Editor/MUI-04.
Bağımsız referans ve lens doğrulaması yok; mm doğruluğu veya kalibrasyon kabulü
verilmedi. Özgün MUI-04 raster/reçete/ROI yeniden üretimi hâlâ açık.
