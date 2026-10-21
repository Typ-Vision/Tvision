# VisionFlow — güncel devir

21 Ekim 2026.
[Arşiv](archive/README.md).

## Devam noktası

- **EXP-03 yazılım/500 seri hazır; fiziksel kopma açık.**
  [Kayıt](archive/2026-10-21-icm-oecs-exp-03.md): ayrı Runtime/Logs/CSV,
  Raw/px; 21 ilgili CTest, UI, Stop/deadline/restart. PLC/robot kapalı.
  1000 hedefi kullanıcı isteğiyle 500 oldu; son süreç zorunlu sonlandırıldı.

- **ICM-OECS-EXP-02 px geliştirme teslimi hazır.**
  [Kayıt/kanıt](archive/2026-10-21-icm-oecs-exp-02.md): canlı Editor ve px replay.
  Köken/etiket korunur; trusted completion yok. mm referansı ve
  deneysel kalibrasyon aday/aktivasyonu açık.
- **ICM-OECS-EXP-01 yazılımı hazır; fiziksel kabul kısmi.**
  [Yazılım](archive/2026-10-21-icm-oecs-exp-01.md),
  [ışık/100 seri](archive/2026-10-21-icm-oecs-exp-01b.md),
  [kopma/manuel açma](archive/2026-10-21-icm-oecs-exp-01c.md),
  [odak/100 seri](archive/2026-10-21-icm-oecs-exp-01d.md).
  Preview/Stop/PGM hazır; Stop/deadline çıktıyı bastırdı, kopmada kare yok.
  Görünür plaka px geliştirmesine yeterli; hassas optik/mm kabulü açık.
  Tetik eşleşmesi ve iptal/boşaltma garantisi yok.
- **MUI-01–03 tamam.** [03 tanı/replay](archive/2026-10-19-mui-03.md);
  01/02 ve tarihli test kanıtları arşivde.
- **MUI-04 sentetik corpus tamam, gerçek kabul açık.**
  [190 kontrol / sınırlar](archive/2026-10-19-mui-04.md),
  [kabul matrisi](architecture/measurement-usability-acceptance.md).
- **EUI-01–08 tamam; kullanıcı tekrar testleri OK.**
  [EUI-08 kanıtı](archive/2026-10-18-eui-08.md),
  [kullanıcı kabulü](archive/2026-10-18-eui-accept-01.md).
  Blob4 / engine28 / editor1–9 no-op korunur.
- [ICM-PLAN-02](archive/2026-10-18-icm-plan-02.md): ortak yazılım/donanım kabulü ayrı.
  [06a](archive/2026-10-18-icm-06a.md)/[06b](archive/2026-10-18-icm-06b.md)
  kalibrasyon/profil/aktivasyon tamam; 04f–05b kapanışları [arşivde](archive/README.md).
  09a/G2 tamam; 09b native G0/G1 bekliyor. [Backend kartları](architecture/industrial-camera-tasks-backends.md).
  OECS native 04d/04e açık/ertelenmiş; ortak yazılımı engellemez. Test adaptörü normal
  ürüne açılamaz; G2 yazılım, G0/G1/G3 backend, G4 kurulum kabulüdür.
- 04e-a/b, 05a-a, 07a hazır; [kapanışlar](archive/README.md).
  [03c ölçüm rasteri](archive/2026-10-18-icm-03c.md) canlı ürün yoluna bağlı değil.
  Doğrulanmış native 04e/G1, 04d'ye bağlı; deneysel yol ayrı planlandı.
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
  Timestamp/metadata sınırları bağlı korelasyon kararında korunur.
- [04c](archive/2026-10-17-icm-04c.md): owned tight Mono8 2448×2048;
  native stride/completion yok. [Baseline](architecture/industrial-camera-baseline.md)
  şema/hash kaynağıdır. [REVIEW-01](archive/2026-10-17-icm-review-01-fix.md) kapalı;
  enumerate kilidi, setup revision/A→B→A, rename/no-op/hash korunur.
- **ICM-01b fiziksel başlangıç açık**; [deneme](archive/2026-10-17-icm-01b.md).
  [Görev kartları](architecture/industrial-camera-tasks-foundation.md).
  ICM-00 veya tamamlanmış KMC görevlerini yeniden başlatmayın.
- Öncelik eldeki COE-U USB3/OECS; Basler/Daheng PoE cihaz bekliyor.
  Tanı probe'u üretim/tazelik garantisi değildir. Editor/Runtime bağlantısı hazır;
  doğrulanmış native edinim bekliyor. Deneysel kullanım production kabulü değildir.

Görevler: [ana plan](architecture/industrial-camera-plan.md), seçili kart ve sözleşme.

## Uygulanan durum ve korunacak sınırlar

- KMC-0–9/10a–e tamam: reçetesiz N kamera/kurulum, bağımsız kalibrasyon,
  kaynak başına immutable model ve çok kaynaklı Editor/Runtime.
  [B kabulü](architecture/multi-source-release-acceptance.md),
  [Runtime/terminal sözleşmesi](architecture/multi-source-runtime-terminal.md).
- editor9/engine28/terminal8–9/vf.socket2 mevcut; sürümleri baseline/kaynakla doğrulayın.
  Runtime hazırlığı sonuç değildir; restart aktif izin vermez.
- UI/application ve sonuç ayrımı AGENTS/README sözleşmesidir.
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
- Eski 04d rasteri karanlık, sahne/tazelik kabulü değil.
  GenTL discovery 1, read-only open **-1007**; C API başarısından ayrı.
  Ayrıntı bağlı 04d kayıtlarında.
- TriggerSelector **FrameBurstStart**, burst=1, cache=false; ready/epoch açık.
  Sayaç/node sınırları [korelasyon kararında](architecture/industrial-camera-trigger-correlation.md).
  ClearImageBuffer **stream çalışırken**, sonra Stop; ters sıra CALLORDER verdi.
- Gerçek seri/open/grab/100'lük metadata tanıları var; H02 sahne/tazelik, fiziksel
  stress, viewer/release, redistributable ve temiz makine dağıtımı açık.
  Pool advertised payload bütçesidir, SDK/RSS hard sınırı değil.
- Fiziksel seri sürekliliği/ölçüm/PLC/robot/lens, native DPI/çoklu monitör,
  KUI-6/T8.4.2b.3c, CUDA ve ürün kurulum/dağıtım kabulü açık kalır.

## Son doğrulama ve çalışma ortamı

- [09a/G2 kabul raporu](architecture/industrial-camera-release-acceptance.md);
  native G1/G3/G4 ve dağıtım açık.
- 07b/c execution/Editor binding, 08a/b/c admission/trusted terminal/Logs tamam;
  [kapanışlar](archive/README.md). Load beyan, production Run kabul ister.
- UI `build/reference-ui/visionflow.exe`; core `build/kmc9-core-off`.
  Build/PATH/başlatma README'dedir.

## Kapanış ve sonraki adım

**Devam: fiziksel kopma kabulü; EXP-02 deneysel kalibrasyon/mm açık.**
EXP-03 deneysel yol native production izni değildir. PGM+JSON açık Editor veya
Runtime seçimi ister; restart kapatır. EXP-01d optik/mm ve özgün MUI-04
raster/reçete/ROI açık; yeni sahne özgün problemi yeniden üretmez.
Manuel çok kaynak/kalibrasyon ve [09b](archive/2026-10-18-icm-09b-start.md)
G0/G1/H01–H08 açıktır. Ölçüm adayı configure girdisi değildir.
[Cancel incelemesi](archive/2026-10-18-icm-04d-cancel-audit.md): AcquisitionAbort yok;
DeviceReset garantisiz, çalıştırılmadı. Üretici taslağı gönderilmedi.
Delay=0, close/open/sessizlik tazelik kanıtı değildir. Native Unavailable korunur.
