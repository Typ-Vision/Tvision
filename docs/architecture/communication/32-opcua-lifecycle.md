# COMM-32 — OPC UA güvenlik, reconnect ve Stop

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-31** tamam.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C4, C5, C6, C9**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/opcua-backend.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Profile security mode/policy/certificate trust ve credentialRef'i backend session'a bağla.
2. Reconnect bounded backoff/generation fence ve Stop/drain'i tamamla; queued callback invalidation uygula.
3. Trust/expired/wrong server/session closure diagnostics'in secret redaction'ını sağla.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `OpcUaClientAdapter.*; CommunicationProfiles.*; tests/opcua_lifecycle_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Unknown/wrong/expired certificate reddi; None/plaintext fallback yok.
- Stop/config switch sonrası callback yeni snapshot/cycle'a karışmaz.
- Reconnect required pending sonucu aynı outbox'tan taşır; otomatik Run veya duplicate acquisition yok.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

OS trust store genel yönetimi veya unattended trust all yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-32/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-32.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-32: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-33**.
