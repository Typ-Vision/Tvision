# T4.2/T4.3 — Yönlü Edge Point ve kullanıcı akışı

## T4.3 — uygulanan kullanıcı akışı

9 Ekim 2026. Güncel binary `build/edge-editor-ui/visionflow.exe`. Toolbox on türdür; **Edge Point** ekleyin → Image input bağlayın → **Ok çiz (A → B)** ile sürükleyin. Ok tam raw şerittir ve reçeteye kaydedilir. Uç/genişlik tutamaçları, **Ok koordinatlarını uygula**, **Ok yönünü ters çevir**, Escape çizim iptali ve Ctrl+Z geometri geri alma aynı ToolRoi sözleşmesini kullanır. Sayısal alanlar subpixel px'tir; Uygula öncesindeki alan düzenlemeleri taslaktır. Görüntü sıfırlama bildirimleri reçeteyi değiştirmez. Geçici **ROI geometrisi…** ayrı çalışma alanıdır, reçete ok öğretimi için Edge Point Properties kullanılır.

Properties: step/widthStep/sigma px, dahil minContrast gray8/px, A→B rising/falling/any, first/last/strongest ve subpixel. Ok yönünü çevirmek polarity'yi otomatik değiştirmez. Tam footprint/frame/format/workload retleri application/vision sınırında korunur; UI algoritma çağırmaz. Save/reopen/no-op ve signature/cache ortak servislerden gelir.

**Run Flow** yeni kare ve tam DAG; **Run Tool** aynı owned kare ve doğrulanmış upstream ile yalnız hedef executor. **Kenar profili** sekmesi gerçek ham yoğunluk (mavi), tam destekli Gaussian (sarı, desteksiz kısım çizilmez), signed türev (yeşil), seçili/kabul/ret işaretleri ve neden/inceltme tablosudur. Görüntü seçilen gerçek raw noktayı ve kayıtlı oku gösterir. Runtime tool seçicisi canlı profil/aday grafiğini ve kendi immutable snapshot okunu sunar; Runtime oku salt okunurdur.

Application `EdgeDiagnostics` UI'ya en çok 2048 eşit aralıklı profil noktası, 2048 türev ve 2048 aday verir; toplam/gösterilen sayılar görünür, seçili aday mutlaka tutulur. Büyük profillerin grafiği seyreltilmiştir ve tepe ayrıntısını kaybedebilir; algoritma seçimi ve typed sonuç tam veri üzerinden hesaplanır. Bu tanı verisi kalıcı port/terminal şeması değildir: restart/Logs/CSV mevcut gerçek point/strength/signedStrength/found/refined/method ve mm kararını korur; profil replay sunulmaz. Execution/9, edge_point v1/editor7/terminal3/vf.socket2 değişmedi.

İki Edge Point `point` çıkışını mevcut Measurement `p1/p2` girişlerine bağlayın; mevcut Calibration ile doğrulanmış ölçek/holdout/catalog binding ve mm toleranslarını hazırlayın. Yeni kalibrasyon sistemi yoktur. Measurement iki noktayı güncel modele dönüştürür; px Calculate ayrı kalır. Found=false başarılı arama ve grafiktir, fakat point invalid olduğu için downstream mm SKIPPED'dir. ERROR/Stop çıktıları ve canlı tanıyı temizler. Genel reçete kararı/confidence/robot pose yoktur.

Birleşik UI kabulü bağımsız 400 px raster stripe/40 px/mm → 10 mm PASS; çizim, subpixel sayısal edit, tutamaç/undo/iptal, ters ok+polarity, Save/reopen/no-op, TargetOnly/0 acquisition/raw hash/zoom, immutable Runtime draft kontrastını kullanmama, gerçek terminal/restart/CSV, no-found/downstream skip ve Stop'u sınar. Büyük stripe core testi gösterim sınırları/son örnek/seçili aday korunmasını doğrular. ON/UI ve OFF/core komut/sonuçları [progress](../progress.md) ve [toolchain](../toolchain.md) içindedir. 1400×900/850×600 ve Runtime/Measurement görselleri build altındadır; küçük ekranda scroll/zoom gerekir, native DPI kabulü değildir. Fiziksel hassasiyet/K4, P7/6D.2, lens T15, kurulum/dağıtım/CUDA kabulü açık kalır. Sonraki **T5.1 — Edge Pair**.

## T4.2 çekirdeği ve tarihsel kabul


9 Ekim 2026. `EdgeFinder` + `EdgePointConfig` + application `EdgePointService` ve registry/executor `edge_point` v1 uygulandı. [T4.1 profil](profile-sampling.md) kullanılır; legacy `edge` değişmedi. Editor schema 7, execution **visionflow.execution/9**; terminal 3/vf.socket/2 korunur. Engine kimliği değişimi eski validation iznini kullanmaz. Üç applicationType ortak engine kullanır.

## Aday ve seçim sözleşmesi

Rising/falling işareti A→B tarama yönüne göredir; any iki işareti kabul eder. Aynı işaretli eşit gradient örnekleri plato olarak gruplanır. Eşitlik toleransı `1e-9 * max(1, |a|, |b|)`; opposite işaretler mutlak gradient alınarak birleştirilmez. İki komşudan büyük yerel tepe bir adaydır. Profil ucundaki tepe `border_support`, yanlış işaret `polarity`, dahil minContrast eşiğinin altı `min_contrast` gerekçesiyle reddedilir. Düz profil veya sabit eğim kenar değildir. En az üç geçerli derivative örneği gerekir; daha azı ERROR'dur. Tek tepe uçta olabilir fakat destek dışından hayali örnek getirilmez.

Tek örnekli iç tepe, subpixel=true ise üç signed/oriented derivative örneğine ikinci derece fit ile inceltilir. Yeterli negatif curvature, finite offset ve `abs(offset) <= 0.5` örnek aralığı gerekir. Nokta gerçek step ile raw'a eşlenir; strength parabol tepe büyüklüğü, signedStrength yönlü karşılığıdır. Başarısız refinement discrete midpoint ve `unrefined_curvature` kalitesini taşır. Subpixel=false midpoint modudur. Çok örnekli düz tepede plateau midpoint kullanılır; `refined=false`, `plateau_midpoint` kalitesi açıkça verilir. Bu alt piksel doğruluk garantisi değildir.

Min Contrast sampled peak büyüklüğüne **dahil** uygulanır; parabolün yükselttiği değer eşik ret kararını geri almaz. Birim 0–255 normalize yoğunluk/px'tir; legacy Edge'in gray8 komşu farkı değildir. First/last kabul edilmiş adayların scan sırası, strongest kabul edilmiş fitted peak büyüklüğüdür; epsilon içinde eşit güçte ilk scan mesafesi seçilir. Ok ters çevrilince fiziksel aynı kenar için polarity de ters çevrilmelidir.

`EdgeSearch` bütün raw/smooth/derivative profilini, accepted/rejected adayların nokta/mesafe/güç/indeks/gerekçe/refinement bilgisini ve optional seçili indeksini tutar. Bu sonuç application katmanından kullanılabilir; T4.3 UI yalnız bounded application tanısını alır, doğrudan vision çağırmaz. Parametre/iş yükü/iptal sınırları ProfileSampler'dan gelir. Aday taraması ve plateau ilerlemesinde cooperative cancel vardır; partial sonuç dönmez. Hard interruption yoktur.

## Reçete ve yürütme

Strict `EdgePointConfig`: `scanRoi`, stepPx, widthStepPx, sigmaPx, polarity (`rising/falling/any`), selection (`first/last/strongest`), minContrast (`0 < x <= 5100`), subpixel Boolean. `scanRoi` tam ToolRoi schema 1/raw ScanSegment'dir; frame ID actual acquisition koordinat sistemiyle aynı olmalıdır. Boş varsayılan ROI editöre ekleme için saklanabilir, yürütme ve production prepare'da açıkça reddedilir. Unknown/missing/future/yanlış tipler kabul edilmez. ROI boyutu/görüntü alanı çalışma anında doğrulanır; source değişiminde eski frame ID ERROR verir.

| Port | Tip / birim | Bulunamadı |
| --- | --- | --- |
| image (girdi) | Image / px / image | Eksik/stale input executor'a verilmez |
| point | Point / px / image | invalid, null persistence |
| strength / signedStrength | Number / gray8/px / image | invalid |
| found | Boolean / image | valid false |
| refined | Boolean / image | invalid |

FOUND/NOT_FOUND başarılı arama durumlarıdır. ERROR/SKIPPED çıktısızdır; geçerli nokta yoksa Calculate/Measurement sayı üretmez. Genel parça kararı, mm, score/confidence veya pose üretilmez. Method `profile-edge` v1 gerçek portlarla terminal/Logs/CSV'de korunur. Profil/aday listesi henüz kalıcı port değildir; bu teslim seçili gerçek point/strength/found/refined ve refinement message'ını saklar. T4.3 canlı tanı adapter'ını ekledi; kalıcı port eklemedi.

Save/reopen/no-op aynı revizyonu korur; ROI/polarity/filter/selection içeriği tool signature'a girer. TargetOnly yalnız seçili executor ve geçerli processed upstream kullanır, acquisition/full-flow fallback yapmaz. Draft edit aktif immutable Runtime sonucunu değiştirmez. Stop/error eski nokta/karar bırakmaz.

## UI sınırı ve kabul

Registry on tür, Toolbox dokuz girdidir: yeni `edge_point`, ok/Properties editörü T4.3'e kadar Toolbox'ta sunulmaz. Mevcut `build/preprocess-code-ui/visionflow.exe` korunur; yeni Release regresyon binary'si `build/edge-point-ui/visionflow.exe`. Yeni UI yalnız Toolbox gate satırıdır; yeni Edge arayüzünün çalıştığı iddia edilmez. T4.3 ok çizimi/profil/aday grafiği, mevcut Measurement bağlantısı ve uçtan uca görsel kabulü kapsar.

`edge_point_contract`: bağımsız üç noktalı parabola, plateau midpoint/quality, inclusive/nextafter contrast, ilk/son/strongest/tie/polarity, border/flat/constant slope/yetersiz destek/iptal. 42 bağımsız Gray16 erf step raster: 128×128, 0/45/90/135/17.3/78.1/167.2°, üç subpixel phase, iki polarity, blur sigma=1.4, scan width=2, step=.5, profile sigma=1. Reverse physical point ve raw hash korunur. Clean RMS **0.00167085 px**, max **0.00362872 px**; geliştirme hedefleri RMS ≤.15/max ≤.30 px geçti. Ayrı 42 degraded raster: blur sigma=2.6, deterministic seed=1234567 ile ±6 gray-level noise, width=4; RMS **0.1474 px**, max **0.259068 px**; ayrı max=.5 px gate. Her iki küme sentetiktir, gerçek üretim/fiziksel corpus kabulü değildir.

`edge_point_tool_contract`: bağımsız 60 px stripe/iki Edge→Calculate; strict config, save/reopen/no-op, TargetOnly/processed cache invalidation, immutable Runtime, gerçek terminal restart/CSV, no-found point null replay/downstream skip, Stop, untaught/wrong-frame ERROR. Legacy Measurement/kalibrasyon regresyonu aynı suite'te korunur; yeni UI mm kabulü T4.3'tür. Sayısal rapor `build/edge-point-*/edge-point-report.txt`; tam koşu ve görsel kanıt [progress](../progress.md), komutlar [toolchain](../toolchain.md). Fiziksel K4/lens T15, P7 restart teslim güvencesi ve dağıtım/CUDA kabulü açık kalır.
