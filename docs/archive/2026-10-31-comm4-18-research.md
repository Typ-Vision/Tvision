# COMM4-18 — Araştırma ve kodlama devri

Uygulayıcı: Codex

31 Ekim 2026. Kullanıcı yalnız araştırma ve uygulanabilir kodlama tarifi istedi;
production/test kodu yazılmadı. COMM4-18 araştırma bölümü tamam, readiness
uygulaması ve yazılım kabulü açık.19 başlatılmadı. Başlangıç HEAD `323558c`;
çalışma ağacı temizdi; COMM4-18 checkpoint/aktif build/test süreci bulunmadı.

## Teslim ve kapsam

- [Araştırma](../architecture/communication-v4/calibration-readiness.md): robot,
  actual/frame/TCP, kamera/lens/K/distortion, plate ve tolerans kanıt tablosu;
  planar servislerin dar kaynak auditi; primary OpenCV5/MVTec/UR kaynakları.
- [Kart](../architecture/communication-v4/cards/COMM4-18.md): kalan pure readiness
  DTO/policy/service dosya sahipliği, deterministik retler, fixture/physical
  ayrımı, kapsamlı test matrisi; gerçek solver/capture/activate19–23'te kalır.
- İki mod için PARK girdi/çıktı ve relatif A/B yönleri; pinhole/IPPE ambiguity;
  telecentrik/fisheye ilk yolda unsupported. Fixture sayısal eşikleri proje
  başlangıç politikasıdır, gerçek ölçümle doğrulanmış fiziksel tolerans değildir.
- Kullanıcı 30 Ekim kısa test tercihini geri aldı. Working-protocol DOC-WORK-03
  ve V1 C/U/F artık hedefli testlerden sonra tam Release ON/OFF regresyon ister.
  Önceki16/17 hedefli başarıları tam kabul olarak yeniden etiketlenmedi.

Değişen dosyalar: kart, yeni calibration-readiness.md, contract S6 bağlantısı,
COMM4 README/source-map/verification, working-protocol, progress ve arşiv indeksi,
bu yeni kapanış kaydı. Kök README kullanım/build davranışı değişmedi.

## Doğrulama ve kanıt

Profil D. `build/comm4/COMM4-18/`: context-check.log, diff-check.log,
document-check.log, research-evidence.json. `python scripts/check-context-docs.py`,
`git diff --check` ve değişen belgelerin yerel link/anchor kontrolü uygulanır;
sonuç/exit ve hash envanteri yerel kanıt kaydındadır. Web kaynakları araştırma
belgesinde ilgili iddianın yanında bağlı; yerel OpenCV başlıkları da okundu.
Release ON/OFF build, CTest, görsel ve fiziksel koşum **yapılmadı**: bu teslim
yalnız belge araştırmasıdır; kapsamlı test kuralı kalan kodlama için geçerlidir.

Graph list_projects/index_status, search_graph, trace/snippet ve9 dosya coverage
denetimi yapıldı; partial satırlar kaynaktan tamamlandı. İlgisiz trace kenarı
source fallback ile elendi; ayrıntı araştırma §2'de. Bir graph sonucunu boş veya
eksiksiz kaynak envanteri sayma. Token/kullanım sayaçları erişilemiyor.

## Açık işler / devir

Robot/controller/actual6DOF, gerçek TCP/frame, lens/intrinsic, sertifikalı plaka,
bağımsız referans/tolerans ve native correlation/drain açık; fiziksel
solve/validation/activation BLOCKED. K/solver API varlığı softwareEvidence=1
veya production permission değildir. Fixture normal ürüne sızmaz.

Sonraki uygulayıcı COMM4-18 kartının **Kodlama tarifi — kalan uygulama** bölümünü
uygular. Araştırmayı yeniden başlatmaz; iki Release build, hedefli yeni/komşu
testler ve tam ON/OFF CTest ile yeni kaynakta kabulü kapatır. Sonra19'a geçilir.
Bu araştırma `COMM4-18: …` ID'li tek commit; push yok. Sonraki kodlama kendi
oturumunda yeni tarihli/devir kaydıyla kapanır; bu tarihsel kayıt değiştirilmez.
