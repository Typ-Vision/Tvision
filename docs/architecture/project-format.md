# Proje ve reçete kayıt sözleşmesi — Adım 2

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
- Copy/Delete, bağımsız recipe import/export, tür değiştirme, şema migration, version history, rol/audit servisi henüz uygulanmadı.

## Doğrulama kanıtı

`project_persistence` testi üç tür/kalıcı ID, metadata roundtrip, sürüm artışı/no-op save, duplicate/boş alan doğrulaması, corrupt JSON, future schema, unknown fields/type, duplicate ID, definition roundtrip, lock/write failure, harici dosya değişimi ve discard senaryolarını kontrol eder.

`ui_shell_smoke` gerçek New Project/New Recipe/Edit Recipe diyaloglarını ve Save/Open Editor düğmelerini tetikler; arama/tür filtresi, kaydet/reopen ve ortak editor bağlamını doğrular. Test verisi `build/ui-workspace.vfproject` içindedir; üretim projesi değildir. Görseller `build/step2-*.png` altında oluşur. Native dosya seçici, QMessageBox tıklamaları, yüksek DPI ve çoklu monitör manuel doğrulama kapsamındadır.
