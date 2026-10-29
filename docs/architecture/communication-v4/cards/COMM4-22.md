# COMM4-22 — İki geometriden aday hand-eye solve

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **18,21** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S6–S8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Eye-in-hand flange_T_camera ve eye-to-hand base_T_camera adaylarını S7 eşitliklerinden türet. Sample set immutable, dependency digest ve kalite/degeneracy metriği saklanır; solve active modeli değiştirmez.

Gerçek calibration solver portu18 araştırmasından; yeni aday HandEyeCandidateSolver. Yeni OpenCV bağımlılığı gerekirse CMake dar kapsam. Uygunluk blokluysa yalnız test fixture yolu.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Validation23, aktivasyon, fiziksel verisiz üretim uygunluğu veya yanlış yönü otomatik düzeltme.

## Somut davranış kabulü

1. İki modda bilinen nonidentity ground truth ve bağımsız reconstructed eşitlik residual testi.
2. Tek eksen/aynıpose/az çeşitlilik ve wrong convention veri setleri açık reject.
3. Ters transform/TCP offset negative fixture; solver failure önceki activei korur, candidate yalnız provenance ile kaydolur.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Candidate solve adapterı, mod başına golden dataset ve algoritma karar kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-22/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-22: …`** commit; push yok.
Sonraki: **COMM4-23**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
