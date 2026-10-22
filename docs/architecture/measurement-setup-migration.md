# Açık kurulum geçişi, aktivasyon ve taşıma — KMC-8

Sistem özeti → Etkinleştir seçili kayıt için inceleme açar. Eski/yeni seçim, camera/setup revision, özgün köken, ayrı bağımsız kontrol, yalnız bu kuruluma bağlı reçetelerin px tolerans/exact world yeniden öğretim etkisi gösterilir. Sıfır reçete geçerli bir incelemedir. Operasyonel kurulum, Runtime/worker/pending kayıt/ACK, trial/ret ve desteklenmeyen geometri engelleri açık görünür. Resim çek komutu uygulama worker’ında yeni gerçek kaynak karesi alır; owner thread opaque completion’ı kabul edip exact hedefi yeniden denetler. Dosya probe olmaz. Başarısız capture/commit eski seçimi korur; close/geç completion aktive etmez.

Eski kayıtları kurulumla eşle komutu açık optik/düzlem/geometri beyanı ister. Global katalog wrapper1–3 veya eski matematik katalogdaki bir ID’nin **bütün ardışık sürümleri** önce denetlenir. Yeni wrapper5 özgün wrapper/modeli ve ayrı türetilmiş kanıtı içerir; ID yeni migration grubudur, sürümler korunur. Kayıt matematiği yeniden oynatılır. Özgün dosyalar/global seçim/proje politikası değiştirilmez. Legacy raster hash/frame eksikse eksik kalır. Yeni adaylar kendiliğinden seçilmez; mapped legacy de gerçek camera probe ve ayrı etkinleştirme ister.

Optik, düzlem, raw koordinat çerçevesi, world eksen yönü veya çözünürlük farklıysa eşleme reddedilir; ayrı uygun kurulum/yeniden kalibrasyon gerekir. Aynı boyut tek başına fiziksel kimlik kanıtı değildir. Wrapper5, geçmiş edinimin kaynak tool ID/frame/model kanıtını ve eski adapter’ını özgün zarfında korur. Açık beyan fiziksel ölçüm kabulü sayılmaz.

Çok kayıtlı geçiş atomik transaction değildir. Bütün matematik preflight yapılır, sonra immutable ardışık prefix yazılabilir. Kesilen geçişte retry mevcut özgün kanıtı doğrulayıp kaldığı sürümden devam eder; conflict mevcut kayıtları ezmez. Kaynak properties’de açık bağlama scoped-source/1 politikasına geçirir. Bu politika bağ kaldırılınca geri dönmez; seçimsiz scoped reçete mm için global fallback kullanmaz.

## CCUX-08 — tek kaydet ve kullan komutu

`MeasurementWorkspace::saveAndUseCalibration` / scoped service `saveAndUse`,
immutable adayı, exact hedefi, beklenen environment revision'ını, kabul edilmiş
gerçek kamera probe generation'ını ve caller'ın current-target/project/Stop
denetimini alır. Dönüş `Failed`, `CandidateSaved` veya `Activated`; content ID,
duplicate, deneysel nitelik, ayrı bağımsız kontrol sonucu ve hata içerir.
Kontrol yoksa null kalır; fiziksel kabul veya sıfır hata üretilmez.

Aday yazımı ile seçim commit'i iki ayrı dosya işlemidir. Aday kaydından sonra
gate, current target, generation, kamera disconnect ve oturum politika epoch'u
yeniden denetlenir. Hata önceki seçimi korur; mevcut aday aynı content/hash ile
yeniden kullanılır. Aynı başarılı komut tekrarlandığında yeni katalog satırı veya
aktivasyon revision'ı oluşmaz. Başka registry değişimi stale komuttur; reload
gerekir. Runtime'a yayın yapılmaz; önceden alınmış snapshot immutable kalır.

Trusted yol önceki accepted gerçek kamera probe denetimini korur. Deneysel
yol ayrıca `setExperimental(true)` oturum onayı ve typed deneysel **canlı** probe
ister. Exact PGM+JSON adayı dosya kökenini korur; dosya aktivasyon probe'u olmaz.
Bağımsız kontrol ret eşiği aynı kalır. Deneysel seçim record schema6 üzerinden
hatırlanır; restart/import onay veya kullanım izni başlatmaz.

`experimentalSnapshots` ayrı `ExperimentalScopedCalibrationSnapshot` tipini döner;
trusted `snapshots`, legacy aktif servis ve `SystemCalibration` bu modeli
reddeder. Edinim zarfı schema2 / `experimental-calibration/1`, özgün typed probe,
backend ve false production/trigger-association/physical-acceptance alanlarını
taşır. Onayı kapatmak deneysel izinleri siler; tekrar açmak eski izni diriltmez.
CCUX-09 UI'sı iki yöntemde bu komutu çağırır. Ready ve tek anlamlı varsayılan
kurulum exact hedefse, `saveAndUse` operasyonel seçimi ayrı kullanıcı adımı olmadan
son seçim commit'ine dahil eder. Legacy `activate` açık operasyonel seçim kuralını
korur; belirsiz/eski varsayılan otomatik seçilmez. Aday sonrası hata operasyonel
seçimi de değiştirmez. Editor/Runtime metrik tüketimi sonraki kartlardır.

Kabul: `calibration_save_use` ile kartın dört regresyon suite'i. Fixture sonuçları
fiziksel kamera veya bağımsız mm kabulü değildir.

## Taşıma

CCUX-01: bundle1 yeni environment2 `registrations` ve pending setup2/profile2
kayıtlarını da taşır. Alias, CameraId/setup/profile ID'leri ve arşivler korunur;
boş projeye import yalnız projectId/revision sahipliğini yeniler. Merge olmadığı
için hedefte alias çakışması overwrite ile çözülmez; kaynak içindeki alias/ilişki
çakışması strict decode ile reddedilir. Save As aynı kayıtları korur. Readback,
yerel route veya oturum/üretim izni eklenmez. Legacy belge açılışında migration yoktur;
ilk açık metadata/default seçim komutu gerektiğinde yeni kayıt biçimini oluşturur.


- **Kalibrasyon bağımlılıklarını dışa aktar / boş projeye al:** .vfmeasurement schema1 registry + immutable katalog. Proje, üretim arşivi ve görüntüleri içermez. Yeni proje ID’sine açık import edilir; remembered seçim kullanım izni değildir. Mevcut registry üzerine merge/overwrite yoktur.
- **Kaydedilmiş projeyi bağımlılıklarıyla farklı kaydet:** saved proje bytes, registry/katalog, varsa legacy .calibrations.json ve tüm snapshots/events/cycles kopyalanır. Worker kaynak/destination lock, SHA256, strict replay, symlink/path/boyut ve son bağımlılık karşılaştırmalarını denetler. Üretim sınırı 10000 kayıt / 512 MiB; dosya başına 20 MiB. Proje en son görünür olur. Kesinti sidecar prefix bırakabilir; aynı snapshot ile retry idempotent, çatışmada overwrite yoktur. Bağımlılıkların tek atomik transaction olduğu iddia edilmez.
- Görüntüler otomatik paketlenmez. Destination’da relative asset aynı içerikle mevcut olmalı (aynı klasörde farklı adla Save As geçerlidir); eksik/değişmiş asset veya tool dependency açık ret alır. Kaynak proje bytes’ı/IDs/geçmiş değiştirilmez. Kopya/import explicit yeniden aktivasyon ister. Taşınabilir tam asset paketi veya disk-full/power-loss kabulü değildir.

UI worker’a widget/live registry vermez. Capture, dosya association ve proje kopyası immutable işlerle yürür. Owner kaydetme/aktivasyon sırasında gate ve current target’ı tekrar denetler. Metadata discovery ve kabul edilmiş webcam kaynak hatası ilgili izinleri düşürür; dosya replay hatası fiziksel kamerayı disconnected saymaz. Donanım sürekli izlenmez; gerçek bağlantı kaybı discovery/edinim veya caller disconnect olayıyla bilinir.

Kabul testleri: measurement_setup_migration, ui_scoped_calibration_activation ve önceki scoped Editor/Runtime/overview/wizard testleri. Görsel kanıt build/reference-ui/kmc8-activation-*.png ve kmc8-migration-small.png. [KMC-9 birleşik A yazılım kabulü](independent-calibration-acceptance.md) tamamlandı. Fiziksel kimlik sürekliliği, native DPI/çok monitör, lens/robot, CUDA ve dağıtım açıktır.
