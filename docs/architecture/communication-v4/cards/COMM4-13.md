# COMM4-13 — Kalıcı terminal ve exact ACK ledger

Durum: tamamlandı. [Kapanış](../../../archive/2026-10-30-comm4-13.md). Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **07,12** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S4; wire W7**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Request intent→begin→terminal+sealed payload→pending→ACK committed atomik recovery yolu kur. Exact station/epoch/request/result/digest eşleşmesini kalıcı ve idempotent uygula.

Gerçek Terminal/DeliveryStore/journal kaynakları01 haritasından; yeni aday station-keyed ledger adapterı. Mevcut dedup/arşiv silinmez.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Retry scheduling14 veya retention temizliği.

## Somut davranış kabulü

1. Her write sınırında crash: ya saklı terminal replay ya RECOVERY_REQUIRED, yeni capture0.
2. Wrong ACK hiçbir bariyeri kaldırmaz; duplicate exact ACK restart sonrası da idempotent.
4. Üretim ve STOP/ABORT control terminal rezervleri bağımsız exact ACK alır; SELECT_RECIPE yeni activation cevabı eski request fingerprint ile recovery-safe eşleşir.
3. Diskfull/torn manifest accepted iş kaybetmez, sahte DONE yok; S1 ACK S2 pendingi değiştirmez.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Ledger migration-safe records, fault injection ve exact digest/ACK fixture kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-13/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-13: …`** commit; push yok.
Sonraki: **COMM4-14**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
