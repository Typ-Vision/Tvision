# CCUX-13 — Runtime, Logs ve CSV'de metrik sonuç akışı

Durum: planlandı. Önkoşul: CCUX-11, CCUX-12. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C4–C5; güncellenmiş [deneysel Runtime](../experimental-runtime.md).
Kaynak: `src/app/MainWindowRuntime.cpp`, `src/application/ExperimentalRuntime.cpp`; Logs/CSV çağrılarını `rg -n 'experimental|csv' src/app` ile bul.

## Uygula

1. Load/Run hazırlığında kaynak kamera ID/adı, kalibrasyon durumu ve gerçek birimleri göster; kaynak/profile iç ayrıntıları normal görünümde azalt.
2. Deneysel Runtime seçim etiketini gerçekten desteklenen raw/metrik kapsamla güncelle; oturumluk onay ve blocked output bilgisi kalsın.
3. Kalıcı commit sonrası gerçek metrik sonuç ve ilgili kaynağı göster. Preview/Load/Run tek başına ölçüm veya PASS oluşturmasın.
4. Logs/ayrı deneysel CSV yeni codec model/birim/deneysel kanıtını taşısın; eski kayıtlar okunabilir. Hata/Stop için sayısal sonuç uydurma.
5. Model/ayar değişimi → Stop/yeniden Load davranışını açık kıl; restart geçmişi gösterir, Run iznini geri getirmez.

## Kabul

Raw ve mm çevrimi, birden çok kaynak, eski/yeni geçmiş, Stop ve kayıt retry, restart, hata satırları; CSV birimleri ve model kimliği; üretim Logs ile karışmama. Normal/küçük ekranlarda sonuçların kaynağı anlaşılır.

**CTest:** ui_multi_source_runtime, multi_source_terminal_trace, runtime_persistence, metrology_runtime_contract

## Sınır ve kapanış

Release + CTest + görsel. Gerçek cihaz 500 seri testini burada yeniden koşmak şart değil; yeni davranışın fiziksel kabulü CCUX-15'tedir.
Kanıt: `build/ccux/ccux-13/`. AGENTS ve ortak sözleşme C6 ile kapat.

