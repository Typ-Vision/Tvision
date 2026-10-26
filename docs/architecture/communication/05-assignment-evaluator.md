# COMM-05 — Atomik çoklu atama evaluator'ı

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-04** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C2**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/contract.md: yalnız C1–C2.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Typed constant/tool-output/upstream-bundle source binding ve assignment config tanımını ekle.
2. Pure evaluator tüm RHS'i immutable input view'dan hesaplasın; bütün grup geçerliyse bundle üretip commit etsin.
3. Duplicate target, missing/stale/cycle/type/unit mismatch için named reason üret.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/domain/AssignmentConfig.*; src/application/AssignmentEvaluator.*; tests/assignment_evaluator_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- İki/üç atama birlikte başarılı; son satır hatalıysa ilk satır görünür commit olmaz.
- A/B aynı grupta eski A'yı okur; aynı hedefe iki satır reddedilir.
- Başka cycle/bundle revision kaynağı, invalid upstream ve read-only system target reddedilir.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

DAG registry, UI, script/expression ve ağ yazımı yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-05/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-05.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-05: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-06**.
