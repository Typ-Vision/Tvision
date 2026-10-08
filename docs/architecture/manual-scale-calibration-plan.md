# Manuel px/mm ve sade Calibration — uygulama planı

8 Ekim 2026. Durum: kullanıcı isteğiyle **C1 matematik ve codec tamamlandı**; C2–C5 açık. Hedef yürütücü: GPT-6.1 Sol, medium. Bu belgenin ekran/katalog/Runtime/CSV kapsamı henüz teslim edilmedi. Önceki kısa manuel ölçek notunu ayrıntılandırır. K1–K3, Runtime operatör görünümü ve Setup yeniden yazılmayacak.

## 1. Ürün davranışı ve kapsam

Setup → Calibration içinde üç açık yöntem:

| Görünen ad | Kullanıcının girdisi | Amaç |
| --- | --- | --- |
| Manuel ölçek (px/mm) | Bildiği pozitif px/mm katsayısı | Sabit, her yönde aynı ölçek |
| Bilinen uzunluktan ölçek | İki görüntü noktası ve gerçek uzunluk mm | Mevcut uniform scale öğretimi |
| Kalibrasyon plakası (perspektif) | İç köşe sütun/satır ve kare kenarı mm | Mevcut raw planar homography |

Yeni, bağlantısız oturumda Manuel ölçek seçili olabilir; katsayı boş/geçersiz başlar, 1 gibi kabul edilebilir sahte varsayılan doldurulmaz. Bağlı veya kanıttan açılmış kalibrasyonda kendi yöntemi seçilir. Ekranı açmak/model seçeneklerini göstermek reçeteyi dirty yapmaz.

Manuel örnek: 20 px/mm → 0.05 mm/px; 200 px → 10 mm. Kullanıcı px/mm girer; ters birim salt okunur gösterilir. X/Y ayrı ölçek, lens düzeltmesi, robot dönüşümü ve perspektif bu yöntemde yoktur. Perspektifli görüntü için plaka yöntemi seçilir; otomatik fallback yapılmaz.

Katsayıyı girip **Ölçeği uygula** ile candidate oluşturmak ve iki nokta arasında **Deneme mesafesi** görmek mümkündür. Deneme açıkça “Doğrulanmadı — deneme” etiketlidir; RMS, PASS/FAIL, recipe VALID veya üretim izni üretmez. Mevcut Measurement/Runtime güvence sınırı korunur: kayıt ve reçeteye bağlama için en az iki bağımsız bilinen segmentle doğrulama gerekir. Doğrudan katsayı girişi, kalibrasyon doğrulamasını atlama seçeneği değildir.

Bu paket: manuel model/kanıt/katalog/resolver/Measurement/Runtime/CSV ve üç yöntemin UI sadeleştirmesi. P7 restart teslim kurtarma, hardware, OPC UA/QR, robot ve K4 fiziksel kabul kapsam dışındadır. Yeni rol/yetki sistemi veya genel UI yeniden tasarımı eklenmez.

## 2. Ekran akışı

Mevcut koyu tema, ImageViewer ve Setup girişi korunur. Solda görüntü, sağda kaydırılabilir beş numaralı bölüm; modal sihirbaz zorunlu değildir. Üstte mevcut adım ve tek cümlelik sonraki işlem gösterilir.

1. **Yöntem seç:** Yukarıdaki üç seçenek ve kısa kullanım açıklaması.
2. **Görüntü ve kurulum:** Kaynak adı, gerçek çözünürlük, lens/kurulum ve düzlem kimliği; Kurulumu uygula → Kare al. “Reçete kaynağı” ve “Dosyadan test” açıkça ayrılır. Eksik reçete/kare/kurulum nedenleri görünür.
3. **Ölçeği belirle:** Yalnız seçili yöntemin alanları. Manuelde px/mm ve salt okunur mm/px; bilinen uzunlukta ayrı öğretim segmenti; plakada iç köşe/kare mm → Köşeleri bul → Perspektif dönüşümünü hesapla. Teknik H/fit ayrıntıları açılır “Teknik ayrıntılar” bölümündedir. Teknik düğme yeniden adlandırması aynı servis eylemine gider.
4. **Bağımsız doğrulama:** “Ölçeği belirlerken kullanmadığınız en az iki bilinen mesafeyi ekleyin.” Tabloda ID, başlangıç/bitiş px, bilinen mm, ölçülen mm, mutlak hata mm. Bilinen uzunluk yönteminin öğretim segmenti bu tablodan ayrıdır; ilk satırın gizli özel anlamı kaldırılır. Hata sınırı mm, Doğrula, gerçek RMS/maksimum hata ve açık ret nedeni burada görünür.
5. **Kaydet ve bağla:** Calibration ID, yeni sürüm, durum ve Kaydet ve bağla. Başarıda “Kalibrasyon kaydedildi, reçete bağlantısı taslak; Save Project / Recipes ile kaydedin.” Katalog yazıldı ama binding/proje kaydı başarısızsa gerçek iki dosya durumu açık anlatılır.

Nokta seçme: aktif hedef “Öğretim”, “Seçili doğrulama satırı” veya “Deneme” olarak görünür. Başlangıç seç/Bitiş seç ile görüntü tıklaması orijinal px alanını doldurur; sayısal subpixel düzenleme korunur. Viewer zoom/pan katsayıyı/koordinatı değiştirmez. Seçilmiş hedef yoksa yanlışlıkla tabloya yazılmaz. Aynı iki noktayı doğrulama tablosuna otomatik başarı kanıtı diye kopyalama.

Manuel/bilinen uzunlukta chessboard alanları, köşe/H düğmeleri ve fit raporu görünmez. Plakada manuel katsayı ve öğretim segmenti görünmez. Kayıtlı kanıtı aç, test kanıtı ve CSV ikincil “Kayıtlar ve rapor” bölümünde yer alır. İnaktif düğmenin gerekçesi yalnız tooltip'te değil görünür durum metninde de bulunur. 850×600 ekranda tüm alanlara kaydırarak ulaşılır.

Dosyadan testte manuel candidate/deneme/doğrulama çalışabilir; fiziksel reçeteye bağlanamaz. Bu pakette manuel test oturumu bellektedir: `.vfcalstudy` kaydı yalnız mevcut homography için korunur; manuel modda test kaydet düğmesi gizlenir ve kalıcı test kaydının desteklenmediği belirtilir. Manuel CSV ancak gerçek doğrulanmış kanıtla dışa aktarılır. Homography test kaydet/aç özelliği kaybolmaz. Bilinen uzunluk yönteminin mevcut acquisition davranışını gereksiz genişletme.

## 3. Servis ve veri sözleşmesi

UI algoritma/SDK çağırmaz. MeasurementService'e açık manuel candidate factory ekle; PlanarCalibration'ın private constructor/validated yönetimini koru. Sahte teaching segmenti yaratıp `teach()` yolunu kandırma. Mevcut `!mapping()` demek teaching vardır varsayımını açık model dispatch ile değiştir.

- Yeni model adı: `manual_planar_scale`, modelVersion 1. Gerçek girdi `pixelsPerMillimetre`, türetilmiş değer `millimetresPerPixel = 1 / pixelsPerMillimetre`. Canonical hash'e gerçek girdi, yöntem, kurulum, kanıt ve doğrulama politikası dahil edilir; yalnız UI'da yuvarlanmış değerden hash üretilmez.
- Pozitif/sonlu katsayı ve pozitif/sonlu tersi; dönüşüm/mesafe/residual taşmaları reddedilir. UI aralığı ve ondalık hassasiyeti açık sabitlerle tanımlanır ve sınır testi yazılır; servis doğrulaması UI'ya güvenmez. Türkçe ondalık virgül QLocale ile tutarlı ele alınır; JSON sayısal kalır. Sessiz clamp veya px/mm ↔ mm/px yorumu yoktur.
- Manuel model uniform scale'in mevcut frame/eksen/ham çözünürlük sözleşmesini kullanır; görüntü dışı/yanlış frame noktası reddedilir. Homography'nin convex-hull alanı ve fit-corner bağımsızlığı aynen korunur.
- Manuel modelde teaching yok; tüm holdout satırları bağımsızdır. 2–256 gerçek segment, benzersiz ID, pozitif gerçek mm, sıfır olmayan segment, ters uçlu tekrar dahil yinelenen geometri reddi ve mevcut dahil maksimum hata kuralı korunur. Ölçülen mm = ham px mesafe / pxPerMm. Doğrulama katsayıyı yeniden fit etmez veya düzeltmez.
- Candidate/accepted durumu servis sonucudur. Diskteki validated/RMS/scale/content alanlarına güvenme; decode candidate'ı gerçek girdiden üretip holdout'u yeniden hesaplar ve kimliği/türetilmiş değerleri karşılaştırır. Referansın fiziksel bağımsızlığı yazılım tarafından kanıtlanmış sayılmaz.
- Manuel kanıt kökeni acquired/file_test, source/frame kimliği ve gerçek çözünürlük ile saklanır. Dosyadan test acquisition kaydı gibi bağlanamaz. Manuel CSV'de yöntem, px/mm, mm/px, köken ve gerçek doğrulama değerleri bulunur; “legacy” etiketi kullanılmaz.

Kalibrasyon record ve dependency için **yeni schema 3** kullan; mevcut record/dependency schema 1 uniform ve schema 2 homography encode/hash davranışını değiştirme. Şema 2 decoder bugün yalnız homography kabul ediyor; aynı şemaya sessizce manuel model ekleme. Manuel modelin alan listesini C1'de belgede somutlaştır; strict keys/type/model-version/future-schema ret testleri ekle. Katalog container schema 1, 4 MiB/128 kayıt, ardışık append ve lock/atomic commit korunur. Root project/recipe/editor şemalarını bu iş için gereksiz artırma; recipe schema 2'nin tipli binding'ine yeni dependency eklenmesini roundtrip ile kanıtla. Eski binary yeni modeli açıkça reddetmeli.

Eski uniform/homography fixture'larının contentId ve encode eşitliği korunur; aç/no-op Save migration veya revision artışı yapmaz. Yeni manuel binding explicit düzenlemedir ve reçeteyi DRAFT yapar. Registry/engine semantic version ve production validation identity etkisini C2'de incele; yürütme anlamı değişiyorsa sürüm artır, eski doğrulama iznini yeniden kullanma; gereksiz şema artışıyla karıştırma.

### C1 uygulanan manuel kayıt sözleşmesi

`MeasurementService::manualScale(setup, pixelsPerMillimetre, provenance)` teaching üretmeden immutable candidate kurar. `trialDistance` ham noktaları denetleyip yalnız double mm döndürür; karar veya doğrulama üretmez. `distance` accepted/acquired kanıt gerektirir; file_test üretim sonucu vermez. `validate` katsayıyı değiştirmez. UI giriş aralığı/locale ve ondalık sunumu C3'te tanımlanıp sınanacaktır; C1 servis girişi UI aralığına dayanmaz.

Record schema 3'ün **tam alan listesi**:

| Alan | Tip / anlam |
| --- | --- |
| schemaVersion, model | 3, `manual_planar_scale/1` |
| dependency | Aşağıda tanımlanan schema 3 dependency |
| pixelsPerMillimetre | Pozitif sonlu gerçek kullanıcı girdisi |
| millimetresPerPixel | Pozitif sonlu `1 / pixelsPerMillimetre`; decode yeniden hesaplar |
| provenance | Tam alanlar: origin (`acquired`/`file_test`), sourceId, frameId (boş olmayan metin), width/height (pozitif tamsayı) |
| holdout | 0 candidate veya accepted için 2–256 sample; her sample tam `id,x1,y1,x2,y2,knownMm` alanlarını taşır |
| validated | Boolean; kayıtlı değere güvenilmez, accepted holdout yeniden hesaplanır |
| maxErrorMm, rmsMm, maxResidualMm | Sonlu sayılar; candidate için 0, accepted için replay edilen politika/istatistik |

Dependency schema 3 tam alanları: `schemaVersion,calibrationId,version,contentId,cameraId,lensId,width,height,workingPlaneId,source,target,modelType,modelVersion`. Son iki değer `manual_planar_scale`, 1'dir. `source`/`target` mevcut strict `id,unit,axes` frame nesneleridir. Schema 1 yalnız uniform; schema 2 yalnız homography; schema 3 yalnız manuel modeldir. Kaynak ve boyut provenance/dependency arasında birebir eşleşir. file_test kaynağı `calibration-test:` öneki taşır; acquired bu öneki taşıyamaz. Bu beyan fiziksel referans kökeninin doğrulandığı anlamına gelmez.

SHA-256 kimlik: schema 3 record'un compact QJsonDocument çıktısı; yalnız `dependency.contentId` hash öncesinde çıkarılır. Gerçek katsayı, ters değer, model, kurulum, provenance, holdout ve kabul politikası/istatistik hash'e dahildir. Decode candidate'ı gerçek girdiden kurar, accepted kanıtı yeniden hesaplar, dependency/content/türetilmiş değerler ve tam encode eşitliğini denetler. Bilinmeyen/eksik alan, yanlış tip/model/sürüm ve future schema reddedilir. Uniform/homography encode/hash yolları korunur; odaklı test sabit contentId ve tüm compact record SHA-256 değerlerini denetler.

C1, katalogda manuel kaydı save/bind etme veya Runtime kabulü olarak sunulmaz; bu zincirin installation/resolver/CSV/engine identity kabulü C2'de tamamlanacaktır. UI yoktur; candidate raporu/locale/deneme gösterimi C3–C4'tür.

## 4. Yaşam döngüsü ve invalidation

- Katsayı/yöntem/öğretim/plaka/kurulum/frame değişimi ilgili candidate, deneme, acceptance ve eski sayısal raporu temizler. Holdout veya hata sınırı değişimi doğrulamayı düşürür; candidate girdileri hâlâ aynıysa yeniden hesaplanmış candidate korunabilir. Başarısız Doğrula eski başarılı raporu bırakmaz.
- UI taslağı değişti diye kaydedilmiş immutable modele gizlice yazma. Explicit kayıt/binding ve kaynak/kurulum değişiminin mevcut DRAFT/current kuralları ayrı kalır. Yeni katalog sürümü eski binding'i uyumsuz yapar; sırf yöntemi ekranda gezmek Runtime snapshot'ını değiştirmez.
- Worker immutable snapshot ve generation/signature ile çalışır. Stop, yeni frame, reçete/yöntem değişimi geç sonucu reddeder; iptal edilmiş IO'nun zaten yazdığı atomik kayıt geri alınmış gösterilmez.
- Yeni model CalibrationWorkflow installation/resolve/bind, CalibrationService current/resolve ve ProductionRecipeService/RuntimeService model whitelist kontrollerinden geçer. Reçete/acquisition source ve çözünürlük bağımsız güncel kurulumdan doğrulanır; model kendi dependency'sini current diye kabul ettirme.
- Measurement ham mm ile tolerans kararı üretir; Calculate px kalır. Invalid kalibrasyonda eski mm/PASS/FAIL yoktur. Calibration denemesi production sonucu veya PLC gönderimi olmaz. Run operatör kilidi, Stop ve Communication ömrü korunur.

## 5. Sıralı iş paketleri

Her paketi tamamlayıp doğrulayarak progress'e kaydet; C1–C5 dışında özellik genişletme. Aynı dosyaları eş zamanlı düzenleyecek ajanlara bölme; bu plan alt ajan gerektirmez.

| Paket | Değişiklik odağı | Bitiş kanıtı |
| --- | --- | --- |
| C1 — tamamlandı | MeasurementService.*, Coordinates.*, manuel model alan/şema sözleşmesi; tests/manual_scale_test.cpp | 20 px/mm → 0.05 mm/px → 200 px/10 mm; candidate deneme/üretim ayrımı; gerçek holdout kabul/ret; encode/decode/tamper; eski model hash/encode sabitleri |
| C2 — Kalıcı bağlama ve yürütme | CalibrationWorkflow.*, CalibrationService.*, CalibrationReportService.*, ilgili Runtime/Production/ToolExecutors/EditorSession model kontrolleri | Kaydet/bağla/Save/reopen; yeni sürüm/current reddi; gerçek Edge→Measurement→Runtime mm ve dahil tolerans; source/resolution değişimi; CSV; candidate/test üretime bağlanamaz |
| C3 — Sade ekran ve manuel giriş | MainWindowCalibration.cpp/.hpp; gerekirse ayrı calibration sunum helper'ı; ImageViewer yalnız ortak nokta etkileşimi gerekiyorsa | Beş bölüm, üç yönteme özel alanlar, boş manuel başlangıç, px/mm/tersi, öğretim/holdout ayrımı, tık/sayısal nokta seçimi, görünür pasif nedenler, worker/stale guard |
| C4 — UI regresyon ve kullanım kabulü | EditorSmoke.cpp, mevcut calibration/chessboard/homography testleri | Üç yöntem, model değişimi/yeniden açılış, gerçek akış, hata/Stop/locale, 1400×900 ve 850×600 görseller; Setup ve Run kilidi korunur |
| C5 — Birleşik kabul ve belgeler | README, measurement/project-format/calibration planları, gereksinimler, backlog/progress | Release UI tam CTest + Widgets OFF tam CTest; son görsel kontrol; uygulanmış durum ve kalan fiziksel sınırlar; sonraki P7 için devir |

Başlıca mevcut riskli noktalar: MeasurementService identify/validate/encode/decode içindeki iki model varsayımı; Coordinates::fromJson schema 2 whitelist; CalibrationWorkflow model/eksen seçimi; CalibrationReportService `else legacy` etiketi; CalibrationStudyStore `mapping()` zorunluluğu; MainWindowCalibration model index 0/1 ve tablonun ilk satırını teaching sayması. Bunları yalnız UI kontrolü ekleyerek geçiştirme. Mevcut çalışmış servisleri yeniden kurma.

## 6. Doğrulama matrisi

- Matematik: yatay/dikey/çapraz segment, subpixel, farklı katsayı, sıfır/negatif/NaN/Inf/ters taşması; zoom bağımsızlığı. Örnek geometri ile sonuç önceden analitik belirlenir.
- Kanıt: 0/1 holdout ret, 2 farklı segment kabul, 256 sınırı/257 ret, yinelenen ID ve ters yönlü aynı segment ret, hata sınırına eşitlik/aşım, yanlış birim/frame/bounds, katsayı değiştirilince acceptance iptali. Başarısız doğrulama katsayıyı değiştirmez.
- Persistence: eski sabit fixture hash/encode, yeni schema 3 roundtrip, yanlış model/sürüm/future schema, sahte validated/ölçek/residual/hash, lock/katalog kapasitesi/sürüm çakışması, binding/proje save hatası, no-op/reopen/discard. Test verisi yalnız build altında.
- Yürütme: iki gerçek upstream Edge.point ile manuel kalibrasyonlu mm; alt/üst dahil PASS ve dışarıda FAIL; candidate/yanlış frame/current/Stop için değer/karar yok; immutable Runtime/draft ve TargetOnly invalidation korunur. Gerçek localhost socket regresyonu, deneme mesafesinin üretime sızmaması.
- UI: kutuya Türkçe locale ile değer girişi, boş/yanlış giriş açıklaması, öğretim/holdout hedefinin görünür seçimi, tık/zoom/pan, model geçişinde eski rapor yok, file_test/acquired ayrımı, kayıtlı legacy/plaka/manuel yöntemin doğru yüklenmesi. Teknik ayrıntılar kapalıyken günlük akış tamamlanır.
- Görsel: her yöntemin başlangıç/aday/doğrulanmış durumundan anlamlı örnekler; 1400×900 ve 850×600, uzun hata mesajı ve tablo kaydırma. Ekran görüntüsünü üretmekle yetinme, açıp incele ve taşmaları düzelt.

README toolchain'ini kullan. Güncel çalışan baseline build/operator-ui; açık exe kilitliyse kullanıcının sürecini sonlandırma, yeni build/manual-calibration-ui ve build/manual-calibration-core Release dizinleri kullan. Core için VISIONFLOW_BUILD_UI=OFF. Gerçek test adlarını ctest -N ile kontrol et; varsayılan test sayısını başarı iddiası olarak kopyalama. Localhost sandbox engelini ürün hatasıyla karıştırma; izinli yerel tekrar sonucunu ayrı kaydet. config/local.ui.ini değiştirilmez. K4/dağıtım/kurulum/CUDA ve fiziksel PLC kabulü iddia edilmez.

## 7. GPT-6.1 Sol medium için başlangıç talimatı

Aşağıdaki metin uygulama oturumuna doğrudan verilebilir:

```text
VisionFlow'da manuel px/mm kalibrasyonu ve Calibration ekranının sadeleştirilmesini uygula.

Önce AGENTS.md, docs/progress.md, README.md, docs/architecture/manual-scale-calibration-plan.md ve ilgili calibration-plan/measurement belgelerini oku. Güncel kullanıcı önceliği bu planın C1–C5 paketleridir; tamamlanan K1–K3, Setup ve Runtime operatör görünümünü yeniden yapma. Mevcut dirty/untracked çalışmaları koru; reset/temizleme yapma.

C1 → C2 → C3 → C4 → C5 sırasıyla ilerle. Her pakette sınırlı kapsamı uygula, anlamlı testlerini çalıştır ve docs/progress.md'ye kapsam/değişen dosyalar/kanıt/sınırlar/sıradaki adımı kaydet. Yeni onay istemeden bu beş paketi tamamla; oturum bölünürse son tamamlanmamış paketten devam et. Gerçek engelde yapılmış işi koru ve somut eksik bilgiyi bildir.

Üç yöntem sun: Manuel ölçek (px/mm), Bilinen uzunluktan ölçek, Kalibrasyon plakası (perspektif). Yalnız seçili yöntemin alanlarını göster. Beş bölümlü akış ve öğretimden ayrı bağımsız doğrulama tablosunu uygula. Manuel girdi için sahte varsayılan/teaching yaratma. Pozitif sonlu px/mm ve tersini doğrula; aday denemeyi “Doğrulanmadı — deneme” göster. Measurement/Runtime binding için en az iki gerçek bağımsız doğrulama segmenti şartı korunur. Deneme PASS veya PLC sonucu değildir.

Planın manual_planar_scale/1, yeni record/dependency schema 3 ve eski hash/encode koruma kararlarını uygula. UI algoritma veya SDK çağırmasın; hesap/codec/kanıt/resolver servislerde kalsın. strict decode, bağımsız current kurulum, source/frame/çözünürlük, immutable snapshot, dirty/DRAFT ve invalidation sözleşmelerini koru. Manuel test dosyası oturumu bellektedir; homography .vfcalstudy desteğini koru. Katalog ve proje iki ayrı atomik kayıt olmaya devam eder, kısmi başarıyı dürüst göster.

Release UI ve Widgets OFF derlemelerini, ilgili testleri ve kapanışta tam CTest suite'lerini çalıştır. UI'da 1400×900 ve 850×600 görüntüleri açıp incele. Test INI/fixture/görseller build altında; config/local.ui.ini ve kullanıcının açık uygulaması korunur. README/mimari/gereksinim/backlog/progress uygulanmış durumla uyumlu olsun. K4 fiziksel kabul veya kurulum/dağıtım/CUDA doğrulaması iddia etme. P7, hardware, OPC UA/QR ve robot işlerine bu kapsamda geçme.

Son yanıtında yeni binary yolunu, kısa manuel kalibrasyon kullanımını, gerçek test/görsel sonuçlarını ve bilinen sınırları ver.
```
