# T8.1 — Pattern Pose çekirdeği

10 Ekim T8.4.2b.3a: **Toolbox16 / Pattern Pose ve Shape Pose teaching UI, altı raw tüketicide ortak pose reference picker ve mevcut world Circle teaching** uygulandı. EditorSession gerçek bağlı image/cache snapshot ile worker öğretimi ve Stop/stale suppression; ProjectService binding+connection atomik edit sağlar. Kayıtlı world Edge/Caliper/Line envelope yeniden öğretimi desteklenir; yeni nominal mm authoring ekranı b.3b'dedir. Engine execution/18, registry16, editor7/terminal3 değişmedi. Production pose whitelist reddi sürer; trusted completion/immutable Runtime/Logs/CSV ve world metric/tolerans/current arc henüz kabul edilmedi. [Güncel kullanım ve sınırlar](pose-roi.md).

Alttaki teslim kayıtları tarihseldir.

10 Ekim güncel durum: **T8.4.2b.1 — Raw Code/Blob tracking bağlantısı tamamlandı; T8.4.2b/T8.4 açık.** Code Read v2 ve Blob v3, shape/texture producer sonucuyla explicit poseTracking teaching ve poseReference DAG/cache yoluna bağlandı. Nominal searchRoi pixel-centre dikdörtgeni rigid taşınır; gerçek dönen maske kullanılır. Blob eşik sonrası maskeler; Code dış pikselleri beyazlayıp tüm decoded köşelerin ROI içinde kalmasını ister. AABB yalnız çalışma tamponudur; clipping/absolute fallback yoktur. Save/reopen/no-op, iki matcher × üç applicationType, TargetOnly/acquisition0, raw alan/delik/merkez, gerçek QR polygon/içerik ve bağımsız 45° maske kabulü eklendi. Engine execution/17; Toolbox14/registry16, editor7/terminal3/vf.socket/2 korunur. UI kaynakları değişmedi; reference picker/teaching ekranı, world sampler/refit ve production trusted completion/immutable Runtime/Logs/CSV tracking kabulü açıktır. Pose production whitelist reddi sürer. [Sözleşme ve sınırlar](pose-roi.md).

Alttaki durum/binary/sıra kayıtları tarihseldir; güncel kapsam yukarıdadır.

`pattern_pose` v1, C++20/OpenCV `PoseMatcher` ve application `PoseMatchService` üzerinden ortak Editor execution DAG'ında çalışır. UI algoritma çağırmaz. Üç applicationType aynı yolu kullanır. Legacy `template_match` sabit açı/ölçek ve Point/Region çıktısını korur; kendiliğinden Pose2D üretmez.

T8.2 güncellemesi: ortak [Pose ROI adapter/codec](pose-roi.md) tamamlandı; bütün araç/UI/Runtime bağlantıları T8.4 kapsamındadır.

T8.1 teslimi programatik teaching/DAG'dır. Toolbox girdisi, özel Properties/Viewer öğretim akışı, pose referanslı ROI ve production Runtime/trusted completion henüz yoktur. ProductionRecipeService whitelist bu kind'ı açıkça reddeder. Typed codec roundtrip, üretim arşiv/Logs/CSV kabulü değildir. T8.2 rigid tracking, T8.3 Shape Match ve T8.4 araç/UI/Runtime bağlantıları ayrı işlerdir.

## Teaching ve arama

`PoseMatchService::teach(image, modelRoi, modelLocalAnchor, parameters)` Gray8 görüntüden PNG model üretir. Anchor model-local piksel merkezidir; merkez olması gerekmez ve subpixel olabilir. Öğretimdeki raw anchor = modelRoi.topLeft + localAnchor. Adaydaki raw anchor ise similarity dönüşümüyle hesaplanır; döndürülmüş bounding box merkezi değildir. ROI/PNG boyutu ve anchor sınırları çalıştırmada yeniden doğrulanır. Model/anchor/arama/policy değişimi mevcut toolSignature/cache'i değiştirir; Save/reopen/no-op editor7 yolu korunur.

Arama, absolute raw `searchRoi` içinde çalışır; boş ROI bütün görüntüdür. Renk/Gray16 girdisi için açık Preprocess → Gray8 gerekir. 1–3 seviyeli image pyramid, her açı/ölçekte en çok sekiz coarse peak, spatial/angular NMS ile en çok128 refinement seed kullanılır. Sonraki seviyelerde aday yakınında translation araması, raw çözünürlükte iki açı/ölçek refinement geçişi uygulanır. Refinement adımı coarse adımın yarısı, sonra dörtte biridir. Sonuçlar sürekli/global optimum veya subpixel translation optimizer iddiası taşımaz; anchor subpixel olsa da template yerleştirmesi integer grid'dir.

Skor `masked-ccoeff-normed-pyramid` v1 yöntemidir; maskeli `TM_CCOEFF_NORMED`, interpolation padding'i dışlayan transformed support ile hesaplanır. Full-resolution raw score kullanılır; finite olmayan/sınır dışı normalizasyon sonuçları aday olamaz. Sabit/çok düşük dokulu model reddedilir. Skor olasılık, fiziksel doğruluk veya genel parça kararı değildir. Görünmeyen/örtülen parça için ayrıca güvenilirlik garantisi yoktur; öğretilmiş mask/brush/result-filter henüz sunulmaz.

Angle +X→+Y yönünde radians; raw Y-down görüntüde pozitifi clockwise'dır. `Pose2D.position` dönüştürülmüş anchor, `Pose2D.angleRadians` gerçek arama açısıdır. `scale` ayrı ratio portudur; rigid Transform2D/TrackingRoi içine gizlice katılmaz. Kalibrasyon/world açı veya robot pose üretilmez.

## Aday ve seçim politikası

`minScore` dahil eşiktir. NMS, anchor aralığı `nmsDistance` içinde ve angular uzaklığı clamp(1.5*angleStep,12°,20°) içinde olan adayları tek hipotez sayar; scale aynı nesne hipotezinin parçasıdır, ayrı nesne kimliği değildir. Sıralama skora göre stable'dır; eşit skorda deterministik arama sırası kullanılır. Aday listesi bütün nesnelerin kesin sayısı değildir; coarse peak/seed bütçeleri, yakın/nested nesneler ve pyramid aliasing recall'ı sınırlayabilir.

NMS sonrası en iyi iki qualifying adayın skor farkı `ambiguityGap` dahilindeyse `ambiguous=true`. 90°/180° simetri adayları aynı konumda olsa da angular NMS dışında tutulur. Listedeki qualifying adaylar `maxCandidates` sınırını aşarsa `truncated=true` ve konservatif ambiguity vardır. `truncated`, final listenin sınırını bildirir; coarse search'ün bütün olası hipotezleri taradığını kanıtlamaz.

Varsayılan `selection=reject_ambiguous`: ambiguity halinde seçili pose/scale/score invalid, overlay boş. `selection=best` açıkça etkinleştirilirse en güçlü aday seçilir; ambiguity/truncated kanıtı korunur. `found` en az bir qualifying aday bulunduğunu bildirir; selected geçerliliğiyle aynı değildir. Aday yokken found=false, ambiguity=false, pose/scale/score invalid; sahte sıfır veya PASS yoktur.

| Port | Tip/birim | Anlam |
| --- | --- | --- |
| image | Image/px/image | Tek gerekli giriş |
| pose | Pose2D/px/image | Yalnız policy seçimi geçerliyken |
| scale, score | Number/ratio/image | Seçili adayın ayrı ölçek ve yöntem skoru |
| found, ambiguous, truncated | Boolean/image | Arama ve seçim tanıları |
| candidates | Text/image | Bounded JSON list: x/y/angleRadians/scale/score |

Typed envelope method/version/evaluation/acquisition frame/cycle/source/coordinates taşır. Engine evaluation kimliğini atar; service farklı bir evaluation üretmez. ERROR/SKIPPED/Stop mevcut merkezi temizleme yolunu kullanır. PartDecision NOT_EVALUATED kalır. Candidates metni snapshot güvenilir completion kanıtı değildir; Runtime açılmadan önce explicit replay validator gerekir.

## Sınırlar ve kabul

Raw görüntü ≤16 Mi piksel; model16–256px/eksen; PNG≤4MiB base64. Açı−180…180°, coarse step1…90°; scale .5…2, step .02….5. En çok37 angle ×17 scale ve toplam255 kombinasyon;1–3 pyramid seviyesi, final2–32 aday, NMS2–1024px. İptal her template çağrısı/peak/refinement arasında denetlenir; OpenCV çağrısına hard timeout yoktur. Bütçe sınırlıdır ancak latency hedefi veya CUDA kabulü yapılmadı.

`pose_match_contract` bağımsız inverse rasterizer ile −27.5/0/+27.5° ve .85/1/1.15 scale, merkez dışında(9.25,28.5) anchor, absolute ROI, pyramid1/2/3, tekrar/simetri, bounded liste ve sabit görüntü sınar. Oracle kapıları1.5px, .05rad, .045scale; fiziksel hassasiyet kabulü değildir. Application typed codec, invalid output, üç applicationType gerçek DAG/Save/reopen/no-op/TargetOnly/cache/Stop ve explicit Runtime reddi sınanır. [Derleme ve loglar](../toolchain.md), [oturum kaydı](../progress.md).

T8.2 ortak rigid adapter ve T8.4.1 shape/texture referans sözleşmesi tamamlandı; [zincir ve sınırlar](pose-roi.md). T8.3 [Shape Match çekirdeği](shape-pose.md) tamamlandı; sonraki T8.4.2 bütün tool/DAG/UI/world sampler/Runtime bağlantılarıdır. T8.4 bütün olarak açık kalır.
