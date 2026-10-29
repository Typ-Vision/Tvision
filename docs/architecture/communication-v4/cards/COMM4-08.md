# COMM4-08 — Station admission ve trigger yönlendirme

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **04,07** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S2,S4; wire W3**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Request station/recipe/template/activation ve input/camera revisionını exact doğrula; mode/resource/recording/pending bariyerlerini tek atomik admissionla birleştir. Accepted öncesi durable intent portunu zorunlu tut.

Gerçek TriggerRouter/Coordinator portları01 haritasından; yeni aday StationAdmissionService. Runtime yürütme topolojisini bu kartta yeniden yazma.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Eşzamanlı worker implementasyonu ve gerçek hardware lease driverı.

## Somut davranış kabulü

1. S1 stale activation reject iken S2 geçerli kabul; aynı istasyon iki tetikten yalnız biri kabul.
2. Busy transient ret aynı key ile daha sonra denenebilir; accepted key farklı içerikte CONFLICT.
3. NotAuto/NotReady/PendingStorage nedenleri farklı; kayıt hatasında acquisition çağrısı0.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Station-aware router ve yarış deterministik testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-08/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-08: …`** commit; push yok.
Sonraki: **COMM4-08a**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
