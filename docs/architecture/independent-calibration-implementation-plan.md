# Reçeteden bağımsız kalibrasyon ve değişken kamera sayısı — geliştirme planı

17 Ekim 2026. **Durum: KMC-0–9 A yazılım kabulü tamamlandı (7a/7b ayrı); KMC-10a DAG/frame sözleşmesi uygulandı; KMC-10b execution/Editor uygulandı; KMC-10c1 çekirdek, KMC-10c2 terminal ve KMC-10d Runtime UI uygulandı; KMC-10e B yazılım kabulü ve fiziksel devir dokümanı tamamlandı; fiziksel kabul ayrı açık.** [Kimlik](measurement-setup-contract.md), [registry](measurement-environment.md), [reçetesiz edinim](independent-calibration-capture.md), [kurulum etkinliği](scoped-active-calibration.md), [Sistem özeti](calibration-overview.md), [scoped Editor/Runtime](scoped-calibration-integration.md) ve [açık migration/taşıma](measurement-setup-migration.md) uygulandı. Kullanıcının dört paket isteği KMC-6 → KMC-7a → KMC-7b → KMC-8 sırasında tamamlandı; [KMC-9 birleşik yazılım kabulü](independent-calibration-acceptance.md) UI/ZXing ON 80/80 ve Widgets OFF/ZXing OFF 68/68 ile tamamlandı. N kalibrasyon yönetimi tamamlandı; Editor editor9 çok-source çalıştırır; Runtime editor9 için bütün-source provider ile çalışır; terminal7 ve N-kamera UI bağlıdır; fiziksel kabul ayrı açıktır. Legacy compatibility yalnız açık geçiş yapmamış reçetelerde korunur.

**Kamera sayısı sabit değildir.** Birçok makine tek kamera, bazıları 8–10 kamera kullanacaktır. Önceki taslaktaki beş kamera yalnız örnektir. Ürün, veri modeli, UI ve testlerde sabit beşli dizi veya 5 üst sınırı kullanılmaz. Tasarım N kamera içindir; kaynak/bellek sınırları açık, ölçülebilir ve yapılandırılabilir olmalıdır. “Sınırsız kamera” iddiası yoktur.

## 1. Kullanıcı kararları ve plan varsayımları

### Kullanıcının kesinleştirdiği yön

- Kalibrasyon, reçete açmadan/oluşturmadan yapılabilmeli.
- Kameradan resim çekme ve dosyadan görüntü açma desteklenmeli; mevcut hesaplama adımları korunmalı.
- Kalibrasyonun kaydı ve etkinleştirilmesi ayrı işlemler olarak kalmalı.
- Ana ekran **Sistem özeti** olmalı. Kurulum odaklı alternatif ayrıca ikinci bir ürün modu olarak geliştirilmez.
- Tek kamera öncelikli sade kullanım; gerektiğinde 8–10 ve farklı kamera sayıları desteklenmeli.
- İlk tasarım turu planlama içindi. Güncel dört paket yönlendirmesi KMC-6, KMC-7a, KMC-7b ve KMC-8 ile tamamlandı; sonraki görev yönlendirmesiyle KMC-9 A yazılım kabulü de tamamlandı; B sözleşmesi KMC-10a ile uygulandı; donanım ve kullanıcı ayarları değişmedi.

### Ayrıntılı planın başlangıç politikaları

Bu seçimler kullanıcı tarafından ayrı ayrı onaylanmış tercihler değildir; tutarlı bir ilk sürüm için önerilen varsayımlardır. Geliştirme bu varsayımları açıkça belgeleyerek ilerleyebilir; kullanıcı değiştirirse ilgili paket revize edilir.

| Konu | İlk sürüm önerisi |
| --- | --- |
| Katalog sahibi | Açık ve kaydedilmiş makine projesi. Reçete sayısı sıfır olabilir. Projesiz taslak veya projeler arası ortak katalog sonraki kapsam. |
| Etkinlik kapsamı | Kurulum başına bir seçili kalibrasyon kaydı; kamera başına bir operasyonel kurulum. Diğer profillerin kayıt/geçmişi korunur. |
| Reçete seçimi | Kaynak → kurulum bağlantısı; kurulumun etkin sürümünü takip eder. Sürüm değişimi hazırlanmış işi geçersiz kılar. Reçete bazında sürüme sabitleme ilk sürümde yok. |
| Aynı kamerada çok profil | Profiller saklanır; operasyonel profil değişimi açık işlemdir. Fiziksel optik/ayar değiştiği varsayılmaz, uyum tekrar denetlenir. Aynı karede çok düzlem kullanımı sonraki kapsam. |
| Yeniden başlatma / proje değiştirme | Son seçimler hatırlanır; mevcut ürün gibi kullanım izni otomatik geri gelmez. Sistem özetinden açık yeniden etkinleştirme gerekir. |
| Çevrimdışı dosya çalışması | Hesaplama ve kayıt yapılabilir. Yeni etkinleştirme, hedef kaynağın gerekli uyum kanıtı alınana kadar bekler. |
| Etkin kaynağın kopması | Seçim/geçmiş korunur; kullanılabilirlik kaldırılır. Yeniden bağlantı ve gerçek kaynak kontrolünden sonra açık yeniden etkinleştirme gerekir. |
| Bağımsız kontrol | Mevcut akış gibi opsiyonel; yapılmadı açık görünür. Yapılmış ve reddedilmiş kontrol kullanım izni vermez. |
| Çalışırken değişim | İlk sürümde Runtime/worker/pending kayıt/ACK varken etkinlik, kurulum, cihaz eşleme ve migration değişiklikleri engellenir. Kamera başına daha ince çalışma izni sonraki kapsam. |
| Çok kaynaklı ilk yürütme | Aynı iş çevriminde sıralı edinim. Eşzamanlı pozlama, senkron trigger veya kameralar arası geometri garantisi yok. |

“Etkin” sözcüğü tek başına bütün kontroller geçti anlamına gelmemeli: UI **seçili kayıt**, **kullanılabilirlik** ve **bağımsız kontrol** durumlarını ayrı gösterir.

## 2. Kod incelemesinde görülen başlangıç durumu

16 Ekim kaynak ağacındaki bulgular; uygulama paketine başlarken tekrar doğrulanır:

| Bölüm | Mevcut sınır / korunacak davranış |
| --- | --- |
| `src/app/CalibrationHome.hpp`, `MainWindowCalibration.cpp` | Home context içindeki kaynak seçili reçeteden geliyor. Katalog sahibi QWidget. |
| `src/app/CalibrationWizard.hpp/.cpp` | Optional ToolDefinition kaynağı; dosyayı kurulumla ilişkilendirmek için reçete kaynağı aranıyor. Matematik/session büyük ölçüde yeniden kullanılabilir. |
| `src/application/CalibrationCapture.hpp/.cpp` | acquire/setup/configuration ToolDefinition üzerinden; dosya configuration kimliği dosya yolunu, webcam kimliği index/backend'i kullanıyor. |
| `src/application/ActiveCalibrationService.hpp/.cpp` | Proje başına tek `active_`; kayıt matematiği, kontrol, strict replay, kilitli/atomik yazım mevcut. |
| `src/application/SystemCalibration.hpp/.cpp` | `source(recipe)` tek enabled kaynak bekliyor; tek snapshot'tan frame'e bağlama yapıyor. |
| `src/domain/ToolDefinition.cpp` | Reçetede edinim kaynağı sayısını açıkça en çok 1 ile sınırlıyor. |
| `src/application/ProductionRecipeService.cpp` | Production tam bir enabled kaynak istiyor. Completion kontrollerinin bir bölümü bütün çıktıları `CycleResult.frameId` ile karşılaştırıyor. |
| `src/application/RuntimeService.hpp` | SystemProvider tek optional snapshot döndürüyor. Configuration hazırlığı ile actual frame doğrulaması zaten ayrı. |
| `src/application/ExecutionService.hpp`, `EditorSession.hpp` | CycleResult frame listesi taşısa da tek ana frameId, tek Editor frame/TargetEvaluation varsayımları var. Sadece listede birden fazla frame bulunması çoklu kaynak kabulü değildir. |
| `src/vision/ImageFrame.hpp` | FrameMetadata source/frame/cycle/coordinates/zaman taşır; kalıcı cihaz ve kurulum kanıtı için yeni sözleşme gerekecek. |
| `src/application/WebcamAdapter.hpp/.cpp` | WebcamSettings index/backend ve process-geneli edinim kilidi. Qt cihaz kimliği/yeniden eşleme ayrıca ele alınmalı. |
| GenTL / CameraService | Discovery ve read-only bağlantı kontrolü var; fiziksel industrial grab tamamlanmış sayılmaz. |
| Sürümler | Kaynakta execution/23, terminal5, project1, recipe1–2, editor6–8; aktif wrapper1–3. Yeni sürümler yazım öncesi envanterlenir, eski numaraya yeni anlam eklenmez. |

KMC-0 başlangıç borcu olan `ui_editor_usability`, `ui_pose_tracking_smoke`, `ui_circle_smoke`, `ui_shell_smoke` kapandı. 16 Ekim tam Release UI 70/70 ve OFF/core 61/61 geçti. Önceki çalışma makinesi kanıtı birleşik 66/70 tarihsel kayıttır. Ayrıntılar [Windows çalışma ortamı](../windows-workstation.md) ve [progress](../progress.md).

## 3. Teslim sınırları

**A — Bağımsız kalibrasyon yönetimi (KMC-0…KMC-9):** N kamera/kurulum tanımlama, seçili kaynaktan reçetesiz kalibrasyon, kurulum başına kayıt/etkinlik, tek kaynaklı mevcut reçetelerin doğru modele bağlanması. Tek kameralı makineler bu aşamada uçtan uca kullanılabilir. N kameranın ayrı ayrı yönetilmesi tek reçetede N kamera yürütüldüğü anlamına gelmez.

**B — Aynı reçetede çoklu kaynak (KMC-10a…KMC-10e):** DAG, Editor cache, gerçek edinim, frame bazlı resolver, immutable Runtime ve terminal kanıtının N kaynak için genişlemesi. 8–10 kaynaklı yazılım kabulü bu aşamada yapılır. Aşama B, bu yol haritasının parçasıdır; A bittiğinde bütün plan tamamlandı denmez.

**C — Ayrı gelecek işler:** kameralar arasında ortak makine koordinatı, stereo/3D, lens intrinsics/undistort, robot kalibrasyonu/hareketi, senkron trigger/paralel edinim optimizasyonu, industrial SDK/grab ve fiziksel makine kabulü. Bunlar yazılım test fixture'larıyla kapanmaz.

## 4. Ekran şartnamesi

### E1 — Sistem özeti / Kalibrasyon

- Mevcut Qt Widgets uygulama kabuğu ve Setup/Calibration yeri korunur. HTML taslak görsel referanstır; WebView/HTML ürüne eklenmez.
- Sıfır kamera/kurulum: “Kamera veya ölçüm kurulumu ekle” ve “Dosyadan deneme başlat” eylemleri. Boş liste hata değildir; reçete oluştur önerisi gösterilmez.
- Tek kamera: kurulum otomatik seçilir, ayrı kamera seçim şeridi yer kaplamaz. Kamera adı ve durum üstte; yöntemler ve geçmiş doğrudan altında.
- Birden çok kamera: genişliğe göre sarılan özet kartları; seçili kartın ayrıntısı altında. 8–10 kamera normal ekranda birden çok satıra yerleşir. Uzun liste doğal dikey kaydırılır; sabit beş sütun veya görünmeyen kameraları sessiz düşürme yok.
- Kart: kullanıcı adı, operasyonel kurulum, seçili sürüm, kullanılabilirlik. Seri numarası/teknik kimlik ayrıntıda. İsim değişikliği kimliği değiştirmez.
- Seçili kurulumda yöntemler: Bilinen ölçü / Kalibrasyon plakası. Robot mevcut açıklamalı gelecek kapsam olarak kalır.
- Kayıt listesi yalnız seçili kurulumun kayıtları; denemeler ayrı anlaşılır görünümden erişilebilir. Satır seçimi etkinliği değiştirmez.
- Yeni kayıt kaydetmek mevcut etkinliği korur. Devre dışı bırak yalnız hedef kurulumu etkiler. Başka kamera kartına tıklamak cihazdan kare çekmez.
- Aynı anda bütün kameralar için canlı preview açılmaz; yalnız seçili ve desteklenen kaynaktan istek üzerine görüntü alınır.

### E2 — Kamera ve ölçüm kurulumları

- Mevcut Camera Manager keşif/bağlantı özellikleri korunur; kayıtlı cihazlar ile keşfedilmiş cihazlar ayırt edilir.
- Kamera ekle/eşle: görünen ad, adapter, mevcut fiziksel kimlik, desteklenen yetenekler. Olmayan seri numarası üretilmez.
- Webcam provider'ın döndürdüğü kimlik mümkünse kullanılır. Bu kimliğin işletim sistemi/USB portu değişiminde sabit kalacağı iddia edilmez. Belirsiz veya çoğul eşleşme otomatik çözülmez; kullanıcı yeniden eşler.
- Kurulum alanları: ad, kamera, optik profil, çalışma düzlemi, doğrulanabilen görüntü geometrisi. Çözünürlük/ROI/binning alanları adapter gerçekten sunuyorsa kullanılır.
- Görünen ad düzenleme ile geometrik kurulum değişimi ayrıdır. Lens/düzlem/geometri değişimi yeni kurulum revizyonu gerektirir; eski kalibrasyon yeni koşullara taşınmaz.
- Silmek yerine referanslı cihaz/kurulumu arşivleme önerilir; eski reçete ve sonuç geçmişi çözülebilir kalır. Kullanılan kurulum sessizce başka kameraya bağlanmaz.
- Kaydet, fiziksel cihaz ayarı uygulanmış gibi gösterilmez. Desteklenmeyen industrial grab eylemi açıklamalı pasif kalır.

### E3 — Görüntü adımı

- Reçeteden bağımsız Resim çek / Görüntü aç. Tek kurulum varsa otomatik hedef; birden çok varsa açık hedef seçimi.
- Dosya için “Bu kurulumdan kaydedilmiş” / “Yalnız deneme”. Görüntü kaynağı (dosya) ve hedef kamera ayrı görünür.
- Dosyanın yolu hedef kamera kimliği değildir. Farklı referans dosyaları aynı doğrulanmış kurulumda kullanılabilir; dosya değişimi yeni görüntü ve sonuç geçersizleştirme üretir.
- Çözünürlük/geometri uyumsuzluğunda hesap denemesi saklanabilir; hedef kurulum için kullanıma hazır gösterilmez.
- Kamera yok/çevrimdışı ise dosyadan hesaplama ve kayıt devam eder. Hedef tanımlı değilse deneme; dosya yüklemek kamera keşfi/bağlantısı gerektirmez.
- Kaynak, hedef, kurulum revizyonu veya görüntü değişimi aşağıdaki nokta/model/kontrol sonuçlarını geçersiz kılar. Yalnız isim değişimi matematiği yeniden başlatmaz.
- Kaynak edinimi calibration session'a aittir; gizli reçete veya geçici acquisition tool eklenmez.

### E4 — Referans / hesap / kayıt

- Mevcut gerçek Edge/Caliper aday seçimi, nokta çifti, chessboard/circle grid ve gerçek mm girişi korunur. Matematik yeniden yazılmaz.
- H modeli düzlemsel perspektif dönüşümü olarak adlandırılır; lens distortion düzeltilmiş iddiası yapılmaz.
- İsteğe bağlı bağımsız kontrol önceki hesapla karıştırılmaz; yapılmadı/sonuç var/kabul/ret ayrı gösterilir. Kontrol yokken sıfır hata veya onay üretilmez.
- Geri gidiş alanları korur; gerçekten değişen girdilerin aşağı akışı invalid olur. Cancel/Stop/geç worker ve pencere kapanış kuralları korunur.
- Kaydet ve bitir yalnız yeni aday/sürüm oluşturur. Deneme ilişkilendirmesi açık yeni türetimdir; eski raster kökeni ve model kanıtı değiştirilmez.
- Deneme görüntüsünü tekrar kullanmak için piksel kanıtı erişilebilir olmalı. İlk sürümde dosya hash'iyle yeniden yükleme; dosya kayıp/değişmişse yeniden görüntü gerekir. Bütün rasterları JSON'a gömmek bu planın varsayımı değildir.

### E5 — Etkinleştirme incelemesi

- Hedef kamera/kurulum; eski/yeni kayıt; kaynak uyumu; bağımsız kontrol; etkilenen reçeteler; exact world ROI/referans ve eski px tolerans etkisi gösterilir.
- Engeller ayrı gerekçelerle: eşleşmemiş cihaz, çevrimdışı, yanlış geometri/kurulum, deneme, reddedilmiş kontrol, çalışan iş, pending kayıt/ACK, kalıcı yazım hatası.
- Aktivasyon actual uyum kanıtını denetler. Desteklenen software acquisition için gerçek kare alınabilir; ileride hardware trigger için sahte probe veya izinsiz zorunlu tetik üretilmez.
- Başarısız aktivasyon eski seçimi korur. Başarılı değişim ilgili cache/hazır snapshot'ı geçersiz kılar; başka kameraların kayıtları aynı kalır.
- İlk sürümde toplu etkinleştirme yok; N kartın biri üzerinden açık işlem. İleride “Uyumlu seçimleri yeniden etkinleştir” ayrıca transaction ve kısmi hata semantiğiyle tasarlanabilir.

### E6 — Recipe Editor

- Source properties'de ölçüm kurulumu seçilir; etkin model ve kullanılabilirlik salt okunur. Araç başına kalibrasyon listesi eklenmez.
- Kamera ediniminde device/geometry ayarlarının sahibi kurulumdur. Aynı ayarın reçetede ikinci bağımsız kopyası bulunmaz; değişiklik Kurulumları aç eylemine gider.
- Image File kaynağında dosya seçimi/replay asset'i ayrı tutulur. Gerçek kameradan kaydedilmiş dosya offline replay kökeniyle kalır, canlı kamera sonucu sayılmaz.
- A tesliminde mevcut tek source korunur. B tesliminde birden çok source dalı, kaynak/kare seçici, branch'e ait Viewer/overlay ve cache bulunur.
- Farklı kamera koordinatlarındaki Point/ROI/feature bağlantıları reddedilir. Aynı mm birimi farklı dünya düzlemlerini eşitlemez.

### E7 — Runtime / Logs

- Yalnız seçili reçetenin kullandığı kaynaklar hazırlanır. Kullanılmayan çevrimdışı kamera işi engellemez.
- A tesliminde bir kaynak; B tesliminde N satır. Her satır kamera, kurulum, seçili sürüm, configuration/actual frame durumu taşır.
- Run hazırlık; actual frame ve fiziksel sonuç çevrimde. Kalibrasyon eksikliği parça FAIL değildir; ilgili yürütme hatası ve NOT_EVALUATED korunur.
- B'de bütün gerekli kaynaklar tek iş çevrimine bağlanır; ilk sürüm edinim sıralıdır. Kaynak başına frameId/zaman farklı, cycleId ortak olur.
- Logs/CSV geçmişte kullanılan tam sürüm/kurulum/kamera ilişkisini gösterir; bugün etkin olan model geçmiş sonucu değiştirmez.

## 5. Application ve veri sözleşmeleri

İsimler hedef sorumluluğu anlatır; C++ sınıf/dosya adları pakette kesinleştirilebilir. UI algoritma/SDK çağırmaz; üç applicationType ortak servisleri kullanır.

### 5.1 Kimlikler ve sahiplik

| Kavram | Anlam / değişim kuralı |
| --- | --- |
| CameraId | Proje içi kalıcı mantıksal kimlik. Kart sırası, ad veya webcam index değildir. |
| DeviceBinding | Adapter ve gerçek keşif kimliği; kimlik güven düzeyi/belirsizlik, yeniden eşleme revizyonu. |
| SetupId + revision | CameraId, optik/düzlem beyanı ve geometrik edinim koşulları. Kimliği etkileyen değişim yeni revision. |
| Calibration record | Original model, öğretim kanıtı, setup revision, kontrol kanıtı ve sürüm; immutable. |
| Selection | SetupId → exact record/content/version; ayrıca CameraId → operasyonel SetupId. İsimle/en yeni kayıtla fallback yok. |
| Frame provenance | Actual source tool veya calibration session kaynağı, kamera/kurulum revizyonu, frame/cycle/coordinates, raster ve işlem kimlikleri. |
| Recipe source binding | SourceToolId → SetupId. Production snapshot gerçek etkin sürümü admission'da yakalar. |

Application owner mevcut CalibrationHome QWidget'inden ayrılır. Home yalnız görünüm/komut tüketicisidir; sayfa kapanınca veya reçete değişince katalog servisi kaybolmaz. Snapshot owner thread'de oluşturulur; worker canlı katalog veya widget tutmaz.

Kalibrasyon uyumu ile görüntü/recipe cache imzası ayrı tutulur: exposure/gain gibi ayarlar detection/cache'i etkileyebilir; sensör geometri değişimi gibi otomatik olarak bütün geometrik kalibrasyonları geçersiz sayılacak alanlar adapter sözleşmesinde açık tanımlanır. Bilinmeyen kritik ayar uyumlu kabul edilmez.

### 5.2 Edinim ve dosya kökeni

- CalibrationCapture reçete ToolDefinition yerine application kaynak isteğini kabul edecek şekilde genişletilir. Legacy çağrılar geçici adapter üzerinden korunabilir.
- Session'a ait kaynak kimliği gerçek calibration edinimini ifade eder; sahte Recipe/source tool oluşturulmaz. Original modeldeki geçmiş sourceToolId alanı yeniden yazılmaz.
- Dosya içeriği hash'i/path ve hedef kurulum beyanı ayrı saklanır. Kamera acquired, setup_file, trial_file ve legacy_record birbirine çevrilmez.
- Yeni association kaydı özgün kanıta referans verir; mevcut matematik hedef koordinat/geometri için tekrar denetlenir. Bu aktarım lens/geometri dönüşümü üretmez.
- Bilinmeyen provenance, yanlış cihaz, eski generation, yanlış setup revision veya farklı raw geometry fiziksel ölçüm yolunda reddedilir.
- Webcam kimlik desteği fiziksel kabul değildir. Qt Multimedia yetenekleri ve kararlılık sınırları uygulama sırasında resmi Qt belgeleriyle doğrulanır.

### 5.3 Kalıcılık / migration

- Önce mevcut schema/codec/hash yollarının envanteri çıkarılır. Yeni registry, record association, source binding ve çok-frame terminal için açık schema sürümleri tahsis edilir; numaralar bu planla erkenden işgal edilmez.
- KMC-1 dosya kararı: `<proje>.measurement-environment.json`; devices, setups, operational profiles ve selections tek atomik belgede olacak. Immutable model kataloğu mevcut `<proje>.active-calibrations.json` yolunu koruyacak. Bu dosyalara yeni registry yazımı KMC-2/4'tür.
- Çok dosyada atomik transaction varmış gibi davranılmaz: önce doğrulanmış aday kalıcı yazılır, sonra registry exact contentId ile seçimi atomik günceller. Kesinti seçilmemiş aday bırakabilir, yarım/geçersiz etkin seçim bırakamaz. Retry idempotent olmalı.
- Kilit, beklenen revision ve kaydetmeden hemen önce bağımlılık kontrolü; harici değişiklikte conflict/reload, sessiz last-write-wins yok.
- Legacy dosyalar read/no-op Save sırasında değiştirilmez. Açık aktarım eski bütün sürümleri/kontrol kanıtını korur. Eksik cihaz/kurulum bilgisi kullanıcı eşlemesi ister; eksik raster veya serial üretilmez.
- Mevcut global sistem politikasından yeni registry politikasına geçiş proje bazında açıktır. Geçiş sonrası seçimsiz/uyumsuz durum eski recipe binding'e geri düşmez. Eşleştirilmemiş eski kaynaklar açık “geçiş gerekli” durumunda kalır; raw-only çalışmanın etkisi ayrıca tanımlanır.
- Save As/import/export/restart kapsamına registry ve kalibrasyon kataloğu dahil edilir. Proje kopyası etkin izin devralmaz; fiziksel eşleme yeniden denetlenir. Yarım kopya/dangling kayıt açık hata verir. Bu akış KMC-2/KMC-8 kabulüne dahildir.

### 5.4 Etkinlik ve kullanılabilirlik

Kayıt durumu: **hesaplandı / bağımsız kontrol yapılmadı veya sonucu mevcut**. Seçim durumu: **seçili / seçili değil / son seçim hatırlanıyor**. Kullanım durumu: **uyum denetimi bekliyor / kullanılabilir / uyumsuz / kaynak çevrimdışı / desteklenmeyen adapter**. İşlem kilidi ayrı **Runtime çalışıyor / worker / pending kayıt-ACK** gerekçesidir.

Etkinlik seçimi, geometri değişikliğini geçerli yapmaz. Geçmişte seçilmiş kayıt bağlantı kesilince silinmez; actual kullanım izni düşer. Startup ve proje değişiminde kullanım izni kaldırılır. Tek başına combo seçimi, kart tıklaması veya dosya Save hiçbirini yükseltmez.

### 5.5 Çok-frame yürütme

- Registry kapasitesi N, UI görünür kart sayısı ve eşzamanlı acquisition sayısı ayrı kavramlardır. Kapasite aşımı açık hata; sessiz ilk 5/10 seçimi yok.
- Tek frameId alanını kaldırmak veya kontrolü gevşetmek yeterli değildir. Her tool/port değeri gerçek source/frame/cycle/coordinate ile doğrulanır; yeni çevrimde typed source→frame index sözleşmesi gerekir.
- Required source kümesi enabled DAG bağımlılıklarından hesaplanır; optional raw/pointMm davranışı korunur. Kullanılmayan cihazlar resolve/prepare edilmez.
- Kamera başına resolver, kullanılan immutable snapshot setinden seçim yapar. Frame'de kullanıcı tarafından yazılmış cameraId tek başına trusted eşleşme sayılmaz; admission/acquisition kanıtıyla çapraz denetlenir.
- Cross-source geometrik bağlantılar ve iki farklı fiziksel düzlemin ölçümlerini birleştiren tool girişleri açıkça reddedilir. İlk B sürümünde otomatik stitching veya world eşitleme yapılmaz.
- Sıralı acquire toplam deadline/bellek bütçesiyle sınırlandırılır. On kamera desteği on bağımsız sonsuz timeout toplamı değildir; Stop/deadline kuyrukta da etkili olur.
- Kaynak hatasında ilgili dal ERROR/SKIPPED, bağımsız dallar mevcut sözleşmeye göre tanı için çalışabilir; zorunlu kaynak eksikken genel başarılı üretim sonucu yok. Kısmi değerler yalnız geçerli kökeniyle tanı olarak korunur.
- Production trusted completion digest ve replay, bütün kaynak snapshot'larını/frame'leri/model kanıtlarını kapsar. Bir kameranın frame veya calibration zarfını diğerine koymak reddedilir.
- vf.socket/2 mevcut scalar sonuç sözleşmesi sessizce genişletilmez. B'de seçili mevcut output mapping korunur; N sonuçlu yeni protokol ayrı ihtiyaçtır. Genel parça kararı ayrıca mevcut decision politikasına bağlıdır.

## 6. Sıralı uygulama paketleri

Her paket kendinden öncekinin kabulüne dayanır. Bütün paketler tek oturuma yüklenmez. Aşağıdaki test adları **önerilen yeni test adlarıdır**; mevcut CTest envanteriyle çakışma kontrolü yapılarak eklenir.

| Sıra | Paket | Teslim |
| --- | --- | --- |
| 1 | KMC-0 | Başlangıç regresyonları ve baseline |
| 2 | KMC-1 | Kamera/kurulum kimliği ve codec sözleşmesi |
| 3 | KMC-2 | Registry, kalıcılık ve kesinti davranışı |
| 4 | KMC-3 | Reçetesiz edinim ve dosya kökeni |
| 5 | KMC-4 | Kurulum başına etkinlik servisi |
| 6 | KMC-5 | Sistem özeti ve kurulum ekranları |
| 7 | KMC-6 | Sihirbazlar ve deneme ilişkilendirme |
| 8 | KMC-7a | Tek kaynaklı Editor bağlantısı |
| 9 | KMC-7b | Tek kaynaklı Runtime / geçmiş |
| 10 | KMC-8 | Aktivasyon incelemesi ve migration |
| 11 | KMC-9 | A birleşik kabulü |
| 12 | KMC-10a | Çok-source DAG / frame sözleşmesi |
| 13 | KMC-10b | Çok-source execution / Editor |
| 14 | KMC-10c1 | Çok-source Runtime / trusted completion çekirdeği |
| 15 | KMC-10c2 | Çok-source terminal / restart / CSV |
| 16 | KMC-10d | Çok-source Runtime UI |
| 17 | KMC-10e | B birleşik kabulü ve fiziksel devir |

Bu tablo süre tahmini değildir. Bir paket oturuma sığmazsa tamamlanan alt hedef,
kalan kabul ve sonraki değişiklik progress'e yazılır; sıradaki paket erken açılmaz.

### KMC-0 — Başlangıç kabulü ve eski UI regresyonları

**Tamamlandı:** UI 70/70, OFF/core 61/61, ilgili offscreen görseller incelendi; progress kaydı 16 Ekim.

- Oku: AGENTS, progress, README, Windows ortamı, bu plan ve KUI-5 sözleşmesi.
- Mevcut dirty dosyaları envanterle; kullanıcı işi üzerine yazma. Güncel branch/build/cache ve config/local.ui.ini hash'ini kaydet.
- Bilinen dört UI hatasını tekrar üret. Eski test beklentisi mi ürün hatası mı kanıtla; testleri atlayarak/koşulu gevşeterek geçirme. KUI-5 mm varsayılanı ve raw diagnostic yollarını birlikte koru.
- İlgili UI düzeltmelerinden sonra Release + ilgili test/görsel ve tam UI CTest. Core baseline yapılandırması mevcut değilse README yollarıyla ayrı Release OFF oluştur; bağımlılıkları yeniden kurma.
- Çıkış: açık başlangıç hataları çözülmüş ve baseline belgelenmiş. Ağ/cihaz ortam engeli ürün hatasından ayrılır; çözülmeyen hata varken tam kabul iddia edilmez.

### KMC-1 — Kamera/kurulum ve kaynak kimliği sözleşmesi

**Tamamlandı:** [uygulanan sözleşme](measurement-setup-contract.md), 100 kontrol, iki Release build'de ilgili 10/10. Yeni UI/registry/aktivasyon yok.

- Yeni domain/application tipleri ve strict codec: CameraId, DeviceBinding, SetupId/revision, geometry descriptor, source origin. UI yok.
- Legacy alanlar, yeni schema tahsis tablosu, adapter yetenekleri, stable-id yokluğu ve geometry/cache hash ayrımı yazılır.
- Yeni servis sınırları mevcut ToolDefinition/CalibrationCapture'a referansla tasarlanır; bu pakette global aktif model veya Runtime davranışı değiştirilmez.
- Kabul: `measurement_setup_contract`; rename kimliği korur, fiziksel/geometrik değişim revision üretir, bilinmeyen schema/duplicate ID/belirsiz eşleşme reddedilir, N=0/1/2/5/8/10 roundtrip.

### KMC-2 — Proje registry'si ve kalıcı seçim altyapısı

**Tamamlandı:** [uygulanan sözleşme](measurement-environment.md); son copy/retry ekleriyle 62 persistence kontrolü, iki Release build ve ilgili regresyonlar. Save As/import/export bağımlılık API'si hazır; eski UI komutlarına bağlama KMC-8 kapsamındadır.

- Uygulama sahibi registry ve atomik/lock/revision kontrollü repository; kamera/kurulum arşivleme, proje izolasyonu, Save As/import/export bağımlılıkları.
- Kalibrasyon kaydı ile seçim arasındaki iki aşamalı commit/recovery; başlangıçta remembered fakat kullanılabilir olmayan seçim.
- Legacy dosyaları otomatik migrate etme; henüz eski UI'yı yeni davranışa geçirme.
- Kabul: `measurement_environment_persistence`; fault injection, disk/lock/revision conflict, kesinti/retry, dangling/tamper, proje kopyası, sıfır reçeteli proje, 10 kayıt ve kapasite limiti.

### KMC-3 — Reçetesiz edinim ve dosya session'ı

**Tamamlandı:** [uygulanan sözleşme](independent-calibration-capture.md); 64 kontrol, gerçek PNG/Edge/Caliper/H ve kamera fixture'ı; UI ilgili 12/12, core 11/11. Yeni session eski UI'ya bağlı değildir; fiziksel kamera kabulü yoktur.

- CalibrationCapture/application request'i ToolDefinition bağımlılığından ayır; gerçek adapter çağrıları worker'da. Webcam cihaz seçim kimliği için gerekli küçük adapter/codec değişimi; legacy index yolu açık uyumlulukla kalır.
- File/open, hedef kurulum, trial, immutable provenance ve bağımsız cancellation/generation. Camera listesi görüntü almaz; yalnız açık capture komutu alır.
- Matematik session'a adapter ekle; Edge/Caliper/plaka algoritmalarını yeniden yazma. Geometri hatası, değişen hedef ve geç completion temizliği.
- Kabul: `independent_calibration_capture`; reçetesiz gerçek PNG decode, dosya yolu değişirken kurulum kimliğinin sabitliği, kaynak/geometry uyuşmazlığı, offline/unsupported adapter, cancel ve late frame. Fiziksel kamera yoksa adapter fixture açık etiketlenir.

### KMC-4 — Kurulum başına etkinlik ve çoklu snapshot servisi

**Tamamlandı:** [uygulanan sözleşme](scoped-active-calibration.md); kabul kanıtları [progress](../progress.md).

- ActiveCalibrationService tek active_ varsayımı yerine registry seçimini kullanan application hizmeti. Kayıt association/replay ve tam setup revision uyumu.
- Bir kameranın seçim değişimi başka kameranın revision/etkin kaydını değiştirmez. Aynı kamerada operasyonel kurulum değiştirme açık ve tek işlem.
- Gate, ready/control ayrımı, restart/disconnect, failed commit eski seçimi koruma. Owner thread'den N immutable snapshot alabilme; mevcut tek kaynak çağrısı compatibility adapter ile korunur.
- Kabul: `scoped_active_calibration`; 1/2/8/10 kurulum, farklı ölçekler, A→B izolasyonu, invalid/trial/control retleri, aynı isim/çözünürlükte farklı fiziksel kamera, tamper, restart, busy.

### KMC-5 — Sistem özeti ve kamera/kurulum UI

**Tamamlandı:** [uygulanan UI ve sınırlar](calibration-overview.md); yeni sihirbaz/aktivasyon eylemleri gerekçeli pasif. Kabul kanıtları [progress](../progress.md).

- E1/E2'yi Qt Widgets ile uygula; servis sahibi MainWindow/application olur, Home context aktif reçete istemez.
- 0/1/N düzenleri, sarılan kartlar, kaynak/kurulum ayrımı ve true capability durumları. N cihazda yalnız seçili görüntü için kaynak tahsisi.
- UI scaffolding davranışı çalışan özellik gibi sunulmaz; bu pakette hazır olmayan capture veya aktivasyon eylemleri gerekçeli pasif kalır.
- Kabul: yeni `ui_calibration_overview`; sıfır reçete, 0/1/2/8/10 kamera, uzun isimler, klavye, seçim/yeni kayıt/no-op; 1400×900 ve 850×600 görseller; 10 kartın tamamına erişim.

### KMC-6 — İki sihirbaz ve dosya/deneme ilişkilendirme

- E3/E4; source recipe context kalkar. Hedef kurulum sihirbaz snapshot'ına alınır; geri/ileri/Stop ve kapatma korunur.
- Bilinen ölçü gerçek noktaları, iki plaka yöntemi, optional gerçek bağımsız kontrol, isim/kayıt ve sonra ana sayfaya dönüş.
- Deneme ilişkilendirme: yeni sürüm/türetim, raster yeniden hash kontrolü ve gerekli replay; eski kayıt değişmez.
- Kabul: `ui_independent_calibration_wizard` ve mevcut `ui_calibration_wizard`; reçetesiz uçtan uca file + test adapter, trial→association, missing/changed raster, target switch, cancel/late worker, kaydetme etkinliği değiştirmez. UI Release/görsel zorunlu.

### KMC-7a — Tek kaynaklı Editor ve kaynak→kurulum bağlantısı

- E6 kaynak properties, source binding persistence; üç applicationType ortak yol. Reçete tool ID ile kalıcı cihaz ID ayrı.
- SystemCalibration/EditorSession bağlamı yalnız reçetenin kullandığı kurulum snapshot'ından oluşur; başka kameradaki değişiklik geçerli cache'i gereksiz silmez.
- Model değişiminde ilgili mm/downstream invalidation, raw cache/TargetOnly acquisition0 ve world exact dependency koruması.
- Kabul: `scoped_calibration_editor`; farklı reçetelerde aynı kurulum, iki farklı kurulum/farklı ölçek, source switch, rename/no-op Save, başka kameranın değişimi, mm invalid/raw korunması ve eski world yeniden öğretim.

### KMC-7b — Tek kaynaklı Runtime ve geçmiş kanıtı

- Runtime SystemProvider tek global seçim yerine reçetenin required kurulumu için snapshot verir; prepare/begin/actual frame/finish kapıları korunur.
- Production dependency/evidence'e gerçek cihaz/kurulum revizyonu ekle; engine/terminal davranış sürümü gerektiği anda yükseltilir. Eski terminal okuyucuları korunur.
- E7 tek kaynak, Logs/CSV yeni zarf; restart/record retry/ACK ve trust digest. Raw-only reçete mm gerektiren reçeteyle aynı zorunluluğa sokulmaz.
- Kabul: `scoped_calibration_runtime`; üç applicationType, kullanılan/kullanılmayan offline kaynak, snapshot değişimi, başka kamera evidence swap/tamper, retry/Stop/restart/CSV ve mevcut production/socket regresyonları.

### KMC-8 — Aktivasyon incelemesi ve açık eski veri geçişi

- E5 gerçek etki incelemesi ve aktivasyon komutları. Legacy global kayıtlar için kamera/kurulum eşleme; history/no-op/hash koruma.
- Proje politikası geçişi ve yarım migration recovery; Save As/import/export UI geri bildirimi; seçimsiz durumda legacy fallback yok.
- Sıfır reçetede Etki incelemesi boş fakat geçerli; aktivasyon bununla engellenmez. Source actual denetimi reçeteden bağımsızdır.
- Kabul: `measurement_setup_migration`, `ui_scoped_calibration_activation`; v1–3 wrapper/legacy kayıtlar, bütün sürümler, ret/çatışma/tekrar, bağımsız aktif seçim, offline/busy/error, exact world etkisi.

### KMC-9 — A tesliminin birleşik kabulü (tamamlandı)

- Yeni/legacy boş ve dolu projeler; 0/1/2/8/10 kamera, tüm iki yöntem/dosya kökenleri ve üç applicationType üzerinden tam senaryolar.
- UI ON/ZXing ON ve Widgets OFF/ZXing OFF Release tam CTest; normal/küçük ekran ve gerçek kullanıcı akışı görsel kabul. Sayılar o koşumdaki envanterden yazılır.
- README/requirements/mimari/progress güncellenir. “N kalibrasyon yönetimi tamamlandı, reçetede tek source sınırı sürüyor” açık teslim cümlesi.
- A kabulü geçmeden B'de tek kaynak sınırı kaldırılmaz. Donanım olmayışı dosya/core kabulünü durdurmaz; fiziksel kabul açık kaydedilir.

### KMC-10a — Çok kaynaklı DAG ve frame sözleşmesi

**Tamamlandı:** [editor9 opt-in, DAG kökeni ve source-frame sözleşmesi](multi-source-graph-contract.md).
Root/recipe1–2, legacy codec/hash ve execution24/terminal6 korunur. Bu paket
programatik sözleşmedir; schema9 execution/production açık ret verir. Son kabul
kanıtları [progress](../progress.md); sonraki paket KMC-10b'dir.

- Recipe/source schema, ToolDefinition/ToolGraph ve ToolValues frame semantiği. Yeni çok-source schema opt-in; eski no-op hash korunur.
- Kaynak→dal/coordinate kimliği, required sources, per-port origin ve bounded source-frame index. CycleResult legacy frameId'nin yeni anlamı açık belirlenir; tek kareye sahte eşleme yapılmaz.
- Cross-source image/region/point/reference/geometry reddi. Bu aşamada Production çok-source kapısı kapalı kalır.
- Kabul: `multi_source_graph_contract`; N=1/2/8/10, isim/ID çakışması, disable/delete/rebind/migration, çapraz kamera bağlantısı ve unsupported schema.

### KMC-10b — Çok kaynaklı execution ve Editor

Uygulandı. [Kullanım, kabul ve sınırlar](multi-source-execution-editor.md); Release/CTest kanıtları [progress](../progress.md).

- ExecutionService source başına frame ve resolver, sıralı acquisition, toplam deadline/cancel ve bellek bütçesi. Mevcut webcam kilidi paralel donanım desteği varmış gibi kaldırılmaz.
- EditorSession per-source frame/cache; hedef tool yalnız kendi upstream kaynağını kullanır. Viewer source seçici, her branch'e doğru overlay, kaynak silme/yenileme davranışı.
- Bir kameradaki parametre/kalibrasyon değişimi yalnız bağımlı dalları invalid eder. Raw/processed frame zinciri kamera kimliğini kaybetmez.
- Kabul: `multi_source_execution`, `multi_source_target_only`, `ui_multi_source_editor`; 10 farklı gerçek dosya rasterı/farklı model, bağımsız ölçü oracle'ları, acquisition sayaçları, timeout/Stop/late frame, karışmış frame retleri. Production kapısı hâlâ kapalı.

### KMC-10c1 — Çok kaynaklı immutable Runtime ve completion çekirdeği

Uygulandı; programatik/bellek kabulüdür. [Sözleşme ve sınırlar](multi-source-production-contract.md). Bu çekirdek paketinin kapanışında persistent/UI kapısı kapalıydı; KMC-10c2 ile bütün-set Runtime üzerinden açıldı.

- Required source kümesinin tek tutarlı snapshot seti; admission anında sürüm seti sabitlenir. İki registry okuması arasında değişim ya bütün seti yeniden alır ya conflict verir.
- ProductionRecipeService kaynak sayısı ve tek frameId varsayımları, ToolResultCodec/replay, completion resolver ve digest birlikte güncellenir. Kontrolü kaldırarak çok-source açılmaz.
- Tüm required frame'ler ortak cycle, farklı source/frame/coordinates taşımalı. Kaynak hatası/missing/duplicate/foreign frame/record retry ve cancel üretim kararını doğru engeller.
- Bu pakette çekirdek/in-memory kabul tamamlanır; kalıcı terminal kanıtı tamamlanmadan kullanıcıya çok-source production açılmaz.
- Kabul: `multi_source_production_contract`; 2 ve 10 kaynaklı gerçek execution, source-frame/calibration swap, same pixels different source, admit sonrası seçim değişimi, partial/error ve trusted completion sahtesi.

### KMC-10c2 — Çok kaynaklı terminal, restart ve dış sonuç kanıtı

Uygulandı; [terminal7/retry/restart/socket sözleşmesi](multi-source-runtime-terminal.md). KMC-10e tam Release UI 87/87 / core OFF 73/73 kabulüne dahildir.

- Yeni terminal schema/okuyucu/Logs/CSV tam snapshot setini taşır; eski 1–5 kayıtlarının anlamı korunur. N source-frame eşleşmesi ve immutable model seti kalıcı kayıttan tekrar doğrulanır.
- Record failure/retry aynı trusted completion'ı saklar; yeniden capture veya model çözümü yapmaz. Restart, yetki veya güncel etkinlik uydurmaz; yalnız geçmiş sonucu okur.
- vf.socket/2 mevcut seçili scalar output mapping semantiği korunur. Çoklu output/aggregate part decision eklenmez; timeout/ACK/dedup/outbox mevcut sınırları aşılmış gösterilmez.
- Kabul: `multi_source_terminal_trace`; disk tamper, kayıp/duplicate source, geçmiş sürümler, append/retry/lock, restart/CSV ve mevcut localhost socket regresyonları. Bu kabulden sonra çok-source production kapısı açılabilir.

### KMC-10d — Çok kaynaklı Runtime görünümü ve N-kamera UX

Uygulandı; [kaynak seçimi, model ve actual frame gösterimi](multi-source-runtime-terminal.md#runtime-kullanımı-ve-n-kaynak-ux).

- E7 N kaynak hazırlık tablosu, selected frame Viewer, kaynak bazlı hata/kalibrasyon gösterimi, Logs/CSV erişimi.
- Kullanılmayan offline kamera iş hazırlığını etkilemez. Aynı kamera bir reçetede iki çelişkili profille istenirse açık ret; aynı fiziksel kaynağın tekrar kullanım politikası açık tanımlanır, iki gizli capture yapılmaz.
- 8–10 kaynakta gereksiz canlı preview yok; listeler klavye ve küçük ekranda erişilebilir.
- Kabul: `ui_multi_source_runtime`; 1/2/8/10 kaynak, busy/Stop, preparation vs actual frame, geçmiş sürüm gösterimi, 1400×900/850×600 görseller.

### KMC-10e — B teslimi ve fiziksel kabul devir paketi

[Birleşik kabul, kapasite ve fiziksel devir listesi](multi-source-release-acceptance.md); tam Release UI/ZXing ON 87/87 ve Widgets OFF/ZXing OFF 73/73 geçti. B yazılım kabulü ve devir dokümanı tamamlandı; fiziksel kabul ayrı açıktır.

- Tam Release ON/OFF CTest, legacy migration/replay/socket/code/world/pose regresyonları ve uçtan uca N kaynak senaryoları.
- Kapasiteyi sayı yerine frame boyutu/toplam byte/deadline ile ölç; fixture boyutları ve kaynak sayısı raporda. 10 küçük PNG başarısı 10 endüstriyel kameranın üretim hızını kanıtlamaz.
- Fiziksel kabul listesi: gerçek cihaz kimlikleri, USB/transport/driver, kopma/yeniden bağlanma, geometri/optik değişimi, gerçek referans ölçü ve hata, gerekli edinim süresi. Cihaz yoksa bu bölüm açık kalır.
- Teslim ayrımı: A yazılım kabulü / B yazılım kabulü / fiziksel kabul / ortak koordinat-senkronizasyon. Hepsi tek “tamamlandı” altında birleşmez.

## 7. Paketler arası kabul matrisi

| Senaryo | Gerekli kanıt |
| --- | --- |
| Hiç reçete yok | Kamera/file → gerçek hesap → kayıt → bağımsız aktivasyon, project recipes array değişmez |
| N=0 | Boş sistem ve dosyadan deneme; sahte kamera/kaynak eklenmez |
| N=1 | Ek kamera seçici yükü olmadan tam akış; eski tek-source sonuçları korunur |
| N=2/5/8/10 | Hepsi listelenir, farklı model/ölçek ve kimlikle ayrılır; sabit sayı varsayımı yok |
| Kapasite aşımı | Açık kontrollü hata, eski kayıt korunur; bellek/timeout sınırsız büyümez |
| İki kamera aynı ad/çözünürlük/piksel | Kalibrasyon kimlikleri karışmaz; ad veya pixel eşitliği physical identity yerine geçmez |
| Bir kamera yeniden eşlendi | Eski seçim korunabilir ama kullanılamaz; yeni bağ uyum/etkinleştirme ister |
| Geometri veya optik/düzlem değişti | Eski model bloke, exact ROI yeniden öğretim; px tolerans mm'ye çevrilmez |
| Dosya yolu farklı / içerik aynı | Kurulum kimliği yola bağlanmaz; görüntü kökeni ve hash ayrı kayıtlı |
| Dosya denemesi hedefe bağlandı | Yeni association/sürüm, orijinal kanıt değişmez; raster kayıp/değişim reddi |
| Kaydetme/aktivasyon yarıda kesildi | Eski seçim geçerli, aday orphan olabilir; retry güvenli/idempotent |
| Başka kamera kalibrasyonu değişti | Kullanılmayan kaynağın Editor cache ve hazır işi gereksiz invalid olmaz |
| Aktif kaynağın bağlantısı koptu | Gerçek yeni frame/mm yok; old result gösterilmez; history korunur |
| Restart / başka proje / Save As | Kullanım izni sessiz taşınmaz; hatırlanan seçim ayrı |
| B'de çok frame | Her tool kendi source/frame'ine bağlı; yanlış eşleşme ve cross-camera geometri reddi |
| Stop / geç completion | Kısmi eski çıktılar yeni çevrime geçmez; pending kayıt/ACK mevcut semantik korunur |
| Eski kayıtlar | Legacy byte/hash/no-op ve terminal1–5 replay; otomatik fiziksel kimlik uydurma yok |

Mevcut regresyon çekirdeği: `active_calibration_contract`, `calibration_wizard_contract`, `calibration_plate_contract`, `system_calibration_contract`, `calibration_persistence`, `measurement_contract`, `measurement_dag`, `manual_scale_contract`, `manual_scale_runtime`, `plane_mapping_contract`, `homography_runtime`, `calibrated_circle_contract`, `metrology_runtime_contract`, `editor_target_only`, `production_recipe_contract`, `runtime_persistence`, `runtime_coordinator`, `plc_socket_runtime`, `owned_frame_contract`, `coordinates_contract`, `tool_values_contract`, `typed_terminal_trace`, `tool_migration_contract`, reference/world tracking testleri. Değişikliğe göre ilgili alt küme seçilir; A/B kapanışında tam koşum yapılır.

## 8. Yerel doğrulama ve kanıt düzeni

Başlangıç kaynağı README ve `docs/windows-workstation.md`; bu makinenin yolları aşağıdadır. Mevcut cache uyumluysa yeniden configure/install yapılmaz. KMC-9 gerçek ayrı ON/UI ve OFF/core koşumları, komutları ve kanıtları [kabul kaydındadır](independent-calibration-acceptance.md). Aşağıdaki komutlar genel örnektir; reference-core önceki ZXing ON cache olarak korunur, KMC-9 OFF koşumu build/kmc9-core-off kullanır.

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
cmake --build build/reference-ui -j 6
ctest --test-dir build/reference-ui --output-on-failure -R '^(ui_editor_usability|ui_pose_tracking_smoke|ui_circle_smoke|ui_shell_smoke)$'

# Yalnız mevcut core cache varsa:
cmake --build build/reference-core -j 6

# A/B teslim kapanışı; iki ayrı Release yapılandırmasının gerçek sonuçları kaydedilir:
ctest --test-dir build/reference-ui --output-on-failure --output-junit kmc-final-ui.xml
ctest --test-dir build/reference-core --output-on-failure --output-junit kmc-final-core.xml
```

Core cache yoksa ayrı configure: `VISIONFLOW_BUILD_UI=OFF`, `VISIONFLOW_WITH_ZXING=OFF`, aynı Qt/OpenCV/MinGW yolları; UI ON pinned ZXing yolu korunur. Var olmayan build dizinine test koşup kabul sayılmaz. Shell exit code ve CTest XML birlikte kaydedilir.

Her paketin log/XML/fixture/görselleri `build/kmc/<paket>/` altında; UI testleri kendi INI dosyasını kullanır. `config/local.ui.ini` değiştirilmez. Offscreen görüntü görsel QA içindir; native DPI/gerçek kamera kabulü ayrıca belirtilir. Ağ sandbox hatasında ürün kapısı gevşetilmez; gerekli erişimli koşum ayrı kanıt olarak yazılır.

## 9. GPT‑6.1 Sol medium için çalışma biçimi

- Bir oturumda bir paket veya büyük paketin tek alt hedefi. Paketlerin acceptance listesini önce oku, sonra o paketin kaynaklarını aç. Bütün geçmiş progress'i çıktı limitine taşırarak okumak yerine güncel başlık ve ilgili tarihsel kayıtları hedefle.
- Kaynak incelemesinde bu planın bulgularını doğrula. Mevcut çalışma ağacındaki kullanıcı değişikliklerini koru. Belge taslağını uygulanmış durum diye kabul etme.
- Paket bittiğinde gerçek test kanıtı + bilinen sınır + sıradaki paketle progress'e yaz. Başarısız test veya eksik kabul varken paketi kapatma.
- Bittiğinde bir sonraki paketi aynı oturumda kendiliğinden başlatma; kullanıcı devam dediğinde progress'teki ilk tamamlanmamış paketten sürdür. Bu, bütün planın tek büyük değişiklik haline gelmesini önleyen oturum kapsamıdır.
- Sahte sonuç/işlevsiz etkin düğme/disabled test ile ilerleme yok. Programatik altyapı teslimi ayrı, kullanıcı özelliği ayrı ifade edilir.
- Kullanıcı açık istemedikçe yeni agent/chat oluşturma, commit/push/deploy veya SDK kurulumu yapma. Sohbeti temizleme aracı yoksa temizledim deme; devir notunu yaz.

### İlk oturuma kopyalanacak talimat

```text
C:\Projects\Tvision için AGENTS.md, docs/progress.md, README.md,
docs/windows-workstation.md ve
docs/architecture/independent-calibration-implementation-plan.md dosyalarını oku.
GPT-6.1 Sol / medium ile çalışıyorum. Bu oturumda yalnız KMC-0 paketini tamamla.

Kullanıcı Sistem özeti tasarımını seçti. Kamera sayısı sabit değil: çoğu makine
tek kamera, bazıları 8–10 kamera. Beş kamera yalnız örnektir.
Reçetesiz kalibrasyon ve kurulum başına etkinlik A tesliminde; tek reçetede
N görüntü kaynağı B teslimindedir. Mevcut kod tek kaynak kabul ediyor.

Önce dirty çalışma ağacını ve build ortamını denetle. KUI-5'i yeniden yazma.
Bilinen dört UI regresyonunu tekrar üret, nedenlerini belirle ve doğruluk
kapılarını gevşetmeden düzelt. Release, ilgili CTest ve UI görsel kontrolünü
tamamla; ardından baseline tam test kanıtını kaydet. Ürün ve test fixture'ı
arasındaki eski beklenti farklarını gerekçeli ele al. Yerel kullanıcı INI'sini koru.

Yalnız KMC-0 kapsamında ilerle. docs/progress.md'ye kapsam, değişen dosyalar,
komutlar/sonuçlar, bilinen sınırlar ve sıradaki KMC-1 adımını yaz.
KMC-1'i bu oturumda başlatma. Donanım, CUDA, dağıtım veya tam KUI-6 kabulünü
gerekli kanıt yokken tamamlandı sayma. Kullanıcının önceki değişikliklerini koru.
```

### Sonraki oturumlara kopyalanacak talimat

```text
AGENTS.md, docs/progress.md, README.md ve
docs/architecture/independent-calibration-implementation-plan.md dosyalarını oku.
Progress'te KMC planının ilk tamamlanmamış paketini bul; tamamlananları tekrarlama.
Bu oturumda yalnız o paketi, belirtilen kapsam ve kabul koşullarıyla tamamla.
Kamera sayısı N; tek kamera akışını sade tut, 8–10 kamera testlerini koru.
Sistem özeti seçilmiş tasarımdır. UI, algoritma/SDK'yı doğrudan çağırmaz.
Legacy schema/hash, source/frame/cycle/coordinate, bağımsız kontrol ve üretim
trusted completion güvencelerini koru. UI değişiminde Release/CTest/görsel kontrol yap.
Sonunda progress ve ilgili README/mimari durumunu gerçek uygulamayla eşleştir;
sıradaki paketi belirt, aynı oturumda ona başlama. Eksik kabulü tamamlandı gösterme.
```

## 10. Devir özeti biçimi

Her paket için: tarih, paket ID/durum, ürün davranışı, değişen dosyalar, schema/engine değişimi varsa gerekçesi, tam komut ve exit/test sayısı, kanıt yolları, config koruması, bilinen sınırlamalar, sıradaki paket. Plan tablosu değil progress uygulama durumunun kaynağıdır.

Görsel referans: [kaba tasarım](independent-calibration-design.md), sohbetin `calibration-workspace.html` taslağındaki **Sistem özeti** alternatifi; yerel PNG `build/design-calibration/overview.png`. Taslak beş örnek kart içerir; uygulanacak UI N kamera yerleşimidir. Taslakta her adımın örnek olması, gerçek hesaplama/edinim yerine örnek veri koyma izni vermez.
