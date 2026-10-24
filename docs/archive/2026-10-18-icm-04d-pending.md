# ICM-04d — Stop/Start sonrasında gecikmiş eski kare

Tarih: 18 Ekim 2026. Uygulayıcı: Codex.
Durum: **uzun pozlama/gecikme tanısı tamam; genel Stop/Start bariyeri çürütüldü;
ICM-04d/native admission açık**.

## Kapsam ve sonuç

[Önceki kısa pozlama kaydı](2026-10-18-icm-04d-barrier.md) değiştirilmedi.
Exact cihaz Opto Engineering Srl / COE-050-M-USB-050-IR-C / 02A2220000627.
Yeni opt-in deney: üç exposure/delay profili × iki tur × command'dan sonra
0/100 ms bekleme = 12 senaryo. Her senaryoda kontrol karesi, tüketilmeyen command,
Clear→Stop→Start, yeni command olmadan 1250 ms gözlem bütçesi ve ayrı yeni command.
Profil değişimi stream kapalıyken; her float yazımından sonra readback doğrulandı.

| ExposureTime / TriggerDelay | Eski command karesi | Yeni command'ın frame/trigger sayacı |
| --- | --- | --- |
| 500000 / 0 µs | 0/4 | 1/1 |
| 5000 / 500000 µs | 4/4 | 2/1 |
| 500000 / 500000 µs | 4/4 | 2/1 |

**36 command, 32 alınan kare; sekiz eski command karesi restart sonrasında geldi.**
O aşamada yeni command yoktu; doğrudan nedensel karşı örnektir. Eski karelerin
frameCounter/triggerIndex/frameNumber alanları 1/0/0; sonraki yeni kareler 2/1/1.
Byte 4 ve 8 little-endian embedded sayaçları alanlarla eşleşti. FrameCounter=1
tek başına freshness değildir; iki-sayaç kuralı bu eski kareleri kabul eder denmedi.

Kontrol karelerinin bekleme süreleri profil sırasıyla 528, 533 ve 1028 ms oldu;
500 ms gecikme software command yolunda da gözlendi. Stop çağrıları yaklaşık
37–57 ms döndü; sekiz eski kare post-restart bekleme başladıktan 365–961 ms sonra
geldi. Dört sıfır-gecikme gözlemi 1265–1268 ms sessizdi; garanti değildir.
Command'ın device/transport içindeki hangi aşamada kaldığı veya tam exposure
başlangıcı bu tanıdan çıkarılamaz. Clear→Stop→Start genel barrier olarak elendi;
sıfır-gecikmeye kısıtlı stratejinin koşulları henüz kanıtlanmadı.

## Ayarlar ve sahiplik

Başlangıç ExposureTime=5000 µs, TriggerDelay=0. Destek min/max okunup gerekli
aralık doğrulandı. İki float değer snapshot alındı, sadece tanı boyunca değişti;
cleanup sırasında eski değerler exact readback ile doğrulandı. ExposureAuto NA;
auto/gain/ROI/UserSet/firmware yazılmadı. Trigger/source/burst/cache ve sekiz
metadata bankası/selector önceki tanıdaki snapshot/restore yolunu kullandı.
Yalnız Timestamp/Framecounter/ExtTriggerCount açıldı. Her borrowed buffer free
edildi; raster ürün edinimine verilmedi, PGM üretilmedi. İlk satır dışlama izni
korunur; ürün geometri/kalibrasyon entegrasyonu hâlâ yoktur.

## Değişen dosyalar

- `tests/industrial-camera-probe/trigger.cpp`: `--pending-barrier`, timing
  snapshot/range/readback/restore, üç profil, bounded gözlem, kısmi senaryo hata
  kanıtını koruma. Mevcut `--stream-barrier` ve diğer tanı seçenekleri korunur.
- `industrial-camera-trigger-correlation.md`: genel bariyerin karşı örneği,
  ayrı sayaç resetleri ve delay=0 yorum sınırı.
- `industrial-camera-vendor-questions.md`: pending command cancellation/drain
  ve 1/0→2/1 reset örneği; **gönderilmedi**.
- Progress ve arşiv dizini. Ürün/UI/kalıcı şema/local INI değişmedi.

## Yeni doğrulama ve kanıt

`build/icm/04d-pending/`:

- `configure.log`, `build.log`: ayrı opt-in Release build exit 0; önceki
  `build/icm/02/abi` binary'si ve eski kanıtlar üzerine yazılmadı.
- `sandbox.json`: cihaz görünmedi, command/frame 0; donanım yokluğu kanıtı değil.
- `native.json`: son binary 12 senaryo/36 command/32 kare, exit 0;
  timing/settings/frameSpec restore ve tüm cleanup kodları başarılı.
- `inventory-after.json`: bağımsız salt okunur process, 1 cihaz; open/close/
  destroy başarılı. Eski trigger/source/burst/cache, aktif metadata false,
  ExposureTime=5000 ve TriggerDelay=0 doğrulandı. Tüm metadata bankalarının
  readback'i tanı cleanup'ında yapıldı.
- `validate.py`, `validated-summary.json`: senaryo matrisi, 8/8 karşı örnek,
  1/0/0→2/1/1 sayaçları, payload/layout, zaman sırası, sessiz pencere süresi,
  restore ve source/binary/report SHA256 assertions geçti.
- `invalid-cli.json`: relative DLL ret, exit 1, command/frame 0.
- `ctest.log`, `ctest.xml`: mevcut SDK build'inde ilgili CTest **2/2 yeni koşum**.
  Ürün yeniden derlenmedi; tam suite/UI/görsel/1000-cycle stress koşulmadı.
- `context-check.json`: bağlam boyut/link kontrolü.

Yeni web araması Opto kaynağında ek Stop completion garantisi bulmadı.
[Üretici kılavuzu](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual_1.2.0_20230428.pdf)
§3.4.2 gecikmeyi açıklar; pending command cancellation/SDK completion sözleşmesi
yerine geçmez. Başka üreticiye ait benzer API açıklamaları exact kanıt sayılmadı.

## Sınırlar ve sonraki görev

Native Unavailable; ICM-04d açık, 04e başlamaz. Gecikmeyi sonradan sıfırlamak,
sessiz beklemek veya close/open yapmak henüz recovery kanıtı değildir. Önceki
sahip/preview'den kalan gecikmiş command ihtimali delay=0 readback'le kapanmaz.
Sıradaki bağımlılık exact firmware için belgeli pending-command cancel veya
completion/drain/reset sınırıdır; üretici soru taslağı somut kanıtla hazırdır.
Gönderim yetkisi bu görevde yok; gönderilmedi. Aynı sessizlik matrisini tekrarlamak
bu engeli kapatmaz. Kopma/reset/command failure ve duplicate teslim semantiği,
G0–G4/H01–H08, fiziksel sahne/lens/ölçüm, dağıtım/CUDA kabulü açık kalır.
ICM-04d görev ID'li tek yerel commit; push yok.
