# COMM-35 — FANUC veri profili ve pose codec'i

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-03 ve COMM-23** tamam; COMM-34 gerekli değildir.
**İhtiyaca bağlı alt faz:** yalnız kullanıcı bu alt fazı seçtiğinde uygulanır;
temel fazın teslim önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C10**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/robot-spatial-data-review.md; COMM-03 spatial codec sözleşmesi.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Named FANUC XYZ-WPR profile'ı için resmi üretici/entegrasyon kaynağındaki matris tanımını doğrula; exact source ve controller belirsizliğini yaz.
2. mm/degree WPR↔common transform codec'i uygula; frame/TCP/group metadata ayrı zarf kalsın.
3. CalibrationSample gerektirmeden genel spatial input/output üzerinde çalış; Known nontrivial combined-angle fixture'larıyla iki yönlü dönüşüm testle; profile binding actual input/output generic record mapping'e bağlanır.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `öneri src/application/RobotPoseCodec.*; tests/robot_pose_codec_test.cpp; docs/architecture/communication/robot-profile.md`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Axis-only değil mixed rotation+translation golden matrix; q/-q ve Euler branch eşdeğerliği testli.
- mm/m, degree/rad veya active TCP/frame eksikse reddi; turn/config alanları uydurulmaz.
- Gerçek endpoint bilgisi olmadan profil yalnız software codec/simulator desteği olarak etiketli.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

FANUC SDK/native driver, inverse kinematics, PR motion veya robot hareketi yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-35/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-35.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-35: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki görev: seçili alt fazın entegrasyon/regresyon kabulü (COMM-36/37 ilgili kapsamı); başka alt faz otomatik başlamaz.
