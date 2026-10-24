# ICM-04d — alternatif strateji ve timestamp karşı örneği

Tarih: 17 Ekim 2026. Uygulayıcı: Codex.
Durum: **yeniden değerlendirme/tanı alt adımı tamam; ICM-04d çözülmedi**.

## Kapsam ve sonuç

Kullanıcının önceki tıkanmayı yeniden inceleme isteği üzerine üretici taslağı,
kod, exact XML ve yeni/önceki üretici kılavuzları incelendi. Önceki
[semantik kaydı](2026-10-17-icm-04d-semantics.md) değiştirilmedi.

DeviceTimestampLatch ilk kez çalıştırıldı. Latch pozitif/artıyor; önceki writesiz
0 okuması destek yokluğu değildi. Metadata kapalı 12 kare ön deneyinden sonra,
host monoton saatleri eklenen son binary ile yeni oturumda 24 kare alındı.
Bu yeni koşumlar eski 100-trigger tanısının tekrarından farklı olarak clock fence
adayını ve eski-command frame'inin yanlış kabulünü sınar.

Exact kimlik öncekiyle aynı: Opto Engineering Srl /
COE-050-M-USB-050-IR-C / 02A2220000627, firmware V1.4.8 190411,19012902.
Sekiz FrameSpecInfo bankası false, payload 2448×2048 Mono8. Metadata enable,
exposure/gain/ROI, firmware/UserSet yazılmadı. Selector bankaları okumak için
geçici seçilip restore edildi; trigger source/mode/burst/cache restore/readback
başarılı. Latch komutu son-latched anlık değeri değiştirir, eski latch geri yüklenmez.

Son koşumda 24/24 frame; frame/trigger embedded counter ve unparsed chunk 0.
Latch yaklaşık 148.502 milyon, SDK frame timestamp yaklaşık 1.250014 milyar
birim/host-saniye ilerledi. Increment 0. Saat oranı veya offset üretim dönüşümü
olarak kullanılmadı; ortak clock domain/olay/reset semantiği çıkarılmadı.

12 örnekte command sonrası 150 ms bekleyip yeni command göndermeden latch
alındı, sonra önceki command'ın karesi okundu. `frameTicks > latch` 12/12 doğru;
24/24 kare timestamp'i alımdan sonraki latch'ten de büyük. Bu doğrudan karşılaştırma
stratejisinin somut karşı örneğidir; belli bir wait süresinin quiescence ispatı değildir.

Mimari yeniden değerlendirmesi: safety hedefi tek stratejiye eşitlenmemeli.
Belgeli eski-frame bariyeri + exclusive tek bekleyen tetik + temiz teslim +
belirsizlikte epoch invalidation başka bir yeterli strateji olabilir. Ready
düğümü ve iki sayaç bunun için aynı anda zorunlu değildir. Ancak exact bariyer
henüz yok; mevcut ActualCounterEpoch/Unavailable davranışı değiştirilmedi.
SingleFrame actual supported listesinde 2/0 bulunması da eski-frame bariyeri
kanıtı değildir.

Kullanıcı inceleme sırasında **ilk satırı ölçüm dışında bırakmayı onayladı**.
Bu izinle yeni `--frame-spec-valid-row` opt-in yolunda metadata açık 12/12 kare
alındı; frame/trigger sayaçları 1–12 ve byte 4/8 eşleşmeleri doğrulandı.
İlk kareden sensör row 0 çıkarılarak 2448×2047 owned Mono8 PGM kaydedildi.
Sensör başlangıcı (0,1), orijinal sensör 2448×2048, çıktı hash'i raporda açık.
Sekiz bankanın başlangıçta kapalı olduğu ve restore readback doğrulandı.
Firmware/ROI/exposure yazılmadı; sadece üç metadata enable bankası geçici açıldı.
Raster engeli için onaylanmış yol var; ürün geometri/kalibrasyon entegrasyonu ve
epoch/bariyer sorunu çözülmüş değildir. Native capability hâlâ Unavailable.

## Değişen dosyalar

- `tests/industrial-camera-probe/trigger.cpp`: explicit `--timestamp-fence`;
  metadata bankalarının kapalı olduğunu doğrular, latch/host/frame zamanlarını
  string uint64 alanlarla kaydeder; 24 örnek ve 12 eski-command senaryosu.
  ayrıca `--frame-spec-valid-row absolute-output.pgm`: 12 kare/layout doğrulama,
  ilk satırı çıkarılmış owned çıktı, dosyaya yazmadan önce başarılı cleanup.
  Önceki modsuz ve `--frame-spec` yolları korunur. Ürünle linklenmez/CTest değildir.
- `industrial-camera-trigger-correlation.md`: yeni kanıt, Chunk/transport ayrımı,
  alternatif stratejinin yeterli koşulları ve değişmeyen admission sınırı.
- `industrial-camera-vendor-questions.md`: sorular alternatif yollara ayrıldı;
  yeni clock uyuşmazlığı sayısal örnekle eklendi. Taslak **gönderilmedi**.
- `industrial-camera-contract-plan.md`: kullanıcı kararının geometri/kalibrasyon
  yükümlülüğü; henüz ürün entegrasyonu olmadığı açık. Progress/arşiv dizini.
  Ürün/UI/recipe/schema/local INI değişmedi.

## Yeni doğrulama ve kanıt

`build/icm/04d-timestamp/`:

- `probe-build.log`, `probe-final-build.log`: opt-in Release diagnostic build exit 0.
- `fence-sandbox.json`: sandbox exact cihazı bulamadı; fiziksel yokluk kanıtı değil.
- `fence-native.json`: 12 kare ön deney; restore/cleanup başarılı.
- `fence-final.json`: son binary 24/24, sekiz metadata bankası false,
  12 eski-command örneği, host/latch saatleri; restore/cleanup/exit 0.
- `inventory-after.json`: bağımsız salt okunur open/query/close/destroy exit 0;
  original trigger/source/burst/cache ve aktif metadata bankası geri okunarak doğrulandı.
- `validate.py`, `validated-summary.json`: 24 örnek, 12 yanlış tazelik karşı örneği,
  metadata kapalı, restore/cleanup, exact kimlik ve dosya/binary SHA256 assertions.
- `valid-row-build.log`, `valid-row.json`, `valid-row.pgm`: sonraki binary Release
  build exit 0, metadata açık 12/12 kare, ilk satırı çıkarılmış PGM. Validator
  header/payload boyutu, hash, origin, sayaç/layout ve restore assertions geçti.
  Timestamp ve valid-row capture binary hash'leri ayrı tutuldu.
- `invalid-cli.json`: exit 1, command/frame 0. Mevcut SDK build'indeki
  `industrial_software_trigger` ve `industrial_backend_binding` **2/2** yeni CTest
  koşumu; `sdk-tests.log`, `ctest.xml`. Ürün kaynakları yeniden derlenmedi;
  tam suite, görsel kontrol, uzun exposure/unplug/reset/stress yapılmadı.
- `context-check.json`: kapanış bağlam sınırları ve link kontrolü.

Yerel CHM'yi hh ile açmadan decompile girişimi dosya üretmedi; yeni kanıt sayılmadı.
Web kaynakları ve yorum sınırları karar belgesinde bağlantılıdır. Başka üreticinin
timestamp semantiği exact cihaz garantisi olarak kullanılmadı.

## Sınırlar ve sonraki görev

Native capability Unavailable; ICM-04d açık, 04e başlamadı. Aynı iki ham timestamp'i
karşılaştırarak veya deneyden ölçek fit ederek üretim açılmamalı. Sonraki iş
**tek bir uygulanabilir korelasyon stratejisi için eksik semantiği kapatmak**:
öncelikli alternatif temiz stream bariyeri + single-flight; timestamp yolu için
clock/event mapping; sayaç yolu için onaylanan ilk satır dışlama kararının ürün
geometri/kalibrasyon entegrasyonu ve epoch. İlk satır izni tekrar sorulmaz.
Üreticiye gönderim bu çalışmada yetkilendirilmedi/gönderilmedi.
G0–G4/H01–H08, sahne/lens/ölçüm, stress/unplug ve dağıtım/CUDA sınırları korunur.
ICM-04d görev ID'li tek commit; push yok.
