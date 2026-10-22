# CCUX — sade kamera ve kalibrasyon uygulama planı

22 Ekim 2026 — **CCUX-PLAN-03: kamera akışını son kullanıcı için sadeleştirme**.
CCUX-01–14 teslimleri korunur; CCUX-15 fiziksel kabulü kısmen açıktır.
CCUX-16 application politikası, CCUX-17 UI sadeleştirmesi ve CCUX-18 eski gezinme temizliği teslim edildi; CCUX-19 bekliyor. Güncel seçili iş ve
tamamlanan kartlar [progress](../../progress.md) üzerinden izlenir.
Öncelik sırası **CCUX-16 → 17 → 18 → 19 → CCUX-15'in kalan fiziksel kabulü**.
Eski EXP/KMC sıraları bu sıranın önüne geçmez.

## 22 Ekim kullanıcı kararı

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

> CCUX-16 görevini uygula. AGENTS.md, docs/progress.md ve
> README.md ardından docs/architecture/camera-setup/README.md, contract.md ve
> 16-automatic-acquisition.md dosyasını oku. Yalnız bu kartı ve gerekli doğrudan
> bağımlılıklarını tamamla. Karttaki kabul/testleri çalıştır, yeni kanıtı kaydet,
> progress ve tarihli kapanışı güncelle, görev ID'li tek commit oluştur; push yapma.
> Hand-eye kapsam dışı. Tamamlanmış KMC/ICM/EXP işlerini yeniden başlatma.

Sonraki oturumda yalnız görev ID'si ve kart dosyası değiştirilir. İlerleme özeti
kısa tutulur; uygulama ayrıntıları ilgili sözleşmeye, test raporu kapanışa yazılır.
Tüketim tasarrufu için tek seferde tüm kartları prompt'a yapıştırmayın.

[OpenAI resmî rehberi](https://developers.openai.com/api/docs/guides/latest-model?model=gpt-6.1-sol)
tekrarlı talimatları azaltmayı önerir. Buradaki kart boyutu ve görev bölümü repo
için mühendislik tercihidir; belirli token/maliyet tasarrufu garantisi değildir.

