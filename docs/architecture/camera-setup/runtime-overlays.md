# Runtime overlay veri sözleşmesi — CCUX-24H

26 Ekim 2026. `ToolResult::overlayGeometry`, yürütmenin geçici sunum kanıtıdır;
disk şeması, reçete hash'i veya üretim completion yetkisi değildir.

ExecutionService, aynı çağrıda referenceBinding/poseTracking çözümünden aldığı
`effective`, `resolvedRoi` ve `WorldTrackingMap` ile `toolOverlayGeometry`
üretir. Nominal dünya ROI'si doğrulanmış nominalToRaw ile; kalibre sabit Circle
ROI'si CircleToolService::mapPoint ile raw'a çevrilir. Düz pixel ROI doğrudan
aynı uzaydadır. Sınır/dönüşüm hatasında tüm tarama çizgileri kaldırılır ve neden
kaydedilir. Çalıştırılmamış/başarısız araç için nominal fallback yapılmaz.
Başarılı Blob/Code araması executor'ın gerçek searchRegion veya çözülmüş ROI'sini
kullanır. Error/Stop sunumu sonuç veya tarama başarısı uydurmaz.

CircleToolService raw `points/rejectedPoints/lines`, MetrologyService raw
`points/lines`, legacy Edge/Measurement raw noktaları ve ölçüm çizgisi taşır.
Blob RegionMask/searchRegion/regions, CodeRead CodeResults köşeleri raw
geometridedir. WorldTrackingService gözlemleri raw; `lineMm/circleMm/arcMm`
gerçek fitleri dünya uzayındadır. Bunların sonuç eğrileri aynı yürütme
kalibrasyonunun world→raw dönüşümüyle `resultLines` içine örneklenir; nominal
ROI dönüşümü sonuç fitine yanlışlıkla tekrar uygulanmaz.

| Mevcut veri | Koleksiyondaki karşılık |
| --- | --- |
| CycleResult in-process completion snapshot + configId/recipeId | Exact immutable yükleme denetimi |
| raw ImageFrame metadata + size | sourceId/frameId/cycleId/coordinateSystemId/rawSize |
| ToolResult regions/points/rejectedPoints/lines | Ayrı araç sonuç ilkel grupları |
| RegionMask / codes typed çıktısı | Maske / kod köşeleri |
| overlayGeometry outlines/guides | Aynı çevrimde çözülmüş raw tarama alanı/yönü |
| Tool ID/name/state/message | Kararlı SHA256 tabanlı renk, kısa etiket, tooltip |

Sunum anahtarı snapshot validation ID/dependency ID ile source/frame/cycle/uzay
ve raw boyutunu taşır. RuntimeService::presentationMatches, completion
içindeki gerçek in-process snapshot nesnesini karşılaştırır; eş içerikli başka
yükleme veya disk kaydı bu bağı üretemez. `runtimeOverlays` yalnız aynı
snapshot/config/source/frame/cycle/uzaydan DONE sonuçları
alır. Geçerli portların envelope'ları ve overlay geometrisi ayrıca denetlenir.
Ortak görüntü raw'dır; mevcut preprocess yalnız aynı raster geometrisini
koruyan fotometrik işlemlerdir. Geometriyi değiştiren gelecekteki preview
doğrulanmış dönüşüm olmadan bu sözleşmeye alınamaz. Seçili araç modu aynı raw
koordinatlarıyla aynı boyutlu preview'ı tek UI çağrısında değiştirir.

ImageViewer::setRuntimeOverlays bir salt okunur koleksiyonu tek çağrıda
değiştirir; roi_ ve düzenleme callback'lerini paylaşmaz. Nokta grupları araçlar
arasında bağlanmaz. Measurement'ın iki noktası yalnız kendi ölçüm çizgisidir.
Tarama kesikli, sonuç düz, ret noktaları kırmızıdır. Tool seçimi kenar profilini
ve ayrıntıları yönetir. Tüm araçlar tercihi yerel INI'de
`runtime/allToolOverlays` altında, varsayılan açık saklanır. Acquisition veya
execution başlatmaz. Yeni çevrim/Stop/Unload/proje/snapshot kaybı mevcut
showRuntimeResult/clearImage yaşam döngüsünde koleksiyonu da kaldırır.
