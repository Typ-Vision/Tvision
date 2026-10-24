# Windows — OECS / GenTL tanısı

PowerShell 7 ile mevcut Release probe'unu explicit x64 U3V producer üzerinden çalıştırın:

```powershell
pwsh -NoProfile -File scripts/diagnose-camera.ps1 -OpenCheck
```

`-ProducerPath`, `-ProbePath`, `-OutputDirectory` değiştirilebilir; rapor varsayılanı
`build/icm/01a/local/diagnostic.json`. Sürüm/hash, mimari, ortam ve exit code kaydedilir; kalıcı ortam değişmez. `NO_DEVICES`/`DEVICES_DISCOVERED` exit 0,
diğer durumlar exit 1'dir. Bu GenTL probe grab yapmaz; sıfır cihaz kabul değildir.

Fiziksel erişim sandbox içinde 0 cihaz dönebilir; sandbox dışı C API exact open/grab/close
kanıtı ve opt-in probe kullanımı [04d kaydındadır](archive/2026-10-17-icm-04d.md).
Probe üretim admission değildir; native production kapalıdır. Ayrı
[100-trigger/metadata tanısı](archive/2026-10-17-icm-04d-correlation.md)
geçici ayarları geri yükler.


## İsteğe bağlı aşama süreleri — CCUX-22a

`VISIONFLOW_ACQUISITION_TIMING` değişkenine yazılabilir **mutlak JSONL dosyası**
verildiğinde normal komutlar istek/oturum ID'si ve monotonic süre kaydeder.
Ürün ekranına mod veya onay eklenmez; değişken yokken tanı kapalıdır.
Yazım akışı açık tutulur; kayıtlar worker işlemi, edinim/kapanış ve UI teslim/
ilk paint sınırlarında flush edilir. Log dosyasını okurken bu sınırı bekleyin.

```powershell
$env:VISIONFLOW_ACQUISITION_TIMING = 'C:/Projects/Tvision/build/ccux/ccux-22a/manual-trace.jsonl'
.\scripts\run-local.ps1
# Uygulama kapandıktan sonra aynı terminalde:
Remove-Item Env:VISIONFLOW_ACQUISITION_TIMING
python scripts/summarize-acquisition-timing.py build/ccux/ccux-22a
```

JSONL aşamaları iç içe **inclusive** aralıklardır; süreleri toplayarak toplam
gecikme üretmeyin. Başarı, hata, iptal, deadline ve superseded örnekleri ayrıdır.
`ui.set-image` yalnız atamadır; `ui.command-to-first-paint` Qt paint tamamlanmasına
kadar olan aralıktır, monitör taraması değildir. Gizli/çizilmeyen viewer için
başarılı paint örneği oluşmaz. Session close/join ayrı ölçülür; açılış ve her kare
öncesi hazırlık giderleri stream start/trigger/wait ile karıştırılmaz.

Elle fiziksel servis baseline'ı için, kullanılmamış mutlak kanıt dizini seçin:

```powershell
.\scripts\measure-acquisition-baseline.ps1 -OutputDirectory 'C:/Projects/Tvision/build/ccux/ccux-22a/new-physical-run'
```

Bu komut Editor/Runtime/kalibrasyon servislerinde aynı kamera ve 56000 µs/0 dB
profiliyle üç yeni süreçte 20'şer çekimi tanı kapalı/açık tekrarlar. GenTL keşfi,
exact SDK enumerate/open ve readback dahildir; gerçek UI tıklama→paint dahil
değildir. Probe, DLL/producer hash ve ortamı kaydeder; ilk hata seriyi keser.
Dosyalar ve geçici projeler `build/` altında kalır. Cihaz erişimi yoksa başarısız
seri fiziksel baseline sayılmaz. UI fiziksel gecikmesi ve native/mm kabulü ayrıdır.

## Camera Manager ve OECS runtime

Üretim yolu açılmadan yerel x64 OECS SDK ile Release derleyin:

```powershell
cmake -S . -B build/reference-ui -DVISIONFLOW_OECS_INCLUDE_DIR="C:/Program Files (x86)/OECS/Development/Includes" -DVISIONFLOW_OECS_RUNTIME_DLL="C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll"
cmake --build build/reference-ui -j 6
```

Camera Manager'da kamerayı kaydedin, ayarları okuyun ve gerçek optik/geometri
taslağını tamamlayın. **Tek görüntü / Canlı görüntü / Durdur** normal komutları
uygun backend'i application katmanında seçer; ayrıca OECS seçim kutusu yoktur.
Ayrıntılar'daki OECS sürücü DLL yolu yerel ayardır. Eski FrameStart profili
FrameBurstStart / Software'e sessiz dönüştürülmez. Günlük kullanım
[kamera/Runtime rehberindedir](camera-runtime-usage.md). Derleme seçeneği kendi
başına capture veya üretim başlatmaz. Teknik tetik/cancel engelleri ve bağımsız
fiziksel Stop/kopma/mm kabulü ayrı kalır.

Elle çalıştırılan `oecs_experimental_probe.exe absolute-dll absolute-output-directory
exposure-us count` aynı application worker yolunu kullanır; otomatik hardware CTest
değildir. `count` 1–1000; her komutun monoton bütçesi 3 saniyedir. Dosyalar üzerine
yazılmaz. Gerçek sonuçlar `build/icm/oecs-exp-01/` altında tutulur; fiziksel kimlik
probe içinde açıkça 02A2220000627 olarak sabittir. Bu probe tazelik kanıtı üretmez. İsteğe bağlı son argüman `--stop-in-flight`
bekleyen Grab’a Stop, `--timeout` 1 ms yazılım deadline gözlemi ekler; ikisi de
fiziksel command’ın gönderildiğini veya donanımsal iptal bariyerini kanıtlamaz.

`--disconnect` ilk gerçek kareyi kaydeder, kanıt dizininde `disconnect-ready.txt`
oluşturur ve stderr'e `DISCONNECT_READY` yazar. Bundan sonra kullanıcı USB'yi
çıkarırken en fazla 180 saniye boyunca tek bekleyen istekle, kareler arasında
100 ms bekleyerek edinir. İlk gerçek acquisition fault'ta durur; hata, kuyruk ve
shutdown durumunu raporlar, otomatik yeniden bağlanmaz. Fault tek başına fiziksel
kopma kanıtı değildir; kullanıcı çıkarma/takma bildirimiyle birlikte değerlendirin.
Kablo çıkarılmışken ayrı normal probe isteğiyle open reddini kaydedin; kullanıcı
geri taktıktan sonra yeni kanıt dizininde normal tek kare isteğiyle manuel reopen
gözlemi alın. Aradaki başarılı kare sayısı, USB çıkarıldıktan sonraki kare sayısı
veya kopma algılama süresi olarak yorumlanamaz.

### CCUX-15 plakasız manuel kontroller

Plaka rasterini uygulamanın mevcut servisleriyle incelemek için:
`calibration_plate_test.exe --inspect-circles absolute-raw-image columns rows
spacing-mm absolute-new-report.json`. Deneysel PGM için sidecar/hash denetlenir;
bu açık manuel mod dosyayı yeniden etiketlemez. Çıktı yalnız yeni dosyaya yazılır.
Exit 1 algılama/yükleme reddidir; exit 0 yalnız matematiksel grid fitidir,
operatör sıra incelemesi, model aktivasyonu veya fiziksel doğruluk kabulü değildir.

`oecs_experimental_probe ... exposure-us 1 --preview-60s`, Camera Manager ile
aynı `CameraPreviewSession` üzerinden 60 saniye sıralı edinim yapar; bu modda
`count` yerine süre kullanılır. JSON stdout'unu dosyaya yönlendirin: tamamlanan
kare/FPS, süre, Stop generation/kare temizliği ve drain; ilk/son PGM+JSON ayrı
yazılır. `--wrong-camera` bulunmayan seri, `--wrong-geometry` 2447×2048 istek
ile ret bekler (probe exit 1; PGM oluşmamalı). Sonrasında exact readback alın.
Probe'un lens/düzlem alanları doğrulanmamış etiketlerdir; fiziksel optik kabulü
veya ürün UI'sındaki eksik kurulum kapısının geçildiği anlamına gelmez.

`oecs_camera_settings_test.exe --physical-update absolute-dll absolute-new-report.json`
gerçek keşif/kimlik ve geçici projede exposure'ı aralıkta 1000 µs değiştirir,
gain'i raporlanan karşı sınıra alır; her kayıttan sonra servisi yeniden açar.
Başarılı koşum eski scalar değerleri geri yükleyip readback'i raporlar.
Hata başarı sayılmaz; sonraki `--physical` yeniden açma/readback ile mevcut
durumu kontrol edin. Bu test görüntü almaz. 22 Ekim denemesinde 7000 µs / 1 dB
isteği readback farkıyla reddedildi; sessiz tolerans genişletilmedi.

## Az görsel tüketimiyle oturum kontrolü — CCUX-22e

Önce test raporu, open/configure/grab/close sayaçları, readback ve hata kayıtlarını
inceleyin. Bağlantı ömrü için plaka algılama veya her karenin görsel yorumlanması
gerekmez. UI kontrolünde değişen akıştan bir normal ve bir küçük pencere görseli
yeterlidir; yalnız sorun varsa ilgili bölgeyi ayrıca inceleyin. Özgün kanıtları
`build/ccux/<kart>/` altında koruyun; geçmiş kanıtın üzerine yazmayın.

Manuel `oecs_experimental_probe ... exposure-us 2 --calibration-session-run`
aynı kalibrasyon oturumunda iki plakasız çekim ve clean/drain kapanışı dener.
`VISIONFLOW_ACQUISITION_TIMING` ile hazırlık/çekim/kapanış çağrıları kaydedilir.
Raster dosyası veya plaka analizi üretmez; readback raporu görüntü kalitesi,
tetik-kare korelasyonu ya da fiziksel mm doğruluğu kabulü değildir. Kamera
bulunamazsa tekrar tekrar çekim istemeyin; SDK hatasını ve kapanışı kaydedin.
Eski `--calibration-run` her-kare bağlantı baseline'ını korur.
