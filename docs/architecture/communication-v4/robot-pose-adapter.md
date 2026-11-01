# COMM4-19 — Robot actual-pose adapter ve frame sözleşmesi

31 Ekim 2026. [Contract S7](contract.md#s7--koordinat-ve-örnek-alma-sözleşmesi)
ve [wire W3/W4](wire.md#w3--request-body-128511-384-byte) uygulama kaydıdır.
Kod: `src/domain/RobotActualPose.{hpp,cpp}`; test:
`tests/robot_actual_pose_adapter_test.cpp` (`robot_actual_pose_adapter_contract`).

## 1. Yön ve birim sözleşmesi

`T_A_B`, B koordinatlarını A'ya taşır; right-handed column vector. İç temsil
Float64 translation **mm** ve normalize quaternion **w,x,y,z**. Mevcut
`SpatialTransform`/`FrameRef` kullanılır; ikinci bir transform sistemi yoktur.
`base_T_flange` doğrudan gelirse tercih edilen yoldur. `base_T_tcp` gelirse:

```text
base_T_flange = base_T_tcp * inverse(flange_T_tcp)
```

Kodda `flangeTcp.inverse().then(baseTcp)`; `then()` bu T_B_C ile next T_A_B'yi
T_A_C yapar. `flange_T_tcp` source=tcp, target=flange'dır. Offset'in ters
yönde verilmesi, eksik offset veya exact olmayan offset revision reddedilir.

## 2. Domain revision ↔ v4 UUID eşlemesi

`FrameRevisionBinding` üç alan taşır: domain `FrameRef` (canonical frame UUID +
pozitif int revision), `wireFrameUuid` ve `wireRevisionUuid`. Kurallar:

- `wireFrameUuid` domain `frame.frameUuid` ile byte olarak eşit olmalıdır;
  yeniden adlandırma/başka UUID'ye taşıma reddedilir.
- `wireRevisionUuid` opaque catalogue kimliğidir; int revision'dan türetilmez,
  cast edilmez. W4'teki frameRevision UUID'si bu alandır.
- Lookup exact `FrameRef` eşitliğidir (owner kimlikleri ve revision dahil).
  Yalnız UUID eşleşmesi yeterli değildir; aynı UUID'nin yeni revision'ı yeni
  binding ister.
- Aynı domain frame veya aynı wire revision UUID'si iki kez kaydedilemez.

Adapter, contract frame'lerinin map'te birebir bulunmasını constructor'da
doğrular. Int revision ile UUID arasında örtük dönüşüm yoktur.

## 3. Adapter davranışı ve retler

`RobotActualPoseAdapter` saf domain kodudur: robot client, ağ, SDK, capture,
solver veya hareket komutu yoktur. `RobotPoseRejectCode` ile ayrılan başlıca
retler:

| Durum | Kod |
| --- | --- |
| `source=Commanded` veya kaynak belirsiz | `COMMANDED_POSE_REJECTED` / `POSE_SOURCE_REJECTED` |
| Frame map'te yok | `FRAME_BINDING_MISSING` |
| Frame UUID/owner farklı | `FRAME_IDENTITY_MISMATCH` |
| Aynı UUID, farklı revision | `FRAME_REVISION_MISMATCH` |
| TCP pose, offset yok | `TCP_OFFSET_REQUIRED` |
| Doğrudan flange pose'a offset | `TCP_OFFSET_UNEXPECTED` |
| Offset revision int/UUID farkı | `TCP_OFFSET_REVISION_MISMATCH` |
| Offset ters yön | `TCP_OFFSET_DIRECTION_INVALID` |
| Euler okuma, convention yok | `EULER_CONVENTION_REQUIRED` |
| Convention kimliği farkı | `EULER_CONVENTION_MISMATCH` |
| Sol elli / ters yön convention | `EULER_HANDEDNESS_UNSUPPORTED` / `EULER_DIRECTION_UNSUPPORTED` |
| Euler + non-identity quaternion | `AMBIGUOUS_ROTATION` |
| Gizli düzeltme (synthetic Z dahil) | `SYNTHETIC_CORRECTION_REJECTED` |
| Norm/NaN/Inf/translation hatası | `INVALID_POSE` |

Quaternion norm kuralı mevcut `Rotation3D` sözleşmesidir: `abs(n-1)<=1e-6`
kabul, sonlu olmayan ara değer ve `n=0` ret; kabul edilen q normalize edilir,
q/-q aynı canonical değere gelir. Adapter çıktıyı normalize/canonical tutar.

Commanded pose hiçbir koşulda actual sayılmaz. `correction` alanı doluysa,
identity veya saf Z rotasyonu olsa da reddedilir; sessiz frame düzeltmesi
sözleşme dışıdır. Bu, "synthetic Z" yasağının uygulama karşılığıdır.

## 4. Euler convention kimliği

Euler yalnız açık `EulerConvention` ile kabul edilir. Kimlik alanları:
`conventionId`, `axisOrder` (XYZ…ZYX), `unit` (rad/deg), `rotationType`
(intrinsic/extrinsic), `handedness`, `direction`, `revision`, `revisionUuid`.
Desteklenen: **right-handed**, **ControllerToInternal**. Sol elli veya ters yön
reddedilir. Marka adı, RX/RY/RZ veya WPR adı convention çıkarmaz; örneğin
`FANUC-WPR` kimliği kayıtlı değilse reddedilir.

Dönüşüm: a ilk eksen, b ikinci, c üçüncü. Intrinsic
`q1(a)*q2(b)*q3(c)`; extrinsic sabit eksenlerde `q3(c)*q2(b)*q1(a)`. Aynı
sayılar derece ve radyan convention'da farklı sonuç verir; birim açıktır.
Golden fixture intrinsic/extrinsic XYZ, bilinen X90 ve bağımsız matris
oracle'ı ile sınanır. Generic quaternion desteği controller uyumluluk iddiası
değildir; bu kart hiçbir marka/controller için uyumluluk doğrulamaz.

## 5. Kapsam dışı ve açık engeller

FK/IK, robot hareket komutu, donanım robot client'ı, sample capture/ledger,
hand-eye solver, UI, aktivasyon ve fiziksel kabul bu kartta yoktur. W4
PoseBlock byte codec'i de bu kartta uygulanmadı; frame/revision UUID eşlemesi
ve convention kimliği adapter sınırında tanımlıdır. Wire freeze değişmedi.

Gerçek robot marka/model/controller/firmware, actual 6DOF kaynağı, ölçülmüş
`flange_T_tcp`, frame UUID/revision kayıtları ve bağımsız fiziksel referans
yoktur. Sentetik adapter testleri fiziksel robot/controller kabulü değildir;
fiziksel solve/validate/activate **BLOCKED** kalır.
