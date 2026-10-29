# COMM4-07 — HELLO session ve peer sahipliği

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **03,06** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **wire W1,W2,W6,W7; contract S4**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Explicit v4 HELLO contract revision ve kalıcı peer/epoch eşleşmesi kurar; connection session yenidir. Tek controlling peer, read-only catalogue/status ve stale session fence uygulanır.

Gerçek endpoint composition01 haritasından; yeni aday CommunicationPeerSessionService ve handshake tests. Ledger API bağları dar tutulur.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

TLS/authentication iddiası, remote Run/recipe change veya ikinci üretim peer desteği.

## Somut davranış kabulü

1. İkinci peer PEER_BUSY; reconnect session değişir ama namespace/request key korunur.
2. Wrong contract/stale session/retired epoch mutating command dispatch etmez.
3. HELLO contract0 discovery mümkün, exact contract tamamlanmadan trigger kapalı; opaque kimlik>2^53 kaybolmaz.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Handshake state portu ve localhost connection testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-07/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-07: …`** commit; push yok.
Sonraki: **COMM4-08**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
