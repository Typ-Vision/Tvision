# COMM4-03 — Kalıcı numeric kimlik kataloğu

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **02** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S2,S4; wire W1,W6**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Station/camera UUID ile recipe/template/model immutable revision kimliklerini never-reused uint32 wire keylere transactionally bağla. Catalogue snapshot revision ve retired key/epoch durumlarını kalıcılaştır.

Yeni aday: StationIdentityRegistry domain/application/store dosyaları ve tests; gerçek store/schema/CMake yolları01 haritasından seçilir. Mevcut projeyi otomatik rewrite etme.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Listener, UI, reçete yürütme ve yeni global variable sistemi.

## Somut davranış kabulü

1. İki UUID aynı key alamaz; silinmiş key yeniden atanmaz; rename key korur, revision yeni key alır.
2. Restart/torn write eski veya yeni tutarlı registry üretir, çakışmayı sessiz onarmaz.
3. UINT32 exhaustion açık blok; UUID opaque16 export/import ve catalogue revision mismatch korunur.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Registry port/store, schema fixture ve collision/restart davranış kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-03/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-03: …`** commit; push yok.
Sonraki: **COMM4-04**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
