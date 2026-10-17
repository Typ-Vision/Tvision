# Endüstriyel kamera entegrasyonu — ICM ana planı

17 Ekim 2026. **ICM-00 envanteri tamamlandı; endüstriyel acquisition başlamadı.** Uygulayıcı hedefi:
**GPT-6.1-sol / medium** (kullanıcının Sol6.1 medium tercihi). Bu belge model
ayarını değiştirmez veya başka bir chat başlatmaz. Yeni kullanıcı yönlendirmesi,
KMC-10e sonrasında fiziksel kalibrasyon kabulünden önce gerçek kamera entegrasyonudur.

## Okuma ve uygulama sırası

1. Kısa başlangıç: AGENTS, [progress](../progress.md), [README](../../README.md).
2. Bu planın ortak kuralları ve seçili tek görev kartı: [altyapı](industrial-camera-tasks-foundation.md)
   veya [ürün entegrasyonu](industrial-camera-tasks-integration.md). Önce başlık/ID arayın.
3. Kartın gerektirdiği [sözleşme](industrial-camera-contract-plan.md),
   [kabul](industrial-camera-acceptance-plan.md) ve gereksinim bölümleri.
   [Mevcut bulgular](industrial-camera-integration-assessment.md) ve
   [baseline](industrial-camera-baseline.md) yalnız ilgili bağımlılık/kanıt için okunur.
4. Arşivin veya bütün gereksinimlerin toplu okunması başlangıç şartı değildir;
   belirsizlik/çelişkide okumayı genişletin. [Devir şablonu](industrial-camera-handoff.md).

Güncel görev sırası ve son kanıt [progress'tedir](../progress.md);
[ICM-00 başlangıç kaydı](industrial-camera-baseline.md) tamamlandı. Bir oturumun varsayılan kapsamı bir görevdir; kullanıcı
bir sıra verirse aynı sırada devam edilir. Tamamlanan KMC paketleri tekrarlanmaz.
Bu plan 6B/industrial acquisition açığını devralır; ayrı ikinci kamera mimarisi kurmaz.

## Sabit başlangıç bilgileri

| Konu | Kanıt / durum |
| --- | --- |
| Kamera | COE-050-M-USB-050-IR-C; kullanıcı kodu iki kez birleşmiş yazdı, normalize edildi |
| Arayüz | COE-U, USB3 Vision; ITALA GigE backend'i bu cihazın kabulü değildir |
| Görüntü | Datasheet: mono IMX264, global shutter, 2448×2048, 35 fps; sahada henüz ölçülmedi |
| Lens | Kullanıcı: Basler 25 mm; tam model, odak/diyafram, çalışma mesafesi bilinmiyor |
| Kimlik | Kullanıcının Device ID=1 değeri seri numarası olarak doğrulanmadı |
| SDK | OECS v1.2 kullanıcı bildirimi; yerel OECS ve ITALA dizinleri mevcut |
| U3V producer | `C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvProducerU3V.cti`, dosya sürümü 1.0.0.5 |
| Son fiziksel ön kontrol | `build/opto-coe-u-pre-reboot-probe.json`: DONE, errors boş, 0 cihaz; open/grab yok |
| Reboot | Kullanıcı installer isteğini erteledi; gerçekleştiği ayrıca doğrulanacak |
| Son yazılım kabulü | KMC-10e UI/ZXing ON 87/87, core OFF 73/73; yeni ICM kabulü değildir |

Yerel `OECS/Development/Includes/MvCameraControl.h` C linkage/API bildirimleri ve
`Development/Libraries/win64/MvCameraControl.lib` mevcut. Bunlar MinGW link/ABI,
export veya fiziksel acquisition kabulü sayılmaz. Başlık/lisanslar repoya izinsiz
kopyalanmaz; yerel SDK keşfi isteğe bağlı bağımlılık olarak tutulur.

## Ürün hedefi ve ilk kapsam

NI Vision Builder/HALCON benzeri genel kullanım hedefi için marka bağımsız cihaz,
özellik ve acquisition sözleşmesi korunur. İlk doğrulanacak backend OECS/COE-U'dur.
Opto'ya özel widget, kalibrasyon matematiği veya ayrı Runtime yaratılmaz.

İlk sürüm: tek fiziksel endüstriyel kamera, Mono8 tam sensör, sabit geometri,
manual exposure/gain (destekleniyorsa), seçili kamera preview ve gerçek software
trigger ile tek çevrim. Preview free-run ile production trigger farklı oturumlardır.
Sıralı N-source çekirdek korunur; tek fiziksel kamera + dosya/webcam karışımı sınanır.
Aynı kameranın iki source'a atanması mevcut konservatif politikayla reddedilir.

İlk kapsam dışında: external trigger'ın ürün akışına açılması, packed10/12 decode,
Bayer/renk, sensor ROI/binning/decimation ile üretim, paralel N-camera stream,
PTP/stereo/ortak dünya/robot, GPU, otomatik recovery ile üretime dönüş ve temiz
makine dağıtım paketi. Bu özellikler desteklenmiyorsa açık capability reddi verir.
Donanımın özelliği desteklemesi VisionFlow'un desteklediği anlamına gelmez.

## Teslim kapıları

| Kapı | Kapanış | Kullanıcıya söylenebilecek kapsam |
| --- | --- | --- |
| G0 — fiziksel başlangıç | ICM-01b | OECS gerçek görüntü ve VisionFlow discovery/open kanıtı; ürün grab'i değil |
| G1 — acquisition | ICM-04e + gerçek grab kanıtı | Ortak adapter tek kameradan doğru owned frame alır; üretim izni değil |
| G2 — ürün yazılımı | ICM-09a | Kamera ekleme, kalibrasyon, Editor, trusted Runtime/terminal bütünleşik yazılım kabulü |
| G3 — fiziksel kamera | ICM-09b | Gerçek cihazda e2e, kimlik, kopma ve tekrar bağlanma kabulü |
| G4 — fiziksel ölçüm | ICM-10 | Belirlenen kurulum/tolerans için bağımsız ölçüm ve tekrar edilebilirlik kabulü |

G0 yokken uygun fixture işleri yapılabilir; G1/G3/G4 donanım yokluğu nedeniyle
tamamlandı işaretlenmez. G2 yalnız yazılımdır; kullanıcıya fiziksel üretim doğruluğu
izni olarak sunulmaz. Production capability, ICM-08b trusted terminal kapısı bitmeden açılmaz.

## Görev envanteri ve bağımlılıklar

**ICM-00, ICM-01a, ICM-03a, ICM-03b ve ICM-04a tamamlandı; ICM-01b açık; ICM-02 offline/koşullu tamamlandı.**
[Backend kararı](industrial-camera-backend-decision.md): OECS C API;
gerçek cihaz readback/grab açık. Diğer satırlar PLANLANDI.
[ICM-01a tanı kabulü](../archive/2026-10-17-icm-01a.md) fiziksel kapıları kapatmaz.
Bağımlılıklar görevin başlatılabilmesini ifade eder; fiziksel kapı ayrıca belirtilir.

| ID | Tek teslim | Önkoşul |
| --- | --- | --- |
| ICM-00 | Başlangıç/sürüm/kapı envanteri | Yok |
| ICM-01a | Tekrarlanabilir SDK/probe tanısı | 00 |
| ICM-01b | Reboot sonrası OECS gerçek görüntü ve seri/open kanıtı | 01a, fiziksel erişim |
| ICM-02 | Backend/GenApi/ABI karar deneyi | 01a; gerçek başarı için 01b |
| ICM-03a | Fiziksel kimlik ve yerel transport ayrımı | 00 |
| ICM-03b | Acquisition profile/geometri ve codec | 03a |
| ICM-04a | Backend session/handle sahipliği | 02 karar kaydı, 03b |
| ICM-04b | Feature erişimi ve transactional configure/readback | 04a |
| ICM-04c | Stream/buffer → owned Mono8 frame | 04b |
| ICM-04d | Software trigger ve çevrim-kare eşleştirme | 04c |
| ICM-04e | Lifecycle/lease/preview/Stop/recovery | 04d |
| ICM-05a | Ortak discovery/registry/factory | 04e |
| ICM-05b | Camera Manager ve kayıt/ayar UI | 05a |
| ICM-06a | Reçetesiz kalibrasyon edinimi | 05a |
| ICM-06b | Gerçek geometriyle aktivasyon/invalidation | 06a, 03b |
| ICM-07a | Industrial source tool/codec/migration | 05a |
| ICM-07b | Execution/TargetOnly edinim bağlantısı | 07a, 06b |
| ICM-07c | Ortak Editor source/ayar/Viewer UI | 07b, 05b |
| ICM-08a | Immutable Runtime admission ve actual evidence | 07b |
| ICM-08b | Trusted completion/terminal/replay/Logs verisi | 08a |
| ICM-08c | Runtime/Logs/CSV kullanıcı sunumu | 08b, 07c |
| ICM-09a | Tam Release yazılım ve uyumluluk kabulü | 05b, 06b, 07c, 08c |
| ICM-09b | Gerçek kamera uçtan uca ve kopma kabulü | 09a, 01b, G1 gerçek grab |
| ICM-10 | Bağımsız fiziksel kalibrasyon/lens karar raporu | 09b, ölçüm referansı |

Tercih edilen sıra tablodaki sıradır. Donanım beklenirse 03a/03b ve fake-backend
test hazırlığı yapılabilir. 02 backend kararı olmadan vendor acquisition kodu
üretilmez; karar kaydı yalnız offline ise gerçek başarı açık kalır. Bir görev çok
büyürse mevcut ID altında a/b alt görev önerilip progress'e yazılır; acceptance
ölçütleri sonraki pakete gizlice bırakılarak tamamlandı denmez.

## Gelecek genişlemeler

G4 hata haritası gerektirirse lens T15 öne alınır: intrinsic/distortion modeli,
raw↔undistorted dönüşüm, model/ROI/replay migration ve ayrı fiziksel doğruluk paketi.
Başka marka/USB3/GigE, packed format, hardware trigger ve N gerçek kamera kapasitesi
ayrı backend kabul matrisiyle açılır. PTP bu USB kameranın varsayılan yeteneği değildir.
KUI-6/native, T8.4.2b.3c pose, PLC/robot ve dağıtım borçları ICM ile otomatik kapanmaz.
