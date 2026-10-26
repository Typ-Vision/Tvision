# COMM-36 — Çoklu hedef ve pose paketi tutarlılığı

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-23, COMM-34, COMM-35** tamam; etkin Modbus/OPC için ayrıca COMM-28/33 kabulü.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C4, C5, C6, C10**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Aynı committed bundle'ın etkin TCP/Modbus/OPC hedeflerine bağımsız delivery state ile fan-out'unu tamamla. SDK yoksa ortak portun ikinci test endpoint'iyle fan-out sınanabilir; bu native adaptör kabulü değildir.
2. Whole Pose assignment ve mapping projection'ın cycle/config/profile coherence denetimini uygula.
3. Required PLC + optional UA kesinti politikasıyla robot actual sample input→typed variable→çıktı simulator roundtrip'ini testle.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `PublicationService.*; DeliveryCoordinator.*; codec/mapping katmanları; tests/multi_endpoint_delivery_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Bir hedef ACK olurken diğeri retry/pending kalabilir; global dağıtık atomiklik iddia edilmez.
- 6 pose component farklı cycle/revision ise commit yok; source invalidse identity/zero pose gönderilmez.
- Required offline admission block, optional bounded backlog ve restart bağımsız ACK doğrulaması.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Yeni gerçek pick-pose producer, hand-eye matematiği veya fiziksel hareket yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-36/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-36.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-36: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-37**.
