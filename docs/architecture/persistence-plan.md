# A9 kayıt, varlık bütünlüğü ve migration planı

7 Ekim 2026. Bu belge sonraki paket uygulamasının tasarım kararıdır; çalışan paket/import/export değildir; 6C ayrı production snapshot/event/cycle arşivi uygulanmıştır. Mevcut `.vfproject` root schema 1, recipe schema 1–2, editor schema 5, inline base64 PNG ve 16 MB sınırı korunur. 6C sürümlü calibration/evidence kataloğu ayrı atomik JSON kaydıdır; taşınabilir container değildir. A8/6C authorization/active state bellektedir; snapshot/terminal kanıtlar ProductionStore ile disk arşivine yazılır. Disk VALID veya hash eşitliği restart üretim izni değildir.

## Paket ve manifest

6C/7'de taşınabilir paket için tek atomik dosya container tercih edilir. Container kütüphanesi ve uzantısı uygulama öncesinde seçilecek; mevcut `.vfproject` dosyası sessizce yeni formatla üzerine yazılmayacak. Manifest format/schema, project UUID, recipe UUID/revision, engine/tool sürümleri ve varlık kayıtlarını taşır. Her varlık role (model/mask/calibration), SHA-256, encoded byte length, media type ve göreli container entry ile tanımlanır. Snapshot içerik/dependency kimliği ayrıca saklanır; yeniden başlatmada servis yeniden doğrular.

Varlık kimliği dosya adı veya timestamp yerine gerçek encoded bytes üzerinden SHA-256'dır. Aynı bytes tek entry'de tutulabilir. Model geometri kimliği ROI ve tool UUID'yi de içerir; byte deduplication referans öğretimini onaylamaz. CalibrationDependency manifesti fiziksel katsayı veya doğruluk kabulünün yerine geçmez. Acquisition image varsayılan olarak dış girdidir; paketlenmeyen yollar ve camera/lens referansları taşınabilirlik raporunda açıkça belirtilir. Cycle görüntüleri paket geçmişine otomatik eklenmez.

Başlangıç bütçesi önerisi: manifest 16 MiB, en fazla 256 asset, asset başına 64 MiB, toplam açılmış asset 256 MiB, image başına 32 milyon piksel. Container overhead ve compressed input ayrıca sınırlandırılacak; decode öncesi boyut ve decode sırasında allocation kontrolü gerekir. Bu yeni limitler mevcut loader'a uygulanmış değildir; gerçek model/veri kabulünden sonra sabitlenir. Sadece 16 MB limitini kaldırmak kabul edilmez.

Absolute path, drive/UNC, `..`, boş/duplicate entry, case-fold path collision, symlink ve manifest dışı entry reddedilir. Import hedefi canonical doğrulamayla staging sınırında kalır. Missing/hash/size mismatch, unknown role/schema ve bozuk PNG/kalibrasyon JSON aktivasyonu engeller. Hash bütünlük kontrolüdür; imza veya güvenilir kaynak kanıtı değildir.

## Atomiklik ve recovery

1. Application taslak ve pending revision snapshot'ını alır; lock ve disk conflict kontrolü mevcut sözleşmeyle sürer.
2. Manifest ve tüm assets sınırlar altında staging container'a yazılır. Direct-write fallback kapalıdır; aynı filesystem üzerinde QSaveFile commit kullanılır.
3. Staging tekrar okunur; manifest, bütün entry hash/length ve semantik decode doğrulanır. Tüm kontrol bitmeden commit yapılmaz.
4. Commit başarılı olunca application dirty/revision/son kaydedilen snapshot'ını ilerletir. Hata/iptal sırasında eski dosya ve taslak korunur.
5. Açılış yalnız son commit'i okur. Staging parçaları active sayılmaz. Staging temizliği kilit/yaş/sahiplik kontrolüyle yapılır; eski snapshot'a referanslı assets retention nedeniyle silinmez.

Çok dosyalı manifest + asset dizinini bağımsız rename etmek atomik proje commit'i sayılmaz. Dosya sistemi rename başarısı elektrik kesintisinde disk durability garantisi değildir; hedef disk üzerinde recovery testi ürünleştirmede yapılacaktır. Audit/cycle kayıtları ayrı append ve retention sözleşmesine sahip olacak; paket commit'inin başarılı olması log teslimi sayılmaz.

## Migration sınırı

Root schema 1 için eski sürüm yoktur; future schema reddi sürer. Recipe schema 1 açılışta korunur; explicit kalibrasyon binding komutu 1→2 yükseltir ve kesin dependency/null alanını yazar. Editor 1–4 mevcut reader tarafından schema 5 çalışma görünümüne normalize edilir; açılış dirty/revision değiştirmez. İlk gerçek değişiklik güncel editor kaydını yazar; inline model bytes, tool/project/recipe UUID, applicationType, bağlantılar ve bilinmeyen editor dışı definition alanları korunur. Legacy referenceGeometry başlangıcı geçmiş öğretim geçerliliğini kanıtlamaz.

Yeni format uygulamasında her geçiş `N -> N+1` saf dönüşüm olarak kayıtlı olacak. Eksik geçiş, future schema ve bilinmeyen alan hata üretir; skip/downgrade yapılmaz. Orijinal bytes korunur, migration raporu source/target schema ve uyarıları gösterir. Legacy inline PNG yeni manifest asset'ine byte değiştirmeden taşınır; eski dosya import sırasında üzerine yazılmaz. Yeni paket roundtrip ve ikinci migration idempotence testleri uygulama tesliminde zorunludur.

## Kabul ve mevcut kanıt

| Senaryo | Bugünkü kanıt / sonraki kabul |
| --- | --- |
| Üç tür, IDs, revision/no-op, unknown/future schema | project_persistence |
| Lock/conflict, failed save dirty/revision | project_persistence |
| Yarım staging/cancel ve oversized load/save, eski commit korunması | A9 project_persistence; gerçek disk full/process crash henüz sınanmadı |
| Editor 1–4, model/bağlantı korunması, save/reopen | editor_services, tool_execution, ui_shell_smoke |
| Missing/corrupt model ve değişmiş asset dependency | production_recipe_contract, tool_execution; paket entry loader henüz yok |
| Widget uygulaması kapalı engine/application/domain | build/core, dokuz CTest; Qt Core/Gui/Concurrent ve OpenCV bağımlılıkları sürer |
| Container hash/path/bomb/crash/import/retention | 6C/7 paket uygulamasında yeni testler; bugün kabul edildiği iddia edilmez |

Windows Release CI planı: uygun Qt/OpenCV/MinGW ABI kurulu runner üzerinde `scripts/validate-local.ps1` çalıştır, core ve UI test sonuçları ile smoke PNG/ROI CSV'yi artifact olarak sakla. Remote repository/runner henüz yapılandırılmadı; yerel script CI çalıştığı anlamına gelmez. Donanım testleri ayrı producer/model/driver/SDK sürüm kaydıyla fiziksel runner'da yapılır.

## 6C uygulanan ayrı production arşivi

`<proje>.production` snapshots/events/cycles; per-record hash/kind/UUID wrapper, QLockFile + QSaveFile, append-only/idempotent. 20 MiB/kayıt, 512 MiB/10000 kayıt toplam; read son 128 (API en fazla 256)/64 MiB. Snapshot ve aktivasyon event commit'i başarısızsa servis izin/durumunu ilerletmez; CYCLE_BEGIN acquisition öncesi, terminal cycle commit tamamlanınca lease serbest kalır. Hata aynı terminal kaydı pending tutar ve Run'ı engeller. Restart arşiv auto-activate değildir. Katalog/proje/arşiv ortak transaction veya container değildir; crash reconciliation/retention/power loss fiziksel kabulü açık. Ayrıntı production-recipes.md.
