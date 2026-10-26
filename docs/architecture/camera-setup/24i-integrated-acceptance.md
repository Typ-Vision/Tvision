# CCUX-24I — birleşik kullanılabilirlik kabulü

Durum: birleşik yazılım/offscreen görsel kabul tamamlandı;
[kapanış ve on talep matrisi](../../archive/2026-10-26-ccux-24i.md).
Native wheel/pan, native DPI/çok monitör ve fiziksel kabul açık.
Önkoşul: 24B–H uygulanmış ve kart kanıtları bağlı olmalı.
[Ortak plan](24-usability-plan.md), contract ve ilgili kapanışlar okunur;
tamamlanmış görevler yeniden uygulanmaz.

## Kabul hazırlığı

1. Son commit, değişen dosyalar, test hedefleri ve kart başına açık sınırlamaları
   birleştir. Kapanmamış zorunlu yazılım kabulü varsa önce ilgili karta dön.
2. Test projesi/reçetesi, legacy hassas değerler/pixel Circle, kalibrasyon
   kayıtları, iki kaynak ve karışık araç fixture'larını ayrı build kanıt alanında
   hazırla. Kullanıcının gerçek proje veya config/local.ui.ini'sini testle değiştirme.
3. Release UI ON ve README'deki UI OFF yapılandırmalarında etkilenen hedefleri
   derle. Her build'in `ctest -N` listesinden ilgili testlerin birleşimini çıkar;
   GUI testlerini UI OFF'ta varmış gibi çağırma. Aynı suite'i gereksiz tekrarlama.

## Uçtan uca senaryolar

| Akış | Birlikte doğrulanan maddeler |
| --- | --- |
| Kamera kaydı → Optik/Geometri → tek/canlı preview → Stop | Büyük viewer, görünür kurulum, zoom, gerçek readback ve drain |
| Kalibre Et → Bilinen ölçü / plaka → canlı/dosya → hesap → kontrol → Kaydet ve kullan | Her iki çalışan yöntem ve mevcut seçenekleri korunmuş; kontrol durumu doğru |
| Kalibre Et → pasif robot; Ayrıntılar aç | Robot yalnız ana seçimde pasif; Ayrıntılar düğme kalabalığı temiz; geçmiş erişilir |
| Yeni Circle → ROI/parametre → Run Tool → Run Flow | Calibrated varsayılanı, mm girişleri, kalıcı Flow, sade panel |
| Eski pixel/çok basamaklı reçete → no-op Save → reopen | Pixel, özgün precision, içerik/hash ve model bağları korunur |
| Çok kaynaklı Runtime Load → doğrula → Run → çevrim → toggle → Stop | Doğru immutable snapshot, güncel kaynak overlay'leri, tanı/Logs/CSV korunur |
| Hata/ret/referans kaybı, ikinci çevrim, kaynak değişimi | Sahte PASS/mm/pose ve stale katman yok; yürütme ile parça kararı ayrı |
| Restart ve eski/bozuk yerel INI | Kalıcı Flow/tercihler güvenli; acquisition/Run kendiliğinden başlamaz |

Üç applicationType aynı Editor/Runtime üzerinden örneklenir; robot türünde
gerçek robot kalibrasyonu veya hareket yapılmaz. 1400×900 ve 850×600'de layout,
klavye odağı, scroll, dialog düğmelerine erişim ve gerçek wheel/pan kontrolü.
Fiziksel kamera mevcutsa ayrı kanıtla temel akış gözlenir; yoksa fixture sonucu
fiziksel kabul yerine konmaz. Native DPI/çok monitör ayrı kontrol yapılmadıkça açık.

## Teslim ölçütü

On talebin her biri için ilgili kart, yeni test/görsel yolu ve geçti/kaldı/açık
durumu raporlanır. 24G'nin ana sihirbaz koruma matrisi zorunludur. Başarısız test
veya yapılmayan görsel kontrol genel “tamamlandı” cümlesiyle örtülmez.

Kanıt `build/ccux/ccux-24i/`: UI ON/OFF build/test komutları ve exit code,
test birleşim listesi, görseller, manuel senaryolar, ortam/fixture kimlikleri.
`python scripts/check-context-docs.py` ve diff kontrolü çalıştırılır. Kullanım
rehberi yalnız teslim edilen değişiklikleri anlatır; plan çalışan özellik sayılmaz.

Progress ve kapanış, OECS native Unavailable, fiziksel mm/lens, 15j/22f canlı
kabul, INT ölçüm sınırlamaları ve diğer mevcut engelleri kaybetmez. Bu UX paketi
robot/PLC, CUDA, dağıtım veya native üretim kabulünü kapatmaz.

Sonraki görev otomatik robot devreye alma değildir; güncel kullanıcı önceliği ve
progress'teki açık fiziksel kabul işleriyle seçilir. Ortak tek commit/push yok
kuralı uygulanır. Commit: `CCUX-24I: verify integrated usability changes`.
