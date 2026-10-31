# COMM4-18 — Robot optik ve 6D gözlenebilirlik araştırma kapısı

Durum: **araştırma tamam (31 Ekim 2026); kodlama ve yazılım kabulü bekliyor**.
Araştırma: [calibration-readiness](../calibration-readiness.md).
Kalan uygulama modeli kullanıcı tarafından seçilecek; bu kart model başlatmaz.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **01,02** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S6–S8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Robot actual6DOF/frames/TCP ve kamera intrinsics/lens/distortion/plate6D yeteneklerini primary kaynak ve güncel kaynakla araştır. Mevcut2Dmm modelini ayır; her iki kurulum için solver seçimi ve sayısal uygunluk/validation ölçütlerini tanımla.

Araştırma sahipliği calibration-readiness.md ve contract S6–S8 bağlantısıdır.
Kullanıcının kodlama devri isteğiyle kalan uygulama yalnız aşağıdaki pure
uygunluk DTO/policy kapısıdır. Gerçek solver/capture/activation19–23'te kalır.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Gerçek donanım hareketi, yeni kamera SDK tahmini veya fiziksel veri eksikken accuracy kabulü.

## Somut davranış kabulü

1. Brand/controller/6DOF/lens/plate/tolerans her alan için mevcut/eksik+kanıt tablosu.
2. 2Dmm→6D dönüşümü olmadığı ve eye-hand/eye-base gözlenebilirlik/pose çeşitliliği koşulları açık.
3. Eksik fiziksel girdide DTO/fixture işleri ilerleyebilir; gerçek activation uygunluğu BLOCKED, sayısal threshold bilinmiyorsa açık bekleyen karar.

## Araştırma sonucu

- İlk aday: OpenCV5 pinhole/Brown-Conrady, planar IPPE aday ayrımı ve PARK
  hand-eye. İki modun açık girdi/çıktı yönleri araştırma §4'te donduruldu.
- Telecentrik/fisheye ilk yol unsupported; 2Dmm modeli 6D'ye yükseltilmez.
- Robot/controller/actual6DOF/TCP, lens/K/distortion, plaka belirsizliği,
  pose dataset ve fiziksel tolerans/ref eksik. Native correlation/drain açık.
- Fixture sayıları, rotation/translation conditioning ve reprojection eşikleri
  araştırma §5'te sayısal; fiziksel eşikler PENDING, activation BLOCKED.

## Kodlama tarifi — kalan uygulama

Bu bölüm uygulayıcı için yeterli dar görevdir. Önce başlangıç paketi ve araştırma
§1–6'yı oku; araştırmayı baştan yapma. Dosya adları **yeni adaydır**: graph ve
exact-path kontrolüyle çakışma olmadığını doğrula. Başka ajan açma.

1. **Dosyalar:** `src/domain/HandEyeReadiness.hpp/.cpp`,
   `src/application/HandEyeReadinessService.hpp/.cpp`,
   `tests/hand_eye_readiness_test.cpp`, `CMakeLists.txt`. Başka production
   dosyasına ihtiyaç çıkarsa gerekçeyi kaydet. Planar servisleri değiştirme.
2. **Domain DTO:** `HandEyeMode {EyeInHand, EyeToHand}`,
   `HandEyeProjection {PinholeBrown, Fisheye, Telecentric, Unknown}`;
   `EvidenceState {Missing, Synthetic, PhysicalVerified}`.
   `ReadinessIssue {code, field}` ve `HandEyeReadinessReport` oluştur.
   Raporda ayrı `fixtureEligible`, `physicalSolveEligible`,
   `physicalValidationEligible` ve issue listeleri olsun. **activationAllowed
   üretme**; aktivasyon yetkisi yalnız23'ün exact candidate/CAS kapısındadır.
3. **Girdi:** mode; kamera/lens/intrinsic/plate/adapter/frame/policy için nonempty
   UUID/revision/digest; projection ve raster; robot marka/controller/firmware,
   actual-pose kind, controller evidence; optional TCP offset revision;
   capture correlation/drain evidence; plate geometry/uncertainty; dataset
   evidence; independent reference ve tolerance approval. Yok değer optional
   veya Missing olur. Bool fiziksel kanıt yerine geçmez; kanıt kimliği ve
   revision zorunludur. Her evidence'ın synthetic/physical kökenini koru.
4. **Policy:** Araştırma §5'in tek `fixture-policy-v1` constructor'ı ve finite,
   pozitif/aralık denetimleri. Fiziksel toleranslar ayrı optional immutable
   policy'de mm/rad max/RMS/p95; fiziksel onay kimliği olmadan eligible olamaz.
   `fixture-policy-v1` fiziksel varsayılan olarak kullanılamaz. Bu kartta SVD,
   PnP veya solver çalıştırma; sayısal dataset metriklerini22 hesaplayacak.
5. **Pure evaluator:** `HandEyeReadinessService::evaluate(input)` değer döndürür;
   dosya/ağ/SDK/kamera açmaz, kayıt/Run/Auto değiştirmez. Şema/policy hatası,
   bilinmeyen mode/projection, boş kimlik, nonfinite değer açık issue üretir;
   identity/default başarı yok. Known unsupported projection her iki yolu
   engeller. Fixture bütün matematiksel girdileri açık synthetic seçebilir;
   robot marka ve fiziksel ref yokluğu fixture yolunu engellemez. Physical
   solve ayrıca gerçek actual adapter, montaj/frame/TCP, optical/plate evidence,
   capture correlation/drain ve onaylı solve kalite politikasını ister.
   Physical validation ayrıca bağımsız physical reference/tolerance ister.
   Bu ön denetimler dataset/solver başarısı veya VALIDATED statüsü değildir.
6. **Ret nedenleri:** En az `MISSING_DEPENDENCY`, `INVALID_POLICY`,
   `UNSUPPORTED_PROJECTION`, `UNVERIFIED_CONTROLLER`, `ACTUAL_POSE_REQUIRED`,
   `TCP_OFFSET_REQUIRED`, `UNVERIFIED_OPTICS`, `UNVERIFIED_PLATE`,
   `UNVERIFIED_CAPTURE`, `PHYSICAL_POLICY_PENDING`,
   `INDEPENDENT_REFERENCE_REQUIRED`, `SYNTHETIC_EVIDENCE_ONLY`.
   Deterministik code+field sırası ve duplicate olmaması; bütün eksikler tek
   değerlendirmede raporlanır. Her issue ilgili fixture/physical yolunda tutulur.
7. **Bağlama:** DTO/service `visionflow_projects` ve QtCore sınırında kalır;
   OpenCV/QtWidgets bağımlılığı ekleme. Gerçek CMake test adını
   `hand_eye_readiness` olarak kaydet (aday ad; henüz mevcut test değil).
   UI, wire metadata/algorithm1 capability ve normal main composition değişmez.
8. **Kapsamlı test:** İki mod × pinhole/fisheye/telecentric/unknown; tüm alanları
   dolu synthetic fixture (fixture=true, physical=false); boş girdi bütün
   eksikleri verir; her fiziksel bağımlılığı tek tek kaldır; TCP/flange ayrımı,
   commanded pose reddi; revision/digest yok/bozuk; invalid enum, NaN/Inf,
   negatif/sıfır policy, sayısal sınırlar; synthetic evidence'ın physical yola
   sızmaması; report determinism ve input mutation olmaması. Bütün physical
   preflight girdileri explicit verified olduğunda positive preflight fixture
   da sınanır; bu fixture fiziksel kabul diye raporlanmaz. Snapshot dependency
   değişimi sonucu yeniden engeller. Restart/IO/CAS testlerini18'e uydurma;
   bu davranışların gerçek sahipleri20–23'tür.

19'da adapter,22'de araştırma §3–5 solver/conditioning/oracle matrisi,23'te
holdout/physical reference/atomik aktivasyon uygulanacak. Bu kartı bitirmek için
onların production kodunu erkenden yazma. Kodlama kapanışından sonra sıradaki19.

## Release ve teslim kabulü

Araştırma teslimi profil **D**, kalan kodlama profil **C**;
[verification V0/V1](../verification.md) uygulanır. Araştırma için belge/link/diff
ve bağlam kontrolü; Release/CTest koşulmaz. Kodlama kapanışında README toolchain
ile her iki Release build, yeni test + ilgili spatial/kalibrasyon regresyonları,
ardından son kaynakta **tam Release ON ve OFF CTest** zorunludur. `-R` koşumu
tam kabul sayılmaz. Temiz koşuma özel INI/fixture kullan; log/exit/sayı/skipped
ve kaynak/config/input kimliği kaydet. Fiziksel kapılar açık raporlanır.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: İki kurulum için algoritma/adapter karar kaydı ve fiziksel blocker listesi.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-18/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-18: …`** commit; push yok.
Bu araştırma teslimi kod uygulandı veya kartın yazılım kabulü kapandı demek
değildir. Sonraki işlem **bu kartın kalan kodlamasıdır**;19'u başlatma.
