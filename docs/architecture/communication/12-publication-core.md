# COMM-12 — Endpoint profili, output group ve yayın intent'i

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-11** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C4**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md: Rol ve sınırlar.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. EndpointProfile DTO/service portlarını machine-local store'a; recipe output group logical binding'ini proje servis yoluna ekle.
2. Publish Data config/registry/DAG/executor bağlantısını ekle; evaluator yalnız PublicationIntent üretsin, adapter fake'ı test injection'da kalsın.
3. Required/optional endpoint ve profile contract revision validation/hash ilişkisini uygula.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/application/CommunicationProfiles.*; PublicationService.*; src/domain/PublicationConfig.*; tests/publication_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Aynı bundle iki farklı logical output group'a bağlanabilir, field limitleri ve dependency mismatch reddedilir.
- Disabled/unresolved profil açık hata; secret recipe/log payload'ına girmez.
- Editor Run Flow/Tool ve Load profile bağlantı/yayın başlatmaz; active config edit hash'i sessiz değiştirmez.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Disk outbox, transport, socket activation ve profile UI yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-12/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-12.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-12: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-13**.
