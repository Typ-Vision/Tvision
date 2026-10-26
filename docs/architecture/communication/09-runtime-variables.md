# COMM-09 — Immutable Runtime variable context

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-08** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C1, C2, C4**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/multi-source-runtime-terminal.md; docs/architecture/runtime-error-recovery.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Loaded snapshot'a definition/assignment dependency kimliğini kat; cycle-local değerleri her çevrimde yeni context'te başlat.
2. Aynı executor/evaluator'ı mevcut Runtime worker yoluna bağla; UI/Editor cache'i production sonucu olarak tüketme.
3. Stop/ERROR/stale generation çıkışında önceki context'in görünmesini engelle; draft edit loaded context'i değiştirmesin.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/application/RuntimeService.*; RuntimeCoordinator.*; ProductionRecipeService.*; ExecutionService.*; tests/runtime_coordinator_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Ardışık iki çevrimde ilki valid ikincisi missing olduğunda eski değişken geri gelmez.
- Draft rename/value-binding edit loaded snapshot/hash'i değiştirmez; Stop+Load gerekir.
- Stop/cancel producer tamamlanmadan PASS/publish sonucu yok; native admission kapıları korunur.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Network ve terminal schema genişlemesi COMM-13'e aittir.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-09/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-09.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-09: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-10**.
