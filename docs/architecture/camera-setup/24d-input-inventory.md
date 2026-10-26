# CCUX-24D giriş envanteri

26 Ekim 2026. Üç basamak yalnız doğrudan uzunluk/koordinat içindir. JSON,
solver ve model hassasiyeti değişmez. Widget başlangıcı aşağıda belirtilmedikçe
kayıtlı parametre/ROI'nin özgün double değeridir; yeni araç varsayılanları domain
config/ToolRegistry'den gelir. Bütün mevcut üst sınırlar korunur.

| Dosya / widget → config | Birim; min/max ve başlangıç | Commit |
| --- | --- | --- |
| MetrologyEditor `mt_{stepPx,widthStepPx,sigmaPx}` | px / synthetic örnek; .05–16, .25–16, 0–16 | editingFinished → onChanged → Properties |
| `mt_{scanWidthPx,minWidthPx,maxWidthPx}` | px / synthetic örnek; .001–1024, 0–65536, .001–65536 | Aynı |
| `mt_{minSpanPx,minSpanMm,inlierDistancePx,inlierDistanceMm,maxRmsPx,maxRmsMm,maxResidualPx,maxResidualMm}` | İsimdeki px/mm; pozitif min .001, max 1e9 | Aynı |
| `mt_{lower,upper,diameterLower,diameterUpper,deviationLower,deviationUpper}` | Moddaki px/mm; 0–1e9, sıfır sınır geçerli | Aynı |
| `mt_{scanWidth,inlierDistance,maxRms,maxResidual}` | Moddaki px/mm; .001–1e9 | Aynı; açık birim başlangıcı komutu edit sayılır |
| `mt_roi_{ax,ay,bx,by,cx,cy,width,height,inner,outer}` → scanRoi.geometry | raw px / nominal world mm; koordinat ±1e8; extent min .001 | Koordinatları uygula → onRoi; mevcut ROI ya da mevcut çizim başlangıcı |
| EdgePointEditor `ep_{stepPx,widthStepPx,sigmaPx}` | .05–16 px, .25–16 px, 0–16 px | editingFinished → onChanged; diğer alan commit'inde özgün korunur |
| RoiEditor tablo sayıları → geçici geometry | px, ±1e8; width/height/inner/outer min .001 | Uygula; nokta ekle/sil önce özgün değerlerle read; şekil değişimi yeni başlangıç |
| PoseMatchEditor `pose_nmsDistance` | model px, 2–1024 | editingFinished → Properties |
| `pose_anchor_{x,y}` → anchor | model px, 0–255; öğretilmemiş anchor 0,0 | Properties / model öğret; diğer alanlar özgün kalır |
| MainWindowEditor `gm_{lower,upper,nominal}` | line_angle dışında seçili px/mm; -1e9–1e12 | editingFinished → Properties; signed sınırlar korunur |
| `gm_{fitInlierDistance,fitMaxRms,fitMaxResidual,lineMinSpan}` | Kalibre refit mm; .001–1e12 UI; domain ek doğrulaması korunur | Aynı |
| `scan_{ax,ay,bx,by,width}` → edge_point scanRoi | raw px / world mm; koordinat ±1e8; width .001–1024 | Koordinatları uygula; kayıt yoksa mevcut frame başlangıcı |
| `{lowerMm,upperMm}` → measurement | mm, 0–1e9 | editingFinished → Properties |
| MainWindowTracking `referenceOffset{X,Y}` → referenceBinding.offsetPx | px, ±1e8; kayıt yoksa 0 | editingFinished → bindReference / açık öğret; diğer ofset özgün kalır |
| `world_roi_{ax,ay,bx,by,cx,cy,width,height,inner,outer}` → nominal scanRoi | mm; koordinat ±1e8; extent .001–1e8 | Öğret ve kaydet; iptal reçeteye yazmaz; eski ROI veya mevcut nominal başlangıç |
| CalibrationWizard `wizard{Sampling,Sigma,StripWidth}` | px; .05–16, 0–16, 1–1024; .5,1,1 | Detect: örnekleme/solver girdisi |
| `wizard{KnownMm,ControlMm,Spacing}` | mm; .001–1e6; yeni boş giriş .001 | Hesapla / kontrol / plateTarget; kayıtlı plaka aralığı loadPrecisionInput ile yüklenir |
| `wizardControlLimit` | mm; 0–1e6; 0 | Kontrol; sıfır residual limiti geçerli |

`unitsPerSample`, `trackingMmPerSample`, `world_roi_mmPerSample` doğrudan
uzunluk değildir: mm/örnek dönüşüm katsayısıdır. Dokuz basamak ve 1e-9 pozitif
aralığı korunur, no-op özgün double ortak helper ile saklanır. minContrast
gray8/örnek, açılar derece, oran/skor/condition ve adet alanları üç basamak
dönüşümünün dışındadır; adetler mevcut sıfır basamaklı tamsayı davranışındadır.
Pozlama/gain bu dosyalardaki uzunluk envanterinde değildir.

## Edit ve no-op sözleşmesi

`PrecisionInput.hpp`: yükleme özgün double ve yuvarlatılmış ilk gösterimi ayrı
tutar. `textEdited` açık edit kanıtıdır; yalnız ilk `valueChanged` yükleme sayılmaz.
Spin adımı/değer değişimi de ilk gösterimden farklıysa edit'tir. Aynı görünen
değeri açıkça yazmak veya değiştirip aynı gösterime dönmek üç basamaklı değeri
kaydeder. Enter ve odak kaybı commit eder. Escape yüklenen gösterim ve özgün
değeri geri getirir; modal iptal mevcut reçete/model seçimini değiştirmez.

Başka alan commit'i, refresh, araç değişimi ve Save/reopen özgün değeri kullanır.
0.001 altındaki eski pozitifler 0.000 görünebilir; tooltip 17 anlamlı basamaklı
özgün değeri gösterir. Bu legacy durumda UI minimumu gösterim için 0 olabilir;
gerçek yeni edit en az 0.001 olarak normalize edilir. Modelde sıfır oluşmaz.
Eski açı/oran/dönüşüm değerleri de başka alan commit'inde yeniden yuvarlatılmaz.

Kanıt: `ui_input_precision`, gerçek reçete içerik/sürüm Save/reopen testini de
çalıştıran `ui_geometry_smoke` ve kartın ilgili diğer UI regresyonları.
