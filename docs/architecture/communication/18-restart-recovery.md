# COMM-18 — Restart, dedup ve reconciliation

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-17** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C5, C6, C7**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/runtime-error-recovery.md; docs/architecture/plc-socket.md: restart sınırları.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Ledger/outbox'tan startup durumunu yeniden kur; session değişse de stable request key duplicate kontrolünü sürdür.
2. Terminal+bundle replay ve begin/intentsiz belirsizlik durumlarını C5'e göre ayır; otomatik acquisition/Run yapma.
3. Recovery inspection/resume-delivery servis komutunu ekle; unknown request'i sentetik ACK/sonuçla kapatma.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `DeliveryStore.*; TriggerRouter.*; CommunicationTcpEndpoint.*; ProductionStore.*; tests/delivery_recovery_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Ayrı process sonlandırma/restart fixture'ları intent, begin, terminal/bundle ve ACK sınırlarını kapsar.
- Terminal pending replay aynı bytes/resultId; duplicate yeni cycle başlatmaz.
- Terminal olmayan request RECOVERY_REQUIRED; startup hazır/Running olmaz; v2 geçmişi kendi policy'sinde kalır.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Genel sistem crash garantisi veya exactly-once iddiası yok; repair UI sonraki diagnostic kartında.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-18/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-18.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-18: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-19**.
