# T6.4 — Circle / Arc Editor ve kalibre Runtime

10 Ekim güncel kapsam: **T8.4.2b.3b — Nominal world ROI authoring ve mm metric/tolerans tamamlandı.** Edge Point/Caliper/Line/Circle için **Nominal mm ROI öğret…** ekranı geometriyi, güncel pose teaching kaydını ve DAG bağlantısını atomik kaydeder; pending/iptal/geçersiz geometri reçeteyi değiştirmez. Caliper gerçek mm genişliğini, Line current mm span/açısını dahil tool toleranslarıyla değerlendirir. Circle tüm gerçek gözlemlerden (fit outlier dahil) radyal sapma, current world açı/gap/observed arc ve kaliteye bağlı tolerans üretir. Raw line/circle fit ve synthetic derivative strength unavailable; genel parça kararı NOT_EVALUATED. Engine **execution/19**, registry16/editor7/terminal3/vf.socket/2 korunur. Production pose whitelist reddi sürer. Sonraki **T8.4.2b.3c — Trusted completion/immutable production Runtime/Logs/restart/CSV ve corpus kabulü**; T8.4 açık. Güncel Release binary: `build/tracking-ui/visionflow.exe`.

Alttaki teslim kayıtları tarihseldir; güncel kapsam yukarıdadır.

Nihai T6.4 yazılım kabulü: Release UI **45/45**, Widgets OFF core **41/41** CTest; normal/küçük ekran ve perspektif/outlier görselleri incelendi. Kalibre Circle testinde280 kontrol.


10 Ekim 2026. Registry `circle_fit` v1, application `CircleToolService` ve ortak executor; üç applicationType aynı yolu kullanır. Toolbox 13 türdür. Engine `visionflow.execution/11`; editor7/terminal3/vf.socket/2 ve root/recipe şemaları korunur. Eski engine validation/cache yeniden hazırlanır. Çalışan UI: `build/circle-ui/visionflow.exe`.

## Kullanım ve birimler

1. Circle / Arc ekleyin, Image input bağlayın ve kaynaktan görüntü edinin.
2. Pixel modda raw halka çizin veya cx/cy/inner/outer/start/sweep subpixel koordinatlarını **ROI koordinatlarını uygula** ile öğretin. Annulus tam halka; signed sweep ile ArcRoi yaydır. Yayın start/end, halka merkez/iç/dış yarıçap tutamaçları, Ctrl+Z ve Escape desteklenir. Zoom/pan geometriyi değiştirmez.
3. Kalibre kullanımda Setup → Calibration ile gerçek edinilmiş kaynakta bağımsız holdout kabulünü kaydedip bağlayın; `mode=calibrated` seçin. Mod değişimi eski ROI'yi temizler; px değerler mm diye yorumlanmaz. Halka/yayı yeniden çizin veya **world mm** koordinatları girin. Uniform/manual Y-up düzleminde raw pozitif Y'nin world Y'si negatiftir. Homography target-local axes kullanılır. Cursor raw noktası application servisiyle world'e, çizim ve tutamaçlar world'den raw'a taşınır. Perspektif raw şekil eliptik görünebilir; raw daireyi scale etmek kullanılmaz.
4. `scanWidth` ve fit inlierDistance/maxRms/maxResidual seçili düzlemin px/mm birimindedir. Pixel `unitsPerSample=1`; calibrated açık mm/synthetic-px örnekleme adımıdır (mod değişiminde başlangıç .1). stepPx/widthStepPx/sigmaPx synthetic sampling birimidir; intensity gradient threshold fiziksel mm veya uncertainty değildir. Kalibre fit eşiklerini işinize göre ayrıca seçin; pixel varsayılanları fiziksel kabul değildir.
5. Diameter lower/upper ve deviation lower/upper **ayrı**, dahil ham double sınırlarıdır. Deviation bütün bulunan noktaların radial peak-to-valley değeridir; fitten elenen çapak/çentik gizlenmez. Bu araca ait Boolean sonuç genel parça PASS/FAIL değildir. Save → Load Recipe → Run → Tek çevrim mevcut immutable Runtime yoludur.

## Ölçüm, kalite ve sonuçlar

[Radyal servis](radial-scan.md) her world intensity tap'ini raw'a inverse-map eder; [fit](circle-fit.md) ve [metrikler](circle-metrics.md) world noktaları üzerinde çalışır. Güncel bağımsız resolver/version/content/setup/source/actual resolution/frame/cycle/time/file-test ve calibrated hull kapıları korunur. **Gerçek fit merkezi** ve fitted yay/halka overlay örnekleri de alan kontrolünden geçer. Alan dışı fit veya bağlam ERROR üretir; sessiz px fallback yoktur. Overlay 180 segmentlik gösterimdir, ölçüm doğruluğunun veya aradaki yüzeyin kanıtı değildir.

`circlePx/Mm`, `arcPx/Mm`, centre/radius/diameter, fit RMS/max, radialMin/Max, maxInward/Outward, observedDeviation/deviation ve circularityIndex typed sonuçlarıdır. Yalnız ölçülen düzlemin portları geçerlidir; diğer düzlem null/OTHER_PLANE. Arc'ta fitted Circle2D yanında signed Arc2D vardır; tam halkada Arc2D invalid/FULL_CIRCLE. Pixel portları diagnostic'tir. `scans`, `slots`, `observationsPx/Mm` missing/used/outlier ve residual kanıtını korur. Scan bütçesi 6–128; çekirdek 4096 sınırı ayrı kalır. `slots` kaydı her scan ID için found/missing ve fit üyeliği; PointSet yalnız found noktaları aynı sırada tutar.

`scope` full-circle / observed-arc / unavailable; `qualified` kalite kapısının sonucudur. Min found/coverage ve max nominal gap geçmezse execution DONE ve fit/observed tanı kalır, **deviation, diameterAccepted, deviationAccepted ve accepted invalid/NOT_EVALUATED** olur. Hiç fit elde edilemezse ERROR ve bütün sonuçlar temizlenir. Arc yalnız görülen yay için tolerans verir; circularityIndex yoktur. Index yalnız eksiksiz, quality-valid, basit gerçek kapalı measured polygon'dan hesaplanır; missing noktalar kapatılmaz. Genel part decision NOT_EVALUATED kalır; PLC vf.socket/2 legacy Measurement mapping'i değişmez.

## Runtime ve kalıcı kanıt

TargetOnly aynı ham kare/cache ile acquisition yapmadan yalnız Circle executor'ını çalıştırır. Mode/ROI/parametre/binding/engine değişimi eski sonuçları geçersiz kılar. Stop/geç worker sonucu eski metric/overlay bırakmaz. Runtime editör taslağından bağımsız snapshot çalıştırır. Logs/restart/CSV mevcut typed terminal codec'iyle Circle/Arc, birim, method, kaynak ve scan kanıtını taşır.

Production completion immutable recipe ve stored nominal-plane observations üzerinden bounded fit/metric replay yapar: slot/membership, geometri, kapsam/coverage/gap, çap/sapma, iki ayrı limit ve combined Boolean denetlenir. Metrikler ve ham tolerans sınırları **birebir** karşılaştırılır. Yalnız karara katılmayan long-double polygon index'in worker/UI replay denetiminde mutlak `16*double epsilon` payı vardır; gözlenen thread farkı 3.3e-16'dır. Limitlerde epsilon/yuvarlama yoktur. Bu tutarlılık denetimi görüntüden bağımsız yeniden acquisition veya dış saldırgana karşı imza değildir; arşiv hash/atomiklik politikası korunur. Replay en çok128 scan'dir; üretim latency kabulü değildir.

ROI px schema1 bytes korunur. World mm yalnız annulus/arc için opt-in **ToolRoi schema2**, explicit unit/axes/frameId taşır; raw `inside(image)` world ROI'yi reddeder ve application mapping gerektirir. Root/recipe/editor şeması yükseltilmez; eski tool konfigürasyonları değişmez.

## Yazılım kabulü ve sınırlar

`calibrated_circle_test`: bağımsız erf raster → gerçek DAG px/uniform Y-up mm; Save/reopen/no-op; TargetOnly/raw hash; dahil ve nextafter çap/sapma sınırları; iki limitin bağımsızlığı; missing/gap NOT_EVALUATED ve kalıcı Runtime; signed arc; Stop/no-fit/stale/unbound; immutable snapshot; Boolean/metric/diagnostic/scope tamper ret; typed Logs/CSV/restart. Non-affine analytic project/unproject Gray16 world radius7mm: diameter14mm ±.06mm, deviation<.02mm, 181 overlay noktası ayrı analitik inverse oracle ile1e-5px; actual fitted centre calibrated hull dışındaysa ret.

`roi_viewer_interaction`: perspektif cursor→world ring/inner handle, mapping refresh sonrası undo, half-arc Escape; `ui_circle_smoke` gerçek Toolbox/numeric mode teaching/Run Tool/Run Flow/Runtime/Stop, signed yay, kalite, perspektif ve hasarlı contour kabulüdür. Bağımsız world rasterındaki ~1mm çapak fitten elense de deviation> .5mm ve deviationAccepted=false kalır. 1400×900 / 850×600 Editor, Runtime ve world handles PNG'leri build altında incelenir. Küçük ekranda Properties/Results scroll gerekir; overlay etiketi viewport kenarında kesilebilir, tam değer Results/Runtime panelindedir. Offscreen test native DPI/çoklu monitör kabulü değildir.

Nihai Release CTest sonuçları ve hata/düzeltmeler [progress](../progress.md) kaydındadır. Mevcut kullanıcı binary/config korunur; yeni uygulama kendiliğinden başlatılmaz. Fiziksel hassasiyet/üretim corpus/yanlış gerçek edge/texture/ringing, minimum-zone/ISO/GD&T/uncertainty, K4/P7/6D.2/lensT15/kurulum/dağıtım/CUDA kabulü açık. Sonraki **T7.1 — Geometrik domain matematiği**.
