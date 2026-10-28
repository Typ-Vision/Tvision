# Shape Pose hızlandırma notu — SHP-PERF-01

28 Ekim 2026. Durum: inceleme/öneri; optimizasyon uygulanmadı. Kullanıcı gözlemi:
tam görüntüde yaklaşık 2 saniye, küçük Search ROI ile 1 saniyenin altı ancak
halen yavaş. Bu süreler bu oturumda yeniden ölçülmedi. Template Match'e açı
ekleme ve Pattern Pose'un bağımsız ürün değişiklikleri şimdilik kapsam dışıdır.

## Kaynakta görülen maliyet

- `src/vision/matching/ShapeMatcher.cpp`: model ve sahne gradyanı çıkarılır;
  aday üretmek için gradyan görüntüsünde `PoseMatcher::find` çağrılır. Ardından
  ROI boyutunda sekiz yön için hassas distance transform ve chamfer iyileştirmesi
  yapılır. Shape, mevcut pose aday aramasının maliyetini de taşır.
- `src/vision/matching/PoseMatcher.cpp`: her açı/ölçek kombinasyonu için maskeli
  korelasyon; kombinasyon başına sekiz tepe; ara aşamalarda 128'e kadar aday;
  piramit boyunca tekrar arama ve iki açı/ölçek iyileştirme turu vardır.
- `src/domain/PoseMatchConfig.cpp`: varsayılan -30…+30 derece/10 derece adım,
  ölçek 1 ve iki piramit seviyesi. Bunlar kullanıcının gerçek reçete ayarlarının
  tespiti değildir. Model/piramit/döndürülmüş şablon hazırlıkları çağrıda tekrarlanır.
- UI'daki `maxCandidates`, pose aramasının iç 128 aday bütçesini doğrudan azaltmaz.
  Shape aday üretiminde ayrıca `maxCandidates=32`, `minScore=0` kullanılır.
  Bu nedenle yalnız son aday ayarını düşürmek yeterli olmayabilir.

ROI küçüldüğünde sahne işi azalır; aday iyileştirme/hazırlık maliyeti kaldığından
süre aynı oranda düşmeyebilir. Bunlar kaynak bulgularıdır; hangi aşamanın gerçek
darboğaz olduğu profil ölçümüyle belirlenecek. Maskeli korelasyonun maskesiz
Template Match'e göre maliyeti kullanılan OpenCV derlemesinde ayrıca ölçülmeli.

## Önce ölçüm

Kaydedilmiş gerçek görüntü/model/reçete ile şu aşamalara ayrı süre ekle:
model decode/hazırlık, sahne gradyanı, kaba aday arama, pose iyileştirmesi,
sekiz mesafe haritası, shape refinement, toplam algoritma ve uçtan uca UI süresi.
Görüntü/model/ROI boyutu, açı/ölçek/adım, piramit, her aşamadaki aday sayısı,
Release/OpenCV/CPU/thread ayarı kaydedilsin. İlk çağrı ile ısınmış çağrılar ayrı;
aynı girdilerde en az 30 sıcak koşum için medyan/p95 ve en kötü süre raporlansın.
Gerçek görüntü kullanıcıdan mevcut dosya/reçete ile sağlanamazsa fixture sonucu
ayrı etiketlensin. Kanıt yeri: `build/shape-perf/shp-perf-01/`.

## Önerilen müdahale sırası

1. **Hazırlığı önbelleğe al.** Model kenarları/piramidi ve döndürülmüş destekler;
   anahtar model içeriği, anchor, tüm ilgili arama parametreleri ve yöntem sürümü.
   Kareye özgü sahne verisi sonraki karede kullanılmaz; bellek bütçesi sınırlanır.
2. **Aday üretimini ayır.** Shape için tam texture iyileştirmesi yerine hafif
   coarse proposal üret; nihai doğrulamayı shape yapsın. Ortak kod değişirse
   Pattern Pose regresyonları da çalıştırılır, bağımsız davranışı sessizce değişmez.
3. **Erken aday bütçesi.** Her seviyede kaliteli ve farklı konum/açı adaylarını
   koruyarak refinement işini sınırla. Belirsizliği tek adayı saklayarak yok etme;
   bütçe nedeniyle tarama kesildiyse truncated/ambiguous politikası korunur.
4. **Yerel hassas çalışma.** Düşük çözünürlükte kaba arama; tam çözünürlükte
   yalnız aday çevresinde gradyan/mesafe haritası. Halo ve sınır pikselleri,
   mevcut altı piksellik mesafe sınırı ve dönen modelin tüm desteğini kapsamalı;
   yerel hesap ile tam ROI skor eşdeğerliği doğrulanmadan devreye alınmaz.
5. **Ayar deneyi.** Mekanik sapma kadar açı aralığı, sabit ölçekte 1/1;
   model ayrıntısı yetiyorsa üçüncü piramit seviyesi. Ayrıntı kaybı/kaçan adaylar
   ölçülür. Açı adımını büyütmek tek başına doğruluk garantisi değildir.

## Kabul ve sınırlar

Aynı corpus ve ayarlarda önce/sonra medyan/p95; ayrıca konum/açı hatası,
kaçan/yanlış eşleşme, benzer aday/simetri, örtülme, sınır ve Stop/iptal karşılaştırılır.
Hız uğruna yanlış pose veya eski kare sonucu kabul edilmez. Performans hedefi
baseline ve çevrim bütçesi görüldükten sonra sayısallaştırılacak; şimdiden ms
veya hızlanma katsayısı taahhüdü yoktur. GPU/CUDA ve fiziksel üretim kabulü yoktur.
Bu not bir sonraki optimizasyon görevinin girdisidir; şu an uygulama sırasına alınmaz.
