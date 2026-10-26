# COMM-30 — OPC UA typed read/write ve node kimliği

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-29** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C4, C9**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/opcua-backend.md (COMM-29 üretir).
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Namespace URI+identifier node resolver ve typed scalar read/write adapter'ını kur.
2. DataType/rank/access/status/timestamps ile common ValueEnvelope dönüşümünü uygula.
3. Logical mapping'in node URI/type/unit bağını doğrula; raw numeric namespace index'i kalıcı ID yapma.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/hardware/OpcUaClientAdapter.*; OpcUaValueCodec.*; tests/opcua_access_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Simulator restart'ta namespace index değişse de doğru URI node'u çözülür.
- Bad/uncertain/status/rank/type/read-only/range yanlışları açık hata.
- Write service success yalnız transport success; consumer delivery ACK değil.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Subscription/trigger ve UI yok; arbitrary ExtensionObject/PubSub yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-30/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-30.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-30: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-31**.
