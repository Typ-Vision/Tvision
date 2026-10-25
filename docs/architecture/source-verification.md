# Kaynak doğrulama ve kamera sahipliği

25 Ekim 2026, INT-05. A0–E2 B3 application komutunun CameraIntegration kamera
sahipliğine uyarlanmış sözleşmesidir. Kullanıcı akışı (Runtime “Kaynağı doğrula”
düğmesi, readiness tablosu) INT-06'dadır; bu belge yalnız application katmanını tanımlar.
[INT politika matrisi](camera-integration-int02-matrix.md),
[edinim oturumları](camera-setup/acquisition-sessions.md).

## Plan (owner thread)

`MeasurementWorkspace::verifySources(currentRecipe, directory, backend)` güncel
reçeteyi her denetimde callback ile owner'da okur ve `verificationPlan` üretir.
Strict DAG'nin enabled required kaynakları incelenir:

- Ham dal probe/model istemez, kardeş kaynağın modelini almaz. Dosya rasterı salt
  okunur decode edilir; bozuk dosya reddedilir. Metrik replay gerçek çözünürlük ve
  model uyumunu, experimental replay ayrıca typed köken/geometri/readback'i doğrular.
- Metrik dal exact `SetupReference` ve `before.selections` içindeki kalıcı seçimi ister;
  model `ScopedActiveCalibrationService::configuration` ile salt okunur çözülür, fit yoktur.
- Normal OECS kamerası (`AcquisitionPolicy::camera`) experimental sınıftadır; webcam
  probe'una gönderilmez. İstek `Automatic` politikayla ortak independent capture yolunu kullanır.
- Aynı fiziksel endüstriyel kameranın aynı setup/profil revizyonlu logical kaynakları
  **tek probe ve tek yeniden izin** paylaşır; readiness/hata her kaynak kimliğine dağıtılır.
  Aynı kamerada farklı setup/profil hazırlıktan önce reddedilir. Webcam için kanıtlı
  reuse politikası yoktur: tekrar eden webcam/legacy backend-index reddedilir.
- Metrik dosya replay canlı fiziksel izni yenileyemez. Güncel oturum izni varsa probe
  olmadan readiness'e katılır; yoksa açık “desteklenmeyen” hatasıyla plan reddedilir.
- Fingerprint: proje, yol, canonical reçete kimliği, environment, model kayıtları,
  replay piksel hash/geometri/format/kökeni ve varsa bounded sidecar hash'i.
  Dosya veya sidecar değişimi Ready'yi ve bekleyen completion'ı geçersiz kılar;
  düzeltmeden sonra açık doğrulama gerekir. Dosya denetimi canlı izin üretmez.

## Worker, owner ve drain

`SourceVerificationService::begin` gate (Runtime/preview/worker/pending kayıt/ACK) ve
kendi sahipliği boşsa her probe için bir `IndependentCalibrationSession` açar. Fiziksel
kamerada `prepareCamera` ortak `CameraAcquisitionSessions` bağlamını kullanır; ikinci
SDK sahibi veya kare başına aç/kapa eklenmez. Worker yalnız statik `run(job, backend)`
çağırır; workspace, reçete callback'i veya widget tutmaz.

`accept(result)` yalnız güncel ticket'ı tüketir; geç, tekrar veya eski generation
completion'ı `false` döner ve durumu değiştirmez. Accept probe sonuçlarını alır,
bağlamları kapatır ve `poll()` gerçek drain'i bekler. Drain bitmeden `busy()` sürer;
kapanış bütçesi aşımı sahipliği bırakmaz. İzin yalnız drain sonrası, gate ve
fingerprint yeniden denetlenerek `reauthorizeRemembered` ile yenilenir. Başlangıçta
alınan kamera/policy invalidation token'ları drain sonunda ve Ready okunurken de
karşılaştırılır. İş/kapanış sırasında gelen disconnect veya policy A→B→A eski
kareyi emekliye ayırır; başlangıçtan önceki izinsizlik yeni kareyle giderilebilir.
Kirli kapanışta izin yenilenmez. Token/izin diske yazılmaz.

Durumlar `Waiting/Verifying/Ready/Failed/Cancelled`, coordinator execution state'inden
ayrıdır. Hata haritası kaynak kimliğiyle, set hatası `sourceSet` anahtarıyla tutulur.
Ready her okunuşta fingerprint ve güncel izin snapshot'larıyla yeniden denetlenir; değişimde
`Waiting` döner. Kısmi başarıda bireysel izinler korunabilir ama bütün set kabul edilmez.

## Admission ve Stop

Komut hiç çağrılmamışsa mevcut aynı-oturum aktivasyon yolu değişmez. Çağrıldıktan
sonra `productionSources` ve (bağlanırsa) `RuntimeService::setSourceVerification`
üzerinden prepare/requestCameraPreparation aynı reçete için tam Ready ister;
begin/requireReady çalıştırılacak immutable snapshot'ın reçete/dizin kimliği için
Ready ister. Tek kaynaklı sistem modeli eklenirken türetilen binding alanları için
özgün reçete kimliği bounded Runtime validation geçmişiyle bellekte eşlenir;
rollback başka reçetenin Ready'sini kullanamaz. Doğrulama sürerken bunlar ile activate/rollback
`Busy` döner. Offline Load (`checkConfiguration`) etkilenmez. Workspace mutasyonları,
keşif ve scoped aktivasyon doğrulama sahipliği sürerken reddedilir.

`RuntimeCoordinator::stop` devam eden doğrulamayı kooperatif iptal eder; SDK çağrısı
zorla kesilmez. Sahiplik worker dönüp drain bitene kadar sürer ve proje değişimi
(`replaceable`) bu sürede reddedilir. Coordinator worker/gerçek drain bitene kadar
`Stopping` kalır; owner `poll` tamamlanınca durur. Tamamlanmış Ready normal Stop ile iptal edilmez.

Doğrulama `ProductionCycle`, aktif production snapshot, terminal kayıt, model fit,
PASS/skor/robot pose üretmez. Fixture testleri fiziksel kamera/mm kabulü değildir.
