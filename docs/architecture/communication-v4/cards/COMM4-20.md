# COMM4-20 — Kalibrasyon session ve lease yaşam döngüsü

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **10,19** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S7,S8; wire W2,W3**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

BEGIN dependency snapshot ve ayrı calibration lease oluşturur. Üretim aynı kaynakta admission kapatılır; ABORT/drain/reconnect/restart state transitionları kalıcıdır.

Gerçek CalibrationWorkflow/StudyStore portları18de; yeni aday RobotCalibrationSessionService. CameraLeaseArbiter10 kullanılır, ikinci lease registry kurulmaz.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Capture eşleştirme21, solve22 veya robot hareketi.

## Somut davranış kabulü

1. S1 calibration kamerayı tutarken S2 ayrı kamerada üretir; production/calibration lease çakışması ret.
2. Duplicate BEGIN yeni lease yaratmaz; ABORT drain tamamlanmadan ownership bırakmaz.
3. Restart stale leasei yeniden capturea açmaz; explicit recovery, dependency revision mismatch reject.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Session state machine ve lease/crash transition testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-20/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-20: …`** commit; push yok.
Sonraki: **COMM4-21**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
