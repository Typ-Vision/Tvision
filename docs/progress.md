# VisionFlow — güncel devir

Güncelleme: 17 Ekim 2026. Bu dosya güncel durum özetidir; tarihsel günlük değildir.
Kullanıcının son yönlendirmesi önceliklidir. Ayrıntılar [arşiv dizininde](archive/README.md);
başlangıçta arşivin tamamını okumayın.

## Devam noktası

- **Son yazılım teslimi: ICM-04c stream/buffer → owned Mono8 frame** (Codex).
  Bounded tanı probe'u, allocation öncesi pool bütçesi, actual raster doğrulama,
  SDK buffer iadesinden önce owned copy ve hata yollarında cleanup uygulandı.
  [Kapanış ve kanıt](archive/2026-10-17-icm-04c.md).
  Native OECS ABI stride/completion flag vermiyor: yalnız exact tight Mono8 kabul;
  padded satırlar explicit stride fixture'ında sınandı. Gerçek stream/grab açık.
  **Sonraki görev ICM-04d software trigger ve çevrim-kare eşleştirme**.
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

- ICM-01a ve ICM-02 offline karar tamamlandı. Fiziksel ICM-01b ve G0–G4/H01–H08 açık.
  Yeni C API enum yine **0 cihaz** gördü; open/feature/grab denenmedi. GenTL remote
  XML/nodemap consumer doğrulanamadı. Fixture başarısı fiziksel kabul değildir.
- Kullanıcı: reboot **yapılmadı**, kamera bağlı, OECS'de görüntü alındı ve program
  kapatıldı; release kesin değil. Exact model/seri/Mono8 raster araçla doğrulanmadı.
  OECS isimli süreç görülmedi; CIM boot/PNP sorgusu erişim hatası verdi.
  Device ID=1 seri değildir. Lens Basler 25 mm beyanı; tam model/kurulum eksik.
- C API x64 header compile/load/version/enum ve fixture stream ABI çalıştı; gerçek
  feature/unit/readback, stream/frame ABI/pool/cleanup cihazda açık. Vendor
  redistributable izni ve temiz makine dağıtımı doğrulanmadı; runtime yokluğunda
  açık unavailable gerekir. Pool bütçesi advertised payload'dır; SDK/RSS hard sınırı değil.
- Fiziksel seri sürekliliği/ölçüm/PLC/robot/lens, native DPI/çoklu monitör,
  KUI-6/T8.4.2b.3c, CUDA ve ürün kurulum/dağıtım kabulü açık kalır.

## Son doğrulama ve çalışma ortamı

- **ICM-04c yeni koşum**: core/UI Release build exit 0, her birinde ilgili **7/7**
  CTest; SDK'lı Release build exit 0 ve ilgili **9/9**. Tam suite koşulmadı; UI
  değişmedi. Sentetik padded Mono8, invalid payload/format/chunk/overflow, pool
  bütçesi, copy exception ve stream/close/destroy/unload failure yolları sınandı.
  Yerel DLL 0x3050200, **0 cihaz**. Kanıt `build/icm/04c/`; üç build'de
  `icm-04c-final-tests.xml`. İlk SDK fixture imza compile hatası düzeltildi;
  ilk ve son loglar kapanışta ayrıdır.
- Tarihsel: 04b core/UI **6/6**, SDK **5/5**; review-fix core/UI **6/6**, SDK **3/3**.
  04a tam suite core **75/75**, UI **89/89**. 03b core **74/74**, UI **87/88**
  (`plc_socket_runtime` aralıklı; kök neden açık). Yeni koşum olarak sunmayın;
  kanıt/kapsam ilgili kapanışlarda korunur.
- Önceki tam yazılım kabulü KMC-10e: UI **87/87**, core **73/73**;
  [B kabulü](architecture/multi-source-release-acceptance.md).
- Güncel UI `build/reference-ui/visionflow.exe`, core `build/kmc9-core-off`.
  Başlatma `.\scripts\run-local.ps1`; build/PATH komutları README'dedir.
- ICM-02'ye kadarki işler `53bb972` commit'indedir; sonrası her görev ID'li tek
  commit'tir. Push yalnız kullanıcı isteğiyle. Ajanlar sıralı çalışır; Codex yönetici,
  Claude Code alt görev uygulayıcısıdır. `config/local.ui.ini` değiştirilmedi;
  test/raporlar `build/` altında kalır.

## Kapanış ve sonraki adım

Sıradaki ICM-04d. ICM-01b için reboot sonrası OECS gerçek Mono8 görüntüsü,
exact model/seri ve viewer release sonrası discovery/open kanıtı gerekir;
reboot/driver değişimi yapılmadı. G1 için gerçek grab kanıtı ve 04e gereklidir.
Önceki [bağlam bakımı](archive/2026-10-17-context-maintenance.md) ve tarihsel
kopyalar korundu. Kapanışta özeti güncelleyin, ayrıntıyı yeni arşiv kaydına bağlayın.
