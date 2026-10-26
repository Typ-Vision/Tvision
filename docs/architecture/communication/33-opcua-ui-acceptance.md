# COMM-33 — OPC UA UI ve yazılım kabulü

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-32** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C4, C5, C9**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md; opcua-backend.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Endpoint profiline URL/role/security/credential reference ve browse-node picker ekle.
2. Mapping panelinde node URI/type/status/time ve trigger/result/ACK alanlarını göster.
3. Simulator üzerinde UI kaynaklı ölçüm/control/tetik/reconnect/ACK matrisini çalıştır.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowCommunication.cpp; mapping editor; OPC UA testleri`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Browse cache invalidation/reconnect ve unsupported SDK state görünür.
- İki boyut görsel, Release ilgili CTest, rejected cert ve stale value UI kontrolü.
- V2/TCP3/Modbus regression korunur; fiziksel UA server/PLC kabulü COMM-38'e açık kaydedilir.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Server role, PubSub, SDK deployment kabulü veya hand-eye sihirbazı yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-33/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-33.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-33: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-34**.
