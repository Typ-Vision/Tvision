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


## Deneysel Camera Manager (EXP-01)

Üretim yolu açılmadan yerel x64 OECS SDK ile Release derleyin:

```powershell
cmake -S . -B build/reference-ui -DVISIONFLOW_OECS_INCLUDE_DIR="C:/Program Files (x86)/OECS/Development/Includes" -DVISIONFLOW_OECS_RUNTIME_DLL="C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll"
cmake --build build/reference-ui -j 6
```

Camera Manager’da kayıtlı kamera/kurulum/profil ve yerel rota seçin; **Deneysel
OECS edinimini kullan** işaretini açıkça seçip **Önizleme** başlatın. Günlük kullanım
[README](../README.md) içindedir. Derleme seçeneği otomatik edinim veya production
izni değildir. Gerçek kullanılabilir sahne ve fiziksel Stop/kopma kabulü ayrıca gerekir.

Elle çalıştırılan `oecs_experimental_probe.exe absolute-dll absolute-output-directory
exposure-us count` aynı application worker yolunu kullanır; otomatik hardware CTest
değildir. `count` 1–1000; her komutun monoton bütçesi 3 saniyedir. Dosyalar üzerine
yazılmaz. Gerçek sonuçlar `build/icm/oecs-exp-01/` altında tutulur; fiziksel kimlik
probe içinde açıkça 02A2220000627 olarak sabittir. Bu probe tazelik kanıtı üretmez. İsteğe bağlı son argüman `--stop-in-flight`
bekleyen Grab’a Stop, `--timeout` 1 ms yazılım deadline gözlemi ekler; ikisi de
fiziksel command’ın gönderildiğini veya donanımsal iptal bariyerini kanıtlamaz.
