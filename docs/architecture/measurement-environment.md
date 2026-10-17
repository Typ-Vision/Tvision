# Measurement environment — KMC-2

Proje başına uygulamanın sahip olduğu `MeasurementEnvironmentRegistry`, QWidget
ve reçete ömründen bağımsızdır. Komutlar owner thread'de; worker'a değer snapshot'ı
verilir. Bu paket eski MainWindow/Runtime davranışını değiştirmez.

`<proje>.measurement-environment.json`: strict schema1, SHA256 contentId,
4 MiB, varsayılan 128 kamera/kurulum (çağıran 1–4096), en çok 4096 tarihsel kurulum
revizyonu. Güncel kamera/kurulum, immutable geometri geçmişi, arşivlenen ID'ler,
kamera başına operasyonel SetupId ve exact model/content/version seçimi saklanır.
İsim değişimi geometri revizyonu üretmez. Silme yerine arşivleme vardır; arşivlenen
kaynak operasyonel olamaz. Geometri değişince geçmiş seçim korunur ve eski
revizyona bağlı kalır; bu seçim güncel uyumluluk veya kullanım izni değildir.

Repository QLockFile ile registry ve katalog yazım kilitlerini alır; yüklediği
belgenin tamamını beklenen snapshot ile karşılaştırır. Harici değişimde reload
gerekir. QSaveFile direct-write fallback kapalıdır. Yazma/commit hatası önceki
belgeyi korur. İçerik hash'i bozulmayı yakalar; bir kimlik doğrulama imzası değildir.

`saveAndRemember`: mevcut `<proje>.active-calibrations.json` codec/replay/hash
yolunda immutable aday önce yazılır; exact seçim sonra registry'ye commit edilir.
Yeni registry, legacy global selection alanını değiştirmez. Aradaki kesinti aday
bırakır; idempotent retry aday çoğaltmaz. Seçili record yoksa veya content/version,
kurulum geometrisi, lens/düzlem uyuşmuyorsa okuma/yazma açık hata verir. KMC-4
association/actual-source uyumu ve kullanım kapılarını ekleyecektir.

`exportBundle/importBundle` explicit Save As/import/export bağımlılık API'sidir.
Schema1 bundle registry ve eski kataloğu kapsar; eski model kanıtı/hash'leri
değişmez. Yeni projectId dış zarflara yazılır; hedef yeni olmalıdır. Önce katalog,
sonra registry yazılır; kesinti sonrası aynı aday katalogla retry mümkündür.
Restart ve kopya remembered seçim taşır, etkin izin taşımaz. Eski UI'daki proje
komutlarına API bağlantısı ve kullanıcı migration akışı [KMC-8 ile uygulanmıştır](measurement-setup-migration.md).

Yeni registry olmayan projelerde read/no-op Save hiçbir yan dosya üretmez.
Eski global dosyalar otomatik aktarılmaz. Bu servis sıfır reçeteli projede de
çalışır. İki ayrı dosya arasında atomik transaction, gerçek power-loss/disk-full,
fiziksel kamera veya çok-source Runtime kabulü yoktur.
