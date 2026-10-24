# ICM — doğrulama ve fiziksel kabul planı

**Planlanan testlerdir; aşağıdaki satırlar henüz geçmedi.**
[Görev sırası](industrial-camera-plan.md), [hedef sözleşmeler](industrial-camera-contract-plan.md).
KMC-10e 87/87 UI ve 73/73 core tarihsel baseline'dır, ICM sonucu değildir.

21 Ekim: [EXP-01–03 kabul kartları](industrial-camera-oecs-experimental-plan.md)
eldeki OECS ile deneysel edinim, ölçüm ve Runtime için güncel sıradır. Başka
kamera/üretici yanıtı beklenmez. Gerçek görüntü ve test sonuçları kaydedilir;
iptal/boşaltma varsayımı G1/G3/G4 kanıtı sayılmaz. Aşağıdaki matris doğrulanmış
production kabulü içindir; EXP sonuçları bu kapıları otomatik kapatmaz.

## 1. Kanıt düzeni

Her görev `build/icm/<id>/` altında configure/build/test logu, JUnit XML, varsa raster,
JSON metadata ve görseller bırakır. Raporda source revision veya dirty-diff hash,
EXE/hash, SDK/producer sürüm/hash, tarih, exact komut, exit code, test sayısı, başarısız
ilk koşum ve tekrarlar bulunur. Kullanıcı beyanı, sentetik fixture, fiziksel gözlem ve
donanımsız disk replay farklı kanıt türleridir. build dosyaları Git'e eklenmez.

Yeni test adları task sırasında CMake'e kaydedilir; aşağıdaki S satırları kabul ID'sidir,
mevcut CTest adı değildir. Fixture gerçek cihazın yerine kullanılmaz. Ürün veri şemasına
test kolaylığı için güvenilir completion veya identity atlama yolu açılmaz.

18 Ekim kararıyla yazılım S matrisi 04f test adaptörü üzerinden native OECS beklemeden
uygulanabilir; G2=09a bağımsız yazılım kabulüdür. G0/G1/G3 backend/model/firmware,
G4 kurulum bazında tutulur. Fixture başarısı native capability açmaz. Gerçek
production capability ilgili G1 + 08b ister; fiziksel e2e/ölçüm beyanı G3/G4'e bağlıdır.

## 2. Yazılım kabul matrisi

| ID | Senaryo / oracle | Görev | Mevcut regresyon grubu |
| --- | --- | --- | --- |
| S01 | Missing/wrong x86 producer, missing export, 0 device, per-producer error | 01a/04a | camera_transport |
| S02 | Partial init/open failure; handle/library yaşamı; busy owner | 04a | camera_transport, session_contract |
| S03 | Same physical device via 2 producers, duplicate serial, no serial, rebind | 03a/05a | measurement_setup_contract, measurement_environment_persistence |
| S04 | Legacy bytes/hash/no-op Save; explicit yeni schema; unknown alan ret | 03a/03b/07a | tool_migration_contract, project_persistence |
| S05 | Width aynı/offset farklı; reverse/binning/unknown geometry | 03b/06b | measurement_setup_contract, scoped_active_calibration |
| S06 | Exposure/gain range, auto, int/float birimi, readback≠requested | 04b | Yeni node-map fixture |
| S07 | Kısmi configure + doğrulanamayan rollback → Fault | 04b | Yeni configure corpus |
| S08 | Padded rows, payload/chunk sınırı, incomplete, overflow, packed ret | 04c | owned_frame_contract |
| S09 | Buffer iade/değiştirme sonrası owned piksel sabit; leak sayacı | 04c | owned_frame_contract, yeni backend lifetime |
| S10 | Eski/duplicate/late frame, counter reset/wrap, trigger failure | 04d | acquisition_contract, yeni trigger fixture |
| S11 | Aynı görüntü hash'li iki yeni frame; farklı source/frame/cycle | 04d/07b | multi_source_execution |
| S12 | Cancel before/during/after wait; geç SDK başarı yayımlanmaz | 04d/04e | acquisition_contract, camera_lifecycle_contract |
| S13 | Preview drop / production FIFO Full; preview→cycle sızması yok | 04e | camera_lifecycle_contract |
| S14 | Calibration/Editor/Runtime lease yarışı; shutdown worker ownership | 04e/05a | session_contract |
| S15 | Reçetesiz industrial frame → gerçek math; file origin korunur | 06a | independent_calibration_capture, calibration_wizard_contract |
| S16 | Model/kurulum/cihaz değişimi; restart/replug izni otomatik dönmez | 06b | scoped_active_calibration, system_calibration_contract |
| S17 | Industrial + file/webcam DAG; yanlış source/model/ROI; acquisition0 | 07b | multi_source_execution, multi_source_target_only, editor_target_only |
| S18 | Aynı fiziksel kamera iki branch; bellek/deadline partial failure | 07b/08a | multi_source_graph_contract, multi_source_production_contract |
| S19 | Prepare/begin/actual identity/profile/geometri farklılığı | 08a | production_recipe_contract, runtime_coordinator |
| S20 | Admission sonrası UI seçimi aynı frozen cycle'ı değiştirmez | 08a | multi_source_production_contract |
| S21 | Caller forged/swapped evidence/digest; private completion ret | 08b | multi_source_terminal_trace, typed_terminal_trace |
| S22 | Record lock/failure retry'da trigger/capture sayacı artmaz | 08b | runtime_persistence, multi_source_terminal_trace |
| S23 | Restart/Logs/CSV SDK ve registry olmadan doğru tarihsel kanıt | 08b/08c | runtime_persistence, typed_terminal_trace |
| S24 | Stop/ERROR boş scalar; vf.socket2 seçilmiş gerçek scalar; pending ACK | 08b/08c | plc_socket_runtime, multi_source_terminal_trace |
| S25 | Save As/export/import profile korur, yerel producer/izin taşımaz | 03b/09a | measurement_setup_migration, project_persistence |
| S26 | 0/1/8/10 kaynak UI, küçük ekran, busy/error, üç applicationType | 05b/07c/08c | ui_calibration_overview, ui_multi_source_editor/runtime |
| S27 | SDK yokken core/UI startup + file/webcam; SDK sınırlı optional build | 04a/09a | backend_capabilities ve tam CTest |
| S28 | Son kaynak Release UI ON/ZXing ON ve core OFF/ZXing OFF | 09a | Tam test envanteri |
| S29 | Test adaptörü yalnız test enjeksiyonunda; normal factory/config/recipe üzerinden seçilemez; sentetik evidence native kabul edilmez | 04f/05a/08b | Yeni provider isolation corpus'u |
| S30 | Bir backend unsupported iken diğer backend'in bağımsız capability'si ve file/webcam çalışır; keşif/preview trigger izni vermez | 04f/05a/08a | backend_capabilities, yeni provider testleri |

Fixture node/stream katmanı gerçekten farklı pikseller ve bağımsız kimlik/event sırası
üretmeli. Testte beklenen değer implementation fonksiyonuyla yeniden hesaplanıp oracle
yapılmaz. Durağan görüntü tekrarı, frame counter wrap ve aynı-boyut crop ayrı negatiflerdir.

## 3. Build ve CTest çalıştırma

Yerel PATH/configure seçenekleri [Windows toolchain](../windows-workstation.md) ve
README'de. ICM-00 mevcut cache seçeneklerini doğrular; build dizinlerini silmez.
Örnek PowerShell (uygulama sırasında; bu plan yazılırken çalıştırılmadı):

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
cmake --build build/reference-ui -j 4
cmake --build build/kmc9-core-off -j 2
ctest --test-dir build/reference-ui -N
ctest --test-dir build/reference-ui --output-on-failure -R 'camera_transport|camera_lifecycle_contract|acquisition_contract|owned_frame_contract|session_contract' --output-junit icm-foundation-tests.xml
```

Regex sadece mevcut temel regresyonları seçer; yeni ICM testleri ayrıca eklenir ve
`ctest -N -R ...` ile boş seçki olmadığı doğrulanır. Etkilenen servis grubunu karttaki
isimlerle genişlet; ilgisiz tam suite'i her küçük edit sonrası tekrar koşma.

ICM-09a son kaynakla iki tam koşum:

```powershell
ctest --test-dir build/reference-ui --output-on-failure --output-junit icm-release-ui.xml
ctest --test-dir build/kmc9-core-off --output-on-failure --output-junit icm-release-core.xml
```

Her configure/build/CTest exit code ayrı kontrol edilir; başarısız build sonrası eski
EXE ile koşum yeni kaynak kabulü değildir. SDK yolu yalnız ilgili process'e eklenir;
makine PATH'i değiştirilmez. Fixture-only default CTest gerçek bağlı kamerayı açmaz.
Physical CLI/test explicit producer + expected serial ister; ilk bulunan cihazı seçmez.
Localhost gerektiren test erişim hatası ürün hatasından ayrılır, tekrar koşumu ayrı kaydedilir.

## 4. UI kontrolü

UI değişen her kartta Release ve ilgili UI CTest; en az 1400×900 ve 850×600 görünüm.
Camera Manager (boş/keşfedilmiş/offline/busy), Calibration (seçili camera/setup ve
activation failure), Editor (industrial+file source/ROI), Runtime (prepare/actual
frame/Stop/error) ve Logs/CSV kontrol edilir. 8/10 kaynak görüntüsü sentetik etiketlenir.
Görsel gerçekten açılıp okunur; sadece PNG dosyası oluşması görsel kabul değildir.
Offscreen kabul native DPI/çok monitör kabulü değildir; native smoke H08'dedir.
Panel ayarları test INI ile build altında; `config/local.ui.ini` korunur.

## 5. Fiziksel kamera kabulü

Önce H01, ardından ürün geliştikten sonra diğer satırlar. Başlangıçta bütün durumlar AÇIK.
Her backend/model için ayrı H01–H08 raporu açılır; OECS sonucu Basler/Daheng kabulü
değildir. USB3 kablo/port veya GigE NIC/switch/PoE/IP/MTU yapılandırması,
kamera/seri/firmware, SDK/hash, actual profile, PC/toolchain ve lens bilgisi
her fiziksel rapora bağlanır. Kamera datasheet fps'i test hedefi veya ölçülmüş sonuç sayılmaz.

| ID | Prosedür | Kabul / kayıt |
| --- | --- | --- |
| H01 | Reboot durumu → ilgili vendor viewer'da gerçek Mono8 görüntü → viewer release → seçili transport discovery/open | Exact model/seri ve raster kanıtı; 0 device başarı değil; OECS için mevcut 01b uygulanır |
| H02 | VisionFlow tek-frame ve 100 ardışık software trigger; sahne değişikliğiyle frame gözlemi | 100 kabul edilen isteğe 100 doğru tamamlanma veya açık hata; sessiz stale/duplicate/missing yok; HW counter varsa kaydı |
| H03 | 20 connect/configure/start/stop/disconnect; preview→production geçişleri | Kilit/handle/buffer sızıntısı belirtisi yok; cycle preview karesi tüketmiyor; her aşama sonucu |
| H04 | Idle/preview/acquisition sırasında kontrollü unplug; replug; 10 Stop denemesi | Hang/stale PASS yok; Fault/timeout açık; yeniden kullanım explicit reconfigure/activation/prepare ile |
| H05 | Vendor viewer cihazı tutarken VisionFlow open, sonra serbest bırakma | Busy/access denied anlaşılır; başka cihaza fallback yok; sonra temiz open |
| H06 | Profile değişimi, aynı çözünürlükte geometri farklılığı destekleniyorsa; restart | Eski model/Runtime kapısı ret; desteklenmeyen ayar testi unsupported olarak raporlanır |
| H07 | Tek camera ve camera+file recipe, üç applicationType; save/load/Run/cycle/Logs/CSV | Doğru source/frame/model; Run trigger0; error/Stop sahte sonuç yok; record retry capture0 |
| H08 | 30 dk bounded preview ve en az 1000 sıralı cycle; native pencere/kapanış | Dropped preview/production error sayıları; kaynak/bellek trendi; p50/p95/max süre; UI kapanışı ölçülür |

100/1000/20/30 dk ilk mühendislik kabulü örneklemidir, endüstriyel ömür/MTBF garantisi
değildir. Deadline/error rate/çevrim süresi hedefi önce beyan edilir; hedef yoksa performans
yalnız ölçülür. RAM/RSS baseline/peak/end ve SDK pool sayaçları raporlanır; gözlenen
bellek trendi tek başına tüm leak'leri dışlamaz. Stop bütçe aşımı gizlenmez.

H04 fiziksel kablo ve H01 reboot kullanıcı adımıdır. H06 test sonunda uygulanan ayarlar
geri okunarak doğrulanır; üretim kamera firmware'i değiştirilmez. H07 bir recipe türünde
olmayan robot pose veya fiziksel PLC sonucu üretmez. H08 UI close bütçesi ve cleanup
gerçek ölçümle raporlanır; kesilemeyen SDK varsa açık tasarım borcu bırakılır.

İkinci gerçek cihaz yoksa wrong-physical-camera ve N-device kapasite yalnız fixture
kanıtıdır; raporda ayrı açık satır tutulur. G3 tek cihaz kapsamıyla sınırlanır.

## 6. Fiziksel ölçüm / lens kapısı (ICM-10)

Öğretim plakasından bağımsız bilinen referans ve istenen mm toleransı gerekir.
Referansın kaynağı/belirsizliği, lens modeli, mekanik sabitleme, odak/diyafram,
çalışma mesafesi, aydınlatma, sensor/profile ve düzlem kaydedilir. Uygun referans
ve tolerans yoksa kabul verilmez, karakterizasyon yapılır.

Önerilen ilk deney: merkez + dört kenar + dört köşede, desteklenen kalibrasyon alanı
içinde, en az iki yön/uzunluk; koşul başına en az 30 yeni frame. Model valid alanı
dışına extrapolation yapılmaz; alan küçükse bu sınır raporlanır. Bilinen uzunluk ile
plaka modelinin uygun olduğu kurulumlar ayrı karşılaştırılır.

Her örnek actual raw frame/hash, source/setup/model, iki uç/geometri, known mm,
measured mm ve residual taşır. Bölge bazlı bias, max abs, RMS ve standart sapma;
tekrar öğretim ile acquisition tekrarı farklı deneydir. Fiziksel referans güvenilirliği
doğrulanmadan yalnız düşük fit RMS ile doğruluk ilan edilmez.

Karar: gerekli tolerans bütün alan/koşullarda karşılanırsa bu kurulum için G4;
sistematik kenar hatası/optik etkisi sınırı aşıyorsa lens T15 önceliklendirilir.
Ölçek/homography kontrol kapılarını gevşetmek çözüm değildir. Gelecek undistort yolu
ayrı coordinateSystemId/model bağımlılığı ve full ROI/replay kabulü ister.

## 7. Kapanış raporu şablonu

```text
Görev / kapı:
Kod sürümü / dirty diff özeti:
Backend / SDK / producer / hash:
Gerçek cihaz modeli / seri / transport:
Uygulanan profil / setup / model:
Yazılım testleri (komut, exit, sayı, log/XML):
Fiziksel testler (H-ID, ölçüm, raster/JSON):
Görsel kontrol (açılıp incelenen dosyalar):
Başarısız/koşulamayan testler ve nedeni:
Kalan sınırlar:
Sonraki görev ve kesin önkoşulu:
```
