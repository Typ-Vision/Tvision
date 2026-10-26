# COMM-06 — Set Variables registry/DAG/executor

Durum: tamamlandı — [kapanış](../../archive/2026-10-26-comm-06.md). Önerilen düzey: **medium**. Önkoşul: **COMM-05** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C2**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/tool-execution.md: Katmanlar ve veri; docs/architecture/editor-definition.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. set_variables tool türünü registry/config codec/executor yoluna ekle; input binding'leri explicit dependency üretir.
2. Assignment bundle'ını typed port olarak çıkar; tüketici bağını producer UUID/port/version ile çöz.
3. Editor full-flow/TargetOnly/cache invalidation yolunda saf evaluator'ı kullan; belirsiz branch write validation ekle.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/domain/ToolRegistry.*; ToolGraph.*; ToolDependency.*; src/application/ExecutionService.*; EditorSession.*; tests/execution_test.cpp; tests/session_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Upstream→SetVariables→consumer sırası ve yanlış/çevrimsel DAG reddi testlidir.
- TargetOnly yeni acquisition yapmaz, stale upstream'de full-flow fallback yok.
- Üç applicationType aynı tool'u kullanır; Editor Run Tool/Flow network call=0.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Tool UI ve Runtime production yayınını açma yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-06/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-06.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-06: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-07**.
