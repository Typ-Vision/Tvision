# VisionFlow — güncel devir

Güncelleme: 18 Ekim 2026.
Son yönlendirme önceliklidir. Ayrıntılar [arşivde](archive/README.md); topluca okumayın.

## Devam noktası

- **Üç sıralı hazırlık:** [04e-b](archive/2026-10-18-icm-04e-b.md) stress ve
  [05a-a](archive/2026-10-18-icm-05a-a.md) exact keşif/resolve tamam; sırada 07a-a.
- **Kullanıcı: destek beklemeden ilerle.** [ICM-04e-a](archive/2026-10-18-icm-04e-a.md)
  tamam: exact fiziksel lease, kalıcı worker, owner polling/JobSession shutdown,
  geç kare bastırma ve cleanup-failure karantinası; native/ürün bağlantısı yok.
  [ICM-03c](archive/2026-10-18-icm-03c.md) owned ölçüm rasteri/geometri adayı da hazır;
  provider/persistence/UI bağlantısı yok. Native 04e/G1 hâlâ 04d'ye bağlı.
- **ICM-04d kısmi: gecikmiş command Stop/Start'ı aşıyor; native Unavailable**.
  [Yeni uzun pozlama/gecikme deneyi](archive/2026-10-18-icm-04d-pending.md):
  12 senaryo/36 command/32 kare. Delay=500 ms iken Clear→Stop→Start sonrası,
  yeni command olmadan **8/8 eski kare** geldi: frame/trigger=1/0; sonraki yeni
  command=2/1. Exposure=500 ms/delay=0 sessiz 0/4; garanti değil.
  Genel Stop/Start bariyeri elendi. Belgeli pending-command cancel veya
  completion/drain/reset eksik; native 04e kapalı.
  [Önceki Clear deneyi](archive/2026-10-18-icm-04d-barrier.md): Clear-only eski kare; restart garanti değil.
  [Karar](architecture/industrial-camera-trigger-correlation.md),
  [üretici soru taslağı](architecture/industrial-camera-vendor-questions.md) (gönderilmedi).
  Belgeli bariyer + tek bekleyen tetik alternatif stratejidir; ready/iki sayaç
  aynı anda zorunlu değil. **İlk sensör satırını ölçümden çıkarma izni
  kalıcıdır**: 2448×2047, origin (0,1); dönüşüm servisi hazır, canlı ürün bağlantısı
  açık. FrameBurstStart sessiz FrameStart alias'ı olmaz.
  [Timestamp karşı örneği](archive/2026-10-17-icm-04d-timestamp.md): ham timestamp admission yok.
  [Metadata](archive/2026-10-17-icm-04d-correlation.md),
  [semantik](archive/2026-10-17-icm-04d-semantics.md): raster byte 4–11 sayaç;
  Chunk/Event bulunmadı; transport metadata yokluğu çıkarılamaz.
- [ICM-04c](archive/2026-10-17-icm-04c.md) tamam: bounded owned Mono8 probe/pool,
  exact 2448×2048 kare/close. Native stride/completion yok; exact tight Mono8 kabul.
  Önceki 02–04b teslimleri [arşiv dizininde](archive/README.md); 04a Claude Code.
  [Baseline](architecture/industrial-camera-baseline.md) şema/hash kaynağıdır.
- **ICM-REVIEW-01 kapalı**: SDK enumerate/owned kopya kilitli; create owned kayıtla.
  Setup revision karşılaştırılır; A→B→A eski kalibrasyonu açmaz; rename/no-op/legacy
  hash korunur. [Düzeltme](archive/2026-10-17-icm-review-01-fix.md),
  [inceleme](archive/2026-10-17-icm-review-01.md).
- **ICM-01b fiziksel başlangıç açık**; [deneme](archive/2026-10-17-icm-01b.md).
  [Görev kartları](architecture/industrial-camera-tasks-foundation.md).
  ICM-00 veya tamamlanmış KMC görevlerini yeniden başlatmayın.
- Kamera hedefi COE-U USB3/OECS. Backend tanı probe'u üretim admission/tazelik
  garantisi değildir; trigger/lifecycle ve kalibrasyon/Editor/Runtime bağlantısı
  henüz uygulanmadı. Capture/production ret kapıları kabul yapılmadan kaldırılmaz.

## Göreve göre oku

Başlangıç AGENTS + bu özet + [README](../README.md). Ardından
[ana plan](architecture/industrial-camera-plan.md), seçili görev kartı ve ilgili
sözleşme/kabul bölümleri; büyük belgelerde önce başlık/ID arayın. Belirsizlikte
[bulgulara](architecture/industrial-camera-integration-assessment.md) genişletin.
[Devir şablonu](architecture/industrial-camera-handoff.md).

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

- ICM-01a/02 offline karar tamamlandı. ICM-01b ve G0–G4/H01–H08 açık.
  **Reboot yapıldı** (kullanıcı + CIM: 17 Ekim 22:42:38 +03). Kullanıcı reboot sonrası
  OECS'yi açmadı; süreç taramasında OECS/MVS/Opto eşleşmesi yoktu.
- Sandbox içi C API/GenTL 0; **sandbox dışı C API 1 cihaz, exact open/readback/grab/
  close başarılı**. Kısıtlı ortamın 0 sonucunu cihaz yokluğu diye yorumlamayın.
  PnP: OE USB3 Machine Vision Camera, Status OK. Actual vendor/model/seri:
  Opto Engineering Srl / COE-050-M-USB-050-IR-C / **02A2220000627**.
  Device ID=1 seri değildir. Lens tam modeli/kurulum eksik.
- `build/icm/04d/real-mono8.{pgm,png}`: piksel **0–12**, sahne neredeyse siyah;
  sahne/ölçüm/tazelik kabulü değil. GenTL discovery 1, read-only open **-1007**;
  C API başarısından ayrı. Ayrıntı bağlı 04d kayıtlarında.
- TriggerSelector yalnız **FrameBurstStart**, burst=1, cache=false. CounterEventSource
  yalnız Off/Line0/Line2; ayrı FrameSpecInfo metadata sayacı var. On/Software aktifken de
  AcquisitionStatusSelector missing, AcquisitionStatus unavailable. Native ready/epoch açık. ClearImageBuffer
  **stream çalışırken**, sonra Stop gerekir; ters sıra gerçek CALLORDER verdi.
- Gerçek seri/open/grab ve metadata kapalı/açık 100'lük tanılar var; H02 sahne/tazelik, 1000 cycle/teardown stress,
  viewer/release sahne kabulü, vendor redistributable ve temiz makine dağıtımı yok.
  Pool advertised payload bütçesidir, SDK/RSS hard sınırı değil.
- Fiziksel seri sürekliliği/ölçüm/PLC/robot/lens, native DPI/çoklu monitör,
  KUI-6/T8.4.2b.3c, CUDA ve ürün kurulum/dağıtım kabulü açık kalır.

## Son doğrulama ve çalışma ortamı

- **ICM-05a-a yeni koşum**: core OFF Release build, ilgili CTest **4/4**;
  `build/icm/05a-a/`. 04e-b stress de bu oturumda 4/4 geçti.
- Eski sonuçlar arşivdedir; yeni koşum değildir.
  `plc_socket_runtime` aralıklı başarısızlığı/kök neden açık kalır.
- Güncel UI `build/reference-ui/visionflow.exe`, core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- ICM-02 baseline `53bb972`; sonrası görev ID'li tek commit. Push kullanıcı
  isteğiyle. Ajanlar sıralıdır. Yerel INI korunur; kanıtlar `build/` altındadır.

## Kapanış ve sonraki adım

**04e-a/b yazılım hazırlığı tamam**; sıradaki native ICM-04e, 04d belgeli cancel/drain/reset bariyerine
bağlı. Bağımsız alt görevde native kapı korunur. 03c helper'ını çalışan
kalibrasyon/Editor özelliği diye sunmayın. Ölçüm adayı kamera configure girdisi değildir.
[04d cancel incelemesi](archive/2026-10-18-icm-04d-cancel-audit.md): AcquisitionAbort
yok; DeviceReset garantisiz, çalıştırılmadı. Üretici taslağı gönderilmedi.
Delay=0, close/open/sessizlik kanıt değil; aynı deneyi tekrarlamayın. Native Unavailable.
ICM-01b için OECS sahne/release, G1 için 04e/stress açık.
[Bağlam bakımı](archive/2026-10-17-context-maintenance.md).
