# CCUX ortak uygulama sözleşmesi

Bu metin **hedef davranıştır**; uygulanmış API diye okunmaz. Mevcut KMC/ICM
katmanları genişletilir. Değişen kuralın eski sözleşmesi ilgili kartta güncellenir;
bütün kurallar her karta veya README'ye kopyalanmaz.

## C0 — 22 Ekim sade ürün akışı kararı

Kullanıcının güncel kararıyla bütün “Deneysel OECS” opt-in seçimleri ve tetik–kare
eşleşmesinin doğrulanmadığını söyleyen banner/tooltip/durum metinleri kaldırılır.
Aynı anlamda yeni bir mod, onay penceresi veya uyarı eklenmez. Kalibrasyon'un
“Önceki akış ve eski kanıt ekranları” gezinmesi kaldırılır. Normal kamera,
kalibrasyon, Editor ve Runtime komutları uygun backend politikasını seçer.
CCUX-16 normal komutlarda application politikasını uygular; görünür seçim/uyarı
ve eski gezinme temizliği CCUX-17–18, birleşik kabul CCUX-19'dadır.

Bu bölüm eski EXP/CCUX kartlarındaki açık deneysel seçim, restart'ta kutuyu yeniden
işaretleme ve kalıcı kamera uyarısı gereğinin yerine geçer. Kaynak başına kimlik,
uyum/probe, izin ömrü, hash, model bağı, immutable snapshot ve Stop/drain kuralları
korunur. Restart/proje açılışı capture veya Run başlatmaz. Kalıcı `experimental`
alanları/şema adları UI etiketi değildir; eski kayıtlar aynı anlamla okunur.
Tetik kanıtı, production admission veya bağımsız fiziksel kabul varmış gibi
üretilmez; PLC/robot çıkışı bu sadeleştirmeyle açılmaz. Donanım engelleri teknik
belgelerde ve kanıtta kalır. Gerçek işlem hataları ve kalibrasyon durumu gösterilir.

## C1 — Kimlik ve varsayılanlar

Kaydedilmiş proje gerekir, reçete gerekmez. Projedeki immutable CameraId ile
kullanıcının CAM_01 gibi görünen kısa ID'si ayrılır; kısa ID benzersiz, düzenlenebilir,
bağlar immutable ID ile kalıcıdır. Açıklama isteğe bağlıdır. Eski UUID yeniden
üretilmez. Fiziksel kimlik vendor/model/seri; USB sırası/Device ID/IP değildir.
Tek fiziksel kamerayı tekrar ekleme yeni kayıt üretmez; açıkça mevcut kayıt gösterilir.

Yeni kamera için tek varsayılan setup/profile application katmanında otomatik
oluşturulur. Lens/çalışma düzlemi kullanıcıya tekrar tekrar sorulmaz; bilinmeyen
fiziksel değerler doğrulanmış gibi doldurulmaz. Geometri gerçek readback'ten gelir;
eksikse kayıt “kurulum bilgisi bekleniyor” durumunda kalabilir, ölçüm etkinleşmez.
Kayıt + varsayılan ilişki atomiktir. Eski çok profilli projede mevcut açık seçim
korunur; belirsizlikte bir defalık seçim gerekir, ilk profile sessiz geçiş yok.

Yeni alanlar yalnız gerekli yeni kayıt/explicit migration yolunda sürümlenir.
Eski no-op encode/hash korunur. Import/Save As alias çakışması, varsayılan referans,
arşivlenmiş kamera ve ilişki bütünlüğü kapsam içidir. Yerel DLL/producer yolları
proje kimliği değildir. Bağlantı kurulumu/capability sonucu admission değildir.

## C2 — Ayarlar ve canlı test

Exposure/gain değerleri backend'in desteklediği aralık, birim ve geçerli başlangıç
değerinden gelir; bütün cihazlara sabit değer uydurulmaz. İstenen ve gerçek
uygulanan değer ayrıdır. Cihaza uygulama başarısızsa başarılı görünmez; rollback
belirsizse durum açık hata olur. Otomatik pozlama/gain politikası destek/readback
ile belirlenir. Yeni desteklenmeyen SDK eklemek bu paketin görevi değildir.

Normal UI: tara, ID/ad, exposure/gain, ekle/kaydet, tek görüntü, canlı görüntü,
durdur. Producer/rota/timeout/geometri ayrıntıları gerektiğinde açılır.
Tek uygun rota veya hâlen exact eşleşen hatırlanmış rota otomatik çözülür;
belirsizlikte anlaşılır seçim gösterilir. OECS için ayrıca onay seçimi yoktur;
normal işlem komutu C0 kapsamındaki application politikasını kullanır.

Canlı görüntü test önizlemesidir, dosyaya video kaydı değildir. Provider'ın gerçek
continuous desteği varsa kullanılır; yoksa tek bekleyen kareli sıralı edinimle
önizleme yapılabilir ve destek türü doğru etiketlenir. FrameBurstStart sessiz
FrameStart/FreeRun dönüşümü olmaz. En fazla bir bekleyen istek, en son kareyi tutan
sınırlı tampon; FPS gerçek tamamlanan karelerden. Kamera değişimi/proje kapanışı/
Stop/kopma eski generation'ı emekliye ayırır. UI SDK çağırmaz. Lease çakışması
önlenir; aynı kamera Runtime/kalibrasyon tarafından kullanılırken preview açılmaz.

## C2a — Çalışma süresince bağlantı

22 Ekim kullanıcı kararı; **CCUX-22 hedefidir**. Ortak application servisi 22b'de
uygulandı ([API/ömür](acquisition-sessions.md)); Editor bağlantısı 22c'de
uygulandı; Runtime bağlantısı 22d, Kalibrasyon bağlantısı 22e'de uygulandı.
Birleşik/fiziksel kabul 22f'de açık kalır.
Önceki her-kare kapanışına göre oturum sınırı aşağıdaki gibidir. C2'nin preview,
tek pending istek, lease ve generation kuralları korunur.

| Kullanıcı eylemi / context | Bağlantı ömrü |
| --- | --- |
| Program/proje açılışı, ekran restore, Runtime Load Recipe | Kamera bağlantısı açmaz |
| Reçeteye çift tıklama veya eşdeğer Editor açma | Etkin bağlı kameraları arka planda hazırlar; açık düzenleme işi boyunca tutar |
| Editor Kaydet, araç/ROI/Properties sekmesi | Oturum korunur |
| Editor'dan çıkma, başka reçete/proje, pencere kapatma | Çıkış kararı kabul edilince drain/close; çıkış iptali mevcut oturumu korur |
| Runtime Run | Yüklü immutable reçetenin gerekli kameralarını hazırlar; tümü başarılı olmadan Running olmaz |
| Runtime Stop | Starting/idle/çekim durumlarının hepsinde yeni isteği keser ve oturumları kapatır |
| Kalibrasyon listesi | Kamera açmaz |
| Seçili kamerada canlı kalibrasyon işlemine giriş | Hedef kamera açılır; aynı işlemdeki çekimler oturumu kullanır |
| Kalibrasyon başarılı bitiş, iptal, çıkış, kamera/dosya kaynağına geçiş | Mevcut oturum bırakılır; gerekiyorsa yeni hedef ayrı hazırlanır |

Bağlantı hazırlığı çekim veya sonuç üretmez; kullanıcı edinim komutu gerekir.
Kamera hazır olmasa da Editor çevrimdışı düzenlemeye izin verir. Bağlantı
beklemesi UI'yi durdurmaz. Kaydet tek başına çıkış değildir; kaydetmeden çıkış
da kapanış ister. Kalibrasyon kaydetme hatası işlemi açık tutuyorsa bağlantı
korunabilir; kamera/SDK hatası oturumu fault/drain'e geçirir.

Application servisi sabit owner thread ve fiziksel kimlik başına tek lease ile
erişimi düzenler. Ekranlar SDK çağırmaz. Context değişimi eski lease temiz
bırakılmadan yeni sahip açmaz; çalışan Runtime'ın kamerası alınamaz, önce Stop
gerekir. Runtime çok kamera hazırlığında bir hata bütün hazırlığı geri alır.
Aynı kameraya çelişen edinim profilleri sessiz seçilmez, hazırlık hatasıdır.

Oturum sağlıklı ve exact kimlik/rota/setup/profile değişmemişse sonraki çekimler
yeniden keşif/open/tam configure yapmaz. Kaynak/çevrim/frame metadata'sı her
istekte yenidir; cache yeni çekim yerine sunulmaz. Başarılı stream cleanup sonrası
kare yayınlanabilir; normal kare yayını için bağlantıyı kapatmak gerekmez.
Stop/çıkış/kopma generation'ı emekliye ayırır; geç completion yayınlanmaz.
Kapanış bütçesi aşımı kamera bırakıldı anlamına gelmez; lease karantinası korunur.
USB geri gelişi veya restart üretimi/çekimi kendiliğinden başlatmaz.

Hazır bağlantı kalibrasyon izni veya production admission değildir. C3/C4
invalidation ve immutable kuralları sürer. İlk uygulama OECS stream start/stop'u
kare başına korur; sürekli akış ve pending-command çözümü ayrı konudur.
[CCUX-22 planı](22-session-lifecycle.md) uygulama kartlarını ve kabulü tanımlar.

## C3 — Model geçerliliği ve kullanıcı akışı

Kalibrasyon sayfası kamera listesidir; alt bölüm seçili kameranın yöntemi, tarihi,
temel değerleri ve durumudur. Ayrım: “Kalibrasyon yok”, “Kayıtlı / bağlantı
bekleniyor”, “Geçerli”, “Yeniden kalibrasyon gerekli”.
Çevrimdışı kayıt yokmuş gibi gösterilmez. Değişmiş geometride eski model etkin
değer gibi gösterilmez; geçmişte kalır. Kamera A değişimi kamera B'yi bozmaz.

Kamera seçimi sihirbaz hedefini sabitler. Bilinen ölçü ve mevcut chessboard/circle
grid yöntemleri kullanılır; yeni solver gerekmez. Varsayılan kaynak kameradır.
Dosya denemesi güncel sihirbazın kaynak seçimiyle yapılır. Legacy kayıt uyumluluğu
korunur; önceki akış/eski kanıt ekranları için ayrı gezinme sunulmaz. Dosya
görüntüsü fiziksel edinim diye yeniden etiketlenmez.

Kaydet ve kullan: hesaplanmış adayın kayıt + uyum/kullanılabilirlik kontrolü +
etkin seçim işlemini tek kullanıcı komutu yapar. İptal, eski revision, yazma hatası
ve başarısız kontrol önceki modeli korur. Matematiksel başarı ile bağımsız fiziksel
kabul ayrı tutulur; doğrulama yapılmadıysa görünürdür. Mevcut model/holdout eşikleri
sessiz gevşetilmez. Preview gerçek sonuç değildir; sahte mm/PASS yoktur.

| Değişiklik | Hedef davranış |
| --- | --- |
| İsim/kısa ID | Bağ ve model korunur, no-op revision üretmez |
| Exposure/gain | Matematik korunur; görüntü/cache ve edinim snapshot'ı yenilenir; gerçek readback/uyum sonrası tekrar kullanılabilir, yeniden öğretim zorunlu değil |
| Lens/focus/konum/düzlem veya kritik geometri | İlgili model kullanımını kes; yeni kalibrasyon/uygunluk süreci, A→B→A eski izin canlandırmaz |
| Kopma/restart | Kayıt korunur, kullanım izni düşer; exact yeniden bağlantı/uyumla aynı matematik kullanılabilir; aktif üretim kendiliğinden başlamaz |
| Kalibrasyon değişimi | İlgili metrik downstream ve world ROI/reference yeniden denetlenir; başka kaynak etkilenmez |

Fiziksel lens/konum değişimi yazılımla algılanamayabilir: ayrıntılarda “Optik /
kamera konumu değişti” bildirimi bulunur. Geometriye etkili ayarlar sırf profil
adı altında oldukları için exposure gibi ele alınmaz. Restart uyum kontrolü
native capability ve güncel kimlik/probe/izin ömrü sınırlarını aşamaz.

## C3a — Çerçeveli plaka kalibrasyonu

23 Ekim hedefi; **CCUX-15e gözlem/model servis ayrımı uygulandı**
([API ve sınırlar](plate-observations.md)). Çerçeveli algılama/model/UI 15f–h
hedefidir; çerçeveli hedef şu an açık Unsupported döndürür.
[CCUX-15e–j planı](15-plate-calibration-plan.md) koyu çerçeveli, köşe işaretli
7×7 plaka için C3'teki mevcut plaka algılamasını genişletir. Lens solver'ı,
distorsiyon düzeltmesi, remap ve robot bu kapsamda yoktur.

Ham plaka gözlemi, raw planar model hesabı ve fiziksel kabul ayrı aşamalardır.
Algılama tüm görüntüde tam 49 blob şartı koymaz; çerçeve/işaret içindeki tam ve
benzersiz 7×7 merkez eşleşmesini arar. Fazla aday geometrik olarak elenebilir;
eksik merkezler üretilmez. Yeni profilde belirsiz yön model oluşturmaya yetmez.
Nihai merkezler özgün raw piksel koordinatlarında ve immutable kare kökeniyle
saklanır; geçici arama ROI/warp işlemleri raster kimliğini değiştirmez.

Gözlem üretmek lenssiz homografi fit başarısına bağımlı olmayacak; mevcut model
oluşturma yolunun 1 px inverse fit, sıra/tamlık, hull ve bağımsız kontrol
kapıları korunacak. Böylece ileride lens solver'ı ham gözlemi kullanabilir;
bugün kötü raw fit kalibrasyon kabulü olmaz. Gözlem başarısı SaveUse veya mm
izni değildir. UI tanı/overlay ile model ve fiziksel kabulü ayrı gösterir.

Yeni hedef/yön kanıtı gerekli şemalarda sürümlenir. Eski generic simetrik grid
ve chessboard kayıtlarının no-op/hash/yön anlamı korunur. Eski raw H gelecekte
düzeltilmiş piksele uygulanamaz; yeni koordinat uzayı ve lens bağı ayrı iştir.
Yeni çekim/hedef/aralık/arama ROI'si eski gözlem/onay/adayı geçersiz kılar.
Ret/iptal/yazma hatasında önceki etkin seçim korunur; C0/C2a/C4/C5 geçerlidir.

## C4 — Kaynak bağı ve immutable yürütme

Editor'da kamera ID'si seçimi mevcut kayıtlı varsayılan setup/profile bağını
application komutuyla atomik kurar. Reçete seçimi veya genel kalibrasyon metni
gerekmez. Sıfır kaynaklı reçete DRAFT olarak açılabilir. Kamera yokken anlaşılır
Setup yönlendirmesi vardır. Kaynak silme, kopyalama, import ve kamera değiştirme
doğru bağı/cache'i günceller. Raw/px kalibrasyonsuz kalır; metrik araç aynı kaynağın
uyumlu etkin modelini kullanır, başka/global modele fallback yapmaz.

Kamera seçimi/değişimi serialized bağı değiştiriyorsa reçete DRAFT/dirty olur;
otomatik disk kaydı yoktur. Aynı kameranın geometriyi değiştirmeyen ayar güncellemesi
Editor Run Flow hazırlığında yeniden bağlanabilir; değişen serialized snapshot
yine dirty gösterilir. Başka kamera, legacy belirsizliği veya değişmiş geometri
otomatik onarılmaz. Yalnız belgeyi açmak tüm eski reçeteleri yeniden yazmaz.

Reçetede kullanıcıya iki genel referans alanı kaldırılır, legacy JSON alanlarını
topluca silmek ayrı bir hedef değildir. Eski kayıt metadata düzenlemesinde
değişmez. Yeni reçete eski proje referanslarını miras almaz. Aktif Runtime
yüklediği immutable snapshot'ı kullanır; kamera/model değişimi durdurma ve yeniden
hazırlama ister. Yeni aktif modeli çalışan çevrime sessiz ekleme yoktur.

## C5 — Deneysel mm, üretim ve kanıt

EXP-02'nin eksik kalibrasyon kökeni ve EXP-03'ün Raw/px sınırı kontrollü genişletilir.
Deneysel model/aktivasyon için ayrı tipli evidence/admission gerekir; trusted
CameraAcquired probe veya production completion taklit edilmez. Yeni kayıt
codec'i eski kayıtları aynı anlamla okur; eski Raw/px kayıtları mm kabulüne yükselmez.

Canlı/PGM+JSON → aday → deneysel etkin seçim → Editor metrik sonuç → deneysel
Runtime/Logs/CSV zincirinde raster hash, kamera/geometri, model ID/content/version
ve experimental niteliği korunur. Replay kamera/kurulum/model eşleşmesi açık kanıt
ister; bilinmeyen ilişki tahmin edilmez. Eksik/yanlış model, değiştirilmiş sidecar,
geometri ve farklı kamera reddedilir. Deneysel mm sonucu da PLC/robota çıkamaz.

OECS pending-command/cancel engeli, native Unavailable, G0–G4 sınırları değişmez.
03c satır kırpma izni vardır fakat canlı yolu bu pakette sessiz değiştirmeyin;
kullanılacak rasterin origin/geometry'si tüm zincirde aynı olmalıdır.
Fiziksel mm doğruluğu bağımsız referansla ayrı kabul; fixture fiziksel kabul değildir.

## C6 — Kapanış ve odak

İlgili kartın testleri + değişen sözleşmenin negatifleri yeterlidir. Kapanışta
komut, tarih, exit code, rapor/görsel yolları, yapılmayan fiziksel kontroller ve
sonraki kart yazılır. Bir kart beklenenden büyürse önce tamamlanabilir servis/UI
sınırında alt kart olarak bölün; kapsam genişlemesini gerekçesiyle plan indeksine
işleyin. Tarihsel arşivi değiştirmeyin; AGENTS'ın tek görev/tek commit kuralını izleyin.

