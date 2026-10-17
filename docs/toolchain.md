# Yerel araç kontrolü

15 Ekim 2026 iş bilgisayarı: Qt/MinGW/OpenCV/ZXing kurulum yolları, yeniden
derleme komutları ve güncel doğrulama kanıtları
[Windows geliştirme ortamı](windows-workstation.md) belgesindedir.
Aşağıdaki kayıtlar önceki bilgisayarda yapılan geliştirme kabulünün tarihçesidir.

10 Ekim güncel kapsam: **T8.4.2b.3b — Nominal world ROI authoring ve mm metric/tolerans tamamlandı.** Edge Point/Caliper/Line/Circle için **Nominal mm ROI öğret…** ekranı geometriyi, güncel pose teaching kaydını ve DAG bağlantısını atomik kaydeder; pending/iptal/geçersiz geometri reçeteyi değiştirmez. Caliper gerçek mm genişliğini, Line current mm span/açısını dahil tool toleranslarıyla değerlendirir. Circle tüm gerçek gözlemlerden (fit outlier dahil) radyal sapma, current world açı/gap/observed arc ve kaliteye bağlı tolerans üretir. Raw line/circle fit ve synthetic derivative strength unavailable; genel parça kararı NOT_EVALUATED. Engine **execution/19**, registry16/editor7/terminal3/vf.socket/2 korunur. Production pose whitelist reddi sürer. Sonraki **T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü**; T8.4 açık. Güncel Release binary: `build/tracking-ui/visionflow.exe`.

Doğrulama: Release core **50/50**, UI **56/56** tam CTest; world sampler/metric **2213**, world DAG **830**, pose/world authoring UI **829 kontrol** geçti. Normal/850×600 ve world ROI dialog görselleri incelendi. Kanıtlar `build/shape-core/t8-4g-final-ctest.log`, `build/tracking-ui/t8-4g-final-ctest.log` ve ilerleme kaydındadır.

Alttaki teslim kayıtları tarihseldir; güncel kapsam yukarıdadır.

10 Ekim güncel kapsam: **T8.4.2b.3a — Shape/Texture teaching ve ortak pose referansı editörü.** Toolbox artık16 araçtır; Pattern Pose / Shape Pose gerçek Gray8 model, explicit model-local anchor, açı/ölçek ve yöntem skor ayarlarıyla öğretilir. Edge Point/Caliper/Line/Circle/Code/Blob için ortak picker, güncel tam producer cache ve bağlı image ile ROI ilişkisini worker üzerinde öğretir; binding ve poseReference tek doğrulanmış edit olarak kaydedilir. Raw çözülen ROI salt okunur gösterilir; unavailable referansta nominal fallback yoktur. Mevcut nominal mm Circle öğretimi ve kayıtlı world envelope yeniden öğretimi aynı servistedir. Yeni Edge/Caliper/Line nominal mm ROI yazma ekranı, world metric/tolerans/current-world arc ve trusted production Runtime/Logs/CSV kabulü sonraki b.3b/b.3c kapsamındadır; **T8.4.2b.3/T8.4 açık**, production pose reddi sürer. Engine execution/18, registry16, editor7 ve terminal3 değişmedi. Güncel binary: `build/tracking-ui/visionflow.exe`. Release core **50/50**, UI **56/56** tam CTest; yeni teaching UI **687 kontrol** geçti. [Kullanım ve sınırlar](architecture/pose-roi.md).

Alttaki teslim/sıra kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.2b — Kalibre tracking teaching/recipe/DAG tamamlandı (feature kapsamı).** Ayrı `poseTracking` schema2 / `world-raw-rigid-v1`: nominal mm ROI SHA256, gerçek taught raw Pose2D, producer/model/anchor, kaynak/çözünürlük, exact calibration dependency ve explicit mm sampling unit. World geometri raw local ROI/AABB içine yazılmaz; eski raw envelope/hash korunur. Engine **execution/18**, validated resolver ve immutable mapping snapshot ile Edge Point/Caliper/Line/Circle/negative Arc örneklerini yürütür. Save/reopen/no-op/yeniden öğretme, iki gerçek matcher × üç applicationType, TargetOnly/acquisition0, cache/model/calibration değişimi ve Stop: **722 DAG kontrolü**; non-affine teaching/executor ve önceki sampler korumaları: **2178 world kontrolü**. Edge actual raw point, Caliper raw endpoint/px ve mm mesafe, Line raw observations/current mm fit, Circle/Arc current mm circle refit üretir. Synthetic strength, raw line/circle fit, taşınmış nominal arc açısı, defect/tolerans/accepted/PASS bu yolda explicit invalid kalır. Kullanıcı teaching UI ve production trusted completion/Runtime/Logs/CSV kabulü **T8.4.2b.3** kapsamında açıktır; T8.4 tamamlanmadı. UI/config değişmedi; production pose whitelist reddi korunur. Release core **50/50**, UI **55/55** tam CTest geçti; güncel binary `build/tracking-ui/visionflow.exe`. [Sözleşme ve sınırlar](architecture/pose-roi.md).

Alttaki teslim/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

## T8.4.2b.2a — World sampler/refit yazılım doğrulaması

README DLL PATH ile Widgets OFF/ZXing OFF `build/shape-core` ve Widgets ON/pinned offline ZXing ON `build/tracking-ui` Release derlendi; her dizinde `t8-4d-build-final.log`. Yeni dependency/download/kurulum yok. İlk sandbox core koşumu48/49,64.08sn; yalnız PLC HELLO ret (`t8-4d-ctest.log`). Nihai binary erişimli tam core **49/49,43.03sn**, UI **54/54,91.95sn** (`t8-4d-final-localhost-ctest.log`). `world_tracking_test.exe` **2134** kontrol; tam CTest'e dahildir. UI geometry2.29sn/circle2.04sn/shell25.27sn geçti; yeniden üretilen geometry-small/circle-small850×600 açılıp incelendi. UI kodu değişmedi; bu mevcut UI regresyonudur, yeni world tracking ekranı kabulü değildir. Ayrıntı ve sıradaki world recipe/DAG işi [ilerleme kaydında](progress.md).

10 Ekim güncel durum: **T8.4.2b.2a — Programatik world sampler/refit tamamlandı; T8.4.2b.2/T8.4 açık.** PoseRoiService::worldMap validated/current exact calibration ve aynı owned image snapshot ile nominal world → taught raw → current raw → current world zincirini kurar. WorldTrackingService Edge Point/Caliper/Line/Circle/Arc için her gerçek longitudinal/transverse tap üzerinde alan kontrolü, mm nokta/mesafe ve taşınmış gözlemlerden line/circle refit sunar. Bağımsız perspektif raster, gerçek iki matcher → uniform/Y-up world edge ve hata/iptal testleri **2134 kontrol** geçti. Bu programatik servis teslimidir; kalibre tracking teaching envelope/recipe/DAG/TargetOnly bağlantısı sıradaki **T8.4.2b.2b** işidir. UI/Runtime/Logs/CSV tracking kabulü b.3 kapsamında açık; production pose whitelist reddi ve execution/17 sürer. Standalone ToolRoi schema2 world scan/rectangle taşır; recipe Edge Point/Caliper/Line config bu world geometrileri henüz reddeder. UI ve yerel config değişmedi. [Sözleşme ve sınırlar](architecture/pose-roi.md).

10 Ekim T8.4.2b.1 tarihsel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](architecture/pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

## T8.4.1 — Shape/texture ROI referans doğrulaması

README DLL PATH ile mevcut `build/shape-core` (Widgets OFF) ve `build/shape-ui` (ON), ZXing OFF Release derlendi. `cmake --build build/shape-core -j 6` ve UI karşılığı; her dizinde `t8-4a-build.log`. Yeni bağımlılık/kurulum yok. Ortak PoseRoiService shape/texture method kapısı; `tracking_tools_test.exe` **398 kontrol**, core `t8-4a-tracking.log`. Her iki tam CTest koşumunda tracking contract geçti.

Tam sandbox CTest core **45/46,48.93sn**, UI **49/51,76.28sn**; yalnız localhost PLC HELLO ve UI Communication timeout, `t8-4a-ctest.log`. Aynı final binary ile erişimli tekrar core PLC **1/1,3.17sn**, UI PLC+shell **2/2,26.65sn**; `t8-4a-localhost-ctest.log`. Birleşik kanıtla 46 ayrı core ve 51 ayrı UI testi geçti; tek tam koşum 97/97 iddiası yoktur. UI kaynakları değişmedi; geometry/circle smoke regresyonları geçti, bu oturum yeni görsel inceleme veya tracking UI kabulü yapmadı. Binary `build/shape-ui/visionflow.exe`; Toolbox14/registry16/execution15/schema'lar korunur. [Servis kapsamı ve T8.4.2 kalanları](architecture/pose-roi.md).

## T8.3 — Shape Match doğrulaması

README DLL PATH ile yeni Release `build/shape-core` Widgets OFF ve `build/shape-ui` ON, ZXing OFF configure/build başarılı. `shape_match_contract`104 kontrol; yöntem unsigned-oriented-chamfer-pyramid v1, engine execution/15, registry16/Toolbox14. Nihai tam localhost CTest core **46/46,44.27sn**, UI **51/51,72.61sn**; her dizinde `t8-3-final-localhost-ctest.log`. İlk sandbox core45/46/UI49/51 yalnız PLC HELLO/Communication timeout; ilk log `t8-3-ctest.log`, erişimli tekrar `t8-3-localhost-ctest.log`. Final selected104 kontrol `t8-3-shape-final.log`, build `t8-3-build-final.log`. İlk Canny simetri bias/debug logları core altında korunur; debug2 tanısal minScore0 kabul değildir, final simetri .95 korunur. Yeni dependency kurulmadı. Final geometry-editor1400×900 ve geometry-small850×600 görselleri incelendi; mevcut küçük overlay etiketi kesilmesi sürer. Güncel binary `build/shape-ui/visionflow.exe`; yeni Shape UI/Runtime T8.4, native DPI/CUDA/kurulum/dağıtım/fiziksel kabul yok. [Kapsam ve sınırlar](architecture/shape-pose.md).

## T8.2 — Pose ROI adapter doğrulaması

README PATH ile `cmake --build build/pose-core -j 6` ve `cmake --build build/pose-ui -j 6`; Release/ZXing OFF, core Widgets OFF/UI ON. Yeni `tracking_tools_contract` 188 kontrol. Tam CTest sandbox core44/45 (37.74sn), UI48/50 (67.40sn); yalnız localhost PLC/Communication hataları. Aynı binary erişim izinli core PLC1/1 (3.06sn), UI PLC+shell2/2 (26.07sn); bütün45/50 testleri birleşik kanıtla geçti. Loglar her dizinde `t8-2-final-ctest.log`, `t8-2-localhost-ctest.log`; tracking ilk/debug/ara logları core altında korunur. Son acquisition-source binding değişimi sonrası iki Release build yeniden derlendi; final selected tracking core/UI1/1,188 kontrol (.62/.48sn), her dizinde t8-2-tracking-final.log. Tam suite önceki187 kontrol sürümündedir. Yeni dependency/UI değişimi yok. [Kapsam ve sınırlar](architecture/pose-roi.md).


## T7.2 — Kalibre geometri adapter

Ayrı Release `build/geometry-adapter-ui` ve `build/geometry-adapter-core`, ZXing OFF. README DLL PATH ile:

```powershell
cmake -S . -B build/geometry-adapter-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/geometry-adapter-ui -j 6
ctest --test-dir build/geometry-adapter-ui --output-on-failure --output-log build/geometry-adapter-ui/t7-2-final-ctest.log
cmake -S . -B build/geometry-adapter-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/geometry-adapter-core -j 6
ctest --test-dir build/geometry-adapter-core --output-on-failure --output-log build/geometry-adapter-core/t7-2-final-ctest.log
```

Tam koşular PLC/UI için localhost erişimiyle yapılır. İlk UI koşusunda eski registry Line Fit v1 assertion'ı tek hataydı; v2 observationsPx sözleşmesine göre düzeltildi, `t7-2-first-ctest.log` korundu. İlk test compile'daki mixed auto int/QString bildirimi ayrıldı. Yeni adapter testi `geometry_measurement_service_contract`; selected verbose çıktı UI dizininde `t7-2-adapter-ctest.log`. Test INI/fixture/raster/PNG/log yalnız build altında. Yeni UI binary geometry-adapter-ui/visionflow.exe; Geometry Toolbox'a henüz açılmaz, kullanıcının mevcut açık uygulaması/config korunur. [Adapter sözleşmesi](architecture/geometry-measurement-adapter.md), [nihai test/görsel kanıt](progress.md). Kurulum/dağıtım/CUDA/fiziksel hassasiyet kabulü değildir.


## T7.1 — Geometrik domain çekirdeği

Yeni ayrı Release `build/geometry-core`, Widgets OFF / ZXing OFF. Mevcut Qt/OpenCV/MinGW kurulumuyla:

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/geometry-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/geometry-core -j 6
ctest --test-dir build/geometry-core -R geometry_measurements_contract --output-on-failure -V --output-log build/geometry-core/t7-1-geometry-ctest.log
ctest --test-dir build/geometry-core --output-on-failure --output-log build/geometry-core/t7-1-localhost-ctest.log
```

Geometri testi yalnız `visionflow_projects`/Qt Core'a bağlanır. Tam suite PLC için localhost erişimi gerektirir; sandbox koşusunun `expected server HELLO` hatası `t7-1-final-ctest.log` içinde korunur. Nihai sonuçlar [progress](progress.md), matematik sözleşmesi [GeometryMeasurements](architecture/geometry-measurements.md). UI kaynağı değişmedi; yeni UI build/görsel kabulü yoktur, mevcut `build/circle-ui/visionflow.exe` korunur. Kurulum/dağıtım/CUDA veya fiziksel ölçüm kabulü yapılmadı.


10 Ekim T6.4 tarihsel kayıt: **T6.4 — Circle / Arc UI ve kalibre Runtime tamamlandı**. Toolbox `circle_fit` v1; raw/world halka-yay öğretme, tutamaçlar, gerçek mm düzleminde fit ve raw inverse overlay. Ayrı çap/sampled sapma dahil limitleri; kalite yoksa NOT_EVALUATED, outlier kusurdan çıkarılmaz. TargetOnly, immutable Runtime, tamper replay, typed Logs/restart/CSV ve Stop bağlıdır. Engine **execution/11**, editor7/terminal3/vf.socket/2 korunur; world ROI opt-in schema2. Güncel Release UI `build/circle-ui/visionflow.exe`. Sonraki **T7.1 — Geometrik domain matematiği**. [Kullanım, birimler ve sınırlar](architecture/circle-tool.md).

10 Ekim T6.3 tarihsel çekirdek kaydı: **T6.3 — Circle kalite/metrikler tamamlandı**. Fit dışı bulunan noktalar dahil radial min/max, signed inward/outward ve peak-to-valley; found coverage/nominal gap kapıları, full-circle/observed-arc/unavailable kapsamı. Circularity yalnız eksiksiz gerçek basit kapalı polygon'dan 4*pi*A/P²; missing/arc/self-intersection için unavailable, sahte kapanış yok. Yalnız çekirdek; Circle UI/registry/kalibre Runtime/Logs T6.4'tedir. Mevcut UI `build/metrology-ui/visionflow.exe`, execution/10/editor7/terminal3/vf.socket/2 korunur. Release OFF/core **40/40**, yeni metrik592 kontrol. Sonraki **T6.4 — UI ve kalibre Runtime**. [Sözleşme ve kanıt](architecture/circle-metrics.md).

10 Ekim T5.3 tarihsel kayıt: **T5.3 — UI/kalibrasyon/kabul tamamlandı**. Toolbox Edge Pair / Caliper ve Line Fit; reçeteye bağlı ok/kutu/Properties, gerçek px veya güncel kalibrasyonla mm, world noktalarından yeniden çizgi fit ve fiziksel açı, dahil tool tolerans Boolean’ı ve raw destek/outlier overlay. TargetOnly, immutable Runtime, typed Logs/restart/CSV ve Stop bağlıdır. Genel parça kararı/confidence/pose üretilmez. Engine **execution/10**, editor7/terminal3/vf.socket/2 korunur. Güncel Release binary `build/metrology-ui/visionflow.exe`; kullanıcı uygulaması/config korunur. [Kullanım, portlar, 128-scan reçete sınırı ve kanıt](architecture/metrology.md). Sonraki **T6.1 — radyal tarama**; fiziksel K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA kabulü açık. Aşağıdaki kayıtlar tarihseldir.

## T4.2 — EdgeFinder ve edge_point

Yeni bağımsız Release dizinleri; [sözleşme ve kabul kümesi](architecture/edge-point.md). Önceki açık UI/binary/config korunur. Mevcut pinned ZXing prefix'i yereldir; indirme yapılmaz.

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/edge-point-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix
cmake --build build/edge-point-ui -j 6
ctest --test-dir build/edge-point-ui --output-on-failure --output-log build/edge-point-ui/t4-2-final-ctest.log
cmake -S . -B build/edge-point-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_WITH_ZXING=OFF
cmake --build build/edge-point-core -j 6
ctest --test-dir build/edge-point-core --output-on-failure --output-log build/edge-point-core/t4-2-final-ctest.log
```

Mevcut PLC/Communication regresyonu için localhost erişimi gerekir. Yeni `edge_point_contract` 359, `edge_point_tool_contract` 27 kontrol; ProfileSampler 3893 kontrolü korunur. 42 clean + ayrı 42 degraded örneğin ground truth ve error raporu `edge-point-report.txt` içindedir. İlk tool test build'inde tek `auto` bildirimindeki QString/int ayrıldı; eski eksik executable sonucu kabul sayılmadı. Sonra insufficient derivative support açık ERROR ve plateau'nun işaret değiştirmeme denetimi tamamlandı. Normal/850×600 `code-editor.png` görselleri yeni UI build'de açılıp incelendi; Toolbox dokuz girdiyi korur, Edge Point editörü bu teslim değildir. Son tam sonuç ve süreler [progress](progress.md).

## T4.1 — Profil sampler çekirdeği

Yeni bağımsız Release/Widgets OFF/ZXing OFF; UI kodu ve açık uygulama korunur. [Profil sözleşmesi](architecture/profile-sampling.md).

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/profile-sampler-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_WITH_ZXING=OFF
cmake --build build/profile-sampler-core -j 6
ctest --test-dir build/profile-sampler-core --output-on-failure --output-log build/profile-sampler-core/t4-1-final-ctest.log
```

Son tam suite yerel soket erişimiyle **32/32**, 16.57 sn; `profile_sampler_contract` **3893** algoritma kontrolü. `profile-sampler-report.txt` yedi açı/gerçek step/analitik yoğunluk hatasını kaydeder (max 4.263256414560601e-14). İlk sandbox tam suite 31/32; yalnız PLC HELLO başarısız, son başarılı kabul onun yerine kullanılmalıdır. İlk iki yeni test koşusunda fixture Gray16 hizalaması ve erf raster quantization oracle düzeltildi. UI değişmedi; yeni UI build/görsel kabul veya ZXing ON kabulü bu adımda yapılmadı. Güncel kullanıcı binary'si `build/preprocess-code-ui/visionflow.exe` kalır. Edge konum hata raporu T4.2, fiziksel K4 ayrıca açıktır.

## T2.3 / T3.1 / T3.2 — Preprocess ve kod çekirdeği

Yeni Release dizinleri; açık uygulama ve `config/local.ui.ini` korunur. [Uygulanan sözleşme](architecture/preprocessing-codes.md). ZXing ON testleri bağımsız yerel Zint fixture'ları ister; normal configure indirme yapmaz. Güncel indirmesiz bootstrap iki fixture grubunu üretir:

```powershell
# T0.2 kaynak/arşiv/prefix zaten build/deps içinde hazırsa:
./scripts/bootstrap-zxing.ps1
# Yalnız yeni fixture üretimini tekrarlamak için:
cmake --build build/deps/fixture-build --target code_fixture_generator -j 4
./build/deps/fixture-build/code_fixture_generator.exe tests/dependency-fixtures/codes-t3.tsv build/deps/code-fixtures-t3
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/preprocess-code-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix
cmake --build build/preprocess-code-ui -j 6
ctest --test-dir build/preprocess-code-ui --output-on-failure --output-log build/preprocess-code-ui/final-ctest.log
cmake -S . -B build/preprocess-code-core-off -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/preprocess-code-core-off -j 6
ctest --test-dir build/preprocess-code-core-off --output-on-failure --output-log build/preprocess-code-core-off/final-ctest.log
```

ON/UI 34 test, OFF/core 31 test. OFF tests reader/model/encoder indirmeden çalışır; code ekleme açık ret, acquisition ve preprocessing çalışır. PLC/smoke localhost erişimi gerektirir; sandbox timeout'u başarı sayılmaz. `preprocessing_contract` 58, `code_reader_contract` 274, ON `code_tool_contract` 31 kontrol; genel TargetOnly, typed replay, dependency, kalibrasyon/mm ve Runtime regresyonları da çalışır. Güncel binary `build/preprocess-code-ui/visionflow.exe`; görseller aynı dizindeki `preprocess-editor.png` ve `preprocess-editor-small.png`, test INI/proje/archive/CSV yalnız build altındadır.

T3 fixture'ları aynı bağımsız Zint generator'ından UTF-8 ECI QR, ham GS içeren Data Matrix ve Code128 üretir; GS1 AI parser kabulü değildir. Zint yalnız ayrı test encoder'ına bağlıdır. Bu oturum kaynak indirmedi veya SDK kurmadı; eski sabit ZXing/Zint source/prefix kullanıldı. Hasarlı/DPM/baskı/gerçek corpus, fiziksel K4, native DPI, dağıtım/kurulum/CUDA kabulü yoktur. Sonraki T3.3.

## T0.2 — ZXing reader-only bağımlılık kabulü

9 Ekim 2026: ZXing-C++ **2.3.0**, kesin commit `d6068bcebeb8fd9f0d35a99b00d202be86a14dbe`; GitHub codeload ZIP SHA-256 `41ee4d0338a9bfc3729238cc5e09a69f56fc376d105d5d124793eaf857c0b3c5`. Seçim sabit API ile reader-only MinGW kabulü içindir; en güncel sürüm olduğu iddia edilmez. [Upstream release](https://github.com/zxing-cpp/zxing-cpp/releases/tag/v2.3.0), [manifest](../third_party/manifest.json).

Bağımsız fixture encoder'ı Zint **2.15.0**, commit `4896136bdbebb1a0908a5cc419513efa5bf4bf70`; codeload ZIP SHA-256 `3e450b6c463e82d2983699b2db0593f5139af5659d5407181be301c7f83a013c`. Yalnız static backend/test generator: Qt/PNG/CLI/frontend kapalı; ZXing veya ürünle link edilmez. Metin/format/semboloji ground truth `tests/dependency-fixtures/codes.tsv`; raster boyutları Zint çıktısından, scale=3, height=60, iki yönde 12 modül boşluk, HRT kapalı. Siyah/beyaz RGB raster'dan kayıpsız Gray8 PGM; fixture üretiminde ZXing kullanılmaz. Sekiz fixture altı aileyi ve EAN/UPC alt türlerini kapsar.

```powershell
# İlk kez: açık kaynak indirme (yalnız build/deps/ altına; sistem kurulumu yok).
.\scripts\bootstrap-zxing.ps1 -Download
# Yerel arşivleri geri yükledikten sonra / sonraki tekrar: indirme yapmaz.
.\scripts\bootstrap-zxing.ps1
.\scripts\probe-zxing.ps1
```

Bootstrap her arşivi SHA-256 ile doğrular ve doğrulanmış arşivi yeniden açar; mevcut lisans kopyasını upstream dosyayla karşılaştırır. Eksik cache `-Download` verilmeden hata olur; hash uyuşmazlığı indirmeyle sessiz düzeltilmez. Normal CMake configure kaynak indirmez; product CMake değişmedi. Yerel cache `build/deps/downloads/{zxing,zint}.zip`, source `build/deps/src/`, reader build `build/deps/zxing-build/`, install prefix `build/deps/zxing-prefix/`, bağımsız generator `build/deps/fixture-build/`, raster `build/deps/code-fixtures/`. Cache ZIP'leri aynı dizine geri koyarak indirmesiz bootstrap yapılır. Son tekrar mevcut build/prefix ile yapıldı; temiz makine/yeniden paketleme kabulü T0.3'te açıktır.

ZXing gerçek kaynak seçenekleri: `ZXING_READERS=ON`, `ZXING_WRITERS=OFF`, `BUILD_SHARED_LIBS=OFF`; `ZXING_EXAMPLES`, `ZXING_UNIT_TESTS`, `ZXING_BLACKBOX_TESTS`, `ZXING_C_API`, `ZXING_EXPERIMENTAL_API`, `ZXING_PYTHON_MODULE`, `ZXING_USE_BUNDLED_ZINT` kapalı; `ZXING_DEPENDENCIES=LOCAL`. C++20 required/extensions OFF, Release, GCC 13.1.0/x86_64-w64-mingw32/pointer 8. İlk deneme GCC PCH `MapViewOfFileEx` allocation hatası verdi; `CMAKE_DISABLE_PRECOMPILE_HEADERS=ON` ile kaynak yaması olmadan geçti. Static upstream export `Threads::Threads` istediğinden local dependency helper önce `find_package(Threads)` yapar. Reader Version.h compile kontrolleri writer kapalı/reader açık/2.3.0/x64 koşullarını doğrular; archive sembollerinde writer/ZBarcode API'si bulunmadı.

`libZXing.a` static; ZXing DLL'si yok. Probe PE doğrudan imports: `libopencv_core500.dll`, `libopencv_imgcodecs500.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `KERNEL32.dll`, `msvcrt.dll`. OpenCV mevcut kurulumdan gelir; encoder exe ZXing/OpenCV kullanmaz. Script PATH'e OpenCV ve MinGW bin ekler; bu liste transitive DLL paketleme/temiz makine kabulü değildir.

Lisanslar `third_party/licenses/`: upstream ZXing Apache-2.0 ve Zint LICENSE birebir; Zint backend BSD-3-Clause copyright/terms header kaydı; ZXing reader'ın embedded libzueci 1.0.1 BSD-3-Clause ve embedded UTF-8 DFA bildirimi. ZXing kaynağında ayrı upstream NOTICE yok; yerel embedded attribution dosyası prefix `share/licenses/ZXing/` altına da kopyalanır. Kaynakların tüm attribution header'ları arşivlerde korunur. Bu kayıt ürün dağıtım lisans paketlemesi kabulü değildir.

Doğrulama: ayrı Release probe CTest **1/1**, **33 kontrol/0 hata** (ilk 0,46 sn; son indirmesiz tekrar 0,10 sn). Her fixture'da bir geçerli sonuç, tam text/raw bytes/format, köşe bounds ve 2D nonzero alan; 90° dönüş ve non-contiguous row stride; yanlış format allow-list no-read; ayrıca blank no-read. Sonuçlar `build/zxing-probe/zxing-results.json`, `zxing-link-inventory.txt`, `compiler-target.txt`, `pe-imports.txt`, `zxing-artifact-inventory.json` (40 dosya SHA-256 yeniden hesaplanıp eşleşti), CTest log; bootstrap/probe logları `build/zxing-*.log`. Core incremental Release build başarılı/no-work; socket dışı **22/22** (6,30 sn), yalnız localhost PLC sandbox dışında **1/1** (3,20 sn); tek suite 23/23 denmedi.

İzole `build/bootstrap-negative/` script/manifest kopyasında eksik cache ve kasıtlı yanlış SHA-256 kaynak dosyası, ağ/expand/build başlamadan açık hatayla reddedildi. Gerçek iki cache ZIP hash'i manifestle yeniden eşleşti.

Sınırlar: temiz sentetik/sabit boyut fixture'ları; hasarlı/DPM/bulanık/gerçek baskı corpus'u, Unicode/GS1/multi-code, iptal/bütçe/ROI ve T3 ürün adapter kabulü değildir. Runtime code capability/eksik prefix kapısı **T0.3**; yeni UI/ürün tool'u/şema yok, UI build/görsel tekrar gerekmedi. config/local.ui.ini ve açık uygulama korundu; fiziksel/kurulum/dağıtım/CUDA kabulü yok. Aşağıdaki T0.1 kaydı tarihseldir.

## T0.1 — OpenCV API ve baseline kabulü

9 Ekim 2026: yeni ve ayrı `build/tool-baseline-core` dizininde C++20/Release/Widgets OFF configure ve 93 build adımı başarılı. Mevcut 23 testten 22'si sandbox içinde geçti (tam koşu 21,08 sn); `plc_socket_runtime` HELLO timeout verdi. Yalnız bu test sandbox dışında localhost ile tekrar **1/1** geçti (3,20 sn). Böylece 23 test doğrulandı; tek koşuda 23/23 sonucu değildir.

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/tool-baseline-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/tool-baseline-core -j 6
ctest --test-dir build/tool-baseline-core --output-on-failure
# Yalnız localhost izni gereken tekrar:
ctest --test-dir build/tool-baseline-core -R '^plc_socket_runtime$' --output-on-failure
.\scripts\probe-opencv.ps1
```

Probe ayrı `tests/probes/CMakeLists.txt` kullanır; normal ürün configure/build/test listesi değişmedi. Script normal configure sırasında ağ/indirme/kurulum istemez. Son script koşusunda **1/1 CTest, yedi API grubu geçti** (0,07 sn; ilk temiz compile/link/run 1,40 sn). Sabit görüntü blur/threshold/morphology, analitik ramp remap, çizgi slope/intercept, elips merkez/eksenleri (üç solver), dikdörtgen contour alan/perimeter/centroid ve homography fitinde kullanılmayan iki nokta doğrulandı. QR fixture OpenCV encoder'ından üretildi; beklenen `VISIONFLOW-T0.1`, dört köşe ve boş görüntü no-read doğrulandı. Bu QR bağımsız encoder/corpus kabulü değildir.

Kanıtlar `build/tool-probes/` içinde:

- `opencv-api-results.json`: compile/runtime sürümü 5.0.0, her probe sonucu, gerçek QR payload/köşe sayısı ve hata nedeni alanı.
- `opencv-build-information.txt`: runtime build bilgisi; GCC 13.1.0, shared Release, OpenCV'nin kendi standardı C++17. Tüketici probe C++20, `x86_64-w64-mingw32`, pointer 8 bayt.
- `opencv-link-inventory.txt`: imported target, include ve import-lib/runtime yolları; kurulu dokuz modül `calib core features flann geometry imgcodecs imgproc objdetect stereo`, link edilen altı modül `core imgproc imgcodecs objdetect calib geometry`.
- `opencv-binary-inventory.json`: config, sekiz API header'ı, dokuz import lib ve dokuz DLL için boyut/SHA-256; `compiler-target.txt`, QR PNG ve `Testing/Temporary/LastTest.log`.

Envanter algoritma planıyla uyumlu; API/DLL yükleme yerel MinGW ABI ile doğrulandı. CMake optional Vulkan header uyarısı core build'i engellemedi. Yeni tool, ZXing/Tesseract, UI, şema, SDK veya sistem kurulumu değişmedi; fiziksel doğruluk/dağıtım/CUDA kabulü değildir. Sıradaki **T0.2**.

Measurement/Calibration UI son kabulü: standart Release build/measurement-ui CTest 15/15; temiz Widgets OFF build/measurement-core Release CTest 13/13. Smoke Calibration/Measurement PASS/FAIL/invalid ve 850×600 görselleri üretir. Geometri/kalibrasyon sentetik; yeni fiziksel webcam, native DPI, kurulum veya CUDA kabulü değildir.

6C kayıt/binding son kabulü: build/calibration-release Release UI ON CTest 14/14; build/calibration-core Release Widgets OFF CTest 12/12. calibration_persistence evidence replay/current provider/schema 2 binding ve eski production validation reddini doğrular. Yeni SDK/toolchain kurulumu yapılmadı; fiziksel kalibrasyon, dağıtım ve CUDA kabulü değildir.

7 Ekim 2026, Windows x64, PowerShell. Aşağıdaki toolchain hazırlığında kurulum/indirme yapılmadı. Sonraki kullanıcı yönlendirmesiyle 6B'de Basler pylon Runtime 26.09.2 ve Daheng Galaxy tam SDK 2.6.2608.9131 kuruldu; Qt/OpenCV/compiler korunur. Kaynak, kapsam, DLL/env ve ABI kanıtı [kamera SDK kurulum kaydında](architecture/camera-sdk-installation.md). Basler tam Developer SDK formu açık; Runtime kurulumunu tam SDK kabulü olarak sunmayın.

| Araç | Tespit |
| --- | --- |
| CMake | 3.30.5, C:/Qt/Tools/CMake_64/bin |
| Ninja | 1.12.1, C:/Qt/Tools/Ninja |
| C++ | MinGW GCC 13.1.0, C:/Qt/Tools/mingw1310_64/bin |
| Qt / qmake / qtpaths | 6.12.0, C:/Qt/6.12.0/mingw_64 |
| OpenCV | 5.0.0, C:/OpenCV/5.0.0/x64/mingw/lib/OpenCVConfig.cmake |
| GPU | NVIDIA GeForce RTX 5050 Laptop GPU, driver 573.22 |
| CPU | 13th Gen Intel Core i5-13500HX; OpenCV varsayılan thread sayısı 20 |
| Git | PATH üzerinde mevcut; klasörde başlangıçta .git yok |
| Python | PATH üzerinde mevcut; bu hazırlıkta kullanılmadı |
| cl / clang++ / pkg-config / nvcc | PATH üzerinde bulunamadı; sistemde hiç kurulmadığı anlamına gelmez |

OpenCV MinGW DLL/import lib modülleri core/imgproc/imgcodecs mevcut. İncelenen kurulumda CUDA modül dosyaları bulunmadı; CUDA benchmark yapılmadı. GPU için CPU ölçümünden hız tahmini çıkarılmaz.

Release CMake yapılandırması ve üç hedefin derlemesi başarılı. `visionflow.exe --smoke-test`, Qt offscreen platformuyla 0 exit code döndürdü; Qt başlangıç/DLL yükleme doğrulandı. CMake isteğe bağlı Vulkan header'larını bulamadı; Widgets derlemesine engel olmadı. Build ve benchmark doğrulaması sonuçları `performance/opencv_matching.md` içinde tutulur. GUI görsel/etkileşim doğrulaması bu hazırlık aşamasında yapılmadı.

## A9 kaynak ve test konumu

TVision ve üst dizinler Git worktree değildi. Komşu TVis C++17 tvis projesidir; ProductionRecipeService kaydı yoktur. TVision/.git main yerel deposu kuruldu; remote yayın yapılmadı. scripts/validate-local.ps1 temiz build/a9-core ve build/a9-ui Release matrisinde geçti: 9/9 ve 11/11 CTest. Qt Gui bağımlılığı sürer; Widgets core build içinde aranmaz. Uzak Windows CI script üzerinden planlandı, henüz çalıştırılmadı.

## Webcam/Measurement teslimi

Qt 6.12.0 Multimedia development/runtime mevcut; CMake artık Qt6::Multimedia ister (Widgets OFF dahil). OpenCV videoio mevcut olmadığından kullanılmaz; kurulum yapılmadı. Güncel Release build/webcam-release 13/13, bağımsız build/webcam-core 11/11 CTest. Gerçek webcam kabulü ve kapsam progress.md/architecture/acquisition.md içindedir; dağıtım Multimedia plugin/DLL paketleme kabulü değildir.

7 Ekim 6C Runtime: `build/runtime-ui` standart Release CTest 16/16; `build/runtime-core` Widgets OFF Release 14/14. runtime_persistence ve Runtime UI gerçek dosya/Edge/Measurement, immutable draft, yetki, activation/start/terminal lock failure/retry, Stop, rollback, restart/archive/invalidation kabulü içerir. runtime-pass/fail/record-pending/invalid/logs/small PNG build/runtime-ui altında görsel incelendi. Fiziksel yeni cihaz veya power-loss/kurulum/dağıtım/CUDA kabulü değildir.

## K2/K3 kalibrasyon kabulü

OpenCV 5 yerel core/imgproc/imgcodecs/objdetect/calib/geometry modülleri; SB objdetect.hpp, perspective/homography geometry.hpp. build/k1-ui ve build/k1-core güncel Release K1–K3 binary/test konumudur; dizin adındaki k1 eski ilk configure adıdır. UI 19/19, Widgets OFF 17/17; homography_runtime ve plane_mapping_contract analytic independent points/persistence/Runtime kabulünü içerir. Kurulum/dağıtım/CUDA veya fiziksel ölçüm kabulü değildir.

## 6D.1 TCP/IP socket kabulü

Yerel Qt 6.12.0 Network development/runtime link edildi; CMake Qt6::Network ister (Widgets OFF dahil). Yeni kurulum yapılmadı. Aynı build/k1-ui ve build/k1-core standart Release binary'leri artık 6D.1'i içerir; tüm CTest UI 20/20 ve core 18/18. Son mapping seçicisi/mapping-yok değişimi sonrası UI plc_socket_runtime/ui_shell_smoke ve core plc_socket_runtime tekrar geçti. Actual localhost TCP, ayrı visionflow_plc_simulator process'i, ACK/timeout/duplicate/reconnect/record retry ve 850×600 görselleri doğrulandı. Sandbox localhost connect iznini engelledi; ağ testleri izinli sandbox dışında yalnız localhost ile çalıştırıldı. Fiziksel PLC/kamera, üretim doğruluğu, kurulum/dağıtım/CUDA kabulü değildir. [Sözleşme ve komutlar](architecture/plc-socket.md).

## T0.3 / T1.1 / T1.2 — offline capability ve tipli altyapı

9 Ekim 2026. Ürün ZXing seçeneği varsayılan OFF; mevcut reçeteler bu build'de çalışır. ON açıkça yerel static reader prefix'i ister, bulunamazsa configure hatasıdır; otomatik indirme veya başka kurulumdan fallback yoktur. Normal configure ağ kullanmaz. T0.2 bootstrap/pin/lisans süreci korunur. Başka makine için önce uyumlu Qt/OpenCV/MinGW toolchain hazırlanmış olmalı; aşağıdaki yollar bu makinenindir. Temiz makinede kurulum/dağıtım testi yapılmadı.

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
# İlk kaynak hazırlama yalnız açık -Download ile; sonra cache'den çalışır.
.\scripts\bootstrap-zxing.ps1 -Download
# ZXing açık bağımsız çekirdek
cmake -S . -B build/tool-foundation-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix
cmake --build build/tool-foundation-core -j 6
ctest --test-dir build/tool-foundation-core --output-on-failure --output-log build/tool-foundation-core/acceptance-ctest.log
# ZXing kapalı ürün/UI; prefix gerekmez
cmake -S . -B build/tool-foundation-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_WITH_ZXING=OFF
cmake --build build/tool-foundation-ui -j 6
ctest --test-dir build/tool-foundation-ui --output-on-failure --output-log build/tool-foundation-ui/acceptance-ctest.log
.\build\tool-foundation-core\backend_capabilities_test.exe 1
.\build\tool-foundation-ui\backend_capabilities_test.exe 0
```

Bu oturumda bootstrap/Download yeniden çalıştırılmadı; önceden kabul edilmiş `build/deps/zxing-prefix` kullanıldı. ON runtime capability: reader=true, version=2.3.0, static archive contentId `fdd2479e3a0fd0301e9d821e08c2ea0f4ef6d422ee41bffc01a4000c6c5e8b6d`; Get-FileHash ile eşleşti. OFF reader=false ve açık reason; Tesseract/unknown unavailable. Bu hash kaynak ZIP SHA-256'sından farklıdır, yeniden derleme/toolchain ile değişebilir. Binary kimliği recipe dependency'sine tam yazılır; değişim yeni doğrulama gerektirir.

ZXing static olduğundan ZXing DLL yolu yoktur; MinGW dinamik runtime (`libgcc_s_seh-1`, `libstdc++-6`, `libwinpthread-1`) ve Qt/OpenCV DLL PATH gereksinimleri sürer. Reader-only/x64/version assert tüketicide derlenir. `ZXing_DIR` cache'i seçilen prefix'e sabitlenir; archive değişirse CMake yeniden configure olur. Zint ürüne link edilmez.

Negatif configure (başarısız olması beklenir):

```powershell
cmake -S . -B build/tool-dependency-missing -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/does-not-exist
```

Bu komut eksik explicit offline prefix hatasıyla reddedildi. Relative mevcut olmayan prefix denemesi de build/tool-dependency-relative altında reddedildi; CMake PATH cache'i bu yolu mutlaklaştırır. Artifact/log ve test INI/PNG/projeleri yalnız build altında. UI smoke localhost TCP kullandığından sandbox içinde Communication timeout verdi; nihai kabul localhost erişimli suite'dir. Güncel sonuçlar [progress](progress.md), alan/limit/migration ayrıntıları [tool-values](architecture/tool-values.md). T1.3 birleşik Runtime/Logs/CSV kabulü, T3 code tool, T11 OCR ve T15 lens açık; fiziksel doğruluk/PLC/kamera/kurulum/dağıtım/CUDA kabulü yok.
## T1.3 / T2.1 / T2.2 — typed replay ve ROI kabulü

README DLL PATH ile, ayrı Release dizinleri:

```powershell
cmake -S . -B build/tool-roi-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/tool-roi-ui -j 6
ctest --test-dir build/tool-roi-ui --output-on-failure --output-log build/tool-roi-ui/final-ctest.log
cmake -S . -B build/tool-roi-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/tool-roi-core -j 6
ctest --test-dir build/tool-roi-core --output-on-failure --output-log build/tool-roi-core/final-ctest.log
```

9 Ekim son localhost erişimli kabul: UI 31/31 (29,67 sn), core 28/28 (18,15 sn). Yeni `typed_terminal_trace`, `roi_geometry_contract`, UI `roi_viewer_interaction`; mevcut TargetOnly/Runtime/persistence/PLC/smoke korunur. PNG/INI/proje/CSV bu dizinlerde; `t2-roi-*`, `t2-editor-roi*`, Runtime/Logs görselleri açılıp incelendi. Kaynak indirme/kurulum yapılmadı; existing ZXing prefix kullanıldı. [Sözleşme/sınırlar](architecture/roi-geometry.md), [ilerleme](progress.md). Preprocessing henüz yok; kurulum/dağıtım/CUDA/fiziksel kabul değildir.

## T3.3 — Kod Oku UI kabulü

Önce README DLL PATH ayarlanır. Mevcut Release ON/UI `build/preprocess-code-ui` ve OFF/core `build/preprocess-code-core-off` güncellendi; yeni indirme/SDK kurulumu yok.

```powershell
cmake --build build/preprocess-code-ui -j 6
ctest --test-dir build/preprocess-code-ui --output-on-failure --output-log build/preprocess-code-ui/t3-3-final-ctest.log
cmake --build build/preprocess-code-core-off -j 6
ctest --test-dir build/preprocess-code-core-off --output-on-failure --output-log build/preprocess-code-core-off/t3-3-ctest.log
```

ON/UI 34/34 (32,15 sn), OFF/core 31/31 (16,95 sn). Son küçük ekran düzenlemesi sonrası ilgili UI/ROI paketi 2/2 (20,49 sn), ayrıca `t3-3-layout-ctest.log` altında. İlk sandbox suite 32/34; localhost HELLO ve Communication timeout kabul sayılmadı; localhost erişimli tekrar ve son tam suite geçti. `code-editor.png`, `code-editor-small.png`, `code-runtime.png`, `code-logs.png` offscreen görsel kanıtıdır; native DPI/fiziksel corpus kabulü değildir. Test ayar/proje/arşiv/PNG yalnız build altında; `config/local.ui.ini` korunur. Sonraki T4.1.

## T4.3 — Edge Point Editor/Runtime/Measurement kabulü

README DLL PATH ile yeni Release dizinleri; kabul edilmiş yerel ZXing prefix kullanılır, indirme/kurulum yoktur.

```powershell
cmake -S . -B build/edge-editor-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/edge-editor-ui -j 6
ctest --test-dir build/edge-editor-ui --output-on-failure --output-log build/edge-editor-ui/t4-3-final-ctest.log
cmake -S . -B build/edge-editor-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/edge-editor-core -j 6
ctest --test-dir build/edge-editor-core --output-on-failure --output-log build/edge-editor-core/t4-3-final-ctest.log
```

Nihai localhost erişimli UI **37/37**, **34.54 sn** (smoke **20.97 sn**); OFF core **34/34**, **19.03 sn**. `edge_point_tool_contract` **41 kontrol**, bounded canlı tanı/late Stop ve büyük stripe rasterı dahil; T4.2 algoritma ve T4.1 sampler kabulü korunur. İlk sandbox Communication timeout kabul sayılmadı. Genişletilmiş undo testi görüntü bildiriminin oku silmesini ortaya çıkardı; ayrı `onRoiEdited` ile düzeltildi, son suite yeni görüntü/undo/geçici pencere izolasyonunu içerir. UI görselleri `edge-editor.png`, `edge-editor-small.png`, `edge-profile.png`, `edge-measurement.png`, `edge-runtime.png`, `edge-runtime-profile.png`, `edge-runtime-measurement.png`; incelendi. Offscreen/scroll gereksinimi native DPI veya fiziksel ölçüm kabulü değildir. Ayarlar/proje/rapor/PNG/log/arşiv build altında, config/local.ui.ini ve açık uygulama korunur. [Kullanım ve sınırlar](architecture/edge-point.md), [ilerleme](progress.md). Sonraki T5.1.


## T5.3 — Metrology UI ve core Release kabulü

10 Ekim 2026. README DLL PATH ile:

```powershell
cmake -S . -B build/metrology-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/OpenCode/TVision/build/deps/zxing-prefix -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/metrology-ui -j 6
ctest --test-dir build/metrology-ui --output-on-failure --output-log build/metrology-ui/t5-3-final-ctest.log
cmake -S . -B build/metrology-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/metrology-core -j 6
ctest --test-dir build/metrology-core --output-on-failure --output-log build/metrology-core/t5-3-final-ctest.log
```

Tam CTest localhost PLC fixture erişimiyle çalıştırılır. Metrology UI/core yeni build dizinleridir; açık kullanıcı binary'si ve config/local.ui.ini korunur. Test INI/proje/arşiv/PNG/log build altındadır. [Son sonuçlar ve görsel kabul](progress.md), [kullanım ve sınırlar](architecture/metrology.md). Kurulum/dağıtım/CUDA veya fiziksel doğruluk kabulü değildir.

## T6.1 — Radyal tarama Release çekirdek kabulü

UI dosyası/binary/config değiştirilmedi; yeni UI build/görsel kabulü yok. OFF/core Release:

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/radial-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/radial-core -j 6
.\build\radial-core\radial_scan_test.exe build/radial-core/radial-scan-report.txt
ctest --test-dir build/radial-core --output-on-failure --output-log build/radial-core/t6-1-final-ctest.log
```

Tam suite localhost PLC erişimiyle38/38,23.19sn; radial2.36sn.7528 bağımsız kontrol. İlk noisy width1 .6px gate başarısızlığı karakterizasyon olarak raporda kalır; aynı raster width4 ortalama ile ayrı .6px gate geçer. [Sayısal sınırlar](architecture/radial-scan.md). ON/ZXing/UI ayrıca derlenmedi; shared sampler/edge regresyonları tam OFF suite içindedir.

## T6.2 — Circle solver Release çekirdek kabulü

UI dosyası/binary/config değiştirilmedi; yeni UI build/görsel kabulü yok. OFF/core Release:

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake -S . -B build/circle-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/circle-core -j 6
ctest --test-dir build/circle-core --output-on-failure --output-log build/circle-core/t6-2-localhost-ctest.log
```

Tam suite localhost PLC erişimiyle **39/39,16.94sn**; circle1.60sn ve **295 kontrol**. İlk sandbox tam koşu38/39,31.73sn, yalnız PLC `expected server HELLO`; aynı binary erişimli koşuda geçti. Başarısız log `build/circle-core/t6-2-final-ctest.log`, nihai kabul `t6-2-localhost-ctest.log`. Rapor `build/circle-core/circle-fit-report.txt`: temiz24 raster diameter RMS.0125033/max.0170644px; degraded24 width4 max.134291px; world7mm projective diameter error−.004514mm. [API, solver birimleri ve kabul sınırları](architecture/circle-fit.md). ON/ZXing/UI ayrıca derlenmedi; T6.3 kalite/metrik ve T6.4 Circle UI sıradadır.

## T6.3 — Circle kalite/metrikler Release çekirdek kabulü

Mevcut T6.2 `build/circle-core` Release/Widgets OFF/ZXing OFF cache'i yeniden kullanılır. UI/binary/local config değişmedi; yeni UI build/görsel kabulü yok:

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake --build build/circle-core -j 6
ctest --test-dir build/circle-core --output-on-failure --output-log build/circle-core/t6-3-final-ctest.log
```

CMake source/test kaydı sonrası otomatik configure başarılı; son selected3/3/4.69sn, tam suite localhost PLC erişimiyle **40/40,26.75sn**, circle_metrics.36sn/**592 kontrol**. Rapor `build/circle-core/circle-metrics-report.txt`:6 raw Gray16 raster PVmax.0137575px; non-affine world7mm PV.00619884mm/index.999357, raw perspective PV4.23533px negatif kontrol. [Formüller, scope/quality, polygon ve numerical kabul sınırları](architecture/circle-metrics.md). Önceki T6.2 sandbox socket kanıtı nedeniyle full suite doğrudan localhost erişimiyle çalıştırıldı. ON/ZXing/UI ayrıca derlenmedi; sıradaki T6.4 Circle UI/kalibre Runtime.


## T6.4 — Circle UI ve kalibre Runtime

README DLL PATH ile Release ON/UI build/circle-ui ve OFF/core build/circle-core; `VISIONFLOW_WITH_ZXING=OFF`. Engine execution/11, ToolRoi world annulus/arc schema2, editor7/terminal3/vf.socket/2 korunur. UI45/45, core41/41; loglar ilgili build köklerinde t6-4-final-ctest.log. PLC localhost nedeniyle final CTest sandbox dışında çalıştırıldı.

```powershell
cmake -S . -B build/circle-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib
cmake --build build/circle-ui -j 6
ctest --test-dir build/circle-ui --output-on-failure
ctest --test-dir build/circle-ui -R 'calibrated_circle|ui_circle|roi_viewer' --output-on-failure
.\build\circle-ui\visionflow.exe
```

Core için build/circle-core ve VISIONFLOW_BUILD_UI=OFF kullanılır. Yeni UI eski çalışan binary'yi kendiliğinden değiştirmez. [Kullanım/birimler/kalite/tamper ve sınırlar](architecture/circle-tool.md), [nihai kanıtlar](progress.md).

## T7.3 — Geometry UI/tolerans/Runtime

README DLL PATH ile Release, Ninja, Qt 6.12 MinGW/OpenCV5. UI: `build/geometry-ui`, core: `build/geometry-t7-3-core`; configure seçeneği `-DVISIONFLOW_BUILD_UI=ON` veya `OFF`, `-DVISIONFLOW_WITH_ZXING=OFF`, diğer prefix yolları README ile aynı. `cmake --build <dizin> -j 6`, ardından `ctest --test-dir <dizin> --output-on-failure`. Localhost PLC/Communication sandbox erişimi ayrıca gerekir. Yeni binary `build/geometry-ui/visionflow.exe`; açık kullanıcı uygulaması değiştirilmez.

`geometry_measurement_service_contract` ve `ui_geometry_smoke` (`visionflow --geometry-smoke`, offscreen) seçili kabul; INI/raster/generated project/archive/PNG/loglar build altında. Geometry Editor/typed feature seçici/850×600/Runtime görselleri aynı dizinde; native DPI/çoklu monitör/kurulum/dağıtım/CUDA/fiziksel doğruluk kabulü değildir. [Gerçek sonuçlar](progress.md).

## T8.1 — Pattern Pose çekirdeği

README DLL PATH ile Release `build/pose-core` (Widgets OFF/ZXing OFF) ve `build/pose-ui` (Widgets ON/ZXing OFF). `cmake -S . -B build/pose-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib`; UI için dizini pose-ui, UI seçeneğini ON yapın. `cmake --build build/pose-core -j 6`; `ctest --test-dir build/pose-core --output-on-failure`. Yeni test `pose_match_contract`; verbose log `t8-1-pose-expanded.log`. [Kapsam ve sınırlar](architecture/pattern-pose.md).

Nihai localhost erişimli UI **49/49,52.21sn**, core **44/44,21.76sn**: her dizinde `t8-1-localhost-ctest.log`. İlk sandbox UI47/49,68.89sn (PLC HELLO ve Communication timeout), core43/44,41.52sn (PLC HELLO) logları `t8-1-final-ctest.log` içinde korundu. Selected Pose **79 kontrol,4.04sn**, `build/pose-core/t8-1-pose-expanded.log`. Aynı final binary'nin geometry-editor.png/geometry-small.png normal1400×900/küçük850×600 görselleri açılıp incelendi; yeni Pattern Pose UI kabulü değildir.
