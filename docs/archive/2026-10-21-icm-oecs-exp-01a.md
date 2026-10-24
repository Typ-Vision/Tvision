# ICM-OECS-EXP-01a — hazırlanmış sahnede ilk fiziksel kontrol

21 Ekim 2026. Uygulayıcı: Codex.
Durum: tanı adımı tamam; EXP-01 fiziksel kabulü açık.

## Kapsam ve değişen dosyalar

Kullanıcı sahnenin hazır ve kameranın bağlı olduğunu bildirdi. Mevcut Release
`oecs_experimental_probe.exe` ile exact seri 02A2220000627 üzerinde edinim yapıldı.
Ürün kodu, profil sözleşmesi veya capability değiştirilmedi. Değişen belgeler:
`docs/progress.md`, bu kayıt ve `docs/archive/README.md`.

## Yeni kanıt

Kök: `build/icm/oecs-exp-01-scene/` (yerel, Git dışında).

- `single-5000.json`: sandbox içinde kamera bulunamadı. Cihaz yokluğu kabulü değildir.
- `single-native-5000.json`, `single-native-5000/frame-0.pgm` ve sidecar:
  sandbox dışında 5000 us / gain 0 ile tek owned Mono8 2448×2048 kare;
  exact kimlik ve profil readback, boş Stop kuyruğu ve temiz shutdown.
  Piksel 0–240, ortalama 27.849. Kayıpsız PNG açılıp incelendi:
  noktalı plaka görünür; görüntü karanlık ve odak yumuşak. Hassas ölçüm,
  kalibrasyon doğruluğu veya kullanılabilir seri kabulü değildir.
- `series-native-20000.json`: 20000 us / gain 0 ile **100/100** owned kare,
  boş Stop kuyruğu ve temiz shutdown. Yalnız ilk/son raster kaydedilir;
  bunlar sırasıyla 0–8 / 0–7 ve ortalama 0.335 / 0.334 ile neredeyse siyah.
  Son rasterin kayıpsız PNG'si açılıp incelendi. 100 karenin tümünün görüntü
  kalitesi incelenmiş değildir.
- `recheck-native-5000.json`: yeniden manuel açılışta 5000 us ile **3/3** owned
  kare ve temiz kapanış. Kaydedilen ilk/son rasterlerin ortalaması yaklaşık
  0.000017 / 0.000015; kararma sürüyor.
- `raster-stats.json`: kaydedilen beş PGM'nin boyut/min/max/ortalama/stddev
  değerleri. `.pgm.png` dosyaları yalnız kayıpsız görsel kontrol kopyalarıdır.

Tüm başarılı raporlarda productionAdmission=false ve
triggerAssociationVerified=false korunur. İlk görünür kare tazelik veya tetik
eşleşmesi kanıtı değildir. Pozlama değişikliği readback'te doğrulanmıştır;
kararmanın donanım, ışık, lens veya edinim kaynaklı olduğu henüz ayrılmadı.
Kullanıcıya seri sırasında ışık/diyafram değişip değişmediği soruldu;
bu kayıt hazırlanırken yanıt bekleniyor. Fiziksel unplug/replug yapılmadı.

## Doğrulama ve devam

Bu adımda ürün kodu değişmediği için yeni build/CTest koşulmadı; eski test
başarıları yeni koşum sayılmaz. Bağlam/belge kontrolü kapanışta çalıştırılır.
Önce ışık/diyafram koşulunu sabitleyerek kararmayı ayır ve görünür seri al;
sonra kullanıcıyla fiziksel kopma/yeniden açma gözlemini tamamla.
EXP-02, MUI-04 gerçek ölçüm, G1/production ve dağıtım kabulü kapanmaz.
