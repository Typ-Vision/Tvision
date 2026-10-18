# VisionFlow — güncel devir

Güncelleme: 18 Ekim 2026.
Son yönlendirme önceliklidir. Ayrıntılar [arşivde](archive/README.md); topluca okumayın.

## Devam noktası

- **ICM-PLAN-02 tamam:** kullanıcı kararıyla ortak ürün yazılımı ile marka/model
  donanım kabulü ayrıldı. [Kapanış](archive/2026-10-18-icm-plan-02.md).
  **Sıradaki ICM-04f:** mevcut çekirdek/worker üzerine test adaptörü ve ortak provider
  sözleşmesi; sonra **05a → 05b → 06a → 06b → 07a → 07b → 07c → 08a → 08b → 08c → 09a**.
  [04f kartı](architecture/industrial-camera-tasks-foundation.md),
  [Basler/Daheng paketleri](architecture/industrial-camera-tasks-backends.md).
  OECS native 04d/04e açık/ertelenmiş; ortak sıra için önkoşul değil. Test adaptörü
  normal ürün yoluna açılamaz; G2 yazılım, G0/G1/G3 backend, G4 kurulum kabulüdür.
- **Üç sıralı hazırlık tamam:** [04e-b](archive/2026-10-18-icm-04e-b.md) stress,
  [05a-a](archive/2026-10-18-icm-05a-a.md) keşif/resolve,
  [07a-a](archive/2026-10-18-icm-07a-a.md) kaynak referans codec; ürün bağlantısı yok.
- [ICM-04e-a](archive/2026-10-18-icm-04e-a.md) tamam: lease/worker, polling/shutdown,
  geç kare ret ve cleanup karantinası.
  [ICM-03c](archive/2026-10-18-icm-03c.md) owned ölçüm rasteri/geometri adayı da hazır;
  provider/persistence/UI bağlantısı yok. Native 04e/G1 hâlâ 04d'ye bağlı.
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
- [ICM-04c](archive/2026-10-17-icm-04c.md) tamam: bounded owned Mono8 probe/pool,
  exact 2448×2048 kare/close. Native stride/completion yok; exact tight Mono8 kabul.
  Önceki 02–04b teslimleri [arşiv dizininde](archive/README.md); 04a Claude Code.
  [Baseline](architecture/industrial-camera-baseline.md) şema/hash kaynağıdır.
- **ICM-REVIEW-01 kapalı**: SDK enumerate/owned kopya kilitli; setup revision
  denetlenir, A→B→A eski kalibrasyonu açmaz; rename/no-op/hash korunur.
  [Düzeltme ve kanıt](archive/2026-10-17-icm-review-01-fix.md).
- **ICM-01b fiziksel başlangıç açık**; [deneme](archive/2026-10-17-icm-01b.md).
  [Görev kartları](architecture/industrial-camera-tasks-foundation.md).
  ICM-00 veya tamamlanmış KMC görevlerini yeniden başlatmayın.
- Ana hedef Basler/Daheng PoE; deney cihazı COE-U USB3/OECS. Tanı probe'u üretim admission/tazelik
  garantisi değildir; trigger/lifecycle ve kalibrasyon/Editor/Runtime bağlantısı
  henüz uygulanmadı. Capture/production ret kapıları kabul yapılmadan kaldırılmaz.

## Göreve göre oku

AGENTS + bu özet + [README](../README.md), ardından [ana plan](architecture/industrial-camera-plan.md)
ve seçili kartın sözleşme/kabulü. Önce başlık/ID arayın.
[Devir](architecture/industrial-camera-handoff.md), [bulgular](architecture/industrial-camera-integration-assessment.md).

## Uygulanan durum ve korunacak sınırlar

- KMC-0–9 ve KMC-10a–e tamamlandı: reçetesiz N kamera/kurulum yönetimi,
  bağımsız kalibrasyon sihirbazları, kaynak başına immutable model ve çok kaynaklı
  Editor/Runtime yolu mevcut. [B kabulü](architecture/multi-source-release-acceptance.md),
  [Runtime/terminal sözleşmesi](architecture/multi-source-runtime-terminal.md).
- editor9, terminal7 ve vf.socket2 yolları mevcut. Kalıcı Runtime hazırlığı gerçek
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

- **Bu plan değişikliğinde:** yalnız belge/bağlantı/boyut ve diff kontrolü;
  build/CTest/UI/donanım koşulmadı. Kanıt: `build/icm/plan-02/`.
- Önceki 04e-b/05a-a/07a-a **4/4** core Release sonuçları arşivdedir; yeniden koşulmadı.
  `plc_socket_runtime` aralıklı başarısızlığı/kök neden açık kalır.
- Güncel UI `build/reference-ui/visionflow.exe`, core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- ICM-02 baseline `53bb972`; sonrası görev ID'li tek commit. Push kullanıcı
  isteğiyle. Ajanlar sıralıdır. Yerel INI korunur; kanıtlar `build/` altındadır.

## Kapanış ve sonraki adım

**Sıradaki ICM-04f, ardından ICM-05a.** 04e-a/b, 05a-a, 07a-a yeniden yapılmaz;
ölçüm adayı configure girdisi değildir. 05a/07a ürün kabulü açık.
[04d cancel incelemesi](archive/2026-10-18-icm-04d-cancel-audit.md): AcquisitionAbort
yok; DeviceReset garantisiz, çalıştırılmadı. Üretici taslağı gönderilmedi.
Delay=0, close/open/sessizlik kanıt değil; aynı deneyi tekrarlamayın. Native Unavailable.
[Bağlam bakımı](archive/2026-10-17-context-maintenance.md).
