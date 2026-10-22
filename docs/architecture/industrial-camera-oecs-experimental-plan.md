# ICM-PLAN-03 — Mevcut OECS kamera ile deneysel kullanım

**22 Ekim ürün akışı güncellemesi:** aşağıdaki açık deneysel seçim ve kalıcı
tetik eşleşmesi uyarısı gerekleri [CCUX C0](camera-setup/contract.md#c0--22-ekim-sade-ürün-akışı-kararı)
ile değiştirilmiştir. [CCUX-16–19](camera-setup/README.md) normal komutlardan
politika seçimi ve görünür seçenek/uyarıların kaldırılmasını kapsar. CCUX-16
application politikası uygulandı; görünür temizliği CCUX-17 bekler. Aşağıdaki
teknik kanıt, kimlik/lease ve kayıt kuralları geçerlidir.

21 Ekim 2026. **EXP-02 px Editor teslimi hazır; gerçek mm/kalibrasyon kabulü açık.**
EXP-03 deneysel Runtime da teslim edildi. Güncel robot dışı kalibrasyon/mm ve
UI devamı [CCUX](camera-setup/README.md) kartlarıdır; aşağıdaki eski “sonraki”
ifadeleri güncel görev sırası değildir.
[Kısmi kapanış ve kanıt](../archive/2026-10-21-icm-oecs-exp-01.md).
Güncel kanıt ve devam noktası [devir özetindedir](../progress.md).
Bu belge native production capability açmaz.
Kullanıcı başka kamera bulamayacağını, mevcut kamera için iptal/boşaltmayı
çalışıyor varsayarak ilerlemek istediğini belirtti. Basler/Daheng cihazı veya
üretici yanıtı aşağıdaki geliştirme sırasının önkoşulu değildir.

## Karar ve kapsam

Mevcut COE-050-M-USB-050-IR-C / seri 02A2220000627, OECS C API üzerinden
gerçek görüntü edinimi, Editor ölçüm geliştirmesi ve deneysel Runtime için
öncelikli cihazdır. Ortak worker/lease, profile, Editor ve Runtime kullanılır;
ikinci uygulama veya test adaptörünü ürün factory'sine açma yolu oluşturulmaz.

Clear/Stop/Start'ın eski işleri temizlediği **kullanıcının kabul ettiği çalışma
varsayımıdır**, doğrulanmış donanım garantisi değildir. [8/8 eski kare
karşı örneği](../archive/2026-10-18-icm-04d-pending.md) geçerlidir. Yanlış çevrim
veya önceki parça görüntüsüyle hesap yapılması bu varsayımın somut sınırıdır.
TriggerDelay=0 readback ve tek bekleyen istek bu riski azaltmaya yönelik çalışma
koşullarıdır; geçmiş command'ın kalmadığını kanıtlamaz.

Bu karar önceki G1 şartını deneysel kullanımın önkoşulu olmaktan çıkarır.
Doğrulanmış production yolu için [korelasyon sözleşmesi](industrial-camera-trigger-correlation.md)
ve G1 şartı korunur. Deneysel sonuç gerçek pikselden hesaplanabilir; sonucu bu
isteğin tetiğiyle/üretimdeki doğru parçayla kesin eşleşmiş diye sunmak kapsam dışıdır.
G0/G1/G3/G4 otomatik kapanmaz; gerçek sahne, kimlik, yaşam döngüsü ve ölçüm
kanıtları kendi kapsamlarında toplanmaya devam eder.

## Uygulama sözleşmesi

- Ayrı, açık deneysel acquisition politikası/capability gerekir. Mevcut
  `ActualCounterEpoch` başarılıymış gibi döndürülmez; sahte arm, baseline,
  ready veya trusted completion üretilmez. Native `Unavailable` bugün korunur;
  yeni politika EXP-01 uygulanınca yalnız deneysel girişten kullanılabilir.
- Normal işlem komutu CCUX-16 application politikasını seçer; ayrıca opt-in
  gerekmez. Exact desteklenen cihaz/profil veya işaretli PGM/JSON kaynağı
  ayrıştırılır; yanlış/eksik kanıt loader/probe tarafından reddedilir. Restart
  veya eski reçete açılışı acquisition/Run başlatmaz; görünür eski kutu/metinlerin
  kaldırılması CCUX-17 teslimidir. Politika production izni değildir.
- Exact identity, gerçek configure/readback, owned Mono8 kopya, payload/geometri
  denetimi, exclusive lease ve monoton süre bütçesi zorunludur. Yanlış cihaz,
  eksik kare veya geçersiz geometri varsayımla kabul edilmez.
- Başlangıç profili explicit FrameBurstStart, burst=1, cache=false,
  TriggerDelay=0; yazma ve readback ayrı doğrulanır. FrameStart alias'ı yoktur.
  Tek istek bekler; önceki SDK işi bitmeden yeni istek gönderilmez.
- Stop/geç completion çıktısı bastırılır. Timeout, cancel, command failure veya
  kopmada oturum kullanımı durur; otomatik retry/üretime dönüş yoktur. Manuel
  yeniden başlatma da aynı doğrulanmamış bariyer varsayımını taşır; iyileşme
  kanıtı olarak raporlanmaz. Clear yalnız stream çalışırken, sonra Stop uygulanır.
- Metadata bankaları kapalıysa tam 2448×2048 temiz raster politikası kullanılır.
  Gömülü metadata açıksa doğrulanmış layout ve ICM-03c ile ilk satırı çıkaran
  2448×2047/origin(0,1) politikası açıkça seçilir. Geometri/hash/kalibrasyon bağı
  değişir; eksik sensör değerleri sıfırlanarak ölçüme geri sokulmaz.
- Kaynak kanıtında gerçek cihaz, profil/readback, raster hash/geometri ve varsa
  gerçek sayaçlar korunur; tetik eşleşmesi ayrıca **doğrulanmadı** olarak taşınır.
  Host request ID fiziksel tetik kanıtı sayılmaz. Cache/Save/reopen/replay bu
  niteliği yükseltemez; şema değişirse eski codec/no-op davranışı korunur.
- Gerçek görüntüden araç ölçüsü ve tolerans sonucu gösterilebilir; deneysel
  bağlam görünür kalır. Geçersiz fit/kalibrasyonda değer/PASS üretilmez.
  Deneysel Runtime kaydı trusted production terminaline dönüştürülmez;
  PLC/robot/otomatik parça karar çıkışına aktarılmaz.

## Sıralı görevler

### ICM-OECS-EXP-01 — Gerçek edinim ve yaşam döngüsü

Önkoşul: mevcut ICM-02 C API kararı, 04c, 04e-a/b ve 05a/05b. Belgeli cancel
bariyeri, başka cihaz ve tamamlanmış G1 aranmaz. İlk adım mevcut cihazı exact
kimlikle açmak, ışık/pozlama ile görülebilir sahne almak ve açık ICM-01b/H01–H02
maddelerine gerçek kanıt eklemektir; eski neredeyse siyah raster yeterli değildir.

Native provider'a ayrı deneysel politika, configure/readback ve owned capture
bağlanır. Camera Manager preview/Stop ve tek kareyi dosyaya kaydetme eklenir.
EXP-01 yalnız metadata bankalarının tamamı kapalı 2448×2048 rasteri kabul eder;
gömülü metadata/ilk satır çıkarma yolu bu teslimde açılmaz. PGM içi deneysel işaret
ve JSON sidecar kullanılır. EXP-02 Editor opt-in ile kanıtı doğrulayarak replay
açar; varsayılan Image File/Runtime yolu bu rasteri reddetmeye devam eder.
Dosya piksel hash/geometri ve doğrulanmamış eşleşme kaydıyla korunur; bu gerçek
raster MUI-04'ün Image File incelemesinde kullanılabilir. Önceki isteğe bağlı
ICM-OECS-P1 işi bu göreve alınır; ayrıca tekrar uygulanmaz.

Uygulamada binning/decimation çiftinin SDK `AM_NI` sonucu açıkça
`NotApplicable` olarak taşınır; `NA`, bilinmeyen veya karışık çift reddedilir.
Okunabilir enum geometride XML’deki factor-one entry/raw=1 doğrulanır.
ExposureAuto/GainAuto gerçek Off readback’i TriggerMode Off iken alınır;
XML’de trigger açıkken unavailable olan auto kontrolü uydurma final değerle
sunulmaz. Kanıtta readback aşaması ayrıca kayıtlıdır. Genel native configuration
ve production factory bu deneysel istisnayı seçmez.

Kabul: Release build, ilgili CTest, normal/küçük UI görselleri; gerçek sahnede
tek/100 sıralı çekim, Stop/geç kare, timeout, kopma ve yeniden açma gözlemleri.
Gecikmiş eski kare karşı örneği fixture ile korunur; yeni politika bunu trusted
evidence'a yükseltmez. Donanım koşumu yapılamazsa fiziksel kabul açık yazılır.
Kanıt: `build/icm/oecs-exp-01/`. Sonraki: EXP-02.

### ICM-OECS-EXP-02 — Editor, ölçüm ve MUI-04

Önkoşul: EXP-01 gerçek kullanılabilir raster ve yaşam döngüsü kabulü.
Deneysel kaynağı ortak Editor/Run Flow'a bağla; Line/Circle ROI, px/mm,
TargetOnly, kaynak/kalibrasyon invalidation ve Save/reopen akışlarını doğrula.
Deneysel kalibrasyon çalışması gerçek rasterle yapılabilir; aday/aktivasyon
ayrımı ve bağımsız referans koşulları korunur, production yetkisi üretmez.
Kalibrasyon ve reçete bağı deneysel kökeni kaybetmez.

[MUI-04 gerçek görüntü matrisi](measurement-usability-acceptance.md) uygulanır.
Yeni kamera görüntüsü önceki telecentrik problem örneğinin yerine geçmiş sayılmaz;
özgün problem görüntüsü/reçetesi yoksa o yeniden üretim maddesi açık tutulur.
Gerçek referans yokken yalnız px/raster incelemesi kabul edilir.

Kabul: ilgili Release/CTest, normal/küçük görseller, gerçek Line/Circle tanıları,
Save/reopen/no-op, Stop/invalidation ve yanlış kalibrasyon negatifleri.
Uygulanan px geliştirme yolu: Editor oturumuna özel açık politika, canlı
FrameBurstStart edinimi ve işaretli PGM + JSON replay. Politika reçeteye/INI’ye
yazılmaz; restart ve varsayılan execution kapalıdır. Dosya kanıtı piksel hash,
geometri, kimlik ve false admission/eşleşme alanlarıyla denetlenir; eksik/bozuk
kanıt reddedilir. JSON değişikliği TargetOnly cache imzasını da değiştirir.
Canlı deneysel istek Editor lease ister, production admission taşıyamaz ve
IndustrialExecutionEvidence/trusted completion oluşturmaz. Profil seçicisi
FrameBurstStart’ı açıkça sunar; FrameStart sessizce dönüştürülmez.

Kanıt: `build/icm/oecs-exp-02/`; [px teslim/kabul sınırı](../archive/2026-10-21-icm-oecs-exp-02.md).
Gerçek mm referansı/deneysel kalibrasyon aktivasyonu açık; sonraki geliştirme EXP-03.

### ICM-OECS-EXP-03 — Deneysel Runtime ve kayıt

Önkoşul: EXP-02; ayrı uygulama kurmadan ortak Runtime'da açık deneysel çalışma
modu ve provenance korunarak Load/Run/tek çevrim/Stop ile Logs/CSV tamamlanır.
Mevcut trusted terminal codec'ine sahte kanıt yazmak yerine deneysel kaydın
admission/completion/persistence ayrımı uygulanır; gerekirse şema sürümlenir.

Kabul: kullanıcı 21 Ekim 2026 yönlendirmesiyle **500** gerçek sıralı çevrim/teardown
raporu (önceki 1000 hedefi azaltıldı), kopma/timeout/Stop,
restart sonrası pasif durum, replay'de deneysel niteliğin korunması; PLC/robot
çıkışının ve trusted production yükseltmesinin engellendiği testler. Üç
applicationType aynı akışı kullanır. Sonuç deneysel e2e kabulüdür, ICM-09b/G3
veya fiziksel doğruluk G4 kapanışı değildir.
Kanıt: `build/icm/oecs-exp-03/`. Uygulama/kayıt sözleşmesi:
[deneysel Runtime](experimental-runtime.md).

## Daha sonra

Üretici yanıtı/yeni teknik kanıtla OECS 04d/04e ve G1'e dönülebilir; soru taslağı
gönderilmemiştir. Basler/Daheng paketleri cihaz bulunana kadar bekler. Doğrulanmış
acquisition sağlandığında ICM-09b/G3 ve bağımsız fiziksel ölçüm ICM-10/G4 tamamlanır.
Bu plan PLC/robot, CUDA, native DPI/çoklu monitör veya dağıtım kabulünü kapatmaz.
