# COMM-02 — Typed scalar değer ve geçerlilik

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-01** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C1**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/tool-execution.md: Katmanlar ve veri.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Mevcut typed value yapısına C1 scalar type/tag/validity/unit zarfını ekle veya uyumlu küçük domain tipi oluştur.
2. Strict serialize/parse, Int64 decimal string, finite/range/string byte limit denetimini aynı saf codec sınırında uygula.
3. Mevcut tool scalar alanlarının anlamını ve hash/no-op yolunu değiştirme.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/domain/ToolValues.*; öneri CommunicationValues.*; tests/execution_test.cpp veya bağımsız communication_values_test`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- false ve 0 geçerli değer; missing null ve reason ile ayırt edilir.
- Int32/UInt32/Int64 sınırları, taşma, NaN/Inf, UTF-8 byte sınırı, bilinmeyen enum/tag reddi testlidir.
- Legacy value/recipe fixture değişmeden okunur; core UI OFF derlenir.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Pose matematiği, variable tanımı, UI ve ağ yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-02/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-02.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-02: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-03**.
