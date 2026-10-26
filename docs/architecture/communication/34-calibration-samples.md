# COMM-34 — Robot actual pose ve CalibrationSample tipleri

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-03, COMM-15, COMM-16** tamam; opsiyonel SDK'lar beklenmez.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C6, C10**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/robot-spatial-data-review.md; docs/architecture/calibration-plan.md: Robot Calibration/model bölümleri.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. RobotActualPose ve CalibrationSample DTO/codec/validation ekle; mevcut camera/target/model varlıklarına ID/revision referansla.
2. Giriş mapping'i üzerinden actual pose'u coherent InputSnapshot alanı olarak taşı; commanded target ayrımını koru.
3. Durağan sample pairing kontrolünü sample/session/frame/clock/birim/kimlik üzerinden yap; geçersiz örneği reason ile döndür.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/domain/CalibrationSample.*; application InputSnapshot/robot data codec; tests/calibration_sample_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Valid known sample roundtrip; farklı time domain veya eşleşmemiş image/robot sequence reddi.
- Base/user/TCP/flange ve revision eksikliği explicit invalid; stale önceki sample'a fallback yok.
- Eye-in/eye-to senaryo metadata'sı taşınır fakat örneklerden kalibrasyon sonucu üretilmez.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Gerçek hand-eye solver, motion program, kalibrasyon UI veya otomatik sample capture yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-34/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-34.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-34: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki görev: seçili alt fazın entegrasyon/regresyon kabulü (COMM-36/37 ilgili kapsamı); başka alt faz otomatik başlamaz.
