# Runtime, reçete yükleme ve tek araç çalışma planı

Bu belgenin aşağıdaki P/T sıraları tarihsel plan bağlamıdır. Güncel ortak
Editor/Runtime akışı [kullanım rehberinde](../camera-runtime-usage.md),
Load → Verify → Run ve Stop/drain sözleşmesi [kaynak doğrulamada](source-verification.md),
immutable completion/kayıt retry davranışı [kurtarmada](runtime-error-recovery.md)
tanımlıdır. Eski binary yolları günlük başlatma hedefi değildir;
`VisionFlow.cmd` → `build/reference-ui/visionflow.exe` kullanılır.

9 Ekim tool planı revizyonu: [T0–T15 reçete tool planı](recipe-tools-plan.md) yeni kapsamın uygulama sırasıdır; ilk adım T0.1, lens en son T15. Bu plan P1–P6'yı yeniden açmaz, P7/6D.2'yi tamamlamaz. Görüntüden kod okuma eski P9'dan T3'e taşınmıştır; otomatik reçete seçimi ve OPC UA aynı iş değildir. T1 typed sonuç/kayıt, T10 genel karar ve explicit vf.socket/3 çoklu sonuç mapping'i planlar; mevcut vf.socket/2 uygulanmış sözleşmesi değişmedi. Aşağıdaki eski ileri aşama sıraları tarihsel kayıttır.

9 Ekim güncel durum: **C1–C5 tamamlandı**. Setup → Calibration üç yöntem ve beş bölüm sunar: Manuel ölçek (px/mm), Bilinen uzunluktan ölçek ve Kalibrasyon plakası (perspektif). Manuel girdi boş başlar; px/mm ve salt okunur tersi, üretim sonucundan ayrı deneme, en az iki bağımsız doğrulama, katalog/recipe Save/reopen, gerçek Edge → Measurement → Runtime ve CSV bağlıdır. Güncel Release binary `build/manual-calibration-ui/visionflow.exe`; Measurement tool 2, execution/6. Sonraki P7/6D.2; K4 fiziksel kabul ertelenmiştir.


8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4 tamamlandı: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 P2 tamamlandı: Recipe Manager tek tıkla yalnız satır seçer; **Load Recipe / Edit Recipe** ayrıdır. Load kaydedilmiş bağımsız kopyayı statik kontrol eder; dirty durumda Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal sunulur. Runtime **Run / Stop / Tek çevrim** kullanır; Run görüntü veya parça sonucu üretmeden coordinator ile otomatik hazırlanır. Manuel çevrim, cooperative Stop ve aynı terminal kayıt retry coordinator üzerinden geçer. Seçili, editörde açık ve Runtime yüklü reçete ayrıdır. Eski validate/activate/rollback düğmeleri kaldırıldı; rollback servis API’si korunur. Bu P2 teslimi P3/P4 ile genişletildi; güncel socket ve Run Tool davranışı yukarıdadır. Fiziksel kabul/K4 ertelenmiş kalır.

## 1. Amaç ve kapsam

Günlük kullanım: **Reçeteyi tek tıkla seç → Load Recipe → Run**. Kullanıcı ayrı Taslağı doğrula/Etkinleştir komutları çalıştırmaz. Kontroller application servislerinde otomatik yürütülür; başarısızlık somut araç/ayar/hata açıklamasıyla sunulur.

Run, programı seçilmiş çalışma kaynağından olay kabul eder duruma getirir. PLC veya robot komutuna özel değildir. Kamera donanım tetiğiyle alınan yeni kare, yazılım isteği, manuel tek çevrim veya ileride sürekli acquisition aynı yürütme yolunu besleyebilir. Sonuç hedefi de tetik kaynağından bağımsızdır: ekran, TCP, ileride OPC UA veya QR metni sunumu.

Yakın uygulama: reçete/Runtime akışının sadeleştirilmesi, bağımsız ve kalıcı socket ayarları/otomatik dinleme, mevcut görüntüde yalnız seçili araç, Blob alan filtreleri ve kırmızı bölge overlay'i. İleri uygulama: fiziksel hardware-trigger kamera, OPC UA, QR ve sürekli çalışma adapter'ları. İleri özellikler bu ilk teslimde çalışıyormuş gibi gösterilmez.

Tema, mevcut ekranlar ve genel yerleşim korunur. Bu kullanıcı talebi gerekli düğme, Properties ve durum gösterimi değişikliklerini açıkça yetkilendirir; eski belgelerdeki “düğmeler değişmez” kuralının bu kapsamda istisnasıdır. C++20/Qt Widgets/OpenCV, tek Recipe Editor/Runtime ve UI → application → engine/adapter sınırı korunur.

## 2. Başlangıç durumu (P1–P4 öncesi tarihsel kayıt)

- MainWindowWorkspace: reçete çift tıklaması editör açar; üretime yükleme ile düzenleme ayrı kullanıcı eylemleri değildir.
- MainWindowRuntime: validate yeni frame edinir, sonra activate ve tek çevrim gerekir. Immutable snapshot, dependency kontrolü ve kalıcı kayıt zaten vardır; kaldırılmayacak.
- PlcRuntimeSocket: RuntimeService referansına/proje arşivine bağlıdır; listen etkin snapshot ister. Dinleme açıkken UI manuel Runtime işlemlerini engeller. Ayarlar oturumluk, 128 accepted request sınırı ve restart recovery eksikliği vardır.
- ExecutionService::run(targetTool, frameSnapshot): hedef + upstream alt grafiğini yürütür. Mevcut UI Run Match yeniden acquisition yapar. Yalnız frameSnapshot vermek “yalnız seçili araç” gereksinimini karşılamaz.
- EditorSession: frame/cycle/signature ve referans cache'i vardır; bütün akış signature invalidation'ı seçili araç cache'i için tek başına yeterli değildir.
- ToolRegistry/ToolExecutors Blob: threshold, searchRoi, 8-connected bileşenler; count ve en büyük bileşen alanı/region/point. Min/max alan yoktur. En fazla 2048 bounding box çizilir; gerçek bölge maskesi yoktur.

İlgili başlangıç dosyaları: src/app/MainWindowWorkspace.cpp, MainWindowRuntime.cpp, MainWindowCommunication.cpp, MainWindowExecution.cpp, MainWindow.hpp/.cpp, ImageViewer.hpp/.cpp; src/application/RuntimeService.*, ProductionRecipeService.*, PlcRuntimeSocket.*, EditorSession.*, ExecutionService.*, ToolExecutors.*; src/domain/ToolRegistry.*, ToolDefinition.*. Properties'in gerçek tanım/uygulama yerini rg ile bul; var olmayan dosya adlarına göre yeni paralel UI oluşturma.

## 3. Reçete seçimi, yükleme ve Run

### 3.1 Reçete ekranı

- Tek tık yalnız satırı seçer; üstte Load Recipe ve Edit Recipe bulunur. Çift tık editörü açabilir.
- Load Recipe kaydedilmiş sürümün bağımsız kopyasını Runtime'a hazırlar ve başarıda Runtime ekranına geçer. Listede seçili, editörde açık ve Runtime'a yüklenmiş reçete kimlikleri ayrı tutulur.
- Dirty taslak varsa “Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal” seçenekleri sunulur; sessiz kaydetme veya düzenlemeleri atma yapılmaz. Kaydetme başarısızsa yükleme gerçekleşmez.
- Yükleme sırasında dosya/şema/DAG/model/bağımlılık gibi kamera gerektirmeyen kontroller yapılır. Bu aşama acquisition veya parça sonucu üretmez.
- Yüklenmiş ad/sürüm ekranda açık görünür. Başarısız yükleme önceki yüklenmiş sürümü korur ve hangi reçetenin kaldığını söyler; otomatik Run yapmaz.
- Çalışırken reçete değiştirme ilk sürümde engellenir; Stop ve devam eden kayıt/teslimin tamamlanması gerekir. Hot-swap kapsam dışıdır.

### 3.2 Run ve kontroller

- Run yüklenmiş kopyayı kullanır; editördeki taslağı kendiliğinden kullanmaz. Gerekli cihaz hazırlığı, permission ve dependency kontrolleri otomatik yapılır, başarıda immutable çalışma snapshot'ı/event kaydı oluşturulur.
- Ayrı Taslağı doğrula/Etkinleştir düğmeleri kullanıcı akışından kaldırılır. İç servis kontrolleri ve audit korunur; UI butonu içinde servis kuralları tekrar yazılmaz.
- Statik kontrol, cihaz hazırlığı ve gerçek frame kontrolü ayrılır. Hardware-trigger kamera için Run sırasında mutlaka yeni frame istemek yanlış olur: sensör henüz tetiklememiş olabilir. Kamera hazırlığı tamamlanınca tetik beklenir; actual frame/çözünürlük/kalibrasyon kontrolü ilk ve sonraki gerçek frame'lerde yapılır. Frame gerektiren eski validation API'si buna göre ayrıştırılır; sahte probe frame üretilmez.
- Run başarı durumu “Çalışıyor — tetik bekleniyor” veya kaynağa uygun eşdeğeridir. Run tek başına PASS veya üretim çevrimi oluşturmaz.
- Stop yeni olay kabulünü önce kapatır, kaynak aboneliğini durdurur/generation'ı geçersiz kılar, mevcut cooperative iptal politikasını uygular. Socket açık kalır. Başlamış kayıt/teslim kaybolmaz; Stop bekleyen DONE/ACK'i silmez.
- Tek Çevrim manuel test modunda erişilebilir; aynı coordinator ve tek in-flight kuralına tabidir. Donanım tetik modunda ikinci acquisition başlatan bir düğme gibi kullanılamaz.
- Yeniden açılışta socket otomatik başlayabilir, Run otomatik başlamaz. Son yüklenmiş reçete referansı gösterilebilir fakat yeniden kontrol edilmeden çalışır sayılmaz.
- Önceki sürüme dönüş gelişmiş sürüm alanına taşınabilir; durdurulmuş halde aynı Load/Run kurallarından geçer.

### 3.3 Durum sözleşmesi

Application düzeyinde yükleme/çalışma durumu: Unloaded, LoadedStopped, Starting, Running, Stopping, Fault. Running içinde çevrim Ready/Busy ayrı tutulur; RecordingPending ve DeliveryPending de bağımsız engellerdir. Socket Listening/Connected ve recipe DRAFT/VALID/INVALID ile part PASS/FAIL/NOT_EVALUATED bunlarla birleştirilmez.

Run başarısızsa yeni tetik kabul edilmez; hata kodu ve düzeltilecek ayar gösterilir. Dependency değişirse yeni çevrim engellenir. Eski sonuç tarihsel olarak tutulsa bile yeni parçanın sonucu gibi gösterilmez. Startup/Stop yarışı, geç callback ve hızlı ardışık Run tıklamaları tek owner-thread coordinator tarafından yönetilir.

## 4. Kaynak, yürütme ve sonuç hedefi ayrımı

Önerilen akış: **CycleEvent → Runtime coordinator → immutable recipe + owned frame → ortak engine → kalıcı sonuç → ResultSink**. İsimler öneridir; mevcut sınıflarla gereksiz paralel altyapı kurmadan uygulanır.

- İstek olayı (TCP/manual) acquisition isteyebilir; FrameAvailable olayı zaten edinilmiş frame taşır ve tekrar acquisition yapmaz.
- Olay kimliği source/session/generation, event/request ID, zaman ve varsa kamera frame kimliğini içerir. Frame kökeni ve tetik ilişkisi kayda geçer; hiçbir ID yalnız UI yenilemesinden türetilmez.
- Çalışma modu reçetede saklanır: Manual, SoftwareTrigger; ileride HardwareFrame ve Continuous. İlk sürümde uygulanmamış modlar seçilebilir çalışma seçeneği olarak sunulmaz. Birden fazla tetik kaynağını aynı anda kabul etme sonraki kapsamdır.
- ResultSink source'tan bağımsızdır. Ekran sonucu her zaman gerçek terminal kayıttan gelir; TCP istek/cevap mevcut correlation kurallarını korur. İleride hardware frame sonucu OPC UA'ya gönderilebilir; aynı cycle'ı göndermek için PLC TRIGGER gerekmez.
- İlk sürüm tek in-flight yürütme korur. İleri hardware modunda sınırlı FIFO, taşma görünürlüğü ve fault/backpressure politikası zorunludur; üretim kareleri preview gibi sessizce düşürülmez. Preview latest-frame politikası ayrıdır.
- Uzun dönem 6D.2 dedup/outbox/restart recovery iptal edilmez. Yeni coordinator'a uyarlanır. Restart'ta belirsiz cycle yeniden otomatik çalıştırılmaz; önce teslim kurtarma çözülür.
- Sink arızası gerçek ölçüm sonucunu FAIL'e dönüştürmez; delivery durumu ayrı kalır. Zorunlu hedef başarısızlığında yeni cycle politikası açık olmalıdır; ilk sürüm mevcut pending teslim engelini korur.

## 5. Socket yaşam döngüsü ve cevaplar

- Transport uygulama ömrüne aittir; proje/reçete veya Runtime referansı olmadan IP/port dinleyebilir. Runtime hedefi sonradan bağlanır; proje değişiminde dangling reference oluşmaz.
- Communication: enabled, autoStart, bind address, port, ACK timeout/retry ayarları kalıcı olur. Makine endpoint ayarları mevcut yerel settings altyapısına; recipe/result mapping reçete/proje bağlamına aittir. Kalıcı kimlik tool UUID'dir, ad yalnız gösterimdir.
- Üretim config/local.ui.ini bu geliştirme/testlerde değiştirilmez; test INI'si build altında olur. Ayar yokken otomatik dinleme kapalıdır; kullanıcı kaydedip etkinleştirince sonraki açılışlarda uygulanır. AutoStart otomatik Run değildir.
- Geçersiz IP/port ayarı açık hata üretir; dinleme için recipe validation istenmez. Port doluysa görünür bağlantı hatası/retry sunulur; UI bloke edilmez veya retry fırtınası oluşturulmaz. Kalıcı port 0 üretim ayarı olarak önerilmez; testte actual port gösterilir.
- Socket açıkken Load/Run/Stop kullanılabilir. Yalnız gerçek busy/pending kayıt/teslim nedenleri ilgili işlemi engeller; salt listening engel değildir.

Yeni, geçerli bir tetik isteği için:

| Durum | Cevap |
| --- | --- |
| Yüklenmiş reçete yok | INVALID / NO_RECIPE_LOADED |
| Yüklenmiş fakat Run kapalı | INVALID / NOT_RUNNING |
| Starting veya Stopping | INVALID / NOT_READY |
| Seçili çalışma modu TCP tetiği kabul etmiyor | INVALID / TRIGGER_SOURCE_DISABLED |
| Çevrim sürüyor | BUSY; yeni acquisition yok |
| Kayıt/teslim bekliyor | INVALID / RECORDING_PENDING veya DELIVERY_PENDING |
| Runtime fault veya geçersiz dependency | INVALID / açıklayıcı reason |
| Running, uygun kaynak, hazır | Kabul → BUSY → gerçek terminal sonuç |

INVALID ölçüm NOK/FAIL değildir: resultValid=false, decision=NOT_EVALUATED, valueMm=null. Varsa requestId geri döner; cycle başlatılmadıysa uydurma cycle/frame kimliği üretilmez. Malformed JSON/protokol hatası ERROR olarak ayrı tutulur. HELLO/STATUS bağlantıyı reçetesiz de tanımlar; recipe/config yokluğu için belgelenmiş boş/null biçim seçilir.

Duplicate kontrolü yeni iş kabulünden önce gelir: daha önce kabul edilmiş aynı session/request yeniden gelirse Stop sonrası bile kayıtlı BUSY/DONE/ACK davranışı korunur, yeni görüntü alınmaz. Request conflict, yanlış session/config ve ACK kuralları korunur. INVALID yeni istek bir üretim sonucu değildir.

Wire değişikliği açık sürümlenir: vf.socket/2 önerilir; simülatör/test/belge birlikte güncellenir. Eski v1 peer sessizce yeni anlama geçirilmez; desteklenmiyorsa UNSUPPORTED_VERSION ile reddedilir. v1 korunacaksa ayrı uyumluluk testleri gerekir. Aynı session'da recipe değişiminde configId kontrolü korunur; önceki sonuç replay'i eski snapshot kimliğini taşır.

Mevcut 128 accepted request sınırı ilk geçişte açıkça korunabilir; sınırsız otomatik üretim iddiası yapılamaz. Kesintisiz çalışma kabulünden önce 6D.2 kalıcı outbox/dedup ve arşiv kapasite/retention politikası tamamlanmalıdır; cache'i sessiz temizlemek çözüm değildir.

## 6. Editor: aynı görüntüde yalnız seçili araç

### 6.1 Kullanıcı akışı

- Editor toolbar ve seçili aracın Properties alanında aynı komuta bağlı **Run Tool / Seçili Aracı Çalıştır** bulunur. Edge, Blob, Template Match ve girdileri uygun diğer işlem araçları için ortaktır.
- Threshold veya başka parametre değişince otomatik acquisition/tam flow çalışmaz. Kullanıcı Run Tool ile mevcut raw frame üzerinde yalnız seçili executor'ı tekrar çalıştırır. Zoom/pan değişmez.
- Run Flow mevcut tam akış/acquisition davranışını korur ve adı/açıklaması bunu belirtir. Görüntü edinme ayrı açık eylemdir. Acquisition source seçiliyse Run Tool yerine görüntü alma eylemi gösterilir.
- Geçerli image veya upstream output eksikse açık mesaj: “Önce görüntü al” ya da “Referans Match sonucu güncel değil; önce Match'i çalıştır.” Gizli upstream veya full-flow fallback yoktur.
- İstenirse ayrı açık “Gerekli önceki araçlarla çalıştır” komutu ileride eklenebilir; Run Tool anlamı değiştirilmez.

### 6.2 Cache ve yürütme sözleşmesi

- ExecutionService'e açık TargetOnly kapsamı veya eşdeğer API eklenir. Tek executor çağrısı aynı ToolExecutors/registry doğrulamasını kullanır; UI doğrudan algoritma çağırmaz.
- Cache anahtarı raw frame generation/content identity, source/coordinate kimliği, proje/reçete/path, tool parametre/bağlantı ve geçişli dependency kimliklerini içerir. Tek global flow hash yüzünden tüm geçerli upstream verileri gereksiz yere silinmez.
- Seçili aracın parametre değişimi kendi sonucunu ve downstream sonuçlarını geçersiz kılar; aynı frame'e ait değişmemiş upstream sonuçlar kalabilir. Upstream ayar/model/bağlantı değişirse ona bağlı cached girdiler reddedilir. Yeni frame/proje/reçete/source değişimi ilgili cache'i temizler.
- Önceki çalıştırmadan gelen girdileri yeni bir cycle'a körlemesine etiketleme. Editor evaluation ID ile acquisition/frame identity ayrı tutulur; yalnız doğrulanmış cache girdileri için yeniden kullanım kökeni korunur. Mevcut PortValue cycle/frame kontrolleri genel olarak gevşetilmez. Editor partial evaluation, Runtime tam cycle sonucu yerine kabul edilmez.
- Geçişli bağımlılık kontrolü enabled durumu, tool sürümü, ROI/referenceGeometry, kalibrasyon ve asset değişimini kapsar. Bağlı region geçersizse full image fallback yoktur.
- Partial sonuç seçili tool kaydını günceller; downstream eski değerler güncelmiş gibi kalmaz. İlgisiz dalın sonucu ancak kendi provenance'i geçerliyse korunur. “Tam akış tamamlandı” mesajı üretilmez.
- Worker çalışırken parametre/frame değişimi veya Stop geç sonucu reddeder. Raw owned buffer değiştirilmez; screenshot/preview üzerinden analiz yapılmaz. Editor testi hiçbir PLC/OPC UA üretim çıktısı göndermez.

## 7. Blob parametreleri ve kırmızı alanlar

İlk uygulanacak Properties grupları:

| Grup | Parametre / anlam |
| --- | --- |
| Giriş/ROI | image ve optional region bağlantısı, search ROI; mevcut öncelik korunur |
| Segmentasyon | Threshold 0–255; açık nesne (pixel > threshold) / koyu nesne (pixel <= threshold); 4 veya 8 connectivity |
| Alan filtresi | Min Area, Max Area, px²; dahil sınırlar; Max için açık “Sınırsız” seçeneği |
| Görünüm | Kırmızı yarı saydam alan, kontur, isteğe bağlı bbox/centroid; görünüm seçenekleri algoritma sonucunu değiştirmez |
| Sonuç | Filtre öncesi aday sayısı, kabul edilen count, en büyük kabul edilen alan, centroid ve bbox; alan birimi px² |

- Varsayılanlar eski davranışı korur: threshold=128, bright, connectivity=8, minArea=1, maxArea sınırsız. Min pozitif tamsayı, sınırlı Max >= Min olmalı; NaN/negatif/kesirli alan değerleri reddedilir. Sınırsız için formatta açık bir representation belirlenir.
- Alan connected-component piksel sayısıdır; bbox alanı veya contourArea değildir. ROI koordinatları tam raw görüntü koordinatına çevrilir. ROI sınırındaki parçalar ilk sürümde dahil kalır; border exclusion/morphology/şekil filtreleri sonraki genişletmedir.
- count/found/area/region/point yalnız kabul edilen bileşenlerden hesaplanır. En büyük eşit alan için deterministik üstten alta/soldan sağa seçim tanımlanır. Sıfır kabul: count=0, found=false, area=0; region/point invalid. Bu tek başına parça FAIL kararı değildir.
- Kırmızı alan gerçek kabul edilmiş label maskesidir; yalnız bounding box'ı kırmızı yapmak yeterli değildir. Delikler ve düzensiz şekiller korunur; reddedilen bileşenler kırmızı doldurulmaz.
- ToolResult'ta presentation-neutral mask/region verisi taşınır; renk Qt viewer'da atanır. Maske ROI-local olabilir ama origin/size açık olur. Aynı overlay yolu Editor ve Runtime'da kullanılır; raw frame'e boya yazılmaz.
- Görsel sınırda bbox/label azaltılabilir, fakat sayım/alan filtrelemesi değişmez. Bellek/overlay limitleri tanımlanır; kısmi gösterim varsa görünür bilgi verilir. Full-frame maskenin sahipliği, stale-result temizliği ve iptal test edilir; maskeyi varsayılan olarak production JSON'a büyük binary olarak yazma.
- Parametre değişince eski Blob overlay/sonucu temizlenir; yeni Run Tool başarıyla bitince yeniden çizilir. Kırmızı renk tespit edilen bölgeyi ifade eder, NOK anlamına gelmez; kısa lejant eklenir.
- Eski Blob kayıtlarına yeni alanlar migration/default normalization ile eklenir. Aç/no-op dirty yapmaz, ilk gerçek değişiklik güncel şemayı yazar. Blob tool/engine semantic version gerektiğinde artırılır; eski Runtime validation izinleri yeniden kullanılamaz. Gelecek şema reddi korunur.

## 8. İleri aşama: hardware trigger, OPC UA, QR

Bu işler ayrı kabul kapılarıdır; ilk UI sadeleştirmesini bekletmez.

1. **HardwareFrame:** gerçek kamera adapter'ı TriggerMode/TriggerSource/aktivasyon ayarlarını desteklediğini keşfeder; Run stream'i hazırlar, sensör pozlamayı tetikler, owned FrameAvailable ortak coordinator'ı besler. Ekranın repaint/görüntü değişim sinyali üretim tetiği değildir. Stop/reconnect generation, duplicate/late frame, kuyruk taşması, cihaz kimliği ve fiziksel test gerekir.
2. **OPC UA result sink:** hedef sistemle client/server rolü, endpoint/security/certificate, namespace/node ID ve veri tipleri belirlenir. CycleId, ResultValid, Decision, ölçüm/QR alanları ve completion/handshake birlikte tasarlanır. Alanların ayrı yazılmasında karışık cycle okunmasını engelleyen sequence/commit sözleşmesi gerekir. OK/NOK yalnız açık decision mapping'ten gelir; ERROR/NOT_EVALUATED üçüncü durumdur. SDK/lisans seçimi bu aşamanın teknik keşfidir.
3. **QR tool:** string/decoded-items port tipi, decoder ve ROI/çoklu kod/okunamadı politikası registry/executor içine eklenir. Ekranda gerçek decoded text görünür; okunamadı durumu boş/geçersiz çıktı olur, önceki QR kalmaz. QR okunması kendiliğinden OK değildir; karar kuralı ayrı tanımlanır. ResultSink aynı kaydı OPC UA/ekrana yönlendirebilir.
4. **Continuous:** kaynak destekliyorsa sınırlı acquisition/işleme döngüsü, hız/kuyruk/backpressure ve Stop politikası eklenir. Tek request adapter'ını zamanlayıcıyla sınırsız çağırmak kabul değildir.

Hedef senaryo: Run → sensör → kamerada yeni frame → Edge/Blob/Match/Measurement veya QR → kalıcı sonuç → ekran ve seçilen sink. Her cycle için robot/PLC komutu gerekmez.

## 9. Sol 6.1 medium için iş paketleri

Paketler sıralı uygulanır; her paket sonunda progress, ilgili README/mimari ve kabul kanıtı güncellenir. Mevcut dirty/untracked çalışmalar korunur; reset/temizleme/yeniden yazım yapılmaz.

| Paket | Uygulama | Kabul / bitiş koşulu |
| --- | --- | --- |
| P1 — tamamlandı | Runtime durum/coordinator; Load ile statik hazırlık, Run ile otomatik hazırlık; frame kontrolünün ayrıştırılması | Servis testlerinde yüklenmiş snapshot/draft ayrımı, Run/Stop/geç callback, başarısız başlangıç, dependency ve kayıt engelleri geçti. UI bağlantısı P2’de tamamlandı; socket P3’te |
| P2 — tamamlandı | Tek tık, Load Recipe, Edit Recipe, sade Runtime, dirty seçimi | Kaydedilmiş kopya→Load→Run, dirty üç seçenek, save conflict, missing asset/failed Load, bağımsız kimlikler, gerçek mm/Stop/kayıt retry ve normal/850×600 UI doğrulandı; socket entegrasyonu P3 |
| P3 — tamamlandı | Socket transport ömrü, settings/autoStart, runtime binding, v2 INVALID ve simülatör | Projesiz açılışta dinler; Load/Run/Stop bağlantıyı bozmaz; invalid/busy/duplicate/ACK/reconnect ve mapping testleri |
| P4 — tamamlandı | Editor TargetOnly API, geçerli dependency cache, Run Tool UI | Threshold değişiminden sonra acquisition ve upstream executor sayaçları sıfır; yalnız hedef executor bir kez; stale girdiler reddedilir |
| P5 — tamamlandı | Blob filtre/polarity/connectivity, migration, mask/overlay/Properties | Analitik piksel alanları ve eşik sınırları, delikli blob kırmızı maske, ROI offset, sıfır kabul, save/reopen ve aynı frame tekrar |
| P6 — tamamlandı | Birleşik regression ve doküman kapanışı | UI Release + tüm ilgili CTest, Widgets OFF çekirdek, normal ve 850×600 görsel kontrol; mevcut Runtime/Measurement/kalibrasyon testleri geçer |
| P7 | 6D.2 kalıcı dedup/outbox/restart, arşiv kapasite politikası | Restart/yarım kayıt/teslim belirsizliği ve 128 istek sonrası sürekli çalışma kanıtı; yeniden acquisition ile retry yapılmaz |
| P8 | Hardware trigger kamera adapter'ı ve fiziksel kabul | Sensörle gerçek frame, tek cycle, kuyruk/Stop/disconnect testi; donanım yoksa yazılım fixture kabulü ayrı raporlanır |
| P9 | OPC UA sink, QR tool ve gerekirse Continuous | Gerçek hedefle tip/handshake/delivery ve QR verisi doğrulanır; hazırlık ekranı özellik sayılmaz |

P1–P6 yakın geliştirme kapsamıdır. P7 mevcut 6D.2'nin devamıdır. P8–P9 gelecekteki genişleme kapsamıdır; cihaz/endpoint/SDK bilgisi gerektiren kararlar bu aşamada çözülür. K4 fiziksel kalibrasyon kabulü ertelenmiş kalır; bu plan onu tamamlamaz. Robot 6E ayrı geliştirme olarak korunur.

## 10. Zorunlu doğrulama matrisi

- Reçetesiz açılış, socket enabled/disabled/port hatası, settings restart; test dosyaları yalnız build altında.
- A seçili/B yüklü/C editörde durumu doğru gösterilir; dirty yükleme üç seçeneği, save hatası, failed Load, busy switch korunur.
- Run yokken INVALID; Run ile doğru trigger mode; Stop sonrası yeni istek INVALID ama eski duplicate sonuç/ACK replay korunur.
- Yeni istek/aynı ID conflict/yanlış session/config; connection kopması cycle'ı çoğaltmaz; kayıt veya ACK commit başarısızlığı sonucu sahte Ready yapmaz.
- TargetOnly için executor/acquisition sayaçları; aynı frame ve raw pixel checksum; parametre sonrası yalnız hedef çalışır. Upstream config/frame/reference/kalibrasyon değişince cache reddedilir. Disabled/missing giriş ve Stop stale-result test edilir.
- Blob threshold eşitliği, bright/dark, 4/8 diagonal, Min/Max dahil sınırlar, sınırsız Max, tie-break, ROI offset, boş sonuç, delikler ve >2048 bileşen; veri sayımı overlay sınırından etkilenmez.
- Eski proje aç/no-op/save/reopen/future schema; engine/tool sürüm değişimi snapshot güvenliğini korur.
- UI smoke: Load/Run/Stop, Properties alan birimleri, kırmızı maske/zoom/pan ve küçük pencere; hiçbir gerçek sonuç yokken PASS/skor/pose gösterilmez.
- README'deki yerel toolchain ile Release ve ilgili CTest; P6'da tam UI/core suite. Bu plan oturumunda build çalıştırılması gerekmez çünkü uygulama değişmedi.

## 11. Kodlama oturumuna verilecek başlangıç talimatı

“Önce AGENTS.md, docs/progress.md, README.md ve docs/architecture/runtime-editor-workflow-plan.md dosyalarını oku. Bu planın P1–P6 paketlerini sırasıyla uygula; P7–P9'u uygulanmış gösterme. Kullanıcının onayladığı Load Recipe → Run akışını, bağımsız otomatik socket'i, aynı raw görüntüde yalnız seçili araç yürütmesini ve Blob alan filtreleri/kırmızı gerçek bölge gösterimini tamamla. Var olan değişiklikleri koru. Her paket için anlamlı testleri, UI değişikliklerinde Release/CTest/görsel kontrolü yap ve progress'e kanıtları kaydet. Mevcut mimari/kalibrasyon/immutable snapshot/kalıcı kayıt güvencelerini koru. Doğrulama düğmesini kaldırmak servis kontrollerini kaldırmak değildir. Donanım/OPC UA/QR ileri aşama kapsamını ayrı bırak.”

## 8 Ekim — Operatör görünümü ve Setup revizyonu

Run başarılı olduğunda Runtime tam ekran olur; sol navigasyon ve File/View/Setup gizlenir ve devre dışı kalır. Sayfa geçişi merkezi guard ile reddedilir. Stop sonrası worker/kayıt durdurma tamamlanınca önceki pencere durumuna dönülür; Runtime Stop ve kayıt retry erişilebilir kalır. Bu bir OS kiosk veya rol/parola sistemi değildir. Communication ve Calibration sol menüden kaldırıldı; File/View yanındaki Setup içinden açılır.

Communication: “TCP iletişimini etkinleştir” işaretlenmeden Dinlemeyi başlat pasiftir. Bu gerekçe ekranda ve tooltip içinde görünür. Ayarları kaydet etkinleştirme veya Run yapmaz; etkinleştir → kaydet → dinlemeyi başlat kullanılır. Otomatik dinleme yalnız uygulama açılışına aittir.

Sonraki geliştirme önerisi (henüz uygulanmadı): önce manuel px/mm ve kalibrasyon ekranını akışa göre sadeleştirme, ardından P7/6D.2 kalıcı dedup/outbox/restart ve kapasite politikası. P8 hardware, P9 OPC UA/QR ve 6E robot daha sonra; K4 fiziksel kabul ertelenmiş kalır. Kullanıcıyla sıra yeniden değerlendirilecek.
## 8 Ekim — Manuel ölçek uygulama planı hazır

[Manuel px/mm ve sade Calibration planı](manual-scale-calibration-plan.md) önceki kısa kapsam notunun ayrıntılı devamıdır. C1 matematik/codec → C2 katalog/Measurement/Runtime → C3 sade ekran → C4 UI kabul → C5 birleşik test/belgeler. C1–C5 tamamlandı: matematik/codec, katalog/reçete/Runtime/CSV, sade üç yöntem ekranı ve birleşik yazılım kabulü mevcut. Yeni manual_planar_scale/1, record/dependency schema 3, eski hash/encode koruması, beş bölümlü üç yöntem UI ve doğrulamasız deneme/üretim ayrımı tanımlandı. Belgenin 7. bölümü GPT-6.1 Sol medium için kopyalanabilir başlangıç talimatıdır. P7 bu kapsamdan sonra; K4 ertelenmiş kalır.
