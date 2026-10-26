# COMM-24 — Modbus dependency ve rol probe'u

Durum: başlamadı. Önerilen düzey: **low**. Önkoşul: **COMM-23** tamam.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C8**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. MinGW/Qt Release ile kullanılabilir Modbus client dependency'sini resmi kaynak/yerel paket üzerinden doğrula; mevcut QtSerialBus varsa probe et.
2. Lisans, clean link/run, timeout/Stop capability ve OFF davranışını küçük build probe'unda kanıtla.
3. modbus-backend.md dosyasına seçilen API/target/install sınırı ve simulator şartını yaz.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `CMakeLists.txt; öneri tests/dependency-fixtures/modbus/; build/comm/COMM-24/`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- UI ON/OFF default build dependency yokken derlenir; adapter unavailable açık durum.
- Probe gerçek simulator'a bounded connect/read/close yapar veya eksik dependency açık kanıtlanır.
- Sürücü/kurulum varsayımı yok; backend doğrulanmadan COMM-25 implementation başlamaz.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Elle Modbus stack yazma veya PLC native register adresi uydurma yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-24/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-24.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-24: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-25**.
