# Çıktı görünürlüğü

INT-07 presentation sözleşmesi. `visionflow_presentation`, domain registry ve
Qt Core kullanır; domain presentation katmanını çağırmaz.

`OutputVisibilityPolicy::resolve` gerçek registry portlarını sıraları ve typed
metadata bilgileriyle korur. Seçim yalnız sonuç listesini etkiler. `nullopt`
araç/mode/operation varsayılanını, explicit boş set tümü kapalıyı temsil eder.
Bilinmeyen anahtarlar yok sayılır; bilinmeyen-only explicit seçim varsayılana
dönmez. ROI ve Industrial Camera dahil mevcut registry tüketilir.

Properties → Çıkışlar gerçek port başına checkbox, Varsayılana dön ve Tümünü
göster sunar. Mode değişimi varsayılanı yeniden çözer; explicit seçimi korur.
Tercih recipe parametresi değildir; dirty/hash/generation değiştirmez, execution,
model decode/replay veya fit başlatmaz. Viewer ve overlay ayrı kalır.

`OutputVisibilityPreferences` mevcut yerel INI içinde project/recipe/tool UUID
üçlüsünü ayrı UTF-8 hex bileşenlerle saklar. Rename/restart kimliği korur;
yeni UUID varsayılanla başlar. Aynı UUID taşıyan proje kopyaları aynı makinede
aynı tercihi paylaşır. Başka makineye proje aktarımı tercihi taşımaz.
Sürümlü JSON kaydı explicit boş listeyi korur. Kilit ve QSaveFile atomik commit
başarısızlığı görünür hata verir; eski dosya korunur. Bozuk/okunamayan kayıt
nedenle birlikte varsayılana düşer. Reset yalnız ilgili kaydı kaldırır.

Editor güncel project/recipe/tool kimliğini kullanır. Runtime project kimliği
Runtime oluşturulurken yakalanır; recipe/tool/mode immutable snapshot'tan gelir.
Başka Editor seçimi Runtime filtresini değiştirmez. Experimental Runtime ek
scalar özeti aynı çözülmüş görünür port kümesini tüketir.

Gerçek unavailable nedeni önceliklidir; üretilmeyen declared port
`OUTPUT_NOT_PRODUCED`, boş actual invalid nedeni `UNAVAILABLE` gösterir.
Gizli invalid port uyarıları, execution durumu/hatası, gerçek parça kararı ve
source/frame/cycle kimliği tümü kapalıyken de korunur. Henüz sonuç yokken değer
veya PASS üretilmez. DAG/downstream, typed payload/cache, trusted digest,
terminal, Logs/CSV ve izinli PLC scalar teslimi görünürlükten bağımsızdır.
Experimental köken ve PLC/robot engeli değişmez.
