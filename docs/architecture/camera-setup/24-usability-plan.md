# CCUX-PLAN-07 — kullanılabilirlik uygulama planı

26 Ekim 2026. Uygulayıcı: Codex. **Plan hazır; uygulama başlamadı.**
Girdi: [CCUX-24A analizi](24-usability-analysis.md). Kaynak bulguları o incelemeye
aittir; aşağıdaki dosya listeleri uygulama başlangıç haritasıdır, eksiksiz etki
analizi iddiası değildir. Her kart başında güncel kaynak/indeks doğrulanır.

## Kesin kapsam ve son kullanıcı düzeltmesi

**Kalibrasyon sayfasındaki “Kalibre Et…” ve onunla açılan mevcut özellikler
korunacak.** Bilinen ölçü ve Kalibrasyon plakası girişleri, devam eden sihirbazlar,
mevcut plaka yöntemleri, kamera/dosya seçimi, hesaplama, bağımsız kontrol ve
Kaydet ve kullan kaldırılmayacak. 7×7 plaka tek zorunlu oluşturma yöntemi yapılmayacak.

Yöntem seçimindeki üçüncü seçenek **“Robot Kalibrasyonu (Sonradan devreye
alınacak)”** adıyla görünür ve pasif kalacak. Bu pakette robot kalibrasyonu,
hand-eye, hareket, bağlantı veya pose üretimi uygulanmayacak.

Sadeleşme yalnız **Ayrıntılar checkbox'u açılınca görünen gereksiz düğmeleri**
hedefler. Aynı işlevin ana akışta bulunan kopyaları kaldırılır. Yönetim,
uyumluluk ve transfer işlevleri gerekiyorsa uygun yerlerine taşınır; erişilebilir
karşılığı doğrulanmadan servis veya çalışan özellik silinmez. Kayıt geçmişi ve
belirsiz eski projelerde gerekli kurulum/profil seçimi düğme kalabalığı değildir.
[24G](24g-calibration-details.md) tek tek karar listesidir. Kullanıcının bu
düzeltmesi 24A bölüm 7'deki tek plaka akışı ve robotu kaldırma önerisinin yerine geçer.

## Sıra ve izlenebilirlik

Ajanlar paralel çalışmaz. Her oturum tek karttır; her tamamlanan kart tek görev
ID'li commit ile kapanır. Bu planın commit'i uygulama kartlarını tamamlamaz.

| Sıra | Kart | Analiz maddesi | Somut teslim / teknik önkoşul |
| --- | --- | --- | --- |
| 1 | [CCUX-24B](24b-editor-layout.md) | 1, 3, 4, 5 panel kısmı | Kalıcı Flow, sade sonuçlar/Properties; mevcut 24A |
| 2 | [CCUX-24C](24c-calibration-status-circle.md) | 2, 5 varsayılan | Tutarlı kontrol durumu, yalnız yeni Circle calibrated; 24B sonrası |
| 3 | [CCUX-24D](24d-input-precision.md) | 6 | Üç basamak, eski hassas değerin korunması; 24C sonrası |
| 4 | [CCUX-24E](24e-cursor-zoom.md) | 9 | Olay konumuna göre zoom ve kare yenileme; 24D sonrası |
| 5 | [CCUX-24F](24f-camera-manager.md) | 8 | Büyük preview ve görünür kurulum yönetimi; 24E viewer davranışı |
| 6 | [CCUX-24G](24g-calibration-details.md) | 7, kullanıcı düzeltmesi | Yalnız Ayrıntılar düğmelerinin sadeleşmesi; 24F yönetim hedefleri |
| 7 | [CCUX-24H](24h-runtime-overlays.md) | 10 | Kaynak/çevrim bağlı çoklu overlay; 24E viewer, 24B sonuç sunumu |
| 8 | [CCUX-24I](24i-integrated-acceptance.md) | 1–10 | Birleşik yazılım/görsel kabul; 24B–H kapanışları |

24C/D/E'nin sıra bağı teslim düzenidir; yeni bir algoritma bağı varsayılmaz.
24F→G ve 24E→H gerçek arayüz/taşıma bağıdır. Sonraki karta geçmeden önce mevcut
kartın kabulü kapanır veya açık engel ve gerekçeli sıra değişikliği belgelenir.

## Ortak uygulama ve doğrulama kuralları

1. AGENTS, progress, ana README, bu indeks, contract ve yalnız seçili kart okunur.
   Analizden yalnız ilgili bölüm, gereksinimlerden yalnız ilgili ekran açılır.
2. `git status` ve başlangıç commit'i kaydedilir. codebase-memory ile proje kökü
   `C:/Projects/Tvision` seçilir; ilgili dosyalarda coverage ve kaynak fallback
   yapılır. Bu plan eski analizin test/indeks sonuçlarını yeni kanıt saymaz.
3. Karttaki mevcut CTest isimleri `ctest --test-dir build/reference-ui -N` ile
   yeniden doğrulanır. Eksik test adı düzeltilir; sıfır test başarılı kabul değildir.
   Yeni davranışlar mevcut suite'e anlamlı etkileşim/negatif senaryolarla eklenir.
4. README toolchain'iyle Release build; ilgili suite için
   `ctest --test-dir build/reference-ui -R '^(ad1|ad2)$' --output-on-failure`.
   Görsel/etkileşim kontrolü 1400×900 ve 850×600'de, test INI'siyle yapılır.
   Offscreen görsel fiziksel ekran/DPI kabulü değildir; zoom için gerçek wheel
   etkileşimi ayrıca kaydedilir. Kamera yoksa fixture ve fiziksel kabul ayrılır.
5. Kanıt `build/ccux/ccux-24<harf>/` altında: komut/exit code, test raporu,
   ekran görüntüleri, manuel adım/sonuç ve kullanılmayan gerçek donanım açıklaması.
   Önceki başarılı testler yeni koşum olarak yazılmaz.
6. Yalnız değişen davranışın gereksinim/sözleşme bölümü güncellenir. Kullanım
   rehberi çalışan ürün değişince güncellenir. `docs/progress.md` güncel özet,
   `docs/archive/YYYY-MM-DD-ccux-24<harf>.md` kalıcı kapanıştır; arşiv indeksine bağlanır.
   `python scripts/check-context-docs.py`, diff kontrolü ve görev ID'li commit;
   kullanıcı istemeden push yoktur.

## Korunacak teknik sınırlar

- UI, SDK veya solver'ı doğrudan çağırmaz. Üç applicationType ortak Editor ve
  Runtime kullanır. Reçete, yürütme ve parça karar durumları ayrıdır.
- Kaynak/model kimliği, exact uyum, restart izni, A→B→A invalidation,
  immutable Runtime, no-op hash, tek pending istek ve Stop/drain korunur.
- UI'dan kimlik/metin kaldırmak kalıcı Logs/CSV, korelasyon veya veri şemasını
  silmez. Görsel tercih reçeteyi dirty yapmaz; kullanıcı parametre değişikliği yapar.
- Eksik sonuçta PASS/skor/pose/mm üretilmez. Bağımsız kontrol başarısı production
  admission, PLC/robot izni veya fiziksel saha kabulü yerine geçmez.
- Eski pixel Circle, eski kalibrasyonlar ve çalışan sihirbazlar korunur. Solver
  temizliği, distorsiyon, hand-eye, native OECS kabulü, CUDA ve dağıtım kapsam dışı.

## Risk ve durma ölçütleri

| Risk | Önlem / kartı tamamlamama koşulu |
| --- | --- |
| Spinbox yüklerken hassasiyet kaybı | Özgün değer + gerçek edit durumu; no-op/hash başarısızsa 24D kapanmaz |
| Yönetim düğmesi kaldırınca erişim kaybı | İşlev-hedef-kabul eşlemesi; karşılığı yoksa ilgili düğme henüz kaldırılmaz |
| Kalibrasyon sadeleşmesini sihirbaza genişletme | 24G ana akış koruma matrisi zorunlu; yöntem/solver silme yok |
| Runtime başka kaynağın/eski çevrimin overlay'ini çizer | Kaynak/frame/cycle/geometri anahtarı; negatif matris geçmeden 24H kapanmaz |
| Zoom davranışının tahmine dayanması | 24E önce yeniden üretim; neden adayı kanıt olmadan kesin neden diye yazılmaz |
| Kartın kapsamı büyür | Önce belgeli alt kart sınırı belirle; tek kartta gizli ek refactor yok |

## Uygulama başlangıç metni

> CCUX-24B'yi uygula. Başlangıç paketini, CCUX indeks/contract'ı,
> 24-usability-plan.md ve 24b-editor-layout.md'yi oku. Yalnız bu kartın kapsamını
> tamamla. Release build, ilgili CTest ve iki boyutta görsel kontrol yap; kanıt,
> progress ve tarihli kapanışı yaz; CCUX-24B görev ID'li tek commit oluştur.
> Push yapma. Kalibre Et akışı ve pasif robot seçeneği korunacak.

Sonraki oturumda kart ID'si/dosyası değiştirilir; bütün kartlar topluca okunmaz.
