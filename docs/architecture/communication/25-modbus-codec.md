# COMM-25 — Modbus register mapping codec'i

Durum: başlamadı. Önerilen düzey: **low**. Önkoşul: **COMM-24** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C8**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/modbus-backend.md (COMM-24 üretir).
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Pure register encode/decode: Bool/UInt16/Int16/Int32/UInt32/Float32 ve explicit Float64/Int64 mapping gerekiyorsa dar destek.
2. Address/count/byte+word order/scale/string sınırı doğrula; overlap ve out-of-range reddet.
3. Example staging request/result/sequence/ACK register tablosunu fixture olarak kaydet.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/application/ModbusRegisterCodec.*; tests/modbus_codec_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Known byte patterns ve mixed endianness fixture'ları exact; Float32 narrowing hatası policy açık.
- 0-based adres, unitId, illegal overlap/length/count ve overflow reddi.
- Pose altı bileşene ancak explicit profile mapping ile açılır; valid/sequence ayrı alan.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Transport ve polling yok; gerçek PLC'ye yazma yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-25/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-25.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-25: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-26**.
