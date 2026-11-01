# COMM4-21 — SAMPLE pose ve yeni frame eşleştirmesi

`CalibrationSampleCoordinator`, yetkilendirilmiş application dispatch'inde
COMM4-20 session otoritesi ile COMM4-13 `StationDeliveryLedger` arasında çalışır.
Main/UI kalibrasyon composition'ı COMM4-24 kapsamındadır. Bu portun varlığı
normal üründe native kamera veya fiziksel hand-eye izni açmaz.

## Sahiplik ve sıra

1. Codec tek SAMPLE içindeki actual pose, session/sample UUID, camera/revision,
   profile key, stationary, robot sequence/clock ve solve/holdout rolünü doğrular.
   Raw request fingerprint'i korunur; aynı ID ile quaternion işareti dahil byte
   değişikliği `CONFLICT` olur. Bu sınır generic `base_T_flange` tüketir; controller
   dönüşümü COMM4-19 adapter'ında yapılır. Burada `base_T_tcp` sessiz dönüştürülmez;
   authoritative offset composition bağlanana kadar `CONTRACT_MISMATCH` döner.
2. Frozen BEGIN header, station/peer/epoch, frame/revision ve güncel dependency
   snapshot exact karşılaştırılır. BEGIN raw bytes session store'a eklenmiştir;
   bu alanı olmayan eski kayıt SAMPLE açamaz, explicit recovery/migration gerekir.
3. Request, accepted response ve fingerprint ortak bounded ledger'da INTENT/BEGIN
   olarak kalıcılaşır. Pairing intent ayrıca `ProductionStore/calibration_sample`
   namespace'inde monoton yerel token, benzersiz camera-session UUID ve raw request
   ile yazılır. Ancak bu bariyerden sonra session COLLECTING olur ve yeni opaque
   `IndependentCaptureJob` oluşturulur. Preview/cache rasterı giriş kabul edilmez.
4. `run(job)` mevcut `IndependentCalibrationSession::run` ve acquisition portlarını
   worker'da çağırır. COMM4-20'nin aynı lease Ticket'ı industrial worker'a aktarılır;
   ikinci rezervasyon/registry yoktur. Job capability'yi weak tutar: eski bir job
   tamamlanmış session'ın lease'ini canlı tutamaz. Wizard'ın Automatic/Experimental
   OECS politikası SAMPLE'a taşınmaz; bu yol Trusted kullanır.
5. Owner-thread `complete` hem coordinator token/camera-session/sample/generation
   hem independent capture'ın opaque ticket/generation değerlerini doğrular.
   Yanlış veya eski completion yeni sample'ın ticket'ını tüketmez.
6. Başarılı frame için UUID, pixel hash, raw raster, boyut/stride/format ve rol
   immutable pairing kaydına yazılır; ardından ledger terminal seal edilir.
   Terminal cameraKey, originating CALIB_SAMPLE, session/sample/frame UUID ve yerel
   capture tick'i exact taşır. Decision notEvaluated kalır; resultValid fiziksel
   kalibrasyon izni anlamına gelmez. Raster üst sınırı 12 MiB; JSON/archive dosyası
   mevcut 20 MiB sınırını korur. Yeni namespace mevcut 512 MiB/10.000 kayıt
   bütçesine dahildir; normal/control ve pairing headroom kabulde ayrılır.

## Duplicate, handshake ve recovery

- Aynı persistent request yüz kez tekrarlandığında aynı accepted/terminal döner;
  yeni job/hardware komutu oluşmaz. Reconnect yalnız transport session envelope'ını
  değiştirir. ACK sonrası da aynı session/sample UUID başka request'e bağlanamaz.
- ACCEPTED, reject veya timeout robotun capture beklemesini bitirmez. Yalnız sealed
  terminal `CAPTURED=6` veya `CAPTURE_FAILED=7` bu handshake anlamını taşır. Motion
  komutu üretilmez. Robot fixture bu enumları kullanır; fiziksel robot kanıtı yoktur.
- Timeout, worker failure veya ambiguous storage failure admission'ı fence eder.
  Geç raster yeni sample'a atanmaz. `recover(session, CalibrationDrainPort&)`
  gerçek worker/hardware drain onayı bekler; false/exception terminal üretmez.
  Drain sonrası acquisition tekrarlamadan CAPTURE_FAILED seal edilir. Daha önce
  durable terminal varsa immutable byte'ları korunur.
- Restart unresolved INTENT/BEGIN kayıtlarını recovery-required bulur; yeni
  coordinator job başlatmaz. INTENT/BEGIN recovery ancak explicit drain'den sonra
  hata terminaline ilerler. Terminal/PENDING sonrası fault yeni capture yaratmaz.
  Yetkilendirme/lease geri alma ayrıca COMM4-20 session recovery'sinin işidir.
- ABORT session callback fence'ini kapatır; gerçek drain lease bırakma koşuludur.
  Dispatch host outstanding worker'ları `CalibrationDrainPort` üzerinden izlemelidir.
- Frame pairing ve terminal ayrı durable yazımlardır. Terminali olmayan orphan
  pairing solve dataset'i değildir; dataset okuyucusu yalnız ledger'da CAPTURED
  terminali olan exact eşleşmeleri tüketmelidir. Raw actual pose ledger request'inde
  durur; raster hash'i bağımsız yeniden üretilebilir.

## Solve/holdout ve kapsam

Rol SAMPLE raw request'inde ve frame kaydında ayrı saklanır. Candidate state'inde
yalnız yeni holdout kabul edilir; candidate/active model değişmez. Solver22 ve
validation/activation23 bu kayıtları role göre ayırmalıdır; bu kart solve yapmaz.

`softwareFixture=true` yalnız in-memory test composition seam'idir; ayar/wire
üzerinden okunmaz. Varsayılan composition exact physical camera ister; normal
factory fixture açamaz. Provider fixture shared lease/worker/correlation yazılımını
doğrular. Robot/controller/TCP, lens/intrinsics/plate, bağımsız reference/tolerans
ve native OECS cancel/drain/frame-correlation fiziksel kapıları açık kalır.

Kabul: `calibration_sample_coordinator_contract`; ortak komşular session, codec,
ledger, acquisition/lease ve bağımsız industrial capture testleridir. Son koşum
ve bilinen sınırlamalar tarihli [kapanış kaydındadır](../../archive/2026-11-02-comm4-21.md).
