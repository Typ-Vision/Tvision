# COMM4-22 — Sentetik aday hand-eye solver

`HandEyeCandidateSolver::solveFixture` yalnız açık `EvidenceState::Synthetic`
girdisini tüketir. Fiziksel kapı18'de bloklu olduğu için normal main, wire ve
SAMPLE21 ledger'ından gerçek solve yolu bağlanmadı. Gerçek plate pose/PnP
kestirimi ve robot controller kabulü bu fixture adapterının sağladığı yetenek
olarak sunulmaz. UI değişmedi; validate/activate23 ve UI24 ayrı sorumluluktur.

## Denklem ve yöntem

S7'de `G_i=base_T_flange`, `C_i=camera_T_plate`:

- Eye-in-hand: `G_i X C_i=Y`; `A=inv(G_j)G_i`, `B=C_j inv(C_i)`;
  `AX=XB`. OpenCV girdisi `G_i,C_i`; çıktı `X=flange_T_camera`.
- Eye-to-hand: `X C_i=G_i Z`; `A=G_j inv(G_i)`, aynı `B`;
  `AX=XB`. OpenCV girdisi `inv(G_i),C_i`; çıktı `X=base_T_camera`.

Açık `CALIB_HAND_EYE_PARK` kullanılır. Sonuç manifest'inde OpenCV version,
mode, policy revision/digest, bütün dependency kimlikleri/digest ve immutable
solve örnekleri/sample-set SHA256 bulunur. İkinci mod birinci çıktının tersini
almaz. Fixed Y/Z translation ortalaması ve iteratif SO(3) geodesic mean ile
fit edilir;100 adımda yakınsamazsa ret. Holdout bu fit'e katılmaz.

## Sınırlar ve kayıt

Girdi değer kopyasıdır; en fazla256 örnek; her sample/frame UUID tekildir.
Frame yönü/revision ve base/flange dependency frame UUID eşleşmesi kontrol
edilir. COMM4-19 actual/TCP adapter'ın yerini almaz; opaque wire revision UUID
ile domain revision eşlemesini fiziksel yolda authoritative map sağlamalıdır.
Fixture golden'ında17 örneğin ilk12'si solve, son5'i holdout'tur. Solve holdout
sayısı istemez; holdout gerekliliğini ve bağımsız referansı23 değerlendirir.

Tek sayısal kalite otoritesi `FixturePolicyV1`'dir: plate RMS/max/ambiguity,
relative rotation spread ve axis separation, rotation s2/s1, translation
(R_A-I) s3/ratio ve flange-origin coverage denetlenir. Küçük rotation pair
sayısı raporlanır; sample sessiz çıkarılmaz. Hatalı plate sample UUID ve ret
nedeni sonuç raporundadır. Başarısız solve candidate kaydı üretmez.
Bu adapter gürültüsüz fixture oracle'ı uygular: closure max1e-6mm/1e-7rad.
Bu eşikler fiziksel tolerans veya gürültülü gerçek solve kabulü değildir.

Başarı `ProductionStore`'un yeni `hand_eye_candidate` namespace'ine immutable
provenance ile atomik append olur; ortak512MiB/10000-record sınırları yeni
namespace'i de sayar. Archive hatasında accepted=false. Active model portu
bulunmaz; önceki snapshot başarıda/başarısızlıkta aynı kalır. Session/wire
state bu adapter ile değiştirilmez; gerçek komut composition fiziksel gate ve
SAMPLE terminal provenance resolver'ı olmadan açılamaz.

## Kanıt ve tanımlanabilirlik

`tests/fixtures/hand-eye-candidate/` iki committed golden JSON ve yeniden
üretici içerir. Üretici Python stdlib4x4 aritmetik kullanır; OpenCV ve ürün
SpatialTransform kodunu çağırmaz. C++ testi known nonidentity X ve Y/Z'yi
kontrol eder; bağımsız domain quaternion composition ile S7 eşitliğini bütün
solve/holdout örneklerinde yeniden kurar. Aynıpose/tek eksen/az çeşitlilik,
yetersiz örnek, yanlış mode, ters/relabel edilmiş transform, yanlış TCP
okuması, quality sınırları, duplicate frame/sample, stale frame revision,
policy değişikliği ve archive hatası retleri sınanır.

Closure tek başına bütün ortak sistematik hataları tanımlayamaz. Örneğin bütün
G'lere aynı sağ TCP offset'i çarpmak X tarafından soğurulabilir. Fixture'daki
TCP negative tek örnekte yanlış offset kullanır; gerçek TCP/frame doğruluğunu
kanıtlamaz. COMM4-19 exact offset/revision ve23 bağımsız fiziksel referans
zorunluluğu korunur. Fiziksel solve/validation/activation **BLOCKED**.
