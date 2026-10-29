# COMM4-05 — Listener ve üretim yaşam döngüsünü ayır

Durum: tamamlandı — [kapanış](../../../archive/2026-10-29-comm4-05.md).
Model: **GPT-6.1 Sol / medium**. [Uygulanan lifecycle sınırı](../server-lifecycle.md).
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **04** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S1; verification V2 başlangıç**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Enabled geçerli v4 profile process açılışında bir listener oluşturur; station Run/Auto listen ömrüne bağlı değildir. Stop station admissionı kapatıp drain ederken STATUS/ACK erişimi sürer.

Gerçek server composition/profile portları01 haritasından; yeni aday CommunicationServerLifecycle ve StationRunController. UI sayfaları henüz değiştirilmez.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Binary parser, legacy migration veya mevcut configi otomatik yükseltme.

## Somut davranış kabulü

1. Başlangıç listen+STOPPED; Auto false tetik admission kapalı; restart Run/Auto restore etmez.
2. Run/Stop/A-B aktive listener generation/port değişmez; bind conflict açık hata.
3. Process shutdown bounded checkpoint/drain, unresolved kayıt korunur; iki listener yaratılmaz.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Tek server composition ve lifecycle state testleri; eski profile etkisi raporu.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-05/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-05: …`** commit; push yok.
Sonraki: **COMM4-06**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
