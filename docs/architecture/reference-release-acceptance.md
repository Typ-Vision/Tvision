# R8 — Ortak Referans nihai Release paketi ve devir

12 Ekim 2026. R1–R8 tamamlandı. R8 mevcut uygulamanın birleşik yazılım kabulü ve belge/devir paketidir; yeni algoritma, UI, şema, engine veya production yetkisi eklemez. Güncel binary `build/reference-ui/visionflow.exe`; engine `visionflow.execution/22`, editor8/raw binding1/world binding2/terminal4/vf.socket2 korunur. Referanssız terminal writer schema3 ve eski okuyucular değişmez.

## Tam paket kabulü

| Release paket | Yapılandırma | Tam CTest sonucu |
| --- | --- | --- |
| `build/reference-core` | Widgets OFF, ZXing OFF | 57/57; 92.14 sn |
| `build/reference-ui` | Widgets ON, pinned offline ZXing ON | 64/64; 217.82 sn |

İki build mevcut CMake cache ile `cmake --build build/reference-core -j 6` / `cmake --build build/reference-ui -j 6` üzerinden başarılıdır. Yeni configure/indirme/kurulum yapılmadı. README process DLL PATH kullanıldı:

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
ctest --test-dir build/reference-core --output-on-failure --output-junit r8-release-tests.xml
ctest --test-dir build/reference-ui --output-on-failure --output-junit r8-release-tests.xml
```

Nihai paketler localhost erişimiyle filtresiz, test atlamadan çalıştırıldı; her build'in tek tam koşumu kendi `r8-release-ctest.log` / `r8-release-tests.xml` kanıtını taşır. İlk sandbox koşumu `r8-full-ctest.log` / `r8-tests.xml` olarak saklanır; PLC HELLO ve UI Communication socket erişim sınırı nihai kabul yerine sayılmaz. Build logları `r8-build.log`; test bazında sonuçlar, binary SHA256 ve görsel SHA256 listesi `build/reference-ui/r8-evidence.json` içindedir. Bu yerel kanıt dağıtılabilir installer veya imzalı paket değildir.

## Birleşik kapsam ve görsel kabul

R6 [Editor/migration/cache matrisi](reference-editor-acceptance.md), R7 [immutable Runtime/trusted completion/terminal/restart/Logs/CSV](reference-production-acceptance.md), raw/world/inspection servisleri ve önceki U1–U4/ROI/kalibrasyon/socket regresyonları aynı son Release binary'leriyle sınandı. 77 kaynak-port/tüketici çifti capability/graph/codec matrisidir; her çift için fiziksel kamera corpus kabulü değildir. OFF capability retleri ve ON gerçek QR decode birlikte korunur.

Tam UI koşumu `ui_pose_tracking_smoke` ile yedi ortak Referans Properties panelini, Point/Pose ayrımını, pending/iptal/bağla/kaldır, gerçek mouse read-only, tek ofset, raw/world teaching, missing/Stop ve üç applicationType davranışlarını yeniden doğrular. Pattern/Shape × raw/world × Runtime/Logs × 1400×900/850×600 gerçek döndürülmüş raster görselleri yeniden üretildi. R8 kopyaları `build/reference-ui/r8-r5-*.png`, `r8-r7-*.png` ve `r8-*tracking*.png`; manifest dosyaları ve üretim zamanını ilişkilendirir.

Normal ve küçük Editor/Properties, raw/world Runtime ve Logs görüntüleri açılarak incelendi. Küçük ekranda dikey kaydırma gerekir; uzun Logs hücreleri kısalır. Mevcut Editor world üst durum metinleri çakışabilir; Properties/geometri erişimi korunur. Offscreen görsel kabul native DPI veya çoklu monitör kabulü değildir.

## Sonraki T8.4.2b.3c ve açık kapılar

- R7 snapshot/trusted completion/Runtime/kayıt/restart/Logs/CSV uygulamasını yeniden yapmadan kanıtı devral.
- Eksik rotated/occluded/symmetric/wrong-candidate corpusunu bağımsız raster ve oracle ile tamamla; bütün production yollarının açık kabul eşlemesini çıkar. Mevcut döndürülmüş örnekler corpusun tümünü kapatmaz.
- Non-affine H/every-tap programatik kabulünü production H corpus kabulüyle karıştırma; exact catalog-backed Runtime kanıtını ayrıca tamamla.
- Standalone Pattern/Shape ve eski poseTracking production kapıları corpus kabulüne kadar kapalıdır. Yeni referenceBinding production yolu R7 kapsamında açıktır; T8.4 bütünü tamamlanmış sayılmaz.
- K4 fiziksel ölçüm, P7/6D.2 PLC outbox/restart recovery, 6E robot, lensT15, native DPI/çoklu monitör, fiziksel hassasiyet/latency/CUDA/kurulum/dağıtım ayrı açık kalır. RobotGuidance applicationType kabulü robot pose veya hareket kabulü değildir.

Önceki dirty/untracked işler, açık uygulama ve üretim `config/local.ui.ini` korundu. INI SHA256: `950C8F2D5A09A3192ADC9307B793367A2347D534CCA989F0FD9C908B572C55B3`. Test ayarları/PNG/projeler/loglar build altındadır. Commit/yayın yapılmadı. [Sıradaki oturum talimatı](reference-system-plan.md#7-sonraki-uygulama-oturumu-için-başlangıç-talimatı--t842b3c), [ilerleme ve tam sonuçlar](../progress.md).
