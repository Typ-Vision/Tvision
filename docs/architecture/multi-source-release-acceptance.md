# KMC-10e — B yazılım kabulü ve fiziksel devir

17 Ekim 2026. A yazılım kabulü [KMC-9](independent-calibration-acceptance.md); B aynı reçetede çok kaynaklı graph, Editor, immutable Runtime, terminal7/restart/Logs/CSV ve Runtime UI'dır. **B yazılım kabulü ve fiziksel devir dokümanı tamamlandı.** Son kaynakla Release UI/ZXing ON **87/87, 445.18 sn**, Widgets OFF/ZXing OFF **73/73, 265.35 sn** geçti. Fiziksel kabul ayrı açıktır. [Ayrıntılı kapanış ve ilk koşumların ayrımı](../progress.md).

Son kanıtlar: `build/kmc10-accepted-tests-ui.log` / UI `kmc10-accepted-tests.xml`; `build/kmc10-release-tests-core-off.log` / core `kmc10-release-tests.xml`. İlk başarısız eski graph oracle ve operator Logs kilidi koşumları ayrı saklandı. Runtime UI **427**, terminal **421**, immutable production **425**, graph **302** kontrol; execution/TargetOnly **856** aynı corpus, Editor UI **580**. Tam test sayıları ayrı bağımsız corpus sayısı değildir. Son UI build `kmc10-operator-close-build-ui.log`; diğer Release build `kmc10-release-build-{ui,core-off}.log`; iki son no-work build `kmc10-no-work-build-{ui,core-off}.log`. Compiler warning/error yok.

`config/local.ui.ini` yok ve değiştirilmedi. Test fixture/INI/görsel/loglar build altındadır. Commit/push yapılmadı; önceki dirty işler korundu. UTF-8/yerel Markdown bağlantısı/diff/EXE SHA256 ve XML durum denetimi `build/kmc10-source-doc-check.json` içindedir.

## Tekrar üretim

Yerel Release Qt/OpenCV/MinGW PATH için [Windows toolchain](../windows-workstation.md) kullanılır. UI/ZXing ON cache `build/reference-ui`, Widgets OFF/ZXing OFF cache `build/kmc9-core-off` korunur.

```powershell
cmake --build build/reference-ui -j 4
cmake --build build/kmc9-core-off -j 2
ctest --test-dir build/reference-ui --output-on-failure --output-junit kmc10-final-tests.xml
ctest --test-dir build/kmc9-core-off --output-on-failure --output-junit kmc10-final-tests.xml
```

`multi_source_graph_contract`, `multi_source_execution` / `multi_source_target_only` (aynı corpus), `multi_source_production_contract`, `multi_source_terminal_trace`, `ui_multi_source_editor`, `ui_multi_source_runtime` B kabul çekirdeğidir. Tam CTest legacy migration/replay/code/world/reference/pose kapıları, calibration ve mevcut localhost socket regresyonlarını içerir. Standalone/legacy pose production kapısının kapalı kaldığı regresyon, o özelliğin üretim kabulü değildir.

## Kapasite kanıtı

Terminal testi üç applicationType × 1/2/8/10 ayrı 640×200 rasterı, ayrı scoped modeli ve her dalda iki Edge Point + Measurement kullanır. stdout `CAPACITY` satırları n/type/raster, gerçek owned raw byte, preview byte, execution süresi, terminal JSON byte ve mevcut 30000 ms / 512 MiB retained image bütçelerini yazar. Bu testin süreleri bu makinedeki dosya fixture'ına aittir; fiziksel kamera fps/transport/industrial SDK veya hard process RSS limiti kanıtı değildir. Raw/preview byte toplamı execution'ın bütün ara port/buffer konservatif retained sayımıyla aynı ölçüt değildir. Test planındaki kapasite sayı yerine raster/byte/deadline ile raporlanır.

UI corpus 256+i × 160 Gray8 dosyalarıyla 1/2/8/10 kaynak, farklı pikseller ve dal overlay'ini sınar. Görseller `build/reference-ui/kmc10d-runtime-{1,2,8,10}-{normal,small}.png`. UI testleri offscreen olduğundan native DPI/çok monitör kabulü sağlamaz.


Localhost erişimli `build/kmc10c2-network-tests-ui.log` koşumunun 12 gerçek fixture örneği `build/kmc10-capacity.json` içinde tutulur. Üç applicationType için gözlenen aralıklar:

| Kaynak | Raster / kaynak | Raw byte toplamı | Preview byte toplamı | Execution ms aralığı | Terminal JSON byte aralığı |
| --- | --- | --- | --- | --- | --- |
| 1 | 640×200 Gray8 | 128000 | 128000 | 4.83–7.70 | 45572–45608 |
| 2 | 640×200 Gray8 | 256000 | 256000 | 9.68–15.79 | 89608–89655 |
| 8 | 640×200 Gray8 | 1024000 | 1024000 | 44.27–46.62 | 353682–353772 |
| 10 | 640×200 Gray8 | 1280000 | 1280000 | 67.03–94.30 | 441782–441917 |

## Fiziksel kabul devir listesi — açık

| Alan | Sahada gerekli kanıt | Durum |
| --- | --- | --- |
| Cihaz kimliği | Gerçek provider/seri, yeniden açmada aynı kamera, aynı isim/çözünürlükte ayrım | Açık |
| Transport | USB/ağ topolojisi, driver/SDK, gereken çözünürlük ve bant genişliği | Açık |
| Süreklilik | Kopma/yeniden bağlanma, geç kare, timeout, Stop, eski sonuç engeli | Açık |
| Geometri/optik | Lens, düzlem, ROI/binning/resolution değişimi ve model invalidation | Açık |
| Ölçüm | Bağımsız gerçek referans, hata/tolerans, tekrar edilebilirlik ve çevre koşulları | Açık |
| Kapasite | Gerçek N cihaz frame boyutu/byte ve gereken çevrim süresi | Açık |
| PLC/robot | Gerçek cihaz protokolü, ACK/retry/timeout ve seçili gerçek sonuç | Açık |
| Ortak koordinat/senkronizasyon | Trigger zamanlaması, cross-camera dünya ve robot dönüşümü | Ayrı kapsam / açık |
| Dağıtım | Hedef makinede kurulum, sürücü/DLL, native DPI/çok monitör, CUDA | Açık |

A yazılım kabulü, B yazılım kabulü, fiziksel kabul ve ortak koordinat/senkronizasyon ayrı teslim durumlarıdır. KUI-6 ve T8.4.2b.3c/lens/industrial/CUDA/dağıtım backlog'u B testleri geçti diye kapatılmaz. Cihaz bulunmadığı için bu devir belgesi fiziksel deneme yapıldığını ileri sürmez.
