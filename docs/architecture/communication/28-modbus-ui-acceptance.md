# COMM-28 — Modbus UI ve yazılım kabulü

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-27** tamam.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C4, C5, C8**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md; modbus-backend.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Endpoint/mapping paneline Modbus role/unitId/register/type/order/scale alanlarını ekle.
2. Fixture register tablosunu açıklayan küçük kullanım notu ve simulator senaryosu ekle.
3. Ölçüm/kontrol output + software trigger + retry/restart akışını UI üzerinden doğrula.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowCommunication.cpp; mapping editor; Modbus testleri`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- V2/TCP3 çalışması korunur; unsupported backend açık disabled/unavailable.
- Release ilgili CTest + iki boyut görsel; simulator typed değerler/sequence/ACK exact.
- Fiziksel PLC yoksa yalnız yazılım kabulü kapanır ve COMM-38 maddesi açık kalır.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Yeni Modbus server rolü veya canlı PLC adresi tahmini yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-28/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-28.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-28: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki görev: seçili alt fazın entegrasyon/regresyon kabulü (COMM-36/37 ilgili kapsamı); başka alt faz otomatik başlamaz.
