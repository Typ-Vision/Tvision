# COMM4 — Tek bağlantı, bağımsız istasyonlar ve robot kalibrasyonu

**Durum: sıralı uygulama sürüyor; hedef sistem henüz tamamlanmadı.**
29 Ekim 2026. Kullanıcı seçimi: GPT-6.1 Sol / medium. Varsayılan oturum kapsamı
tek karttır; ajanlar paralel çalışmaz. **Sonraki görev: [COMM4-13](cards/COMM4-13.md).**

**COMM4-01 tamam:** [güncel kaynak, test ve kart sahiplik haritası](source-map.md).
**COMM4-02 tamam:** [wire fixture freeze](freeze-comm4-02.md).
Freeze D tasarım kabulüdür; bağımsız codec COMM4-06'da uygulandı.

**COMM4-03 tamam:** [kalıcı kimlik registry ve store](identity-registry.md);
station Runtime/listener entegrasyonu sonraki kartlardadır.

**COMM4-04 tamam:** [station snapshot/CAS ve Runtime sınırı](station-activation.md).
Kalıcı seçim üretim izni açmaz; listener/Auto/dispatch sonraki kartlardadır.

**COMM4-05 tamam:** [process listener ve station Run sınırı](server-lifecycle.md).
Startup listen, bağımsız Stop/drain ve bounded checkpoint.

**COMM4-06 tamam:** [bounded binary codec/framer ve write queue](binary-codec.md).
F yazılım kabulü; dispatch sonraki kartlardadır.

**COMM4-07 tamam:** [HELLO/session ve peer sahipliği](peer-session.md).
Listener codec'e bağlı; discovery/exact negotiation ve stale/retired fence var.
Normal main catalogue manifest provider'ı henüz yok; station portu aşağıda08'dedir.

**COMM4-08 tamam:** [station admission ve mevcut Runtime adapter'ı](station-admission.md).
Exact snapshot, atomik resource/durable-intent bariyeri ve session TRIGGER portu;
normal main station/ledger/worker composition'ı sonraki entegrasyonları bekler.

**COMM4-08a tamam:** [PLC reçete seçimi ve ortak Auto/Stop portu](station-control.md).
Durable control port, generation fence ve exact kontrol ACK'i; gerçek disk ledger13'te.

**COMM4-09 tamam:** [bağımsız bounded istasyon yürütmesi](station-execution.md).
Ortak Runtime motoru, station-local completion/drain ve kayıt/sink retry izolasyonu.

**COMM4-10 tamam:** [ortak UUID/fiziksel kamera lease portu](camera-leases.md).
Atomik küme, provider claim, gerçek drain ve sahiplik diagnostics.

**COMM4-11 tamam:** [sabit template ve typed assignment domain](publication-template.md).
Recipe dışı revision, strict schema ve type/unit/whole Pose doğrulaması.

**COMM4-12 tamam:** [immutable fixed-slot evaluator](publication-evaluation.md).
Exact cycle/provenance/DAG, rational ties-even ve seal öncesi validity verisi.

## Okuma ve kapsam

Başlangıç paketi AGENTS, `docs/progress.md`, kök README ve çalışma protokolüdür.
Ardından bu indeks, [ortak sözleşme](contract.md), seçili kart ve yalnız kartın
işaret ettiği [wire bölümü](wire.md) / [kabul matrisi](verification.md) okunur.
Kartların tamamını her oturumda yüklemeyin.

Hedef; uygulama açılışında tek TCP listener, aynı bağlantıda hedef istasyona
tetik, istasyon başına aktif reçete/kamera ve kalıcı terminal sonuç+ACK akışıdır.
Run/Auto ağın ömründen ayrıdır. Üç istasyon alt sınır örneğidir; tasarım sabit üçlü
değildir. Set Variables ve doğrudan typed output ataması aynı sabit Number/Flag/
Text/whole Pose şablonuna gider. Eye-to-hand ve eye-in-hand oturumları üretimden
ayrı lease kullanır; robot hareket komutu kapsam dışıdır.

Mevcut [v3 sözleşmesi](../communication/contract.md) ve
[kullanımı](../communication/tcp-v3-usage.md) tarihsel/uygulanan baseline'dır.
COMM4 hedefleri bunların uygulanmış olduğuna dair iddia oluşturmaz. Eski arşiv,
dedup ve ACK kayıtları korunur. Native OECS/frame correlation, lens/bağımsız mm,
fiziksel PLC/robot ve dağıtım engelleri bu planla kapanmaz.

## Sıra ve durma kapıları

| Kart | Tek sorumluluk | Önkoşul |
| --- | --- | --- |
| [01](cards/COMM4-01.md) | Güncel kaynak ve davranış envanteri — tamam | Yok |
| [02](cards/COMM4-02.md) | Wire byte fixture ve sözleşme dondurma — tamam | 01 |
| [03](cards/COMM4-03.md) | Kalıcı kimlik kataloğu — tamam | 02 |
| [04](cards/COMM4-04.md) | Station domain ve immutable aktivasyon — tamam | 03 |
| [05](cards/COMM4-05.md) | Listener/prod yaşam döngüsü ayrımı — tamam | 04 |
| [06](cards/COMM4-06.md) | Bounded binary codec/framer | 02,05 |
| [07](cards/COMM4-07.md) | HELLO, session ve peer sahipliği — tamam | 03,06 |
| [08](cards/COMM4-08.md) | İstasyon admission ve tetik yönlendirme — tamam | 04,07 |
| [08a](cards/COMM4-08a.md) | PLC reçete seçimi, Auto/Stop ve yerel kontrol yarışı — tamam | 08 |
| [09](cards/COMM4-09.md) | Bağımsız istasyon yürütmesi — tamam | 08a |
| [10](cards/COMM4-10.md) | Ortak kamera kaynak arbitrajı — tamam | 09 |
| [11](cards/COMM4-11.md) | Sabit şablon ve typed assignment domain — tamam | 04 |
| [12](cards/COMM4-12.md) | Immutable publication değerlendirmesi — tamam | 09,11 |
| [13](cards/COMM4-13.md) | Kalıcı terminal ve exact ACK ledger | 07,12 |
| [14](cards/COMM4-14.md) | Reconnect/retry/backpressure | 13 |
| [15](cards/COMM4-15.md) | Transactional legacy migration | 03,14 |
| [16](cards/COMM4-16.md) | Tek Communication UI | 05,10,14,15 |
| [17](cards/COMM4-17.md) | Publish assignment UI | 11,12,16 |
| [18](cards/COMM4-18.md) | Robot/optik araştırma ve 6D uygunluk kapısı | 01,02 |
| [19](cards/COMM4-19.md) | Frame ve robot pose adapter sözleşmesi | 03,18 |
| [20](cards/COMM4-20.md) | Kalibrasyon session ve lease | 10,19 |
| [21](cards/COMM4-21.md) | SAMPLE pose/frame eşleştirme | 13,20 |
| [22](cards/COMM4-22.md) | İki kurulum için aday solve | 18,21 |
| [23](cards/COMM4-23.md) | Bağımsız validate ve atomik activate | 22 |
| [24](cards/COMM4-24.md) | Kalibrasyon UI ve operatör akışı | 16,23 |
| [25](cards/COMM4-25.md) | PLC/robot simulator ve dayanıklılık kabulü | 14,17,24 |
| [26](cards/COMM4-26.md) | Entegre kapanış ve fiziksel kabul raporu | 25 |

Kartlar listedeki sırayla uygulanır; önkoşul tablosu bağımlılığı açıklar,
paralel ajan yetkisi vermez. COMM4-02 dondurulmadan codec/PLC uygulaması başlamaz.
Toplam 27 kart vardır. COMM4-02 robot pose taşıma kabiliyetleri için 18'in dar
protokol ön kontrolünü erkene alır; kapsamlı optik/solver araştırması 18'de kalır.
Marka bilinmiyorsa generic quaternion adapter açıkça seçilir, controller uyumluluğu
kanıtlanmış sayılmaz; sonradan gereken wire değişikliği yeni contract freeze ister.
COMM4-18 eksik fiziksel bilgiyle yazılım arayüzlerini ve fixture araştırmasını
ilerletebilir; gerçek solver/activation ve fiziksel kabul uygunluk kapısını
atlayamaz. Gerekli veri yoksa 22–26 yazılım fixture kapsamı ve fiziksel blok ayrı
raporlanır; fixture üretim uygunluğu doğurmaz.

## Başlatma mesajı

```text
COMM4-01, docs/architecture/communication-v4/cards/COMM4-01.md.
D:/TVision; GPT-6.1 Sol / medium; tek uygulayıcı, başka ajan açma.
AGENTS ve working-protocol'u uygula. Kartın sahipliğinde çalış;
kullanıcının README/prototypes değişikliklerini koru. Önce güncel kaynak,
graph coverage ve baseline envanterini tamamla. Gerçek COMM4 uygulaması
başlatma. Kapanış kaydı, progress ve arşiv indeksi, bağlam kontrolü ve
tek COMM4-01 commit; push yok. Sonraki kartı otomatik başlatma.
```

Her kart kendi ID'siyle tek commit, tarihli kapanış kaydı ve progress güncellemesi
ister. Bu planın hazırlanması kartların tamamlandığı anlamına gelmez. Gerçek dosya
sahipliği COMM4-01'de netleşir; kartlardaki yeni sınıf/dosya adları açıkça adaydır.
