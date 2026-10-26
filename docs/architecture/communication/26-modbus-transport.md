# COMM-26 — Modbus client transport/lifecycle

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-25** tamam.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C4, C8**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/modbus-backend.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Seçilen backend'de async connect/read/write/timeout/close adapter'ını ortak transport portuna uygula.
2. Owner thread, bounded request/response ve generation callback fence ekle; disabled/OFF hali açık unavailable.
3. Gerçek localhost Modbus simulator process'inde bağlantı kopması ve Stop dene.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/hardware/ModbusTcpAdapter.*; application transport ports; CMakeLists.txt; tests/modbus_transport_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Yavaş/no-response simulator UI thread'ini bloklamaz; timeout sonlu.
- Stop ardından gecikmiş callback yeni config'e yazmaz; socket/session kapanır.
- Dependency OFF ve UI OFF Release korunur; write sonucu consumer ACK sayılmaz.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Trigger/result sequence semantics COMM-27; ekran yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-26/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-26.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-26: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-27**.
