# CCUX-08 — Kaydet ve kullan application komutu

Durum: planlandı. Önkoşul: CCUX-06, CCUX-07. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C3,C5; [taşıma/aktivasyon](../measurement-setup-migration.md); [sistem özeti](../calibration-overview.md).
Kaynak: `src/application/ScopedActiveCalibration.hpp/.cpp`, `src/application/ActiveCalibrationService.hpp/.cpp`, `src/application/MeasurementWorkspace.hpp`, `src/application/IndependentCalibrationCapture.hpp/.cpp`.

## Uygula

1. Tek application komutu: hesaplanmış aday + exact hedef/generation → immutable kayıt → uyum/kullanılabilirlik kontrolü → etkin seçim. Dönen sonuç aday kaydedildi/etkinleşti/başarısız ayrımını içersin.
2. Native akışın mevcut accepted probe denetimini koru. Deneysel yol için ayrı etkinlik/evidence doğrula; false trigger association hiçbir aşamada true olmasın.
3. İptal/proje değişimi/disk hatası/bağlantı kaybında önceki etkin seçimi koru. Kaydedilmiş aday kalmışsa açık sonuç ver; retry mükerrer kayıt üretmesin.
4. Bağımsız kontrol durumunu ve mevcut eşikleri taşı; kontrolsüz modelin fiziksel kabulü oluşmasın. Eksik kalibrasyon yerine mm/success üretme.
5. Current-target/gate kontrolünü commit anında tekrarla; çalışan Runtime snapshot'ına model yayınlama.

## Kabul

Başarılı native fixture ve deneysel akış; aday yazıldıktan sonra aktivasyon hatası, yeniden deneme, duplicate, stale setup, Runtime busy, Stop, başka kamera ve opt-in değişimi. Eski aktif modelin korunması ve content/hash roundtrip.

**CTest:** active_calibration_contract, scoped_active_calibration, industrial_scoped_calibration, measurement_setup_migration

## Sınır ve kapanış

UI sonraki kart; fiziksel kabul ayrı kalır. İşlem disk/SDK arasında tam atomik değilse gerçeği durum modeliyle ifade et.
Kanıt: `build/ccux/ccux-08/`. AGENTS ve ortak sözleşme C6 ile kapat.

