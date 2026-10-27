# COMM-36 — Çoklu hedef ve pose paketi tutarlılığı

Durum: yazılım/görsel kabul tamam — [28 Ekim 2026 kapanışı](../../archive/2026-10-28-comm-36.md). Önerilen düzey: **medium**. Önkoşul: **COMM-23** tamam. Seçilmiş Modbus/OPC entegrasyonu için ayrıca COMM-28/33; sample/FANUC senaryosu seçilirse COMM-34/35 gerekir. Temel kabul bu ekleri beklemez.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C4, C5, C6**; C8/C9/C10 yalnız seçilmiş ekler için.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Aynı committed bundle'ı iki TCP simulator hedefine bağımsız delivery state ile gönder; çoklu hedef temel kabulünü hiçbir opsiyonel SDK'ya bağlama. Seçili Modbus/OPC adaptörleri sonradan aynı ortak port üzerinden sınanır.
2. Whole Pose assignment ve mapping projection'ın cycle/config/profile coherence denetimini uygula.
3. Required hedef + optional hedef kesinti politikasını iki TCP simulator ile testle. Genel pose input→typed variable→çıktı roundtrip'ini frame/birim/yönelim formatıyla doğrula; CalibrationSample ve FANUC codec gerekmez. Simulator verisi normal Runtime'da gerçek pose üreticisi sayılmaz.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `PublicationService.*; DeliveryCoordinator.*; codec/mapping katmanları; tests/multi_endpoint_delivery_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Bir hedef ACK olurken diğeri retry/pending kalabilir; global dağıtık atomiklik iddia edilmez.
- Genel pose tek coherent paket olarak taşınır; x,y,z,rx,ry,rz projection seçilirse
  rx/ry/rz yönelim tanımı ve birimleri açık profil/codec sözleşmesiyle belirtilir;
  isimlerden Euler sırası veya robot markası çıkarılmaz. Farklı cycle/revision
  bileşenleri reddedilir; invalid kaynakta identity/zero pose gönderilmez.
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
