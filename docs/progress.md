# VisionFlow — güncel devir

Güncelleme: 17 Ekim 2026. Bu dosya güncel durum özetidir; tarihsel günlük değildir.
Kullanıcının son yönlendirmesi önceliklidir. Ayrıntılar [arşiv dizininde](archive/README.md);
başlangıçta arşivin tamamını okumayın.

## Devam noktası

- **Son ürün teslimi: ICM-04a backend session/handle ömrü** (Claude Code):
  SDK'sız `OecsCApi` sınırı, opsiyonel dinamik binding, worker'a bağlı `OecsSession`;
  her aşamada ters cleanup, busy tanısı, canlı handle varken unload yok; gerçek open yok.
  [Kapanış](archive/2026-10-17-icm-04a.md). Önceki 03b profile/geometri
  [kapanışta](archive/2026-10-17-icm-03b.md), 03a kimlik/rota [kapanışta](archive/2026-10-17-icm-03a.md).
  Backend kararı OECS C API ([ICM-02](archive/2026-10-17-icm-02.md)); şema/hash haritası
  [ICM-00 kaydındadır](architecture/industrial-camera-baseline.md).
- **ICM-REVIEW-01 bulguları kapatıldı — ICM-REVIEW-01-FIX** (Codex): SDK enumerate
  ve tam cihaz bilgisi kopyası süreç genelinde kilitli; create owned kopyayı kullanır.
  Setup revision uyumlulukta karşılaştırılır; A→B→A eski kalibrasyonu geri açmaz,
  rename/no-op ve legacy hash korunur. [Kapanış ve kanıt](archive/2026-10-17-icm-review-01-fix.md).
  [İnceleme](archive/2026-10-17-icm-review-01.md) arşivdedir.
  **Sonraki görev ICM-04b configure/readback**.
  **ICM-01b fiziksel başlangıç açık**; [deneme kaydı](archive/2026-10-17-icm-01b.md).
  [Görev kartları](architecture/industrial-camera-tasks-foundation.md).
  ICM-00 veya tamamlanmış KMC görevlerini yeniden başlatmayın.
- Kamera hedefi COE-U USB3/OECS. Endüstriyel backend/feature/stream/grab ve
  kalibrasyon/Editor/Runtime entegrasyonu henüz uygulanmadı; capture/production
  ret kapıları kabul yapılmadan kaldırılmaz.

## Göreve göre oku

1. AGENTS + bu özet + [README](../README.md) başlangıç paketidir.
2. ICM çalışmasında [ana planın](architecture/industrial-camera-plan.md) ortak
   kurallarını ve seçili görev kartını okuyun. Büyük belgelerde önce başlık/ID arayın.
3. ICM-01a için [mevcut bulguların](architecture/industrial-camera-integration-assessment.md)
   COE-U/OECS ve probe bölümleri ile baseline'ın camera_transport/CLI envanteri
   başlangıç haritasıdır. Sözleşme/kabul belgelerinde kartla ilgili bölümleri okuyun.
4. Gereksinim, mimari ve eski kayıtları yalnız ilgili bölüm/kanıt gerektiğinde açın;
   bağımlılık veya çelişki varsa kapsamı genişletin. Arşivdeki eski “sıradaki” ifadeleri
   güncel sıra değildir. [ICM devir şablonu](architecture/industrial-camera-handoff.md).

## Uygulanan durum ve korunacak sınırlar

- KMC-0–9 ve KMC-10a–e tamamlandı: reçetesiz N kamera/kurulum yönetimi,
  bağımsız kalibrasyon sihirbazları, kaynak başına immutable model ve çok kaynaklı
  Editor/Runtime yolu mevcut. [B kabulü](architecture/multi-source-release-acceptance.md),
  [Runtime/terminal sözleşmesi](architecture/multi-source-runtime-terminal.md).
- editor9, terminal7 ve vf.socket2 yolları uygulanmış durumda. Kalıcı Runtime
  hazırlığı gerçek çevrim sonucu değildir; restart aktif kullanım iznini geri yüklemez.
  Yeni şema/codec değişiminde numaraları baseline ve güncel kaynakla doğrulayın.
- UI application servislerini kullanır. Üç applicationType ortak Editor/Runtime'dır.
  Reçete, execution ve parça kararı ayrıdır; gerçek sonuç olmadan PASS/skor/pose yoktur.
- Kaynak/model/kalibrasyon değişimi ve legacy no-op/hash davranışı korunmalıdır.
  Standalone Pattern/Shape ve eski poseTracking production kabulünün açık sınırları
  T8.4.2b.3c kapsamındadır; yeni referans teslimleri bunları topluca kapatmaz.

## Açık bağımlılıklar ve kabul

- ICM-01a tamamlandı; ICM-02 offline karar tamamlandı. Fiziksel ICM-01b ve
  G0–G4/H01–H08 açıktır. Yeni 01b U3V probe ve 02 C API enum **0 cihaz** gördü;
  open/grab denenmedi. GenTL remote XML/nodemap consumer doğrulanamadı.
- Kullanıcı: reboot **yapılmadı**, kamera bağlı, OECS'de görüntü alındı ve program
  kapatıldı; release kesin değil. Exact model/seri/Mono8 raster araçla doğrulanmadı.
  OECS isimli süreç görülmedi; CIM boot/PNP sorgusu erişim hatası verdi.
  Device ID=1 seri değildir. Lens Basler 25 mm beyanı; tam model/kurulum eksik.
- C API x64 header compile/load/version/enum çalıştı; feature/unit/readback ve
  stream/frame ABI/cleanup gerçek cihazda açık. Vendor redistributable izni ve
  temiz makine dağıtımı doğrulanmadı; runtime yokluğunda açık unavailable gerekir.
- Fiziksel kamera/seri sürekliliği/ölçüm/PLC/robot/lens, native DPI/çoklu monitör,
  KUI-6/T8.4.2b.3c kalanları, CUDA ve ürün kurulum/dağıtım kabulü açık kalır.
  Fixture/offscreen başarıları bu kapıları kapatmaz.

## Son doğrulama ve çalışma ortamı

- ICM-REVIEW-01-FIX: core/UI Release build exit 0; her birinde hedefli **6/6**
  CTest, SDK'lı yapıda binding/lifetime/setup **3/3** geçti.
  Tam suite koşulmadı. Önceki iki kusurun probe'u artık doğru kamera ve üç uyumluluk
  reddini gösteriyor. Kanıt `build/icm/review-01-fix/`; JUnit ilgili build'lerde
  `icm-review-01-fix-final-tests.xml` (ön koşum kayıtları da korundu).
- ICM-04a: Release UI/core tam build exit 0; tam suite core **75/75**, UI **89/89**.
  SDK'lı opt-in build `build/icm/04a/sdk` 2/2: DLL yüklendi, sürüm 0x3050200, **0 cihaz**.
  Kanıt `build/icm/04a/`, JUnit `<build>/icm-04a-tests.xml`; UI değişmedi.
  ICM-03b tarihsel: core 74/74, UI 87/88 (`plc_socket_runtime` aralıklı; kök neden açık).
- ICM-02/01a tarihsel: UI/core build ve her birinde ilgili **2/2 CTest** geçti;
  tam suite koşulmadı. ABI/export/negatif senaryolar ve tanı kanıtları
  `build/icm/02/`, `build/icm/01a/` ve ilgili kapanış kayıtlarında korundu.
- Yeni SDK koşumu `build/icm/01a/local/diagnostic.json`; sürüm/hash/PE, ortam
  farkı, doğrudan import envanteri ve exit 0 kayıtlı. Bu fiziksel kabul değildir.
- ICM-00 tarihsel: UI/core Release build ve ilgili core **12/12 CTest** geçti;
  tam suite koşumu değildir. Ayrıntı/kanıt [baseline'da](architecture/industrial-camera-baseline.md)
  ve `build/icm/00/verification.json` içinde korunur.
- Önceki tam yazılım kabulü KMC-10e: UI **87/87**, core **73/73**;
  ayrıntı ve kapsam [B kabul belgesinde](architecture/multi-source-release-acceptance.md).
- Güncel UI `build/reference-ui/visionflow.exe`, core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- ICM-02'ye kadarki işler `53bb972` commit'indedir; bundan sonra her görev ID'li
  tek commit'tir. Codex yönetici, Claude Code alt görev uygulayıcısıdır (AGENTS).
  `config/local.ui.ini` değiştirilmedi; test/raporlar `build/` altında kalır.

## Kapanış ve sonraki adım

ICM-01a [tanı kapanışı](archive/2026-10-17-icm-01a.md) ve diğer teslimler arşivde korundu.
Önceki [bağlam bakımı](archive/2026-10-17-context-maintenance.md) ve tarihsel kopyalar korundu.
ICM-01b için kullanıcı reboot sonrası OECS gerçek Mono8 görüntüsü, exact model/seri
ve viewer release sonrası discovery/open kanıtı gerekir; reboot/driver değişimi yapılmadı.
ICM-01b açık. ICM-REVIEW-01 bulguları [düzeltme teslimiyle](archive/2026-10-17-icm-review-01-fix.md)
kapandı; sıradaki ICM-04b (gerçek readback cihaz ister). Claude Code ile sıralı görev ve Codex incelemesiyle devam.

Kapanışta özeti güncelleyin ve ayrıntıyı arşive bağlayın; AGENTS boyut sınırlarını koruyun.
