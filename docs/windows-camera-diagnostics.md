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

