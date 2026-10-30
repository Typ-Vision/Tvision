# VisionFlow — devir

[Arşiv](archive/README.md). [MCP](archive/2026-10-24-mcp-01.md).

## Devam noktası

**[COMM4-10](archive/2026-10-30-comm4-10.md) tamam:** ortak kamera lease.
F ON179/OFF140 geçti; 1'er dış plaka skipped.
Sıradaki **COMM4-11**; main/ledger13 açık.
Probe INI restore SEGFAULT/UI INI birikimi açık; temiz girdide kabul.
[DOC-WORK-02](archive/2026-10-30-doc-work-02.md); ölçüm3/3 erişilemiyor.
[EFR-04](archive/2026-10-29-efr-04.md): Wheel açık, Match/θ ertelendi.

[D:/TVision](archive/2026-10-28-dev-drive-01.md): OpenCV geçti; CTest kısmi.

[Değişken rehberi](variables-usage.md).

**[COMM-37](archive/2026-10-28-comm-37.md):** UI 164/core 127 geçti;
dış plaka skipped. Faz kapandı; cihaz varsa COMM-38.
[36](archive/2026-10-28-comm-36.md), [23](archive/2026-10-28-comm-23.md),
[22](archive/2026-10-27-comm-22.md),
[21](archive/2026-10-27-comm-21.md),
[20](archive/2026-10-27-comm-20.md),
[19](archive/2026-10-27-comm-19.md),
[18](archive/2026-10-27-comm-18.md),
[17](archive/2026-10-27-comm-17.md) tamam.
COMM-12–16: profil/intent/bundle/outbox/input-router/codec.
Editor/Load ağ açmaz; tombstone silinmez; fiziksel kabul açık.

**[COMM-11](archive/2026-10-27-comm-11.md):** Result DAG/ALL/ANY.
**[COMM-10](archive/2026-10-27-comm-10.md):** Missing-first karar/export.
**[COMM-09](archive/2026-10-27-comm-09.md):** Frozen cycle variable context/digest.

**[COMM-02–08 tamam](archive/2026-10-26-comm-08.md):**
UUID/type/unit/frame/enum/no-op/hash korunur.
Editor dışarı yazmaz; değerler tanımda tutulmaz. Circle: COMM-37’de kapandı.

[Kaynak haritası](architecture/communication/source-map.md),
[Plan](architecture/communication/README.md): 01–23 → 36 → 37 tamam;
opsiyonel Modbus 24–28, OPC UA 29–33, sample 34, FANUC 35.
Çevrim sonunda gönderim; akış ortası gönder/bekle ayrı kapsam. Fiziksel COMM-38,
Hand-eye/robot hareketi ayrı; araştırma notları plan indeksindedir.

**[INT-10](archive/2026-10-26-int-10.md):** CameraIntegration main’e birleşti;
dal `Variables`, fiziksel/native açık.

**[CCUX-24I yazılım/görsel tamam](archive/2026-10-26-ccux-24i.md):** iki Release;
UI 39/UI OFF 19 test; tekrar, context hata sınırı ve iki boyut fixture kayıtta.
Native wheel/pan pencere yakalama hatasıyla açık; fiziksel kabul açık.
Tercihler repo TEMP/localhost ile doğrulandı; varsayılan TEMP kabulü açık.

**[INT-09](archive/2026-10-25-int-09.md):** başlatıcı/kamera entegrasyonu.
[Restart devri](architecture/camera-integration-handoff.md).
[INT-08 kanıtı](archive/2026-10-25-int-08.md): üç medyan aşımı ve fiziksel
E2/mm/native açık; gerçek proje/reçete/kaynak/model gerekli.

**[CCUX-23b-1–6 yazılım tamam](archive/2026-10-24-ccux-23b-6.md).**
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
`multi_source_production_contract` önceki hatası 22d kapanışında korunur.
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

**[CCUX-14](archive/2026-10-22-ccux-14.md):** yazılım/görsel tamam; fiziksel açık.

CCUX-01–13 tamam; [arşiv](archive/README.md).
Kamera kimliği/profil, oturum drain, kalibrasyon izni, typed köken/hash,
Editor binding ve immutable Runtime kararlarının [korunan ayrıntıları](archive/2026-10-26-comm-plan-01.md#önceki-kamera-özetinden-korunan-ayrıntılar).
Restart üretim izni başlatmaz; PLC/robot çıkışı kapalı kalır.

Arşiv rasteri karanlık/kısmen bulanık; **bağımsız fiziksel mm kabulü açık**.
CCUX-14 fixture bunu kapatmaz. [CCUX planı](architecture/camera-setup/README.md) sürer;
**Hand-eye kapsam dışı.** KMC/ICM/EXP yeniden başlatılmaz.

## Hazır olanlar

- EXP-01–03, MUI-01–03/EUI-01–08 ve KMC/ICM/G2 yazılımı hazır;
  [kanıt/korunan sınırlar](archive/2026-10-28-efr-plan-01.md).
  **MUI-04 özgün raster/reçete/ROI açık**; fiziksel/optik/mm kabulü açık.

## Engeller ve kararlar

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
