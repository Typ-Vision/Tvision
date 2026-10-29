# COMM4-01 — Güncel kaynak ve davranış envanteri

Durum: tamamlandı (yalnız kaynak/belge envanteri, 29 Ekim 2026).
Model: **GPT-6.1 Sol / medium**. [Kaynak ve kart sahiplik haritası](../source-map.md),
[kapanış ve doğrulama](../../../archive/2026-10-29-comm4-01.md).
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **Yok**; ilk envanter kartıdır. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S0–S5; wire W0–W8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Tek loaded recipe, endpoint sahipliği, publication/terminal/ACK ve calibration servis sınırlarını güncel kaynakla haritala. Domain/application/transport/UI bağımlılıklarını ve gerçek CMake test adlarını kaydet; v2/v3 davranışlarını uygulanmış ile plan ayrımıyla işaretle.

Gerçek RuntimeCoordinator, CommunicationTcpEndpoint, PublicationService ve MainWindowCommunication/PublishDataEditor yolları [source-map](../source-map.md) ile doğrulandı. Sahiplik source-map ve içindeki kart sahiplik tablosu, bu kart/indeks ve zorunlu progress/arşiv kapanışıdır; C++ değişikliği yok.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Refactor, codec implementasyonu, native üretim izni veya bütün arşivi yeniden audit.

## Somut davranış kabulü

1. Graph list_projects/index_status çalışma kökü D:/TVision; her relied file coverage ve missed source fallback.
2. En az bir trigger→terminal→ACK zinciri doğrudan kaynakla doğrulanır; generic constructor graph edge kanıt değildir.
3. README/prototypes dirty değişiklikleri hash/diff ile korunur; gerçek dosya yolu ve aday adı ayrılır.

## Release ve teslim kabulü

Profil **D**, [verification V0/V1](../verification.md) uygulanır.
Yalnız belge/fixture tasarımı: Release ON/OFF koşulmaz; bu durum raporda açık yazılır. Link/diff/byte boyutu ve bağlam kontrolü zorunludur.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: source-map.md; gerçek dosya/test hedef haritası ve risk/bağımlılık listesi.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-01/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-01: …`** commit; push yok.
Sonraki: **COMM4-02**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
