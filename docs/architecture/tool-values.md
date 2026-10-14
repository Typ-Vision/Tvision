# T0.3 / T1.1 / T1.2 uygulanan çekirdek sözleşmesi

9 Ekim güncel durum: **T2.3 / T3.1 / T3.2 tamamlandı**. Preprocess tam kare Gray8/kanal ve sıralı Gaussian/median/eşik/adaptif eşik/morphology, owned çıktı ve Editor/Runtime önizleme sunar. Code Read pinned ZXing adapter/ROI/kod listesi ve içerik/adet kontrolü, typed terminal/Logs/CSV ve cache desteği servis katmanındadır; Toolbox/format/içerik/overlay etkileşimi **T3.3** adımına bırakıldı. Dokuz registry türü, sekiz Toolbox aracı; genel karar değişmedi. Editor 7 opt-in, **execution/8**, terminal 3/vf.socket/2 korunur. [Uygulanan sözleşme ve sınırlar](preprocessing-codes.md). Aşağıdaki eski sonraki-adım ve yedi-tool kayıtları tarihseldir.

9 Ekim 2026. Yeni kullanıcı tool'u eklenmedi; yedi built-in korunur. Code reader T3, ailelere özel UI/algoritmalar sonraki paketlerdedir. Execution kimliği `visionflow.execution/7`; eski validation izni kullanılmaz. Root 1, recipe 1–2, calibration record/dependency 1/2/3 ve vf.socket/2 değişmez.

## Backend kapısı

`VISIONFLOW_WITH_ZXING` varsayılan OFF. ON seçilirse mutlak yerel `VISIONFLOW_ZXING_PREFIX` gerekir. `VisionDependencies.cmake` yalnız bu prefix içindeki exact ZXing 2.3.0 static export'u kabul eder; eski ZXing_DIR cache'i seçimi değiştiremez. Configure indirme yapmaz. Reader açık/writer kapalı ve x64 compile assertion'ları vardır. `BackendCapabilities::get("zxing-reader")` linked reader ile blank decode çağrısını bir kez yapar; available/version/statik archive SHA-256 döndürür. OFF/unknown/Tesseract unavailable döndürür. Bu kimlik o build'in binary kimliğidir; kaynak commit/hash/lisans [manifestte](../../third_party/manifest.json), hazırlama [toolchain'de](../toolchain.md).

Tool descriptor `requiredBackend` bildirir; çalıştırma/üretim doğrulaması tam version/contentId bağımlılığı olmadan o tool'u etkinleştirmez. Registry'de boş code/OCR tool'u yoktur. OFF build eski yedi tool'u çalıştırır. Capability bir decode corpus'u, kurulum veya dağıtım kabulü değildir.

## Tipli değerler

`ToolValues` domain tipleri OpenCV içermez. PortData closed variant ile genişletilir; keyfi JSON portu yoktur. Codec schema 1, exact alanlar, sonlu sayılar, geçerli Unicode (unpaired surrogate reddi), canonical base64, enum ve bounded array doğrular. Text/raw binary bağımsızdır; normalize edilmez, aynı metinli kodlar birleştirilmez. ZXing confidence alanı yoktur.

| Port | Payload / birim |
| --- | --- |
| Text | QString, birimsiz |
| CodeResults | text, bytes, sekiz sabit symbology adı, dört raw px köşe; px/image |
| PointSet | Noktalar + CoordinateFrame; px/image veya mm/plane |
| Profile | raw konum, value, signed derivative; gray8/image |
| Line2D | Destek başlangıç/bitişi + frame; px veya mm |
| Circle2D | Merkez, pozitif yarıçap + frame; px veya mm |
| Arc2D | Circle + start/sweep derece; 0 < abs(sweep) ≤ 360 |
| Pose2D | Mevcut frame/konum/radyan sözleşmesi; px veya mm |
| InspectionResults | Benzersiz ad, accepted bool, reason; birimsiz; genel parça kararı değildir |

Sınırlar: 8192 nokta/profil örneği; 128 kod/inspection; text ve binary alanı 65536 karakter/bayt; payload compact JSON 2 MiB; port envelope buna 4096 bayt ekler. Dizi kesilmez; sınır aşımı hata verir. Geçerli boş koleksiyon ile invalid + reason ayrıdır. Görüntü portu JSON'dan yeniden kurulmaz; owned frame store'da kalır.

`PortValue` source/frame/cycle/koordinat, evaluationId, method/version, diagnostic ve invalidReason taşır. Yeni valid tip method/version gerektirir; engine actual evaluation ID'yi damgalar, çelişen kimliği reddeder. Registry'nin sabit type/unit/frame tanımıyla union karşılaştırılır. Px geometrinin frame ID'si raw origin ile eşleşir; mm geometriler plane frame taşır. Bir tool'un geometrik girdilerinin frame/birim/eksenleri eşleşir. Legacy scalar provenance ve Measurement davranışı korunur.

Engine hatalı output'u üreticide reddeder ve temizler; downstream SKIPPED kalır. Production completion undeclared port, yanlış union/unit/frame/cycle/evaluation ve diagnostic'ten parça kararı üretimini reddeder. Yeni tiplerin payload ve invalidReason'u kayıt adapter'ında korunur; hiçbir varyant sessiz düşmez. Results/Runtime exhaustive display visitor kullanır; aileye özel sunum T1.3 ve ilgili tool paketlerinde geliştirilir. Genel PASS T10'a aittir. Üretim whitelist'i mevcut yedi araçtır; test extension'ı toolbox/Runtime ürünü değildir.

## Schema 7 ve bağımlılıklar

Schema 1–6 read normalization önceki davranışı korur. Sıradan legacy edit schema 6 yazar. İlk dependency düzenlemesi veya `minimumEditorSchema=7` tipli descriptor kullanımı schema 7'yi seçer. Schema 7'de her tool'un `dependencies` dizisi zorunludur (boş olabilir); schema 6'ya sessiz downgrade yapılmaz. Aç/no-op Save aynı bytes/hash/revision'ı korur. Schema 8+, extra alan veya kapalı tool'da bilinmeyen yapılandırma reddedilir. ProjectRepository mevcut domain doğrulama yolunu kullanır; ikinci migration sistemi yoktur.

Her dependency exact `{kind,id,version,contentId,path}` kaydıdır: kind backend/model/asset, version boş olmayan sürümlü kimlik, contentId lowercase SHA-256. En çok 32/tool, kind+id benzersiz. Backend path boştur; model/asset canonical project-relative dosya yoludur. `..`, drive/absolute/backslash ve proje dışına çözülen symlink yolu reddedilir. Dosya sınırı 64 MiB; streaming SHA-256 gerçek dosyadan hesaplanır. Version model formatının beyanıdır; henüz model ailesi parser'ı değildir.

`ToolDependencyResolver` backend availability/exact binary hash ve dosya içerik hash'ini Run Tool/full execution/üretim validation/activate/next cycle sınırında kontrol eder. Snapshot dependency kimliği bu kayıtları kapsar. TargetOnly imzası bağımlılık kaydını ve unavailable durumunu içerir; değişmiş/eksik model eski cache sonucunu kullanamaz. Eksik dosya recipe açmayı engellemez; execution/üretim iznini engeller. Dosyalar projeye gömülmez/kopyalanmaz. Kontrol ile kullanım arasındaki dış dosya değişimine karşı OS lease/paketleme kabulü yoktur.

## Doğrulama ve sonraki paket

`tool_values_contract` roundtrip/tamper/Unicode/GS/binary/duplicate text, invalid/empty, bounds/NaN/Inf/dejenere geometri, fixed unit/frame/cycle ve aynı engine'de typed extension'ı sınar. `tool_migration_contract` byte/hash/revision no-op, schema 7 Save/reopen, model/asset gerçek hash, tamper/missing/backend ret, snapshot/TargetOnly invalidation ve disabled unknown config'i sınar. `backend_capabilities` ON/OFF build'de ayrı çalışır. Legacy completion sahteciliği testleri genişletildi. Gerçek komut/sonuçlar [progress](../progress.md) ve [toolchain](../toolchain.md) içindedir.

Sıradaki **T1.3 — Runtime/Logs/cache birleşik altyapı kabulü**: typed terminal replay/Logs/CSV ve ailelerin diagnostic/metric sunumu. Bu teslim T1.3'ü veya gerçek code/metrologi/OCR ürününü tamamlamaz. Lens T15, P7/6D.2 ve K4 açık kalır.

## T1.3 terminal replay

Yeni terminal cycle schema 3 bütün non-image portların exact envelope'unu kaydeder. ProductionResultCodec append/read sınırında 16 MiB toplam, 256 tool/128 port ve typed payload/cycle/evaluation/diagnostic tutarlılığı denetler. Legacy 1–2 scalar kayıtları okunur; unknown type/schema reddedilir. Logs özet + JSON ve bounded CSV sunar. Replay aktif izin veya genel karar oluşturmaz; görüntü bytes taşınmaz.

Görüntü `imageInfo` işlenmiş frame için optional `processingId` taşır; raw imageInfo ve pixel content hash semantiği korunur. [T2 görüntü/ROI sözleşmesi](roi-geometry.md).
