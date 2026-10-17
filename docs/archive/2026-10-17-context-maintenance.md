# 17 Ekim 2026 — başlangıç bağlamı bakımı

## Kapsam ve dosyalar

Kullanıcı onaylı verimlilik planının iki paketi uygulandı:

- `AGENTS.md`: kısa başlangıç, görev/bölüm bazlı okuma, tek güncel devir,
  ayrı tarih/görev kapanışı ve boyut sınırı.
- `README.md`: giriş/kullanım/build rehberi; `docs/progress.md`: güncel durum,
  doğrulama kapsamı, açık engeller ve ICM-01a devam noktası.
- `docs/architecture/industrial-camera-{plan,handoff,tasks-foundation}.md`:
  okuma ve kapanış kuralları; tarihsel ICM-00 mesajı arşive taşındı.
- Üç `docs/requirements/*.md`: her birinde aynı 30 teslim/tekrar paragrafı
  çıkarıldı; özgün gereksinimler ve ortak 8 Ekim tasarım revizyonu korundu.
- `docs/archive/2026-10-17/`: progress'in 133 başlığı, bölüm sınırlarında
  dokuz dosya; önceki README/devir ve üç gereksinimin tam kopyaları,
  görev/başlık dizini ve kaynak/arşiv SHA256 envanteri. `docs/archive/README.md`
  yeni kapanışların bulunacağı üst dizindir.
- `scripts/check-context-docs.py`: UTF-8, başlangıç boyutu, yerel inline
  Markdown bağlantı/başlıkları ve değişmez taşıma arşivi hash kontrolü.

## Doğrulama

Taşıma öncesi dosyalar `build/context-maintenance/before-*` altında korundu.
Arşivde yalnız göreli Markdown bağlantıları yeni konuma uyarlandı, arşiv uyarısı
ve progress dizin hedefleri eklendi. Bağlantı dönüşümünü geri alarak eski metin
ve baytların birebir elde edildiği kontrol edildi. Gereksinimlerde kaldırılanlar
üç belgede birebir ortak teslim paragraflarıdır; diğer paragraflar korundu.
Kaynak/hash envanteri [manifest](2026-10-17/manifest.json), arama dizini
[index](2026-10-17/index.md) dosyasındadır.

Son belge kontrolü **27 belge / 503 yerel bağlantı / 14 arşiv hash'i** için geçti.
Yedi izole CLI kontrolü geçti: geçerli paket, aşırı README boyutu, eksik hedef,
eksik başlık, değişmiş arşiv, Windows CRLF checkout ve geçersiz UTF-8.
Hatalı durumlar exit 1, geçerli/CRLF durumları exit 0 döndürdü.
`git diff --check` exit 0. Sonuçlar `build/context-maintenance/verification.json`,
tekrar üretim yardımcıları `verify.py`, `migrate.py`, `requirements.py` altındadır.
Arşiv hash'i Git/Windows satır sonu dönüşümüne dayanmak için yalnız CRLF→LF
normalizasyonu kullanır; kaynak hash'leri taşıma öncesi özgün baytları temsil eder.

| Başlangıç belgesi | Önce (bayt) | Sonra (bayt) |
| --- | ---: | ---: |
| AGENTS | 1.166 | 2.533 |
| README | 69.729 | 6.677 |
| progress | 474.742 | 5.525 |
| Toplam | 545.637 | 14.735 |

Başlangıç paketi %97,3 küçüldü ve 20.000 bayt sınırının altında.
Üç gereksinimin toplamı 140.311 → 79.121 bayt oldu. Arşivler zorunlu başlangıç
okumasına dahil değildir; bilgi silmek yerine görev sırasında bulunabilir tutuldu.

Ürün/UI/CMake/SDK davranışı değişmediğinden Release/CTest ve görsel ürün kontrolü
bu bakımda çalıştırılmadı; önceki başarılar yeni koşum gibi sunulmadı.

## Sınırlamalar ve devam

Boyut kazancı UTF-8 dosya baytlarıdır; model token/süre ölçümü değildir.
Kontrol betiği web erişimini, tüm Markdown sözdizimini veya ürün kanıtının içeriğini
doğrulamaz. Gereksinimlerde özgün tarihli plan/kararlar, mimari belgelerde tarihsel
kayıtlar kalır; bunlar görevle ilgili bölümler üzerinden okunur. Bütün teknik
belgeler yeniden yazılmadı. Arşiv Git'e dahil edilebilir; `build/` kanıtları yereldir.

Önceki dirty değişiklikler korundu; commit/push ve `config/local.ui.ini` değişikliği
yok. Sonraki ürün görevi **ICM-01a**; bu bakım endüstriyel acquisition veya fiziksel
kabul kapılarını açmaz. Yeni kapanışta kısa progress yenilenir, ayrıntı ayrı arşiv
kaydına yazılır ve `python scripts/check-context-docs.py` çalıştırılır.
