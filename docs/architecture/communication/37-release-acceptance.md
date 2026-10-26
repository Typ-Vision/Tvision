# COMM-37 — Birleşik Release ve regresyon kabulü

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-36** tamam; her etkin adaptörün COMM-23/28/33 kabulü mevcut.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0–C7**; C8–C10 yalnız seçili ekler için.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. UI ON/OFF Release ve mevcut full CTest + yeni haberleşme suite'lerini çalıştır; fail'leri dar düzeltmelerle kapat.
2. Ölçüm, mevcut kontrol ve genel pose/çoklu TCP hedef simulator senaryolarını üç applicationType için ortak pipeline'da doğrula. Sample/FANUC/Modbus/OPC yalnız seçilmiş ve uygulanmışsa eklenir; temel fazı bekletmez.
3. Kısa kullanım rehberini ve implemented capability tablosunu kaynak/kanıtla güncelle; kapalı/SDK engelli adaptörleri ve koşulmayan kabulü açık listele, hazırlığı ürün özelliği gibi sunma.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `CMakeLists.txt; tests; docs/camera-runtime-usage.md veya ayrı communication usage rehberi; README ilgili kullanım linki`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Legacy no-op/reopen/v2, 2D/kalibrasyon/native admission regression korunur.
- İki boyut görsel, Stop/restart/dedup/disk/queue/invalid/fan-out matrisi kanıtlı.
- SDK OFF ve opsiyonel adapter unavailable halleri; full sonuç listesi gerçek koşumdan.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Fiziksel PLC/robot ve kurulum/deployment/CUDA kabulünü bu kartla kapatma.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-37/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-37.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-37: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Temel yazılım fazı burada kapanır. Sonraki görev: cihaz sağlanmışsa **COMM-38**; aksi halde fiziksel kabul açık kalır. İhtiyaca bağlı alt faz yalnız kullanıcı seçimiyle başlar.
