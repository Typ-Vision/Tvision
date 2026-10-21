# ICM-OECS-EXP-03 — deneysel Runtime sözleşmesi

21 Ekim 2026. [Deneysel kamera planı](industrial-camera-oecs-experimental-plan.md).

Ortak Runtime'daki oturuma özel seçim `experimental_runtime/1` admission üretir.
Seçim reçeteye/INI'ye yazılmaz. Mod değişimi önce Stop/kayıt/teslimin bitmesini
ister, yüklü controller'ı ve sonucu temizler; yeniden Load Recipe → Run gerekir.
Load graph beyanıdır, Run configuration snapshot'ıdır; ikisi de görüntü üretmez.
Tek çevrim ortak ExecutionService ile edinim ve DAG yürütür. Üç applicationType
aynı yolu kullanır. Mevcut teslim yalnız editor9 Industrial Camera / Image File
ve Raw/px içindir. Deneysel kalibrasyon kökeni tamamlanana kadar mm isteyen
reçeteler Run'da reddedilir; eksik model yerine değer veya PASS üretilmez.

Canlı giriş registry/profile/sourceReference bağını ve arşiv durumunu denetler;
yerel rota seçimini kullanır. Exact kimlik, gerçek configure/readback, owned
2448×2048 Mono8 ve tek istek/lease/deadline kuralları EXP-01/02 ile aynıdır.
Deneysel edinim özel Editor lease sınıfını kullanır; Runtime production lease'i
ve admittedDevices taşıyamaz. Snapshot'taki kamera/kurulum/profil begin'de
yeniden kontrol edilir. File snapshot gerçek piksel hash'ini ve varsa PGM+JSON
kökenini dondurur; yürütmede değişen dosya/köken completion olarak kabul edilmez.

Deneysel completion'ın private snapshot/ticket/digest alanları production
completion alanlarından ayrıdır. Digest, frame/port kökenini, gerçek raster
hash'ini, araçları ve ölçüm tanısını kapsar. Editor sonucu, başka ticket,
sonradan değişmiş çıktı veya köken kaydı kabul edilmez. Stop geç completion'ı
doğruladıktan sonra kare/araç çıktısı/tanıyı temizler; SKIPPED kayıt kalabilir.
ERROR veya iptal yeni çevrimi kapatır; açık manuel Run gerekir.

Kalıcı kayıt `visionflow.experimental-runtime1` / schemaVersion=1 olup
ProductionStore'un ayrı `experimental_cycles` grubuna atomik yazılır. Terminal1–9
codec'i bunu reddeder; trusted production kaydına çevrilmez. Mevcut 16 MiB kayıt,
20 MiB dosya, 512 MiB toplam, 10000 dosya, QLockFile ve idempotent append sınırları
deneysel grubu da kapsar. Kayıt hatasında payload ve completion dondurulur;
tekrar yazma edinim/algoritma çalıştırmaz. Başarılı commit öncesi UI sonuç yayımlamaz.
Kayıt beklerken Stop verilirse dondurulmuş kayıt değişmez; başarılı tekrar yazma
canlı Viewer/sonuçları geri getirmez. Geçmişte gerçek completion korunur.

Offline okuyucu graph/kaynak/frame/port, piksel hash bağları, profile/readback,
false admission/eşleşme bayrakları ve NOT_EVALUATED kararını yeniden denetler.
Logs deneysel etiketi ve tam JSON'u gösterir. Ayrı CSV, her çevrimin sınırlı
JSON kanıtını bir satırda taşır; ERROR/Stop için scalar uydurmaz. Raster gömülmez.
Dosya hash'i dijital imza değildir; dışarıdan bütünü yeniden yazılan JSON'a karşı
kimlik doğrulama veya tetik/parça tazeliği kanıtı sağlamaz.

Deneysel mod yalnız manuel çevrim kabul eder. Communication içeren begin ve
socket TRIGGER reddedilir; socket READY/DONE/scalar/PLC_RESULT üretemez. Robot
pose/otomatik parça çıkışı açılmaz. Restart yalnız geçmiş okur; seçim, aktif
snapshot veya completion yetkisini geri yüklemez.

Test ve donanım koşumlarının güncel kanıtı [devir özetinden](../progress.md)
bağlanan kapanış kaydındadır. G1/G3/G4, mm doğruluğu, fiziksel kopma, native DPI,
CUDA ve dağıtım kabulleri ancak kendi yeni kanıtlarıyla kapanabilir.
