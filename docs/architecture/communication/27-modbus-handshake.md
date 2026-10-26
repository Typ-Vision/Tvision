# COMM-27 — Modbus tetik/result/ACK handshake

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-26** tamam.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C5, C6, C8**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/modbus-backend.md; COMM-25 register fixture tablosu.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. PLC request staging snapshot'ını before/after sequence ile tutarlı oku; fresh trigger'ı ortak router'a dönüştür.
2. Outbox payload→register staging→commit sequence sırasını uygula; matched ACK'i ledger'a bağla.
3. Reconnect/high-bit/initial sample/sequence wrap durumunda re-arm ve duplicate namespace policy uygula.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `ModbusTcpAdapter.*; TriggerRouter.*; DeliveryCoordinator.*; tests/modbus_handshake_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Kısmi/karışık request ve pose bileşenleri cycle başlatmaz.
- Duplicate/new while busy, stale ACK, wrap/reconnect ve write interruption simulator testli.
- Consumer commit'ten önce payload kullanmaz; ACK olmadan Ready yok; read failure NOK üretmez.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

PLC motion/robot komutu yok; kısa pulse capture garantisi yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-27/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-27.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-27: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-28**.
