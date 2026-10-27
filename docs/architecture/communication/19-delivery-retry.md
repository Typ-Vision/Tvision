# COMM-19 — Retry, kapasite ve namespace politikası

Durum: yazılım kabulü tamamlandı ([kapanış](../../archive/2026-10-27-comm-19.md)).
Önerilen düzey: **medium**. Önkoşul: **COMM-18** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C5, C6**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md: timeout/128 session/storage sınırları.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Monotonic bounded retry scheduler'ı endpoint portuna bağla; retry aynı persisted bytes kullanır.
2. C5 required blocking/optional128/1-inflight ve mevcut disk budget denetimini uygula.
3. Dedup tombstone retention ile controlled namespace rollover/re-arm davranışını API ve belgede kesinleştir.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `DeliveryStore.*; öneri DeliveryCoordinator.*; TriggerRouter.*; tests/delivery_retry_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- ACK kaybı/geç ACK/retry bitti/reconnect durumunda tek result ve tek cycle.
- Required offline admission kapanır; optional backlog sınırda açık QUEUE_FULL, silent drop yok.
- 128'den fazla ardışık ACK'li v3 request RAM session limitine takılmaz; disk dedup budget dolunca kontrollü ret, old key yeniden kullanımı yok.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

ACK/silent eviction ile veri kaybı yok; v2 128 sınırı sessiz genişletilmez.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-19/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-19.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-19: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-20**.
