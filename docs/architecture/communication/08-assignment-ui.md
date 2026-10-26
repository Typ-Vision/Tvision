# COMM-08 — Çok satırlı Set Variables UI

Durum: **tamamlandı** — [kapanış ve kanıt](../../archive/2026-10-26-comm-08.md).
Önerilen düzey: **medium**. Önkoşul: **COMM-07** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C2**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/requirements/02_machine_vision_ui_gereksinimleri.md: bölüm 33–34.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Toolbox'a Set Variables ekle; Properties'de satır ekle/sil/taşı, target ve compatible source picker oluştur.
2. UI draft'ını tek servis komutuyla atomik commit et; boş/uyumsuz atamayı açık hata göster.
3. Her satır source/target type/unit ve sonuç validity'sini gösterebilsin; normal tool seçimi/Run Tool davranışını koru.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowEditor.*; MainWindowExecution.cpp; öneri SetVariablesEditor.*; tests/editor_property_commits.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- 3 satır farklı gerçek port/constant ile Save→reopen→Run Flow denenir.
- Satır sırası aynı-grup RHS anlamını değiştirmez; duplicate/invalid target görünür reddedilir.
- İki boyut görsel; seçili satır/klavye çalışır; preview dış yazım=0.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

İlk Editor kilometre taşı; Runtime/publish veya script alanı açma yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-08/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-08.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-08: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-09**.
