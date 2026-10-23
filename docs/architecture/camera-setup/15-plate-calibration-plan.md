# CCUX-15e–j — Çerçeveli plaka ile planar kalibrasyon

23 Ekim 2026 — CCUX-PLAN-05. Durum: **planlandı, uygulanmadı**.
Kullanıcı yalnız plaka yöntemi için ayrıntılı plan istedi; bu oturum uygulama
yetkisi/teslimi değildir. Başlangıç kartı 15e. Önceki 15a–d işleri yeniden yapılmaz.
[Araştırma](../../archive/2026-10-23-ccux-15d.md),
[lens uyumluluğu](../../archive/2026-10-23-ccux-15d-lens.md),
[ortak sözleşme](contract.md#c3a--çerçeveli-plaka-kalibrasyonu).

## Amaç ve sınır

Kullanıcının koyu çerçeveli, köşesinde üçgensi yön işareti bulunan 7×7 daire
plakasını normal kalibrasyon sihirbazında tanımak; gerçek 49 merkezi göstermek;
mevcut raw planar homografi ile aday → Kaydet ve kullan → Editor/Runtime mm
zincirini doğrulamak. Fazla arka plan adayları tek başına ret nedeni olmayacak.

Lens intrinsics, distorsiyon katsayısı, undistort/remap, çok pozlu lens çözümü,
robot/hand-eye, PLC açma, yeni SDK, native kabul ve kurulum paketi kapsam dışı.
Plaka tanıma, model hesabı ve fiziksel kabul ayrı sonuçlardır. Bu çalışmayla
distorsiyon düzeltilmiş sayılmaz. Bilinen ölçü ve chessboard yolları korunur.
Mevcut generic simetrik daire hedefi silinmez; yeni çerçeveli hedef açık türdür.

CCUX-22b–e oturum altyapısı kullanılır; bağlantı katmanı yeniden yazılmaz.
22f birleşik bağlantı kabulü açık kalır; 15i aynı kontrolleri paylaşabilir fakat
22f'yi otomatik kapatmaz. C0/OECS/native engeller değişmez.

## Kullanıcının plaka tanımı

| Bilgi | Mevcut dayanak | Planlanan kullanım |
| --- | --- | --- |
| 7 sütun × 7 satır | Kullanıcı + önceki görüntü | Tam 49 gerçek merkez |
| Merkez aralığı 12 mm | Önceki kullanıcı beyanı; doğrulanmadı | Kalibrasyondan önce açık teyit, fiziksel kabulde ölçü/köken |
| Çerçeve kalınlığı 3 mm | Kullanıcı beyanı | Lokalizasyon ipucu; ölçek referansı yerine geçmez |
| Köşede üçgensi işaret | Kullanıcı + önceki görüntü | P1 ve eksen yönünü belirleme |
| Daire çapı, çerçeve iç boyutu/boşluğu, düzlemsellik | Kesin fiziksel veri yok | Çizim/ölçü varsa kaydet; bilinmeyeni sıfır/uydurma ile doldurma |

İlk profil koyu daire/açık zemin, eşit X/Y aralığı ve tam görünür plaka içindir.
Görselden fiziksel mm/üretici/sertifika türetilmez. Bilinmeyen çap/çerçeve
ölçüsü algılama prototipini engellemez: görüntü ölçeğinden tahminler yalnız
arama parametresidir. Gerçek aralık doğrulanmadan fiziksel doğruluk iddiası yok.
7×7 profili ilk kabul hedefidir; her olası plaka türünü destekleme kapsamı yok.

## Tasarımın değişmezleri

1. **Gözlem, model değildir.** Algılayıcı ham pikselde gerçek merkezleri,
   nokta kimliklerini, hedef tanımını, yön kanıtını ve kalite/tanı bilgisini
   üretir. Lenssiz H fit başarısı algılayıcının zorunlu çıkış koşulu olmaz.
2. **Mevcut mm kapısı korunur.** Raw planar model; tamlık, benzersizlik, doğru
   sıra, dejenere geometri, mevcut 1 px inverse fit, hull ve bağımsız kontrol
   kurallarından geçer. H fit için nokta atılmaz, eksik nokta uydurulmaz.
3. **Ham koordinat tek gerçektir.** Çalışma ROI/resize/warp lokalizasyon içindir.
   Son merkezler özgün çözünürlükte ölçülür; raw raster/hash/origin korunur.
   Yeni gözlem açık raw uzay kimliği taşır; düzeltilmiş uzay taklit edilmez.
4. **Yön kanıta dayanır.** Yeni çerçeveli profilde üçgen bulunmadan veya birden
   fazla yorum varken hazır gözlem/model yok. İlk sürümde manuel yön atama ve
   merkez sürükleyerek düzeltme yok. Generic eski grid eski anlamıyla kalır.
5. **Geleceğe sınır bırakılır, lens kodu yazılmaz.** Gözlem ileride başka solver'a
   girdi olabilir. Lens eklenince ayrı koordinat/model sürümü ve lens bağı
   gerekir; mevcut raw H düzeltilmiş piksele uygulanamaz.
6. **Kayıt uyumluluğu.** Yeni hedef/yön/gözlem kanıtı sürümlenir. Eski kayıtlar
   no-op aç/kaydet sırasında yeniden kodlanmaz; legacy hash değişmez. Eksik eski
   yön bilgisi üçgenle doğrulanmış gibi doldurulmaz. Hedef/fit sayıları geriye
   dönük yeniden anlamlandırılmaz.

Önerilen kavramsal servis sınırı (nihai isimler 15e'de mevcut API'ye uyarlanır):

`RawFrame + PlateDescriptor + optional SearchRoi → DetectionResult`

`DetectionResult = diagnostics + optional immutable RawPlateObservation`

`RawPlateObservation + setup + controls → mevcut RawPlanarModelCandidate`

Gözlem; frame/hash/kamera/geometri, hedef ID/içerik sürümü, her merkezin
satır/sütun kimliği, raw konumu, ölçüm kalite bilgisi ve yön kanıtını taşır.
Tanıdaki kısmi noktalar model girdisi olamaz. Pikselde benzer görünen iki tam
eşleşme varsa en yüksek skoru sessiz seçmek yerine belirsizlik döndürülür.

## Algılama hattı

1. Kaynak/hash/format/boyut doğrula; immutable ham görüntüden çalışma kopyası.
   Mevcut 16 MP sınırı, iptal ve worker yaşam döngüsü korunur.
2. Çok ölçekli aramada koyu çerçeve, iç açık alan ve nokta grubuyla ROI öner.
   Üçgen iç konturu beşgenleştirebilir; yalnız dört köşeli kontur şartı koyma.
   Eğrilik için toleranslı aday üret; kusursuz çizgi şartıyla gelecekte lens
   gözlemlerini engelleme. İlk sürümün desteklediği bozulma aralığı testle sınırlı.
3. ROI içinde yerel/çok eşikli aday çıkar. Çerçeve, dış yazı, küçük kir ve
   yinelenmiş eşik adaylarını ayır. Alan/kontrast/eliptiklik/komşuluğu birlikte
   kullan; tek minArea veya global sayıyı 49'a ayarlayarak çözme.
4. Komşuluk ve satır/sütun topolojisiyle birebir 7×7 eşleme kur. Kaba homografi
   yardımcı hipotez olabilir; 1 px raw H fit burada başarı kapısı değildir.
   58 adaydan 49 doğru merkez seçilebilir; gerçek merkez eksikse başarı yok.
5. Köşe işaretini çerçeve ve gridle birlikte doğrula. Kanonik görünümde
   işaretli köşeye yakın merkez P1=(0,0); +X sütun, +Y satır; aynalama/orientation
   metadata'sı açık olmalı. Dört dönüşte aynı fiziksel kimlikler korunmalı.
   İşaretin belirsiz/mirror yorumu sessizce tamamlanmaz.
6. Atanan noktaların konturlarını özgün rasterde subpixel kenar/elips yaklaşımıyla
   hassaslaştır. Merkez düzeltmesi aşırıysa veya kenar eksik/doygunsa kalite ret.
   Perspektifte elips merkezi hatası fiziksel kabulde değerlendirilir; kusursuz
   izdüşüm iddiası yok. Nokta kimliğini refinement sırasında değiştirme.
7. Tamlık/yön/kalite tanısını döndür. H model hesabı ayrı application adımı.
   Fit başarısızsa gözlem tanısı gösterilir, model/SaveUse üretilmez.

Otomatik ROI başarısız olduğunda kullanıcı arama alanını daraltabilir. Bu ROI
yalnız kalibrasyon oturumuna aittir; reçete ROI'si veya raster crop değildir.
ROI çerçeve/işareti keserse açıklayıcı ret; ROI seçmek yön/49 nokta kapısını atlamaz.
İlk kartlarda sınırlı deneme sayısı/aday bütçesi ve aşama iptal noktaları konur;
ms hedefi uydurulmaz, süre 15f gerçek raster koşumunda ölçülür.

## Kullanıcı akışı ve durumlar

Kamera → Kalibre Et → Kalibrasyon plakası → çerçeveli daire profili →
7×7/aralık bilgilerini kontrol → Kare al → Plakayı bul → grid/P1/eksenleri incele
→ mevcut model hesabı ve bağımsız kontroller → Kaydet ve kullan.

Normal ekranda yalnız gerekli hedef bilgileri; tanı ayrıntıları açılabilir.
İç algoritma eşikleri/SDK/lens hazırlık ayarları ana kullanıcı akışına konmaz.
49 noktayı tek tek elle onaylatma yok; mevcut tek grid inceleme/onay adımı yeterli.
Gözlem durumu, model durumu, fiziksel doğrulama ayrı gösterilir. Kaydet/kullan
mevcut atomik semantiğini korur; önceki etkin model ret/iptalde değişmez.

| Durum | Kullanıcıya gösterilecek neden ve sonraki işlem |
| --- | --- |
| Plaka bölgesi yok | Çerçeveyi kadraja al veya arama ROI'sini daralt |
| İşaret yok/belirsiz | İşaretli köşeyi görünür yap; model yok |
| Grid eksik | Hangi satır/sütun bölgesinin eksik olduğu; noktayı uydurma yok |
| Ek adaylar | Seçilen/reddedilen adayları ayır; tek başına hata değil |
| Birden fazla grid/yorum | ROI ile tek plakayı ayır; otomatik keyfi seçim yok |
| Yetersiz kenar/kontrast | Yeniden görüntü al; otomatik exposure değişikliği yok |
| Gözlem hazır, H fit reddi | Geometrik/model uyumsuzluğu; kamera/lens bozulması olası ama kesin tanı değil |
| Aday hazır | Fit bilgisi, geçerli ölçüm alanı; fiziksel kabul ayrı |
| İptal/eski sonuç | Yeni kare/ayar/target sonucunu üzerine yazamaz |

Overlay: seçilen merkezler + sıra, P1/+X/+Y, plaka ROI'si, isteğe bağlı
reddedilen adaylar, model varsa geçerli hull. Renk yanında şekil/metin kullan.
Örnek tanı “58 aday, 49 eşleşen merkez”; 58 ölçülmüş plaka dairesi diye sunulmaz.

## Sıralı uygulama kartları

Bir oturum bir kart; ajanlar paralel çalışmaz. Her kart için yalnız ortak planın
tasarım/sınır bölümü ve seçili kart okunur; kaynak bağımlılıkları aşağıdadır.
Her kart tarihli kapanış, progress güncellemesi ve görev ID'li tek commit ister.

| Kart | Teslim | Bağımlılık |
| --- | --- | --- |
| CCUX-15e | Hedef/gözlem/tanı sözleşmesi ve fit ayrımı | Bu plan |
| CCUX-15f | Çerçeve, üçgen, 49 merkez algılama ve regresyon | 15e |
| CCUX-15g | Raw planar aday, sürümlü kayıt/replay, SaveUse | 15e–f |
| CCUX-15h | Sihirbaz profili, overlay, tanı ve oturum davranışı | 15f–g |
| CCUX-15i | Editor/Runtime/Logs birleşik yazılım kabulü | 15g–h |
| CCUX-15j | Gerçek kamera, bağımsız mm ve operatör kabulü | 15i + donanım/ref |

### CCUX-15e — sözleşme ve gözlem/model ayrımı

Oku: CalibrationGrid, CircleGridService, PlaneMappingService,
CalibrationWizardSession::run/accept/teachPlate, tests/calibration_plate_test.cpp.

- Yeni çerçeveli hedef profilini, orientation/polarite ve isteğe bağlı fiziksel
  geometriyi tanımla; eski aggregate/alias ve codec kullanıcılarını envanterle.
- Typed tanı ile hazır ham gözlemi ayır. Gözlem alanlarının geçerlilik/limit,
  finite değer, duplicate ID ve hedef eşleşme kurallarını test et.
- CircleGridService içindeki ve WizardSession::run içindeki H fit kapılarını
  gözlemden ayır; model oluşturma/SaveUse kapıları eksiksiz kalsın. Sadece ilk
  fit çağrısını silmek yeterli değildir. Session onayı model kabulü sayılmasın.
- Henüz desteklenmeyen çerçeveli algoritma açık Unsupported döndürsün; eski
  generik algoritma sessizce yeni profil yerine çalışmasın.
- Sözleşme testi: doğru kimlikli, raw H'ye uymayan örnek gözlem saklanabilir;
  aynı gözlemden mevcut H modeli reddedilir. Bu örnek lens solver testi değildir.

Kabul: mevcut generic/chessboard çağrıları ve ret güvenliği korunur; yeni veri
API'si tanıdan model üretemez. Release + calibration_plate_contract,
calibration_wizard_contract, plane_mapping_contract, chessboard_contract.
Kanıt `build/ccux/ccux-15e/`. Devam 15f.

### CCUX-15f — plaka algılama

Oku: 15e API, CircleGridService/ImageAccess, mevcut plaka testi ve 15c tanı kaydı.

- Yukarıdaki 7 aşamalı hattı application/vision katmanında uygula. Algoritma
  sürümü, kullanılan parametreler, aday/atanan/reddedilen noktalar ve aşama
  sürelerini raporla. Başarı kadar ret overlay'ini de kaydet.
- Eski iki ham kareyi immutable hash/sidecar ile baseline'a al; yeni koşum
  ayrı dizinde olsun. Test verisi build altında kaybolabiliyorsa küçük/lisansı
  uygun fixture veya açık dış fixture manifesti sağla; CI başarıyla skip'i ayır.
- Son 58 olayının karesi mevcut değil; yeni edinim gelene dek yeniden üretildi
  deme. Sentetik 58 aday testi gerçek olay yerine sunulmasın.
- Görsel/kontur incelemesiyle gerçek merkezleri bağımsız işaretle; çıktıdan
  beklenen cevap üretme. Saklanan 49 yanlış aday örneği başarıya dönüştürülmesin.

Kabul matrisi: temiz/ek blob/kir, aydınlık gradyanı, farklı ölçek, dört 90° dönüş
ve ara açılar, perspektif; eksik/örtülü nokta, kırpılmış çerçeve/işaret, yanlış
grid, çok plaka, sahte üçgen, duplicate, blank, blur/doygunluk ve iptal.
Çapraz yön kimliği, raw koordinat geri dönüşü ve deterministik sıralama ölçülür.
Sınırlı sentetik eğrilik örneği gözlem/H kabul ayrımını sınar; sınırsız lens
bozulması toleransı taahhüt edilmez. Sayısal lokalizasyon toleransı fixture
doğruluğu/kenar belirsizliğine göre gerekçelendirilir, yalnız sayı eşitliği yetmez.

Release + calibration_plate_contract/plane_mapping_contract/chessboard_contract.
Ham formatlar Gray8/16/RGB/RGBA ve limitler korunur. Gerçek eski karede hâlâ
başarısızsa nedeni/kanıtı yaz; gerçek plaka algılaması tamamlandı deme.
Kanıt `build/ccux/ccux-15f/`. Devam 15g.

### CCUX-15g — model/kayıt ve yeniden oynatma

Oku: PlaneMappingService encode/decode, CalibrationModelRecord,
CalibrationWizardSession, ScopedActiveCalibrationService::saveAndUse,
ActiveCalibrationService ve ilgili persistence testleri.

- Yeni profili mevcut homografi solver'ına bağla. Tam 49, birebir kimlik/yön,
  1 px inverse max, hull ve kontrol noktası bağımsızlığı korunur.
- Descriptor/yön/raw gözlem için gerekli yeni şemayı ekle; katmanlar arasında
  hedef ve observation ID/hash bağı doğrula. Yeni veri için yeni sürüm; eski
  bytes/hash no-op fixture'ları korunur. Unknown sürüm/alan açık ret.
- Replay saklı başarı bayrağına/matrise güvenmesin; kanıttan model denetimlerini
  yinele. Yön kanıtının doğrulama kapsamını açık yaz; yerel hash'i imzalı
  sertifika gibi sunma. Yanlış hedef, değişen piksel/sidecar, sahte kimlik,
  crop/origin ve yanlış kaynak negatiflerini denetle.
- İptal, kayıt hatası, eski revision ve etkinleştirme reddinde önceki seçimi
  koru. Aralık/ROI/hedef/kare değişimi mevcut gözlem/onay/adayı geçersiz kılsın.
- Rapor yön alanı yeni profilde gerçek kanıtı anlatsın; legacy kayıtlara
  “yön tespit edildi” yazılmasın. Yeni profil legacy recipe-only yola kaçmasın.

Release + plane_mapping_contract, calibration_wizard_contract,
calibration_save_use, scoped_active_calibration, active_calibration_contract;
değişen codec için measurement_environment_persistence/calibration_persistence.
Kanıt `build/ccux/ccux-15g/`. Devam 15h.

### CCUX-15h — sihirbaz ve tanı

Oku: CalibrationWizard, CalibrationWizardSession, kullanılan image viewer,
CalibrationAcquisitionSession ve CCUX-22e API; yalnız ilgili UI testleri.

- Açık çerçeveli plaka seçimi ve 7×7 profil, merkez aralığı teyidi; bilinmeyen
  fiziksel alanlara zorunlu uydurma değer yok. Eski seçeneği sessiz dönüştürme.
- Durum tablosunu, overlay/numara/eksen/hull gösterimini uygula. ROI ve zoom
  görüntüyü değiştirmez; seçim koordinatı raw'a doğru taşınır. Renk tek bilgi
  taşıyıcısı olmaz; normal/küçük pencere ve HiDPI mantıksal konumları denetle.
- Worker kullan; yeni çekim/target/ROI/ayar değişimi eski işi iptal/invalidate
  eder. Geç completion yeni ekranı güncelleyemez. Stop/iptal/dosya geçişi ve
  KaydetUse mevcut drain semantiğini korur; Camera Manager'la lease çakışmaz.
- Algılama başarılı fakat fit reddedilmişse 49 merkez gösterilebilir; KaydetUse
  etkinleşmez. Fiziksel doğrulama eksikliği mevcut ürün politikasıyla gösterilir,
  yeni lens ayar ekranı veya otomatik pozlama eklenmez.

Release UI build, ui_calibration_wizard/ui_independent_calibration_wizard,
calibration_camera_sessions_contract, calibration_wizard_contract;
viewer değişirse roi_viewer_interaction. İki pencerede gerçek görsel kontrol:
başarı, ek aday, eksik nokta/yön, fit ret, busy/iptal ve yeniden açılış.
Kanıt `build/ccux/ccux-15h/`. Devam 15i.

### CCUX-15i — birleşik yazılım kabulü

Oku: scoped calibration Editor/Runtime, experimental metric testleri ve 15g/h
kapanışları; tamamlanmış yürütme mimarisini yeniden tasarlama.

- Yeni profilli modelle normal SaveUse → Editor mm → Stop/Load/Runtime mm →
  Logs/CSV hattını üç applicationType'da doğrula. Raw/px dalı ayrılığını koru.
- Restart/USB benzetimi, kaynak/geometri/model revision değişimi, lens/kurulum
  invalidation, A→B→A, yanlış kameranın modeli ve eski aktif snapshot negatifleri.
- İkinci kamera fixture'ı ile model sızıntısı olmadığını kontrol et; fiziksel
  ikinci kamera yoksa donanım testi diye sunma. PLC/robot kapalı kalır.
- İki Release build'de ilgili regresyonları birleşik bir kez koş; 22f testleriyle
  ortak kanıt varsa açık bağlantı kur. Önceki multi_source_production_contract
  hatasını gizleme veya yeni çalıştırılmadan geçti deme.

CTest: calibration_plate_contract, plane_mapping_contract,
calibration_wizard_contract, calibration_save_use, scoped_active_calibration,
scoped_calibration_editor, scoped_calibration_runtime,
experimental_metric_runtime_contract, calibration_camera_sessions_contract;
UI için ilgili sihirbaz/activation ve multi-source smoke hedefleri. Tam suite
iddiası yalnız gerçekten tam koşumla. Kanıt `build/ccux/ccux-15i/`. Devam 15j.

### CCUX-15j — fiziksel devreye alma

Oku: ana CCUX-15 kartı, calibration-acceptance.md, kamera tanı rehberi; yeni
algılayıcı testlerinin başarılı olması fiziksel önkoşulların yerine geçmez.

- Exact kamera/seri, lens/odak/düzlem, gerçek geometri, plaka ID/aralık kökeni,
  bağımsız referans/belirsizlik ve iş toleransını kaydet. Şu an bilinmeyenler
  kullanıcı/ölçüm gerektirir; bunlar yazılım kartlarını gereksiz yere durdurmaz.
- Yeni raw/sidecar al; çalışma alanında farklı yerleşim ve kullanılabilir
  pozlama seviyelerinde 49 merkezin doğru bulunduğunu görsel doğrula. Bunlar
  lens çözümü değildir; her kareden model geçerliliği ayrı değerlendirilir.
- Ölçüm modeli için plaka ölçülecek parçayla aynı düzlemde olsun. Odak ve kamera
  sabit; güvenilir alan hull ile sınırlı. Bağımsız referansla merkez/kenar,
  X/Y/diagonal ve tekrarları kabul rehberine göre ölç; fit noktaları holdout olmaz.
- Gerçek SaveUse → Editor/Runtime mm ve Logs/CSV, Stop/reopen/restart deneyimi;
  operatör grid yönünü ve işlem akışını teyit etsin.
- Fit veya fiziksel hata bütçesi aşılırsa toleransı gevşetme, yalnız iyi
  örnekleri seçme veya distorsiyonu gizlice açma. Kurulum/ref/plaka nedenlerini
  incele; lens düzeltmesi gerekirse fiziksel devreye alma açık kalır.

Kabul: yeni tarihli raw/sidecar/model/reçete/ölçüm CSV ve kullanıcı kabul kaydı
birbirine bağlı; sayısal tolerans karşılaştırması, engeller ve gerçek test
kapsamı açık. Kamera/ref yoksa 15j tamamlanmaz; 15i yazılım hazır kalabilir.
Kanıt `build/ccux/ccux-15j/`. Sonraki iş sonuçlara göre 22f açık maddeleri veya
ayrı kullanıcı kararıyla lens planıdır; lens uygulaması otomatik başlamaz.

## Çalıştırma/kapanış kuralları

Build komutları ana README'de. Önce `ctest --test-dir build/reference-ui -N` ile
seçili hedeflerin varlığını doğrula; sıfır test başarı değildir. Kaynakta adı
geçen test hedefleri bu plan anında CMakeLists'te mevcuttur; yeni test gerekirse
kart kapsamında adlandırıp CTest'e kaydet. UI kapalı build'de UI hedefi bekleme.
Manuel raw replay/süre/overlay komutunu kart kapanışında eksiksiz kaydet.
Hedefler/çıktılar, exit code, yeni/önceki kanıt ayrımı ve açık fiziksel maddeler
tarihli kapanışa; kısa özet progress'e. `python scripts/check-context-docs.py`
ve `git diff --check` zorunlu. Her kart bir commit; push yalnız kullanıcı isteğiyle.

Başlatma örneği: “CCUX-15e'yi uygula. Başlangıç paketini, ortak sözleşme C3a'yı,
15-plate-calibration-plan.md tasarım/sınır ve yalnız 15e kartını oku. Gözlem/model
ayrımını yap; distorsiyon açma. Kartın testlerini ve kapanışını tamamla.”
