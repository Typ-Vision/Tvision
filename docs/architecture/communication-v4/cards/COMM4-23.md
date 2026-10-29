# COMM4-23 — Bağımsız validate ve atomik activation

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **22** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S8; wire W3**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Holdout sample+bağımsız referansla ölçülebilir hata raporu üret; exact validated candidate/dependency snapshotı idle/drained koşulunda compare-and-swap active yap.

Gerçek calibration validation/activation store portları18den; yeni aday HandEyeValidationReport/ActivationService. Üretim run izni ayrı kalır.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Training residualını fiziksel accuracy kabulü sayma veya lens/native engelini kaldırma.

## Somut davranış kabulü

1. Training/holdout overlap reject, max/RMS/rotation tolerance fail activate etmez.
2. Stale candidate/dependency/expectedActive revision reject; her write crash old/new atomic.
3. Sentetik validation sadece software-verified; gerçek reference/tolerans eksikse production activation BLOCKED.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Immutable validation report, activation transaction ve invalidation testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-23/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-23: …`** commit; push yok.
Sonraki: **COMM4-24**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
