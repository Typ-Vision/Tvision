# R7 — Ortak Referans production sözleşmesi

12 Ekim 2026. R7a immutable production/trusted completion ve R7b kayıt/restart/Logs/CSV uygulaması. R8 nihai birleşik paket kabulü tamamlandı; [tam koşum ve devir](reference-release-acceptance.md). T8.4.2b.3c kalan corpus/production kabulü ayrı adımdır. Fiziksel ölçüm, robot pose, PLC restart recovery veya dağıtım kabulü değildir.

## Snapshot ve kabul kapısı

Yeni `referenceBinding` kullanan yedi raw ve dört world tüketicisi mevcut ProductionRecipeService/Runtime üzerinden çalışır. Registry/DAG doğrulamasına ek olarak producer port/geometri kimliği, acquisition kimliği, nominal ROI ve world exact calibration binding denetlenir. Pattern/Shape yalnız enabled yeni Referans tüketicisinin producer'ı olarak production kapsamına alınır. Standalone matcher production ve eski `poseTracking` production corpus kapısı T8.4.2b.3c'ye kadar kapalıdır; eski reçeteler Editor'da okunur ve no-op Save korunur.

Reçetenin tamamı contentId içinde; tam binding (sourceTool/sourcePort, taught anchor, nominal ROI, X/Y/açı ofsetleri, source/resolution, calibration/mmPerSample) ayrıca snapshot `dependencies.referenceBindings` içinde bulunur. Pattern/Shape model/anchor geometryId ve tool/backend/engine bağımlılıkları dependencyId'ye girer. Engine `visionflow.execution/22`; editor8, raw binding1/world binding2, root1/recipe1–2 ve vf.socket/2 korunur.

## Trusted completion

`RuntimeService::execute` immutable ProductionCycle snapshot'ından mevcut ExecutionService DAG'ını çalıştırır. World yolunda recipe/catalog'a bağlı CalibrationWorkflow resolver kullanılır; UI tarafından sonuç veya mapping verilmez. ExecutionService'in aynı ReferenceService/WorldTrackingService yolu gerçek sampling/refit/tolerans üretir.

Worker döndüğünde CycleResult'ın private, yalnız Runtime'ın üretebildiği in-process capability'si snapshot nesnesi ve cycle ticket'ını taşır. SHA256 özeti cycle/evaluation/config, state/error/süre, bütün declared output'lar, frame ve acquisition zaman metadata'sı, owned image content/processing kimlikleri, calibration, mask, ROI/overlay ve profil/aday tanılarını kapsar. Production completion aynı capability ve özeti yeniden doğrular. Typed variant/port/frame/karar/tolerans kapıları ayrıca korunur. Yeni bağlı Circle'da nominal ROI replay yerine gerçek resolver/executor çıktısını kapsayan bu kanıt kullanılır; sabit/legacy Circle replay değişmez.

Editor run/partial, elle kurulmuş sonuç, başka snapshot/ticket veya sonradan değişmiş source/point/number/Boolean/image/provenance/overlay sonuçları kayıt yetkisi vermez. Capability diske yazılmaz; arşiv hash'i veya disk VALID çalışma izni olamaz. Bu mekanizma süreç içindeki uygulama sahipliğidir; dijital imza veya bağımsız fiziksel gözlem değildir.

Worker sonrasında gelen Stop yalnız değiştirilmemiş doğru lease sonucunu `RuntimeService::suppressCancelled` üzerinden temizleyip yeniden mühürleyebilir. Manual ve PLC callback'leri aynı yolu kullanır. Outputs/frames/mask/overlay/decision temizlenir; önceki gerçek sonuç yayımlanmaz. Kayıt kilidi/hatası lease'i tutar; aynı sonuçla retry tek atomik terminal record üretir.

## Kalıcı kanıt ve Viewer

Yeni Referans içeren cycle terminal schema4'tür. `referenceBindings` snapshot dependencies ile aynı tam JSON'u taşır; configId/dependencyId/validationId ve gerçek typed sonuçlar aynı record içindedir. Schema1–3 okuyucusu korunur; Referans olmayan yeni kayıt schema3 kalır. Restart yalnız kanıtı açar, aktif snapshot/izin geri yüklemez. Logs özeti kaynak tool/port ve X/Y px/açı radyan verir. CSV eski sütunlara configId, validationId, dependencyId ve tam referenceBinding JSON sütunlarını ekler. Socket şeması değişmez; P7/6D.2 outbox/recovery yapılmaz.

Runtime Viewer yeni binding'de yalnız bu çevrimin tam producer sonucu ve immutable recipe ile current raw veya mapped world ROI gösterir. Missing/error/Stop halinde nominal ROI fallback yoktur; ROI salt okunurdur. Mapped world scan oku raw görüntüde 8 px çizilir; nominal mm cinsinden 8 birimlik dekorasyon sampler sınırına taşınmaz. UI algoritma veya donanım SDK'sı çağırmaz.

## Kabul eşlemesi

- `reference_execution_contract`: mevcut Template Point / Pattern ve Shape Pose × üç applicationType gerçek rasterları üzerinden altı raw tüketici ve dört world tüketicisinde gerçek dosya edinimli Runtime; bağımsız Edge/width/current-world-circle oracle'ları. Y-up world fixture catalog-backed gerçek resolver kullanır. Önceki non-affine H/every-tap corpus `reference_world_contract` içinde korunur; R7 production H corpus kapanışı diye sunulmaz.
- `reference_inspection_contract`: aynı üç producer × üç applicationType, gerçek Blob hole/area/centroid ve ZXing ON'da bağımsız QR decode; Runtime + aynı kayıt helper'ı. OFF'ta capability retleri korunur.
- `reference_production_support.hpp`: Editor completion ret, source Point/Number/Boolean tamper, metadata/overlay/state/partial ret, immutable ofset edit, duplicate/foreign-ticket ret, kilit/retry/tek kayıt, exact binding/restart/Logs/CSV, not-found downstream SKIPPED, acquisition ERROR, worker ve geç Stop. Helper kontrolleri önceki test sayaçlarına eklenmez.
- `ui_pose_tracking_smoke`: gerçek raw/world Load Recipe → Run → Tek çevrim → Stop → Logs, current readonly Runtime ROI, schema4/restart CSV, normal1400×900/küçük850×600 Logs görselleri; mevcut Editor/U1–U4 regresyonu sürer.
- `production_recipe_contract`, `runtime_persistence`, `typed_terminal_trace`, migration/target-only, eski tracking DAG, Circle/Geometry UI ve localhost PLC/shell regresyonları mevcut kapıları korur.

Tam komutlar, son sonuçlar, görsel artifact'lar ve kalan sınırlar [ilerleme kaydındadır](../progress.md). R8 tamamlandı; sıradaki T8.4.2b.3c eksik corpus/production kabulüdür.
