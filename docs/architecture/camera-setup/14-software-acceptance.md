# CCUX-14 — Birleşik yazılım ve kullanım kabulü

Durum: planlandı. Önkoşul: CCUX-09, CCUX-13. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md); CCUX-01–13 kapanışlarının yalnız test/arayüz özeti.
Kaynak: ilgili mevcut UI/core testleri, `CMakeLists.txt`, [mevcut çok kaynak kabulü](../multi-source-release-acceptance.md).

## Uygula

1. Tek e2e fixture senaryosu: projeyi kaydet → 1/5 kamera ekle → tek/canlı test → seçili kamera iki kalibrasyon yöntemi → yeni reçete → Editor kamera seçimi → Run Flow → Runtime → Logs/CSV.
2. Bağımsız eksenler: 0/1/5/10 kamera UI ölçeklenmesi; 3 applicationType; native test provider/deneysel canlı fixture/replay; eski projeyi no-op açma ve açık geçiş.
3. Restart, Save As/import, duplicate kamera/ID, unplug, Stop/late frame, settings/model invalidation ve yanlış kaynak negatiflerini birleşik senaryoda kapsa.
4. UI Release ve core-off Release'te ilgili mevcut testlerin birleşimini bir kez koş; yalnız yeni failure/değişiklik varsa tekrar. Şema/ortak katman değişimi gerekçelendiriyorsa tam suite, sonucu doğru etiketle.
5. 1400×900 ve 850×600 görselleri incele. README/kamera rehberini yalnız çalışan akışla güncelle; hazırlanmış robot özelliği iddia etme. Kullanıcı için kısa devreye alma adımlarını hazırla.

## Kabul

Normal akışta profil/kurulum veya reçete genel referansı sorulmaz. Sıfır reçetede kalibrasyon, çok kaynakta doğru model, raw/px kalibrasyonsuz, mm uygun modelle; gerçek sonuçsuz PASS yok. UI donması/sonsuz istek/tampon yok. Fiziksel kabul ayrı açık.

**CTest:** ui_camera_manager, ui_calibration_overview, ui_independent_calibration_wizard, ui_multi_source_editor, ui_multi_source_runtime, project_persistence, measurement_setup_migration, multi_source_production_contract, industrial_oecs_experimental

## Sınır ve kapanış

Test listesine değişen kartların eklediği hedefleri de kat; core-off'ta UI hedefi bekleme. Donanım yokluğu yazılım kabulünü engellemez. Tek kapanış matrisi ve kanıt dizini; aynı teslim metnini planlara kopyalama.
Kanıt: `build/ccux/ccux-14/`. AGENTS ve ortak sözleşme C6 ile kapat.

