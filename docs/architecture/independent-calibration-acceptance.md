# KMC-9 — A teslimi birleşik yazılım kabulü

16 Ekim 2026. **A yazılım kabulü tamamlandı:** Release UI/ZXing ON tam CTest 80/80 (310.02 sn), Widgets OFF/ZXing OFF 68/68 (139.32 sn). Son erişimli iki koşum exit 0; loglar build/kmc9-final-tests-{ui,core-off}.log, XML her build altında kmc9-final-tests.xml. İlk sandbox UI 78/80, core 67/68; yalnız localhost PLC/Communication zaman aşımı, ilk log/XML korunur. Sihirbaz senaryosu 343 kontrol; iki CTest adı aynı executable çalıştırır. Kapsam: [KMC planındaki](independent-calibration-implementation-plan.md)
A teslimi; reçeteden bağımsız N kamera/kurulum yönetimi ve mevcut tek kaynaklı
Editor/Runtime entegrasyonu. B çok kaynaklı yürütme bu kabulün dışındadır.

## Kabul senaryoları

| Senaryo | Çalıştırılan test / gözlem |
| --- | --- |
| Kaydedilmiş boş proje, hiç reçete yok | `ui_calibration_wizard` / `ui_independent_calibration_wizard`: gerçek hesap/kayıt sonrası proje bytes ve boş recipes korunur. `ui_scoped_calibration_activation`: sıfır reçeteyle gerçek aktivasyon incelemesi. |
| N=0/1/2/8/10 | `ui_calibration_overview`: normal/küçük ekran, stable ID, on karta erişim, kaydırma/klavye, refresh edinim yapmaz. |
| N=1/2/8/10 farklı kalibrasyon | `scoped_active_calibration`: aynı ad/çözünürlük/raster, farklı gerçek ölçekler, exact seçim ve immutable snapshot, A değişimi B'yi değiştirmez. |
| Bilinen ölçü × üç köken | Bağımsız UI sihirbazında trial file, setup file, camera adapter fixture; gerçek 400px/10mm oracle. |
| İki plaka deseni × üç köken | Bağımsız UI sihirbazında chessboard/symmetric circle grid × trial file/setup file/camera adapter fixture; gerçek tespit/H, 45px/5mm komşu mesafe oracle, strict replay. |
| Kayıt/etkinlik/kontrol ayrımı | Sihirbaz kaydı ve satır seçimi etkinlik üretmez; kontrol yapılmadı/kabul sınırı yok/ret ayrıdır. `scoped_active_calibration`, aktivasyon UI. |
| Üç applicationType | `scoped_calibration_editor`, `scoped_calibration_runtime`, `system_calibration_contract`: ortak Editor/Runtime, gerçek px/mm, farklı etkin modeller, TargetOnly ve immutable admission. RobotGuidance şablonunda robot pose üretilmez. |
| Plaka H ve metrik tüketiciler | `system_calibration_contract`, `calibration_plate_contract`, `homography_runtime`, world/reference regresyonları: pointwise H, bağımsız oracle, alan dışı ret ve scale fallback olmaması. |
| Offline/remap/geometri/optik/düzlem | Registry, independent capture, scoped activation/Editor/Runtime; yanlış provider, ambiguous, actual çözünürlük, eski revision ve kullanılmayan offline kaynak ayrımı. |
| Dosya ilişkilendirme | Trial→setup açık türetimi; özgün kanıt/hash korunur, eksik/değişmiş raster ret, retry no-op. |
| Legacy boş/dolu ve yeniden açma | `project_persistence`, calibration/system/scoped testleri, migration/UI; eski byte/hash/no-op, wrapper1–3 ve bütün sürümler, eksik raster korunur, seçimsiz scoped durumda global fallback yok. |
| Kayıt hatası/kesinti/çatışma | Registry/activation/migration/transfer testlerinde kilit, expected revision, kesilmiş immutable prefix, project-last ve idempotent retry; eski seçim/özgün proje korunur. |
| Save As/import/export | `measurement_setup_migration`, aktivasyon UI: registry/katalog/legacy/production geçmişi, eksik hedef asset/foreign evidence ret, kopya ve restart kullanım izni vermez. |
| Stop/geç completion/pending kayıt/ACK | Independent capture, scoped Runtime, coordinator/socket/UI regresyonları; trusted completion/source/frame/origin/scope/downgrade ret; NOT_EVALUATED ve tarihsel Logs/CSV. |
| Backend capability | UI/ZXing ON gerçek kod fixture regresyonları; ayrı Widgets OFF/ZXing OFF derlemesi ve `backend_capabilities`, code/reference/tracking OFF kapıları. |

Bu matris ortak servisler ile UI senaryolarının birleşik kanıtıdır; her kamera
sayısı × yöntem × köken × applicationType kombinasyonunun ayrı bir fiziksel
uçtan uca denemesi değildir. `ui_calibration_wizard` ve
`ui_independent_calibration_wizard` aynı executable/senaryoyu çalıştırır;
CTest envanterindeki iki ad iki bağımsız corpus olarak sayılmaz.

## Koşum ve görsel kanıt

Son koşum sonuçları ve log/XML yolları [ilerleme kaydında](../progress.md)
tutulur. Release UI/ZXing ON: `build/reference-ui`; Release Widgets
OFF/ZXing OFF: ayrı `build/kmc9-core-off`. Önceki ON/core cache değiştirilmez.
Tüm testler kendi build fixture/INI dosyalarını kullanır.

README'deki process PATH ile tekrar koşum:

```powershell
cmake --build build/reference-ui -j 6
ctest --test-dir build/reference-ui --output-on-failure --output-junit kmc9-final-tests.xml
cmake -S . -B build/kmc9-core-off -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF
cmake --build build/kmc9-core-off -j 6
ctest --test-dir build/kmc9-core-off --output-on-failure --output-junit kmc9-final-tests.xml
```

Soket ve Communication testleri localhost erişimi gerektirir; sandbox kaynaklı
HELLO/Communication timeout ilk logda korunur, erişimli tam koşum ayrı log/XML'dir.

Görseller UI CTest ile yeniden üretilir: `kmc5-overview-{0,1,2,8,10}-{1400,850}.png`,
`kmc6-independent-{0,1,2}-small.png`,
`kmc9-plate{0,1}-origin{0,1,2}-{normal,small}.png`,
`kmc7a-source-properties-{normal,small}.png`,
`kmc7b-runtime-{normal,small}.png`, `kmc7b-logs-small.png`,
`kmc8-activation-{normal,small,offline}.png`, `kmc8-migration-small.png` ve
`kmc8-transfer-{small,progress}.png`; hepsi `build/reference-ui` altında.
origin0 deneme, origin1 kurulum dosyası, origin2 kamera adapter fixture'dır.
Qt offscreen render ve gerçek widget komutları/mouse/keyboard etkileşimi,
native masaüstü/DPI/çok monitör kabulü değildir.

## Açık sınırlar ve devir

N kalibrasyon yönetimi yazılım kapsamıdır; reçetede tek source sınırı sürer.
Fiziksel kamera/provider kimlik sürekliliği/USB/industrial grab, bağımsız gerçek
referansla ölçüm doğruluğu, fiziksel PLC/robot, lens düzeltmesi, senkronizasyon,
CUDA ve dağıtım bu testlerle doğrulanmaz. Kamera edinimi adapter fixture,
dosya edinimi gerçek PNG decode kullanır. Sürekli bağlantı izleme ve çok
dosyalı transaction/gerçek disk-full/power-loss kabulü de açık kalır.
KUI-6 ve T8.4.2b.3c'nin ayrı native/fiziksel/corpus kapsamı bu paketle kapanmaz.

Sonraki paket KMC-10a: çok kaynaklı DAG/frame sözleşmesi; A kapanışı B
uygulamasını başlatmış sayılmaz. [Fiziksel kabul prosedürü](calibration-acceptance.md)
ve [migration/taşıma sınırları](measurement-setup-migration.md) korunur.
