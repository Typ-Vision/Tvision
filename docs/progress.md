# VisionFlow — güncel devir

[Arşiv](archive/README.md). [MCP](archive/2026-10-24-mcp-01.md).

## Devam noktası

**[CCUX-24B tamam](archive/2026-10-26-ccux-24b-completion.md):** Kalıcı sağ Flow,
sade sonuçlar ve metrologi Properties; Release güncel, yeni CTest 4/4,
1400×900/850×600 görsel kontrolü. Sonraki: [24C](architecture/camera-setup/24c-calibration-status-circle.md).
Sıra yalnız 24B→24C; 24D yapılmayacak.
[Plan](architecture/camera-setup/24-usability-plan.md): Kalibre Et korunur; Robot pasif.
24C: yalnız yeni Circle calibrated; kullanıcı Circle hata bildirimini geri çekti.

**[INT-09 tamam](archive/2026-10-25-int-09.md):** Yerel CameraIntegration ve
başlatıcı eşleşti; push/main değişmedi. [Kullanım/restart devri](architecture/camera-integration-handoff.md).
[INT-08 önceki test/600 ölçüm kanıtı](archive/2026-10-25-int-08.md) tekrarlanmadı;
üç medyan aşımı ve fiziksel E2/mm/native açık; gerçek proje/reçete/kaynak/model gerekli.
[Snapshot/bundle](archive/2026-10-25-int-01.md) korunur.

**[CCUX-23b-1–6 yazılım kabulü tamamlandı](archive/2026-10-24-ccux-23b-6.md).**
Canlı proje/2448×2048 kabulü açık; fixture fiziksel kabul değildir.
Kullanıcı plaka kalibrasyonunu başardı; bağımsız mm kabulü ayrıdır.

**[CCUX-15j fiziksel kabul açık](archive/2026-10-23-ccux-15j.md):**
Exact SDK raw/sidecar: 56000 µs/0 dB kare karanlık/odak dışı,
0 merkez; model yok. Kanıt `build/ccux/ccux-15j/`.
Devam: ışık/odak/düzlem; referans/tolerans ve operatör kabulü.
[15i yazılım kabulü](archive/2026-10-23-ccux-15i.md) korunur;
`multi_source_production_contract`: Missing result açık.
[15f](archive/2026-10-23-ccux-15f.md): eski kare 50→49; fiziksel eksen/mm açık.

**[CCUX-22e yazılımı tamamlandı](archive/2026-10-23-ccux-22e.md):** Kalibrasyon
bağlantı ömrü, dosya/iptal/SaveUse drain. Kanıt: `build/ccux/ccux-22e/`.
SDK kayıtlı kamerayı bulamadı; kare/mm kabulü yok. 22f açık.
[22d Runtime](archive/2026-10-23-ccux-22d.md), [22c Editor](archive/2026-10-23-ccux-22c.md),
[API/ömür](architecture/camera-setup/acquisition-sessions.md).
Ek `multi_source_production_contract` önceki hatası 22d kapanışında korunur.
**[22a](archive/2026-10-23-ccux-22a.md):** yazılım tamam; fiziksel baseline/paint açık.
**[21 ROI](archive/2026-10-22-ccux-21.md) tamam.**
[20 hash/preview](archive/2026-10-22-ccux-20.md) korunur; canlı teyit açık.
Geçici viewer geometrisi ayrı adlandırılır; araç ROI'leri reçeteye bağlıdır.
CCUX-16–19 [arşivde](archive/README.md); C0 açık.

**CCUX-15 fiziksel açık:** 7×7, 12 mm kullanıcı beyanı; [15c tarihsel kanıtı](archive/2026-10-22-ccux-15c.md); güncel başarı yukarıda.
**CCUX-10b:** [Editor seçici/klavye düzeltmesi](archive/2026-10-22-ccux-10b.md).
**CCUX-03b:** [Ayrı Opto ayar servisi ve fiziksel readback](archive/2026-10-22-ccux-03b.md);
görüntü/mm kabulü değildir.
**[15a tarihsel plakasız kanıt](archive/2026-10-22-ccux-15a.md):** SDK kopma/Stop
temizleme hatası ve 7000 µs / 1 dB readback reddi açık. CCUX-15 bağımsız referans,
tolerans/mm, lens/düzlem ve belgeli fiziksel kabulü tamamlanmadı.

**[CCUX-14 tamamlandı](archive/2026-10-22-ccux-14.md):** 1/5 kamera, üç tür,
Stop/retry yazılım/görsel kabulü; fiziksel açık. `build/ccux/ccux-14/`.

CCUX-01–13 tamamlandı; [arşiv](archive/README.md).
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

Arşiv rasteri karanlık/kısmen bulanık; **bağımsız fiziksel mm kabulü açık**.
CCUX-14 fixture bunu kapatmaz. [CCUX planı](architecture/camera-setup/README.md) sürer;
**Hand-eye kapsam dışı.** KMC/ICM/EXP yeniden başlatılmaz.

## Hazır olanlar

- **EXP-01–03 hazır:** [son devir ve önceki kanıtlar](archive/2026-10-21-icm-oecs-exp-03b.md).
  Uzun probe zorunlu sonlandırma sınırı; PLC/robot kapalı, restart izni kapatır.
  Fiziksel/optik/mm kabulü açık. MUI-01–03/EUI-01–08 tamam;
  **MUI-04 özgün raster/reçete/ROI problemi açık**. Eski testler yeni kabul değildir.
- KMC/ICM/G2 yazılımı tamam: [önceki devir](archive/2026-10-21-ccux-plan-02-oncesi.md).

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
