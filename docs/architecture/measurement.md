# 6C — Tipli planar kalibrasyon ve Measurement

9 Ekim yeni ölçüm kapsamı **planlandı**: [T4–T9](recipe-tools-plan.md), [matematik tanımları](recipe-tools-algorithms.md#5-çizgi-daire-ve-kalibrasyon). Serbest yönlü subpixel edge, caliper, line, circle/arc ve geometry; doğrudan mm, fit merkezine göre radial peak-to-valley ve ayrı boyutsuz circularity. Raw-H altında world fit ve geçerli alan kapısı gerekir; geçersiz kalibrasyonda px fallback yoktur. Legacy Calculate/Measurement değişmedi. Lens düzeltmesi en son T15, K4 fiziksel kabul açık. Bu paragraf yeni araçların uygulandığı anlamına gelmez.

9 Ekim C1–C5: Manuel px/mm matematik, strict schema 3, katalog/current/recipe binding, gerçek Edge/Runtime/CSV ve sade üç yöntem ekranı tamamlandı. UI sınırları ve kullanım README içindedir; fiziksel K4 açıktır.

7 Ekim 2026. MeasurementService/CalibrationService/CalibrationWorkflow, versioned evidence/current resolver ve recipe binding ortak Measurement DAG/Properties ve planar Calibration ekranına bağlıdır. Calculate editor çıktısı hâlâ px'dir. Runtime/production activation configuration scope ile bağlıdır; fiziksel kalibrasyon doğruluğu kabul edilmemiştir.

## Model ve kanıt

MeasurementService::teach bilinen pozitif uzunluk ve iki farklı image pixel-centre noktasıyla uniform mm/px üretir. Model yalnız paralel çalışma düzlemi ve yeterince düşük distortion varsayımına uygundur; lens correction, perspective/homography, robot dönüşümü veya pixel-area ölçeği uygulamaz. Point frame/unit/axis, raw resolution sınırı ve finite değerler kontrol edilir.

Candidate tek başına ölçüm izni değildir. validate 2–256 ayrı kimlikli/geometrili holdout uzunluk örneği ister; teaching segmenti veya tekrar edilmiş segment kanıt olarak kullanılamaz. Her mutlak residual verilen mm sınırında olmalı; RMS mm kaydedilir. Bu matematiksel evidence kontrolüdür; numunelerin gerçek dünyadan/bağımsız ölçümden geldiğini servis kanıtlamaz. Hedef numune dağılımı ve gerçek parça doğruluğu ayrıca kabul edilmelidir.

PlanarCalibration (legacy API alias PlanarScaleCalibration) alanları ve constructor private; dış kod validated bayrağı veya scale değiştiremez. Validate yeni bir değer döndürür, candidate değişmez. SHA-256 contentId model sürümü, setup, scale, teaching, holdout, residual/RMS ve kabul sınırına bağlıdır; kullanıcı contentId'si güvenilmez. Version ayrı explicit dependency alanıdır. Teaching/dependency veya kabul kanıtı değişince eski validation kullanılamaz.

## Çalışma sınırı

distance iki gerçek DONE ToolResult'taki valid Point/px/image portlarını alır. Source/frame/cycle/coordinate/completion metadata eşleşmeli; cameraId burada edinim source kimliğidir. Current CalibrationDependency bağımsız application resolver tarafından kamera/lens/çözünürlük/çalışma düzlemi ve mevcut calibration content/version ile çözülmelidir. Caller'ın calibration.dependency() değerini körlemesine current diye vermesi gerçek kurulum değişimini denetlemez. Webcam index tek başına kalıcı fiziksel kamera kimliği değildir; üretim bağımlılığında ayrıca cihaz/kurulum kimliği çözülecektir.

Servis accepted dependency ile current kaydını birebir karşılaştırır; rawSize ayrıca current.resolution ile eşleşir. Lens/çözünürlük/plane/source-target/version/content değişimi değer üretimini engeller. Geçersiz/ERROR/SKIPPED upstream, yabancı frame/cycle/source, NaN/Inf, sınır dışı nokta veya hatalı tolerans ERROR ve NotEvaluated üretir; sayısal değer yoktur. CALIBRATION_INVALID, MEASUREMENT_INPUT_INVALID ve TOLERANCE_INVALID kodları ayrı tutulur.

Geçerli Euclidean px mesafe mm'ye çevrilir. Alt/üst tolerans dahilidir; karşılaştırma ham double mm ile yapılır. Sonuçta execution DONE ile ayrı Pass/Fail kararı, px/mm, origin ve calibration dependency vardır. Bu tek ölçüm kararıdır; reçetenin final AND/OR kararı veya üretim izni değildir. Snapshot/Run authorization/cycle-log bağlantısı tamamlandı; fiziksel ölçüm kabulü ayrıdır.

## Doğrulama

measurement_contract bilinen 3-4-5/scale örnekleri, raw rounding sınırı, candidate/holdout/duplicate/residual, setup değişimleri, upstream ve provenance engellerini sınar. Owned webcam-source snapshot → gerçek Edge DAG → Measurement yolu 399 px / 9.975 mm üretir. Testler sentetiktir. UI/core Release sonuçları progress.md içindedir.

## Sürümlü kayıt, güncel kurulum ve reçete bağlantısı

CalibrationService explicit mutlak katalog yolunu kullanır. `visionflow.calibrations` schema 1, uniform_planar_scale/1 kayıtları teaching/holdout/maxError/RMS/scale ve CalibrationDependency içerir. MeasurementService encode/decode; decode katsayıları ve holdout kabulünü yeniden hesaplar, SHA-256/content ve kayıtlı türetilmiş değerleri karşılaştırır. Bilinmeyen alan/model/şema, yanlış tip, değiştirilmiş kanıt ve sahte validated/content reddedilir. Matematiksel olarak tutarlı bir kaydın hash'i fiziksel bağımsız kanıt veya kullanıcı yetkisi değildir.

Katalog en fazla 4 MiB/128 doğrulanmış kayıt taşır. Her calibrationId için sürümler 1'den başlayarak ardışık append edilir; overwrite, gap ve otomatik silme yoktur. Son kaydedilen sürüm current olur; önceki sürümler load ile okunabilir. QLockFile ile read/check/write ve QSaveFile direct fallback kapalı commit uygulanır. Yeni sürüm kaydı eski binding'i hemen uyumsuz yapar; bu production Activate değildir. Eksik katalog load/current hatasıdır; save ilk kataloğu oluşturabilir. Bozuk mevcut katalog üzerine yeni kayıt yapılmaz.

InstallationProvider application tarafından bağımsız camera/source, lens, raw resolution, çalışma düzlemi ve source/target frame/birim/eksen kimliklerini sağlar. current en son disk sürümünü bu kurulumla denetler; resolver yoksa izin verilmez. resolve aynı katalog snapshot'ından binding/current/kanıt eşleşmesini ve tek enabled acquisition source UUID'sini kontrol eder. Editor provider edinilmiş frame'den source/koordinat/çözünürlük, ayrı recipe.definition.planarSetup alanından uygulanan lens/düzlem beyanını kullanır. Bunlar calibration.dependency() kopyası değildir; fiziksel lens veya kalıcı webcam kimliği otomatik tespiti değildir. Production cihaz kimliği kabulü açık, index tek başına yeterli değildir.

Recipe schema 2 `calibrationBinding` kesin dependency veya null taşır. Legacy schema 1 açılışı dirty/revizyon değiştirmez; explicit bind ile schema 2/DRAFT olur, no-op bind kirletmez. Normal ProjectRepository commit ile save/reopen/discard çalışır. Recipe kamera/reference veya acquisition source ID/kind/enabled/parameters değişimi binding'i temizler; ad/açıklama korur. ProductionRecipeService içerik hash'i binding'i kapsar; bağımsız güncel dependency eşleşmezse veya provider hata verirse DependencyChanged olur. Testte CalibrationService resolve provider'ı ile eski validation activation reddedilir; bu test tek başına Runtime kabulü değildir; Runtime entegrasyonu ayrıca tamamlandı.

Katalog ve proje ayrı atomik dosyalardır; birlikte transaction/taşınabilir paket değildir. Katalog commit sonrası proje save başarısızsa eski recipe binding korunur ve yeni current karşısında reddedilir. Measurement worker'ı acquisition sonrasında çözülmüş immutable kalibrasyon/current setup snapshot'ı taşır. Çevrim sürerken UI ayar/binding değişimi completion imzasıyla reddedilir; harici fiziksel kurulum değişiminin otomatik tespiti production provider kabulünde açık. Elektrik kesintisi/disk full kabulü ve production yetkisi/audit bu teslimde doğrulanmadı.

calibration_persistence testi restart/evidence replay, tip/şema/content/residual bozulması, byte/kayıt sınırı, sürüm gap/overwrite/lock/failed save, kurulum değişimleri, schema 1→2 binding/save/reopen/no-op/discard, kaynak değişimi ve eski production validation reddini kapsar. Release core/UI sonuçları progress.md içindedir.

## Ortak Measurement ve Calibration UI

ToolRegistry yedi built-in tool sunar. Measurement `p1`, `p2` Point/px/image alır; `value` Number/mm/plane, `pixels` Number/px/image ve `pass` Boolean/plane üretir. `lowerMm`/`upperMm` finite, nonnegative ve dahil toleranstır. DONE ile PartDecision ayrıdır; ERROR/SKIPPED/Stop durumunda mm/px/PASS/FAIL yoktur. UI üç ondalık overlay gösterebilir; karar ham double ile hesaplanır. Bu tek ölçüm kararıdır, reçetenin final karar birleşimi değildir.

ExecutionService version 6 kaynak ediniminden sonra worker'da CalibrationWorkflow resolver'ını çağırır. Yeni acquired raw frame ve immutable recipe snapshot üzerinden lens/düzlem/current katalog/binding doğrulanır; bağlam çözülemezse Edge çalışabilir, Measurement CALIBRATION_INVALID/ERROR olur. Mevcut input lineage kontrolleri korunur. Cycle boyunca çözülen kalibrasyon snapshot'ı sabittir; katalog her yeni çevrimde yeniden okunur. Run Match aynı engine yolundadır, Measurement yoksa resolver hatası Match'i hataya dönüştürmez. EditorSession stale-result imzasına calibrationBinding dahil edildi; ayar/binding değişimi eski çevrimi sunmaz.

Calibration ekranı: lens ve çalışma düzlemi beyanını Kurulumu uygula ile reçete taslağına alın; değişim binding'i temizler ve UI yeni acquisition ister. Kare al mevcut Image File/Webcam source'u aynı editor worker'ında tek tool scope ile çalıştırır. Orijinal px boyutu, hover koordinatı ve seçili örnek segmenti görünür. ID/yeni version girin; ayrı öğretim alanı ve yalnız bağımsız doğrulama içeren en az iki tablo satırı kullanılır. ID, X1/Y1/X2/Y2 ve gerçek referans Known mm kullanıcı tarafından girilir; boş/geçersiz örnekle hesap kabul edilmez. Hesapla candidate/scale gösterir, RMS göstermez. Doğrula servis holdout kabulünden sonra RMS sunar. Kaydet ve bağla kataloğu worker'da kaydeder, stale signature/generation kontrolü ardından owner-thread reçeteye bağlar; reçete DRAFT kalır, normal Save bağlantıyı kalıcı kaydeder. Kayıtlı kanıtı aç aynı codec'ten yükler; yeni sürümü otomatik doğrulanmış saymaz.

Tablo/ID/version/error bound değişimi candidate'ı temizler. Kaynak/kurulum değişimi binding'i kaldırır. Worker sürerken kalibrasyon komutları kapalıdır; session close guard geçerlidir. Katalog ve recipe iki ayrı commit'tir; başarısız binding/save katalog sürümünü silmez. Kalibrasyon taslak tablosu yalnız kabul edilip kaydedilince kalıcıdır; recipe değişiminde kaydedilmemiş kalibrasyon taslağı silinir.

measurement_dag ve UI smoke acquisition → gerçek Edge → Measurement 399 px / 9.975 mm, raw tolerance DONE/FAIL, missing edge SKIPPED, changed resolution/setup/latest version ERROR, Stop boş çıktı/NotEvaluated, candidate/holdout/save/reopen ve eski değerin temizlenmesini doğrular. Geometri/kalibrasyon sentetiktir. Üç applicationType ortak tool/servis yolunu kullanır; ProductionRecipeService Measurement/Webcam yapılandırma kabulünü açar; Runtime aynı resolver/engine yolunu kullanır. Raw planar homography modeli de aynı yolu kullanır; lens/robot modeli yok; Runtime configuration scope ile çalışır.

Runtime validation/snapshot/cycle kayıt bağlantısı tamamlandı; güncel scope/yetki/kalıcı kayıt sınırları [production-recipes.md](production-recipes.md) içindedir. Sonraki: [6C-K plaka tabanlı kamera/düzlem kalibrasyonu](calibration-plan.md), ardından 6D PLC handshake ve sonuç teslimi.

## Runtime bağlantısı

6C RuntimeService fresh source probe → yetkili configuration validation → immutable active snapshot → actual acquired frame resolver → Measurement → atomik terminal cycle record yolunu kullanır. Tool kararı/birim/kalibrasyon/tolerans/lineage finishCycle içinde yeniden kontrol edilir; global recipe kararı veya PLC teslimi yoktur. Taslak tolerans değişimi aktif snapshot'ı etkilemez; beyan edilen lens/düzlem/source değişimi yeni cycle'ı engeller. Kayıt hatasında UI mm/kararı yayımlamaz; aynı kayıt retry edilir. Restart otomatik etkinleşmez. Fiziksel ölçüm/lens/webcam kabulü açık kalır.

## 8 Ekim — K2/K3 raw planar homography

PlaneMappingService immutable H/gözlem/convex hull/fit sınırı ve hata istatistikleri; typed PlanarCalibration bunu veya legacy scale'i taşır. `teachHomography` source/frame alanı/resolution ve kurulumu eşleştirir. Homography `millimetresPerPixel()` döndürmez; kullanım hatasıdır. `distance` iki raw Point'i ayrı map eder, world düzleminde Öklid mesafesini inclusive ham toleransla karşılaştırır. Hull dışı/invalid/current mismatch gerçek değer veya karar taşımaz. Intrinsics/undistort yolu yok; model açıkça raw_assumed_negligible lens politikasını kaydeder.

Model record schema 2 `raw_planar_homography/1`; H/target/source/fit/evidence replay ve dependency schema 2 modelType/modelVersion içerir. Legacy uniform scale schema/hash değişmedi; PlanarScaleCalibration alias mevcut çağrıları korur. Source coordinate identity `image` veya sourceId/image olabilir; normalized/undistorted alanı kabul edilmez. Target-local axes columns/rows kayıtlıdır; robot frame'i değildir. InstallationProvider canonical target convention'ı desteklenen modelden kurar; bağımsız kurulum/source/axes eşleşmesi hâlâ kesin denetlenir.

CalibrationService mevcut 4 MiB/128 kayıt append-only consecutive-version kataloğunda üç modeli okur; replay yapılmadan saved VALID kullanılmaz. Homography holdout 2–256 ayrı segment; fit corner endpoint reuse (0.5 px), duplicate geometri, domain dışı ve hata sınırı aşımı reddedilir. Measured/absolute residual/RMS/max gerçek hesap değerleri UI ve CSV'de gösterilir. Matematiksel kabul fiziksel referans kökenini kanıtlamaz.

CalibrationStudyStore offline test candidate/accepted evidence manifesti; original image path + decoded pixel SHA256 doğrular. Kaynak eksik/değişmişse açmaz; test source bir fiziksel recipe kaynağına bağlanamaz. UI source acquired kayıtlarını existing catalog yoluna, offline testi ayrı study dosyasına gönderir. Atomik commit/cancel sonrası IO etkisi ayrı sınırlamadır. CalibrationReportService gerçek evidence CSV üretir; fiziksel kabul prosedürü [calibration-acceptance.md](calibration-acceptance.md) içindedir.
