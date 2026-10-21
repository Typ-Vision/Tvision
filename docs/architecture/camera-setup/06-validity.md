# CCUX-06 — Model geçerliliği ve yeniden bağlantı politikası

Durum: planlandı. Önkoşul: CCUX-01, CCUX-02. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C3–C4; [scoped bağlantı](../scoped-calibration-integration.md) ICM-06b; [taşıma](../measurement-setup-migration.md).
Kaynak: `src/application/ScopedActiveCalibration.hpp/.cpp`, `src/application/SystemCalibration.hpp/.cpp`, `src/application/MeasurementSourceIdentity.hpp/.cpp`, `src/application/MeasurementEnvironment.hpp/.cpp`.

## Uygula

1. Kayıtlı etkin model tercihi, matematik uyumu ve oturum kullanım iznini ayrı temsil et. Mevcut valid/active tek bayrağını gevşetmekle yetinme.
2. Exposure/gain değişiminde model matematiğini koru; yeni gerçek readback ve exact uyum kontrolüyle edinim snapshot'ını yeniden hazırla. Eski scope2 kanıtını yeniden yazma; gerekirse yeni sürümlü zarf üret.
3. Restart/replug sonrasında hatırlanan modeli göster; native destek/kimlik/geometri kontrolü başarılıysa kullanıma hazırlayabil. Deneysel opt-in yeniden gerekir; üretim otomatik başlamaz.
4. Optik/düzlem/kritik geometri değişiminde model kullanımını kes; A→B→A izin canlandırmasın. Bilinmeyen fiziksel değişim için uygulama bildirim komutu ekle.
5. İlgili kaynak/cache/Runtime invalidation'ını koru; başka kamera etkilenmesin. UI için açık durum DTO'su sun ve scoped sözleşmenin değişen bölümünü güncelle.

## Kabul

Exposure/gain matematik contentId korunması + yeni edinim bağının kanıtı; rename/no-op; crop/offset/reverse/binning; başka kamera; SDK/backend değişimi; restart/opt-in; legacy no-op; aktif Runtime güncelleme engeli. Tetik güveni bu uyum kontrolünden türemez.

**CTest:** scoped_active_calibration, industrial_scoped_calibration, scoped_calibration_editor, scoped_calibration_runtime, system_calibration_contract

## Sınır ve kapanış

İç kalibrasyon/robot/lens solver yok. Bu kart politika ve servis sınırıdır, UI eklenmez.
Kanıt: `build/ccux/ccux-06/`. AGENTS ve ortak sözleşme C6 ile kapat.

