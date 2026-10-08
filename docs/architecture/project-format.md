# Proje ve reçete kayıt sözleşmesi — Adım 2

8 Ekim C1: Manuel matematik/codec `manual_planar_scale/1` için ayrı record/dependency schema 3 kullanır. Schema 3 dependency mevcut frame ve kurulum alanlarına modelType/modelVersion ekler; schema 2 hâlâ yalnız homography kabul eder. Root/recipe/editor şemaları değişmedi. Tam alan listesi [manuel kayıt sözleşmesinde](manual-scale-calibration-plan.md#c1-uygulanan-manuel-kayıt-sözleşmesi). C1 yalnız servis matematik/replay kabulüdür; manuel katalog/recipe Save/reopen/Runtime zincirinin kabulü C2'de, ekran C3–C4'tedir.

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
- Açıklama en fazla 2048 karakter; görüntü kaynağı referansı boş olamaz. Robot/protokol/kalibrasyon opsiyoneldir.
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
