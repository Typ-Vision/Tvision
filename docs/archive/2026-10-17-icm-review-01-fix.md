# ICM-REVIEW-01-FIX — iki inceleme bulgusunun kapanışı

17 Ekim 2026. Uygulayıcı: Codex.

## Kapsam ve sonuç

[ICM-REVIEW-01](2026-10-17-icm-review-01.md) P1 ve P2 bulguları kapatıldı.
Tarihsel inceleme ve önceki teslim kayıtları değiştirilmedi. ICM-04b uygulanmadı.

- P1: `OecsCApi.cpp` enumerate çağrısı ve tam `MV_CC_DEVICE_INFO` kopyasını
  süreç genelindeki mutex altında yapar. Instance, SDK adresi yerine değer kopyası
  saklar; create bunu kullanır. Mevcut üretim kodunda diğer C API enumerate yolu yoktur;
  bağımsız probe ayrı süreçtedir. Gelecekteki SDK enumerate yolları aynı kilide katılmalıdır.
- P2: setup alan farkları revision üretiminde ayrı yardımcıyla değerlendirilir;
  `acquisitionChange` ayrıca setup revision farkını reddeder. A→B→A rev1→rev2→rev3
  geçmişi kalibrasyon/cache/preparation uyumunu geri getirmez. `revised`, çağıranın
  revision değerini kullanmaz; rename/no-op ve legacy codec/hash korunur.

## Değişen dosyalar

- `src/hardware/OecsCApi.cpp`: owned bilgi ve enumerate/kopya kilidi.
- `src/domain/MeasurementSetupContract.cpp`: revision üretimi/uyumluluk ayrımı.
- `tests/oecs_test_sdk.cpp`, `tests/oecs_binding_test.cpp`, `CMakeLists.txt`:
  yalnız yerel SDK başlığıyla derlenen fixture DLL ve gerçek dinamik binding testi.
  Vendor başlığı veya binary repoya kopyalanmadı; SDK'sız derleme korunur.
- `tests/measurement_setup_test.cpp`: A→B→A, iki yönlü ret, yüksek revizyonda
  rename/no-op ve mevcut geometri değişim beklentisi. Legacy golden testleri korundu.
- `docs/architecture/industrial-camera-contract-plan.md`: iki sözleşme açıklaması;
  `docs/progress.md`, `docs/archive/README.md` ve bu kapanış kaydı.

## Yeni doğrulama ve kanıt

Kanıt kökü: `build/icm/review-01-fix/` (commit dışında).

- README toolchain PATH'iyle Release tam build: `build/kmc9-core-off` ve
  `build/reference-ui`, exit 0; loglar `core-build.log`, `ui-build.log`.
- Her ikisinde hedefli CTest **6/6**: setup contract, setup migration, project
  persistence, camera transport, backend lifetime ilk koşumda 5/5; environment
  persistence ayrı koşumda 1/1. İlk regex environment adını tam karşılamadığı için
  bu test ayrıca çalıştırıldı. Loglar `core-tests.log`, `ui-tests.log`,
  `core-environment.log`, `ui-environment.log`; her build'de
  `icm-review-01-fix-tests.xml`, `icm-review-01-fix-environment.xml`.
- SDK'lı Release `build/icm/04a/sdk`: binding fixture/test, session ve setup test
  hedefleri derlendi, exit 0; CTest binding/lifetime/setup **3/3**. Loglar
  `sdk-build.log`, `sdk-tests.log`; JUnit bu build'de `icm-review-01-fix-tests.xml`.
  Gerçek vendor DLL enum/open testi bu teslimde çalıştırılmadı.
- Yeni binding testi iki instance'ın enumerate/create sırasını SDK'nın ortak
  buffer'ını yenileyen DLL üzerinden sınar; serial ve ek ABI alanının korunduğunu,
  null entry/index eşlemesini, canlı handle unload reddini ve iki worker'ın toplam
  80 enumerate/create/destroy çevriminde enumerate çakışması olmadığını doğrular.
- Önceki izole probe mevcut üretim kaynaklarıyla yeniden derlenip çalıştırıldı:
  `selected=CAMERA-A created=CAMERA-A`; revision 1→3, farklı geometryId,
  calibration/cache/preparation uyumları 0. `reproduction-build.log`, `reproduction.log`.
- `git diff --check` ve `python scripts/check-context-docs.py` kapanış kontrolüdür.

Son kaynak kontrolünde mutex anonim namespace içine alındı. Üç yapılandırma yeniden
derlendi (exit 0); core/UI hedefli altı testin tamamı tek koşumda **6/6**, SDK seçimi
**3/3** geçti. Son kanıt `core-final-build.log`, `ui-final-build.log`,
`sdk-final-build.log` ve karşılık gelen `*-final-tests.log`; her build'de
`icm-review-01-fix-final-tests.xml`. Context kontrolü ilk denemelerde progress boyutu
nedeniyle reddetti; yinelenen tarihsel ayrıntılar arşiv bağlantıları korunarak kısaltıldı.
Son `context-check.json`: ok=true, progress 8.000 bayt altında.

Tam suite yeniden koşulmadı; önceki aralıklı `plc_socket_runtime` kök nedeni açık.
UI kodu değişmedi; görsel kontrol gerektiren bir UI teslimi yoktur. Makine ayarları,
driver/reboot ve gerçek cihaz değiştirilmedi. Bu fixture başarısı fiziksel kamera,
kurulum/dağıtım veya production kabulü değildir.

## Devam

Sıradaki görev ICM-04b configure/readback; gerçek readback cihaz kanıtı ister.
ICM-01b ve G0–G4/H01–H08 fiziksel kapıları ile mevcut diğer açık sınırlamalar
progress'te korunur. Push yapılmaz.
