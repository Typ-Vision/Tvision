# KMC-10a — Çok kaynaklı DAG ve frame sözleşmesi

17 Ekim KMC-10c2/10d güncellemesi: [kalıcı Runtime, terminal7 ve N-kamera UI](multi-source-runtime-terminal.md) uygulandı. Aşağıdaki kapalı production/yalnız bellek ifadeleri bu belgenin önceki paket kapanış durumudur; Runtime editor9 bütün-set provider ile açılır. [Birleşik kabul/devir](multi-source-release-acceptance.md).

16 Ekim 2026. Programatik sözleşme teslimidir. Çok-source execution/Editor
KMC-10b, immutable production ve terminal kabulü KMC-10c1/c2 kapsamındadır.
KMC-10b ile [çok kaynaklı Editor/execution](multi-source-execution-editor.md) açıldı; aşağıdaki KMC-10a kapanışındaki execution reddi tarihsel durumdur. Production kapısı KMC-10c2 kabulüne kadar kapalıdır.

## Opt-in veri modeli

Reçetenin `definition.editor.schemaVersion = 9` değeri çok-source DAG opt-in'idir.
Root project1 ve recipe1–2 değişmez: bu genişleme definition içindedir; ikinci bir
root/recipe migration gerekmez. `withMultiSourceSchema` açık geçiş komutudur;
araç kimliklerini, parametrelerini, bağımlılıklarını ve bağlantılarını korur.
Geçiş idempotenttir. Normal açma/Save veya ikinci kaynak ekleme eski şemayı
otomatik yükseltmez. Schema1–8 tek edinim kaynağı sınırını korur; schema9
mevcut 256 tool bütçesi içinde N kaynak tanımlar. 10 kamera bir ürün üst sınırı
değildir. Schema10, fractional schema ve bilinmeyen editor alanları reddedilir.
`requiredEditorSchema` schema9'u reference/world özellikleri nedeniyle 8'e düşürmez.

Kaynak kimliği edinim tool UUID'sidir. Görünen ad, fiziksel CameraId, SetupId,
dosya yolu ve raster hash'i bu kimliğin yerine geçmez. Mevcut `sourceSetup`
şema1 tek kaynak bağını korur; bu paket N fiziksel kurulum/model snapshot'ı
veya yeni scoped kalibrasyon edinimi uygulamaz.

## DAG kökeni

`validateSourceGraph` topolojik sırada her tool'un upstream edinim kaynağını çözer:

- `requiredSources`: enabled edinim araçları. Tam akış bütün enabled kaynakları
  gerektirir; hedefe özel daraltma KMC-10b'nin işidir.
- `toolSources[toolId]`: dalın kaynak tool ID'si.
- `portSources[toolId][portId]`: her çıkışın aynı kaynak kökeni; Preprocess kimliği
  değiştirmez. Eksik veya çatışan köken çözülmüş gibi sunulmaz.
- Bir tool'a birden fazla kaynaktan giriş bağlanamaz. Image/region/point,
  reference/pose, typed line/circle/observations ve scalar bağlantılar aynı kurala
  tabidir. Aynı world plane adı kameralar arası dönüşüm yetkisi değildir.
- Disabled düğümler de köken çözümlemesine katılır; enabled tüketicinin disabled
  upstream'i reddedilir. Legacy `validateGraph` disabled → SKIPPED yürütmesini korur. Silinen tool/port ve duplicate ID mevcut DAG kapılarından
  reddedilir. Explicit aynı-kaynak rebind yeni dal kökenini çözer.

ToolDefinition codec ayrıca canonical UUID ve case-folded ad çakışmasını reddeder.
GraphValidation alanları yalnız `valid()` başarılıysa kullanılmalıdır. Bu sözleşme
ortak robot/world koordinatı veya kamera senkronizasyonu sağlamaz.

## Source-frame ve ToolValues

`SourceFrameIndex` en çok 256 required kaynak için tek cycle içinde tam, değişmez
bir kimlik indeksi kurar. Her kaynak tam bir kez bulunmalı; foreign/missing/duplicate
source veya duplicate frame reddedilir. Her frame ayrı `frameId`, ortak `cycleId`
ve `<sourceToolId>/image` koordinat kimliğini taşır. Eşit pikseller frame paylaşımı
anlamına gelmez. Bu indeks görüntü byte'larını saklamaz; bellek/deadline bütçesi ve
başarısız/kısmi edinim yürütmesi KMC-10b kapsamındadır.

`validatePort` çağrısına GraphValidation'ın çözdüğü expected source ve gerçek port
bildirimi verilmelidir. PortValue source/coordinate/frame/cycle kimlikleri indekse
eşit olmalı; ToolResultCodec ayrıca union/type/unit, ImageFrame iç kökeni ve typed
raw geometry koordinatlarını denetler. ToolValues payload/codec sürümü değişmez;
payload tek başına edinim kökeni veya ortak kamera kanıtı değildir. World modelinin
exact dependency ve alan doğrulaması ilgili calibration resolver'ın sorumluluğudur.

`legacyFrameId()` yalnız tek kaynakta frame ID döndürür; N>1'de boştur.
Çok-source CycleResult bu kurala uyacak; `frames`/indeks esas alınacak ve bir
kamera karesi bütün çevrime sahte eşlenmeyecektir. Mevcut execution/24 tek-source
CycleResult davranışı korunur; schema9 execution edinimden önce açık ERROR verir.
ProductionRecipeService schema9'u, tek kaynağa düşürülmüş olsa da, KMC-10c2 kabulüne
kadar reddeder. Terminal6/eski okuyucular/vf.socket2/trusted completion değişmez.

## Kabul

`multi_source_graph_contract`: N=1/2/8/10, mixed file/webcam tanımları, bağımsız
Preprocess dalları, her port kökeni, rename/duplicate ID/ad, disable/delete/rebind,
point/region/reference/typed geometry çapraz bağları, proje save/reopen ve üç
applicationType, legacy no-op bytes/explicit migration/future schema, tam frame
indeksi/missing/duplicate/foreign/source-frame-coordinate-cycle swap, eşit raster
kökeni ve kapalı execution/production kapıları. Fixture edinim yapmaz; fiziksel
webcam/industrial kamera veya N-kamera üretim hızı kabulü değildir.

Release build/CTest sonuçları ve sonraki paket [progress](../progress.md) içinde
kaydedilir. Yeni kullanıcı ekranı yoktur; mevcut tek-source uygulama
`build/reference-ui/visionflow.exe` üzerinden test edilir.
