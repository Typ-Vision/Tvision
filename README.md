# VisionFlow Industrial Vision Suite

C++20 / Qt Widgets / OpenCV endüstriyel görüntü işleme uygulaması.
Üç applicationType aynı Recipe Editor, yürütme servisleri ve Runtime'ı kullanır.
UI algoritma veya donanım SDK'sını doğrudan çağırmaz.

**Güncel görev, son doğrulama ve açık engeller [devir özetindedir](docs/progress.md).**

## Uygulanan kapsam

- Ortak Recipe Editor, typed DAG, worker üzerinde yürütme, kaynak başına görüntü/cache,
  ROI/model öğretimi ve gerçek araç çıktıları.
- Kaydedilmiş projeden reçetesiz kamera/kurulum yönetimi, bilinen ölçü/plaka/dosya
  kalibrasyon sihirbazları; **Kaydet ve kullan** aday kaydı ve uygunluk denetimini
  tek komutta yapar, başarısız denetimde önceki model seçimi korunur.
- Çok kaynaklı Editor ve immutable Runtime hazırlığı; gerçek çevrim, kalıcı terminal
  kayıtları, restart geçmişi, Logs/CSV ve vf.socket2 scalar mapping.
- Image File/Webcam edinimi; Camera Manager'da keşif, kamera kaydı ve Mono8 profil
  taslağı. Industrial Editor kaynağına kayıtlı kamera/kurulum/profil bağlanır;
  kare isteği ortak worker/provider yolundadır. Industrial Load çevrimdışı beyanı
  gösterir. Desteklenen Opto/OECS kamera ayarları gerçek readback ile okunup
  kaydedilir; bu işlem görüntü edinim izninden bağımsızdır. Industrial Run
  tam native kabul ister. Native production kapalıdır; mevcut COE-U için normal kamera komutları OECS edinim yolunu seçer.

Editor reçetesini açık düzenleme komutuyla açmak bağlı kameraları arka planda
hazırlar; Görüntü al/Run Flow aynı bağlantıyı kullanır. Save bağlantıyı korur;
kabul edilen çıkış ve Stop kapatır. Program/proje/ekran restore bağlantı açmaz.

Hazırlık/keşif üretim kabulü değildir; gerçek sonuç olmadan PASS, skor veya
robot pose gösterilmez. Robot ekranları hazırlıktır. Yazılım testleri fiziksel
kamera/ölçüm, PLC/robot, lens, native DPI, CUDA veya dağıtım kabulünü kapatmaz.

## Yerel derleme ve test — PowerShell

Yerel toolchain: Qt 6.12.0 / MinGW 13.1.0 ve MinGW OpenCV 5.0.0.
[Toolchain/ZXing rehberi](docs/windows-workstation.md).

```powershell
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
cmake -S . -B build/reference-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/Projects/Tvision/build/deps/zxing-prefix
cmake --build build/reference-ui -j 6
ctest --test-dir build/reference-ui --output-on-failure
.\scripts\run-local.ps1
```

UI kapalı Release yapılandırması:

```powershell
cmake -S . -B build/kmc9-core-off -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_BUILD_UI=OFF -DVISIONFLOW_WITH_ZXING=OFF
cmake --build build/kmc9-core-off -j 6
ctest --test-dir build/kmc9-core-off --output-on-failure
```

`ctest --test-dir build/reference-ui -N` testleri listeler; `-R` tam suite değildir.
Socket testleri localhost ister. UI değişikliğinde Release build,
ilgili CTest ve normal/küçük ekran görsel kontrolü yapılır.

`run-local.ps1` DLL yollarını yalnız başlattığı sürece ekler. Başka build için
`-BuildDirectory build`, offscreen smoke için `-SmokeTest` verilebilir.
EXE'yi tek başına kopyalamak doğrulanmış dağıtım değildir.

OECS / GenTL probe komutları, sandbox sınırları ve kanıt bağlantıları
[Windows kamera tanı rehberindedir](docs/windows-camera-diagnostics.md).

## Temel kullanım

1. Projeyi oluştur/aç ve kaydet; kamera/kalibrasyon için reçete gerekmez.
2. Camera Manager'da kamerayı kaydet; Setup → Calibration aynı kayıtları kullanır.
   Kamera → Kalibre Et → Bilinen ölçü veya Kalibrasyon plakası → Kaydet ve kullan.
   Yeni kaydın varsayılan kurulum/profili otomatik bağlanır; eksik fiziksel kurulum
   ayrıntıları tamamlanmadan ölçüm izni oluşmaz.
3. Reçeteyi Editor'da kur; kaynak/araçları bağla, görüntü al ve Run Flow çalıştır.
4. Kaydedilmiş reçete için Load Recipe → Run → Tek çevrim; sonuçlar Logs/CSV'de.
   Load/Run hazırlığı tek başına sonuç değildir. Stop → yeniden Load sürümü değiştirir.

Normal kamera komutları uygun edinim yolunu seçer; ek OECS modu/onayı yoktur.
Profil bağlama, PGM+JSON, kalibrasyon, Runtime ve panel
kullanımı [kısa kullanım rehberinde](docs/camera-runtime-usage.md). Restart
edinim/Run başlatmaz; PLC/robot çıkışı kapalıdır. Test INI/görselleri
build altında, yerel panel ayarları config/local.ui.ini içindedir.

## Göreve göre okuma

- Güncel görev ve engeller: [progress](docs/progress.md).
- Aktif geliştirme: [CCUX plan indeksi](docs/architecture/camera-setup/README.md),
  ortak sözleşme ve yalnız seçili kart. Hand-eye bu paketin dışında.
- Kamera backend/kimlik: [ICM](docs/architecture/industrial-camera-plan.md).
- UI/ürün kuralları: [UI gereksinimleri](docs/requirements/02_machine_vision_ui_gereksinimleri.md).
- Proje/katman: [proje formatı](docs/architecture/project-format.md),
  [mimari](docs/architecture/architecture.md).
- Geçmiş kanıt: [arşiv dizini](docs/archive/README.md); yalnız gereken kaydı açın.

Belge kontrolü: `python scripts/check-context-docs.py`.
