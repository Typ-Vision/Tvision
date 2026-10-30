# COMM4-12 — Immutable publication değerlendirmesi

Durum: tamamlandı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **09,11** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S3,S4; wire W5**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Variable ve direct typed outputu aynı immutable cycle görünümünde çöz, exact provenance/DAG ile değerlendirilmiş sealed-ready slot verisi üret. Terminal commit öncesi socket etkisi yoktur.

Gerçek PublicationService capture/dependency/evaluate yolları01 haritasından; yeni aday fixed-slot evaluator. Yeni network motoru kurulmaz.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

UI assignment editor veya ACK store implementasyonu.

## Somut davranış kabulü

1. İki kaynak biçimi aynı fixturede aynı slot değerlerini verir; stale/missing output önceki cycleı kullanmaz.
2. Signed32 min/max, rational scale ties-even, overflow invalid; false0empty validity1.
3. Required invalid resultValid0, execution/decision ayrı; Editor preview ledger/socket çağrısı0.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Fixed slot evaluator ve conversion/provenance davranış testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-12/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-12: …`** commit; push yok.
Sonraki: **COMM4-13**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.

Uygulama sınırı: [evaluator sözleşmesi](../publication-evaluation.md).
Kapanış/kanıt: [30 Ekim 2026](../../../archive/2026-10-30-comm4-12.md).
