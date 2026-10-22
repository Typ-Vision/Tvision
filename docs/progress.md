# VisionFlow — güncel devir

22 Ekim 2026. [Arşiv](archive/README.md).

## Devam noktası

**Sonraki görev: [CCUX-08](architecture/camera-setup/08-save-use.md).**
**CCUX-07 tamamlandı:** reçetesiz açık deneysel politika, CCUX varsayılanından
immutable edinim; canlı OECS Calibration lease ve exact PGM+JSON replay. Typed
köken, source schema2 ve aday schema6 hash içinde kimlik/geometri/profil ile
korunur; replay dosya kalır, trusted active/system/completion reddedilir.
Yeni Release + kartın 5 CTest'i (socket suite sandbox dışında yeniden) ve 6 codec/
aktivasyon regresyonu geçti; [kapanış](archive/2026-10-22-ccux-07.md),
`build/ccux/ccux-07/`. Fixture kaliper/plaka adayları fiziksel mm kabulü değildir.
Aktivasyon CCUX-08, Runtime mm CCUX-12; UI bu kartta eklenmedi.
**CCUX-06 tamamlandı:** kayıtlı tercih, matematik uyumu ve oturum izni ayrı DTO;
yeni gerçek probe ile `prepareRemembered`, immutable model + yeni edinim zarf3;
optik değişikliği bildirimi ilgili kurulumları pending revision'a alır.
Restart/replug izin başlatmaz; exposure/gain yeniden fit istemez, yeni denetim ister.
Yeni Release ve kartın 5 CTest'i geçti; [kapanış](archive/2026-10-22-ccux-06.md),
`build/ccux/ccux-06/`. UI eklenmedi; fiziksel kamera/mm kabulü yapılmadı. Kullanıcıya göre kamera bağlı, ortam karanlık.
**CCUX-05 tamamlandı:** Tek görüntü / Canlı görüntü / Durdur ortak application
session’ına bağlandı. Gerçek FPS ve edinim türü, generation ile eski kareyi
silme, worker drain sonrası sayfa/proje/pencere geçişi ve yalnız geçerli görünen
kareden deneysel PGM+JSON kaydı var. Yeni kabul sonuçları ve sınırlar
[kapanışta](archive/2026-10-21-ccux-05.md), `build/ccux/ccux-05/`.
Release + kartın 3 CTest’i geçti. Ek shell smoke temiz dizinde de eski Runtime
Stop/retry sonuç beklentisinde başarısız; bu kapsam dışı test/sözleşme uyumsuzluğu
kapanışta açık bırakıldı.
**CCUX-04:** tek pending komut/latest-frame yuvası, deadline/generation ve cihaz
lease’i korunur; [servis kabulü](archive/2026-10-21-ccux-04.md).
Continuous capability ayrı etiketlenir; deneysel OECS sıralı FrameBurstStart
kalır. Fiziksel uzun video kabulü yapılmadı.
**CCUX-03:** sade kayıt/ayar ekranı, rota/profil belirsizliğinde açık seçim ve
oturumluk deneysel onay korunur; [yazılım/görsel kabul](archive/2026-10-21-ccux-03.md).
Native backend hâlâ Unavailable; eksik settings desteği açık unsupported kalır.
Pending optik kurulum tamamlanmış sayılmaz. Önkoşullar
[CCUX-01](archive/2026-10-21-ccux-01.md) ve
[CCUX-02](archive/2026-10-21-ccux-02.md) kayıt/servis sözleşmeleri korunur.
[CCUX planı](architecture/camera-setup/README.md) sırasıyla sürer; **Hand-eye kapsam
dışı.** Tamamlanmış KMC/ICM/EXP işleri yeniden başlatılmadı.

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
kanıtla. CCUX-03 yazılım UI kabulüdür; fiziksel kamera, native DPI veya kurulum
kabulü sayılmaz. Detay ve kalıcı kanıt kapanışta, sonraki tek görev bu özetin başında tutulur.
