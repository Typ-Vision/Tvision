# CCUX-24C — kontrol durumları ve yeni Circle varsayılanı

Durum: uygulandı; [kapanış](../../archive/2026-10-26-ccux-24c.md). [Ortak plan](24-usability-plan.md), contract
C3–C5 ve [analiz](24-usability-analysis.md) 2/5 okunur. Sonraki: 24D.

## Hedef ve dosya haritası

Modelin bağımsız kontrol sonucu tutarlı sunulur; Editor/Runtime'ın sabit fiziksel
kabul cümleleri kalkar. Yeni Circle calibrated ve mevcut mm başlangıçlarıyla
oluşur. Eski pixel araçlar ve kullanıcı seçimi değişmez.

Başlangıç: `src/application/ActiveCalibrationService.cpp`,
`ExperimentalRuntime.cpp`, `src/app/CalibrationWizard.cpp`,
`CalibrationOverview.cpp`, `MainWindowExecution.cpp`, `MainWindowEditor.cpp`;
varsayılan tüketicileri için `src/domain/CircleConfig.cpp` ve
`src/application/ProjectService.cpp` doğrulanır.

## Uygulama adımları

1. Durum tablosunu hazırla: kontrol yok, ölçüm var/sınır yok, sınır içinde,
   sınır dışında. Eksik legacy alanı kontrol geçti sayma; fit/RMS ayrı bilgidir.
2. Aynı kayıt girdisinden aynı metni üreten ortak sunum formatter'ı kullan.
   `controlAccepted` ve varsa ölçüm/sınır kanıtını birlikte yorumla; yalnız
   boolean ile dört durum uydurma. Eksik veri “bilinmiyor/yapılmadı” olarak kalır.
3. Kalibrasyon ekranında gerçek kontrol durumu kalsın. Editor/Runtime'daki
   rutin sabit fiziksel kabul uyarıları kalksın; uyumsuz model, edinim veya
   ölçüm hataları gizlenmesin. Gösterilen durum doğru model/snapshot'tan gelsin.
4. Tarihsel Logs/CSV yeniden yazılmasın. Sunumun ortak yardımcıyı kullanması
   gerekiyorsa kayıtlı çevrimin modeli esas alınsın; güncel etkin model değil.
5. Yeni Circle oluşturma politikasında calibrated seç ve mevcut mm başlangıçlarını
   birlikte uygula: scanWidth/inlierDistance/maxResidual/unitsPerSample=0.1,
   maxRms=0.035. Alanların geçerli config karşılıkları uygulama başında doğrulansın.
6. Tercih oluşturma yolu değişikliğidir; domain default veya legacy decode
   değiştirilmez. UI dışındaki yeni araç oluşturma yolları da sınıflandırılır;
   copy/import/open yeni araç varsayılanı uygulanacak yol sayılmaz.

## Kabul

- Kontrol dört durumunda ve eksik legacy alanlarda doğru metin; fit başarısı
  kontrol başarısı gibi gösterilmez. Aynı model çelişkili “yapılmadı” üretmez.
- Aktif Runtime sırasında yeni model etkinleştirmek yüklü snapshot'ı değiştirmez;
  eski kayıt görünümü yeni modelden veri almaz. `physicalAcceptance`, admission
  ve PLC/robot yetkileri değişmez.
- Yeni Circle calibrated açılır, çizilir, Run Tool ve Save/reopen ile korunur.
  Mevcut/kopyalanan pixel araç pixel kalır; elle pixel seçimi desteklenir.
  Model yokken gerçek hata üretilir; otomatik/global model kullanılmaz.

Testler: `calibration_save_use`, `scoped_active_calibration`,
`calibrated_circle_contract`, `ui_circle_smoke`,
`experimental_metric_runtime_contract`, `ui_multi_source_editor`,
`ui_multi_source_runtime`. Dört durum ve pixel copy/no-op senaryoları eklenir.
Release ve iki boyutta kontrol alanı/Circle görselleri alınır.

Kanıt `build/ccux/ccux-24c/`; ortak kapanış uygulanır. Gereksinimde yeni oluşturma
politikası, contract'ta matematik/kontrol/fiziksel kabul ayrımı netleştirilir.
Commit: `CCUX-24C: align calibration status and new Circle defaults`.
