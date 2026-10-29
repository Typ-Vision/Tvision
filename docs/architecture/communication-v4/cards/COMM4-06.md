# COMM4-06 — Bounded binary codec ve framer

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **02,05** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **wire W0–W8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Freeze fixturelerini encode/decode eden512/1024 byte codec, partial deadline ve bounded receive/write çerçeveleyici oluştur. Bilinmeyen sürüm/flag/reserved alanı strict reddet.

Yeni aday CommunicationV4Codec/Framer ve tests; gerçek infrastructure/CMake dizini01 ile kesinleşir. Domain semantic authority codec içine taşınmaz.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Station dispatch, kalıcılık, auth ve protocol auto-detect.

## Somut davranış kabulü

1. Her golden vector byte-exact; bir byte split ve coalesced4request aynı parse sonucu verir.
2. 2000ms trickle deadline yenilenmez; invalid size allocation büyütmez, buffer/write cap korunur.
3. InvalidUTF8, padding, NaN/Inf, signedness ve endian reversal reject; parser fuzz bounded sonuç verir.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Codec/framer kaynakları, fixture roundtrip ve limit testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-06/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-06: …`** commit; push yok.
Sonraki: **COMM4-07**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
