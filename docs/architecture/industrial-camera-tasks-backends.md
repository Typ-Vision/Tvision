# ICM — marka ve model bazlı kamera entegrasyonları

18 Ekim 2026 kullanıcı kararı. **PLANLANDI; hiçbir yeni native destek açılmadı.**
Ortak uygulama ve G2 yazılım zinciri [ana plandadır](industrial-camera-plan.md).
Bu paketler aynı application adapter/worker/lease sözleşmesine bağlanır.
Ajanlar sıralı çalışır; paketlerin ayrı olması paralel ajan çalışması anlamına gelmez.

## Kapsam ve kabul birimi

Hedef Basler/Daheng PoE kameralardır. Exact model, firmware, seri, SDK sürümü,
transport ve gerekli Mono8/software-trigger yetenekleri seçilmeden destek ilan edilmez.
PoE güç beslemesidir; GigE Vision transport veya vendor SDK desteği tek başına
tetik–kare eşleşmesi garantisi değildir. Ortak kod belirli node/sayaç isimlerini
bütün markalara zorunlu kılmaz; eşdeğer doğruluk kanıtı ayrı typed strateji ve test ister.

Her paket başlangıçta tek kamera, Mono8, software trigger, owned frame, preview,
Stop/recovery kapsamındadır. Hardware trigger, PTP, çoklu eşzamanlı kamera, renk/packed
format ve dağıtım ayrı işlerdir. Model bunları desteklese bile ürün desteği varsayılmaz.
Model/firmware/SDK değişiminde etkilenen kabul yeniden değerlendirilir.

| Paket | SDK adayı; karar 01'de | Başlangıç durumu |
| --- | --- | --- |
| ICM-BAS | Basler pylon veya uygun GenTL yolu | Model/SDK/cihaz kanıtı bekleniyor; ortak G2'yi engellemez |
| ICM-DAH | Daheng Galaxy veya uygun GenTL yolu | Model/SDK/cihaz kanıtı bekleniyor; ortak G2'yi engellemez |
| OECS | Mevcut ICM-02 C API kararı | 04d native açık/ertelenmiş; G1 kapalı |

## ICM-BAS-01 / ICM-DAH-01 — model, SDK ve fiziksel başlangıç

- Her ID ayrı görev/commit'tir; ilk erişilebilir hedef cihazla başlanabilir.
- Önkoşul: exact hedef model/transport ve SDK dokümanları; G0 için fiziksel erişim.
- İş: SDK lisansı/runtime, MinGW C++20 ABI veya dar C sınırı, export/struct ve
  optional dependency deneyi; discovery, exact identity, exclusive open/close,
  feature readback ve viewer görüntüsü. GigE NIC/IP/MTU/PoE koşullarını kaydet.
  Ortak discovery'nin bulduğu cihazın native open kimliği ayrıca doğrulanır.
- Kabul: backend seçim kararı, sürüm/hash/env kanıtı ve H01; SDK yokken ürün
  başlangıcı/file/webcam etkilenmez. Fiziksel cihaz yoksa offline karar kısmi
  kalır, G0 açık yazılır. OECS 01b/04d bu görevin bağımlılığı değildir.
- Çıktı: ilgili backend karar belgesi, `build/icm/bas-01/` veya `dah-01/` kanıtı.
- Sonraki: aynı paketin 02'si; SDK binary/header'ları repoya kopyalanmaz.

## ICM-BAS-02 / ICM-DAH-02 — native adapter ve G1

- Önkoşul: ilgili 01 kararı ve G0, ortak 04f/05a sözleşmesi.
- İş: native session/configure/readback/stream/owned copy/trigger/lifecycle
  implementasyonu. 04a–04e kartlarının kabul maddelerini ilgili SDK semantiğiyle
  uygula; tamamlanan ortak worker/lease/codec işlerini tekrar yazma.
- Kabul: belgeli korelasyon stratejisi; eski preview/pending command ve SDK kuyrukları
  yeni çevrime taşınmaz. Timeout/cancel sonrası belirsiz oturum yeniden kullanılamaz;
  belgeli ve sınanmış recovery gerekir. Stop→Start, flush veya host generation tek
  başına kanıt değildir. Owned frame/geometri, eksik payload, kimlik, disconnect,
  geç başarı, bütçe ve cleanup negatifleri sınanır.
- Kanıt: fixture corpus'u yanında gerçek tek/100 trigger, preview geçişi, Stop ve
  kaynak ömrü; H02–H05'in acquisition kısmı. Sahne ve olay/kare eşleşmesi gösterilir.
  G1 yalnız ilgili backend/model kapsamında kapanır. Tam H01–H08 raporu 03'tedir.
- Ürün capability'si G1 + ortak 08b tamamlanmadan production'a açılmaz. G1 tek
  başına G3/G4 veya kurulum/dağıtım kabulü değildir.
- Sonraki: ilgili 03; kanıt `build/icm/bas-02/` veya `dah-02/`.

## ICM-BAS-03 / ICM-DAH-03 — ürünün fiziksel e2e kabulü

- Önkoşul: ilgili backend G0/G1 ve ortak ICM-09a/G2.
- İş/kabul: [ICM-09b](industrial-camera-tasks-integration.md) kartını ve
  [H01–H08](industrial-camera-acceptance-plan.md) matrisini seçili cihazla uygula.
  Gerçek 1000 cycle/teardown, kopma, viewer contention, üç applicationType,
  mixed-source, Logs/CSV ve native UI kanıtı gerekir.
- Çıkış: ilgili backend/model için G3; başka marka/model veya N-camera kapasitesi
  kabulü değildir. Kanıt `build/icm/bas-03/` veya `dah-03/`.
- Sonraki: ölçüm gerekiyorsa ilgili kurulum için ICM-10/G4; bağımsız referans ve
  tolerans olmadan fiziksel doğruluk beyan edilmez.

## OECS ertelenen iş ve isteğe bağlı önizleme

- **ICM-04d native / 04e native:** açık/ertelenmiş; mevcut karşı örnekler korunur.
  Belgeli pending-command cancel/completion/drain/reset veya başka doğrulanmış
  korelasyon yolu gelince devam edilir. Aynı sessizlik/Stop→Start deneyi tekrarlanmaz.
  [Korelasyon kararı](industrial-camera-trigger-correlation.md) geçerlidir.
- **ICM-OECS-P1 — sınırlı gerçek preview (PLANLANDI, ortak sırayı durdurmaz):**
  önkoşul 04f/05a, mevcut 04c ve 04e-a/b; UI bağlantısı için 05b.
  Gerçek owned Mono8 probe'u exclusive lease ve bounded preview akışına bağla;
  normal Stop/cleanup, incomplete payload, unplug ve geç kare bastırmayı test et.
  Her görüntü preview olarak sunulur; ölçüm/kalibrasyon aktivasyonu, Editor
  çevrim girdisi, Runtime production veya trigger freshness kanıtı olamaz.
  G1/04d kapısını açmaz. 03c crop'u varsa gerçek raster geometri/origin korunur;
  ilk satır çıkarımı tetik sorununu çözmüş sayılmaz.
  Release, ilgili CTest, normal/küçük görsel ve fiziksel preview/Stop kanıtı gerekir;
  bunlar yoksa yalnız fixture kabulü kaydedilir. Kanıt `build/icm/oecs-p1/`.

Bu plan yalnız görev ayrımıdır. Simülasyon/test adaptörü açık etiketlenir;
gerçek cihazın doğrulanmamış karesi yeni tetikle eşleşmiş gibi gösterilmez.
