# COMM4-17 — Publish slot assignment editorü

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **11,12,16** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S3,S5**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Number/Flag/Text/Pose slotuna Variable veya doğrudan typed tool output seçimi ver. Sabit tip/birim/ölçek salt contract metadata; özel slot adları gösterilsin.

Gerçek PublishDataEditor.* ve typed port selector01 haritasından; yeni aday slot assignment model. Machine template ve recipe binding düzenleme yetkileri ayrılır.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Serbest script/expression veya Editor preview dış ağ yayını.

## Somut davranış kabulü

1. Variable/direct source iki yoldan aynı preview; incompatible port seçimi açık validation.
2. Required missing/overflow/UTF8 limit kullanıcıya alanıyla gösterilir; sıfırpose placeholder başarı görünmez.
3. Apply atomik, cancel no-op byte/hash; normal/küçük ekran typed selector ve klavye kontrolü.

## Release ve teslim kabulü

Profil **U**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI Release ve normal/küçük ekranda gerçek görsel kontrol zorunlu; offscreen sonucu native/DPI kabulü değildir.

Teslim: Slot editorü, schema roundtrip ve UI görsel/behavior kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-17/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-17: …`** commit; push yok.
Sonraki: **COMM4-18**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
