# CCUX — sade kamera ve kalibrasyon uygulama planı

21 Ekim 2026 — CCUX-PLAN-02. Hedef uygulayıcı **GPT-6.1 Sol / medium**.
Plan hazır; aşağıdaki uygulama kartlarının hiçbiri bu planlama tesliminde yapılmadı.
Güncel seçili iş ve tamamlanan kartlar yalnız [progress](../../progress.md) üzerinden
izlenir. Başlangıç **CCUX-01**. Eski EXP/KMC sıraları bu planın önüne geçmez.

## Kapsam

Setup → Kameralar: tara, ID/ad, exposure/gain, ekle, tek/canlı görüntü testi.
Setup → Kalibrasyon: kayıtlı kamera, aktif değerler/kalibrasyon yok, Kalibre Et,
bilinen ölçü veya plaka, kaydet ve kullan. Editor kaynak başına kamera seçer;
yeni/düzenlenen reçetede iki genel referans alanı kaldırılır. Deneysel OECS mm
Editor/Runtime'a kökeni korunarak bağlanır.

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

Sıra aşağıdaki gibidir; çalışma paralel değildir. “Bağlı” teknik önkoşuldur.

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

> GPT-6.1 Sol medium ile CCUX-01 görevini uygula. AGENTS.md, docs/progress.md ve
> README.md ardından docs/architecture/camera-setup/README.md, contract.md ve
> 01-registration.md dosyasını oku. Yalnız bu kartı ve gerekli doğrudan
> bağımlılıklarını tamamla. Karttaki kabul/testleri çalıştır, yeni kanıtı kaydet,
> progress ve tarihli kapanışı güncelle, görev ID'li tek commit oluştur; push yapma.
> Hand-eye kapsam dışı. Tamamlanmış KMC/ICM/EXP işlerini yeniden başlatma.

Sonraki oturumda yalnız görev ID'si ve kart dosyası değiştirilir. İlerleme özeti
kısa tutulur; uygulama ayrıntıları ilgili sözleşmeye, test raporu kapanışa yazılır.
Tüketim tasarrufu için tek seferde tüm kartları prompt'a yapıştırmayın.

[OpenAI resmî rehberi](https://developers.openai.com/api/docs/guides/latest-model?model=gpt-6.1-sol)
tekrarlı talimatları azaltmayı önerir. Buradaki kart boyutu ve görev bölümü repo
için mühendislik tercihidir; belirli token/maliyet tasarrufu garantisi değildir.

