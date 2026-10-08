# Yerel araç kontrolü

Measurement/Calibration UI son kabulü: standart Release build/measurement-ui CTest 15/15; temiz Widgets OFF build/measurement-core Release CTest 13/13. Smoke Calibration/Measurement PASS/FAIL/invalid ve 850×600 görselleri üretir. Geometri/kalibrasyon sentetik; yeni fiziksel webcam, native DPI, kurulum veya CUDA kabulü değildir.

6C kayıt/binding son kabulü: build/calibration-release Release UI ON CTest 14/14; build/calibration-core Release Widgets OFF CTest 12/12. calibration_persistence evidence replay/current provider/schema 2 binding ve eski production validation reddini doğrular. Yeni SDK/toolchain kurulumu yapılmadı; fiziksel kalibrasyon, dağıtım ve CUDA kabulü değildir.

7 Ekim 2026, Windows x64, PowerShell. Aşağıdaki toolchain hazırlığında kurulum/indirme yapılmadı. Sonraki kullanıcı yönlendirmesiyle 6B'de Basler pylon Runtime 26.09.2 ve Daheng Galaxy tam SDK 2.6.2608.9131 kuruldu; Qt/OpenCV/compiler korunur. Kaynak, kapsam, DLL/env ve ABI kanıtı [kamera SDK kurulum kaydında](architecture/camera-sdk-installation.md). Basler tam Developer SDK formu açık; Runtime kurulumunu tam SDK kabulü olarak sunmayın.

| Araç | Tespit |
| --- | --- |
| CMake | 3.30.5, C:/Qt/Tools/CMake_64/bin |
| Ninja | 1.12.1, C:/Qt/Tools/Ninja |
| C++ | MinGW GCC 13.1.0, C:/Qt/Tools/mingw1310_64/bin |
| Qt / qmake / qtpaths | 6.12.0, C:/Qt/6.12.0/mingw_64 |
| OpenCV | 5.0.0, C:/OpenCV/5.0.0/x64/mingw/lib/OpenCVConfig.cmake |
| GPU | NVIDIA GeForce RTX 5050 Laptop GPU, driver 573.22 |
| CPU | 13th Gen Intel Core i5-13500HX; OpenCV varsayılan thread sayısı 20 |
| Git | PATH üzerinde mevcut; klasörde başlangıçta .git yok |
| Python | PATH üzerinde mevcut; bu hazırlıkta kullanılmadı |
| cl / clang++ / pkg-config / nvcc | PATH üzerinde bulunamadı; sistemde hiç kurulmadığı anlamına gelmez |

OpenCV MinGW DLL/import lib modülleri core/imgproc/imgcodecs mevcut. İncelenen kurulumda CUDA modül dosyaları bulunmadı; CUDA benchmark yapılmadı. GPU için CPU ölçümünden hız tahmini çıkarılmaz.

Release CMake yapılandırması ve üç hedefin derlemesi başarılı. `visionflow.exe --smoke-test`, Qt offscreen platformuyla 0 exit code döndürdü; Qt başlangıç/DLL yükleme doğrulandı. CMake isteğe bağlı Vulkan header'larını bulamadı; Widgets derlemesine engel olmadı. Build ve benchmark doğrulaması sonuçları `performance/opencv_matching.md` içinde tutulur. GUI görsel/etkileşim doğrulaması bu hazırlık aşamasında yapılmadı.

## A9 kaynak ve test konumu

TVision ve üst dizinler Git worktree değildi. Komşu TVis C++17 tvis projesidir; ProductionRecipeService kaydı yoktur. TVision/.git main yerel deposu kuruldu; remote yayın yapılmadı. scripts/validate-local.ps1 temiz build/a9-core ve build/a9-ui Release matrisinde geçti: 9/9 ve 11/11 CTest. Qt Gui bağımlılığı sürer; Widgets core build içinde aranmaz. Uzak Windows CI script üzerinden planlandı, henüz çalıştırılmadı.

## Webcam/Measurement teslimi

Qt 6.12.0 Multimedia development/runtime mevcut; CMake artık Qt6::Multimedia ister (Widgets OFF dahil). OpenCV videoio mevcut olmadığından kullanılmaz; kurulum yapılmadı. Güncel Release build/webcam-release 13/13, bağımsız build/webcam-core 11/11 CTest. Gerçek webcam kabulü ve kapsam progress.md/architecture/acquisition.md içindedir; dağıtım Multimedia plugin/DLL paketleme kabulü değildir.

7 Ekim 6C Runtime: `build/runtime-ui` standart Release CTest 16/16; `build/runtime-core` Widgets OFF Release 14/14. runtime_persistence ve Runtime UI gerçek dosya/Edge/Measurement, immutable draft, yetki, activation/start/terminal lock failure/retry, Stop, rollback, restart/archive/invalidation kabulü içerir. runtime-pass/fail/record-pending/invalid/logs/small PNG build/runtime-ui altında görsel incelendi. Fiziksel yeni cihaz veya power-loss/kurulum/dağıtım/CUDA kabulü değildir.

## K2/K3 kalibrasyon kabulü

OpenCV 5 yerel core/imgproc/imgcodecs/objdetect/calib/geometry modülleri; SB objdetect.hpp, perspective/homography geometry.hpp. build/k1-ui ve build/k1-core güncel Release K1–K3 binary/test konumudur; dizin adındaki k1 eski ilk configure adıdır. UI 19/19, Widgets OFF 17/17; homography_runtime ve plane_mapping_contract analytic independent points/persistence/Runtime kabulünü içerir. Kurulum/dağıtım/CUDA veya fiziksel ölçüm kabulü değildir.

## 6D.1 TCP/IP socket kabulü

Yerel Qt 6.12.0 Network development/runtime link edildi; CMake Qt6::Network ister (Widgets OFF dahil). Yeni kurulum yapılmadı. Aynı build/k1-ui ve build/k1-core standart Release binary'leri artık 6D.1'i içerir; tüm CTest UI 20/20 ve core 18/18. Son mapping seçicisi/mapping-yok değişimi sonrası UI plc_socket_runtime/ui_shell_smoke ve core plc_socket_runtime tekrar geçti. Actual localhost TCP, ayrı visionflow_plc_simulator process'i, ACK/timeout/duplicate/reconnect/record retry ve 850×600 görselleri doğrulandı. Sandbox localhost connect iznini engelledi; ağ testleri izinli sandbox dışında yalnız localhost ile çalıştırıldı. Fiziksel PLC/kamera, üretim doğruluğu, kurulum/dağıtım/CUDA kabulü değildir. [Sözleşme ve komutlar](architecture/plc-socket.md).
