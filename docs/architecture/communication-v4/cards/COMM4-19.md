# COMM4-19 — Robot pose adapter ve frame sözleşmesi

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **03,18** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S7; wire W3,W4**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

base_T_flange iç sözleşmesini kur; açık base_T_tcp+flange_T_tcp dönüşümü, source/target revision, quaternion ve adapter convention kimliğini doğrula.

Gerçek SpatialTransform/FrameRef/communication value DTOları01/18den; yeni aday RobotActualPoseAdapter. Donanım robot clientı varsayılmaz.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

FK/IK, commanded poseun actual sayılması veya marka varsayımlı WPR yorumu.

## Somut davranış kabulü

1. Bilinen noncommuting rotation/translation golden vector ve inverse negative fixture.
2. TCP offset yok/yanlış revision reject; quaternion norm/NaN/zero norm reject.
3. Euler adapter explicit order/units/intrinsic-extrinsic testli; yanlış yön ve synthetic Z reddedilir.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Pose adapter/DTO, kaynak convention kaydı ve frame transformation golden tests.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-19/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-19: …`** commit; push yok.
Sonraki: **COMM4-20**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
