# COMM-11 — Result aracı, portlar ve UI

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-10** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C2, C3**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/requirements/02_machine_vision_ui_gereksinimleri.md: bölüm 26,33.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Result tool'unu registry/DAG/executor'a bağla; COMM-10 DecisionEvaluator kullan.
2. Properties'de ALL/ANY ve compatible accepted port picker ekle; karar/validity portlarını sun.
3. Genel karar summary'sini Runtime/Editor'da mevcut execution state'ten ayrı göster.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/domain/ToolRegistry.*; src/application/ExecutionService.*; src/app/MainWindowEditor.*; MainWindowRuntime.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Gerçek tolerans sonucu ile ölçüm OK/NOK; gerçek kontrol Boolean'ı ile aynı mekanizma.
- Missing/skipped kaynakta summary önceki PASS'ı korumaz; Save/reopen rule aynı.
- İki ekran boyutu, read-only decision ve legacy v2 karar anlamı regression'ı.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Defect classifier/segmenter geliştirme veya fiziksel robot/PLC çıkışı yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-11/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-11.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-11: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-12**.
