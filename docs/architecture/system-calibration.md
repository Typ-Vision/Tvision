# KUI-5 — Ortak etkin kalibrasyon, Editor ve immutable Runtime

15 Ekim 2026. KUI-4 ana sayfa/sihirbazların etkin seçimi artık SystemCalibration application servisi üzerinden metrik tüketicilere bağlanır. UI SDK/algoritma çağırmaz. Üç applicationType tek Editor ve Runtime yolunu kullanır. Fiziksel doğruluk, lens düzeltmesi, robot hesabı/hareketi ve CUDA/kurulum/dağıtım kabulü değildir.

## CCUX-18 güncel UI ve sahiplik

22 Ekim 2026: eski ortak ana sayfa ve kanıt ekranları kaldırıldı. Ürün gezinmesi
kamera merkezli `CalibrationOverview` ve iki mevcut sihirbazdır. Eski ortak katalog
`MeasurementWorkspace::legacyCalibrations()` içinde proje ömrüyle yaşar; Editor ve
Runtime resolver'ları widget üretimine bağlı değildir. Proje değişimi bu oturum
iznini düşürür. `.vfcalstudy`, eski model/proje/reçete codec ve hash'leri korunur;
sistem politikası açılmış eski katalogda restart otomatik fallback/etkinlik sağlamaz.
Gerekli ilişkilendirme güncel kamera sayfasının mevcut eski kayıt eşleme komutundadır.
Aşağıdaki KUI-5 teslimindeki ana sayfa/dialog tarifleri tarihsel UI'dir.

## Politika ve birimler

Ana sayfadan ilk açık etkinleştirme katalog revision'ını artırır ve proje için sistem politikasını açar. Daha önce bu işlem yapılmamış eski projeler legacy binding yolunu korur. Sistem politikası açıldıktan sonra devre dışı bırakma/restart/uyumsuzluk eski recipe binding'e geri dönmez. Restart son seçimi yalnız hatırlatır; ana sayfadan yeniden etkinleştirme gerekir. Proje değişimi etkin izni kaldırır. Yeni aday kaydetmek aktif seçimi/cache'i değiştirmez.

Measurement, kalibre Caliper, Line Fit, Circle/Arc ve Geometry Measurement aynı bağlamı tüketir. Uniform model gerçek iki noktanın uzaklığını; H modeli her noktayı ayrı world düzlemine eşleyerek mesafe/refit/metrikleri hesaplar. H hull dışı, kaynak/config/çözünürlük uyumsuzluğu ve eksik etkinlik mm/karar üretmez; ölçeğe fallback yoktur. Edge Point raw `point` portunu korur ve ayrı typed `pointMm [mm/plane]` verir; desteklenmeyen/uyumsuz durumda yalnız bu fiziksel port invalid kalabilir. World reference executor da gerçek raw/world noktalarını ayrı sunar.

Calculate px olarak kalır. Pattern/Shape/Code/Blob konum/score/piksel alanı keyfi şekilde fiziksel birime çevrilmez. Ana sayfa **Etki incelemesi** bütün reçetelerin kaynak/kurulum, eski px tolerans ve exact world ROI durumunu gösterir. Yeni UI'dan eklenen Caliper/Line/Circle metrik modda başlar; eski kayıtlar otomatik değiştirilmez. Mevcut px toleransının sayısal değeri mm diye yorumlanmaz. Circle'ın mm sampling/fit başlangıç değerleri yalnız yeni araç oluşturma yolunda açıkça atanır.

## Asıl kanıt ve yürütme bağı

`CalibrationModelRecord` içindeki original model, öğretim kaynak ID/frame/hash ve bağımsız kontrol kanıtı değişmeden korunur. `SystemCalibration` owner thread'de yakalanmış immutable `ActiveCalibrationSnapshot` değerini alır. Her gerçek frame için bağımsız kaynak adapter/config, actual source tool ID, çözünürlük, raw koordinat alanı ve varsa eski lens/düzlem beyanını denetler. Aynı boyut tek başına uyum sayılmaz. Aynı fiziksel kuruluma ait farklı reçete tool ID'leri desteklenir; Webcam index kalıcı fiziksel kimlik değildir.

Sistem politikasının transient execution dependency hash'i; wrapper contentId, seçimin ID/sürüm/revision'ı, actual source tool ID ve raw coordinate ID'den türetilir. Bu bağımlılık asıl modelin hash'i diye sunulmaz. Yalnız kaynak/koordinat **adlarını** bağlayan, geometriyi değiştirmeyen private bir execution adapter oluşur; original model/kaynak kanıtı yeniden etiketlenmez. Full raster px geometrisini koruyan Preprocess aynı bağı kullanır.

`PlanarCalibration::validated()` eski bağımsız holdout durumudur. Yeni `measurementReady()` yalnız eski gerçek validation veya sealed transient sistem bağı ile sağlanır. Writable `validated=true`, sahte holdout veya sıfır fiziksel hata eklenmez. Transient adapter eski `MeasurementService::encode` yoluyla kaydedilemez; kalıcı kanıt original model-ready record + selection + executionDependency zarfıdır. Worker canlı katalog/UI nesnesi tutmaz.

## Editor ve world ROI

EditorContext/evaluation signature sistem content/revision kimliğini taşır. Etkin değişimde `EditorSession::calibrationChanged` eski mm sonuçları ve bunların downstream cache'lerini atar; ham frame, acquisition ve uygun raw sonuçları yeniden imzalar. Edge Point'in eski pointMm çıktısı silinir, raw point korunur. TargetOnly yeni modelle aynı raw endpoint cache'ini kullanır; acquisition fallback yapmaz. Geç eski completion yeni signature'a kabul edilmez.

World ReferenceBinding/WorldPoseTracking exact dependency kontrolü sürer; başka modele otomatik bağlanmaz. Standalone nominal mm Circle ROI için opt-in editor8 `worldCalibration` dependency eklenir. Yeni modelde eski/eksik dependency açık yeniden öğretim ister. UI çizimi/koordinat uygulaması bunu gerçek current context ile birlikte kaydeder. Yeni world Circle referans öğretimi aynı bağı taşır; referans kaldırılınca rastgele başka modele geçmez. Kalibre olmayan eski px ROI sayıları değiştirilmez.

## Runtime ve geçmiş

RuntimeService owner thread provider'dan etkin immutable seçimi alır. Saved recipe dosyasını değiştirmeden production kopyasına explicit execution binding ve recipe schema2 projeksiyonu uygulanır. Snapshot dependencies asıl record/selection/revision ve derived dependency'nin tamamını içerir. Run yapılandırma hazırlar; görüntü veya parça sonucu üretmez. Worker yalnız admitted snapshot'ı kullanır. Etkin seçim veya source/config değişirse önceki hazır snapshot yeni çevrim başlatamaz; tekrar hazırlanır. Admitted çevrimdeki model değişmez.

Runtime çalışırken, worker veya pending kayıt/ACK varken ana sayfa değiştirme kapısı kapalıdır. Stop sonrasında immutable eski snapshot bellekte dursa bile yeni açık seçim yapılabilir; yeni çevrim için güncel hazırlık gerekir. Raw-only Runtime kalibrasyon yokken çalışabilir; mm isteyen reçete hazırlanamaz. Raw-only projeksiyonda da eski binding'e fallback yapılmaz. Opsiyonel fiziksel Edge Point çıktısı uygun aktif model yoksa invalid kalır.

Yeni sistem terminal kaydı **schema5**: immutable original record/selection/execution binding, dependency SHA256 ve varsa world reference kanıtı birlikte denetlenir. Bütün sistem çevrimleri trusted in-process completion digest/ticket/snapshot kapısından geçer; değiştirilmiş veya Editor sonucu kabul edilmez. Recording lock/failure yeni çevrimi bloke eder; aynı trusted sonucu tekrar yazma yolu korunur. Stop gerçek mm/PASS/FAIL'i temizler.

Logs ve CSV, çevrimde kullanılan geçmiş modeli taşır; yeni modele göre tekrar yorumlamaz. CSV sistem kayıtlarında ek `systemCalibration` sütununda full zarfı verir; yalnız eski kayıtların CSV başlığı korunur. Terminal1–4/read/restart yolları sürer, eski permission restore edilmez. Archive hash/append-only/lock ve mevcut boyut sınırları değişmez.

## Legacy kayıt aktarımı

Ana sayfadaki **Eski kayıtları incele / aktar** dialog'u eski katalog kayıtlarını gösterir. Kullanıcı hedef ortak kurulum adını ve aynı lens/düzlem/çözünürlük kurulum beyanını açıkça verir. Actual source acquisition/config/resolution denetlenir. `CalibrationMigration` seçilen ID'nin **bütün sürümlerini** sırayla kopyalar; mevcut exact kopyalar no-op, çakışma ret olur. Geçmiş lens/düzlem/çözünürlük değiştirmişse tek ortak kurulum varsayımı yapılmaz. Çok kayıt yazımında commit hatası consecutive prefix bırakabilir; retry aynı kanıtı denetleyerek devam eder. Aktarım etkinliği değiştirmez.

Yeni legacy wrapper **schema3 / legacy-validated/1**, yalnız gerçek legacy `validated()` modelini yeniden replay ederek benimser. Original model/holdout/hash byte/field yolu korunur. Legacy katalogda olmayan raster hash/frame **üretilmez**; `legacyEvidence.rasterEvidence=not_recorded` açıktır, bağımsız kontrol original holdout'tan gelir. Kullanıcı beyanı fiziksel kaynak kimliğini otomatik doğrulamaz. Original katalog/proje/binding'ler yeniden yazılmaz. Schema1 (unnamed), schema2 (name/date) ve schema3 (legacy adoption) aynı bounded shared catalog içinde strict sürümlenir.

## Sürüm ve kabul

Execution **23**; Edge Point **3** (`pointMm`), Circle **3** (exact standalone world dependency). Root project1/recipe1–2/editor6–8/raw1/world2/vf.socket2 korunur; editor8 yalnız explicit yeni world teaching'de gerekir. Terminal schema5 yeni sistem kayıtlarına aittir; eski codec/hash/no-op yolları korunur.

Kabul: `system_calibration_contract` bağımsız stripe/ring, üç applicationType ve farklı source UUID'leri, bütün metrik araçlar/px koruma, A→B/cache/TargetOnly0, immutable lease/provider değişimi, Stop/tamper/record retry, schema5/CSV/restart, complete legacy history/no fake raster/no-op/conflict ve gerçek perspektif circle rasterından non-affine H/bağımsız world oracle/hull ret sınar. `ui_system_calibration` proje servisi kataloğu → Editor → Runtime → Logs, eski widget yokluğu ve proje/kamera/pencere yaşam döngüsünü sınar. `ui_calibration_wizard` güncel sihirbaz mouse drag/click + iki plaka/bağımsız kontrol/iptal akışını, legacy geçiş ise doğrudan application servisini sınar. Nihai Release/CTest sayıları [progress kaydındadır](../progress.md); KUI-6 tam paket kabulü ayrıca yapılır.

Sınırlar: offscreen görsel kabul native DPI/çoklu monitör değildir. Fiziksel K4/lens/robot, industrial grab, makine genelinde çok süreçli aktif seçim koordinasyonu, CUDA/kurulum/dağıtım ve T8.4.2b.3c corpus ayrı açık işlerdir.
