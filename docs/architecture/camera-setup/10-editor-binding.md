# CCUX-10 — Editor'da kamera seçimiyle kaynak bağlama

Durum: planlandı. Önkoşul: CCUX-06, CCUX-08. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C4–C5; [scoped bağlantı](../scoped-calibration-integration.md); [çok kaynaklı Editor](../multi-source-execution-editor.md) ilgili source binding bölümü.
Kaynak: `src/app/MainWindowEditor.cpp`, `src/application/ProjectService.hpp/.cpp`, `src/application/EditorSession.hpp/.cpp`, `src/application/SystemCalibration.cpp`, `src/domain/IndustrialSourceReference.hpp/.cpp`.

## Uygula

1. Kaynak properties'de kısa ID/ad seçici sun; seçim application komutuyla default setup/profile ve exact kaynak bağını birlikte güncellesin.
2. Normal akıştan ayrı profil bağla/güncelle adımını kaldır. Ayar/model değişiminde durumu açık göster ve uygun yeni snapshot'ı Run Flow hazırlığında çöz; eski sonucu koruyarak kullanma.
3. Kaynak başına model otomatik çözümü; raw/px kalibrasyonsuz, mm yalnız uygun modelle. World ROI/reference etkisini mevcut kuralla yeniden doğrula.
4. Çok kaynakta A/B ayrımı, kaynak kopyalama/silme ve başka kameraya geçişi koru. Import/legacy kaynakları kamera seçilmeden sessiz yeniden eşleme.
5. Deneysel opt-in/etiket/cache imzası canlı ve replay'de korunur; uygun deneysel modelle Editor mm akışını bağla.

## Kabul

İki kamera farklı ölçek; başka kameranın değişimi; TargetOnly no acquisition; yeni frame/model değişiminde stale cache reddi; dosya/raw regresyonu; eksik modelde mm/PASS yok; Save/reopen/no-op ve yanlış seri/profile.

**CTest:** industrial_source_tool_contract, scoped_calibration_editor, system_calibration_contract, multi_source_target_only, ui_multi_source_editor, industrial_oecs_experimental

## Sınır ve kapanış

Release + CTest + görsel. Yeni reçete penceresi sonraki kart; Runtime snapshot politikası değiştirilmez.
Kanıt: `build/ccux/ccux-10/`. AGENTS ve ortak sözleşme C6 ile kapat.

