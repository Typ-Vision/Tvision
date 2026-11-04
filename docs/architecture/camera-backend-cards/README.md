# Kamera SDK ve Basler uygulama kartları

5 Kasım 2026 · CAM-PLAN-03. Durum: **BAS-03 hedefli yazılım kabulü ve fiziksel envanter; G1/G3 açık, production kapalı. Tam suite kullanıcı isteğiyle koşulmadı**.
Kullanıcı kararı: Basler **pylon C API**; uygulama C++20/Qt/MinGW olarak kalır.
[Ana plan](../camera-backend-activation-plan.md),
[önceki analiz](../../archive/2026-11-04-cam-analysis-01.md).

| Sıra | Kart | Bağımlılık | Durum |
| --- | --- | --- | --- |
| 1 | [CAM-SDK-01](CAM-SDK-01.md) | Mevcut yerel konfigürasyon/keşif | Uygulandı; tam ON ertelendi |
| 2 | [CAM-OECS-01](CAM-OECS-01.md) | CAM-SDK-01 | Uygulandı; hedefli ON/OFF geçti |
| 3 | [ICM-BAS-01](ICM-BAS-01.md) | Önceki iki kart; Basler fiziksel erişimi | Uygulandı; native G0, ON 6 + startup, OFF 10 geçti |
| 4 | [ICM-BAS-02a](ICM-BAS-02a.md) | ICM-BAS-01/G0 | Uygulandı; ON 7/OFF 5 hedefli, fiziksel scalar restore ve iki boyut UI |
| 5 | [ICM-BAS-02b](ICM-BAS-02b.md) | ICM-BAS-02a | Yazılım/önizleme uygulandı; tek/100 tetik geçti; G1 açık |
| 6 | [ICM-BAS-03](ICM-BAS-03.md) | ICM-BAS-02b/G1 + mevcut ortak G2 | [Hedefli kabul/engel raporu](../../archive/2026-11-05-icm-bas-03.md); G1/G3 açık |

## Ortak uygulama ve kabul

AGENTS + progress + README + çalışma protokolünden sonra bu dizin ve yalnız
seçili kartın Oku bölümü açılır. Bir oturumun varsayılanı bir karttır; ajanlar
sıralı çalışır. Mevcut ortak sözleşme/test altyapısı kullanılır, test provider
ürün backend'i yapılmaz. Dosya sahipliği karttadır; keşifle gereken ek bağımlılık
belgelenir. Kullanıcı INI/projesi test fixture'ı yapılmaz.

Kod kartında hedefli davranış, hata, sınır ve negatif testlerden sonra son kaynakta
Release ON/OFF tam CTest zorunludur. UI kartında normal/küçük görsel kontrol eklenir.
README toolchain PATH'iyle mevcut build dizinlerinde incremental çalışılır:

```powershell
cmake --build build/reference-ui -j 6
cmake --build build/kmc9-core-off -j 6
ctest --test-dir build/reference-ui -N -R '<kartın hedefli regexi>'
ctest --test-dir build/reference-ui --output-on-failure -R '<kartın hedefli regexi>'
ctest --test-dir build/reference-ui --output-on-failure
ctest --test-dir build/kmc9-core-off --output-on-failure
python scripts/check-context-docs.py
git diff --check
```

Regex her kartta yazılıdır; yeni testler eklenir ve envanterde gerçekten bulunduğu
doğrulanır. UI OFF'ta UI hedefi aranmaz; ilgili core hedefler ve tam suite çalışır.
SDK bulunmayan konfigürasyonda başlangıç/file/webcam/tarihçe ayrıca sınanır;
SDK'sız test başarısı SDK'lı gerçek backend kabulü değildir. Fiziksel probe normal
CTest'e otomatik eklenmez; fixture ve gerçek kanıt ayrı raporlanır.

Her koşum göreve özel temiz ayar/kanıt dizini kullanır; komut, kaynak/test/config,
başlangıç/bitiş, exit ve sayılar sonuç/checkpoint'e yazılır. Başarısız koşum
saklanır; fixture fiziksel kapıyı kapatmaz. Oturum/lease kalmışsa sonraki kart
başlatılmaz. Kapanış: progress güncel özeti + yeni tarihli arşiv + arşiv dizini,
`Uygulayıcı: Codex` veya `Uygulayıcı: Claude Code`, kart ID'li tek commit; push yok.

Basler hedefi acA1920-40um/23102780/U3V; başka model/transport desteği ilan edilmez.
OECS native engelleri korunur. Aynı PC'de iki markanın keşfi ve ayrı oturumları
kapsamdadır; aynı Runtime reçetesinde OECS+Basler ve eşzamanlı çok kamera ayrı iştir.
Bağımsız mm/robot/PLC fiziksel kabulü bu kartların sonucu olarak varsayılmaz.
