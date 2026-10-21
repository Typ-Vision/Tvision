# VisionFlow Industrial Vision Suite

C++20 / Qt Widgets / OpenCV endüstriyel görüntü işleme uygulaması.
Üç applicationType aynı Recipe Editor, yürütme servisleri ve Runtime'ı kullanır.
UI algoritma veya donanım SDK'sını doğrudan çağırmaz.

**Güncel görev, son doğrulama ve açık engeller [devir özetindedir](docs/progress.md).**

## Uygulanan kapsam

- Ortak Recipe Editor, typed DAG, worker üzerinde yürütme, kaynak başına görüntü/cache,
  ROI/model öğretimi ve gerçek araç çıktıları.
- Kaydedilmiş projeden reçetesiz kamera/kurulum yönetimi, bilinen ölçü/plaka/dosya
  kalibrasyon sihirbazları; aday kaydı ile etkinleştirme ayrı işlemlerdir.
- Çok kaynaklı Editor ve immutable Runtime hazırlığı; gerçek çevrim, kalıcı terminal
  kayıtları, restart geçmişi, Logs/CSV ve vf.socket2 scalar mapping.
- Image File/Webcam edinimi; Camera Manager'da keşif, kamera kaydı ve Mono8 profil
  taslağı. Industrial Editor kaynağına kayıtlı kamera/kurulum/profil bağlanır;
  kare isteği ortak worker/provider yolundadır. Industrial Load çevrimdışı beyanı
  gösterir; Run tam native kabul ister. Native production kapalıdır; mevcut COE-U için Camera Manager’da açık deneysel önizleme yolu vardır.

Hazırlık veya keşif sonucu üretim kabulü değildir. Gerçek sonuç olmadan PASS, skor
veya robot pose gösterilmez. Robot hazırlık ekranları çalışan robot entegrasyonu
değildir. Yazılım/fixture kabulü fiziksel kamera, ölçüm, PLC/robot, lens, native DPI,
CUDA performansı veya ürün kurulum/dağıtım kabulünün yerine geçmez.

## Yerel derleme ve test — PowerShell

Yerel toolchain: Qt 6.12.0 / MinGW 13.1.0 ve MinGW OpenCV 5.0.0.
Ayrıntılı kurulum/yeniden hazırlama
ve ZXing bağımlılığı [Windows ortamı belgesindedir](docs/windows-workstation.md).


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

`ctest --test-dir build/reference-ui -N` testleri listeler; `-R` seçimi tam suite değildir.
Socket testleri localhost erişimi gerektirir. UI değişikliğinde Release build,
ilgili CTest ve normal/küçük ekran görsel kontrolü yapılır.

`run-local.ps1` DLL yollarını yalnız başlattığı sürece ekler. Başka build için
`-BuildDirectory build`, offscreen smoke için `-SmokeTest` verilebilir.
EXE'yi tek başına kopyalamak doğrulanmış dağıtım değildir.

OECS / GenTL probe komutları, sandbox sınırları ve kanıt bağlantıları
[Windows kamera tanı rehberindedir](docs/windows-camera-diagnostics.md).

## Temel kullanım

1. Project Manager'dan proje oluşturun/açın ve kaydedin. Recipe Manager'dan reçete
   oluşturun; kaynak ve araçları ortak Editor'da bağlayın. Save taslağı kaydeder.
2. Image File için gerçek görüntü açın; kamera kaynağında edinim yapın. **Run Flow**
   akışı yürütür. **Run Tool** güncel girdilerle seçili aracı çalıştırır; eksik
   upstream sonucu veya görüntü yerine değer üretmez.
3. **Camera Manager** ile kamerayı projeye kaydedin; yerel producer hatırlanır.
   **Setup → Calibration** aynı kamera kayıtlarını kullanır. Kalibrasyonda kayıtlı edinim
   profilini seçin; aday kaydı etkin model değildir. Aktivasyon ve kaynak→kurulum bağı ayrıdır.
   Editor’da Industrial Camera ekleyin, Properties üzerinden kayıtlı profili seçip
   **Profili bağla / güncelle** düğmesine basın. **Görüntü al** yeni kare ister; seçim
   tek başına edinim yapmaz. Profil düzenlenirse açıkça yeniden bağlayın.
   Mevcut COE-050-M-USB-050-IR-C için Camera Manager’da **Deneysel OECS edinimini
   kullan** seçimi ve kurulu x64 runtime ile **Önizleme** açılır. Tam sensör
   Mono8 gerekir; SDK derlemesi [tanı rehberindedir](docs/windows-camera-diagnostics.md).
   **Görünen kareyi kanıtıyla kaydet** yeni `.pgm` ve `.pgm.json` yazar.
   Tetik eşleşmesi doğrulanmamıştır; Stop kareyi temizler. Tercih restart’ta kapalıdır.
   Deneysel dosyanın Editor/replay bağlantısı EXP-02’yi bekler; normal Image File
   yolundan açılıp doğrulanmış kaynağa yükseltilemez.
4. Kaydedilmiş reçeteyi **Load Recipe → Run** ile Runtime'a hazırlayın.
   **Tek çevrim** gerçek edinim/yürütmeyi başlatır; Run tek başına sonuç üretmez.
   Stop eski sonucu bastırır; farklı sürüm için Stop → Load Recipe → Run kullanın.
5. Kalıcı sonuçları **Logs/CSV** üzerinden inceleyin. Kayıt hatasında tekrar yazma
   aynı sonucu kaydeder; yeni acquisition yapmaz. Restart geçmişi korur, aktif
   üretim iznini geri yüklemez.

Editor'de reçete adı ve draft/dirty durumu üst satırdadır. **...** reçete bilgilerini
ve çok kaynaklı geçişi açar. Viewer **...** menüsünde Reload, zoom ve ROI geometrisi
bulunur. Ayrıntılı [Editor kullanım rehberi](docs/editor-usage.md): ROI, açıklamalar,
Toolbox, nominal tarama kılavuzları ve küçük ekran komutları.

Panel düzeni ve son seçimler `config/local.ui.ini` içindedir; bu proje autosave'i
değildir. Test INI, fixture, görüntü ve raporlar `build/` altında tutulur.
Sıfırlama: **View → Panel düzenini sıfırla**.

Workspace pin düğmesi veya **View → Workspace sabit** otomatik gizlemeyi açar.
Sol şerit hover/tıklama veya Tab ve Space ile açılır; içerik üzerinde yüzer.
Panelde klavye odağı varken açık kalır; Escape odağı şeride döndürür.
İlk kullanım ve panel sıfırlama sabit moddur.

## Göreve göre belge haritası

Başlangıç: AGENTS + [progress](docs/progress.md) + README; ardından seçili görev
kartı ve ilgili sözleşme. Güncel sıra progress'tedir.

| Konu | Kaynak |
| --- | --- |
| Kamera geliştirme | [ICM ana planı](docs/architecture/industrial-camera-plan.md), [devir şablonu](docs/architecture/industrial-camera-handoff.md) |
| Ürün/UI/teknik gereksinim | [Ürün](docs/requirements/01_machine_vision_yazilim_tanimi.md), [UI](docs/requirements/02_machine_vision_ui_gereksinimleri.md), [C++/Qt/OpenCV](docs/requirements/03_cpp_qt_opencv_gereksinimleri.md) |
| Katmanlar ve persistence | [Mimari](docs/architecture/architecture.md), [proje formatı](docs/architecture/project-format.md) |
| Editor ve yürütme | [Çok kaynaklı Editor](docs/architecture/multi-source-execution-editor.md), [DAG sözleşmesi](docs/architecture/multi-source-graph-contract.md) |
| Kalibrasyon | [Sistem özeti](docs/architecture/calibration-overview.md), [kaynak bağlantısı](docs/architecture/scoped-calibration-integration.md), [taşıma/Save As](docs/architecture/measurement-setup-migration.md) |
| Runtime ve kabul | [Runtime/terminal](docs/architecture/multi-source-runtime-terminal.md), [B kabulü](docs/architecture/multi-source-release-acceptance.md), [PLC socket](docs/architecture/plc-socket.md) |
| Geçmiş karar ve teslim | [Arşiv dizini](docs/archive/README.md), [önceki README](docs/archive/2026-10-17/README.md) |


Belge kontrolü:

```powershell
python scripts/check-context-docs.py
```
