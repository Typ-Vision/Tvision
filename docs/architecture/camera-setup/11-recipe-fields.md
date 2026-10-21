# CCUX-11 — Yeni/düzenle reçeteden referans alanlarını kaldırma

Durum: planlandı. Önkoşul: CCUX-10. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C4; [UI gereksinimleri](../../requirements/02_machine_vision_ui_gereksinimleri.md) “New Recipe diyaloğu”.
Kaynak: `src/app/MainWindowWorkspace.cpp` recipeDialog; `src/application/ProjectService.hpp/.cpp` createRecipe/editRecipe; `src/application/ProductionRecipeService.cpp` calibrationReference kontrolü; `tests/project_service_test.cpp`.

## Uygula

1. Görüntü kaynağı referansı ve Kalibrasyon referansı alanlarını yeni/düzenle penceresinden kaldır; ad/açıklama/applicationType kalsın.
2. Yeni reçete genel referansları projeden miras almasın, boş kaynakla DRAFT oluşturulabilsin; Editor kaynak seçimi tek normal bağlama yolu olsun.
3. Metadata-only edit API'si veya eşdeğer güvenli yol ile legacy alanları/bağları koru. İki widget silinip editRecipe'ye boş string vererek eski bağları temizleme.
4. Legacy runtime branch kontrolünü koru; yeni scoped kaynakları eski global referansa zorunlu kılma. Proje referans alanları yeni reçetenin gizli önkoşulu olmasın.
5. Kopyala/import/export ve üç applicationType başlangıçlarını doğrula. Yeni projelerde ilgili legacy proje metinleri gerekiyorsa Ayrıntılar'a taşınır; diskten toplu alan silme yok.

## Kabul

0 kaynakla oluştur; Editor'da kamera ekle; ad-only edit/no-op eski bağ/hash; kalibrasyonsuz defect px; iki kamerada mm; legacy recipe roundtrip; boş isim/duplicate isim; Copy/import bozulmaması. Pencerede iki alan gerçekten yok.

**CTest:** project_persistence, production_recipe_contract, industrial_source_tool_contract, ui_shell_smoke, ui_multi_source_editor

## Sınır ve kapanış

Release + CTest + normal/küçük görsel. Reçete türü değiştirme veya yeni robot flow hesabı kapsam dışı.
Kanıt: `build/ccux/ccux-11/`. AGENTS ve ortak sözleşme C6 ile kapat.

