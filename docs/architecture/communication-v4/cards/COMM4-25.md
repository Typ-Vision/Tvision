# COMM4-25 — PLC robot simulator ve dayanıklılık kabulü

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **14,17,24** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **verification V2; wire W8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Freeze binary protokolünü bağımsız peer simulatorla kullan;3+ station üretimi ve iki calibration modu için uçtan uca fault/concurrency senaryolarını çalıştır.

Gerçek mevcut PLC simulator/test harness01den; yeni aday v4 simulator mode ve robot handshake fixture driver. Üretim binary fixture producer içermez.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Fiziksel robot hareketi veya simulatoru gerçek accuracy kanıtı olarak raporlama.

## Somut davranış kabulü

0. PLC SELECT_RECIPE→SET_AUTO→READY→TRIGGER→ACK akışı; yerel Stop yarışı ve pending ACK sırasında remote Stop.
1. Aynı streamde3istasyon overlap, shared camera collision ve A/B geçişinde socket kesilmez.
2. Consumer crash-after-effect-beforeACK duplicate terminal verir; exactly-once iddiası yok.
3. Split/coalesced/slow peer, restart her journal sınırı, sample duplicate, ABORT timeout ve stale revision matrisi.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Simulator kullanım rehberi, machine-readable E2E sonuç ve failure/recovery logları.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-25/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-25: …`** commit; push yok.
Sonraki: **COMM4-26**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
