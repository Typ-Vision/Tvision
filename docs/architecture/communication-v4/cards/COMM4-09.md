# COMM4-09 — Bağımsız istasyon yürütmesi

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **08a** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S2,S3**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Tek shared Runtime davranışını station-owned execution context ve bounded worker scheduler üzerinden paylaş. Her cycle recipe, variable context, frame ve terminal kökeni immutable kalsın.

Gerçek RuntimeService/Coordinator ve execution scheduler kaynakları01 haritasından; yeni aday StationExecutionRegistry. Kopya applicationType veya ikinci algoritma motoru yok.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Kamera paylaşım arbiterı10, publication template evaluator12 ve UI.

## Somut davranış kabulü

1. Üç ayrı fixture kamera/worker bariyeriyle S1/S2/S3 yürütmesi zaman aralığında overlap eder.
2. Aynı station ikinci cycle BUSY; diğer stationın değişkeni/frame/kararı sızmaz.
3. S1 error/cancel S2/S3ü durdurmaz; Stop doğru contexti drain eder ve geç callback fence edilir.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Multi-station executor, concurrency timeline ve isolation kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-09/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-09: …`** commit; push yok.
Sonraki: **COMM4-10**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
