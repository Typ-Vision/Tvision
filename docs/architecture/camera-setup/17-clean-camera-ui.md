# CCUX-17 — Kamera, kalibrasyon, Editor ve Runtime seçeneklerini sadeleştirme

Durum: yazılım teslimi tamamlandı. [22 Ekim kapanışı](../../archive/2026-10-22-ccux-17.md). Önkoşul: CCUX-16.
[Ortak sözleşme](contract.md) C0–C5; [CCUX-16](16-automatic-acquisition.md).

## Oku

`CameraManager.cpp/.hpp`, `CalibrationOverview.cpp/.hpp`,
`MainWindowEditor.cpp`, `MainWindowExecution.cpp`, `MainWindowRuntime.cpp`,
`ExperimentalOecsAcquisition::notice`, `ExperimentalRuntime::summary/sourceSummary`.
Önce aşağıdaki objectName ve metinlerle ara; yalnız ilgili fonksiyonları aç.
Test bağımlılıkları: `camera_manager_test.cpp`, `calibration_overview_test.cpp`,
`EditorUsabilitySmoke.cpp`, `MultiSourceSmoke.cpp`.

## Uygula

1. `cameraExperimental`, `overviewExperimental`, `experimentalEditorReplay` ve
   `experimentalRuntime` seçimlerini ve bağlı notice widget'larını kaldır.
   Görünmez/önceden işaretli kutu bırakma. Bütün `findChild`, signal, refresh,
   proje değişimi ve busy kontrolü erişimlerini CCUX-16 servisine taşı.
2. “Deneysel OECS kalibrasyonu”, “Deneysel OECS edinimini kullan”, Editor ve
   Runtime/replay seçenekleri artık yoktur. “Deneysel kamera”,
   “Tetik–kare eşleşmesi doğrulanmadı”, “Tetik eşleşmesi doğrulanmadı” ve aynı
   anlamdaki banner/tooltip/durum tekrarlarını görünür ürün akışından kaldır.
   Bunların yerine “test”, “beta”, “doğrulanmamış” modu veya onay penceresi ekleme.
3. Kamera → Görüntü al/Canlı görüntü; Kalibrasyon → Kalibre Et → yöntem →
   Kaydet ve kullan; Editor → Görüntü al/Run Flow; Runtime → Load/Run/Tek çevrim
   akışları ek mod seçmeden çalışır. Sürücü yolu gerektiğinde “OECS sürücü DLL”
   gibi teknik ayrıntı olarak kalabilir; bir çalışma modu değildir.
4. Görüntü kaydı, sonuç/kalibrasyon özetleri, Logs satırları ve CSV düğme/diyalog
   metinlerindeki deneysel nitelemeyi kaldır. Ortak `notice()` çağrılarını tek tek
   düzenle; boş string döndürüp `contains("")` ve boş satır yan etkisi yaratma.
   Normal kayıt ve OECS kayıt export'u aynı isimli iki belirsiz düğme oluşturmasın:
   seçili kayıt türüne uygun exporter'ı kullanan tek CSV eylemi sun. CSV kanıt
   sütunları ve schema anahtarları uyumluluk için aynı anlamı taşısın.
5. Kullanıcıya gösterilen `BLOCKED_EXPERIMENTAL` gibi ham durumları anlaşılır
   iletişim durumuna çevir. Kalıcı JSON/CSV teknik kodlarını topluca yeniden adlandırma.
   Eksik kamera/kalibrasyon, gerçek işlem hatası, fiziksel kontrol durumu ve
   iletişim kullanılabilirliği bağlamındaki gerekli durumlar doğru kalır.

## Kabul

Release UI build; ilgili CTest `ui_camera_manager`, `ui_calibration_overview`,
`ui_calibration_wizard`, `ui_independent_calibration_wizard`, `ui_editor_usability`,
`ui_multi_source_editor`, `ui_multi_source_runtime`, `ui_shell_smoke`.
Önce `ctest -N`; fixture bağımlılıklarını koru. Kontroller yeni kullanıcı akışından
geçsin; kaldırılan kutuları test yardımcısıyla geri oluşturarak çalıştırma.

1400×900 ve 850×600 Kameralar, Kalibrasyon/iki sihirbaz, Editor, Runtime ve Logs
görsellerini incele. Boş banner alanı/taşma/ölü seçim kalmasın. Ekran metni,
tooltip, hata yolu ve export diyaloğunda yasaklanan ifadeleri doğrula; backend
schema/adlarında `experimental` bulunması UI hatası sayılmaz. Genel string
silme işlemiyle gerçek hata mesajlarını boşaltma.

Kanıt `build/ccux/ccux-17/`; görev ID'li tek commit, push yok. Sonraki CCUX-18.
