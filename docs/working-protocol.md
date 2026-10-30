# VisionFlow çalışma protokolü

DOC-WORK-02 · 30 Ekim 2026. Amaç, kabul kalitesini koruyarak bağlamı,
tekrarları ve görev devri maliyetini azaltmaktır. Kullanıcının güncel talimatı,
AGENTS ve seçili kartın zorunlu kabulü önceliklidir. Bu belge bir oturumda bir
kez okunur; ajan mesajına veya her araç çağrısına tekrar kopyalanmaz.

## 1. Görevi daralt

Başlangıç paketi AGENTS, progress ve README'dir. Ardından yalnız seçili kartı,
ilgili sözleşme bölümlerini ve gerekiyorsa son devir kaydını oku. Önce başlık
veya görev ID'sini bul; bütün planı/arşivi topluca yükleme. Aynı içerik
değişmediyse yeniden okuma. Açık engelleri ve ürün güvenlik kapılarını koru.

Görev başlamadan şu beş alan net olsun: ID, davranış, sahiplik dosyaları,
kapsam dışı işler ve kabul komutları. Dosyalar keşifle netleşecekse ilk adım
bunu belirlesin. Belirsizlik çözmek için okuma kapsamını gerektiği kadar genişlet.
Yapısal keşifte mevcut graph/coverage kuralları aynen geçerlidir; tasarruf için
coverage, kaynak fallback veya kritik çağrı zinciri doğrulaması atlanmaz.

İlk işlem mevcut checkpoint, aktif süreç ve kanıt envanterini kontrol etmektir.
Bitmiş koşumu çalışan sayma; süreç kimliğini ve son log/exit bilgisini doğrula.
Kaynak/test/config/girdi eşleşmesi doğrulanan başarıları devral; yalnız eksik
veya değişiklikten etkilenen kabulü planla. Bu kontrol bitmeden aynı koşumu açma.

## 2. Tek uygulayıcı, kısa devir

Ajanlar sıralı çalışır. Kullanıcı yeni alt ajan istemedikçe yönetim katmanı
ekleme. Yönetici uygulama yapmıyorsa aynı kod keşfini ikinci kez yürütmez.
Kullanıcı sonuç incelemesini istemiyorsa yönetici yalnız görev durumunu izler;
bu çalışma biçimi bağımsız kod incelemesi yapılmış sayılmaz.

Seçili model ve reasoning korunur; bu protokol bunları değiştirme yetkisi
vermez. Basit belge işleri için daha düşük reasoning ayrı bir pilot olabilir;
zor C++/geometri işlerinde maliyet uğruna doğrulama seviyesi düşürülmez.

Yeni görev mesajı, genel kuralları kopyalamadan yaklaşık şu kapsamda olsun:

```text
Görev: <ID>, <kart yolu ve bölümü>. Çalışma kökü: D:/TVision.
Sahiplik: <dosyalar/modül>. Kapsam dışı: <işler>.
Devir: <son kayıt veya kontrol noktası>. Model/reasoning: <kullanıcı seçimi>.
AGENTS ve çalışma protokolünü uygula; mevcut kullanıcı değişikliklerini koru.
Kabul: <komutlar ve görseller>. Sonunda kapanış kaydı ve tek ID'li commit;
push yok. Başka ajan açma. Son bildirim: durum, commit, kanıt yolu, açık engel.
```

## 3. Doğrulamayı değişikliğe göre seç

| Değişiklik | Asgari doğrulama |
| --- | --- |
| Yalnız belge | Belge/link/boyut kontrolü ve diff kontrolü; build gerekmez. |
| Sınırlı core davranışı | İlgili Release hedefleri ve davranışı sınayan CTestler. |
| UI davranışı | UI Release, ilgili CTest, normal/küçük ekran görsel kontrolü. |
| Ortak sözleşme, codec, katmanlar arası değişim veya faz kapanışı | Etkilenen hedefli testlerden sonra son kaynakta tam Release ON/OFF regresyonu. |

Kart daha geniş kabul istiyorsa o uygulanır. Komut/toolchain README'dedir.
Mevcut build dizininde incremental derle; CMake girdisi, toolchain veya
yapılandırma değişmediyse gereksiz configure/clean build yapma. Yeni ilgili
CMake girdisi varsa yeniden yapılandır. UI OFF/ZXing OFF başarısı, ON
özelliğinin kabulü yerine geçmez.

Hata düzeltme döngüsü: başarısız testi ve etkilenen komşu testleri çalıştır;
bunlar geçince gereken tam kabulü son kaynakta yap. Yeni değişiklik, yeni
hata veya kanıtın güncelliğine ilişkin belirsizlik yoksa başarılı koşumu
tekrarlama. Kapsam dışı hata varsa kökenini ve kabul üzerindeki etkisini
belgele; testleri kapatma, doğrulamayı gevşetme veya sessizce kapsamı büyütme.

Aynı hata veya timeout ikinci kez görülürse tam suite'i tekrar başlatma.
Başarısız testi ve gerekli fixture'larını izole et; girdi, süreç, ortam ve log
kanıtlarıyla nedeni araştır. Yeni koşumdan önce değişen koşulu veya sınanacak
somut hipotezi kaydet. Hedefli düzeltme doğrulandıktan sonra kartın zorunlu tam
kabulünü yap. Sürenin uzaması tek başına tekrar gerekçesi veya timeout artırma
izni değildir; neden çözülemiyorsa açık engeli kaydet, başarı beyan etme.

Her bağımsız kabul koşumu temiz, göreve/koşuma özel test ayarları ve çıktı
dizinleri kullanır. Aynı koşumun fixture hazırlığı ile testleri ortak girdiyi
paylaşır; test ortasında gerekli fixture verisini silme. Birikmiş INI/tercih
verisi yeni koşuma taşınmaz; mevcut kanıtı üzerine yazmadan yedekle. Kullanıcı
`config/local.ui.ini` dosyasına dokunma. Başlangıç girdisini (yok/temiz/fixture),
fixture kimliğini ve gereken hash'leri kaydet. Kalıcılık/restore senaryosu eski
durum gerektiriyorsa bunu açık fixture olarak kur ve temiz koşumdan ayır.

Eski kanıt ancak aynı ilgili kaynak, test, yapılandırma ve girdilerle eşleşmesi
kontrol edilirse devralınır; kaynağı ve kontrol yöntemi yazılır. Eski koşumu
yeni test gibi sunma. Sadece tarih veya commit ID'si eşleşmesi tek başına
yeterli değildir. Hedefli koşum, tam suite değildir; skipped ayrı raporlanır.

## 4. Çıktıyı ve beklemeyi küçült

Tam build/test logunu `build/<görev>/` altında sakla. Başarıda exit code,
geçen/başarısız/skipped sayıları ve log yolunu getir; hata varsa ilgili hata
bölümünü aç. Uzun başarılı logu veya aynı diff'i tekrar tekrar modele taşıma.
Kısa ve bağımsız sorguları toplu yap; bağımlı işlem ve düzeltmeleri sırayla yap.

Bekleme aracının izin verdiği olay odaklı beklemeyi kullan; sık durum sorgusu
yerine bitiş/hata bildirimi bekle. Platformun süre sınırlarına ve zorunlu
ilerleme bildirimlerine uy. Bu bildirimler kısa olsun; yalnız bekliyor olmak
için yeni kod, log veya geçmiş sohbet incelemesi başlatma.

15 dakikayı bulan görevde en geç bu eşikte kapsamı, kalan işi ve gerekçeli
tahmini süreyi bildir. Uzun build/tam kabul koşumunun beklenen süresini mümkünse
başlatmadan belirt. 30 dakikada görev sürüyorsa uzama nedenini, eldeki kanıtı ve
bir sonraki somut adımı bildir; uzun sürüyorsa sonraki her 30 dakikada bu hesabı
yenile. Bunlar daha sık zorunlu ilerleme bildirimlerinin yerine geçmez.
Eşikler otomatik durma veya kabulü atlama sınırı değildir; süre aşımı yeni test
tekrarını otomatik başlatmaz. Gerçek süreyi bilmiyorsan tahmin uydurma.

## 5. Kesintide işi yeniden keşfetme

Uzun build/test başlamadan, önemli aşama bitince veya devir öncesinde
`build/<görev>/checkpoint.md` dosyasını güncelle. Kısa görevde ayrı dosya
gerekmez; kapanış kaydı yeterlidir. Kalıcı proje kararı ilgili repo
sözleşmesine yazılır; yalnız build altındaki kontrol noktasına bırakılmaz.

Build/test koşumunu yöneten komut veya wrapper her tamamlanmada (başarı, hata,
timeout, iptal) sonuç kaydını ve checkpoint'in ilgili alanlarını otomatik
güncellemelidir: komut, kaynak/test/config/girdi kimliği, başlangıç/bitiş,
exit/sonuç, sayılar ve log yolu. Sonuç bilinmiyorsa açıkça bilinmiyor/kesildi
yaz; çalışıyor kaydını bırakma. Kullanılan süreç aracı bu kaydı üretemiyorsa
ajan tamamlanma çıktısını alınca, başka koşuma geçmeden kaydı günceller ve
otomasyon eksikliğini açık bırakır. Kayıt tamamlanmadan zincirleme retry yapma.
Otomatik sonuç kaydı, kartın belge/commit kapanışı tamamlandı anlamına gelmez.

```text
Görev / durum: çalışıyor | kesildi | tamamlandı
HEAD ve kullanıcıya ait korunacak değişiklikler:
Tamamlanan işler / değişen dosyalar:
Son doğrulama: komut, yapılandırma, sonuç, kaynak eşleşmesi, log/görsel yolu
Devam eden işlem: session/process kimliği ve çıktı yolu (varsa)
Kalan iş / açık engel / bir sonraki somut adım:
Kapanış kaydı ve commit: var/yok, yol/kimlik
```

Yeni ajan önce mevcut işlemi ve kanıtı kontrol eder; aynı testi ikinci kez
başlatmaz, önceki değişiklikleri sıfırlamaz. Kullanım sınırı, hata veya ajanın
kaybolması tamamlanma değildir. Tamamlandı bildirimi ancak kart kabulü,
belgeler ve görev ID'li commit kapandıktan sonra verilir. Yöneticiye devir
depo kayıtlarıyla yapılır; ajan belleğine güvenilmez. Başarısız/kesilmiş kartın
ardından bağımlı kart otomatik başlatılmaz.

## 6. Üç görevlik ölçüm

Sonraki üç uygulama kartında mevcut oturum sayaçları erişilebiliyorsa
`build/usage-audit/<görev>/` altında kısa ölçüm tut. Kullanıcı/hesap verisinin
tamamını veya ham sohbeti rapora kopyalama. Sayaç yoksa **erişilemiyor** yaz;
harcama yüzdesi veya ücret uydurma.

- Model/reasoning, görev türü, kapsam ve kabul sonucu.
- Girdi, bunun içindeki cache girdi, cache dışı girdi ve çıktı.
- Yönetici ve her uygulayıcı ayrı; yeniden başlatma, araç çağrısı ve yinelenen
  doğrulamanın nedeni. Yeni ve devralınan koşumları ayır.

Aynı oturumun kümülatif sayaç satırlarını toplama: görev başı/sonu farkını al;
yeni oturumda son toplamı kullan. Yeni oturum miras sayaç taşıyorsa başlangıç
değerini düş. Cache girdi girdinin, reasoning çıktısı çıktının içindedir;
ikinci kez eklenmez. Salt token toplamını Plus kota yüzdesi veya ücret sayma.
Üç görev sonunda kapsam/kaliteyi de dikkate alarak karşılaştır; sabit tasarruf
oranı vaat etme. Daha az araç çağrısı tek başına daha iyi sonuç değildir.

Başlangıç referansı: EFR-04 + devam ajanı 287.812 cache dışı girdi,
19.591.424 cache girdi ve 59.480 çıktı; bunlar yalnız build'in değil bütün
görevin sayaçlarıdır. Ölçüm ayrıntıları yerel
`build/usage-audit/2026-10-29/report.md` ve `token-summary.json` dosyalarındadır;
build kanıtlarının Git ile taşınacağı varsayılmaz.

Resmî kaynak: [OpenAI kullanım rehberi](https://learn.chatgpt.com/docs/pricing).
Bağlamı, ilgili kaynakları ve gereksiz çıktıyı daraltma önerileri bu projedeki
zorunlu kabul kurallarını kaldırmaz.
