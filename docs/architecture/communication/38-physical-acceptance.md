# COMM-38 — Cihaz bazında fiziksel haberleşme kabulü

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-37** ve seçilen protokolün yazılım kabulü tamam; ilgili gerçek cihaz sağlanmış. Temel yazılım tesliminin önkoşulu değildir.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0, C5, C6, C8, C9, C10**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md; mevcut kamera physical/native engel kayıtlarından yalnız ilgili olanlar.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Sağlanan gerçek PLC/robot controller/UA endpoint model-version-option ve mapping sözleşmesini kaydet.
2. İlk olarak salt status/read; yalnız robot pose kapsamı seçildiyse actual pose metadata doğrula; yetkili gerçek sonuç hattında ölçüm/control→payload→consumer ACK korelasyonunu gözle.
3. Kopma/geç ACK/Stop/restart ve config mismatch davranışını cihaz bazında kanıtla; sağlanmayan cihazı açık bırak.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `docs/archive/YYYY-MM-DD-comm-38*.md; build/comm/COMM-38/ trace/screenshots; gerekirse dar device mapping fixture`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Her gerçek endpoint için request/cycle/result/sequence ve uygulama ACK eşleşmesi kayıtlı.
- Geçerli olmayan kamera/ölçüm veya pose producer'ı simulatorla production'a yükseltilmez.
- Cihaz yoksa yalnız hazırlık/engel kaydı; kart fiziksel tamam diye kapanmaz; robot hareket deneyi bu kapsamda değil.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Yetkisiz fiziksel actuator/motion komutu yok. Donanım eksikliği ayrı protokol yazılım kabulünü geri başlatmaz.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-38/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-38.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-38: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **Sağlanmayan endpoint kabulü açık kalır; kullanıcı yeni kapsam seçer**.
