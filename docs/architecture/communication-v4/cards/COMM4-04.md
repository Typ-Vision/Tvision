# COMM4-04 — Station domain ve immutable reçete aktivasyonu

Durum: tamamlandı — [kapanış ve kanıt](../../../archive/2026-10-29-comm4-04.md).
Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **03** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S1–S3**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Station başına active recipe revision, activation UUID, kaynak binding ve mode modeli oluştur. A/B değişimi idle/drained iken compare-and-swap; Editor selection aktif snapshotı değiştirmez.

Uygulanan sahiplik: `src/domain/StationDefinition.*`,
`src/application/StationActivationService.*`, `StationRuntimeBoundary.*`,
`src/persistence/StationActivationStore.*`, `tests/station_activation_test.cpp` ve
CMake hedefi. Mevcut `RuntimeCoordinator.hpp` yalnız eşleşen service/coordinator
çiftini doğrulayan friend sınırı taşır. [Port/store sözleşmesi](../station-activation.md).

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Çok worker dispatch, socket ve UI implementasyonu.

## Somut davranış kabulü

1. Üç station farklı recipe/camera tutar; dördüncü station eklemek hardcoded üçlü değişikliği istemez.
2. A→B→A activation kimliği yenilenir; active cycle varken değişim reddedilir.
3. Başka station snapshotı değişmez; pending eski terminal exact metadata ile korunur.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Station application portları ve immutable state transition testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-04/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-04: …`** commit; push yok.
Sonraki: **COMM4-05**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
