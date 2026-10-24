# Proje ve reçete kayıt sözleşmesi — Adım 2

EUI-02 (18 Ekim 2026): Blob `rejectBorderTouching` boolean, varsayılan false.
Eski editor1–9 kayıtlarında eksik alan yalnız okuma modelinde tamamlanır; no-op
Save eski bayt/hash ve şemayı korur. Gerçek düzenleme alanı açıkça yazar. Eski
okuyucular bilinmeyen parametreyi strict key kontrolüyle reddeder; sessizce yok saymaz.
Editor şeması değişmez; Blob descriptor4 ve engine `visionflow.execution/28`
yeni yürütme/cache/Runtime dependency kimliğidir. [Görev kartı](editor-refinement-plan.md).


12 Ekim güncel durum — **R1–R8 tamamlandı.** Yeni ortak Referans binding’leri immutable snapshot ve Runtime’ın süreç içi trusted completion kanıtıyla production yoluna bağlandı; tam kaynak/port/ofset/calibration kimliği terminal schema4, restart, Logs ve CSV’de korunur. Runtime Viewer aynı current resolver’ı kullanır; Stop/missing nominal fallback üretmez. Engine **execution/22**, editor8/raw1/world2/vf.socket2 korunur. Standalone Pattern/Shape ve eski poseTracking production kapısı T8.4.2b.3c corpus kabulüne kadar kapalıdır. **R8 tam Release kabulü: OFF/core 57/57, ON/UI 64/64. Sonraki T8.4.2b.3c** eksik corpus ve bütün production yollarının kapanışıdır. Güncel binary build/reference-ui/visionflow.exe. Alttaki teslim ve sıra kayıtları tarihseldir. [R7 sözleşmesi ve kabul eşlemesi](reference-production-acceptance.md). [R8 paket ve devir kanıtı](reference-release-acceptance.md).


11 Ekim R4/R5: raw ReferenceBinding schema1 byte biçimi korunur. Yeni world binding schema2 / world-reference-v1; nominal exact mm ToolRoi, calibration ve mmPerSample alanını taşır. Point taught frame calibration.source üzerinden çözülür; offset daima px/radyan kalır. editor8 opt-in, root/recipe/tool/terminal/socket biçimleri korunur; engine execution21. Eski codec ve no-op hash yolları değiştirilmez. [Ortak sözleşme](reference-integration.md). Önceki kayıtlar tarihseldir.

11 Ekim R2/R3: yeni `parameters.referenceBinding` R1 strict schema1 kaydını taşır; yalnız gerçek edit **editor schema8** seçer. Her tool schema7 gibi exact dependencies dizisini korur. Eski Edge reference/raw-world poseTracking codec ve no-op byte/hash yolları değişmedi; root1/recipe1–2/terminal3/socket2 korunur. Bind/clear + connection tek candidate projede doğrulanır. Current source geometri kimliği ve nominal ROI hash denetlenir; eşik düzenlemesi current cache'i değiştirir fakat teaching geometri kimliğini değiştirmez. Engine **execution20**, yeni production binding izni R7'ye kadar kapalı. [Kesin servis/kayıt sözleşmesi](reference-integration.md).

10 Ekim T5.3: `edge_pair` / `line_fit` v1 ve editor7 typed portları ortak yürütme/kayıt yolundadır. Engine **visionflow.execution/10**; root/recipe/editor7/terminal3/vf.socket2 codec’leri korunur. Calibrated sonuç kesin binding ve gerçek actual frame ister; pixel çıktılar diagnostic, mm invalid/null. `accepted` tool tolerans Boolean’ıdır; genel parça kararı değildir. Runtime terminal commit bu Boolean’ı immutable toleransla ve kalibrasyonu binding ile denetler. [Ayrıntılı sözleşme](metrology.md).

9 Ekim T4.2: [edge_point v1 sözleşmesi](edge-point.md) registry/executor ve production whitelist'e eklendi; raw ScanSegment ToolRoi schema 1 parametre, editor 7. Execution/9, terminal 3/vf.socket/2; legacy reçete no-op korunur. Gerçek found/point/gray8-per-px strength/refined method evidence; no-found point invalid/null ve downstream SKIPPED. Profil/aday listesi kalıcı port veya UI değildir; T4.3 beklenir.

9 Ekim güncel durum: **T2.3 / T3.1 / T3.2 tamamlandı**. Preprocess tam kare Gray8/kanal ve sıralı Gaussian/median/eşik/adaptif eşik/morphology, owned çıktı ve Editor/Runtime önizleme sunar. Code Read pinned ZXing adapter/ROI/kod listesi ve içerik/adet kontrolü, typed terminal/Logs/CSV ve cache desteği servis katmanındadır; Toolbox/format/içerik/overlay etkileşimi **T3.3** adımına bırakıldı. Dokuz registry türü, sekiz Toolbox aracı; genel karar değişmedi. Editor 7 opt-in, **execution/8**, terminal 3/vf.socket/2 korunur. [Uygulanan sözleşme ve sınırlar](preprocessing-codes.md). Aşağıdaki eski sonraki-adım ve yedi-tool kayıtları tarihseldir.

9 Ekim güncel teslim: **T1.3 / T2.1 / T2.2 tamamlandı**. Typed terminal cycle schema 3 replay/Logs CSV; owned görüntü processing provenance ve raw subpixel ROI geometri/çizim/sayısal edit. ROI çalışma alanı geçicidir, reçeteye kaydedilmez/algoritma çalıştırmaz; recipe binding yeni tool paketlerinde. Release OFF/UI 31/31, ON/core 28/28; normal/küçük ekran görselleri incelendi. [Uygulanan sözleşme](roi-geometry.md). Sonraki T2.3; aşağıdaki eski sıra ifadeleri tarihseldir.

9 Ekim T1.1/T1.2: legacy editor 6 yanında typed descriptor/dependency düzenlemesi **editor schema 7** seçer; her tool exact dependencies dizisi taşır. Aç/no-op Save aynı byte/hash/revision'ı korur; root 1/recipe 1–2 ve calibration kayıt 1/2/3 değişmez. Schema 8+ ve kapalı tool'da bilinmeyen alan reddedilir. [Güncel alanlar ve sınırlar](tool-values.md). Aşağıdaki schema 7+ reddi/writer 6 ifadeleri tarihsel kayıtlardır; terminal replay/Logs/CSV birleşik kabulü T1.3'tedir.

9 Ekim tool planı hedefi: [T1](recipe-tools-plan.md#t1--tipler-sürümleme-ve-sonuç-altyapısı) yeni tool/typed parametreler için editor schema 7 opt-in migration ve bounded result codec tasarlar. **Mevcut writer hâlâ editor 6**, root 1/recipe 1–2 ve kalibrasyon kayıt 1/2/3'tür; bu plan şema değişikliği uygulamaz. Legacy aç/no-op Save/hash korunacak; gerçek edit ve capability/backend/model kimlikleri sürümlenecek. Production kayıt sürümü ve wire v3 ayrı açık tasarımdır; lens T15'te ayrı yeni model/dependency olacaktır.

8 Ekim C1: Manuel matematik/codec `manual_planar_scale/1` için ayrı record/dependency schema 3 kullanır. Schema 3 dependency mevcut frame ve kurulum alanlarına modelType/modelVersion ekler; schema 2 hâlâ yalnız homography kabul eder. Root/recipe/editor şemaları değişmedi. Tam alan listesi [manuel kayıt sözleşmesinde](manual-scale-calibration-plan.md#c1-uygulanan-manuel-kayıt-sözleşmesi). C1–C5 matematik/codec, manuel katalog/recipe Save/reopen/Runtime/CSV ve ekran kabulü tamamlandı. Engine execution/6 ve Measurement tool 2 eski validation kimliğini kullanmaz; şemalar değişmedi.

Blob P5 migration: editor schema 6 yazar; schema 1–5 read normalization polarity=bright, connectivity=8, minArea=1, maxArea=null ekler. Aç/no-op Save diskteki eski schema ve recipeVersion’ı korur. Gerçek tool/bağlantı değişikliği schema 6’ya geçer; schema 7+ reddedilir. Root schema 1 ve recipe schema 1–2 değişmez. maxArea null sınırsızdır; sonlu alanlar pozitif int32 ve Max ≥ Min olmalıdır. Maskeler production JSON’a yazılmaz.

8 Ekim 2026 P5/P6: Blob min/max dahil alan filtresi (px²), açık/koyu nesne ve 4/8 komşuluk uygulanmıştır. Gerçek kabul edilen piksel maskesi delikleri koruyarak Editor ve Runtime’da kırmızı gösterilir; bu parça kararı değildir. Editor schema 6, Blob tool 2, execution/5; eski reçeteler temiz açılır ve ilk gerçek değişiklikte yükselir. Birleşik kabul: Release UI 23/23, Widgets OFF 21/21; normal/850×600 görseller incelendi. Sıradaki P7 (6D.2) kalıcı dedup/outbox/restart teslim kurtarma; K4 ertelenmiş, P8–P9 gelecektedir.

## A8 bellek içi üretim sözleşmesi ve sonraki persistence planı

[A8 ProductionRecipeService](production-recipes.md) taslak, servis kaynaklı doğrulama kaydı ve değişmez aktif snapshot'ı bellekte ayırır. Recipe içerik kimliği, model/varlık, kalibrasyon ve engine/tool sürüm bağımlılığı, policy kontrollü activation/rollback, cycle sınırı ve bounded transient geçmiş/trace uygulanmıştır. Dosyadaki VALID değeri tek başına izin değildir. Bu kayıtlar `.vfproject` içine yazılmaz; Runtime/Users/kalıcı geçmiş/log entegrasyonu ve fiziksel üretim kabulü 6C/7 kapsamındadır. Restart kayıtları tekrar güvenilir doğrulamadan geçirmelidir.

A9 [persistence-plan.md](persistence-plan.md) manifest/kimlikli varlık paketleme ve sıralı schema migration tasarımını belirler; atomik kayıt, dirty/revizyon tutarlılığı ve başarısız kayıtta eski verinin korunması devam edecektir. Eksik/bozuk asset, yarım commit ve eski dosya roundtrip kabulü gerekir. Mevcut tek JSON, base64 model ve 16 MB sınırı bu belgeyle değişmez; yeni format henüz uygulanmadı. Ayrıntılar geliştirme planındadır.


Tarih: 7 Ekim 2026. Native format: `.vfproject`, UTF-8 JSON. Bu format HTML demo JSON'u değildir.

## Katmanlar

- `domain/Project.hpp/.cpp`: Project/Recipe değer modelleri, ApplicationType/RecipeStatus enum'ları ve doğrulama.
- `application/ProjectService.hpp/.cpp`: aktif proje/reçete, değişiklik takibi, create/open/edit/select/save/discard işlemleri.
- `persistence/ProjectRepository.hpp/.cpp`: JSON load/save ve QSaveFile atomik commit. UI doğrudan dosya yazmaz.
- `app/MainWindowWorkspace.cpp`: Project Manager, Recipe Manager, New Project/New Recipe ve metadata düzenleme diyalogları. İşlem hataları form içinde veya ana pencerede gösterilir.

`visionflow_projects` Qt Core kullanır, Widgets/OpenCV gerektirmez. UI bu servisi kullanır; matching hedefi ayrı kalır.

## Dosya yapısı

6C eklemesi: root schema 1 içinde recipe schema 1 ve 2 desteklenir. Schema 2 zorunlu `calibrationBinding` alanında CalibrationDependency schema 1 nesnesi veya null taşır. Referans ID ile binding ID eşleşmelidir; eksik/yanlış tip/future schema reddedilir. Legacy recipe schema 1 binding içeremez; açılışta sürüm/dirty değişmez. Explicit binding komutu 1→2 yükseltir; mevcut JSON/base64 ve atomik proje commit korunur. Kanıt katalog dosyası ayrı CalibrationService kaydıdır, `.vfproject` içine gömülmez; proje ile tek transaction veya taşınabilir paket değildir.

Planar Calibration UI `definition.planarSetup` altında iki zorunlu metin alanı lensId/workingPlaneId taşır; alanlar 1–256 trimmed karakterdir. ProjectService setCalibrationSetup değişimi binding'i temizler ve DRAFT/dirty yapar. Source/raw resolution/coordinate current değerleri acquisition frame'den alınır; kurulum kaydı calibration dependency'den türetilmez. Evidence kataloğunun convention yolu `<proje.vfproject>.calibrations.json`; proje ve katalog birlikte taşınmalıdır. Runtime/provider henüz üretim kabulü değildir.

Proje ve tüm reçete metadata/configuration verileri aynı dosyadadır. Görüntü/model/kalibrasyon varlıklarını paketleme sonraki aşamalarda tasarlanacaktır. Tek dosya bütün reçetelerin birlikte atomik kaydedilmesini sağlar.

```json
{
  "format": "visionflow.project",
  "schemaVersion": 1,
  "id": "8ccdc0ce-142a-426c-802a-b4a51cda9d08",
  "name": "Inspection Line",
  "description": "Geliştirme projesi",
  "cameraReference": "ImageFile",
  "robotType": "",
  "communicationProtocol": "",
  "calibrationReference": "",
  "createdAt": "2026-10-07T00:00:00.000Z",
  "modifiedAt": "2026-10-07T00:00:00.000Z",
  "recipes": [
    {
      "id": "132ed549-5e43-4144-b5c2-179a46e82b32",
      "schemaVersion": 1,
      "recipeVersion": 1,
      "name": "Part_A",
      "description": "Örnek reçete",
      "applicationType": "defect_inspection",
      "cameraReference": "ImageFile",
      "calibrationReference": "",
      "status": "DRAFT",
      "createdAt": "2026-10-07T00:00:00.000Z",
      "modifiedAt": "2026-10-07T00:00:00.000Z",
      "definition": {}
    }
  ]
}
```

ID'ler UUID'dir; yeniden adlandırma, seçme, kayıt ve yeniden açma ID'leri değiştirmez. Proje ve reçete `schemaVersion` alanları format sürümüdür. `recipeVersion` pozitif tamsayı revizyondur; ilk kayıt v1. Türler `robot_guidance`, `defect_inspection`, `measurement`. Status değerleri DRAFT/VALID/INVALID; execution ve parça kararı burada saklanmaz.

`definition` bir JSON nesnesidir. Adım 3 ile `definition.editor` içinde ayrı schemaVersion 1 ve kalıcı tool/kaynak yapılandırması tanımlandı; ayrıntılar `editor-definition.md` içinde. Editor dışı alanlar korunur. Editor alanı olmayan Adım 2 dosyaları değişmeden açılır. Tür kartlarının başlangıç flow açıklaması gerçek tool listesi oluşturmaz.

## Değişiklik ve kayıt davranışı

New Project ve New Recipe önce bellekte oluşur. `Save Project / Recipes`, bütün değişiklikleri seçilmiş `.vfproject` dosyasına kaydeder. UI başlığındaki `*` ve Kaydedilmedi göstergesi başarılı commit'e kadar sürer.

Kaydedilmiş reçetenin metadata'sı değişirse DRAFT olur. Revizyon yalnız ilk değişiklikten sonraki başarılı kayıtta bir artar; tekrar değişiklik olmadan Save sürümü artırmaz. Yeni reçetenin ilk kayıt öncesindeki metadata değişiklikleri yine v1 olarak kaydedilir. Proje metadata değişikliği diğer reçeteleri değiştirmez. Şu anda geçmiş revizyon snapshot'ları tutulmaz; Version History sonraki geliştirmedir.

Proje değiştirme/kapatma sırasında Save/Discard/Cancel sunulur. Discard son başarılı kaydın bellek snapshot'ını geri getirir; yeni ve hiç kaydedilmemiş proje için proje kapanır. Reçeteler arasında geçiş bellekteki tüm taslakları korur.

QSaveFile direct-write fallback kapalıdır: atomik commit başarısızsa dirty/revizyon ve önceki dosya korunur. `.lock` dosyası eş zamanlı VisionFlow kayıtlarını engeller. Son okunan dosya içeriği değişmişse kayıt reddedilir; bellek değişiklikleri korunur. Bu kontrol genel amaçlı bir merge sistemi değildir; lock protokolüne uymayan harici editörlerin kayıt anındaki tüm yarışlarını önleme garantisi vermez.

## Doğrulama ve sınırlar

- Proje/reçete isimleri 1–64 yazdırılabilir karakter; reçete adı proje içinde case-fold karşılaştırmayla benzersiz.
- Açıklama en fazla 2048 karakter. CCUX-11 ile proje/reçete genel görüntü kaynağı ve
  kalibrasyon referansları opsiyonel legacy metadata'dır; yeni reçete bunları
  projeden miras almaz. Normal bağlama Editor kaynak düğümündedir. JSON alanları
  ve root/recipe şemaları korunur; aç/no-op kayıtta toplu silme/migration yapılmaz.
  `editRecipeMetadata` yalnız ad/açıklama değiştirir; legacy referans ve binding'leri
  korur. Eski explicit referans edit API'si ayrı kalır. Runtime legacy kalibrasyon
  çözümleme kontrolü korunur; boş referans doğrulanmış model veya üretim izni değildir.
- ID, tür, status, tamsayı sürüm, UTC tarih ve nesne/dizi tipleri doğrulanır.
- Bilinmeyen şema/tür ve root/recipe alanları reddedilir; daha yeni veriyi sessizce silerek kaydetme yapılmaz.
- Dosya boyutu en fazla 16 MB. Yeni proje yolu mevcut olmamalı; üst klasör mevcut olmalı. İlk aşamada klasör açma veya Save As yoktur.
- Hatalı açma mevcut proje/aktif reçeteyi değiştirmez. İşlem hatası dirty'yi kapatmaz ve pending sürümü artırmaz.
- Kamera/robot/kalibrasyon alanları metadata referanslarıdır; fiziksel cihaz veya kalibrasyon geçerliliği bu adımda doğrulanmaz. Yeni reçeteler VALID olmaz. Diskten okunan VALID metadata üretim yetkisi anlamına gelmez; Runtime yürütmesi henüz yoktur.
- Copy/Delete, bağımsız recipe import/export, tür değiştirme, genel paket migration, production version history ve rol/audit servisi henüz uygulanmadı. Recipe 1→2 explicit binding yükseltmesi ve ayrı calibration history uygulanmıştır.

## Doğrulama kanıtı

`project_persistence` testi üç tür/kalıcı ID, metadata roundtrip, sürüm artışı/no-op save, duplicate/boş alan doğrulaması, corrupt JSON, future schema, unknown fields/type, duplicate ID, definition roundtrip, lock/write failure, harici dosya değişimi ve discard senaryolarını kontrol eder.

`ui_shell_smoke` gerçek New Project/New Recipe/Edit Recipe diyaloglarını ve Save/Open Editor düğmelerini tetikler; arama/tür filtresi, kaydet/reopen ve ortak editor bağlamını doğrular. Test verisi `build/ui-workspace.vfproject` içindedir; üretim projesi değildir. Görseller `build/step2-*.png` altında oluşur. Native dosya seçici, QMessageBox tıklamaları, yüksek DPI ve çoklu monitör manuel doğrulama kapsamındadır.

## K3 tipli kalibrasyon bağımlılığı

Recipe schema 2 calibrationBinding, legacy dependency schema 1 yanında raw homography dependency schema 2 modelType/modelVersion taşır. Root/recipe sürümü artırılmaz; explicit bind DRAFT/revision-on-save korunur. Legacy dependency JSON/hash aynıdır. New model record/catalog ve offline study dosyaları ayrı atomik assetlerdir; açılış otomatik production izni vermez.

## P3 Runtime seçenekleri

Recipe definition.runtime isteğe bağlı {"trigger":"Manual" veya "SoftwareTrigger", "measurementToolId":"tool UUID veya boş"} nesnesidir. Alan yoksa Manual/mapping yok; açma/no-op Save migration veya dirty üretmez. Explicit setRuntimeOptions değişikliği DRAFT ve normal Save sürüm artışını kullanır. Bilinmeyen trigger veya alan/tip reddedilir. Endpoint/autoStart/ACK machine ayarları burada tutulmaz. Loaded immutable kopya draft seçeneklerini kendiliğinden izlemez. Eski binary ile yeni trigger anlamının uygulanması garanti edilmez; vf.socket/2 peer gerekir.
