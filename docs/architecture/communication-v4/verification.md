# COMM4 kabul ve kanıt matrisi

Bu dosya gelecek uygulama kabulünü tarif eder. Mevcut test başarısı yeni koşum
olarak aktarılmaz; bu plan için build veya fiziksel kabul yapıldığı iddia edilmez.

## V0 — Her kartın kapanışı

Kartın ID/davranış/dosya sahipliği önce kesinleştirilir. Yeni dosyalar adaydır;
COMM4-01 envanteri ve kart başı graph coverage doğrulaması olmadan var olduğu
varsayılmaz. Structural keşif codebase-memory skill ile D:/TVision projesinde,
list_projects/index_status, ilgili her dosya check_index_coverage; eksik satırlar
kaynak fallback ile tamamlanır. MCP hata boş sonuç değildir. İlgili sözleşme
değişirse bu dizindeki tek otorite bölüm güncellenir; teslim paragrafı çoğaltılmaz.

Kanıt `build/comm4/COMM4-xx/` altındadır: source revision+dirty diff identity,
configure seçenekleri/toolchain, komut/exit, geçen/başarısız/skipped ayrı sayılar,
tam log, fixture kimliği, UI görüntüleri ve açık fiziksel engeller. Eski sonuç
yalnız aynı kaynak/test/config/input eşleşmesi belgelenirse devralınır.
Kapanış `docs/archive/YYYY-MM-DD-comm4-xx.md`: kapsam, değişen dosyalar, kanıt,
sınırlama, sonraki kart ve `Uygulayıcı: Codex` veya `Claude Code`.
Progress güncel özet ve arşiv indeksi bağlanır; `python scripts/check-context-docs.py`
çalıştırılır. Tek `COMM4-xx: …` commit, push yok. Kullanıcının unrelated dirty
dosyaları stage edilmez. Sayaç varsa working-protocol görev farkı yöntemiyle
ölç; yoksa erişilemiyor yaz, ücret/kota oranı uydurma.

## V1 — Release kabul profilleri

Komutlar kök README toolchain/PATH ile uygulanır. CMake girdisi değişmediyse
yeniden configure gerekmez. Test regex/target COMM4-01 ve kart başında gerçek
CMake isimlerinden belirlenir; öneri test ismi uygulanmış test gibi sunulmaz.

| Profil | Zorunlu kapsam |
| --- | --- |
| D — yalnız belge/fixture tasarımı | Link, byte-size/aralık, diff ve bağlam boyutu; build gerekmiyor. Çalıştırılmayan Release ON/OFF açıkça belirtilir. |
| C — core uygulama | İlgili Release hedeflerini hem `build/reference-ui` ON hem `build/kmc9-core-off` OFF'da derle; ilgili CTest. Qt Widgets bağımlılığı core'a sızmaz. SDK OFF capability unavailable kabulü korunur. |
| U — UI | C + UI Release, ilgili CTest ve gerçek normal/küçük ekran görsel kontrolü; örnek1920×1080 ve1280×720, DPI kayıtlı. Offscreen tek başına native kabul değildir. |
| F — ortak sözleşme/faz | Hedefli testler sonra son kaynakta tam Release ON ve OFF CTest; skipped sebep/adet ayrı. UI etkisi varsa U da zorunlu. |

```powershell
cmake --build build/reference-ui -j 6
ctest --test-dir build/reference-ui --output-on-failure -R '<gerçek-ilgili-testler>'
cmake --build build/kmc9-core-off -j 6
ctest --test-dir build/kmc9-core-off --output-on-failure -R '<gerçek-ilgili-testler>'
```

Tam F kabulünde `-R` kaldırılır. Başarısız testi ve komşularını düzeltip yeniden
çalıştır; yeni gerekçe olmadan başarılı tam koşumu tekrar etme. Hedefli sonuç tam
suite değildir. Yalnız belge planı için bu komutlar şimdi çalıştırılmaz.

### COMM4-02 tasarım fixture kabulü

[Fixture README](../../../tests/fixtures/communication-v4/README.md) ve
[freeze raporu](freeze-comm4-02.md) frozen revision/manifest'i bağlar.
`python tests/fixtures/communication-v4/verify.py` committed bin/hex/PLC word
verisini independent width/hex arithmetic ve literal anchors ile kontrol eder;
generator çağırmaz. Negatif frame'lerin beklenen ret nedeni sınırlı fixture oracle'ı
ile denetlenir. Bu script CTest veya üretim codec/framer değildir. Stream split,
deadline, backpressure ve kontrol rezervi fixture beklentileridir; gerçek socket,
ledger/crash veya fiziksel davranış koşumu olarak raporlanamaz. Sonraki kartlar
aynı fixture manifest'ini üretim uygulamasına karşı çalıştırır. Metadata256 byte
şemaları W6a, enumlar W2/W7, digest exact slice'ları W7 otoritesidir.

## V2 — Davranış matrisi

| Alan | Somut kabul senaryosu |
| --- | --- |
| Başlangıç | Enabled geçerli profil açılışta listen; üç station STOPPED, PLC STATUS alır, TRIGGER NOT_AUTO; bind conflict açık alarm ve port sabit. |
| Recipe A/B | Station1 A cycle terminal+ACK sonrası B aktivasyon; socket/session değişmez. A activation tetik stale; station2/3 etkilenmez. A→B→A yeni activation. |
| Remote kontrol | SELECT_RECIPE→SET_AUTO→READY→TRIGGER; Remote Enable kapalı ret, yerel Stop yarışında geç hazırlık Auto açmaz. Pending ACK varken STOP/ABORT control rezervi işler, ayrı exact ACK; yeniden bağlantıda komut tekrarı yeni iş yaratmaz. |
| Holdout | SAMPLE role=holdout solve'a katılmaz; kamera ID ve origin command cevapta exact, candidate sonrası yeni holdout çekilebilir. |
| Multi-station | Aynı peer stream'inde S1/S2/S3 tetikleri, üç ayrı kamera ve kontrollü worker bariyeriyle overlap kanıtı; doğru recipe/frame/result/ACK her station'a gider. Thread sayısı tek başına eşzamanlılık kanıtı değildir. Dördüncü station eklemek kod sabiti istemez. |
| Ortak kamera | S1 lease tutarken S2 CAMERA_BUSY, S3 başka kamerada biter. İki multi-camera talep ters sırada deadlock yaratmaz, kısmi lease kalmaz. |
| Fairness | S1 yavaş ACK, S2/S3 sonuç ve ACK gecikme bütçesinde ilerler; socket scheduler starvation yapmaz. Global storage full açık bütün etki nedeni verir. |
| Publication | Variable ve direct output aynı typed slot değerini üretir; custom Number15/Flag31/Text3 ataması; missing required invalid, false/0/empty valid. Editor ağ/ledger etkisi yok. |
| Numeric/Text | Min/max signed32, overflow/scale unit mismatch,64byte Unicode sınırı, invalid UTF8 ve65byte reject; truncation/saturation yok. |
| Pose | Whole record frame/revision ve quaternion sırası korunur; Point2D→Pose reddedilir. Missing pose validity0; fake identity/PASS görünmez. |
| Idempotency | Aynı trigger/sample100 kez aynı sonucu verir, acquisition sayacı1; aynı key farklı body CONFLICT. Peer epoch+station isolation; request>2^53 ve MAX wrap rejection. |
| Kalıcılık | Intent, begin, terminal seal, pending, ACK commit sınırlarında crash-injection; restart terminal replay veya recovery, yeni acquisition0. Torn manifest/disk-full accepted iş kaybetmez. |
| ACK | Yanlış station/result/hash/epoch state değiştirmez; duplicate exact ACK idempotent; Stop ve reconnect sonrası geç exact ACK kalıcı kapanır. |
| Parser | Her byte konumunda split, dört coalesced request, partial/trickle deadline, malicious size, nonzero reserved, buffer/write queue limit; bounded memory ve UI thread responsivity. |
| Migration | Byte/hash baseline unchanged; backup/manifest; unsupported text/fan-out açık rapor; her staging/swap crash noktası eski veya yeni tutarlı config; pending ACK drain/rollback geçmişi korunur. |
| UI | Tek Communication; eski v2/v3 page yok, mevcut status/migration raporu var. Üç station, camera owner, Auto, delivery distinct. Küçük ekran clipping/focus ve klavye; fake ready/pose yok. |
| SAMPLE | Aynı mesaj actual pose+session/sample/camera; robot ACCEPTED'da durur, CAPTURED/FAILED terminalinden sonra capture beklemesi biter. Yeni frame token exact; preview/late callback reddedilir. |
| Calibration lease | Üretim S1 aynı kamerada blocked, S2 bağımsız sürer; ABORT gerçek drain sonrası lease bırakır, duplicate sample capture0 ek. Reconnect otomatik capture yok. |
| Solve geometry | İki kurulum için bilinen nonidentity ground truth, ters frame ve yanlış TCP negative fixture; degenerate/aynı eksen örnekleri reject. Yeni candidate eski active modeli değiştirmez. |
| Validate/Activate | Eğitim/holdout ayrımı ve independent reference; stale revision/dependency activate reject, crash atomik old/new; başarısızlıkta önceki active korunur. |

## V3 — Fiziksel kapılar ve raporlama

Robot marka/controller actual 6DOF, fiziksel TCP/frame tanımı, lens/intrinsics,
plate ölçüsü ve bağımsız reference/tolerans gelmeden fiziksel kabul **BLOCKED**.
Sentetik ground truth, prerecorded frame ve simulator yalnız yazılım kanıtıdır.
Kalibre düzlemsel mm modeli hand-eye kabulünün yerine geçmez. Native OECS
cancel/drain/frame correlation açıkken native acquisition izni açılmaz. Fixture
adapter normal ürüne sızmaz. Camera-manager readback, gerçek frame/mm/6D accuracy
kanıtı değildir. Kurulum, CUDA hız ve dağıtım ayrı kanıt olmadan tamam yazılmaz.

COMM4-26 raporu her capability için implemented / software-verified / physical-
blocked / unsupported durumlarını ayrı yazar. Robot motion hiçbir aşamada ürün
çıktısı veya test amacı değildir. Fiziksel kabul için robot sistemi kendi güvenli
hareketini yürütür; VisionFlow yalnız gözlem/pose alır ve sonuç döndürür.
