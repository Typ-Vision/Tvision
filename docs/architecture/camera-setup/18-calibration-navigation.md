# CCUX-18 — Kalibrasyon eski ekran gezinmesini kaldırma

Durum: planlandı, uygulanmadı. Önkoşul: CCUX-17.
[Ortak sözleşme](contract.md) C0/C3/C4.

## Oku

`MainWindowCalibration.cpp::makeCalibration/refreshCalibration`, `MainWindow.hpp`,
`CalibrationOverview`, `CalibrationHome`; kaynaklarda `calibrationLegacyNavigation`,
`calibrationPreviousView`, `calibrationLegacyView`, `calibrationOverviewView`,
`calibrationHome_`, `systemCalibrationSnapshot` ve `calibrationAction` çağıranları ara.
Testlerde `ui_system_calibration` ve shell smoke bağlantılarını incele.

## Uygula

1. “Ayrıntılar • önceki akış ve eski kanıt ekranları” grubunu, gezinme düğmelerini
   ve eski sayfalara ürün içi erişimi kaldır. Başka menü/ayrıntı/özellik bayrağı
   altında tekrar sunma. Kalibrasyon tek kamera merkezli ana sayfa ve iki yöntemdir.
2. Eski sayfalara ait stack, widget üretimi ve signal/refresh bağımlılıklarını
   kaldır. Ortak hesap, kayıt okuma ve resolver işlevlerini UI yaşam süresinden
   ayır; erişilemeyen widget'ları iş mantığını ayakta tutmak için gizli tutma.
3. Eski `.vfcalstudy`, model, proje ve reçete kayıtlarını silme/dönüştürme.
   Çalışan mevcut reçetenin model çözümü ve no-op/hash uyumluluğu korunsun.
   Görünür metinlerde artık kaldırılan sayfaya yönlendirme kalmasın; işlem
   gerekiyorsa mevcut kamera/kalibrasyon akışını veya somut uyumsuzluğu belirt.
4. Eski UI'nin kullandığı testleri davranışına göre güncel ana sayfa/sihirbaz
   veya servis testine taşı. Kayıt/codec/hesap regresyonunu gezinme düğmesi
   kaldırıldı diye silme. Robot/hand-eye ekranı ekleme.

## Kabul

Release UI build ve `ui_calibration_overview`, `ui_calibration_wizard`,
`ui_independent_calibration_wizard`, `ui_system_calibration`, `ui_shell_smoke`,
`active_calibration_contract`, `system_calibration_contract`,
`calibration_persistence`, `scoped_calibration_editor` ilgili testleri.

Projesiz/açık proje, kamerasız/tek kamera/çok kamera, sayfa geçişi, proje kapatma,
Save/reopen ve eski bağlı reçete kullanımını doğrula. Kaldırılan widget erişimi
null dereference veya gizli legacy sayfa ihtiyacı doğurmamalı.
Kalibrasyon 1400×900 ve 850×600 görsellerinde yalnız güncel akış bulunmalı.

Kanıt `build/ccux/ccux-18/`; görev ID'li tek commit, push yok. Sonraki CCUX-19.
