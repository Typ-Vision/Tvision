# VisionFlow Industrial Vision Suite

C++20 / Qt Widgets / OpenCV ile endüstriyel görüntü işleme uygulaması.
Üç applicationType aynı Recipe Editor, yürütme servisleri ve Runtime'ı kullanır.
UI algoritma veya donanım SDK'sını doğrudan çağırmaz.

**Güncel görev, son doğrulama ve açık engeller [devir özetindedir](docs/progress.md).**
Bu README giriş ve kullanım rehberidir; teslim günlüğü değildir.

## Uygulanan kapsam

- Ortak Recipe Editor, typed DAG, worker üzerinde yürütme, kaynak başına görüntü/cache,
  ROI/model öğretimi ve gerçek araç çıktıları.
- Kaydedilmiş projeden reçetesiz kamera/kurulum yönetimi, bilinen ölçü/plaka/dosya
  kalibrasyon sihirbazları; aday kaydı ile etkinleştirme ayrı işlemlerdir.
- Çok kaynaklı Editor ve immutable Runtime hazırlığı; gerçek çevrim, kalıcı terminal
  kayıtları, restart geçmişi, Logs/CSV ve vf.socket2 scalar mapping.
- Image File/Webcam edinimi ve GenTL keşif/salt okunur kontrolü. Endüstriyel
  stream/grab ve bunun kalibrasyon/Editor/Runtime bağlantısı henüz uygulanmadı.

Hazırlık veya keşif sonucu üretim kabulü değildir. Gerçek sonuç olmadan PASS, skor
veya robot pose gösterilmez. Robot hazırlık ekranları çalışan robot entegrasyonu
değildir. Yazılım/fixture kabulü fiziksel kamera, ölçüm, PLC/robot, lens, native DPI,
CUDA performansı veya ürün kurulum/dağıtım kabulünün yerine geçmez.

## Yerel derleme ve test — PowerShell

Bu makinede Qt 6.12.0 / MinGW 13.1.0 ve MinGW ile derlenmiş OpenCV 5.0.0 kullanılır.
Komutlar mevcut bağımlılık kurulumunu varsayar. Ayrıntılı kurulum/yeniden hazırlama
ve ZXing bağımlılığı [Windows ortamı belgesindedir](docs/windows-workstation.md).
Başka makinede yolları ve ABI uyumunu doğrulayın.

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

Göreve ait test seçimi için `ctest --test-dir build/reference-ui -N` ile adları
inceleyip `-R` kullanın; ilgili test sonucunu tam suite kabulü diye kaydetmeyin.
Socket testleri localhost erişimi gerektirir. UI değişikliğinde Release build,
ilgili CTest ve normal/küçük ekran görsel kontrolü yapılır.

`run-local.ps1` DLL yollarını yalnız başlattığı sürece ekler. Başka build için
`-BuildDirectory build`, offscreen smoke için `-SmokeTest` verilebilir.
EXE'yi tek başına kopyalamak doğrulanmış dağıtım değildir.

### OECS / GenTL tanısı

PowerShell 7 ile mevcut Release probe'unu explicit x64 U3V producer üzerinden çalıştırın:

```powershell
pwsh -NoProfile -File scripts/diagnose-camera.ps1 -OpenCheck
```

`-ProducerPath`, `-ProbePath`, `-OutputDirectory` ve bağımlılık dizinleri değiştirilebilir.
Varsayılan rapor `build/icm/01a/local/diagnostic.json`; ham probe çıktısı ve doğrudan
DLL import envanteri aynı dizindedir. Betik sürüm/hash, PE mimarisi, process/machine
ortam farkı, child PATH, komut ve exit code kaydeder. Kalıcı ortamı değiştirmez.
`NO_DEVICES` discovery'nin tamamlanıp sıfır cihaz bulduğudur; kamera kabulü değildir.
`NO_DEVICES`/`DEVICES_DISCOVERED` exit 0, diğer tanı durumları exit 1 döner.
Open/close yalnız keşfedilen cihazlarda denenir; grab uygulanmış değildir.
DLL aday yolları Windows loader çözümlemesinin veya dağıtımın doğrulandığı anlamına gelmez.

## Temel kullanım

1. Project Manager'dan proje oluşturun/açın ve kaydedin. Recipe Manager'dan reçete
   oluşturun; kaynak ve araçları ortak Editor'da bağlayın. Save taslağı kaydeder.
2. Image File için gerçek görüntü açın; kamera kaynağında edinim yapın. **Run Flow**
   akışı yürütür. **Run Tool** güncel girdilerle seçili aracı çalıştırır; eksik
   upstream sonucu veya görüntü yerine değer üretmez.
3. **Setup → Calibration** sistem özetinden kamera/kurulum ve reçetesiz denemeleri
   yönetin. Kaydet aday oluşturur; aktivasyon incelemesi ve kaynak→kurulum bağı
   ayrı adımlardır. Dosya denemesi fiziksel kamera kanıtı üretmez.
4. Kaydedilmiş reçeteyi **Load Recipe → Run** ile Runtime'a hazırlayın.
   **Tek çevrim** gerçek edinim/yürütmeyi başlatır; Run tek başına sonuç üretmez.
   Stop eski sonucu bastırır; farklı sürüm için Stop → Load Recipe → Run kullanın.
5. Kalıcı sonuçları **Logs/CSV** üzerinden inceleyin. Kayıt hatasında tekrar yazma
   aynı sonucu kaydeder; yeni acquisition yapmaz. Restart geçmişi korur, aktif
   üretim iznini geri yüklemez.

Panel düzeni ve son seçimler `config/local.ui.ini` içindedir; bu proje autosave'i
değildir. Test INI, fixture, görüntü ve raporlar `build/` altında tutulur.
Görünümü sıfırlamak için **View → Panel düzenini sıfırla** kullanılır.

## Göreve göre belge haritası

Yeni chat: AGENTS + [kısa progress](docs/progress.md) + bu README; ardından seçili
görev kartı ve ilgili bölümler. Bütün gereksinim/mimari/arşiv dosyalarını topluca
okumayın. Önce başlık veya görev ID'si arayın; belirsizlikte bağımlılıklara genişletin.

| Konu | Kaynak |
| --- | --- |
| Kamera geliştirme | [ICM ana planı](docs/architecture/industrial-camera-plan.md), [devir şablonu](docs/architecture/industrial-camera-handoff.md) |
| Ürün/UI/teknik gereksinim | [Ürün](docs/requirements/01_machine_vision_yazilim_tanimi.md), [UI](docs/requirements/02_machine_vision_ui_gereksinimleri.md), [C++/Qt/OpenCV](docs/requirements/03_cpp_qt_opencv_gereksinimleri.md) |
| Katmanlar ve persistence | [Mimari](docs/architecture/architecture.md), [proje formatı](docs/architecture/project-format.md) |
| Editor ve yürütme | [Çok kaynaklı Editor](docs/architecture/multi-source-execution-editor.md), [DAG sözleşmesi](docs/architecture/multi-source-graph-contract.md) |
| Kalibrasyon | [Sistem özeti](docs/architecture/calibration-overview.md), [kaynak bağlantısı](docs/architecture/scoped-calibration-integration.md), [taşıma/Save As](docs/architecture/measurement-setup-migration.md) |
| Runtime ve kabul | [Runtime/terminal](docs/architecture/multi-source-runtime-terminal.md), [B kabulü](docs/architecture/multi-source-release-acceptance.md), [PLC socket](docs/architecture/plc-socket.md) |
| Geçmiş karar ve teslim | [Arşiv dizini](docs/archive/README.md), [önceki README](docs/archive/2026-10-17/README.md) |

Gereksinim/mimari belgelerindeki tarihli teslimler geçmiş kapsamı anlatabilir;
güncel devam noktası progress'tir. Teknik sözleşme değişiminde ilgili belge ve
kaynak kod birlikte doğrulanır. Günlük ayrıntılarını README'ye eklemeyin.

Belge boyutu ve başlangıç bağlantılarını kontrol etmek için:

```powershell
python scripts/check-context-docs.py
```
