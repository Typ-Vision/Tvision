# COMM-31 — OPC UA subscription/trigger/ACK

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-30** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C5, C6, C9**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/opcua-backend.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Sequence/commit/ACK node mapping'ini C6/C9'a göre subscription input ve outbox output'a bağla.
2. Callback'leri bounded application queue'ya geçir; coherent input snapshot sonrası router çağır.
3. İlk notification/reconnect re-arm, duplicate ve stale sequence denetimini uygula.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `OpcUaClientAdapter.*; TriggerRouter.*; DeliveryCoordinator.*; tests/opcua_trigger_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- İlk connect/high bit/reconnect notification tek başına cycle üretmez.
- Payload partially updated iken trigger alınmaz; matched sequence snapshot testli.
- Typed ölçüm/decision output field'ları commit'ten önce staging, ACK exact request'e kapanır.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Security lifecycle'i büyütme; smoke simulator açık test profilidir.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-31/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-31.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-31: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-32**.
