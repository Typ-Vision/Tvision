# COMM-04 — Variable tanımı, codec ve proje komutları

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-03** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C1, C2**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/editor-definition.md; docs/architecture/project-format.md: format/recipe bölümleri.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Recipe variable UUID/name/type/unit/frame constraint tanımını C2 sınırlarıyla ekle.
2. Add/rename/remove/type edit komutlarını ProjectService'in atomik candidate/dirty/DRAFT yoluna bağla; kullanımda silme/tip değiştirme için açık validation hatası üret.
3. Yeni definition codec'ini explicit edit ile etkinleştir; value'ları recipe'ye serialize etme.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/domain/Project.*; ToolDefinition.*; src/application/ProjectService.*; src/persistence/ProjectRepository.*; tests/project_service_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Save/reopen UUID sabit, rename binding korur, duplicate name/limit/illegal type reddedilir.
- Legacy dosya aç/no-op Save hash'i aynı; yeni alan yalnız gerçek edit ile yazılır.
- Undo/cancel/no-op mevcut komut davranışını korur; bu kartta hiçbir runtime değer kalıcı tanıma yazılmaz.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Variable UI, evaluator ve network yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-04/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-04.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-04: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-05**.
