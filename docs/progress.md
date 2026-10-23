# VisionFlow — güncel devir

23 Ekim 2026. [Arşiv](archive/README.md).

## Devam noktası

**[PLAN-05](archive/2026-10-23-ccux-plan-05.md):** Plaka planı hazır, kod değişmedi.
Başlangıç [15e](architecture/camera-setup/15-plate-calibration-plan.md); lens kapalı.

**[CCUX-22e yazılımı tamamlandı](archive/2026-10-23-ccux-22e.md):** Kalibrasyon
bağlantı ömrü, dosya/iptal/SaveUse drain. Kanıt: `build/ccux/ccux-22e/`.
SDK kayıtlı kamerayı bulamadı; kare/mm kabulü yok. 22f açık.
[22d Runtime](archive/2026-10-23-ccux-22d.md), [22c Editor](archive/2026-10-23-ccux-22c.md),
[API/ömür](architecture/camera-setup/acquisition-sessions.md).
Ek `multi_source_production_contract` önceki hatası 22d kapanışında korunur.
**[22a ölçümü](archive/2026-10-23-ccux-22a.md):** yazılım tamam; fiziksel baseline/paint açık.
**[21 ROI tamamlandı](archive/2026-10-22-ccux-21.md)**.
CCUX-20 canlı teyit ve CCUX-15 fiziksel kabul açık; hash/preview korunur.
[CCUX-20](archive/2026-10-22-ccux-20.md), `build/ccux/ccux-20/`.
Geçici viewer geometrisi ayrı adlandırılır; araç ROI'leri reçeteye bağlıdır.
CCUX-16–19 teslimleri [arşivde](archive/README.md); C0 teknik/fiziksel engeller açık.

**CCUX-15 plaka denemesi açık:** 7×7 simetrik daire, kullanıcı beyanı 12 mm.
56000 µs / 0 dB ham kare: 48 aday ret, tanıdaki 49 da yanlış grid. Model/mm yok.
[CCUX-15c](archive/2026-10-22-ccux-15c.md), `build/ccux/ccux-15/plate-20261022-02/`.
[Önceki kare](archive/2026-10-22-ccux-15b.md).
**CCUX-10b:** [Editor seçici/klavye düzeltmesi](archive/2026-10-22-ccux-10b.md).
**CCUX-03b:** [Ayrı Opto ayar servisi ve fiziksel readback](archive/2026-10-22-ccux-03b.md);
görüntü/mm kabulü değildir.
**CCUX-15a önceki plakasız kontroller:** 60 s / 558 kare / 9,36 FPS, Stop/reopen,
USB, Editor 3 ve Runtime 10 Raw/px çevrim/CSV. O koşum sonu 6000 µs / 0 dB;
yanlış seri/geometri reddi var. SDK kopma stop temizleme hatası ve 7000 µs / 1 dB
readback reddi açık. [Kanıt](archive/2026-10-22-ccux-15a.md),
`build/ccux/ccux-15/no-plate-20261022/`. **CCUX-15 tamamlanmadı:** lens/düzlem,
ışık/odak, bağımsız referansın ölçü/kökeni, tolerans/mm ve kullanıcı işlem kabulü
açık. [Önceki teyit](archive/2026-10-22-ccux-15-teyit.md) tarihsel durumdur.

**[CCUX-14 tamamlandı](archive/2026-10-22-ccux-14.md):** 1/5 kamera, üç tür,
Stop/retry yazılım/görsel kabulü; fiziksel açık. `build/ccux/ccux-14/`.

CCUX-01–13 tamamlandı; ayrıntılı kayıtlar [arşiv dizinindedir](archive/README.md).
- Kamera immutable ID/kısa ID ve atomik varsayılan kurulum/profil; belirsiz eski
  profil/rota açık seçim ister. Exposure/gain gerçek readback ile kaydedilir.
  Pending optik kurulum ölçüm izni değildir; Opto/OECS ayar servisi edinimden ayrıdır.
- Tek/canlı preview ortak worker/session, bir pending istek/latest kare,
  generation/Stop/drain ve gerçek FPS kullanır; deneysel OECS sıralı
  FrameBurstStart continuous diye sunulmaz. Sayfa/proje/pencere geçişi drain bekler.
- Kalibrasyon kayıtlı matematik, uyum ve oturum iznini ayırır; restart/replug izin
  başlatmaz. Exact yeniden probe gerekir. Optik değişikliği pending revision
  üretir; A→B→A izni canlandırmaz. Tek Kaydet ve kullan önceki seçimi hata/rette korur.
- Deneysel canlı/PGM+JSON → aday → ayrı etkin seçim → Editor/Runtime mm zinciri
  typed köken/hash ve exact kamera/geometri/model ID/content/version taşır.
  Trusted active/system/completion'a yükselmez; Raw/px dalı global model almaz.
  [Son servis/UI sözleşme teslimi](archive/2026-10-22-ccux-13.md).
- Editor kamera seçimi varsayılan bağları atomik kurar; aynı kameranın uygun ayar
  güncellemesi Run Flow hazırlığında serialized snapshot/DRAFT/dirty yeniler.
  Geometri/seri/legacy otomatik onarılmaz. Yeni reçete genel referans miras almaz;
  üç tür boş kaynakla DRAFT; metadata edit legacy/scoped bağ ve no-op hash korur.
- Runtime aktif immutable snapshot'ı kullanır; model/ayar değişimi Stop ve yeniden
  Load ister. Deneysel runtime1/2 Logs/CSV birimleri/kökeni korur; hata/Stop sonuç
  üretmez, restart geçmişi okur. PLC/robot kapalıdır.

Yeni raster karanlık/kısmen bulanık; **bağımsız fiziksel mm kabulü açık**.
CCUX-14 fixture başarısı bunu kapatmaz. [CCUX planı](architecture/camera-setup/README.md)
sırasıyla sürer; **Hand-eye kapsam dışı.** Tamamlanmış KMC/ICM/EXP işleri yeniden başlatılmaz.

## Hazır olanlar

- **EXP-03 Raw/px deneysel Runtime/Logs/CSV, 500 seri hazır**;
  [teslim](archive/2026-10-21-icm-oecs-exp-03.md),
  [kopma/manuel açma](archive/2026-10-21-icm-oecs-exp-03b.md).
  Eski 21 CTest ve fiziksel koşum yeni test değildir. Uzun probe sonunda zorunlu
  sonlandırma sınırı var. PLC/robot kapalı; restart deneysel onayı kapatır.
- **EXP-02 canlı Editor + PGM/JSON px replay hazır**;
  [kanıt](archive/2026-10-21-icm-oecs-exp-02.md). Kalibrasyon zinciri CCUX-07–13'te.
- **EXP-01 görüntü/Stop/PGM hazır, fiziksel kabul kısmi**;
  [kanıt](archive/2026-10-21-icm-oecs-exp-01d.md). Optik/mm kabulü yok.
  MUI-01–03/EUI-01–08 tamam; **MUI-04 özgün raster/reçete/ROI problemi açık**.
- KMC-0–9/10a–e ve ICM ortak yazılım/G2 tamam: reçetesiz N kamera, bağımsız
  kalibrasyon, kaynak başına immutable model, çok kaynaklı Editor/Runtime.
  [G2 raporu](architecture/industrial-camera-release-acceptance.md).
  Tamamlanmış altyapıyı yeniden başlatmayın. Geçmiş teslim/kanıt ayrıntıları
  [önceki devir kaydında](archive/2026-10-21-ccux-plan-02-oncesi.md).

## Korunacak engeller ve kararlar

- **OECS native Unavailable**. ICM-04d gecikmiş command, Clear→Stop→Start sonrası
  yeni tetik olmadan **8/8 eski kare** üretti;
  [kanıt](archive/2026-10-18-icm-04d-pending.md),
  [korelasyon sözleşmesi](architecture/industrial-camera-trigger-correlation.md).
  Belgeli cancel/completion/drain/reset eksik; delay=0/close-open/sessizlik kanıt
  değildir. Native 04e/G1 kapalı; deneysel kullanım production izni oluşturmaz.
- Belgeli bariyer + tek bekleyen tetik alternatif stratejidir. FrameBurstStart
  sessiz FrameStart alias'ı olmaz. **İlk satırı ölçümden çıkarma izni kalıcıdır:**
  2448×2047 origin (0,1); dönüşüm servisi var, canlı ürün bağlantısı açık.
  Ölçüm adayı configure girdisi değildir. Mevcut canlı raster 2448×2048 Mono8.
- Cihaz: Opto Engineering Srl / COE-050-M-USB-050-IR-C / **02A2220000627**.
  Device ID=1 seri değildir. Lens tam modeli/kurulum eksik. Sandbox içi keşfin 0
  sonucu cihaz yokluğu değildir; sandbox dışı C API keşif/open/readback/grab var.
  [Tanı/başlatma rehberi](windows-camera-diagnostics.md).
- ICM-01b, native G0/G1/G3/G4 ve H01–H08 açık; Basler/Daheng cihaz/model/SDK
  kanıtı bekler. [09b](archive/2026-10-18-icm-09b-start.md).
  Test adaptörü normal ürüne açılmaz. G2 yazılım; G4 kurulum kabulüdür.
- Kaynak/model invalidation, immutable snapshot ve legacy no-op/hash korunur.
  Rename farklı, fiziksel/geometri değişimi farklıdır; A→B→A eski izni canlandırmaz.
  Restart kayıtları korur, aktif üretimi başlatmaz. Gerçek sonuç olmadan PASS,
  skor veya robot pose yok. Şemalar [baseline](architecture/industrial-camera-baseline.md)
  ve güncel kaynakla doğrulanır; tarihsel sürüm sayısı kopyalanmaz.
- Manuel çok kaynak/fiziksel ölçüm, özgün MUI-04, KUI-6/T8.4.2b.3c,
  robot/PLC fiziksel kabulü, lens, native DPI/çoklu monitör, CUDA ve dağıtım açık.
  [Cancel incelemesi](archive/2026-10-18-icm-04d-cancel-audit.md): AcquisitionAbort
  yok; DeviceReset garantisiz ve çalıştırılmadı; üretici soruları gönderilmedi.

## Çalışma ve kapanış

Komutlar README; UI [kullanım rehberi](camera-runtime-usage.md); kanıtlar arşivde.
