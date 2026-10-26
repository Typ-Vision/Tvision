# COMM-13 — Immutable terminal bundle ve kayıt codec'i

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-12** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C3, C4**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/multi-source-runtime-terminal.md: terminal bölümleri; docs/architecture/project-format.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Typed fields/decision/origin ile ResultBundle'ı terminal modeline optional versioned extension olarak ekle.
2. ProductionResultCodec/Store yolunda aynı cycleId/bundleId/configHash ilişkisinin persist/read/restart görünümünü ekle.
3. Legacy terminal decode/no-op ve CSV/log scalar davranışını koru; schema bump'ı source-map'teki current max'tan seç.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/application/ProductionResultCodec.*; ProductionStore.*; RuntimeService.*; tests/multi_source_terminal_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Scalar/Pose/missing fields roundtrip; required invalid grup normal payload gibi işaretlenmez.
- Terminal kayıt failure→retry aynı kimlik ve byte bundle; yeni acquisition yok.
- Legacy terminal geçmişi okunur; bu aşamada network yazımı=0.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Outbox disk state machine ve ACK COMM-14; yeni kayıt retention tasarımı yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-13/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-13.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-13: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-14**.
