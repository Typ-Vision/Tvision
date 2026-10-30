# COMM4-15 — Transactional legacy migration

Durum: tamamlandı; Profil F yazılım kabulü geçti. Model: **GPT-6.1 Sol / medium**.
Uygulama ve rollback sözleşmesi [migration](../migration.md), kanıt/sınırlar
[kapanış kaydı](../../../archive/2026-10-30-comm4-15.md) içindedir.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **03,14** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S5**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Read-only analiz, backup+manifest, unsupported raporu, explicit apply staging ve atomik activation pointerı kur. Eski pending ACK drain veya belgeli rollback preflight zorunlu.

Gerçek project/profile serializers ve legacy ledger01 haritasından; yeni aday CommunicationMigrationPlanner/Transaction. Eski v2/v3 kaynak belgeleri/arşivi silme.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Protocol auto-detect, silent rewrite veya v4 ekranında gizli legacy runtime.

## Somut davranış kabulü

1. No-op eski byte/hashı korur; >64byte text/fanout/çokPose unsupported raporuyla bloklanır.
2. Staging/swap her crash noktasında eski veya yeni tutarlı config; ledger/dedup aynı kalır.
3. Pending varsa migration blok; rollback binary+config+ledger ve single-writer şartını doğrular, v4 pending varken kör restore reddedilir.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Migration manifest/backup fixture, rollback rehberi ve fault injection kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-15/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-15: …`** commit; push yok.
Sonraki: **COMM4-16**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
