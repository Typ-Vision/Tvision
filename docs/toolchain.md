# Yerel araç kontrolü

7 Ekim 2026, Windows x64, PowerShell. Kurulum/indirme yapılmadı.

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
