# VisionFlow — güncel devir

Güncelleme: 18 Ekim 2026.
Kullanıcının son yönlendirmesi önceliklidir. Ayrıntılar [arşiv dizininde](archive/README.md);
Arşivi topluca okumayın.

## Devam noktası

- **ICM-04d kısmi: Clear-only bariyeri çürütüldü; native hâlâ Unavailable**.
  [Yeni karşı deney](archive/2026-10-18-icm-04d-barrier.md): 12 senaryo/36 command,
  27 kare. Hemen Clear sonrası yeni command olmadan 3/3 eski kare geldi.
  Clear→Stop→Start sonrası 0/6; fakat sessizlik garanti değil. Altı restart'ta
  aynı açık handle'ın sonraki frame/trigger sayacı 1/1'e döndü. Exact firmware
  stop completion/gecikmiş command/teslim semantiği açık; 04e başlamaz.
  [Karar](architecture/industrial-camera-trigger-correlation.md),
  [üretici soru taslağı](architecture/industrial-camera-vendor-questions.md) (gönderilmedi).
  Belgeli eski-frame bariyeri + exclusive tek bekleyen tetik adaydır; iki sayaç ve
  ready düğümü aynı anda zorunlu değil. **İlk sensör satırını ölçümden çıkarma izni
  kalıcıdır**: 2448×2047, origin (0,1) tanı doğrulandı; ürün geometri/kalibrasyon
  entegrasyonu açık. FrameBurstStart sessiz FrameStart alias'ı olmaz.
  [Timestamp karşı örneği](archive/2026-10-17-icm-04d-timestamp.md): latch çalışıyor,
  ham frameTicks>latch 12 eski kareyi yanlış geçirdi; fit ölçeğiyle admission yok.
  [2×100 metadata](archive/2026-10-17-icm-04d-correlation.md): sayaç 1–100,
  raster byte 4–11 metadata. [Semantik](archive/2026-10-17-icm-04d-semantics.md)
  Chunk/Event bulmadı; transport metadata yokluğu çıkarılamaz.
- [ICM-04c](archive/2026-10-17-icm-04c.md) tamam: bounded owned Mono8 probe/pool,
  exact 2448×2048 kare/close. Native stride/completion yok; exact tight Mono8 kabul.
  Önceki teslimler: [04b](archive/2026-10-17-icm-04b.md) transaction/rollback,
  [04a](archive/2026-10-17-icm-04a.md) session (Claude Code),
  [03b](archive/2026-10-17-icm-03b.md) geometri,
  [03a](archive/2026-10-17-icm-03a.md) kimlik,
  [02](archive/2026-10-17-icm-02.md) C API;
  [baseline](architecture/industrial-camera-baseline.md) şema/hash.
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
Tarihsel “sıradaki” ifadeleri güncel sıra değildir.
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

- **Yeni ICM-04d bariyer tanısı**: opt-in diagnostic Release build exit 0;
  12 senaryo/36 command/27 kare; 3 Clear-only eski kare ve 6 restart reseti.
  Dokuz sessiz pencere 362–379 ms; quiescence garantisi değil. Restore/cleanup,
  bağımsız readback ve JSON/layout/hash assertions geçti. `build/icm/04d-barrier/`.
  Mevcut SDK build'inde ilgili CTest **2/2 yeni koşum**. Ürün/UI değişmedi;
  ürün rebuild, tam suite/görsel/stress koşulmadı. Ön denemeler final kanıt değil.
  Önceki timestamp/ilk satır kanıtı `build/icm/04d-timestamp/` altında korunur.
- Eski test/build sonuçları arşivdedir; yeni koşum değildir.
  `plc_socket_runtime` aralıklı başarısızlığı/kök neden açık kalır.
- Güncel UI `build/reference-ui/visionflow.exe`, core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- ICM-02 baseline `53bb972`; sonrası görev ID'li tek commit. Push kullanıcı
  isteğiyle. Ajanlar sıralıdır. Yerel INI korunur; test/raporlar `build/` altındadır.

## Kapanış ve sonraki adım

Sıradaki **ICM-04d stop completion/epoch semantiği**: exact firmware için belgeli
Clear→Stop→Start garantisi, gecikmiş command/USB teslimi ve sayaç reset noktası.
Gerekirse uzun exposure karşı deneyi; kısa sessizlik yeterli değil. Taslak gönderilmedi.
İlk satır kararı kalıcı; ürün geometri/kalibrasyon invalidation açık. Native Unavailable,
04e bundan sonra. ICM-01b için OECS sahne görüntüsü/release ve G1 için 04e/stress açık.
[Bağlam bakımı](archive/2026-10-17-context-maintenance.md).
