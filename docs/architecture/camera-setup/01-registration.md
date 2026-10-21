# CCUX-01 — Kamera kimliği ve otomatik varsayılan kayıt

Durum: planlandı. Önkoşul: Yok. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C1; [baseline](../industrial-camera-baseline.md) “Şema ve sürüm tablosu”; [taşıma](../measurement-setup-migration.md) “Taşıma”.
Kaynak: `src/domain/MeasurementSetupContract.hpp/.cpp`, `src/application/MeasurementEnvironment.hpp/.cpp`, `src/application/MeasurementWorkspace.hpp`, `src/application/MeasurementProjectTransfer.cpp`.

## Uygula

1. Görünen kısa ID ve kamera başına varsayılan setup/profile ilişkisinin minimal sürümlü temsilini belirle; mevcut UUID ve eski JSON/hash değişmesin. Yeni şema numarasını kaynaktan doğrula.
2. Tek application kayıt komutu ile kamera + varsayılanları oluştur. Eksik geometri/readback için açık eksik durum tanımla; sahte tamamlanmış geometri üretme.
3. ID benzersizliği, aynı fiziksel cihazı tekrar ekleme, kısa ID/ad güncelleme ve arşiv kontrolünü ortaklaştır.
4. Eski projede tek uygun kayıt veya mevcut açık seçimi çöz; çoklu belirsizliği raporla. Açılışta migration/ilk profile geçiş yapma.
5. Export/import/Save As yeni ilişkileri korusun; yeni projeye taşınan kayıt üretim izni almasın.

## Kabul

Yeni 1/5 kamera ekleme, duplicate ID/cihaz, yeniden adlandırma, stale revision, kayıt hatasında yarım ilişki olmaması; eski kayıt no-op/hash; tek/çok profilli legacy çözüm; taşıma roundtrip. Başka kameranın revision'ı değişmez.

**CTest:** measurement_setup_contract, measurement_environment_persistence, measurement_setup_migration

## Sınır ve kapanış

UI ve native ayar uygulama yok. Sonraki kartın kullanacağı kayıt/varsayılan çözüm API'sini ve şema kararını kapanışta belirt.
Kanıt: `build/ccux/ccux-01/`. AGENTS ve ortak sözleşme C6 ile kapat.

