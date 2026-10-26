# CCUX — sade kamera ve kalibrasyon uygulama planı

26 Ekim 2026 — **[CCUX-PLAN-07: ayrıntılı kullanılabilirlik planı](24-usability-plan.md)**.
[24A analizi](24-usability-analysis.md) 24B–I sıralı uygulama kartlarına ayrıldı;
[24C tamamlandı](../../archive/2026-10-26-ccux-24c.md); sıradaki [24D](24d-input-precision.md). Kullanıcının düzeltmesi:
yalnız Kalibrasyon Ayrıntılar düğmeleri sadeleşir; Kalibre Et'in Bilinen ölçü,
plaka ve devamındaki mevcut özellikleri korunur. Robot ana seçimde pasif kalır.
[Plan kapanışı](../../archive/2026-10-26-ccux-plan-07.md).

24 Ekim 2026 — **[CCUX-PLAN-06: Editor gecikmesi](23-editor-responsiveness.md)**.
CCUX-23a analiz olarak tamamlandı. Uygulama CCUX-23b-1–6 sıralı alt kartlarına
ayrıldı; ilk kart tekrarlanabilir baseline'dır. Sol 6.1 alt görevleri paralel
çalışmaz. 23b-1–6 yazılım kabulü tamamlandı; canlı proje/kamera kabulü açık.
[Birleşik kabul](../../archive/2026-10-24-ccux-23b-6.md), [plan kapanışı](../../archive/2026-10-24-ccux-plan-06.md).

23 Ekim 2026 — **CCUX-PLAN-05: çerçeveli plaka kalibrasyonu planı**.
Yeni kullanıcı yönlendirmesi: [CCUX-15e–j](15-plate-calibration-plan.md) ayrıntılı
planı hazırlandı; **15e gözlem/model servis ayrımı tamamlandı**
([kapanış](../../archive/2026-10-23-ccux-15e.md), [API](plate-observations.md)).
**15f algılama servisi tamamlandı** ([kapanış](../../archive/2026-10-23-ccux-15f.md),
[sınırlar](framed-plate-detection.md)); 15g model/kayıt/replay tamamlandı ([API](plate-observations.md)); 15h sihirbaz/tanı tamamlandı ([kapanış](../../archive/2026-10-23-ccux-15h.md)); 15i birleşik yazılım kabulü [kapanıştadır](../../archive/2026-10-23-ccux-15i.md); devam 15j fiziksel devreye almadır.
56000 µs eski karede 49 merkez bulundu; karanlık ilk kare ret kalır.
Distorsiyon çözümü devreye alınmayacak. [C3a](contract.md#c3a--çerçeveli-plaka-kalibrasyonu)
hedef sözleşmedir. [Kapanış](../../archive/2026-10-23-ccux-plan-05.md).

22 Ekim **CCUX-PLAN-03** kamera akışını sadeleştirme teslimleri korunur.
CCUX-01–14 teslimleri korunur; CCUX-15 fiziksel kabulü kısmen açıktır.
CCUX-16 politika, CCUX-17 UI, CCUX-18 gezinme temizliği ve CCUX-19 birleşik
yazılım/görsel kabulü teslim edildi. CCUX-15 fiziksel kabulü açık. Güncel seçili iş ve
tamamlanan kartlar [progress](../../progress.md) üzerinden izlenir.
Önceki çalışma: **[CCUX-22 çalışma süresince kamera bağlantısı](22-session-lifecycle.md)**;
**22a yazılım ölçümü tamamlandı**, fiziksel baseline kamera erişimi bekliyor.
[Kapanış](../../archive/2026-10-23-ccux-22a.md). **22b ortak servis tamamlandı**
([API/ömür](acquisition-sessions.md), [kapanış](../../archive/2026-10-23-ccux-22b.md)).
**22c Editor entegrasyonu uygulandı** ([kapanış](../../archive/2026-10-23-ccux-22c.md)).
**22d Runtime entegrasyonu uygulandı** ([kapanış](../../archive/2026-10-23-ccux-22d.md)).
**22e Kalibrasyon entegrasyonu uygulandı** ([kapanış](../../archive/2026-10-23-ccux-22e.md)).
22f birleşik kabul ve fiziksel kamera doğrulaması açık; yeni plaka planı bunları kapatmaz.
[CCUX-20](20-image-responsiveness.md) hash/preview teslimi ve
[CCUX-21](21-preprocess-roi.md) ROI yazılım/görsel kabulü korunur;
CCUX-20 gerçek kamera teyidi ve CCUX-15 fiziksel kabulü açık kalır.
Eski EXP/KMC sıraları bu sıranın önüne geçmez.

## 22 Ekim kullanıcı kararı

Bağlantı artık her tek çekimde kurulup kapatılmayacak: Editor reçete açılışı/çıkışı,
Runtime Run/Stop, canlı kalibrasyon işlem başlangıcı/bitişi sınırlarında tutulacak.
Program açılışı ve Runtime Load bağlantı kurmaz. Bu yeni hedef
[C2a](contract.md#c2a--çalışma-süresince-bağlantı) ve CCUX-22'de tanımlıdır.

“Deneysel OECS” seçenekleri ve “Tetik–kare eşleşmesi doğrulanmadı” dahil kamera
uyarıları görünür ürün akışından kaldırılacak; eşdeğer yeni uyarı/mod eklenmeyecek.
Kalibrasyon'daki “Önceki akış ve eski kanıt ekranları” bölümü de kaldırılacak.
Kamera/kalibrasyon/Editor/Runtime normal komutları mevcut uygun edinim yolunu
application katmanında seçer. Bu hedef yalnız checkbox gizlemek değildir.
[Ortak sözleşme C0](contract.md#c0--22-ekim-sade-ürün-akışı-kararı) eski opt-in ve
kalıcı uyarı gereğinin yerine geçer; teknik kanıt ve kayıt uyumluluğu korunur.

## Kapsam

Setup → Kameralar: tara, ID/ad, exposure/gain, ekle, tek/canlı görüntü testi.
Setup → Kalibrasyon: kayıtlı kamera, aktif değerler/kalibrasyon yok, Kalibre Et,
bilinen ölçü veya plaka, kaydet ve kullan. Editor kaynak başına kamera seçer;
yeni/düzenlenen reçetede iki genel referans alanı kaldırılır. OECS mm
Editor/Runtime'a kökeni korunarak bağlanır; kullanıcı ayrıca mod seçmez.

**Hand-Eye / Eye-to-Hand / Eye-in-Hand bu paketin dışında.** Robot hesabı, robot
örnek toplama, ChArUco/intrinsics/lens solver geliştirmesi, robot/PLC çıkışını açma,
yeni vendor SDK'ları ve kurulum paketi yok. Robot seçeneği mevcutsa açıkça
“Henüz desteklenmiyor” gösterilir; yeni çalışan robot sihirbazı yapılmaz.

## Tek kartla çalışma

AGENTS + progress + README ardından bu dizin, [ortak sözleşme](contract.md) ve
yalnız seçili kart okunur. Kartın “Oku” bölümü zorunlu dar bağımlılık listesidir;
kaynaklarda önce sembol arayın. Önceki kartın kapanışını ancak kullandığınız
arayüz/şema belirsizse açın. Robot taslağı, arşiv dizini ve bütün eski planları
başlangıçta okumayın. Model ayarı kullanıcı tarafından seçilir; bu dosya değiştirmez.

Her kart bir teslim/commit sınırıdır; sonraki karta kullanıcı sırası olmadan
kendiliğinden geçmeyin. Eksik donanım yalnız fiziksel kabulü açık bırakır;
fixture ile yazılım kartını bitirin, sahte donanım kabulü üretmeyin. Önkoşulun
API'si eksikse ilgili dar bağımlılığı düzeltin; bütün kamera katmanını yeniden yazmayın.

## Sıra ve bağımlılıklar

İlk paket aşağıdadır; güncel öncelik yukarıdaki sıradır. Çalışma paralel değildir.
“Bağlı” teknik önkoşuldur; tamamlanmış kartlar yeniden başlatılmaz.

| Kart | Teslim | Bağlı |
| --- | --- | --- |
| [CCUX-01](01-registration.md) | Kamera ID'si ve otomatik varsayılan kayıt | — |
| [CCUX-02](02-settings.md) | Kamera ayarları ve gerçek readback servisi | 01 |
| [CCUX-03](03-camera-ui.md) | Sade Kameralar ekranı | 02 |
| [CCUX-04](04-preview-service.md) | Sınırlı canlı önizleme yaşam döngüsü | 02 |
| [CCUX-05](05-preview-ui.md) | Tek/canlı görüntü ve Stop UI | 03,04 |
| [CCUX-06](06-validity.md) | Kalibrasyon geçerliliği ve yeniden açılış | 01,02 |
| [CCUX-07](07-experimental-calibration.md) | Deneysel kalibrasyon edinimi ve kayıt kökeni | 04,06 |
| [CCUX-08](08-save-use.md) | Kaydet ve kullan application komutu | 06,07 |
| [CCUX-09](09-calibration-ui.md) | Kamera listesi ve iki sade sihirbaz | 05,08 |
| [CCUX-10](10-editor-binding.md) | Editor'da kamera seçerek otomatik bağlama | 06,08 |
| [CCUX-11](11-recipe-fields.md) | Reçete referans alanlarını kaldırma | 10 |
| [CCUX-12](12-runtime-metric.md) | Deneysel Runtime metrik snapshot/kayıt | 07,08,10 |
| [CCUX-13](13-runtime-ui.md) | Runtime/Logs/CSV metrik gösterim | 11,12 |
| [CCUX-14](14-software-acceptance.md) | Birleşik yazılım ve görsel kabul | 09,13 |
| [CCUX-15](15-physical-acceptance.md) | Gerçek kamera ve bağımsız mm kabulü | 14 + cihaz/referans |
| [CCUX-16](16-automatic-acquisition.md) | Normal komutlardan otomatik edinim politikası | Mevcut 01–14 |
| [CCUX-17](17-clean-camera-ui.md) | Bütün OECS seçim/uyarıları ve metinlerini kaldırma | 16 |
| [CCUX-18](18-calibration-navigation.md) | Eski kalibrasyon gezinmesi ve UI bağımlılıklarını kaldırma | 17 |
| [CCUX-19](19-final-flow-acceptance.md) | Birleşik kabul ve güncel kullanım rehberi | 16–18 |

22 Ekim kullanıcı hata bildirimi: **CCUX-03b**, Camera Manager kayıt/ayar kilidi
onarımıdır; CCUX-15'i tamamlamaz. Ayrı OECS settings backend'i ve görünür kayıt/
unsupported yönlendirmesi [kapanışta](../../archive/2026-10-22-ccux-03b.md).

Kart içindeki CTest listesi mevcut hedef adlarıdır; yeni davranış için test
gerekiyorsa aynı suite genişletilir veya anlamlı yeni hedef eklenir. Komut:
`ctest --test-dir build/reference-ui -R '^(ad1|ad2)$' --output-on-failure`.
Önce `ctest -N` ile hedeflerin gerçekten kayıtlı olduğunu doğrulayın; sıfır test
başarı değildir. Release/toolchain komutları ana README'dedir. Her kartın
kanıtı `build/ccux/<kart-id>/`; görüntüler normal ve küçük pencere boyutunda.
UI kartları Release + ilgili CTest + görsel kontrol ister. Saf servis kartları
UI görseli istemez. Son birleşik kabul iki build'de ilgili regresyonları bir kez
birleştirir; her kartta bütün suite'i tekrarlamaz.

## Kopyalanabilir başlangıç

> CCUX-15e görevini uygula. AGENTS.md, docs/progress.md ve
> README.md ardından docs/architecture/camera-setup/README.md, contract.md ve
> 15-plate-calibration-plan.md tasarım/sınır ve 15e bölümünü oku. Yalnız bu kartı ve gerekli doğrudan
> bağımlılıklarını tamamla. Karttaki kabul/testleri çalıştır, yeni kanıtı kaydet,
> progress ve tarihli kapanışı güncelle, görev ID'li tek commit oluştur; push yapma.
> Distorsiyon ve hand-eye kapsam dışı. Tamamlanmış KMC/ICM/EXP işlerini yeniden başlatma.

Sonraki oturumda yalnız görev ID'si ve kart dosyası değiştirilir. İlerleme özeti
kısa tutulur; uygulama ayrıntıları ilgili sözleşmeye, test raporu kapanışa yazılır.
Tüketim tasarrufu için tek seferde tüm kartları prompt'a yapıştırmayın.

[OpenAI resmî rehberi](https://developers.openai.com/api/docs/guides/latest-model?model=gpt-6.1-sol)
tekrarlı talimatları azaltmayı önerir. Buradaki kart boyutu ve görev bölümü repo
için mühendislik tercihidir; belirli token/maliyet tasarrufu garantisi değildir.

