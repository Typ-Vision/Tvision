# EFR-03a-1 — tek ROI crop sözleşmesi ve uygulama devri

28 Ekim 2026. **EFR-03a-2/3/4 core ve EFR-03b checkbox/viewer entegrasyonu uygulandı.**
[Ana plan](editor-flow-roi-update-plan.md#efr-03a--roi-kırpmanın-görüntükoordinat-sözleşmesi).
Bu belge EFR-03a-2/3/4 için bağlayıcı API ve dosya/test kapsamıdır.
EFR-03b checkbox/viewer kabulü ayrıdır. Mevcut Region özelliği çalışır;
ROI Image portu core katmanda vardır; core consumer kabulü EFR-03a-4 kapanışında, UI kabulü EFR-03b kapsamındadır.

## 1. Tek araç, parametre ve kayıt

- Kind daima `roi`; ikinci kind, legacy executor veya kullanıcı sürüm seçimi yok.
- Parametreler: `searchRoi`, `imageBinding`, `cropImage` (JSON boolean, default
  `false`). Eksik alan yalnız `cropImage` için false normalleştirilir; null,
  sayı, metin ve bilinmeyen alan reddedilir. Eski iki parametre korunur.
- Girdi `image: Image/px/image`; çıktılar sabit sırayla
  `region: Region/px/image`, `image: Image/px/image`. Port ID'leri moddan
  bağımsızdır. `region` ilk kalır; kod tercihen ID ile port seçer.
- `enabled=false` mevcut skip davranışıdır. `cropImage=false` çalışan ROI'dir:
  geçerli çizim/binding ister, Region üretir ve aynı immutable ImageFrame'i
  geçirir; content/processing/geometry/metadata değişmez, kopya gerekmez.
- `true`: integer QRect `(x,y,w,h)`, `w,h>0`, `x,y>=0`, checked
  `x+w<=W,y+h<=H`; clamp, maske, resize veya resample yok. 1×1 ve tam kare
  geçerlidir. Her iki modda boş/eksik/sınır dışı çizim ve değişmiş binding
  hata verir; hata/Stop çıktıları boşaltır, downstream mevcut skip kuralını izler.
- Region koordinatları **girdiye**, Image koordinatları açık modda **child'a**
  aittir. Her çıktının `PortValue.origin` değeri kendi koordinatlarını taşır;
  source/frame/cycle/zaman aynıdır. Aynı ROI'nin iki çıktısını tüketiciye
  bağlamak geometri eşitliği sağlamaz. İlk teslimde çapraz Region/reference
  frame bağları açık hata verir; gizli rebase yapılmaz. Consumer kendi crop
  alanında çizilmiş ROI veya tüm child rasterı kullanabilir.

EFR-03a-3 atomik değişiklik seti: `src/domain/ToolRegistry.cpp`
default/strict validation/ROI descriptor revision **1→2**, `ToolDefinition.cpp`
`editorTools()` yalnız ROI eksik alan normalleştirmesi, `RoiService.hpp`
teach/execute ve codec/graph/cache testleri. `teach` mevcut crop tercihini
koruyan argüman alır (`bool cropImage=false`); çizimi değiştirmek checkbox'ı
sessiz sıfırlamaz. UI çağrısı bu tercihi EFR-03b'de iletir.

Editor schema 7/8/9 ROI'yi zaten temsil eder: yeni editor/root/recipe şeması
gerekmez; iki çıktı tanımı registry'den gelir. Writer yeni ROI kaydında açık
boolean yazar. Eski dosyayı açma yalnız bellekte normalleştirir; kendiliğinden
dirty/revision artışı gerekmez. ROI içeren kaydın eski hash'ini birebir koruma
zorunluluğu yoktur; tek canonical false tanımıyla save/reopen kararlı olmalıdır.
ROI'siz kaydın byte/no-op/hash yolu değişmez. Eski UUID, isim, rect, imageBinding,
connections ve dependencies korunur. `ExecutionService::version()` gerçek
semantik değişiklikte EFR-03a-3'te artırılır; eski production validation yeniden
kullanılmaz. Kayıt güncellemesi ProjectService'in tek candidate/atomik save
yolundan geçer; açılışta UI veya edinim çalıştırılmaz.

### EFR-03a-3 uygulama sınırı ve sonraki karta API devri

`roi` descriptor revision 2, execution engine `/35` oldu. Okuma schema 7/8/9
missing `cropImage` alanını bellekte false'a çevirir; yeni yazım canonical
boolean taşır. Aynı ImageFrame false modda kopyasız geçirilir; true modda
`ImageAccess::cropped` kullanılır. `RoiService::teach(image, rect, bool=false)`
tercihi açık alır; UI çağrısının tercihi koruması EFR-03b kapsamındadır.

`PortValue::imageGeometry` optional immutable scope'tur. ROI Region girdinin,
Image çıktısı kendi frame'inin scope'unu taşır; execution bütün Image
çıktılarında bunu doldurur. Target-only ve SourceFrameIndex root coordinate
soyunu kabul eder, farklı local Image/Region girişlerini eşitlemez.
Production completion aynı soy kontrolünü yapar ve scope'u capability digest'e
katar. **Kalıcı strict scope codec/trace ve diğer consumer çıktıları EFR-03a-4
ile scope taşır**; replay yalnız kökeni geri yükler, ImageFrame/piksel veya
canlı çalışma izni üretmez.

ProjectService setTools/updateToolConnections/setConnections transaction'ları
upstream parametre/enabled/dependency veya bağlantı değişiminde downstream
ROI `imageBinding` alanını temizler. Search rect ve crop tercihi korunur;
A→B→A ve save/reopen öğretimi canlandırmaz. Rename/description etkisizdir,
ilgisiz dal korunur. Bu kart yalnız downstream ROI öğretimini temizler;
bütün model/reference/pose öğretim yolları EFR-03a-4 ile tamamlandı.
EditorSession reconcile sonuçla birlikte cached reference anchor ve eski
presentation imzasını da siler. Crop/preprocess allocation preflight ve
retained raster bütçesi mevcut multi-source execution sınırını paylaşır.

## 2. Owned raster ve koordinat API'si — EFR-03a-2

Yeni `src/vision/ImageGeometry.hpp/.cpp` OpenCV'siz, immutable değer sağlar.
`ImageFrame` geometriyi sahiplenir; dışarıdan keyfi child geometri takılamaz.
Önerilen public yüzey ve kesin anlamı:

```cpp
struct ImageGeometry {
    QSize rootSize, rasterSize;
    QString rootCoordinateSystemId, coordinateSystemId;
    QPoint rootOffset; // child (0,0)'ın edinim raw kökündeki pixel centre'ı
    QByteArray identity; // SHA-256; acquisition frame/cycle içermez
    int cropDepth;
    QPointF toRoot(QPointF rasterPixel) const;
    QPointF fromRoot(QPointF rootPixel) const;
    void requireFootprint(const QVector<QPointF>& rasterPixels) const;
};
// ImageFrame üzerinde yalnız const erişim; üretim/validation vision içindedir.
const ImageGeometry& ImageFrame::geometry() const;
static ImageFrame ImageAccess::cropped(const ImageFrame&, const QRect&,
                                     bool preview=true);
```

Alanlar temsili public gösterilmiştir; ImageFrame'e yerleştirme/sealing private
constructor/friend sınırında yapılır. Kök `fromBuffer`/dosya ediniminde
`rootSize=rasterSize`, offset=(0,0), depth=0, iki coordinate ID eşittir.
Kaynak/frame kimliği henüz verilmemiş kök, metadata atanırken kök olarak
bağlanır. Child'ın `withMetadata()` çağrısı dönüşümünü kaybetmez ve yerel
coordinate ID'yi değiştiremez; source/root değiştirme yeni kök edinimi ister.
Frame/cycle/zaman yeniden damgalamak yeni geometri üretmez.

EFR-03a-2 kesin public erişim: alanlar aynı adlı const getter'lardır
(`rootSize()`, `rasterSize()`, `rootCoordinateSystemId()`, `coordinateSystemId()`,
`rootOffset()`, `identity()`, `cropDepth()`). `ImageGeometry` OpenCV'sizdir;
root/child üretimi private, frame'e dışarıdan scope yerleştirme yoktur.
Boş varsayılan scope yalnız boş frame içindir ve mapping'de reddedilir.
`requireFootprint` boş vektörü reddeder; verilen bütün pixel centre'larını
denetler (bilinear komşuları üretmek sampler'ın sonraki kart sorumluluğudur).
Mevcut kök `withMetadata` yeniden bağlama davranışı uyumluluk için korunur:
source/coordinate değişirse yeni root identity oluşur. Bu API child'ı köke
çevirmez; child source ve local coordinate değişiklikleri açık hata verir.
Child oluşturmadan önce acquisition metadata'sı bağlanmalıdır; anonim kökten
üretilmiş child sonradan farklı kaynağa bağlanamaz. Kök kimliği canonical
JSON `image-root/1`, source, coordinate ve size içerir. Crop geometry identity
aynı canonical `roi-crop/1` girdisinin hash'i olduğundan ProcessingStep'in
`parametersId` alanında doğrudan bu 32 byte değer kullanılır.

`toRoot(p)=p+rootOffset`, `fromRoot(p)=p-rootOffset`; iki yönde sonlu sayı,
root ve raster pixel-centre domain `[0,w-1]×[0,h-1]` kontrolü vardır. Yerel
`(10,20)` ve rect `(300,200,100,80)` kökte `(310,220)` olur; ikinci crop
offsetleri checked int64 toplar. Depolamaya daraltmadan önce taşma ve kök
footprint kontrol edilir. Rect extent `(x+w,y+h)` kontrolü ile son pixel
centre `(x+w-1,y+h-1)` karıştırılmaz. Zincir en fazla 32 işlem adımıdır;
crop mevcut ProcessingStep bütçesini paylaşır, ayrı sınırsız zincir açmaz.

Kimlik SHA-256 canonical girdisi: sürümlü `roi-crop/1`, parent geometry
identity, parent coordinate ID/boyut, exact integer rect ve child boyut.
Kaynak ve raw kök koordinatı kök kimliğine dahildir; frame/cycle/content/zaman
değildir. Child coordinate ID, 256 karakter limitine sığan sabit prefix +
hex identity'dir. Aynı geometri yeni karede aynı ID, farklı rect/parent zinciri
farklı ID üretir. Tam kare crop da açık bir child dönüşümüdür. Aynı pikseller
geometri eşitliği kanıtı değildir; A→B→A geometri ID'si A'ya dönebilir.
**Teaching/cache uygunluğu bu eşitlikten ayrı yaşam döngüsüdür** (bölüm 5).

`cropped` valid satırları ayrı owned depoya kopyalar. Parent yok edilince child
yaşar; mutable Mat/view/SDK tamponu dışarı kaçmaz. Crop `input.format()` ve
kanonik piksel byte'larını korur. `fromBuffer` RGB8/RGBA8'i mevcut davranışla
BGR8/BGRA8'e normalleştirir; altı giriş formatı testi bu normalleştirmeden
sonraki doğru kanalları doğrular, gereksiz ikinci RGB swap yapmaz. Padding
çıktı pikseli sayılmaz. Preview child rasterdan üretilir.

`contentId` mevcut boyut/format/valid piksel hash'idir; `ProcessingStep`
`method=roi-crop,version=1,inputContentId=parent.contentId()` ve canonical
geometri/rect parametre SHA-256'sını taşır. `processingId` böylece aynı
piksel/farklı rect'i ayırır. `processed()` boyut kontrolü korunur; child üzerinde
preprocess çalışınca geometriyi aynen korur. Eski kök/no-op processing kimliği
değiştirilmez; `ImageFrame.hpp/.cpp`, `ImageAccess.hpp/.cpp` bu ayrımı uygular.

## 3. Kalibrasyon ve sampling API'si — EFR-03a-4

`MeasurementExecutionContext.rawSize` edinim kökü boyutudur; asla crop boyutu
olarak yeniden yazılmaz. `CalibrationDependency.source` kök koordinatıdır.
`SystemCalibration` ve legacy resolver gerçek acquisition kaynağında resolve
edilir; `ExecutionService` context'i source ID ile saklar ve Image soyuna göre
seçer. Son çalışmış başka kaynağın calibration context'i child'a taşınmaz.
Kaynak, gerçek kök çözünürlüğü, lens, düzlem, model/version/content, acquisition
izni, file-test/production ayrımı ve current kapıları aynen korunur.

Yeni `src/application/CalibratedImageGeometry.hpp/.cpp` ortak uygulama sınırı:

```cpp
class CalibratedImageGeometry {
public:
    static CalibratedImageGeometry bind(const ImageFrame&,
        const MeasurementExecutionContext&,
        const std::shared_ptr<std::atomic_bool>& cancel={});
    QPointF imageToWorld(QPointF imagePixel) const;
    QPointF worldToImage(QPointF worldMillimetres) const;
    void requireImage(const ImageFrame&) const;
};
```

`bind` immutable image/calibration/context snapshot'larını tutar, root frame ve
rawSize eşleşmesini doğrular. İleri yol `geometry.toRoot` → mevcut uniform
scale/axis veya `PlaneMappingService::map`; ters yol inverse scale/axis veya
`inverseMap` → `geometry.fromRoot`. Homography her noktaya uygulanır; mm/px
ötelemesi türetilmez. Her iki yönde gerçek root domain ve calibration hull
kontrolü vardır; `requireImage` content/processing/geometri/source/frame/cycle
ve tam timestamp kökenini denetler. İptal her mapping/tap'te gözlenir.

`CircleToolService::mapPoint` mevcut bool arabirimini uyumluluk wrapper'ı
olarak bu servise delege eder; `toRaw=true` mevcut consumer'lar için **girdi
rasterına** döner. Yeni kod açık `worldToImage` adını kullanır. Metrology,
GeometryMeasurement, EdgePoint mm adapter ve RadialScanService aynı kapıyı
kullanır; kopyalanmış `image.size()==rawSize` kontrolleri kaldırılıp root
kontrolüyle değiştirilir. MeasurementService'in raw matematiği korunabilir:
adapter local Point/PointSet'i root'a açıkça çevirir, root origin ve rawSize
ile çağırır; sonuç local px origin'iyle yayınlanır. Bir frame etiketi değiştirmek
tek başına koordinat dönüşümü sayılmaz.

Point-only `measurement` ve `calculate` girdilerinde ImageFrame bulunmaz.
Bu nedenle EFR-03a-3 `ExecutionService.hpp` içindeki `PortValue`'a optional
`ImageGeometry imageGeometry` kökenini ekler; Image çıktısında frame ile eşit,
Region çıktısında ROI girdisinin geometrisidir. EFR-03a-4 bütün child px
çıktılarına (scalar/Point/typed geometry/CodeResults dahil) bunu taşır ve
ToolResultCodec/ProductionResultCodec'de sürümlü bounded scope'u kodlar.
Child coordinate ID'li fakat scope'u olmayan kayıt reddedilir. Kök portlarda
alanın yokluğu mevcut raw anlamını korur. Measurement adapter iki portun
scope/source/frame/cycle uyumunu doğrulayıp bu dönüşümle root'a taşır;
calculate local px mesafesini verir ve aynı scope'u sonuca taşır. Böylece
Image almayan bu iki downstream araç da crop koordinatını kaybetmez.

`WorldTrackingMap`'e `nominalToImage`/`imageToWorld` adları eklenir (eski
`nominalToRaw`/`rawToWorld` kullanımları aynı kartta taşınır). Dünya tracking
delta'sı root pixel frame'de hesaplanır: taught/current crop anchor açıkça
root'a taşınır, rigid transform orada uygulanır; ters sampling son adımda
child'a döner. Ham referans binding'i kendi child frame'inde kalır. Root ve
child reference'ı doğrudan karıştırmak hata; yeniden öğretim veya açık adapter
gerekir. `ReferenceBinding`/`WorldPoseTracking` rawSize alanının tarihsel
anlamını sessiz değiştirmek yerine yeni crop teaching scope'u ayrıca kaydet.
Kök binding'in mevcut JSON/no-op biçimini koru; child için sürümlü optional
image geometry scope (rootSize/root ID, rasterSize/local ID, geometry identity)
ve strict codec ekle. Crop scope'u olmayan tarihsel teaching child'a otomatik
taşınmaz. Bu genişletme `ReferenceBinding`, `PoseRoi`, `PoseTrackingConfig`,
`ReferenceService`, `PoseRoiService` testleriyle atomik yapılır.

`ProfileSampler::sampleMapped` callback'i **verilen ImageFrame'in yerel
pixel-centre koordinatını** döndürür; ProfileRawMap adı ImageSampleMap olarak
netleştirilir. WorldTrackingService/RadialScanService/EdgePairService mapping
adaptörleri root pikseline erişmez. Ön köşe kontrolüne ek her boyuna/enine tap
kalibrasyon hull'u ve crop sınırını geçer. Bilinear örneklemede kullanılan
pozitif ağırlıklı komşu pixel centre'ları da root hull'da olmalı; child dışı
komşuya kök tamponundan erişim yoktur. Sınırdaki son merkez mevcut clamp
convention'ını korur, dış footprint clip edilmez. Gaussian desteği/sample/tap
bütçeleri korunur. Geometri ölçümünün kalibre line/circle fit'i crop gözlemlerini
tek tek dünyaya taşır; homography altında yerel circle'ı world circle diye
etiketlemez, mevcut yeniden fit kuralını sürdürür.

## 4. Registry Image tüketici matrisi

`ToolRegistry::entries/builtinInputs` + `geometryMeasurementInputs` ve
`supportsPoseTracking` kaynak incelemesiyle **13** yerleşik tür aşağıdadır.
21 yerleşik türün kalan 8'i Image tüketmez: image_file/webcam/industrial_camera
kaynaktır; calculate/measurement nokta alır; set_variables/result/publish_data
typed bağımlılık alır. Gelecekte kayıtlı extension Image alıyorsa aynı sözleşme
zorunludur; bu envanter gelecekteki plugin'ler için test kanıtı değildir.

Her satır için EFR-03a-4 aynı fixture'ı crop kapalı/açık, doğrudan kaynak /
preprocess / nested ROI kökeniyle çalıştırır. Geçerli kendi local ROI/öğretimi
kurulur; piksel sonucu crop alanına ait gerçek oracle ile karşılaştırılır.
`R`: ham referans, `W`: world referans/poseTracking, `—`: mevcut yetenek yok;
crop nedeniyle yeni yasak konmaz. Her satırda frame/cycle/source/coordinate
kökeni ve eski sonuç sızıntısı da doğrulanır. Aşağıdaki test adları mevcut CTest
hedefleridir; **crop kabulünün bugün geçtiği anlamına gelmez**.

| Kind | Uygulama dosyası (src/ altında) | Ham / kalibre / referans | Genişletilecek test dosyası → CTest |
| --- | --- | --- | --- |
| roi | application/RoiService.hpp | px; kalibre çıktı yok; — | tests/roi_test.cpp → roi_contract; nested crop/binding/mode |
| preprocess | application/ToolExecutors.cpp, vision/Preprocessing.cpp | px; child geometri korunur; — | tests/preprocessing_test.cpp → preprocessing_contract; crop→preprocess→crop |
| template_match | application/ToolExecutors.cpp, application/MatchService.cpp | px; mm yok; kaynak olabilir, consumer reference yok | tests/execution_test.cpp → tool_execution; tests/roi_crop_consumers_test.cpp → roi_crop_consumers_contract (yeni) |
| blob | application/ToolExecutors.cpp | px; mm yok; R + raw poseTracking | tests/blob_test.cpp → blob_contract; tests/reference_inspection_test.cpp → reference_inspection_contract; tests/tracking_inspection_test.cpp → tracking_inspection_contract |
| edge | application/ToolExecutors.cpp | px; mm downstream measurement; R, legacy Point anchor | tests/reference_execution_test.cpp → reference_execution_contract; tests/measurement_dag_test.cpp → measurement_dag |
| code_read | application/CodeReadService.cpp, vision/codes/CodeReader.cpp | px; mm yok; R + raw poseTracking | tests/code_tool_test.cpp → code_tool_contract; reference_inspection_contract, tracking_inspection_contract |
| pattern_pose | application/PoseMatchService.cpp, vision/matching/PoseMatcher.cpp | Gray8 px pose; mm yok; reference kaynağı | tests/pose_match_test.cpp → pose_match_contract; tests/tracking_tools_test.cpp → tracking_tools_contract |
| shape_pose | application/ShapeMatchService.cpp, application/PoseMatchService.cpp, vision/matching/ShapeMatcher.cpp | Gray8 px pose; mm yok; reference kaynağı | tests/shape_match_test.cpp → shape_match_contract; tracking_tools_contract |
| edge_point | application/EdgePointService.cpp, application/ToolExecutors.cpp | px + gerçek pointMm; R/W | tests/edge_point_tool_test.cpp → edge_point_tool_contract; tests/reference_world_test.cpp → reference_world_contract; tests/world_tracking_dag_test.cpp → world_tracking_dag_contract |
| edge_pair | application/MetrologyService.cpp, application/EdgePairService.cpp | pixel/calibrated; R/W | tests/edge_pair_test.cpp → edge_pair_contract; tests/metrology_runtime_test.cpp → metrology_runtime_contract; reference_world_contract |
| line_fit | application/MetrologyService.cpp | pixel/calibrated; R/W | tests/line_fit_test.cpp → line_fit_contract; world_tracking_dag_contract; metrology_runtime_contract |
| circle_fit | application/CircleToolService.cpp, application/RadialScanService.cpp | pixel/calibrated; R/W | tests/calibrated_circle_test.cpp → calibrated_circle_contract; tests/radial_scan_test.cpp → radial_scan_contract; tests/circle_metrics_test.cpp → circle_metrics_contract; reference_world_contract |
| geometry_measurement | application/GeometryMeasurementService.cpp, domain/GeometryMeasurementConfig.cpp | pixel/calibrated; typed geometriler, reference parametresi yok | tests/geometry_measurement_service_test.cpp → geometry_measurement_service_contract; crop Point/Line/Circle/PointSet kökeni ve projective refit |

Ortak yeni `tests/roi_crop_consumers_test.cpp` (EFR-03a-4) registry'den Image
input'u olan kind setini çıkarıp bu 13 satırla eşitler; yeni consumer eklenince
test açıkça başarısız olur. Her kind için enabled bir crop→consumer DAG koşar;
normal execution/gerçek sonuç veya gerçek not-found oracle'ı sınanır, yalnız
graph kabulü yeterli değildir. ZXing ON koşumunda gerçek code fixture gerekir;
OFF backend-unavailable kabulü Code Read crop desteğinin kanıtı sayılmaz.
Pattern/Shape için mevcut Gray8/model minimumları; tüm consumer'larda mevcut
format/boyut/fit/timeout/cancel sınırları korunur. Raw-only araçlara sahte mm
veya yeni world reference özelliği eklenmez.

## 5. Ortak entegrasyon ve kapanış kapıları

| Kart | Sorumlu yollar ve zorunlu test senaryoları |
| --- | --- |
| 03a-2 | ImageGeometry, ImageFrame, ImageAccess; yeni tests/image_crop_test.cpp → image_crop_contract, tests/roi_geometry_test.cpp → roi_geometry_contract, tests/frame_test.cpp → owned_frame_contract. Altı giriş formatı, padded stride, bytes/ownership, 1 pixel/kenar/tam/nested, overflow/32 adım, aynı piksel-farklı rect, kararlı ID/yeni frame, preprocess öncesi/sonrası. |
| 03a-3 | RoiService, ToolRegistry, ToolDefinition, ToolGraph, application/ExecutionService.cpp ve EditorSession.cpp, application/ProductionRecipeService.cpp; roi_contract, tests/tool_migration_test.cpp → tool_migration_contract, tests/tool_values_test.cpp → tool_values_contract, tests/target_only_test.cpp → editor_target_only, tests/session_test.cpp → session_contract, tests/production_recipe_test.cpp → production_recipe_contract. Eski iki alan fixture save/reopen, false/true/eksik/hatalı tip, UUID/bağlar, aynı ROI Region+child Image ret, downstream topology/signature/budget. |
| 03a-4 | Bölüm 3/4 bütün consumer ve adapter'ları, application/SystemCalibration.cpp, MeasurementService.cpp, PlaneMappingService.cpp; yeni tests/roi_crop_calibration_test.cpp → roi_crop_calibration_contract ve tests/profile_sampler_test.cpp → profile_sampler_contract, tests/plane_mapping_test.cpp → plane_mapping_contract, tests/system_calibration_test.cpp → system_calibration_contract. Uniform/homography ileri/ters/subpixel, hull içinde crop dışında, crop içinde hull dışında, bilinear enine tap/hull, stale calibration ve iki kaynak. |
| 03a-4 kanıt/Runtime | application/ToolResultCodec.cpp, ProductionResultCodec.cpp, ProductionRecipeService.cpp, RuntimeService.cpp, MultiSourceRuntimeService.cpp, RuntimeOverlay.cpp, ToolOverlayGeometry.hpp; tests/typed_trace_test.cpp → typed_terminal_trace, tests/multi_source_terminal_test.cpp → multi_source_terminal_trace, tests/runtime_test.cpp → runtime_persistence, tests/multi_source_execution_test.cpp → multi_source_execution. Immutable snapshot'ta rect/mod/kalibrasyon edit izolasyonu; restart trace yeni crop kökeniyle replay edilir, eski trace schema'sı okunur, replay yeni canlı raster/üretim izni üretmez. |
| 03b UI | app/RoiEditor.cpp, MainWindowEditor.cpp, MainWindowRuntime.cpp, app/ImageViewer.cpp ve application/RuntimeOverlay.cpp; gerçek raster seçimi, local overlay ve crop'ta çizim. 03a-4 core overlay frame seçimini hazırlar; checkbox/Properties/normal-küçük ekran kabulü yalnız 03b'de. |

**Invalidation:** upstream parametre/model/source/boyut/geometri edit'i tüm
downstream sonuçları siler. Geometriye bağlı teaching ayrıca geçersizleşir;
A→B→A eski teaching iznini yeniden canlandırmaz. EditorSession invalidation
nesli/tombstone'u yalnız hash eşitliğiyle silinmez; kalıcı teaching scope'u
yeniden öğretme komutuyla yenilenir. Mod/rect/source edit transaction'ı gerekli
binding'i stale bırakır veya temizler; Undo/eski rect'e dönüş çalıştırma izni
vermez. Böylece geometri ID'si deterministik kalırken uygunluk monoton
geçersizleşir. Girdisi değişmeyen ilgisiz dalların no-op/cache davranışı korunur.
03a-3 ROI downstream kapsamını; 03a-4 bütün model/reference öğretim yollarını
bağlar. Stale scope ret nedeni açıktır; yeni bir crop unsupported kapısı yoktur.

ToolResultCodec'de Image trace'i boyut/format/processing ID yanında bounded
geometri özetini taşır (root/local ID ve boyut, offset, identity/depth).
Kalıcı image bytes veya parent raster eklenmez. Yeni child trace alanları
sürümlü/strict okunur; eski kök trace biçimi/no-op korunur. Px portlar local
origin ve typed frame ile, mm portlar world typed frame ve acquisition source
ile doğrulanır. ProductionResultCodec'in fiziksel kaynağın raw çözünürlüğü
kontrolü yalnız acquisition çıktısına uygulanır; child imageInfo'ya yeni kamera
çözünürlüğü muamelesi yapılmaz. RuntimeOverlay/ToolOverlayGeometry hangi
rastera ait olduğu belli geometri üretir; ham viewer'a child sonucu yanlış
koordinatla çizilmez. ROI'nin parent Region ve child Image çıktıları için tüm
ToolResult'a tek coordinate ID dayatılmaz: overlay port/layer scope'una göre
seçilir; her port kendi kökeniyle doğrulanır. Cache'te root frame ile child coordinate ID'nin eşit
olmasını zorlayan ExecutionService target-only kontrolü soy doğrulamasına
dönüşür; Image/Region/reference girişlerinin birbirleriyle yerel eşitlik
kontrolü korunur.

03a-2/3/4 kendi Release build ve ilgili CTest'lerini gerçekten çalıştırır;
03a-4 matrisi tamamlamadan 03b UI kabulüne geçilmez. Fiziksel mm doğruluğu,
native kamera izni, CUDA/kurulum veya robot hareketi bu yazılım kapılarıyla
doğrulanmış sayılmaz.

## EFR-03a-4 uygulama ve EFR-03b API devri

`CalibratedImageGeometry::bind` rootSize/rawSize, acquisition source/root frame,
current calibration ve zaman kökenini doğrular. `imageToWorld`/`worldToImage`
local raster kullanır; `rootToWorld`/`worldToRoot` yalnız koordinat matematiği
sağlar, root piksellerine erişmez. `requireSamplingFootprint` gerçek bilinear
floor/ceil komşularını child domain ve calibration hull içinde denetler.
Uniform ve projective yollar aynı kapıları kullanır. Pixel-debug araçların
kalibrasyon önkoşulu yoktur; destekli kalibre modlar crop nedeniyle reddedilmez.

`ImageGeometry::toJson/fromJson` scope schema 1'i strict/bounded okur. En fazla
32 crop rect'i root'tan replay edilip local/root boyut, offset, depth ve SHA-256
identity bütünü karşılaştırılır; yalnız kimliği tutup offset değiştirmek
geçerli değildir. Bu codec scope üretir; ImageFrame'e dışarıdan piksel/geometri
takma API'si açmaz. Image trace `imageInfo.imageGeometry`; child non-Image
portları envelope schema 2 kullanır. Eski root envelope schema 1/no-op biçimi
korunur. Child ID scope olmadan, yanlış kaynak/rect/chain ile okunamaz.

Raw ReferenceBinding/PoseRoi eski `rawSize` alanını local teaching raster boyutu
olarak korur ve ayrıca optional `imageGeometry` kaydeder. World teaching'de
`rawSize` edinim root boyutu, taught anchor root koordinatıdır; current child
anchor açıkça root'a taşınır, rigid delta root'ta uygulanır ve sampling child'a
döner. Aynı `WorldTrackingMap` içindeki `nominalToImage`/`imageToWorld` yeni
adları application kodunda kullanılır. Eski UI caller'ları için salt yönlendiren
`nominalToRaw(p)`/`rawToWorld(p)` yöntemleri korunur; EFR-03b bunları taşıyabilir.

Project edit upstream scope'u değiştirdiğinde ROI imageBinding boşalır, model
PNG silinir; reference/pose kaydı optional `invalidated:true` tombstone taşır.
Tombstone strict codec ile kalır ve execution yeniden öğretim ister. Böylece
world ROI/bağlantı grafiği okunur kalır, A→B→A veya reopen eski izni canlandırmaz.
İlk bağlantının kurulması yeni öğretilmiş binding'i silmez; mevcut input bağının
kaynak/port değişimi ve upstream semantik edit invalidate eder.

Core overlay gerçek input raster metadata'sıyla üretilir. RuntimeOverlay seçilen
frame'in local coordinate ID'siyle eşleşmeyen consumer sonuçlarını göstermez;
ROI'nin parent Region/child Image portlarına tek ID dayatmaz. EFR-03b viewer'da
actual child ImageFrame seçimi, checkbox ve `RoiService::teach(..., cropImage)`
tercihini koruma işini tamamlar. Ortak engine sürümü `/36`'dır. Core test/kanıt
ve kapsam ayrıntısı [EFR-03a-4 kapanışındadır](../archive/2026-10-29-efr-03a-4.md).

## Kaynak incelemesi ve sınır

Tier 2 graph başlangıcı `D:/TVision`, `D-TVision`, generation
`2026-10-28T19:57:00Z`, git watch açık. `builtinInputs`/`processed` keşfi,
`processed` ve `calibratedMap` inbound/outbound trace ve source snippet'leri
kullanıldı. Graph bazı C++ çağrılarını yanlış eşleyebiliyor (örneğin
`processed` içindeki append'i DeliveryStore'a); bu tasarım çağrı hedefini
doğrudan kaynakla doğrular. Registry envanteri sonlu entries listesi + port
sağlayıcıları kaynak okumalarıyla çıkarıldı; geniş regex'in boş sonucu yokluk
kanıtı yapılmadı. Dayanılan yollar için coverage ve parse_partial satırları
doğrudan kaynakla tamamlandı; ayrıntılı kanıt `build/efr/efr-03a-1/` altında.
Bu belge gelecekteki implementasyonun test başarısı değildir.

## EFR-03b — uygulanmış UI sınırı

Properties `cropImage` reçete transaction'ına bağlıdır; mod/çizim ve mevcut
bağlantılar korunur. Güncel ROI açıkken child raster, kapalıyken input raster +
Region gösterilir. Girişte çiz/düzenle eylemi parent rasterı seçer; tüketicinin
çizimi kendi Image girdisindedir. Viewer sunum kimliği local coordinate ID
barındırır; aynı boyut/farklı konum eski tutamaç/overlay'i taşımaz. Piksel sorgusu
da gösterilen gerçek rasterı kullanır. Geometri etiketleri görüntü px der;
child raster edinim raw kökü gibi adlandırılmaz.

Runtime seçili araçta actual output/input ImageFrame'i, tüm araçlar görünümünde
edinim root'unu gösterir. Non-Image port scope'u source/frame/cycle/root ve local
kimliğiyle doğrulanır; rootOffset farkı point/region/line/scan/mask/code layer'ına
bir kez uygulanır. ROI parent Region'ı child çıktı üstüne yanlış çizilmez. Mm
çıktıların typed birimi korunur; overlay yine kanıtlanmış raster koordinatıdır.
Stop downstream cache'i ve sunumu temizler; eski crop Image yeniden kullanılamaz.
Reference/pose tombstone'u Properties'de yeniden öğretim uyarısıdır. Metrologi
sayısal/drawing hazır kapısı güncel Image girdisi ve busy durumuyla yenilenir;
kalibrasyon edit'inden sonra güncel kaynak çıktısı edinilmeden öğretim yapılmaz.

Yeni yazılım/görsel kanıt, korunan Save odak hatası ve sınırlar
[EFR-03b kapanışında](../archive/2026-10-29-efr-03b.md) kayıtlıdır. EFR-04 birleşik
kabul bu teslimde başlatılmadı.

## EFR-04 — birleşik kabul sözleşmesi

Üç applicationType için Run Flow, güncel child cache'inden Run Tool ve
Save/reopen sonrası Runtime aynı koordinat anlamını taşır. Runtime rect/mod
taslak edit'ini izlemez. `editor_flow_roi_acceptance` ortak senaryoyu iki mod,
nested crop ve ham dal ile sınar; terminal Image geometry ve typed port codec'i
raw/local ID, boyut, rect zinciri, dönüşüm, frame/cycle/evaluation kökenini
korur. Yeni raster başka cycle'a aitse tarihsel overlay için kullanılamaz.
Logs ve ortak terminal özeti rasterin mevcut olmadığını açıkça söyler.
Eski root kayıtlarına crop scope eklenmez; replay piksel arşivleme, yeniden
üretme, algoritma çalıştırma veya aktif snapshot oluşturma değildir.
Yeni koşumlar ve açık regresyonlar [EFR-04 kapanışında](../archive/2026-10-29-efr-04.md) tutulur.
