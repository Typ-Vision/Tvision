# COMM-17 — TCP v3 servis entegrasyonu

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-16** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C4, C5, C6, C7**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Ayrı explicit v3 endpoint'i async Qt Network owner thread'de bağla; profile port çakışmasını açık hata yap.
2. INPUT→snapshot, TRIGGER→router/store, committed bundle→RESULT, ACK→ledger yolunu bağla.
3. Local simulator'a v3 seçimi ve ölçüm/kontrol verisi doğrulama komutları ekle; sahte vision sonucu üretme.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/application/PlcRuntimeSocket.* veya ayrı CommunicationTcpEndpoint.*; RuntimeCoordinator.*; src/app/plc_simulator.cpp; tests/plc_socket_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Localhost gerçek Runtime cycle sonucu, ACK commit ve duplicate tek acquisition ile doğrulanır.
- V2 ve v3 ayrı profillerde çalışır; v3 alanları v2 response'una sızmaz.
- Disk commit yoksa socket RESULT yok; disconnect/Stop buffered write sonrasında yanlış Ready yok.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Normal kullanıcı aktivasyonu COMM-23 kabulüne kadar dev feature olarak kalır; restart recovery bu kartı büyütmez.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-17/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-17.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-17: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-18**.
