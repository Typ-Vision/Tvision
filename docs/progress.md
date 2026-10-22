# VisionFlow — güncel devir

22 Ekim 2026. [Arşiv](archive/README.md).

## Devam noktası

**Sonraki görev: [CCUX-16](architecture/camera-setup/16-automatic-acquisition.md).**
**CCUX-PLAN-03 hazır, uygulanmadı:** OECS seçenek/uyarıları ve Kalibrasyon eski
gezinmesi kalkacak; normal komutlar edinim yolunu seçecek. Sıra **16 → 17 → 18 →
19 → CCUX-15**. [Plan/C0](architecture/camera-setup/README.md) eski opt-in/uyarı
gereğini değiştirir; teknik engeller korunur. [Kapanış](archive/2026-10-22-ccux-plan-03.md).
**CCUX-10b:** Editor kamera seçimi sırasında seçicinin silinmesi düzeltildi.
Klavye olayı regresyonu ve kanıtlar [kapanışta](archive/2026-10-22-ccux-10b.md).
**CCUX-03b:** Opto ayar servisi edinimden ayrıldı. Gerçek cihazda kayıt/oku/mevcut
ayar kaydı doğrulandı; görüntü/mm koşumu değildir.
[Kanıt](archive/2026-10-22-ccux-03b.md), `build/ccux/ccux-03b/`.
**CCUX-15a plakasız kontroller:** yeni gerçek readback/kayıt, 60 s / 558 kare /
9,36 FPS preview, Stop/reopen, kullanıcı bildirimli USB kopma/takma, Editor 3
kare ve Runtime 10 Raw/px çevrim/CSV doğrulandı. Exposure/gain değişip geri
yüklendi: son 6000 µs / 0 dB. Yanlış seri/geometri reddedildi. Kopmada SDK stop
temizleme hatası; 7000 µs / 1 dB isteğinde readback reddi kaydedildi.
**CCUX-15 tamamlanmadı:** plaka/referans/kurulum hazır değil; ışık/odak,
tolerans/mm ve henüz denenmeyen kullanıcı işlem kabulü açık. Yeni UI paketinden
sonra bu maddelere dönülecek. [Yeni kanıt/kapanış](archive/2026-10-22-ccux-15a.md),
`build/ccux/ccux-15/no-plate-20261022/`; [kullanıcı teyidi](archive/2026-10-22-ccux-15-teyit.md).

**CCUX-14 tamamlandı:** 1/5 kamera fixture'ı, iki yöntem, üç applicationType,
Editor → Runtime → Logs/CSV ve Raw/mm negatifleri. UI 42/43, düzeltme sonrası
etkilenen 7/7; core-off 33/33. Tam suite/fiziksel kabul değildir. İki boyut görseli
incelendi. Stop/retry engeli kapandı: kayıt kalıcılaşır, Stop sonrası sonuç dönmez.
[Matris ve kanıt](archive/2026-10-22-ccux-14.md), `build/ccux/ccux-14/`.

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
  [kanıt](archive/2026-10-21-icm-oecs-exp-02.md). Deneysel kalibrasyon/aktivasyon
  ve bağımsız mm referansı açık; CCUX-07–13 bu açığı devralır.
- **EXP-01 görüntü/Stop/PGM hazır, fiziksel kabul kısmi**;
  [odak/100 seri](archive/2026-10-21-icm-oecs-exp-01d.md).
  Optik/mm kabulü yok. MUI-01–03, EUI-01–08 tamam; **MUI-04 sentetik kabulü
  tamam, özgün gerçek raster/reçete/ROI problemi açık**.
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

UI: build/reference-ui/visionflow.exe; core: build/kmc9-core-off.
PATH/build/test/başlatma README'de; mevcut UI kullanımı
[kullanım rehberinde](camera-runtime-usage.md). Her kartta yalnız değişen kapsamı
kanıtla. CCUX-03 fiziksel kamera, native DPI veya kurulum kabulü değildir.
Detay ve kanıt tarihli kapanıştadır.
