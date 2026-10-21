# ICM-OECS-EXP-01d — odak karşılaştırması ve geliştirme rasteri

21 Ekim 2026. Uygulayıcı: Codex.
Durum: kullanıcıyla üç elle odak ayarı karşılaştırıldı; görünür raster ve seçili
konumda seri edinim tamam. Hassas optik/mm doğruluk kabulü açık.

## Kapsam ve değişen dosyalar

Kullanıcı kamera/ışık/hedefin hazır olduğunu ve odağı elle ayarlayabileceğini
bildirdi. Başlangıç çekiminden sonra her fiziksel ayarı kullanıcı yaptı ve hazır
bildiriminden sonra ayrı tek kare alındı. İlk ayar kötüleşince ters yönde,
ardından aynı yönde daha küçük ayar istendi. Açı, yön ve mekanik konum ölçülmedi.
Ürün, UI veya probe kaynak kodu değişmedi. Değişen izlenen belgeler bu kayıt,
`docs/progress.md` ve `docs/archive/README.md`.

Kanıt kökü: `build/icm/oecs-exp-01-focus/`. `inspect_focus.py` Pillow/NumPy ile
kayıpsız PNG/kesit ve `focus-stats.json` üretir; ürün algoritması değildir.
Özgün PGM ve JSON sidecar deneysel kökeni korur. PNG'ler yalnız görsel inceleme
kopyasıdır; kaynak/replay dosyası olarak kullanılmaz.

## Yeni doğrulama ve kanıt

- Sandbox içindeki `baseline.json` kamera bulunamadı döndürdü. Donanım yokluğu
  sayılmadı; sandbox dışındaki `baseline-exact.json` gerçek kareyle başarılı.
- Mevcut Release probe ile exact Opto Engineering Srl /
  COE-050-M-USB-050-IR-C / **02A2220000627** kullanıldı. 20000 us, gain 0,
  Mono8 2448×2048 origin(0,0), metadata kapalı; FrameBurstStart/Software,
  burst=1, cache=false, TriggerDelay=0; gerçek readback JSON'larda.
- `baseline-exact`, `adjust-01`, `adjust-02`, `adjust-03`: her biri 1/1 owned
  kare; kamera açılıp kapandı. Merkez ROI (780,680)–(1420,1320), exclusive üst
  sınırlar; centered-difference gx/gy için ortalama gx²+gy² sırasıyla
  **46.626 / 32.045 / 42.911 / 47.853**. Bu yardımcı gradyan göstergesinin
  kabul eşiği yoktur; gürültü/kontrast/hareketten etkilenir. Optimum odak veya
  başlangıca göre anlamlı optik iyileşme iddiası kurulmaz. Kesitler açılıp
  incelendi; ilk ayar daha yumuşak, son ayar başlangıca yakın.
- `selected-series.json`: seçili son konumda **100/100 owned kare**,
  shutdownReady=true, stoppedQueueEmpty=true. 100 örneğin seri, geometri,
  pozlama readback ve deneysel false bayrakları Python assertion ile denetlendi.
  Sonuç: `100/100 identity/geometry/exposure/experimental assertions passed`.
- `selected-series/frame-0.pgm` ve `frame-99.pgm` özgün rasterleri ve sidecar'ları
  kaydedildi. Her ikisinin kayıpsız PNG'si açılıp incelendi: plaka, daireler ve
  çerçeve görünür; kenarlar hâlâ yumuşak. İlk/son ortalama **54.168 / 54.162**,
  stddev **47.372 / 47.367**, gradyan **47.785 / 47.734**, min/max **0–240**,
  255 piksel oranı 0. Ara 98 kare saklanmadı; görsel kalite kabulü verilmez.

Yeni build/CTest veya UI görsel koşumu yapılmadı; ürün kodu değişmedi. Önceki
Release/CTest kanıtı önceki kayıtlardadır. Bu kapanışta belge boyut/bağlantı
kontrolü ve `git diff --check` çalıştırılır; çıktılar aynı kanıt kökündedir.
Görev ID'li tek commit kaydedilir; push yapılmaz.

## Kabul sınırı ve sonraki görev

Bu kayıt görünür raster ve göreli odak incelemesini kapatır. EXP-02'nin **yalnız
px/raster geliştirmesi** için bu görüntü kullanılabilir; gerçek Line/Circle fit,
Editor bağlantısı ve MUI-04 ölçüm davranışı EXP-02'de sınanacaktır. Keskinlik için
bağımsız kabul eşiği, lens modeli, referans ölçü/tolerans ve mm doğruluğu yoktur;
EXP-01 bütünüyle fiziksel hassas ölçüm kabulü kazanmaz.

Native Unavailable, productionAdmission=false ve triggerAssociationVerified=false
korunur. Native cancel/drain varsayımı ve 8/8 eski kare karşı örneği geçerlidir;
bu seri tazelik veya doğru parça/tetik eşleşmesi kanıtı değildir. G0/G1/G3/G4,
PLC/robot, özgün MUI-04 raster/reçete/ROI ve dağıtım kabulü açık kalır.
Normal Image File replay EXP-02 köken desteğine kadar deneysel PGM'yi reddeder.

Sonraki görev EXP-02 Editor/Run Flow ve provenance; önce px Line/Circle,
Stop/invalidation, Save/reopen ve yanlış kalibrasyon negatifleri. Bağımsız gerçek
referans sağlanmadan mm doğruluğu kabul edilmez; EXP-03 daha sonra gelir.
