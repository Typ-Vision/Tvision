# COMM-29 — OPC UA SDK/toolchain probe'u

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-23** tamam; Modbus probe'u engeldeyse bağımsız ilerleyebilir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C9**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Qt/MinGW için OPC UA client SDK adayını güncel resmi API/lisans/ABI belgeleriyle incele; küçük bir backend seç.
2. Clean Release browse/read/write/subscription/close probe'u oluştur; UI OFF ve dependency OFF'i koru.
3. opcua-backend.md içinde exact SDK/version/build flags/security capability ve packaging sınırını kaydet.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `CMakeLists.txt; öneri tests/dependency-fixtures/opcua/; build/comm/COMM-29/`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Simulator'da sertifika/session kuralları açık probe; yanlış cert örneği otomatik trust olmaz.
- Yerel clean link/run veya eksik runtime/ABI engeli somut kanıtlı.
- SDK seçimi olmadan COMM-30'a geçilmez; lisans/dağıtım doğrulaması iddia edilmez.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

SDK'yı sessizce tüm build'lerin zorunlu dependency'si yapma; server yazma yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-29/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-29.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-29: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-30**.
