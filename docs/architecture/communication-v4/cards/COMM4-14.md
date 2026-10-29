# COMM4-14 — Reconnect retry ve socket backpressure

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **13** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S4; wire W0,W7**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Saklı terminali session-only yeni zarfla replay et; station başına pending1, bounded global queues ve adil frame scheduler kur. Retry bütçesi acquisition tekrarına dönüşmez.

Gerçek endpoint write/retry scheduler01 haritasından; yeni aday fair station delivery scheduler. Ledger otoritesi korunur.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Yeni transport, exactly-once consumer effect garantisi veya silent eviction.

## Somut davranış kabulü

1. S1 ACK timeout S2/S3 sonuçlarını durdurmaz; station başına sıra korunur.
2. Partial write frame bytes interleave olmaz; highwater64KiB/64response sınırı bounded, disconnect replay güvenli.
3. 2s/3retry sonrası pending alarm; reconnect aynı digest/body, stale session reject; retired epoch trigger reject.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Delivery scheduler, reconnect ve fairness/slow-reader testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-14/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-14: …`** commit; push yok.
Sonraki: **COMM4-15**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
