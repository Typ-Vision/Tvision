# COMM4-02 — Byte fixture ve wire sözleşmesini dondur

Durum: tamamlandı — [freeze raporu](../freeze-comm4-02.md),
[kapanış](../../../archive/2026-10-29-comm4-02.md). Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **01** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **wire W0–W8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Her request512/response1024 ve header128 düzenini bağımsız byte hesabı ile doğrula; metadata category sabit şemalarını, enumları, quaternion normalizasyonunu ve digest girdisini tamamla. Taslak açık kararları çözüp contract revision kimliğiyle freeze raporu üret.

Gerçek: bu dizindeki wire.md ve verification.md. Yeni aday: tests/fixtures/communication-v4/ altındaki golden binary/hex ve bağımsız word tabloları; yalnız fixture/doc, üretim codec yok.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Codec/server implementasyonu, Pickit uyumluluk iddiası veya henüz ölçülmemiş fiziksel tolerance seçimi.

## Somut davranış kabulü

1. Tüm kinds için exact byte length, tüm offset aralıkları bitişik/overlap-free, reserved0 ve BE fixture.
2. INT_MIN/MAX, uint64>2^53/MAX, UUID/GUID word order,64/65byte UTF8, Pose frame/revision ve exact SHA/ACK bağımsız hesapla.
4. Remote SELECT_RECIPE/SET_AUTO/STOP, control-terminal rezervi, STATUS active snapshot ve holdout SAMPLE fixtureleri dahil. Robot pose taşıma ön kontrolü: quaternion/explicit Euler adapter yeri freeze öncesi kararlaştırılır; marka bilinmiyorsa generic kapsam açık kalır.
3. Her catalogue category için metadata schema doldurulmadan ve bütün açık wire kararları kapanmadan kart PASS olmaz.

## Release ve teslim kabulü

Profil **D**, [verification V0/V1](../verification.md) uygulanır.
Yalnız belge/fixture tasarımı: Release ON/OFF koşulmaz; bu durum raporda açık yazılır. Link/diff/byte boyutu ve bağlam kontrolü zorunludur.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Fixture manifest/digest ve freeze kararı; sonraki codec yalnız bu baseline üzerinden.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-02/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-02: …`** commit; push yok.
Sonraki: **COMM4-03**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
