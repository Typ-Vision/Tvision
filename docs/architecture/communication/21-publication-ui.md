# COMM-21 — Publish Data aracı ve mapping UI

Durum: yazılım/görsel kabulü tamamlandı; [kapanış](../../archive/2026-10-27-comm-21.md). Önerilen düzey: **medium**. Önkoşul: **COMM-20** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C2, C3, C4**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/requirements/02_machine_vision_ui_gereksinimleri.md: bölüm 26,33–34.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Publish Data Toolbox/Properties'de source bundle/field picker, output group ve required/optional endpoint binding'i sun.
2. Logical mapping ile physical TCP JSON alan eşlemesini ayrı panelde profile servisine bağla.
3. Type/unit/frame/duplicate field validation ve resolved preview göster; preview gönderim yapmasın.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowEditor.*; MainWindowCommunication.cpp; öneri PublishDataEditor.*; PublicationService.*`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- SetVariables→Result/Publish kayıt/açılış sonrası aynı UUID binding'leri kullanır.
- 3 field bir pakette; incompatible/missing mapping açık hata.
- İki boyut görsel, Run Tool network=0 ve normal Runtime terminal sonrası publish=1.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Robot hareketi, otomatik OPC browse ve gelişmiş script yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-21/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-21.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-21: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-22**.
