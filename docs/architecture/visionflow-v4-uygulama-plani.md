# VisionFlow V4 · Kullanıcı kurulumu ve otomatik çalışma planı

**K0 ürün kapsamı · 2 Kasım 2026.** İlk sürümde her istasyonun bir kalıcı kamera eşleşmesi ve o kamerayı temsil eden bir dosya kaynağı vardır. İlk kabul İstasyon 1 ve yüzey kontrolüdür. İkinci kabul aynı TCP bağlantısında İstasyon 2 ile ölçüm, reçete değişimi ve teslim kurtarmadır. Bu belge uygulama ve test planıdır.

## 1. Operatörün tek sayfalık akışı

| Durum | Ekran → düğme → seçim | Beklenen sonuç | Kart |
|---|---|---|---|
| Bugün var | Communication → Listener profili → yerel adres ve port → Kaydet → uygulamayı yeniden aç | Aynı bilgisayardaki testte 127.0.0.1:5000 dinlenir. İstasyonlar henüz kurulmamış olabilir. | VF-01 |
| Yeni | Communication → Makine kurulumu → Kurulumu başlat | Kalıcı kurulum kimliği ve iki varsayılan sonuç şablonu, **Yüzey** ve **Ölçüm**, kullanılabilir olur. | VF-01 |
| Yeni | Communication → İstasyonlar → İstasyon ekle → ad ver → Kamera kaynağı bağla → Kamera 1 → Dosyadan test görüntüsü seç → Kaydet | İstasyon 1 adı, kamera ve kaynak dosyasıyla listelenir. Yeniden açılışta eşleşme korunur. | VF-02 |
| Değişecek | Recipe Editor → Yüzey A → Publish Data → Yüzey → Kaydet → İstasyonda kullanım için yayımla | Yayımlanmış değişmez revizyon Dashboard'da seçilebilir. Mevcut istasyon seçimi etkilenmez. | VF-03 |
| Yeni | Dashboard → İstasyonlar → İstasyon 1 → Reçete yükle → Yüzey A | Uyumlu reçete yüklenir. Satırda Kamera 1, Yüzey A ve Durduruldu görünür. | VF-04 |
| Yeni | Communication → İstemciler → Simülatör istemcisi kaydet → Simülatör testi → Test profilini dışa aktar | İstemci profili oluşturulur. Kullanıcı UUID üretmez. Profil dışa aktarımı gerçek PLC veya kamera kurulumu için şart değildir. | VF-05 |
| Yeni | Dashboard → Otomatik üretimi başlat | Önkoşullar geçerse üretime dahil istasyonlarda Run ve Auto açılır. Eksik varsa başlamadan istasyon, neden ve düzeltme düğmesi gösterilir. | VF-06 |
| Değişecek | V4 simülatörü → Profili aç → Bağlan → İstasyon 1 → Tetikle | Beklenen yüzey kararı ve geçerlilik görünür. Teslim exact ACK ile kapanır. VF07-A sonrası operatör K2 tekrarı burada durur. | VF-07 / VF07-A |
| Yeni | Aynı ekranlardan İstasyon 2 → Kamera 2 → Ölçüm A → Ölçüm şablonu → sayısal çıktıyı seç → yayımla ve yükle | Aynı TCP bağlantısında İstasyon 2 tetiklenir. Beklenen ölçüm doğru birim ve ölçekle gelir. | VF-08 |
| Değişecek | Dashboard → Stop (tüm istasyonlar) → İstasyon 1 → Reçete değiştir → Yüzey B | Durdurma ve bekleyen teslim tamamlanınca uyumlu B yeni activation ile yüklenir. Ret önceki seçimi korur. | VF-04 / VF07-A / VF-08 |
| Değişecek | Uygulamayı kapat → yeniden aç → Dashboard → İstasyonlar | Son seçimler geri gelir. Run, Auto ve Remote Enable kapalıdır. Bekleyen terminal varsa teslim kurtarılır. | VF-02/06/08 |

## 2. Uygulamayı bağlayan kararlar

- **Sahiplik.** İstasyonun kalıcı kimliği, adı ve kamera eşleşmesi makineye aittir. Dosya seçimi bu kameranın yazılım testi kaynağıdır. Kaynak aracı kimliği farklı olan uyumlu reçeteler aynı istasyon kamerasını kullanır. Eski seçimlerden kaynak ilişkisi kesin çıkarılamıyorsa kullanıcıdan bir kez eşleme istenir.
- **Reçetenin üç durumu.** Editörde bir çalışma kopyası, makine kataloğunda yayımlanmış değişmez bir revizyon, istasyonda açıkça yüklenmiş bir seçim bulunur. Bir reçeteyi açma, kaydetme veya yayımlama çalıştırılan seçimi değiştirmez. Yükleme sırasında görüntü sayısı, tür, gerekli setup/kalibrasyon ve çıktı sözleşmesi doğrulanır. Başarısız yüklemede önceki activation da korunur.
- **İki hazır şablon.** Yüzey için terminal kararı ve sonuç geçerliliği yeterlidir. Ek kusur bayrağı yalnız gerçek Boolean çıktı varsa kullanılır. Ölçüm için Number[0] zorunludur. Kullanıcı ölçüm aracını/çevrim değişkenini ve uyumlu birimi seçer. Ölçek hem önizlemede hem simülatörde gösterilir. Piksel çıktısı kalibrasyon olmadan mm olarak sunulmaz.
- **Dashboard.** Reçete başlığı yerine İstasyonlar bölümü gelir. Satırda istasyon, bağlı kamera, yüklü reçete/revizyon, çalışma durumu, teslim durumu ve yükleme/değiştirme işlemi bulunur. Listener durumu makine düzeyindedir. İstasyon hazırlanıyor, Auto açık, çevrim çalışıyor ve ACK bekliyor birbirinden ayırt edilir.
- **Tek düğmeyle Auto.** İlk sürümün iki istasyonlu kabulünde kurulan iki istasyon da üretime dahildir. Ön kontrolde biri hazır değilse hiçbiri başlatılmaz. Başlatma sırasında hata oluşursa bu işlemle başlatılanlar durdurulur, geçiş tamamlanana kadar gerçek durumları gösterilir. Önceden çalışan istasyonların durumu ayrı izlenir. Yeniden açılışta otomatik üretim başlamaz.
- **Yetkiler.** Operatörün tek düğmesi Run/Auto içindir. Uzak reçete seçimi ve uzak çalışma modu yönetimi için Remote Enable ayrıca gerekir. Yerelden Auto verilmiş istasyona kayıtlı PLC'nin normal tetiği için ayrıca Remote Enable gerekmez. Mevcut davranış gereği Remote Enable kapatılınca bağlı istasyonlara Stop verildiği ekranda belirtilir.
- **Geçmiş.** Katalog anahtarları yeniden kullanılmaz. Eski yayımlar, activation geçmişi, pending kayıtlar ve mühürlenmiş sonuçlar korunur. Bozuk kayıt ile ilk kurulum arayüzde ayrılır. Geçmiş kayıtlar güncel reçeteye göre yeniden yorumlanmaz.

## 3. Teknik başlangıç ve bağımlılıklar

Ön incelemede normal başlatıcı D:/TVision/VisionFlow.cmd üzerinden build/reference-ui/visionflow.exe idi. Makine dizininde listener profili ve kapanış kaydı vardı. İstasyon kimlik kataloğu ve publication-templates-v4 dizini yoktu. Mevcut panelin reçete yükleme düğmesi kayıtlı istasyon ve yayımlanmış reçete gerektiriyor. Publish Data şablonları makineden okuyor. Bugünkü station tanımı kimlik, anahtar ve ad taşırken kamera eşleşmeleri reçete seçimiyle saklanıyor. Mevcut dosya yolu reçete aracından yürütmeye ulaşıyor. Bu nedenle yalnız tabloya kamera adı eklemek yeterli değildir.

Sıra **VF-01 → VF-02 → K1 → VF-03 → VF-04 → VF-05 → VF-06 → VF-07 → VF07-A → K2 operatör tekrarı → VF-08 → K3**. VF-03 ve VF-05'in veri önkoşulu VF-01'dir. İş sırası ilk istasyon gösterimini bölmeden ilerlemek için seridir. Normal ürün yolu kurulum verisini kendisi üretir. Servis fixture'ları bu önkoşulları karşılamaz.

## 4. Uygulama kartları

### VF-01 · Makine kurulumu ve hazır şablonlar
- **Başlangıç:** Boş uygulama verisi veya yalnız kaydedilmiş listener profili. Kullanıcı Communication ekranında.
- **İşlem:** İlk kurulum eylemi kimlik kataloğunu ve iki varsayılan şablonu hazırlar. Mevcut kurulum açıldığında durum denetlenir.
- **Modüller:** CommunicationPanel, CommunicationWorkspace, uygulama başlangıcı, StationIdentityRegistry, kimlik ve şablon depoları. Birbiriyle ilişkili katalog ve manifest kayıtları için kesintiden sürdürülebilir kayıt sırası tasarla. İki şablon yalnız bir kez oluşturulsun. UI ve Runtime aynı kayıtları görsün.
- **Koruma:** Var olan listener, kurulum kimliği, makine verileri, eski şablonlar ve geçmiş.
- **Olumsuz:** Çift tıklama, iki işlem çakışması, yazma kesintisi ve bozuk katalog. Onarım seçeneği veri üzerine yazmadan nedeni gösterir.
- **Kanıt/devir:** Hedefli temiz kurulum ve yeniden deneme testleri. Normal uygulamada iki şablonun Publish Data'da görünmesi, ikinci açılışta aynı kimlik ve anahtarların korunması. VF-02 ve VF-03 bu kurulumu tüketir.

### VF-02 · Kalıcı istasyon ve kamera eşleşmesi
- **Başlangıç:** VF-01'in oluşturduğu makine, boş istasyon listesi.
- **İşlem:** İstasyon oluştur, adlandır, mantıksal kamera ve dosya bağla, görüntüyü doğrula, kaydet, yeniden aç.
- **Modüller:** StationDefinition, makine persistence katmanı, CommunicationStationComposition ve CommunicationWorkspace. İstasyonun kaynak sürümünü ve dosya kaynağını reçete seçiminden ayrı sakla. Katalog yazıldıktan sonra panel/Runtime bileşiminin yeni istasyonu ne zaman göreceğini açıkça yönet. Yeniden başlatma gereken durumda bunu kullanıcıya göster.
- **Koruma:** Yeniden adlandırma ve reçete oluşturma kamera kimliğini değiştirmez. Mevcut activation kayıtları okunmaya devam eder.
- **Olumsuz:** Okunamayan dosya, kayıtta kesinti, belirsiz eski eşleşme. Geçerli kaydın üzerine yarım eşleşme yazılmaz. Kaynak sonradan kaybolursa satır doğrudan kaynak ayarına götürür.
- **Kanıt/devir:** Kalıcılık ve güncelleme testleri. Normal uygulamayı kapat/aç gösteriminde aynı istasyon, kamera ve dosya, çalışma izinleri kapalı. **K1.** Görüntünün B reçetesiyle işlendiğinin kanıtı VF-08'e aittir.

### VF-03 · Çıktı bağlama ve değişmez reçete yayımlama
- **Başlangıç:** VF-01 şablonları ve kullanıcının mevcut projeden açtığı veya editörde oluşturduğu reçete.
- **İşlem:** Şablon seç, gerekli araç çıktısını/çevrim değişkenini bağla, birim ve ölçeği önizle, kaydet ve yayımla.
- **Modüller:** PublishDataEditor, PublicationTemplate, FixedSlotPublication, proje kaydetme ve makine manifest deposu. Geçerli kayıtlı reçeteden değişmez revizyon üret. UI ve istasyon kompozisyonunun yayımlanmış revizyonları migration paketi olmadan keşfetmesini sağla.
- **Koruma:** Taslak, önceki yayımlar, yüklü revizyon ve eski sonuçlar korunur. Şablonu değiştiren işlem yeni revizyon üretir.
- **Olumsuz:** Yanlış tip/birim, eksik ölçüm slotu, kaydedilmemiş değişiklik, kısmi manifest yazımı. Geçersiz aday yüklenebilir listeye girmez.
- **Kanıt/devir:** Tip, revizyon ve kesintili yayımlama testleri. Normal editörden yayımlanan A'nın yüklenebilir listede görünmesi. VF-04 bu ürünü kullanır.

### VF-04 · Dashboard seçimi ve gerçek kaynakla yürütme
- **Başlangıç:** VF-02 istasyonu, VF-03 yayımlanmış A reçetesi.
- **İşlem:** Dashboard'dan A'yı yükle. Stop ve teslim tamamlandıktan sonra B'ye geç. Seçimden önce kamera, uyumluluk ve hedef revizyon gösterilir.
- **Modüller:** Dashboard, StationActivationService, StationControlService, StationRuntimeBoundary, StationExecutionRegistry, CommunicationStationComposition. Mevcut yapı aynı recipeKey altında kamera listesi dahil aynı seçim baytlarını bekliyor. Yayımlanmış reçete içeriğini istasyona özgü çözülmüş eşlemeden ayır. Sürümlü okuma ve geçiş tasarla. Admission, yürütme ve sonuç aynı çözülmüş snapshot'ı kullansın. Kamera değişikliğini yalnız UI veya geçici dosya yolu üzerinde yapma.
- **Koruma:** Kaynak araç UUID'si değişse bile uyumlu B, aynı Kamera 1'i kullanır. Yerel ve yetkili uzak seçim aynı doğrulamadan geçer. Başarıda yeni activation, Auto kapalı. Başarısızlıkta A ve activation korunur.
- **Olumsuz:** Uyumsuz kaynak, eksik kalibrasyon, çalışan çevrim, bekleyen sonuç ve eski activation. Eşzamanlı iki seçimde kaybeden işlem önceki/yeni geçerli durumu ezmez.
- **Kanıt/devir:** Kaynak çözümleme, yürütmeye aktarılan snapshot, uyumluluk ve seçim yarış testleri. Dashboard'da açık yükleme gösterimi. TCP üzerinden B'nin doğru görüntüyü işlemesi VF-08'de ayrıca doğrulanır.

### VF-05 · İstemci kaydı ve test profili
- **Başlangıç:** VF-01 makinesi. Kayıtlı bir PLC bulunabileceği dikkate alınır.
- **İşlem:** Yetkili istemci kaydet. Simülatör testi için profil dışa aktar. Gerçek PLC'ye kurulum bilgilerinin nasıl aktarılacağı ayrı gösterilir.
- **Modüller:** CommunicationPanel/Workspace, StationIdentityRegistry, CommunicationPeerSessionService ve simülatör profil okuyucusu.
- **Koruma:** Tek etkin kontrol kimliği, kurulum kimliği, peer/epoch ve bekleyen teslimler. Profil bağlantı bilgilerini taşır. Aktif seçim alanları oturumda güncel katalogdan okunur.
- **Olumsuz:** İkinci etkin istemci, yanlış kurulum profili veya bekleyen teslimle epoch değiştirme. Kullanıcıya hangi işlem gerektiği gösterilir.
- **Kanıt/devir:** Kimlik ve geçiş testleri. Normal uygulamadan üretilen profille bağımsız Python istemcisinin başarılı HELLO kaydı. VF-07 gerçek TCP istemcisini buradan alır.

### VF-06 · Tek eylemle otomatik çalışma
- **Başlangıç:** VF-04'te yüklenmiş istasyon. Operatör Dashboard'da.
- **İşlem:** Otomatik üretimi başlat. Kaynak, seçili revision, yayın, pending teslim ve çalışma hazırlığı ön kontrolden geçer. Durum satırları gerçek Run/Auto sonucuyla güncellenir.
- **Modüller:** Dashboard, CommunicationWorkspace, StationRunController ve kontrol servisi. Kontroller geçtikten sonra istasyonlar sırayla hazırlanır. Geçiş boyunca değişen durumlar yeniden denetlenir.
- **Koruma:** Remote Enable bağımsızdır. Stop, pending sonucu silmez. Uygulama açılışında üretim izni devralınmaz.
- **Olumsuz:** Ön kontrolde eksik bulunan istasyon, ikinci istasyon hazırlanırken hata, art arda tıklama veya geç gelen hazırlık cevabı. Kullanıcı her istasyonda gerçek durumu ve düzeltme yolunu görür.
- **Kanıt/devir:** Toplu başlatma, hata sırasında durdurma ve yeniden açılış testleri. Dashboard'da nominal yol ile eksik kaynak yolu gösterilir. İki istasyonlu ürün gösterimi VF-08'de tamamlanır.

### VF-07 · Tek istasyonun normal uygulamada TCP kabulü
- **Başlangıç:** VF-01–06 aracılığıyla hazırlanmış İstasyon 1, Kamera 1, Yüzey A, test istemcisi ve Auto izni.
- **İşlem:** Simülatörde profili aç, bağlantı kur, istasyonu seç, Tetikle. Karar ve teslim durumunu oku.
- **Modüller:** scripts/communication_v4_simulator.py ve ürün testi. Bağımsız codec ve digest kontrolü korunur. Katalog keşfi, kalıcı request sayacı, istasyon + request üzerinden yanıt eşleme, geçerlilik ve durum gösterimi eklenir. Eski v2/v3 executable'ı bu akışın dışında kalır.
- **Koruma:** ACCEPTED çevrim kabulünü gösterir. Teslim ancak doğru terminal için ACKED alındığında kapanır.
- **Olumsuz:** Yanlış digest, eski seçim, timeout ve kopma. Otomatik yeni üretim isteği gönderilmez.
- **Kanıt/devir:** Hedefli protokol testleri. Normal başlatıcı + bağımsız simülatörle, ekrandan hazırlanan istasyonda beklenen yüzey kararı ve exact ACK. Operatör adımları kendisi tekrar edebilir. **K2 geçmeden ikinci istasyon kabulüne geçilmez.**

### VF07-A · K2 operatör akışı düzeltmeleri

- **Durum:** Analiz/plan hazır; uygulama başlamadı. [Detaylı görev kartı](communication-v4/cards/VF07-A.md).
- **Başlangıç:** VF-07 yazılım kanıtı korunur. Operatör ekranında Blob DONE/found=true iken Result seçimiyle NOT_EVALUATED/Acquire hatası; belirsiz yayın listesi ve Stop gerektiren reçete değişimi var.
- **İşlem:** Result/çevrim önbelleğini Viewer seçiminden ayır; Publish Data'yı V4 şablon/slotlarına indir; Communication Stop/drain'i Dashboard'a **Stop** adıyla taşı. Stop tüm bağlı istasyonları kapsar. Yayın tarihini, son/yüklü etiketlerini göster; uyumluluk ile yüklemeye hazır durumunu ayır.
- **Koruma:** Gerçek girdi değişiminde invalidation, legacy kayıt roundtrip'i, immutable revision/activation ve pending exact ACK korunur. Stop sırasında gerçek geçiş gösterilir; teslim silinmez, Auto yeniden açılmaz.
- **Kanıt/devir:** Karttaki A–E matrisi, UI normal/küçük ekran, son kaynakta tam Release ON/OFF ve normal ürün K2 operatör tekrarı. **VF07-A kapanmadan ve K2 operatör teyidi olmadan VF-08 başlamaz.**

### VF-08 · İki istasyon, yeniden seçim ve kurtarma
- **Başlangıç:** Kapanmış VF07-A ve operatörce tekrar edilmiş K2. İkinci istasyon, Ölçüm A ve alternatif Yüzey B normal ürün işlemleriyle hazırlanır.
- **İşlem:** Aynı TCP oturumunda iki istasyonu ayrı tetikle. A→B geçişini, eski alanlarla isteği, kopmayı ve uygulama açılışını göster.
- **Modüller:** Dashboard, seçim ve çalışma servisleri, V4 istemci, StationDeliveryLedger ve normal uygulama kabul düzeni. Mevcut kayıt formatı ve exact ACK davranışından yararlan.
- **Koruma:** Diğer istasyonun seçimi, kamera, kayıt geçmişi ve mühürlenmiş terminal aynı kalır.
- **Olumsuz:** Aşağıdaki kabul matrisinin tüm satırları. Hatalı komutun ardından yalnız hata kodu değil, önceki seçim ve çevrim sayısı da kontrol edilir.
- **Kanıt:** İzole temiz kurulumdan tekrar edilebilir normal uygulama gösterimi, negatif testler, UI görsel kontrolleri ve tam Release ON/OFF regresyonu. **K3.**

## 5. PLC ve simülatör sözleşmesi

| İşlem | Gerçek istemci sorumluluğu |
|---|---|
| HELLO ve keşif | Kayıtlı peer UUID, kalıcı epoch UUID ve exact sözleşme revizyonuyla HELLO yap. Sunucunun verdiği yeni session'ı kullan. CATALOG_GET ile gerçek stationKey, aktif recipeRevisionKey, templateRevisionKey, activation ve cameraSetRevision al. Görünen istasyon numarasını wire anahtarı varsayma. STATUS üzerinden ready/busy/pending durumunu kontrol et. |
| TRIGGER (10) | 512 baytlık istek başlığında session, peer, epoch, stationKey, sıfır olmayan kalıcı uint64 requestId, iki revision anahtarı ve activation taşınır. Gövdede expectedCameraSetRevision ile inputSnapshot ve inputRevision vardır. Harici girdi bulunmayan bu örnekte son iki UUID sıfırdır. Ayrılmış alanlar sıfır kalır. Normal tetik SELECT_RECIPE gerektirmez. |
| Sonuç | ACCEPTED (110) ile TERMINAL (111) ayrı izlenir. Terminalde istasyon, isteğin kimliği, revizyonlar, karar, geçerlilik, gerekli slotlar ve digest doğrulanır. Ölçümün geçerlilik biti kapalıysa sayısal alan başarılı ölçüm sayılmaz. |
| Exact ACK | ACK (11) terminaldeki stationKey, requestId, revizyon anahtarları, activation, result UUID ve digest ile gönderilir. Ancak ACKED (112) sonrası teslim tamamlandı gösterilir. |
| Uzak seçim | Remote Enable ve uygun durmuş istasyon gerekir. SELECT_RECIPE güncel seçimi başlıkta, hedef reçete/şablon anahtarlarını gövdede taşır. Başarı terminalindeki yeni activation tanınır ve kontrol terminali ACK'lenir. Sonraki tetikler yeni seçimi kullanır. |
| Eski bilgi veya kopma | Eski activation/revision ile yeni tetik kabul edilmez. Güncel seçim okunup operatöre gösterilir. Önceden kabul edilmiş aynı isteğin sonucu kalıcı kayıttan kurtarılır. Aynı requestId ve farklı içerik çakışma sayılır. Belirsiz sonucu çözmek için yeni tetik üretilmez. |

## 6. Geçiş kapıları ve son kabul

| Kapı | Gösterim | Geçiş koşulu |
|---|---|---|
| K0 · Ürün ve kart incelemesi | Boş makineden iki terminale kadar bütün adımlar, sahipleri ve önkoşulları bulunur. | Açık kullanıcı davranışı varsa ilgili karta geçmeden plana işlenir. |
| K1 · İlk kalıcı kurulum | VF-01/02 ile normal uygulamada oluşturulan istasyon, kamera ve dosya kapanıp açıldıktan sonra geri gelir. | Elle katalog oluşturulmamıştır. Çalışma izinleri kapalıdır. |
| K2 · İlk gerçek çevrim (VF07-A sonrası operatör tekrarı) | Kullanıcı normal uygulamada istasyonu kurar, A'yı yayımlar/yükler, istemciyi kaydeder, Auto açar ve beklenen yüzey sonucunu alır. | Beklenen içerik ve exact ACK kaydı görülür. Eksik ürün işlemi fixture ile geçilemez. VF07-A Result/Publish Data/Stop akışı kapanır; operatör teyidi ayrıca kaydedilir. |
| K3 · Tam kabul | İki istasyon ve aşağıdaki matris izole temiz kurulumda tekrarlanır. | Hedefli testler, normal uygulama kanıtları ve son kaynakta tam Release ON/OFF regresyonu geçmiştir. |

| Senaryo | Ölçülebilir beklenen sonuç | Sorumlu |
|---|---|---|
| İki ayrı görüntü, tek soket | Önceden seçilmiş yüzey örneklerinde beklenen OK/NOK, ölçüm örneğinde beklenen değer, birim ve önceden belirlenen tolerans elde edilir. Her terminal doğru istasyon, kamera, reçete ve istekle eşleşir. | VF-07/08 |
| Editörde B | B açılır, kaydedilir ve yayımlanır. Sonraki tetik açık yükleme yapılana kadar A revizyonuyla çalışır. | VF-03/04/08 |
| Uyumlu B | B'nin kaynak aracı UUID'si A'dan farklıdır. B'nin testte kendi başına seçeceği görüntü ile Kamera 1 görüntüsü ayırt edilebilir olmalıdır. B yüklendikten sonra Kamera 1 görüntüsüne özgü beklenen sonuç alınır. | VF-04/08 |
| Uyumsuz B | Yükleme gerekçesiyle reddedilir. Önceki kamera, revizyon ve activation aynıdır. İstasyon diğer istasyonun kamerasına geçmez. | VF-04/08 |
| Bayat tetik | Eski recipe/template revision, activation ve kamera kümesi revizyonu ayrı ayrı sınanır. Yeni çevrim başlamaz. Kullanıcı değişen seçimi görür. | VF-07/08 |
| Uzak kontrol | Remote Enable kapalıyken uzak seçim reddedilir. Yetki açıkken kontrol terminali yeni seçimle ACK'lenir. Değişiklik diğer istasyonun reçetesini etkilemez. | VF-04/05/08 |
| Yanlış ACK | Yanlış istasyon, result UUID veya digest pending kaydı kapatmaz. Ardından doğru ACK aynı teslimi kapatır. | VF-07/08 |
| TCP kopması ve yeniden açılış | ACK öncesi TCP kopması ve uygulama kapanması ayrı sınanır. Yeni HELLO oturumunda aynı result UUID/digest kurtarılır. Tek üretim çevrimi vardır. Kamera ve son reçete geri gelir, çalışma izinleri kapalıdır. | VF-02/06/07/08 |
| Veri uyumluluğu | Eski makine/aktivasyon kayıtları, mevcut proje reçeteleri ve sonuç geçmişi açılabilir. Önceki terminal yeniden hesaplanmaz. Bozuk katalog sessizce sıfırlanmaz. | VF-01/02/04/08 |

## 7. Kanıt, devir ve saha sınırı

Her kartın kapanışında üç ayrı sonuç kaydedilir: **uygulandı**, **hedefli test geçti**, **normal kullanıcı akışında doğrulandı**. Her birine ayrı kanıt yolu, kaynak sürümü, başlangıç verisi, beklenen ve gerçekleşen sonuç eklenir. Ürün gösterimi sonraki karta aitse ilgili kabul satırı açıkça oraya devredilir. UI değişiklikleri normal ve küçük ekranda doğrulanır. Depo çalışma protokolündeki hedefli testler ve tam Release ON/OFF kontrolleri ilgili kod değişikliklerinin kapanışında uygulanır.

K1–K3 normal başlatma ve izole kullanıcı verisiyle yapılır. Kullanıcının gerçek makine kayıtları üzerinde temizleme yapılmaz. Test görüntüleri ve beklenen karar/ölçüm önceden seçilir. Servis fixture'ları katman testlerinde kalır. Kullanıcıya ek kurulum yükü, kamera sahipliği veya izin davranışı getiren değişiklik önce K0 ve ilgili kabul satırlarına işlenir.

K3 dosyadan görüntü, gerçek uygulama, TCP, sonuç ve teslim yolunun yazılım kabulüdür. Gerçek kameranın kimlik ve SDK bağlantısı, fiziksel tetik zamanlaması, saha PLC'sinin wire uygulaması ve gerçek donanımla kesinti/kurtarma ayrı saha kabulünde raporlanır.
