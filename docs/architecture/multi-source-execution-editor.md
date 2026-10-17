# KMC-10b — Çok kaynaklı execution ve Editor

17 Ekim KMC-10c2/10d güncellemesi: [kalıcı Runtime, terminal7 ve N-kamera UI](multi-source-runtime-terminal.md) uygulandı. Aşağıdaki kapalı production/yalnız bellek ifadeleri bu belgenin önceki paket kapanış durumudur; Runtime editor9 bütün-set provider ile açılır. [Birleşik kabul/devir](multi-source-release-acceptance.md).

16 Ekim 2026. Explicit editor9 reçeteleri ortak Editor'da çalışır. ProductionRecipeService editor9'u KMC-10c2 kabulüne kadar reddeder; Runtime, trusted completion, terminal6 ve socket üretim yolu açılmadı.

## Kullanım

1. Recipe Editor → **Çok kaynaklı Editor** reçeteyi açıkça editor9'a geçirir. Normal açma/Save ve Add Tool eski reçeteyi otomatik taşımaz. Migration araçları ve bağlantıları korur; reçete DRAFT olur.
2. Toolbox'tan N Image File veya Webcam ekleyin. Görüntü kaynağını seçip **Open Image** ile yalnız o kaynağın dosyasını atayın. Properties görüntü girişlerini aynı kaynak dalı içinde bağlayın. Çapraz kaynak geometri/scalar birleşimi reddedilir.
3. Kaynağın Properties → **Ölçüm kurulumu / Kurulumu bağla** ile o kaynağa exact kurulum seçin. Bağımsız Calibration sistem özetinde hazır/etkin model gerekir. Bağsız dal raw px çalışabilir; mm sonucu veya global model fallback'i üretmez.
4. **Run Flow** bütün enabled kaynakları sıralı edinir. Viewer üzerindeki ayrı kaynak seçici doğru ham kareyi gösterir; tool seçimi kendi upstream kaynağına geçer. Preprocess kendi source/frame/cycle kimliğini korur. Seçili tool'un gerçek overlay'i yalnız o kaynak üzerinde gösterilir.
5. Kaynakta **Görüntü al / Reload** yalnız o kaynağı yeniler ve kendi downstream cache'ini temizler. Diğer kaynakların kareleri ve cache'leri korunur. Hedefli edinim kısmi değerlendirme olarak işaretlenir; sonuçlarda her tool için source/frame/cycle kimlikleri görünür. **Run Tool** mevcut dal karesi/cache'i üzerinde yalnız hedefi değerlendirir; acquisition sayısı sıfırdır. Güncel upstream yoksa SKIPPED/hata olur, gizli edinim yapılmaz.

## Definition ve kalibrasyon

`sourceSetups` yalnız editor9'da, source UUID → mevcut sourceSetup schema1 binding nesnesi haritasıdır. Anahtar ve `sourceToolId` aynı olmalıdır; en çok 256 kayıt, mevcut acquisition tool zorunludur. Aynı kaynak singular `sourceSetup` ve haritada iki kez bulunamaz. Legacy singular bağ explicit migration sırasında korunur; sonraki açık bağlama onu haritaya alır. Unbind yalnız ilgili kaydı kaldırır; scoped politika fallback sağlamaz. Silinen source'un kaydı temizlenir. Root project1 / recipe1–2 ve eski no-op codec yolları korunur.

`SystemCalibration::branch` tek kaynağın araç/bağlarını ve exact kurulumunu türetir. UI owner thread kaynak başına immutable model ve acquisition tanımını yakalar; worker `CalibrationResolver(ImageFrame)` ile frame.sourceId'ye göre o modeli kullanır. Başka kaynak/model ve actual çözünürlük değişimi mevcut SystemCalibration kapılarından reddedilir. N snapshot'ın **atomik production admission seti** bu pakette yoktur; KMC-10c1 işidir. Kullanılmayan dalın kalibrasyonu TargetOnly'yi bloke etmez.

EditorSession source başına raw frame ve branch imzası taşır. Kaynak parametresi/dosya içeriği/silme/binding değişimi yalnız bağlı dalları invalid eder. Kalibrasyon değişimi ilgili mm/world araçlarını temizler; raw frame ve px cache korunur. Seçici değişimi cache'i invalid etmez. TargetOnly cached portlarının source/frame/cycle/coordinates ve owned image iç kökeni SourceFrameIndex + ToolResultCodec ile yeniden denetlenir.

## Execution, süre ve bellek

- Her source ayrı frameId, bütün tam akış ortak cycleId ve `<sourceId>/image` koordinatı alır. `CycleResult.frames` editor9'da yalnız raw acquisition kökenlerini içerir; processed frame tekrarları eklenmez. N>1 compatibility frameId boştur. Başarısız çevrimde frames kısmi olabilir; DONE yalnız tam source-frame indeksiyle mümkündür.
- Enabled kaynaklar tek worker'da sıralı edinilir; mevcut WebcamAdapter kilidi korunur. Fiziksel eşzamanlı çekim, kamera senkronizasyonu veya paralel SDK garantisi yoktur.
- `ExecutionLimits`: varsayılan toplam 30000 ms ve 512 MiB konservatif retained image bütçesi. Acquisition timeout kalan toplam süreyle sınırlanır. Her tool öncesi/sonrası ve resolver sonrası toplam süre kontrol edilir; geç sonuç kabul edilmez. Stop değer/overlay/kararları temizler. SDK/OpenCV çağrıları zorla kesilmez; deadline dönüşte geç sonucu reddeder.
- Retained bütçe her image portunun raw stride×height ve preview byte'larını sayar; paylaşılan buffer iki portta konservatif olarak tekrar sayılabilir. Dosya metadata boyutu biliniyorsa decode öncesi 12 B/px üst tahmini, Preprocess öncesi aynı boyut tahmini uygulanır. Bilinmeyen dosya metadata'sı ve boyutu önceden bilinmeyen webcam için gerçek boyut capture sonrası denetlenir. **Bu bütçe process RSS veya algoritma/codec geçici allocation için hard limit değildir.**
- Kısmi kaynak hatasında yalnız bağımsız başarılı dalın gerçek tanısal sonucu kalabilir; cycle ERROR'dur. Genel parça PASS veya aggregate karar üretilmez. Timeout/outer contract hatası bütün çevrim çıktısını bastırır.
- ExecutionService optional application adapter factory deterministic edinim testleri içindir. Normal ürün yolu dosya/Qt webcam adapter'larını kullanır; UI adapter veya algoritma çağırmaz. Production editor9 reddi bu test hook'undan bağımsızdır.

## Kabul ve sınırlar

`multi_source_execution` ve `multi_source_target_only` aynı executable/corpus'u iki CTest adıyla çalıştırır; bağımsız iki corpus değildir. 1/2/8/10 gerçek farklı dosya rasterı, 10 farklı scoped model, üç applicationType, bağımsız mm/tolerans oracle'ları, save/reopen, TargetOnly acquisition0, yabancı raw/cached point kökeni, kaynak yenileme/parametre değişimi/unused offline dal, kalibrasyon invalidation, unbind/no-op/delete, Stop, resolver deadline ve bellek reddi sınanır. Mixed file/webcam ve read sırasında Stop/geç kare optional adapter fixture ile sınanır; fiziksel kamera kabulü değildir.

`ui_multi_source_editor` üç applicationType × 1/2/8/10 kaynak, explicit opt-in, ayrı seçici, gerçek Viewer pikselleri/overlay kaynağı, TargetOnly, bağımsız refresh/delete ve 1400×900/850×600 offscreen görsellerini sınar. Native DPI/çok monitör/gerçek kamera/seri sürekliliği, fiziksel ölçüm/PLC/robot, lens/CUDA, kurulum/dağıtım ve production kabulü açık kalır.

Release/CTest kanıtları [progress](../progress.md) içinde tutulur. Sonraki paket [KMC-10c1](independent-calibration-implementation-plan.md#kmc-10c1--çok-kaynaklı-immutable-runtime-ve-completion-çekirdeği).
