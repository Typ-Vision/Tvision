# VisionFlow — devir

[Arşiv](archive/README.md). [MCP](archive/2026-10-24-mcp-01.md).
[LIC-01](archive/2026-11-02-lic-01.md): lisans; SDK/paket açık.

## Devam noktası

**[VF-03](archive/2026-11-03-vf-03.md): değişmez reçete yayını.**
Editor `İstasyonda yayımla`; liste Communication seçicisinde.
Hedefli ON 14/OFF 11. Tam ON 166/202'de durdu (164 geçti, 0 hata);
tam OFF yok. VF-04 başlatılmadı.

**[VF-02](archive/2026-11-02-vf-02.md): K1 geçti.** Restart/küçük ekran OK.
Hedefli8/7; OFF158+1skip. ON196/1skip/3timeout; 2 tekrar geçti.
Pose UI/tam ON kullanıcı isteğiyle açık.

**[VF-01](archive/2026-11-02-vf-01.md):** şablonlar; hedefli5/5,
restart/1280×720 onaylı. Tam ON/OFF kesildi (OFF timeout arşivde).

**[COMM4-26](archive/2026-11-02-comm4-26.md)**: yazılım tamam;
ON197/1skip, OFF156/1skip; hata0. Hedefli34/29; Qt iki viewport.
[Fiziksel devir](architecture/communication-v4/acceptance-comm4-26.md):
COMM4-PHYS-01/02 ve UI-NATIVE-01 açık. COMM4-21–25 arşivdedir.

COMM4-17–20 geçmişi/kalıcı kararları COMM4-26 ve arşivdedir.
İki UI hatası 26’da kapandı; fiziksel6D BLOCKED.
INI/probe SEGFAULT açık; temiz girdi kullan.
[EFR-04](archive/2026-10-29-efr-04.md): Wheel açık, Match/θ ertelendi.

[D:/TVision](archive/2026-10-28-dev-drive-01.md): OpenCV geçti; CTest kısmi.

[Değişkenler](variables-usage.md).

**[COMM-37](archive/2026-10-28-comm-37.md):** faz kapandı; dış plaka skipped;
cihaz varsa COMM-38. Kabul/17–23/36 ayrıntısı arşivde.
COMM-12–16: profil/intent/bundle/outbox/input-router/codec.
Editor/Load ağ açmaz; tombstone silinmez; fiziksel kabul açık.

**[COMM-11](archive/2026-10-27-comm-11.md):** Result DAG/ALL/ANY.
**[COMM-10](archive/2026-10-27-comm-10.md):** Missing-first karar/export.
**[COMM-09](archive/2026-10-27-comm-09.md):** Frozen cycle variable context/digest.

**[COMM-02–08](archive/2026-10-26-comm-08.md) tamam:** UUID/type/unit/frame/enum/no-op/hash
korunur. Editor dışarı yazmaz; tanım değer tutmaz. Circle37'de kapandı.

[COMM planı](architecture/communication/README.md):01–23/36/37 tamam;
opsiyonel24–35 ve fiziksel38 açık. Çevrim sonu gönderim var;
akış ortası gönder/bekle, Hand-eye/robot hareketi ayrı kapsam.

**[INT-10](archive/2026-10-26-int-10.md):** CameraIntegration main'de; fiziksel/native açık.

**[CCUX-24I yazılım/görsel tamam](archive/2026-10-26-ccux-24i.md):** iki Release;
UI 39/UI OFF 19 test. Native wheel/pan yakalama hatası ve fiziksel kabul açık.

**[INT-09](archive/2026-10-25-int-09.md):** başlatıcı/kamera entegrasyonu.
[Restart devri](architecture/camera-integration-handoff.md).
[INT-08](archive/2026-10-25-int-08.md): üç medyan aşımı ve fiziksel
E2/mm/native açık; gerçek proje/reçete/kaynak/model gerekli.

**[CCUX-23b-1–6 yazılım tamam](archive/2026-10-24-ccux-23b-6.md).**
Canlı proje/2448×2048 kabulü açık; fixture fiziksel kabul değildir.
Kullanıcı plaka kalibrasyonunu başardı; bağımsız mm kabulü ayrıdır.
**[CCUX-15j fiziksel kabul açık](archive/2026-10-23-ccux-15j.md):**
Exact SDK raw/sidecar: 56000 µs/0 dB kare karanlık/odak dışı,
0 merkez; model yok. Kanıt `build/ccux/ccux-15j/`.
Devam: ışık/odak/düzlem; referans/tolerans ve operatör kabulü.
[15i](archive/2026-10-23-ccux-15i.md) korunur;
`multi_source_production_contract`: Missing result açık.
[15f](archive/2026-10-23-ccux-15f.md): eski kare 50→49; fiziksel eksen/mm açık.

**[CCUX-22e yazılımı tamamlandı](archive/2026-10-23-ccux-22e.md):** Kalibrasyon
bağlantı ömrü, dosya/iptal/SaveUse drain. Kanıt: `build/ccux/ccux-22e/`.
SDK kayıtlı kamerayı bulamadı; kare/mm kabulü yok. 22f açık.
[22d Runtime](archive/2026-10-23-ccux-22d.md), [22c Editor](archive/2026-10-23-ccux-22c.md),
[API/ömür](architecture/camera-setup/acquisition-sessions.md).
`multi_source_production_contract` 22d kapanış hatası korunur.
**[22a](archive/2026-10-23-ccux-22a.md):** yazılım tamam; fiziksel baseline/paint açık.
**[21 ROI](archive/2026-10-22-ccux-21.md) tamam.**
[20 hash/preview](archive/2026-10-22-ccux-20.md) korunur; canlı teyit açık.
Geçici viewer geometrisi ayrı adlandırılır; araç ROI'leri reçeteye bağlıdır.
CCUX-16–19 [arşivde](archive/README.md); C0 açık.

**CCUX-15 fiziksel açık:** 7×7, 12 mm kullanıcı beyanı; [15c](archive/2026-10-22-ccux-15c.md).**CCUX-10b:** [Editor seçici/klavye düzeltmesi](archive/2026-10-22-ccux-10b.md).
**CCUX-03b:** [Ayrı Opto ayar servisi/readback](archive/2026-10-22-ccux-03b.md);
görüntü/mm kabulü değildir.
**[15a](archive/2026-10-22-ccux-15a.md):** SDK kopma/Stop temizleme hatası ve
7000 µs / 1 dB readback reddi açık. CCUX-15 bağımsız referans, tolerans/mm,
lens/düzlem ve belgeli fiziksel kabulü tamamlanmadı.

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
