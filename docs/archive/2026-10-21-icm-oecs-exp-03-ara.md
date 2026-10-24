# ICM-OECS-EXP-03 — ara devir

21 Ekim 2026. Uygulayıcı: Codex.
Kullanıcı kısa ara istedi; çalışma durduruldu. Görev tamamlanmadı, commit/push yok.

## Çalışma ağacı

Yeni ExperimentalRuntime.{hpp,cpp}: ayrı ephemeral experimental_runtime/1 snapshot,
production alanlarından ayrı private completion; experimental_cycles arşivi ve
sınırlı offline codec/CSV. RuntimeService, Coordinator, ProductionStore, PLC socket,
MeasurementWorkspace ve ortak Runtime/Logs UI bağlantıları değişti. Completion
digest frame/port experimentalAcquisition alanını da kapsıyor. README kullanım,
architecture/experimental-runtime.md teknik sözleşme taslağıdır; kabul beyanı değildir.
EditorUsabilitySmoke mevcut deneysel replay koşumuna Runtime/Logs görselleri ekler.
oecs_experimental_test üç applicationType, retry/Stop/restart/provenance/PLC
negatiflerini; probe --runtime-run/--runtime-stop/--runtime-timeout seçeneklerini ekler.
Tam dosya listesi git status/diff ile görülür; başlangıçta çalışma ağacı temizdi.

## Yeni koşumlar ve açık hata

- Release build build/reference-ui tamamlandı: build/exp-03-build-ready.log.
  Önceki build loglarında eksik include hataları vardı; giderildi.
- Son targeted build: build/exp-03-build-targeted.log (test yeniden derlendi).
- Sandbox CTest: build/exp-03-ctest-first.log ve exp-03-ctest-retry.log başarısız;
  socket HELLO timeout. Sandbox dışında socket ilerledi, fakat test NOT_RUNNING
  hatasında kaldı: build/icm/oecs-exp-03/ctest-experimental-exact.log.
  Muhtemel sonraki inceleme: ExperimentalRuntime::requireReady busy durumunu
  InvalidCompletion ile fırlatıyor; coordinator.begin bunu Fault yapıyor. Testin
  busy negatifinden sonra coordinator Running kalmalı; doğru Busy türünü kontrol et.
  Bu henüz düzeltilmedi veya yeniden doğrulanmadı.
- Sandbox gerçek probe --runtime-run 1 başarısız:
  build/icm/oecs-exp-03/live-sandbox.json. Hata metni boş Runtime capture failed;
  probe tool mesajlarını da raporlamalı. Donanım yokluğu/tetik başarısı sonucu çıkarma.
  live-first klasöründe proje oluştu; tekrar koşum yeni kanıt klasörü ister.
- 1000 gerçek Runtime çevrimi, Runtime timeout/Stop/kopma, görsel inceleme ve ilgili
  regresyon CTest henüz koşulmadı. Önceki EXP-01/02 başarıları yeni koşum değildir.

## Sonraki adım ve sınırlar

Önce busy/fault testini düzelt, aynı CTest’i localhost erişimiyle tekrar doğrula.
Ardından gerçek cihaz probe hata ayrıntısı ve sandbox dışı 1 → 1000 sıralı koşum,
Stop/deadline, offscreen deneysel Editor+Runtime smoke ve PNG incelemesi; ilgili
production terminal/coordinator/socket/industrial/UI regresyonları. Son değişiklikle
Release build ve context-docs kontrolü; güncel sonuçlarla kapanış kaydı ve tek görev
commit’i. Bu ara kayıt tarihsel olarak korunacak; son kapanış ayrı dosya olmalı.

Mevcut deneysel Runtime taslağı Raw/px ile sınırlı; mm/deneysel kalibrasyon kökeni
açık. PLC/robot ve trusted production yükseltmesi kapalı kalmalı. Native Unavailable,
8/8 eski kare karşı örneği, G0/G1/G3/G4 ve dağıtım sınırları korunur. Kullanıcı ara
isteğinden sonra çalışma sürdürülmedi; yeniden devam yönlendirmesi beklenir.
