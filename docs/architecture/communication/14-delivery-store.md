# COMM-14 — Kalıcı request ledger/outbox

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-13** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C4, C5**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md: Kalıcı kanıt ve hata politikası.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Mevcut ProductionStore atomik/event altyapısını kullanarak REQUEST_INTENT, bundle commit, attempt ve ACK ledger API'sini ekle.
2. Persist edilmiş payload bytes/hash immutable olsun; terminal+bundle tutarlılığını manifest/commit marker ile tanımla.
3. Fake transport testinde yalnız committed payload dispatch edilsin; eksik journal state'lerini reader açık raporlasın.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/application/ProductionStore.*; öneri DeliveryStore.*; tests/delivery_store_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Disk failure intent/terminal/bundle/ACK sınırlarında inject edilir; Ready/ACKED erken verilmez.
- Aynı request key farklı fingerprint conflict; aynı payload retry overwrite etmez.
- Append/reopen ve partial record bozukluğu explicit recovery; eski PLC event'leri v3'e yükseltilmez.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Process-level recovery policy COMM-18; retention/retry scheduler COMM-19; ağ transport yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-14/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-14.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-14: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-15**.
