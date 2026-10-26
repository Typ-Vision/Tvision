# COMM-16 — TCP v3 codec ve framing

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-15** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C5, C7**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md: vf.socket/2.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. V3 command/result/input/ack strict JSON codec'i ve bounded JSON-LF framer'ı ayrı ekle.
2. C7 zarf/alan kurallarını versioned examples/fixture olarak kaydet; v2 parser'a yeni alan zorlaması yapma.
3. Canonical persisted payload ve hash için deterministik format tanımla.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/application/CommunicationWireCodec.*; tests/communication_wire_test.cpp; src/app/plc_simulator.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Fragment/coalesced LF, 64KiB/partial deadline, invalid UTF-8/schema/Int64 ve future version reddi.
- Scalar/null/Pose wire fixture roundtrip; field order hash canonical.
- V2 bilinen packet fixture'ları byte/semantic olarak korunur.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Socket dinleme, Runtime dispatch ve robot emulator hareketi yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-16/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-16.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-16: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-17**.
