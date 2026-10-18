# VisionFlow — güncel devir

18 Ekim 2026.
[Arşiv](archive/README.md).

## Devam noktası

- **EUI-07 tamam.** Kompakt Editor ve geniş viewer.
  [Kapanış](archive/2026-10-18-eui-07.md).
  [EUI-06](archive/2026-10-18-eui-06.md): Workspace pin/28 px şerit ve yüzer açılma.
  [EUI-05](archive/2026-10-18-eui-05.md): gruplu Toolbox/arama ve eski Edge korunur.
  Önceki [04](archive/2026-10-18-eui-04.md) N ok/canlı yay,
  [03](archive/2026-10-18-eui-03.md) zoom,
  [02](archive/2026-10-18-eui-02.md) sınır filtresi ve
  [01](archive/2026-10-18-eui-01.md) ROI kabulü korunur.
  Blob4 / engine28 / editor1–9 no-op korunur. **Sıradaki EUI-08.**
  [Kartlar](architecture/editor-refinement-plan.md).
- [ICM-PLAN-02](archive/2026-10-18-icm-plan-02.md): ortak yazılım/donanım kabulü ayrı.
  [06a](archive/2026-10-18-icm-06a.md)/[06b](archive/2026-10-18-icm-06b.md)
  kalibrasyon/profil/aktivasyon tamam; 04f–05b kapanışları [arşivde](archive/README.md).
  09a/G2 tamam; 09b native G0/G1 bekliyor. [Backend kartları](architecture/industrial-camera-tasks-backends.md).
  OECS native 04d/04e açık/ertelenmiş; ortak yazılımı engellemez. Test adaptörü normal
  ürüne açılamaz; G2 yazılım, G0/G1/G3 backend, G4 kurulum kabulüdür.
- Hazır: [04e-a](archive/2026-10-18-icm-04e-a.md),
  [04e-b stress](archive/2026-10-18-icm-04e-b.md),
  [05a-a resolve](archive/2026-10-18-icm-05a-a.md),
  [07a codec](archive/2026-10-18-icm-07a.md).
  [03c ölçüm rasteri](archive/2026-10-18-icm-03c.md) canlı ürün yoluna bağlı değil;
  native 04e/G1, 04d'ye bağlı.
- **ICM-04d kısmi: gecikmiş command Stop/Start'ı aşıyor; native Unavailable**.
  [Gecikme deneyi](archive/2026-10-18-icm-04d-pending.md): delay=500 ms iken
  Clear→Stop→Start sonrası yeni command olmadan **8/8 eski kare** geldi.
  Belgeli pending-command cancel/completion/drain/reset eksik; native 04e kapalı.
  [Karar](architecture/industrial-camera-trigger-correlation.md),
  [üretici soru taslağı](architecture/industrial-camera-vendor-questions.md) (gönderilmedi).
  Belgeli bariyer + tek bekleyen tetik alternatif stratejidir; ready/iki sayaç
  aynı anda zorunlu değil. **İlk sensör satırını ölçümden çıkarma izni
  kalıcıdır**: 2448×2047, origin (0,1); dönüşüm servisi hazır, canlı ürün bağlantısı
  açık. FrameBurstStart sessiz FrameStart alias'ı olmaz.
  [Timestamp](archive/2026-10-17-icm-04d-timestamp.md) admission yok;
  [metadata/semantik](archive/2026-10-17-icm-04d-semantics.md) sınırları korunur.
- [04c](archive/2026-10-17-icm-04c.md): owned tight Mono8 2448×2048;
  native stride/completion yok. [Baseline](architecture/industrial-camera-baseline.md)
  şema/hash kaynağıdır. [REVIEW-01](archive/2026-10-17-icm-review-01-fix.md) kapalı;
  enumerate kilidi, setup revision/A→B→A, rename/no-op/hash korunur.
- **ICM-01b fiziksel başlangıç açık**; [deneme](archive/2026-10-17-icm-01b.md).
  [Görev kartları](architecture/industrial-camera-tasks-foundation.md).
  ICM-00 veya tamamlanmış KMC görevlerini yeniden başlatmayın.
- Ana hedef Basler/Daheng PoE; deney cihazı COE-U USB3/OECS. Tanı probe'u üretim
  admission/tazelik garantisi değildir. Editor/Runtime bağlantısı hazır; native edinim bekliyor. Native/production kapıları korunur.

Görevler: [ana plan](architecture/industrial-camera-plan.md), seçili kart ve sözleşme.

## Uygulanan durum ve korunacak sınırlar

- KMC-0–9/10a–e tamam: reçetesiz N kamera/kurulum, bağımsız kalibrasyon,
  kaynak başına immutable model ve çok kaynaklı Editor/Runtime.
  [B kabulü](architecture/multi-source-release-acceptance.md),
  [Runtime/terminal sözleşmesi](architecture/multi-source-runtime-terminal.md).
- editor9, engine/28, terminal8–9 ve vf.socket2 yolları mevcut. Kalıcı Runtime hazırlığı gerçek
  çevrim sonucu değildir; restart aktif kullanım iznini geri yüklemez. Şema/codec
  değişiminde numaraları baseline ve kaynakla doğrulayın.
- UI application servislerini kullanır. Üç applicationType ortak Editor/Runtime'dır.
  Reçete, execution ve parça kararı ayrıdır; gerçek sonuç olmadan PASS/skor/pose yoktur.
- Kaynak/model/kalibrasyon invalidation, legacy no-op/hash davranışı korunur.
  Standalone Pattern/Shape ve eski poseTracking production kabulünün açık sınırları
  T8.4.2b.3c kapsamındadır; yeni teslimler bunları topluca kapatmaz.

## Açık bağımlılıklar ve kabul

- ICM-01a/02 offline karar tamamlandı. OECS ICM-01b ve G0–G4/H01–H08 açık.
  Basler/Daheng model/SDK/cihaz kanıtı henüz yok; kendi G0/G1/G3/G4 kapıları açık.
  **Reboot yapıldı** (kullanıcı + CIM: 17 Ekim 22:42:38 +03); sonrasında OECS açılmadı.
- Sandbox içi C API/GenTL 0; **sandbox dışı C API 1 cihaz, exact open/readback/grab/
  close başarılı**. Kısıtlı ortamın 0 sonucunu cihaz yokluğu diye yorumlamayın.
  PnP: OE USB3 Machine Vision Camera, Status OK. Actual vendor/model/seri:
  Opto Engineering Srl / COE-050-M-USB-050-IR-C / **02A2220000627**.
  Device ID=1 seri değildir. Lens tam modeli/kurulum eksik.
- `build/icm/04d/real-mono8.{pgm,png}`: piksel **0–12**, sahne neredeyse siyah;
  sahne/ölçüm/tazelik kabulü değil. GenTL discovery 1, read-only open **-1007**;
  C API başarısından ayrı. Ayrıntı bağlı 04d kayıtlarında.
- TriggerSelector **FrameBurstStart**, burst=1, cache=false; ready/epoch açık.
  Sayaç/node sınırları [korelasyon kararında](architecture/industrial-camera-trigger-correlation.md).
  ClearImageBuffer **stream çalışırken**, sonra Stop; ters sıra CALLORDER verdi.
- Gerçek seri/open/grab/100'lük metadata tanıları var; H02 sahne/tazelik, fiziksel
  stress, viewer/release, redistributable ve temiz makine dağıtımı açık.
  Pool advertised payload bütçesidir, SDK/RSS hard sınırı değil.
- Fiziksel seri sürekliliği/ölçüm/PLC/robot/lens, native DPI/çoklu monitör,
  KUI-6/T8.4.2b.3c, CUDA ve ürün kurulum/dağıtım kabulü açık kalır.

## Son doğrulama ve çalışma ortamı

- **ICM-09a/G2 tamam:** son kaynak Release UI ON/ZXing ON **105/105**, core
  OFF/ZXing OFF **89/89**, optional SDK fixture **2/2**.
  [Kapanış](archive/2026-10-18-icm-09a.md).
  [Birleşik rapor](architecture/industrial-camera-release-acceptance.md),
  kanıt `build/icm/09a/`; native G1/G3/G4 ve dağıtım açık.
- **08b/08c tamam:** trusted completion/terminal8–9 ve Runtime/Logs/CSV.
  [08b](archive/2026-10-18-icm-08b.md), [08c](archive/2026-10-18-icm-08c.md).
  [08a admission](archive/2026-10-18-icm-08a.md): Load beyan, Run kabul ister; native kapalı.
- [07c](archive/2026-10-18-icm-07c.md) Editor profil seçimi/açık binding ve worker bağlantısı.
  [07b](archive/2026-10-18-icm-07b.md) execution/provider/TargetOnly tamam.
  `plc_socket_runtime` aralıklı başarısızlığı/kök neden açık kalır.
- UI `build/reference-ui/visionflow.exe`; core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- Baseline `53bb972`; ID'li tek commit, istekle push. Yerel INI korunur; kanıtlar `build/`.

## Kapanış ve sonraki adım

**EUI-06 ve EUI-07 sıralı tamam. Sıradaki EUI-08 birleşik kabul.**
Sonra kullanıcı testi/düzeltme ve kamera önkoşulları.
[ICM-09b](archive/2026-10-18-icm-09b-start.md) G0/G1 bekliyor; H01–H08 açık.
Basler/Daheng model/SDK 01→02 veya OECS belgeli pending-command bariyeri gerekir.
Tamamlanan hazırlıklar tekrarlanmaz; ölçüm adayı configure girdisi değildir.
[04d cancel incelemesi](archive/2026-10-18-icm-04d-cancel-audit.md): AcquisitionAbort
yok; DeviceReset garantisiz, çalıştırılmadı. Üretici taslağı gönderilmedi.
Delay=0, close/open/sessizlik kanıt değil; aynı deneyi tekrarlamayın. Native Unavailable.
[Bağlam bakımı](archive/2026-10-17-context-maintenance.md).
