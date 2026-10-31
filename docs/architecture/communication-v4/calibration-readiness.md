# COMM4-18 — Robot/optik araştırması ve uygulama kararları

31 Ekim 2026 · Araştırma ve pure readiness kapısı tamam; gerçek solver/capture/
activation 19–23'te. Yetki sınırı [contract S6–S8](contract.md#s6--robot-ve-optik-araştırma-kapısı).
Fiziksel solve/validate/activate **BLOCKED**. Sentetik çalışma ilerleyebilir.

## 1. Mevcut ve eksik kanıt

| Girdi | Durum ve dayanak | Açılması için gereken |
| --- | --- | --- |
| Robot marka/model/controller/firmware | Eksik; seçilmiş robot yok. `freeze-comm4-02.md` yalnız generic adapter seçer. FANUC adlı eski doküman donanım kanıtı değildir. | Üretici/controller sürümü ve resmi pose protokolü |
| Actual 6DOF, sample kimliği, stationary/hold handshake | Eksik; W4 quaternion DTO bir actual producer değildir. | Gerçek actual kayıt, birim/frame ve capture terminaline kadar hold kanıtı |
| Base/flange/TCP ve offset | Matematik mevcut: `src/domain/SpatialTransform.hpp`; fiziksel tanımlar eksik. | Frame UUID/revision ve ölçülmüş `flange_T_tcp`; flange doğrudan gelirse offset gerekmez |
| Kamera | Progress: Opto COE-050-M-USB-050-IR-C / 02A2220000627, raw 2448×2048 Mono8. | Session/frame correlation ve gerçek drain kabulü; mevcut native engel korunur |
| Lens/projection | Tam model, perspektif/telecentrik türü ve montaj eksik. | Lens datasheet/seri, odak/diyafram, sabit montaj ve çalışma hacmi |
| Intrinsics/distortion | Bu araştırmaya sunulmuş geçerli K/distortion + bağımsız rapor yok; planar model bunun yerine geçmez. | Kamera/lens/raster revision'a bağlı intrinsic kayıt ve kontrol görüntüleri |
| Plaka | Progress'te 7×7 / 12 mm kullanıcı beyanı var; sertifika/flatness/origin/montaj kanıtı yok. | 7×7'nin iç köşe mi hücre mi olduğu, nokta listesi mm, yön/origin, ölçü belirsizliği |
| Pose çeşitliliği | Gerçek solve/holdout dataset yok. | İki kurulum için ayrı, farklı eksen/eğim/konum örnekleri |
| Bağımsız referans/tolerans | Translation/rotation fiziksel sınırı ve belirsizlik bütçesi eksik. | Onaylı mm/rad max/RMS/p95 sınırları, bağımsız metrologik referans |

Eksik alanlar `0`, identity veya varsayılan başarılı kanıtla doldurulmaz.
Erişilemeyen artifact yok sayılmaz; tabloda araştırmaya sunulmadığı belirtilir.

## 2. Dar kaynak auditi ve yeniden kullanım

| İncelenen gerçek kaynak | Bulgu / uygulama kararı |
| --- | --- |
| `src/application/CalibrationService.hpp` | Save/load/resolve `PlanarCalibration`; robot intrinsic/hand-eye deposu olarak kullanma. |
| `src/application/CalibrationWorkflow.cpp` | Uniform/manual scale ve raw planar homography; 2Dmm→6D dönüşümü yok. |
| `src/application/CalibrationWizardSession.cpp:106–146` | Pair→scale; plate→`MeasurementService::teachHomography`/`PlaneMappingService::fit`. Plaka tespiti 6D gözlemlenebilirlik kanıtı değil. |
| `src/application/CalibrationCapture.cpp` | Legacy Image File/Webcam acquisition; SAMPLE ledger/robot hold/fence yerine kullanma. Yeni oturum 20–21'in lease/acquisition yolunda kurulacak. |
| `src/application/CalibrationStudyStore.cpp` | Chessboard test study + raster SHA, 4 MiB QSaveFile; robot durable sample ledger değil. Formatı robot verisiyle sessiz genişletme. |
| `src/application/ActiveCalibrationService.hpp`, `ScopedActiveCalibration.hpp` | Planar kayıt/seçim, owner gate ve remembered authorization ayrımı mevcut; robot validated candidate/holdout/CAS kanıtını bu API'lerin varlığından çıkarma. |
| `src/domain/SpatialTransform.hpp` | Float64 mm, quaternion wxyz, exact frame/revision; `a.then(b)` matematikte `b*a`. Domain int revision ile v4 UUID revision eşlemesi19'da açık yapılmalı. |
| `CMakeLists.txt` ve yerel OpenCV başlıkları | OpenCV `calib` ve `geometry` zaten aranıyor. `opencv2/calib.hpp`: hand-eye/PARK; `opencv2/geometry/3d.hpp`: PnPGeneric/IPPE/RefineLM mevcut. Henüz link/runtime solver testi yapılmadı. |

Graph proje `D:/TVision`, generation `2026-10-31T06:51:29Z`, Tier 2.
Coverage9 dosyada kontrol edildi. Partial: CalibrationService.hpp18,
ActiveCalibrationService.hpp35/108, ScopedActiveCalibration.hpp57/74/75/80;
bu satırlar ve ilgili sınıflar doğrudan okundu. Graph calculatePlate trace'inde
ilgisiz `CommunicationV4Framer.reset` kenarı görüldü; çağrı zinciri kaynaktan
doğrulandı, o kenar kanıt sayılmadı. Bu audit tüm repo için yokluk iddiası değildir.

## 3. Optik ve plate pose kararı

İlk destek: doğrulanmış **perspektif pinhole + Brown-Conrady**. Kalibre K ve
distortion ile raw/root pixel noktalardan `solvePnPGeneric(..., SOLVEPNP_IPPE)`
adayları al; planar ve en az4 non-collinear eşleşme zorunlu. Özel dört köşeli
marker sırası kurulmadığı için IPPE_SQUARE seçme. Noktaların fiziksel mm ölçeği,
origin ve yönü açık olmalı; simetrik grid ters indeksini otomatik doğru sayma.
Her adayda finite R/t, proper rotation, tüm noktalar için pozitif derinlik,
reprojection ve ambiguity denetimi yap. Tek aday açıkça ayrılmıyorsa
`AMBIGUOUS_PLATE_POSE`; yalnız en düşük hatayı seçip devam etme.
RefineLM ancak seçimi doğrulanmış adaydan sonra; iki adayın raporu saklanır.
[OpenCV 5 projection API](https://docs.opencv.org/5.0/main_modules/d_projection.html).

Raw pixel + K/distortion veya bir kez undistort edilmiş pixel + yeni K/zero
distortion seçimini manifest'e yaz; ikisini karıştırma. Crop/resize/raw ilk satır
çıkarma root pixel koordinatına taşınmalı veya K açık dönüştürülüp yeni revision
olmalı. Focus/zoom/raster/mount değişirse eski kanıt geçersizdir.

Telecentrik/ortografik lensi pinhole gibi çözme: tek görüntüde optik eksen
translation'ı gözlenemez ve planar pose ikili olabilir. Karar: ilk solver yolu
**UNSUPPORTED_PROJECTION**; ek ölçüm/model ve ayrı araştırma olmadan 6D üretim
izni yok. Fisheye wire metadata'da bulunsa da ilk uygulamada **UNSUPPORTED**;
ileride açık fisheye ray/undistort yolu ve fixture gerekir.
[MVTec vector_to_pose](https://www.mvtec.com/doc/halcon/2511/en/vector_to_pose.html).
HALCON referans olarak incelendi; yeni SDK/lisans bağımlılığı seçilmedi.

## 4. İki kurulum için solver yönleri

Bu bölüm S7'den türetilen proje kararıdır. `G_i=base_T_flange[i]`,
`C_i=camera_T_plate[i]`; tüm hesap Float64 mm ve right-handed column vector.
OpenCV5 `calibrateHandEye`, açık `CALIB_HAND_EYE_PARK` başlangıç yöntemi seçilir;
varsayılan TSAI'ye güvenme. Yöntem/version ve bağımlılıklar manifest'e girer.
PARK'ın seçimi fiziksel üstünlük veya accuracy iddiası değildir.
[OpenCV hand-eye API](https://docs.opencv.org/5.0/main_modules/calib.html#calibratehandeye).

| Mod | Solver girdisi → çıktı | Bağımsız eşitlik / relatif A,B |
| --- | --- | --- |
| Eye-in-hand | `G_i, C_i` → `X=flange_T_camera` | `Y_i=G_i X C_i` sabit `base_T_plate`; `A=inv(G_j)G_i`, `B=C_j inv(C_i)`, `AX=XB` |
| Eye-to-hand | `inv(G_i), C_i` → `X=base_T_camera` | `Z_i=inv(G_i)X C_i` sabit `flange_T_plate`; `A=G_j inv(G_i)`, `B=C_j inv(C_i)`, `AX=XB` |

OpenCV parametre adları ikinci satırda anlamsal frame isimleri değildir;
wrapper bunu modla açık etiketler. İkinci sonuç birinci sonucun tersi değildir.
Sabit Y/Z solve kümesinden hesaplanıp candidate ile dondurulur; holdout'ta tekrar
fit edilmez. Translation aritmetik ortalama; rotation SO(3) geodesic mean,
yakınsama başarısızsa candidate FAILED. Robot-world/hand-eye SHAH alternatif API
mevcut ama ilk uygulamaya ikinci solver eklenmez.

Adapter19: generic quaternion doğrulanır; commanded pose reddedilir. TCP gelirse
`G=base_T_tcp * inv(flange_T_tcp)` ve exact offset revision gerekir. Marka
bilinmeden FANUC WPR, UR axis-angle veya Euler çıkarımı yapılmaz. Örneğin UR resmi
actual TCP belgesi metre/radyan axis-angle tanımlar; bu robot seçimi değildir.
[UR actual pose](https://www.universal-robots.com/articles/ur/programming/read-a-single-coordinate-or-axis-rotation/).

## 5. Sayısal politika ve bekleyen fiziksel kararlar

Aşağıdaki `fixture-policy-v1` sınırları **proje başlangıç politikasıdır**;
üretici garantisi veya fiziksel tolerans değildir. Policy immutable revision/digest
taşır. Değerler tek yerde tutulur; UI/wire'a sabit kopyalanmaz.

| Ölçüt | Fixture politikası / ret |
| --- | --- |
| Dataset | En az12 ayrı solve +5 ayrı holdout; sample/frame UUID kesişimi0. Holdout solve'a giremez. OpenCV matematiksel minimum3 pose'u ürün kabulü sayma. |
| Rotation çeşitliliği | Solve relatif hareketlerinde en az iki eksen: her açı≥5°, eksenler arası `acos(abs(dot))`≥15°. Tek eksen/yalnız translation ret. |
| Rotation conditioning | i<j relatif rotation log vektörlerini satır yap; norm<5° satırları ayıkla ve nedenini kaydet. SVD'de s1>0, s2/s1≥0.1; sonlu değilse ret. Tam rank3 zorunlu değil. |
| Translation conditioning | `A` için üst üste `(R_A-I)` matrisinin rank3 olması; s3≥1e-6 ve s3/s1≥1e-3. Tüm s değerleri/ratio rapora girer. |
| Translation kapsamı | Robot flange origin bulutunun maksimum çift mesafesi / plate diagonal≥0.25 (mm/mm); plate diagonal finite>0. Bu tarama tek başına accuracy/observability kanıtı değil. |
| Plate hata | Her örnekte raw pixel RMS≤0.5px, max≤1.5px; aday2 RMS−aday1 RMS≥0.2px; daha küçük fark ambiguity ret. Sınırlar fixture gürültüsüne göre ayrıca test edilir. |
| Gürültüsüz sentetik oracle | Known X ve sabit Y/Z için translation≤1e-6mm, geodesic rotation≤1e-7rad. Fixture koordinat büyüklükleri yaklaşık 10–1000mm. |
| Fiziksel hata | mm/rad max/RMS/p95, reprojection/ambiguity, condition ve çeşitlilik sınırlarının gerçek çalışma hacmiyle doğrulanması **PENDING**; bunlar gelmeden physical eligible=false. |

Eşik eşitliği kabul, hemen alt/üst komşuları test edilir. Rapor bütün örneklerin
ret nedenlerini saklar; sessiz outlier atma yok. Açı hatası
`acos(clamp((trace(R_ref^T R_est)-1)/2,-1,1))`; translation Euclidean norm.
RMS=`sqrt(sum(e²)/n)`, p95 sorted nearest-rank `ceil(.95*n)-1`, max ayrı.
Holdout residual'i aynı dondurulmuş Y/Z'ye karşı ölç; fiziksel doğrulama23'te
bağımsız referans/ölçü belirsizliği ile ayrıca yapılır. Başarılı kapanış residual'i
tek başına yanlış frame veya ortak sistematik hata olmadığını kanıtlamaz.

## 6. Uygulama devri

[COMM4-18 kartındaki kodlama tarifi](cards/COMM4-18.md#kodlama-tarifi--kalan-uygulama)
yalnız pure readiness DTO/policy kapısını uygular.19 adapter,20 session/lease,
21 sample ledger/capture,22 PnP/hand-eye,23 validation/activation sahibidir.
18'de aday solver API'sinin bulunması `softwareEvidence=1` yapmaz; W6a algorithm
ve evidence alanları gerçek uygulama/test olmadan yükseltilmez.

Son durum: software fixture planı **IMPLEMENTED**; physical solve/validation/
activation **BLOCKED**. Fiziksel tolerans ve controller kararı bekliyor; bu
araştırma ne kodun ne fiziksel kabulün tamamlandığıdır.
