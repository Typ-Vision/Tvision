# Onaylanan geliştirme ve mimari güçlendirme planı

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

8 Ekim 2026 P3/P4 tamamlandı: bağımsız socket, kalıcı enabled/autoStart/endpoint/ACK ayarları, reçetede Manual/SoftwareTrigger ve Measurement UUID mapping; vf.socket/2 INVALID ve coordinator teslim/cancel korumaları uygulanmıştır. Run Tool aynı ham karede yalnız seçili executor’ı çalıştırır; doğrulanmış upstream cache kullanır, eksik/stale girdide acquisition veya full-flow fallback yapmaz. Kaynak için Görüntü al, tüm akış için Run Flow kullanılır. P5/P6 tamamlandı; güncel Blob ve kabul bilgisi yukarıdadır. K4 ertelenmiş, P7 restart recovery açıktır.


8 Ekim 2026 P2 tamamlandı: Recipe Manager tek tıkla yalnız satır seçer; **Load Recipe / Edit Recipe** ayrıdır. Load kaydedilmiş bağımsız kopyayı statik kontrol eder; dirty durumda Kaydet ve yükle / Kaydedilmiş sürümü yükle / İptal sunulur. Runtime **Run / Stop / Tek çevrim** kullanır; Run görüntü veya parça sonucu üretmeden coordinator ile otomatik hazırlanır. Manuel çevrim, cooperative Stop ve aynı terminal kayıt retry coordinator üzerinden geçer. Seçili, editörde açık ve Runtime yüklü reçete ayrıdır. Eski validate/activate/rollback düğmeleri kaldırıldı; rollback servis API’si korunur. Bu P2 teslimi P3/P4 ile genişletildi; güncel socket ve Run Tool davranışı yukarıdadır. Fiziksel kabul/K4 ertelenmiş kalır.

8 Ekim 2026 son kullanıcı yönlendirmesi: Sıradaki geliştirme **P1–P6 Runtime/Recipe Load/Run, bağımsız socket, Editor Run Tool ve Blob** paketleridir; ardından mevcut 6D.2/P7 gelir. [Ayrıntılı plan ve kabul koşulları](runtime-editor-workflow-plan.md) bu belgedeki eski sıra ve düğmelerin değişmezliği ifadelerine göre bu kapsamda önceliklidir. P1–P6 uygulandı; sıradaki P7; hardware trigger/OPC UA/QR P8–P9 gelecek kapsamıdır.

8 Ekim 2026 revizyonu: İki kalibrasyon akışı Camera Calibration ve Robot Calibration olarak korunur. Mevcut uniform scale temelinden sonra **6C-K geçici test görüntüsü import/plaka tespiti → tek görüntüden planar homography → Measurement/Runtime → fiziksel kabul (gerektiğinde ayrı lens düzeltmesi)**, ardından 6D PLC ve 6E Eye-to-Hand/Eye-in-Hand gelir. 6C-K1–K3 target/import/köşe tespiti, tek görüntü raw homography/mm denemesi ve tipli kayıt/Measurement/Runtime tamamlandı. K4 fiziksel kurulum hazır olana kadar kullanıcı isteğiyle ertelendi; lens ihtiyacı verilerle belirlenecek. Kullanıcı TCP/IP socket önceliğiyle 6D PLC geliştirmesini yetkilendirdi. 6D.1 gerçek localhost socket/Runtime/ACK ve simülatör yazılım kabulü tamamlandı; 6D.2 restart teslim kurtarma ve fiziksel PLC kabulü açık. [Ayrıntılı plan](calibration-plan.md).

7 Ekim 2026. Kullanıcı genel mimari incelemesindeki önerilerin tamamını onayladı. A1–A9 çekirdek/sözleşme ve plan teslimleri tamamlandı. Güncel kullanıcı yönlendirmesiyle webcam tek kare → DAG ve bağımsız 6C planar scale/Measurement/tolerans servis çekirdeği teslim edildi; 6C kalibrasyon persistence/current resolver ve reçete binding servisleri de tamamlandı; ortak Measurement tool/Properties ve planar Calibration UI tamamlandı; yerel servis yetkili production validation/Runtime/kalıcı snapshot-event-cycle kayıtları tamamlandı; sıradaki 6C-K plaka tabanlı kamera/düzlem kalibrasyonu; ardından 6D PLC handshake ve simülatör. Endüstriyel kamera kabulü açık. Fiziksel kamera ve sonraki üretim aşamaları henüz tamamlanmış değildir. Güncel sıra için bu belge ve `../backlog.md`, tamamlanmış işlerin kanıtları için `../progress.md` esas alınır. Önceki teslim kayıtlarındaki “sonraki adım” ifadeleri tarihsel kayıttır.

## Korunacak ürün ve GUI kararları

8 Ekim güncel teslim: **6D.1 TCP/IP socket → immutable Runtime → kalıcı sonuç → ACK ve ayrı PLC simülatörü tamamlandı**. UI Release 20/20, Widgets OFF 18/18; [wire/limit/kayıt sözleşmesi](plc-socket.md). K4 kullanıcı isteğiyle ertelendi. Sonraki 6D.2 kalıcı dedup/outbox ve restart sonrası belirsiz teslim kurtarma; hedef PLC eşleştirmesi/fiziksel kabul açık. Bu yeni kullanıcı yetkisi önceki kalibrasyon kapsam sınırını 6D için genişletir; robot 6E henüz başlanmadı.

- Mevcut GUI görünümü, tema, panel yerleşimleri, menü adları, menü sırası ve navigasyon korunur. Bu plan bir GUI yeniden tasarımı veya yeni menü/node canvas çalışması değildir.
- Mevcut düğmeler ve ekranlar aynı yerlerinde kalır. Refactor davranış tutarlılığını düzeltir; yeni servisler mevcut ekranlara bağlanır. Runtime/Communication işlevleri mevcut ayrılmış ekranlarda geliştirilecek. Calibration legacy uniform scale ve raw planar homography için test import/köşe/H/mm/holdout/kayıt sunar; lens correction/robot modeli henüz yok, fiziksel kabul açık.
- Üç applicationType tek Recipe Editor, ortak engine ve tek Runtime kullanır. UI algoritma veya donanım SDK'sı çağırmaz.
- Reçete durumu, execution durumu ve parça kararı ayrı kalır. Gerçek sonuç olmadan PASS, skor veya robot pose gösterilmez.
- Tamamlanan Adım 1–5 ve 6.1–6.2 yeniden yapılmaz. Büyük bir yeniden yazım yerine mevcut servislerin aşamalı ayrıştırılması tercih edilir.

## Geliştirme sırası ve çıkış koşulları

| Aşama | Kapsam | Çıkış koşulu |
| --- | --- | --- |
| 6A — Mimari güçlendirme | A1–A9: yürütme tutarlılığı, referans invalidation, tool/frame/session sınırları, üretim ve kayıt sözleşmeleri, bağımsız test yapısı | Davranış düzeltmeleri ve gerekli çekirdek sınırlar uygulanmış; kamera, koordinat ve üretime alma sözleşmeleri tanımlı; mevcut kabul testleri korunmuş |
| 6B — Tek gerçek kamera | GenApi/runtime veya vendor C API seçimi, özellikler, trigger/stream/grab, owned buffer dönüşümü, kamera yapılandırmasının kaydı | Gerçek producer/modelle trigger → owned frame → mevcut DAG çalışır; disconnect, geç frame, Stop, yeniden bağlanma ve kuyruk davranışı sınanır |
| 6C — Measurement ve ilk Runtime | Pixel ölçüm → kalibrasyon → tolerans → doğrulama → değişmez üretim reçetesi → mevcut Runtime ekranı | Ham değerle tolerans; geçersiz kalibrasyonda sonuç engeli; taslak düzenlemesi çalışan sürümü değiştirmez; servis yetkisi ve cycle logları çalışır |
| 6C-K — Plaka tabanlı kamera kalibrasyonu | Target/model ve geçici import, tek görüntü H, Measurement/Runtime/kayıt; gerektiğinde lens desteği ve fiziksel kabul | Bağımsız mm doğruluğu, lens/perspektif dönüşümü ve dependency reddi; ayrıntılar calibration-plan.md |
| 6D — PLC sonuç teslimi | Request/cycle ID, Ready/Busy/Done/ACK, mapping, timeout, tekrar istek ve yeniden bağlantı | Sonuç doğru isteğe bir kez mantıksal olarak teslim edilir; duplicate/retry ve ACK kaybı politikası test edilir; hata PASS olmaz |
| 6E — Robot guidance | Koordinat dönüşümü, versioned calibration, pick reference ve robot çıktısı | Bilinen örneklerle dönüşüm/offset doğrulanır; geçersiz veri pose üretmez; sonuç gönderimi ve hareket komutu ayrıdır |
| 7 — Ürünleştirme | Ayrıntılı Diagnostics/Users/Logs, audit/retention, revizyon geçmişi ve paketleme tamamlamaları, app-data, dağıtım ve native kabul | Kurulum/bağımlılık/lisans envanteri, recovery ve hedef makine kabul kanıtları |
| Sonraki genişletmeler | Gelişmiş tool'lar, açı/ölçek, çoklu sonuç, OCR/AI, çoklu kamera, GPU | Her özellik için gerçek veri doğruluğu ve performans kabulü; GUI değişikliği bu onayın kapsamında değildir |

6A tüm gelecek özelliklerin implementasyonu değildir: kayıt paketi ve koordinat sözleşmesi burada kararlaştırılır; kalibrasyon/üretim uygulaması 6C'de, çoklu kamera uygulaması sonraki genişletmelerde yapılır. Fiziksel kamera bulunamazsa 6B kabulü açık kalır; dosya kaynağıyla bağımsız 6C servis işleri ilerleyebilir. Sentetik producer fiziksel kabul yerine geçmez.

## Onaylanan iş paketleri

### A1 — Run Match / Run Flow tutarlılığı — tamamlandı (7 Ekim 2026)

Run Match ve Run Flow ortak ExecutionService ve UI worker yaşam döngüsünü kullanır. Run Match seçili tool ve gerekli upstream alt grafiğini çözer; ilgisiz dalları çalıştırmaz. Image/Region, disabled/missing upstream, not-found ve iptal kuralları ortaktır. İki UI komutu da kaynağı yeniden edinir; servis aynı immutable frame snapshot ile iki kapsamın karşılaştırılmasını destekler. Referans cache kimliği frame generation, proje/reçete/path ve upstream tool/bağlantı yapılandırmasını kapsar. Kabul kanıtı progress.md içindedir.

Kabul: Aynı frame/config snapshot'ında bağlı ROI ile tek tool ve tam flow aynı sonucu verir. Eksik/disabled/geçersiz upstream geçerli sonuç üretemez. Görüntü veya ayar değişiminde eski sonuç reddedilir. Run Match düğmesinin yeri ve görünümü değişmez.

### A2 — Model/geometri değişiminde referans invalidation — tamamlandı (7 Ekim 2026)

Referans öğretimi yalnız tool ID ve referenceOrigin ile sınırlı kalmayacak; model/geometri sürümü veya içerik kimliğiyle ilişkilendirilecek. Referansın geometrisini değiştiren yeniden öğretim, bağlı ROI'yi yeniden öğretim gerekli durumuna geçirecek. Eski offset ile sessiz yürütme olmayacak. Geometriyi değiştirmeyen metadata değişiklikleri gereksiz yeniden öğretim istememeli. Aynı bağımlılık yaklaşımı kalibrasyon sürümlerinde kullanılacak.

Kabul tamamlandı: Schema 5 referenceGeometry kimliği source UUID/kind + Match modelRoi/modelPng SHA-256 değerine bağlıdır. Model merkezi veya içeriği değişirse Edge ERROR, downstream ölçüm SKIPPED olur. Search ROI yeniden çizimi ilişkiyi günceller. Metadata/eşik/Match search ayarı değişimi kimliği değiştirmez. Save/reopen ve schema 3/4 migration kabulü, UI teaching/recovery ve Release CTest 6/6 doğrulandı. Eski şemalarda geçmiş değişim kanıtlanamaz; mevcut model baseline kabul edilir. Kalibrasyon sürüm bağımlılığı 6C'de uygulanacaktır.

### A3 — ToolRegistry ve ayrı yürütücüler — tamamlandı (7 Ekim 2026)

Tool türü, sabit port sözleşmesi, varsayılanlar, parametre doğrulaması ve sürüm bilgisi ortak registry üzerinden erişilir. Match/Blob/Edge/Calculate yürütücüleri engine sıralama kodundan ayrılır. JSON kalıcı format olarak kalabilir; çalışma sırasında doğrulanmış parametre modelleri kullanılır. Properties mevcut görünümünü koruyarak kayıtlı tool tanımını veya araca özel presenter'ı kullanır.

İlk uygulama statik registry'dir; DLL plugin yükleme zorunlu değildir. Bilinmeyen tür açık hata üretir, başka algoritmaya düşmez. Kabul: test tool'u eklemek engine'in merkezî kind koşullarını değiştirmeyi gerektirmez; mevcut port/sonuç davranışları korunur.

### A4 — MainWindow'dan session/controller ayrımı — tamamlandı (7 Ekim 2026)

EditorSession/ExecutionController ve CameraSession gibi bileşenlerle çalışma snapshot'ı, worker yaşam döngüsü, iptal, cache ve invalidation UI'dan ayrılır. Referans öğretimi gibi iş kuralları application katmanına taşınır. Runtime aynı servisleri kullanır. Ayrı busy boolean'larının geçişleri açık bir iş/oturum durum sözleşmesiyle yönetilir. UI yalnız komut gönderir ve durumu sunar.

Kabul: Proje/reçete değişimi, Stop, eski worker sonucu ve kapanış senaryoları servis testleriyle doğrulanır; mevcut GUI ve session restore korunur. UI smoke yardımcıları üretim iş kurallarının sahibi olmaz.

### A5 — Kaynaktan bağımsız frame ve metadata — tamamlandı (7 Ekim 2026)

ImageFrame dosya servisinden ayrılır. Kamera ve dosya adapter'larının kullanabileceği kontrollü oluşturma yolu, buffer sahipliği, stride ve piksel formatı tanımlanır. Ham boyut önizlemeye bağımlı olmaz; preview ayrı sunumdur. Kaynak ID, frame ID, cycle ID, edinim zamanı ve varsa cihaz zamanı taşınır; exposure zamanı ile completion zamanı karıştırılmaz.

Frame ID her edinime aittir; cycle birden fazla frame içerebilecek şekilde modellenir. Sonuçların image kökeni ile geometrik koordinat sistemi ayrı kavramlardır. Aynı cycle farklı kameraların noktalarını otomatik uyumlu yapmaz. Immutable paylaşım ve SDK buffer'ının yeniden kullanımı açıkça güvenceye alınır.

Kabul: Sahte adapter buffer'ı serbest bırakıp yeniden kullansa da owned frame değişmez; Gray8/Gray16/renk/stride korunur. Source/time metadata execution sonucuna ulaşır. Uyuşmayan frame/koordinat girdileri reddedilir; çoklu kamera özelliği henüz teslim edilmez.

A3–A5 kabulü: statik built-in registry + startup compiled extensions; engine tür koşulu eklenmeden test tool yürütmesi; typed Match/Blob/Edge ayarları. EditorSession/CameraSession job tickets, state/future/cancel, stale-result, cache/teaching sahipliği; UI yalnız presentation callbacks ve düzenleme komutlarını taşır. Owned ImageFrame fromBuffer, optional preview, source/frame/cycle/coordinate/time metadata ve input mismatch engeli. Release CTest A3 6/6, A4 7/7, A5 8/8; GUI görselleri korundu. Ayrıntılar progress.md ve tool-execution.md. Tek file source, cooperative cancellation ve fiziksel kamera sınırları sürer.

### A6 — Kamera yaşam döngüsü ve durdurma sözleşmesi — çekirdek tamamlandı (7 Ekim 2026)

Bağlanma, yapılandırma, stream başlatma, grab, durdurma, hata ve yeniden bağlanma durumları tanımlanır. Deadline ile geç sonucu reddetmek, SDK çağrısını kesmekten ayrı kabul edilir. Durdurma/kapanış bütçesi, sınırlı kuyruk kapasitesi, doluluk/drop politikası ve geç frame davranışı açık olur. Preview/live ile üretim trigger kuyruğu aynı politika varsayılmaz.

Mevcut process-geneli GenTL kilidi kalıcı çoklu cihaz mimarisi sayılmaz. Producer ve cihaz oturum sahipliği gerçek SDK thread/lifetime kurallarına göre tasarlanır. SDK çağrısının kesilebilirliği fiziksel testle kanıtlanamazsa ayrı kamera süreci ve kontrollü yeniden başlatma değerlendirilir; süreç ayırma peşinen zorunlu değildir. Kabul 6B'de gerçek donanımla tamamlanır.

A6 çekirdek kabulü: CameraLifecycle owner-thread komut/ticket ve stream generation, explicit Fault/cleanup/reconnect; preview oldest-drop ve production FIFO Full/metadata/deadline kontrolü; AcquisitionRequest mutlak monoton deadline ve JobSession bloklamayan shutdown bütçesi. Release CTest 9/9 ve mevcut GUI görsel kontrolü geçti. Mevcut GenTL discovery/read-only sahipliği korunur; yeni controller SDK stream'e henüz bağlı değildir. 1000 ms Stop/kapanış gözlem bütçesi hard interruption/exit garantisi değildir. Ayrıntılar [acquisition.md](acquisition.md), fiziksel kabul ve SDK cleanup testleri 6B'de açık.

### A7 — Ortak koordinat ve tracking ROI sözleşmesi — çekirdek tamamlandı (7 Ekim 2026)

Pose2D, koordinat sistemi ID'leri, kaynak/hedef frame, birim, dönüşüm yönü ve lokal offset tanımlanır. Tracking ROI araçtan bağımsız aynı dönüşümü kullanır. Açılar, eksen yönleri ve piksel merkezi konvansiyonu açık olur. Kalibrasyon kimliği/sürümü, kamera/lens/çözünürlük ve çalışma düzlemi bağımlılıkları kaydedilir. Mevcut translation-only Edge davranışı migration ile korunur.

Kabul: Öteleme/dönme ve dönüşüm sırası bilinen geometrilerle test edilir; uyumsuz frame/birim reddedilir. 6C'de kalibrasyon değişimi sonuçları geçersiz kılar; 6E'de robot frame dönüşümleri aynı sözleşmeyi kullanır. 2D veriden kanıtsız tam 3D pose türetilmez.

A7 çekirdek kabulü: domain CoordinateFrame/Pose2D/Transform2D ve dört köşeli TrackingRoi; inverse/composition, px/mm ve Y-down/Y-up doğrulaması. Edge legacy point takibi aynı dönüşüme bağlı; schema 3–5 migration/roundtrip ve bir kez delta yuvarlama korunur. CalibrationDependency kamera/lens/çözünürlük/düzlem/kimlik/sürüm/frame bağımlılıklarını schema 1 JSON olarak taşır ve değişimi reddeder; reçete/kalibrasyon yürütmesi 6C'dedir. Release CTest 10/10, güncel Edge/measurement görselleri incelendi. Pose portu/açı tespiti/dönmüş raster crop/robot çıktısı uygulanmadı. Ayrıntı: [coordinates.md](coordinates.md). A8 çekirdek kabulü aşağıdadır.

### A8 — Taslak / doğrulanmış sürüm / aktif üretim ayrımı — çekirdek tamamlandı (7 Ekim 2026)

Draft düzenleme modeli, doğrulama kaydı ve Runtime'ın kullandığı değişmez snapshot ayrılır. Doğrulama reçete içerik kimliği, model/varlık, kalibrasyon ve engine/tool sürümlerine bağlıdır. Diskte VALID yazması tek başına üretime alma izni değildir. Aktivasyon, rollback ve geçmiş sürüm saklama politikaları tanımlanır. Çevrim boyunca aktif sürüm değişmez; yeni sürüme geçiş sınırı açıktır.

Temel servis yetkilendirmesi, yapılandırılmış hata kodları ve cycle logları 6C kabulünün parçasıdır; ayrıntılı Users/Diagnostics ekranlarını beklemez. Loglar recipe/version/config, cycle/frame/source, calibration/version ve sonuç/iletişim durumunu ilişkilendirir. PASS/FAIL ile ERROR/SKIPPED ayrı kalır.

Kabul: Draft değişimi çalışan snapshot'ı etkilemez; değişmiş bağımlılıkla eski doğrulama kullanılamaz; yetkisiz aktivasyon servis tarafından reddedilir. Çevrim kayıtları hangi sürümün çalıştığını tekrar izlemeye yeterlidir.

A8 çekirdek kabulü: ProductionRecipeService immutable snapshot ve servis tarafından oluşturulan execution_configuration doğrulaması; içerik/model/engine/tool/asset/calibration dependency hash, varsayılan ret policy, yetkili activation/rollback ve tek cycle version boundary. Sonuç/ticket/provenance kabulü ve transient cycle trace, bounded history/log testleri; Release CTest 11/11. Runtime/Users/kalıcı log ve gerçek üretim doğrulaması 6C'de bağlı değildir. Ayrıntı [production-recipes.md](production-recipes.md). A9 teslimi aşağıdadır.

### A9 — Persistence, migration ve bağımsız build/test — tamamlandı (7 Ekim 2026)

Tek JSON + base64 model + 16 MB sınırı mevcut uygulanmış formattır. Büyüme için manifest ve kimlikli varlık paketleme, bütünlük kontrolü ve boyut sınırları tasarlanır. Format değişimi yalnız limiti kaldırarak yapılmaz; atomik commit, başarısız kayıtta eski veri ve dirty durumunun korunması devam eder. Şema migration'ları açık, sıralı ve test edilebilir olur; future-schema sessizce kaydedilmez. Geçmiş snapshot ve taşınabilir paket uygulamaları 6C/7 ihtiyaçlarına göre tamamlanır.

CMake domain/application/engine testlerini Widgets uygulamasından bağımsız derleyebilmeli; Qt Core bağımlılığı tek başına hata sayılmaz. UI smoke ayrı kalır. Yeni abstraction'lar için anlamlı testler, Windows Release CI ve tekrarlanabilir toolchain yapılandırması planlanır. Çalışma klasöründe Git deposu bulunmadığından başka yerde kaynak sürümleme olup olmadığı doğrulanır; yoksa yerel Git kurulur. Uzak depoya yayın bu planın parçası değildir.

Kabul: Eski dosyalar ve modeller korunarak migration/roundtrip; eksik/bozuk asset ve yarım kayıt hata senaryoları; UI kapalı çekirdek build/test; kaynak sürümleme konumunun kaydı. Mevcut araç sayısı ve model boyutlarında performans regresyonu kontrol edilir.

## Doğrulama ve kapsam disiplini

Her uygulama adımında ilgili core testler ve mevcut regression testleri çalışır. UI bağlantısı/refactor'ı olduğunda Release, ilgili CTest ve görsel karşılaştırmayla mevcut görünüm/menülerin korunduğu doğrulanır. Ayarlar ve test görselleri build altında kalır; config/local.ui.ini korunur. Gerçek üretim görüntüleriyle yanlış kabul/ret, konum hatası ve hedef donanım latency ölçülür. CUDA, kurulum ve fiziksel kamera başarısı ayrı kanıt gerektirir.

Bu plan onayı kod değişikliği yapıldığı, kamera kabulünün tamamlandığı veya GUI yeniden tasarımının onaylandığı anlamına gelmez. Her teslimde progress, backlog ve ilgili uygulanmış sözleşme güncellenir.

A9 teslim kanıtı: [persistence-plan.md](persistence-plan.md) manifest/hash/byte bütçesi, tek container atomik commit/recovery ve sıralı migration tasarımını belirler; mevcut format değişmedi. Widgets OFF Release core 9/9, UI ON 11/11 CTest; staging/cancel ve oversized persistence testleri eklendi. Windows doğrulama scripti temiz iki build üzerinde geçti; uzak CI yok. Yerel VisionFlow Git deposu kuruldu; TVis ayrı proje. Paket entry/import/retention ve kalıcı snapshot uygulaması 6C/7 kapsamındadır.


## 6C ilk Runtime teslimi

Mevcut Runtime/Logs ekranları aynı engine/resolver/snapshot yoluna bağlıdır. OS yerel Engineer policy, fresh frame validation, activation/rollback/cycle boundary, gerçek mm/PASS/FAIL, Stop ve başarısız kayıt retry kabulü; UI 16/16 ve Widgets OFF 14/14. Yapılandırma kabulü fiziksel lens/ölçüm kabulü değildir. Users/Operator ayrımı, sürekli stream, global recipe decision, PLC, robot, recovery/retention/paketleme açık kalır. Sonraki 6D komutu için kullanıcı onayı beklenir.

## 8 Ekim — Manuel ölçek uygulama planı hazır

[Manuel px/mm ve sade Calibration planı](manual-scale-calibration-plan.md) önceki kısa kapsam notunun ayrıntılı devamıdır. C1 matematik/codec → C2 katalog/Measurement/Runtime → C3 sade ekran → C4 UI kabul → C5 birleşik test/belgeler. C1 matematik/codec tamamlandı; C2 katalog/Runtime/CSV ve C3–C5 ekran/kabul işleri açık. Yeni manual_planar_scale/1, record/dependency schema 3, eski hash/encode koruması, beş bölümlü üç yöntem UI ve doğrulamasız deneme/üretim ayrımı tanımlandı. Belgenin 7. bölümü GPT-6.1 Sol medium için kopyalanabilir başlangıç talimatıdır. P7 bu kapsamdan sonra; K4 ertelenmiş kalır.