# Transactional legacy migration — COMM4-15

`application/CommunicationMigration.hpp/.cpp` uygulama portudur. UI/normal main
bu portu henüz çağırmaz; seçim dosyasını okuyacak composition COMM4-16'da
bağlanmalıdır. Mevcut v2/v3 okuyucular, project/profile serializer'ları ve eski
belgeler değiştirilmez. Migration listener, Run, Auto veya kamera açmaz.

## Analiz ve açık taslak

`CommunicationMigrationInput`: mutlak project/profile/registry/kurulu legacy
binary yolları, mevcut archive dizini, açık station UUID, gözden geçirilmiş
PublicationTemplate ve **disabled** v4 server profili. Binary dosyası yalnız
kimlik/yedek içindir; çalıştırılmaz. Frame/adapter UUID'leri kullanıcı tarafından
verilir; migration bunlara fiziksel uygunluk kazandırmaz.

`registryPath` çağıran composition'ın **güncel authority** catalogue yoludur;
legacy yedeği yeni kurulum/registry sayılmaz. Rollback sonrası yeni bir migration
taslağı hazırlanacaksa önceki v4 catalogue key/retired/epoch geçmişi authority
olarak taşınmalıdır; eski `legacy-registry.json` dosyasını seed olarak yeniden
seçmek uygun değildir. Port farklı workspace'ler arasında registry authority
seçimini yönetmez; bu sahiplik normal main/COMM4-16 entegrasyonunun sorumluluğudur.

`CommunicationMigrationPlanner::analyze()` gerçek ProjectRepository,
CommunicationProfiles ve registry serializer'larını okur. Kaynakların exact
byte SHA256/uzunluk/yolunu, archive JSON envanterini, pending listesini,
UUID→numeric catalogue ve field→slot eşlemesini raporlar. Analysis kalıcı config
yazmaz; journal okuyucuları geçici QLockFile kullanabilir. Başlangıç/son kaynak
ve archive karşılaştırması eşzamanlı değişimde analizi reddeder.

Kayıpsız otomatik aday: bir enabled publisher/grup, required delivery,
Boolean→Flag veya Int32→Number, aynı unit ve identity scale/full Int32 range.
Legacy alanların hepsi valid olmak zorundaydı; karşılık gelen slotlar required
olmalıdır. Aynı değişkene ikinci Set Variables yazarı varsa producer bundle ile
son çevrim değişkeni farklılaşabilir: otomatik aktarım bloklanır. Eski fiziksel
destination alanı raporda tutulur; v4 slot eşlemesi açık review ister.

Fan-out, optional delivery policy, >64 byte sabit veya sınırı belirsiz dinamik
String, birden çok Pose/pose frame-adapter dönüşümü, Float/UInt/Int64/Enum
dönüşümü, kapasite aşımı, credential policy, stale profile, inaktif publication
ve mevcut v4 assignments unsupported olur. Sessiz truncation veya scalar→pose
yoktur. Bunlar ayrı düzeltilmiş taslakla yeniden analiz edilir; unsupported
raporu olduğu sürece activate çalışmaz. Geniş legacy 64 KiB paket yolu v4'e
taşınmaz; sabit slot sınırları uygulanır. V2 scalar protokol beyanı açık taslak
ister; v2 event geçmişi ayrıca incelenir.

Aday project'te UUID'ler korunur, ilgili recipeVersion artar ve status Draft
olur. Eski Publish Data düğümleri/dependency kimlikleri korunup disable edilir;
legacy publication yerine typed v4 assignments yazılır. Registry kopyası mevcut
installation/key/retired/epoch geçmişini korur; yeni station/template/recipe
revision kimlikleri yalnız yeni numeric key alır. Orijinal registry değişmez.

## Backup, staging ve atomik seçim

1. Runtime/capture/drain tamamlanır, listener'lar kapatılır. Başka binary aynı
   archive/socket üzerinde çalışmamalıdır. Bu operasyonel koşul eski kurulu
   binary yeni bakım protokolünü bilmeyebileceği için özellikle zorunludur.
2. `CommunicationMigrationTransaction` Qt owner thread'inde oluşturulur.
   Process-local açık legacy/v4 listener reddedilir. Workspace migration.lock ve
   **archive'ın gerçek archive.lock** kilidi nesnenin tüm ömrünce tutulur.
   ProductionStore'un tüm v2/v3/v4 journal append'leri bu sırada fail closed
   olur. Kilitler başka process/transaction yazıcısını engeller; başka process'in
   kapalı olduğunu tek başına ispatlamaz. Çağıran bakım ömründe listener/Runtime
   başlatmamalıdır.
3. Kullanıcı raporu gözden geçirir; `prepare(packageUUID, reviewedPlanDigest)`
   aynı analizi tekrar karşılaştırır. Yeni, daha önce kullanılmamış klasörde
   legacy project/profiles/registry/binary exact yedekleri ve aday project,
   registry, template, disabled server dosyaları atomik yazılır. Manifest en son
   commit edilir. Unsupported rapor için de backup alınabilir.
4. Manifest SHA256 review sonrası `activate(package, manifestDigest,
   expectedPointerDigest)` explicit uygulanır. Gerçek serializer'lar adayları,
   bütün dosya hash'leri backup'ları doğrular. Kaynak/ledger/review değişmişse veya
   v2/v3/v4 pending varsa ret verir. V3 intent/begin ve ACK_RECEIVED de pending'dir;
   v4 normal/control tüm unresolved kayıtları engeldir. V2 PLC_REQUEST/RESULT
   session+request anahtarı PLC_ACK olmadan reconcile ister.
5. `active.json` tek QSaveFile commit ile yeni revision/package/manifest/archive
   seçer; direct-write fallback kapalıdır. Pointer yokken legacy aktiftir.
   `active()` bozuk pointer/paket için eski config'e sessiz fallback yapmaz.
   Staging yarım kalırsa seçilemez. Eski aktif pointer swap tamamlanana kadar
   aynen kalır; yeni pointer yalnız tamamlanmış doğrulanmış paketi gösterir.

Paket uygulama tarafından üzerine yazılmaz; manual bozulma SHA256 ile reddedilir.
Bu protokol QSaveFile'ın dosya rename/commit garantisi kapsamındadır; fiziksel
disk/controller güç kaybı, dosya sistemi dayanıklılığı ve dağıtım kabulü değildir.

## Drain ve rollback rehberi

Legacy pending varsa mevcut kurulu v3 binary/profil ile exact ACK drain alınır;
v2 intent için session geçmişi ve operatör reconciliation gerekir. Migration
pending kaydını silmez, ACK uydurmaz veya eski parser'ı v4 altında çalıştırmaz.
Drained kanıttan sonra yeniden analyze/prepare/review yapılır.

V4'ten geri dönmek için önce Runtime ve listener'ı durdurun, transaction bakım
kilitlerini alın, aktif revision'ı ve **güncel** `ledgerDigest()` değerini
inceleyin. `rollback(expectedPointerDigest, reviewedLedgerDigest)` binary/config
hash'lerini immutable backup ile, installation/archive yolunu manifest ile ve
v2/v3/v4 pending durumunu gerçek journal ile yeniden doğrular. Pending v4 işi
varsa önce v4 binary ile drain/reconcile gerekir; kör restore reddedilir.

Rollback yalnız yeni revision'lı legacy pointer commit eder. Live archive
yerinde kalır; yeni v4 ACK/tombstone dahil hiçbir journal/dedup kopyalanmaz,
silinmez veya eski yedeğe döndürülmez. Legacy original dosyaları backup kimliğiyle
eşleşmiyorsa rollback durur; backup'tan kontrolsüz config restore yapılmaz.
Composition legacy seçimini yüklerken manifestteki doğrulanmış backup/binary
kimliğine uymalıdır. Kilitleri bırakmadan yeni binary/listener başlatmayın.

## Test sınırı

`communication_migration_contract`: gerçek serializer fixture'ları, byte/no-op,
Boolean/Int32, unsupported text/fanout/multi-Pose, stale kaynak/manifest/pointer,
pending v2/v3/v4, exact ACK sonrası tombstone koruyan rollback, gerçek archive
yazıcı kilidi ve açık listener reddi. Ayrı child process `_Exit(73)` ile 38 nokta:
sekiz dosyanın half/written/committed sınırları, manifest sınırları ve aktivasyon/
rollback pointer half/written/before/after sınırları. Restart eski/yeni tutarlı
seçimi okur. Disk-full/yazma hatası exception'ları aynı atomik commit yolunda
fail closed kalır; fiziksel disk-full/güç kesintisi yapılmış sayılmaz.
