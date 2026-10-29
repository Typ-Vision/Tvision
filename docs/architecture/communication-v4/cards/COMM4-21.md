# COMM4-21 — SAMPLE actual pose ve yeni kareyi exact eşleştir

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **13,20** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S7; wire W3–W5**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Tek SAMPLE mesajındaki actual pose/session/sample/camera verisini durable intent ile yeni acquisition tokenına bağla. Capture terminali frameUUID ve tamamlanma/başarısızlık bildirir; ACCEPTED release izni değildir.

Gerçek CalibrationCapture/StudyStore/acquisition portları18den; yeni aday CalibrationSampleCoordinator. Kalıcı request/sample ledger13 yeniden kullanılır.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Robot hareket komutu, eski preview frameini yeniden kullanma veya timestamp eşitliğiyle pairing kanıtı.

## Somut davranış kabulü

0. Solve/holdout role ayrı kalır; candidate sonrası holdout capture, cameraKey/origin command exact echo.
1. Duplicate SAMPLE100kez acquisition1; same ID farklı pose CONFLICT.
2. Robot fixture ACCEPTEDda bekler CAPTURED/FAILED terminalde capture beklemesini bitirir; late frame yanlış samplea gidemez.
3. Pose/camera/session mismatch reject; timeout+reconnect yeni çekim yapmaz, belgeli drain/recovery gerekir.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Capture pairing records, handshake ve fault/duplicate davranış kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-21/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-21: …`** commit; push yok.
Sonraki: **COMM4-22**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
