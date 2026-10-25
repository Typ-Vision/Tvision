# Runtime tanı kaydı — INT-03

25 Ekim 2026. A1 (`230cc72`) mevcut kamera oturumlarına uyarlanmıştır.
Tanı capture, izin, terminal şeması veya PLC/robot yetkisi oluşturmaz.

`RuntimeDiagnostics` servis ömrü boyunca session ID, mutex korumalı son 256 olay
ve halkadan silinmeyen ilk failure tutar. Busy ve iptal failure değildir. Yeni
Run/retry ilk tanı failure'ını silmez; yeni servis yeni tanı oturumudur. Hata metni
2048 karakterle sınırlı; görüntü/piksel/port payload'ı taşınmaz. Manifest gerçek
çalışan EXE yolu/SHA256, build tipi, Widgets/ZXing, Qt ve derleyiciyi içerir.

Coordinator load/requestStart/completeStart/begin/Stop/finish/delivery olayları
stateBefore/After, generation, son kabul edilen ticket, trigger, kayıt/ACK ve
kamera pending/draining bilgisini taşır. Değişmeyen poll halka doldurmaz.
Asenkron kamera hazırlık/kapanış hatası ilk nedenle kaydedilir; Starting ve gerçek
Stopping/drain davranışı değişmez. Recipe kimliği/sürümü/schema/applicationType ve
enabled file/webcam/industrial source listesi olay bağlamıdır.

Admission yalnız kabul edilen ticket'ı verir; worker acquisition/completion ve
record aynı ticket/cycle/source/frame ile ilişkilidir. Automatic dosya politikası
ön kontrolde reddederse typed source ID, completion üzerinde yalnız private tanı
bağlamıdır; `policyPreflight/attempted=false` olayı kare veya acquisition sayısı
uydurmadan ilk kaynağı korur. Execution/terminal/digest/payload anlamı değişmez. Experimental execution da
aynı tanı yolunu kullanır; experimental terminal/trusted ayrımı korunur. İlk hata
`firstFailureSequence` ile izlenir. Scoped disconnect mevcut cameraEpoch ve izin
silme davranışını koruyarak ayrı permissionInvalidation olayı yayımlar; mevcut
Webcam acquisition invalidation'ında source/cycle/errorCode da bulunur. Yeni bir
N-source invalidation politikası eklenmemiştir.

Qt `visionflow.runtime` logging kategorisi compact JSON yayımlar. Diskte rolling
log veya crash recovery garantisi yoktur. İstenirse launcher çıktısı build altında
saklanabilir (README'deki DLL/toolchain ortamı):

```powershell
$env:QT_FORCE_STDERR_LOGGING = '1'
$env:QT_LOGGING_RULES = 'visionflow.runtime.info=true'
.\scripts\run-local.ps1 *> build/runtime-user-diagnostics.log
```

`runtime_diagnostics` fixture'ı gerçek dosya yürütme, Webcam read hata enjeksiyonu,
foreign completion, disk kilidi, dependency failure, EXE hash doğrulaması ve
bounded ring'i sınar. Runtime/session/PLC testleri kamera drain ve frozen retry
karşılıklarını kapsar. [Kurtarma](runtime-error-recovery.md),
[INT-03 kanıtı](../archive/2026-10-25-int-03.md).
Fiziksel süreklilik/C2 kök nedeni bu tanıyla çözülmüş sayılmaz.
