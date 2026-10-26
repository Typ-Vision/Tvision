# Haberleşme verileri — uygulama planı

**COMM-PLAN-03 · 26 Ekim 2026 · COMM-01 baseline, COMM-02 scalar ve COMM-03 spatial tamamlandı.**
Bu paket [ilk araştırmanın](../communication-data-phase.md) sekiz geniş kartının
yerine geçer. [Kalibrasyon veri tipi notu](../robot-spatial-data-review.md) girdi
olarak korunur. Önceki COMM-01–08 kart başlıkları uygulanmış teslim değildir.

## Hedef ve sınır

Tek Recipe Editor/Runtime içinde çoklu değişken ataması, gerçek ölçüm ve
OK/NOK/NOT_EVALUATED kararları, birden çok hedefe sonuç gönderimi ve PLC/robot
software tetikleri. Temel teslim mevcut TCP hattını genişletir; protokolden bağımsız veri/atama/
yayın/tetik modeli, çoklu hedef ve frame/birimli genel Pose aktarımı içerir.
Modbus TCP, OPC UA client ve kalibrasyon/robot profilleri ayrı, ihtiyaca bağlı
alt fazlardır; temel teslimin tamamlanma şartı değildir.

Hand-eye solver, yeni kalibrasyon sihirbazı, tam 3D parça bulma, robot hareket
kontrolü, OPC UA server, PROFINET/EtherNet/IP ve genel script dili bu paket
dışındadır. Pose taşımak çalışan robot guidance veya kalibrasyon demek değildir.
Mevcut native kamera ve fiziksel kabul engelleri bu işlerle kapanmaz.

## Bir oturumda tek kart

Uygulayıcı model kullanıcı tarafından **GPT-6.1 Sol / medium veya low** seçilir.
Tablodaki düzeyler görev boyutuna ilişkin öneridir, model performans garantisi
değildir. Agent/model ayarı otomatik değiştirilmez; paralel ajan çalıştırılmaz.

1. AGENTS + progress + kök README, bu indeks, kartın belirttiği ortak sözleşme
   bölümleri ve yalnız seçili kartı oku. Bütün kartları/PDF'yi yükleme.
2. Kartın bağımlılığı tamamlanmadan başlamama; mevcut commit/kapanıştan doğrula.
3. Kartın tek hedefini uygula. Mimariyi yeniden tasarlama veya sonraki kartı aynı
   commit'e ekleme. Bir kart beklenenden büyükse kalanını alt ID ile böl ve
   ilerleme/kart indeksine işle; yarım işi tamamlandı işaretleme.
4. Kabul ölçütlerini çalıştır. UI değişiminde Release + ilgili CTest ve normal/
   küçük ekran görsel kontrolü şarttır. Genel kontrol [doğrulama rehberinde](verification.md).
5. progress özetini güncelle; `docs/archive/YYYY-MM-DD-comm-NN.md` kapanışını ve
   arşiv indeksini ekle; `COMM-NN: ...` tek yerel commit. Push yalnız kullanıcıyla.
   Kapanışta `Uygulayıcı: Codex` kullan; Claude Code uygulanırsa AGENTS kuralını izle.

## Başlangıç önkoşulu

[COMM-01 kaynak haritası](source-map.md) actual dosya/sembol/test/şema ve Release
baseline'ını sabitler. Eski `C:/Projects/Tvision` yoktur; mevcut checkout ve hazır
indeks `C:/OpenCode/TVision` kökü/HEAD'iyle eşleştirildi. Seçili coverage'ın kısmi
satırları kaynakla tamamlandı. Sonraki kartlar bu haritanın ilgili bölümünü kullanır;
indeks erişim/kapsama hatası boş sonuç olarak yorumlanmaz.

Sınıf/test/şema adları COMM-01 kaynak haritasında current source ile sabitlendi. Aşağıdaki yeni
dosya adları önerilen sahipliktir; mevcut eşdeğeri varsa onu genişlet, paralel
ikinci framework kurma. Numaralı güncel şemalar eski belgelerden kopyalanmaz.

## Fazlar ve yürütme sırası

Kart ID'leri korunur; numara sırası otomatik görev sırası değildir.
**Temel faz: COMM-01–23 → COMM-36 → COMM-37** (25 kart). İlk kart COMM-01.
Bu faz ölçüm/kontrol çıktısını çoklu değişkene atar, PLC/robot/uzak hedef için
veri paketi hazırlar ve TCP üzerinden dış tetik alır. Genel pose taşıma birim,
frame ve açık yönelim formatı ister; hand-eye çözümü veya robot hareketi üretmez.

| Alt faz | Kartlar | Başlatma koşulu |
| --- | --- | --- |
| Modbus TCP | COMM-24–28 | Kullanıcı hedef PLC için Modbus ihtiyacını seçtiğinde; COMM-23 sonrası |
| OPC UA client | COMM-29–33 | Kullanıcı uzak hedef için OPC UA client rolünü seçtiğinde; COMM-23 sonrası, Modbus bağımsız |
| Kalibrasyon örnek verisi | COMM-34 | Gerçek robot pose/görüntü eşleme ihtiyacı seçildiğinde; kartın temel bağımlılıkları sonrası |
| FANUC profili | COMM-35 | FANUC formatı ihtiyacı seçildiğinde; COMM-03 ve COMM-23 sonrası, COMM-34 bağımsız |
| Fiziksel saha kabulü | COMM-38 | Sağlanan cihaz ve seçilen protokol için yazılım kabulü sonrası |

Alt faz seçimi olmadan COMM-23/28/33/35 kapanışı başka protokol veya robot
alt fazını kendiliğinden başlatmaz. Her seçili genişletme sonrası COMM-36'nın
ilgili entegrasyon senaryoları ve COMM-37 regresyonu yeni kanıtla doğrulanır;
önceki temel teslim yeniden başlatılmaz. SDK engeli yalnız ilgili alt fazı
bekletir. Fiziksel kabul açık kalabilir; bu durum yazılım teslimiyle karıştırılmaz.

Set Variables hesaplar; Publish Data gönderilecek paketi tanımlar. Gerçek
iletim çevrim sonunda kalıcı terminal kayıt sonrası yapılır. Akış ortasında
robota veri gönder → yanıt bekle → aynı akışa devam et davranışı bu fazda yoktur;
ihtiyaç olursa ayrı etkileşim/handshake tasarımı gerekir.

Low kartları sınırlı UI/codec/probe işidir; DAG, snapshot, kayıt ve ağ yaşam
döngüsü kartlarını medium seçmek önerilir. Aşağıdaki tablo kart envanteridir;
yürütme sırası yukarıdaki fazlara göre seçilir.

| Kart | Teslim | Düzey |
| --- | --- | --- |
| [COMM-01](01-source-baseline.md) | Kaynak/kapsama envanteri ve güncel baseline | medium |
| [COMM-02](02-scalar-values.md) | Typed scalar değer ve geçerlilik | medium |
| [COMM-03](03-spatial-values.md) | Frame/birimli Pose/Transform | medium |
| [COMM-04](04-variable-definition.md) | Variable tanımı, codec ve proje servisi | medium |
| [COMM-05](05-assignment-evaluator.md) | Atomik çoklu atama evaluator'ı | medium |
| [COMM-06](06-assignment-dag.md) | Set Variables registry/DAG/executor | medium |
| [COMM-07](07-variable-ui.md) | Variable Catalog UI | low |
| [COMM-08](08-assignment-ui.md) | Çok satırlı Set Variables UI | medium |
| [COMM-09](09-runtime-variables.md) | Immutable Runtime variable context | medium |
| [COMM-10](10-decision-core.md) | Genel parça kararı evaluator'ı | medium |
| [COMM-11](11-decision-ui.md) | Result aracı, portlar ve UI | medium |
| [COMM-12](12-publication-core.md) | Endpoint profili, yayın grubu ve intent | medium |
| [COMM-13](13-terminal-bundle.md) | Immutable terminal bundle ve kayıt codec'i | medium |
| [COMM-14](14-delivery-store.md) | Kalıcı delivery ledger/outbox | medium |
| [COMM-15](15-trigger-router.md) | Ortak giriş snapshot'ı ve Trigger Router | medium |
| [COMM-16](16-tcp-v3-codec.md) | Yeni TCP v3 codec/framing | medium |
| [COMM-17](17-tcp-v3-service.md) | TCP v3 → Runtime → kayıt → outbox | medium |
| [COMM-18](18-restart-recovery.md) | Restart/dedup/recovery | medium |
| [COMM-19](19-delivery-retry.md) | Timeout, retry ve kapasite sınırları | medium |
| [COMM-20](20-endpoint-ui.md) | Endpoint bağlantı profili UI | low |
| [COMM-21](21-publication-ui.md) | Publish Data ve mantıksal mapping UI | medium |
| [COMM-22](22-diagnostics-ui.md) | Trigger/delivery diagnostics | low |
| [COMM-23](23-tcp-acceptance.md) | TCP yazılım/görsel kilometre taşı | medium |
| [COMM-24](24-modbus-probe.md) | Modbus bağımlılık/rol probe'u | low |
| [COMM-25](25-modbus-codec.md) | Modbus register mapping codec'i | low |
| [COMM-26](26-modbus-transport.md) | Modbus client transport/lifecycle | medium |
| [COMM-27](27-modbus-handshake.md) | Modbus tetik/result/ACK handshake | medium |
| [COMM-28](28-modbus-ui-acceptance.md) | Modbus UI ve yazılım kabulü | medium |
| [COMM-29](29-opcua-probe.md) | OPC UA SDK/toolchain probe'u | medium |
| [COMM-30](30-opcua-access.md) | OPC UA typed read/write ve node kimliği | medium |
| [COMM-31](31-opcua-trigger.md) | OPC UA subscription/trigger/ACK | medium |
| [COMM-32](32-opcua-lifecycle.md) | OPC UA güvenlik, reconnect ve Stop | medium |
| [COMM-33](33-opcua-ui-acceptance.md) | OPC UA UI ve yazılım kabulü | medium |
| [COMM-34](34-calibration-samples.md) | Opsiyonel: Robot actual pose ve CalibrationSample | medium |
| [COMM-35](35-robot-format.md) | Opsiyonel: FANUC veri profili ve pose codec'i | medium |
| [COMM-36](36-multi-endpoint.md) | Çoklu hedef ve pose paketi tutarlılığı | medium |
| [COMM-37](37-release-acceptance.md) | Birleşik Release/regresyon kabulü | medium |
| [COMM-38](38-physical-acceptance.md) | Cihaz bazında fiziksel haberleşme kabulü | medium |

## Teslim kapıları

- **COMM-08:** Editor'da çok satırlı atama denenebilir; dışarı yazmaz.
- **COMM-23:** TCP üzerinden ölçüm/kontrol verisi ve dış software tetik çalışır;
  restart belirsizliği/ACK testlidir. Fiziksel PLC kabulü ayrı kalır.
- **COMM-28 / 33 (seçilirse):** Modbus / OPC UA simulator kabulü. Gerçek PLC yoksa
  yazılım teslimi kapanabilir; fiziksel kabul açıkça COMM-38'e taşınır.
- **COMM-36:** genel pose paketi ve çoklu hedef teslimi TCP ile testlidir;
  CalibrationSample, FANUC, Modbus veya OPC gerektirmez. Gerçek kalibrasyon
  veya robot hareket yetkisi oluşturmaz.
- **COMM-37:** temel fazın birleşik yazılım kabulü; sonradan seçilen ekler ayrıca
  regresyondan geçer. **COMM-38:** sağlanan her gerçek endpoint
  için ayrı fiziksel kabul. Eksik donanımın kabulü kapatılmaz.

## Başlatma mesajı

> Haberleşme planındaki yalnız COMM-01 kartını uygula. AGENTS.md, progress,
> README ve docs/architecture/communication/README.md ardından seçili kartı ve
> işaret edilen sözleşme bölümlerini oku. Codebase-memory önkoşulunu doğrula.
> Önceki işleri yeniden başlatma, başka karta geçme. Kabul kanıtını, progress ve
> arşiv kaydını tamamla; görev ID'li tek yerel commit yap, push yapma.

Sonraki oturumlarda mesajdaki kart ID'si ve dosyası güncellenir; kullanıcı
model/düzeyi uygulamadan seçer.
