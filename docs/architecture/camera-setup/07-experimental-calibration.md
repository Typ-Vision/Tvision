# CCUX-07 — Deneysel kalibrasyon edinimi ve köken kaydı

Durum: planlandı. Önkoşul: CCUX-04, CCUX-06. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C3,C5; [EXP planı](../industrial-camera-oecs-experimental-plan.md) EXP-02; [scoped bağlantı](../scoped-calibration-integration.md).
Kaynak: `src/application/IndependentCalibrationCapture.hpp/.cpp`, `src/application/MeasurementSourceIdentity.hpp/.cpp`, `src/application/CalibrationWizardSession.hpp/.cpp`, `src/application/ActiveCalibrationService.hpp/.cpp`, `src/application/ExperimentalOecsAcquisition.hpp/.cpp`.

## Uygula

1. Reçetesiz capture isteğine açık oturumluk deneysel politika ekle; kaynağı CCUX varsayılanlarından dondur. Production factory/admission'a deneysel adaptör enjekte etme.
2. Canlı OECS veya PGM+JSON'dan gerçek raster, köken, kimlik/geometri/profile ve hash'i doğrula. Politika kapalıysa reddet; normal PNG trial davranışını koru.
3. Bilinen ölçü/plaka matematiğine mevcut frame'i ver; yeni solver yazma. Deneysel aday kaydında fiziksel kabul ve eşleşme yokluğunu sürümlü codec ile koru.
4. File replay'i camera-acquired yapma. Aynı modelin canlı/replay kullanımı için açık uyum şartlarını taşı; hash/geometry/identity uyuşmazsa aday reddi.
5. Stop/generation, değiştirilmiş JSON, cache ve kayıt roundtrip negatiflerini ekle; kaynak kanıtı silinerek izin yükseltilemesin.

## Kabul

Gerçek fixture rasterden iki mevcut yöntemle aday; eksik/bozuk sidecar; farklı seri/geometri; opt-in kapalı; geç callback; immutable eski codec/hash; deneysel adayı trusted active/terminal'e taşıma reddi.

**CTest:** independent_calibration_capture, industrial_calibration_capture, calibration_wizard_contract, calibration_plate_contract, industrial_oecs_experimental

## Sınır ve kapanış

Aktivasyon CCUX-08; runtime mm CCUX-12. Geliştirme fixture'ı gerçek mm doğruluk kabulü sayılmaz. Codec değişim sözleşmesini aynı commit'te belgele.
Kanıt: `build/ccux/ccux-07/`. AGENTS ve ortak sözleşme C6 ile kapat.

