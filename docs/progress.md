# VisionFlow — güncel devir

Güncelleme: 17 Ekim 2026. Güncel durum özetidir.
Kullanıcının son yönlendirmesi önceliklidir. Ayrıntılar [arşiv dizininde](archive/README.md);
başlangıçta arşivin tamamını okumayın.

## Devam noktası

- **ICM-04d kısmi: gerçek trigger tanısı tamam, native admission açık** (Codex).
  İki ayrı oturumda 100'er software trigger/100'er kare gözlendi; ayarlar geri
  yüklendi, bağımsız readback ve temiz close doğrulandı. Raw frameCounter/triggerIndex
  hep 0, frameNumber her açılışta 0–99. Ready/counter/epoch kanıtı oluşmadı.
  [Yeni tanı kaydı](archive/2026-10-17-icm-04d-native-diagnostic.md),
  [çekirdek/fixture](archive/2026-10-17-icm-04d.md).
  **Devam: ICM-04d korelasyon sözleşmesi** — FrameBurstStart/burst=1 fiziksel
  gözlemi FrameStart alias'ı değildir. Vendor ready/epoch garantisi veya ayrı,
  kanıtlanabilir sözleşme olmadan 04d tamamlandı veya 04e hazır denmez.
- [ICM-04c](archive/2026-10-17-icm-04c.md): bounded owned Mono8 probe/pool kabulü.
  Nested GenApi enum parser ve ClearImageBuffer sırası önceki kayıtta düzeltildi.
  **Exact cihazdan 2448×2048 Mono8 tanı karesi ve temiz close alındı.**
  Native stride/completion alanı yok: yalnız exact tight Mono8 kabul edilir.
- Önceki [ICM-04b](archive/2026-10-17-icm-04b.md): SFNC feature erişimi,
  transactional configure/readback, exact int/binary32 ve doğrulanan rollback.
  [04a](archive/2026-10-17-icm-04a.md): session/handle ömrü (Claude Code).
  [03b](archive/2026-10-17-icm-03b.md): profile/geometri;
  [03a](archive/2026-10-17-icm-03a.md): kimlik/rota.
  Backend kararı OECS C API ([ICM-02](archive/2026-10-17-icm-02.md)); şema/hash
  haritası [ICM-00 baseline](architecture/industrial-camera-baseline.md).
- **ICM-REVIEW-01 bulguları kapalı**: süreç genelinde SDK enumerate + tam cihaz
  bilgisi owned kopyası kilitli; create owned kaydı kullanır. Setup revision
  uyumlulukta karşılaştırılır; A→B→A eski kalibrasyonu geri açmaz; rename/no-op ve
  legacy hash korunur. [Düzeltme](archive/2026-10-17-icm-review-01-fix.md),
  [inceleme](archive/2026-10-17-icm-review-01.md).
- **ICM-01b fiziksel başlangıç açık**; [deneme](archive/2026-10-17-icm-01b.md).
  [Görev kartları](architecture/industrial-camera-tasks-foundation.md).
  ICM-00 veya tamamlanmış KMC görevlerini yeniden başlatmayın.
- Kamera hedefi COE-U USB3/OECS. Backend tanı probe'u üretim admission/tazelik
  garantisi değildir; trigger/lifecycle ve kalibrasyon/Editor/Runtime bağlantısı
  henüz uygulanmadı. Capture/production ret kapıları kabul yapılmadan kaldırılmaz.

## Göreve göre oku

1. AGENTS + bu özet + [README](../README.md) başlangıç paketidir.
2. ICM çalışmasında [ana planın](architecture/industrial-camera-plan.md) ortak
   kurallarını ve seçili görev kartını okuyun. Büyük belgelerde önce başlık/ID arayın.
3. Kartın gerektirdiği sözleşme/kabul/gereksinim bölümlerini açın. Gerektiğinde
   [bulgular](architecture/industrial-camera-integration-assessment.md) ve baseline'ın
   ilgili COE-U/OECS, probe veya CLI bölümüne genişletin; belgeleri topluca okumayın.
4. Tarihsel “sıradaki” ifadeleri güncel sıra değildir.
   [ICM devir şablonu](architecture/industrial-camera-handoff.md).

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
- `build/icm/04d/real-mono8.{pgm,png}` ve aynı dizindeki actual/failure logları:
  piksel aralığı **0–12**, sahne neredeyse siyah; sahne/ölçüm veya tazelik kabulü
  değil. O eski raster tanısında ayar/tetik yoktu; yeni tetik denemesi ayrı. GenTL discovery 1;
  read-only open **-1007** (C API başarısından ayrı).
- TriggerSelector yalnız **FrameBurstStart**, burst=1, cache=false. CounterEventSource
  yalnız Off/Line0/Line2; software sayaç kaynağı yok. On/Software aktifken de
  AcquisitionStatusSelector missing, AcquisitionStatus unavailable. Native ready/epoch açık. ClearImageBuffer
  **stream çalışırken**, sonra Stop gerekir; ters sıra gerçek CALLORDER verdi.
- Gerçek seri/open/grab ve 2×100 tanı tetiği var; H02 sahne/tazelik, 1000 cycle/teardown stress,
  viewer/release sahne kabulü, vendor redistributable ve temiz makine dağıtımı yok.
  Pool advertised payload bütçesidir, SDK/RSS hard sınırı değil.
- Fiziksel seri sürekliliği/ölçüm/PLC/robot/lens, native DPI/çoklu monitör,
  KUI-6/T8.4.2b.3c, CUDA ve ürün kurulum/dağıtım kabulü açık kalır.

## Son doğrulama ve çalışma ortamı

- **Yeni ICM-04d native tanı**: opt-in probe ve SDK Release build exit 0;
  ilgili SDK CTest **10/10**, `build/icm/04d-native/`, SDK'da
  `icm-04d-native-tests.xml`. UI/core suite yeniden koşulmadı; UI değişmedi.
- Önceki ICM-04d: core/UI **9/9**, SDK **10/10**; `build/icm/04d/`.
  Fixture fiziksel trigger admission kabulü değildir.
- ICM-04c tarihsel: core/UI 7/7, SDK 9/9; `build/icm/04c/`.
- 04a/04b/review test kapsamı ilgili arşivlerde. 03b core **74/74**, UI **87/88**:
  `plc_socket_runtime` aralıklı, kök neden açık. Tarihsel sonuçlar yeni koşum değildir.
- Önceki tam yazılım kabulü KMC-10e: UI **87/87**, core **73/73**;
  [B kabulü](architecture/multi-source-release-acceptance.md).
- Güncel UI `build/reference-ui/visionflow.exe`, core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- ICM-02'ye kadarki işler `53bb972` commit'indedir; sonrası her görev ID'li tek
  commit'tir. Push yalnız kullanıcı isteğiyle. Ajanlar sıralı çalışır; Codex yönetici,
  Claude Code alt görev uygulayıcısıdır. `config/local.ui.ini` değiştirilmedi;
  test/raporlar `build/` altında kalır.

## Kapanış ve sonraki adım

Sıradaki **ICM-04d ready/epoch sözleşmesi kararı**. Salt 100/100 gözlemi mevcut
actual sayaç sözleşmesini kapatmaz. Vendor garantisi veya doğrulanabilir alternatif
gerekiyor; aynı düğüm/tetik tanısını tekrarlamak engeli çözmez. 04e bundan sonradır.
ICM-01b için reboot sonrası OECS sahne görüntüsü/release ve G1 için 04e/stress açık.
[Bağlam bakımı](archive/2026-10-17-context-maintenance.md).
