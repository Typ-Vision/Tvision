# COMM-03 — Frame/birimli Pose ve Transform

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-02** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C1, C10**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/robot-spatial-data-review.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. C1'e göre FrameRef, Rotation3D, SpatialTransform değerlerini ve codec'lerini ekle; translation mm, quaternion wxyz.
2. Inverse/compose gerekiyorsa dar saf yardımcı sun; frame/revision uyuşmazlığını reddet, silent Euler/px dönüşümü yapma.
3. Tool value türüne explicit spatial tag ekle; eski Pose2D/Reference koordinatlarını dönüştürme.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/domain/Coordinates.*; ToolValues.*; öneri SpatialTransform.*; tests/spatial_values_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Quaternion zero/nonfinite reddi ve q/-q eşdeğerliği; frame/revision mismatch reddi testlidir.
- Identity/inverse ve iki nontrivial rotation+translation zinciri known fixture ile doğrulanır.
- Translation+quaternion 7'li ile HALCON 7-tuple formatları karıştırılmaz; old Pose2D fixture korunur.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Hand-eye solver, robot profile, calibration UI ve yeni pose üretimi yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-03/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-03.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-03: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-04**.
