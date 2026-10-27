# COMM-10 — Genel parça kararı evaluator'ı

Durum: tamamlandı — [kapanış](../../archive/2026-10-27-comm-10.md). Önerilen düzey: **medium**. Önkoşul: **COMM-09** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C3**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/project-format.md: tool accepted/recipe karar ayrımı.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. ResultConfig için required Boolean binding listesi ve ALL/ANY enum'u ekle; pure evaluator kullan.
2. C3 missing-first karar kuralını uygula; boş rule invalid; execution failure export kararını düşürür.
3. Decision/system output'un write-protected anlamını koru; scalar ölçüm valid ama decision yok durumu destekle.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/domain/ResultDecision.*; src/application/DecisionEvaluator.*; tests/decision_evaluator_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- ALL/ANY truth table; false geçerli NOK, eksik veri NOT_EVALUATED.
- ANY(true,missing) ve ALL(false,missing) NOT_EVALUATED; short-circuit sahte karar yok.
- Boş liste, başka cycle ve ERROR/CANCELED; constant OK ile system overwrite reddi.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Yeni yüzey kusur algoritması, skor→otomatik PASS ve UI yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-10/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-10.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-10: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-11**.
