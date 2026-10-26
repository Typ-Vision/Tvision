# COMM-07 — Variable Catalog UI

Durum: tamamlandı — [kapanış ve kanıt](../../archive/2026-10-26-comm-07.md). Önerilen düzey: **low**. Önkoşul: **COMM-06** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C2**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/requirements/02_machine_vision_ui_gereksinimleri.md: bölüm 34.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Mevcut ortak Recipe Editor'da Variable Catalog erişimi ekle; ayrı applicationType ekranları kurma.
2. Ad/tip/birim/açıklama ve salt okunur system alanlarını göster; add/edit/remove servis komutlarına gider.
3. Dirty/cancel, referenced variable hata mesajı ve current-cycle value/validity gösterimini bağla.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowEditor.*; uygun yeni küçük VariableCatalogWidget; tests/editor_property_commits.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Add/rename/cancel/delete error; immutable UUID; salt okunur system değiştirilemiyor.
- 1400×900 ve 850×600'de table scroll/klavye/uzun ad okunur.
- Properties editlerini sessiz başka tool'a uygulama yok; kayıtlı reçete no-op dirty olmaz.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Evaluator/DAG tasarımını değiştirme, global variable ve ağ yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-07/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-07.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-07: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-08**.
