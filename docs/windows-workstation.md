# Windows geliştirme ortamı — 15 Ekim 2026

Günlük giriş repo kökündeki `VisionFlow.cmd`; varsayılan hedef
`build/reference-ui/visionflow.exe` olur. `VisionFlow.cmd -SmokeTest` offscreen
smoke çalıştırır; normal kullanımda parametresiz açılır. Kaynak/EXE eşleşmesi ve
birleşik kabul [INT devir kaydındadır](architecture/camera-integration-handoff.md).
Aşağıdaki tarihli kurulum/test sayıları tarihsel kanıttır; yeni ürün kabulü değildir.

Bu bilgisayarın çalışma kopyası `C:/Projects/Tvision`. Qt araçları `C:/Qt`
altındadır. Aşağıdaki yollar geliştirme kurulumudur; taşınabilir ürün paketi değildir.

## Bileşenler

- Qt 6.12.0, MinGW 64-bit; Core/Gui/Widgets/Concurrent/Multimedia/Network.
- Qt ile gelen MinGW GCC 13.1.0, x86_64-posix-seh; CMake 3.30.5, Ninja 1.12.1.
- OpenCV 5.0.0 kaynağı: kullanıcı tarafından açılmış `C:/OpenCV/sources`.
  Paketin `build/x64/vc16` ikilileri MinGW ürünü için kullanılmaz.
- Yerel `opencv-5.0.0-windows.exe` SHA-256:
  `9C6C1FCEA58ACDF06EDBA13148B2246E00C2658143FA51E61ECD370DB8C39F63`.
  Bu yerel envanter bilgisidir; bağımsız yayıncı hash doğrulaması değildir.
- OpenCV Release/shared modülleri: core, imgproc, imgcodecs, objdetect,
  calib, geometry, features; zorunlu flann ve stereo bağımlılıkları.
- PNG/JPEG/TIFF/WebP ve dahili BMP/PNM gibi codec yolları açık.
  CUDA/OpenCL/IPP/ITT, FFmpeg/GStreamer, OpenEXR/OpenJPEG/AVIF kapalı.
  OpenCV highgui/videoio kullanılmaz; pencere ve webcam yolu Qt'dedir.
  NASM bulunmadığı için libjpeg-turbo SIMD kapalı; performans eşdeğerliği iddia edilmez.
- ZXing 2.3.0 static reader ve yalnız fixture üretiminde kullanılan Zint 2.15.0,
  `third_party/manifest.json` içindeki sabit commit/SHA-256/options ile hazırlanır.

## Tekrar hazırlama

```powershell
# OpenCV: kaynaklar önceden açılmış olmalı. C:/OpenCV altına yazma yetkisi gerekir.
.\scripts\build-opencv.ps1 -Install

# Yalnız bu PowerShell sürecinin PATH'i; sistem PATH'i değiştirilmez.
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
.\scripts\bootstrap-zxing.ps1 -Download
.\scripts\probe-opencv.ps1
.\scripts\probe-zxing.ps1

cmake -S . -B build/reference-ui -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib -DVISIONFLOW_BUILD_UI=ON -DVISIONFLOW_WITH_ZXING=ON -DVISIONFLOW_ZXING_PREFIX=C:/Projects/Tvision/build/deps/zxing-prefix
cmake --build build/reference-ui -j 6
ctest --test-dir build/reference-ui --output-on-failure
.\scripts\run-local.ps1
```

Başka bilgisayarda repo/Qt/derleyici yollarını değiştirin. `build-opencv.ps1`
Source/InstallPrefix/CompilerBin/CMakeBin/NinjaBin/Jobs parametrelerini kabul eder.
İndirme önbelleği geri yüklendiyse ZXing bootstrap'ta `-Download` verilmez.
Windows Git checkout CRLF dönüşümü lisans metni karşılaştırmasında yalnız CRLF→LF
normalizasyonuyla ele alınır; arşiv hash kontrolü ve metnin diğer bütün karakterleri korunur.

## Çalıştırma

Güncel hedef `build/reference-ui/visionflow.exe`. `run-local.ps1` gerekli DLL
yollarını process PATH'e ekler; başka build için `-BuildDirectory build` gibi
açık bir dizin verilebilir. Bu klasörler Git'e eklenmez. EXE'yi başka makineye
tek başına kopyalamak dağıtım değildir; Qt/OpenCV/runtime paketleme ayrı iştir.

## Doğrulama

16 Ekim KMC-0 güncellemesi: aşağıdaki tarihsel dört UI hatası açık pixel
modunu seçen ve yeni mm varsayılanını doğrulayan smoke senaryolarıyla çözüldü.
Tam Release UI **70/70**, yeni ayrı OFF/core **61/61** geçti; KMC-1 yeni
kimlik sözleşmesi iki build'de ilgili **10/10** ile kabul edildi.
Kanıtlar ve kapsam [progress](progress.md) / [KMC-1](architecture/measurement-setup-contract.md).
Fiziksel/KUI-6/dağıtım borçları sürer. Aşağıdaki kurulum kayıtları tarihseldir.

Kurulum tamamlandı. `build/opencv-mingw/first-configure.log`, `first-build.log`
ve `install.log` ilk configure/769 build adımı/kurulumu kaydeder. Betikle indirmesiz
tekrar configure/build başarılı, Ninja `no work to do` döndürdü (`build/opencv-repeat.log`).

- OpenCV API probe: **1/1**, yedi API senaryosu, compile/runtime version 5.0.0.
  `build/opencv-probe.log`, `build/tool-probes/opencv-api-results.json` ve
  `opencv-binary-inventory.json` (kurulu header/import/DLL hash envanteri).
- ZXing probe: **1/1**, sekiz format/**33 kontrol, sıfır hata**.
  `build/zxing-bootstrap.log`, `build/zxing-probe.log`,
  `build/zxing-probe/zxing-results.json`. Kaynak arşiv hash ve lisans kontrolleri geçti.
- Tvision UI ON/ZXing ON Release configure ve **259 build adımı başarılı**.
  `build/workstation-configure.log`, `build/workstation-build.log`.
- Tam CTest ilk sandbox koşumu **65/70**, 163.71 sn.
  Başarısız beş testin ağ erişimli tekrarında PLC geçti; dört UI testi kaldı.
  Birleşik kanıt **66 farklı test geçti / 70** (61 core, 5 UI geçti);
  tek tam 66/70 veya 70/70 başarılı koşum iddiası yoktur.
  `build/workstation-ctest.log`, `build/workstation-retest.log`,
  `build/reference-ui/workstation-tests.xml`, `workstation-retest.xml`.
- Kalanlar: `ui_editor_usability` — `new tool immediately armed on current source`;
  `ui_pose_tracking_smoke` — `Raw reference requires pixel mode`;
  `ui_circle_smoke` — `Circle numeric ROI commit`;
  `ui_shell_smoke` — ağ erişimli tekrarda `pixel mode explicitly diagnostic`.
  İlk shell koşumunun Communication timeout'u tekrarda aşılmıştır.
  Hatalar bu kaynak sürümünün KUI-6 ürün/test incelemesinde çözülmelidir;
  bunlar başarılı kurulumun tam ürün kabulü sayılmasını engeller.
- `run-local.ps1 -SmokeTest` kurulu DLL'lerle uygulamayı başlattı, aynı
  `pixel mode explicitly diagnostic` kontrolünde exit 1 döndürdü;
  betik başarısızlığı doğru raporladı (`build/workstation-launcher-smoke.log`).
- `build/reference-ui/kui5-home-a.png` açılıp incelendi; kalibrasyon ana sayfası
  render edildi. Native DPI/çoklu monitör veya tam kullanıcı akışı kabulü değildir.
- Üç değişen PowerShell betiği parse kontrolünden ve `git diff --check` geçti.
  Ürün C++/UI kaynakları ve kullanıcı `config/local.ui.ini` değiştirilmedi.

Fiziksel kamera/PLC/robot, CUDA, üretim performansı ve temiz makine dağıtımı bu
geliştirme ortamı kabulüne dahil değildir.
