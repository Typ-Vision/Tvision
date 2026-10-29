# COMM4-10 — Kamera sahipliği ve acquisition admission

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **09** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S2,S7**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Station/preview/calibration aynı cameraUUID arbiterını kullanır. Tek kamera exclusive; multi-camera reserve atomik ve sıralı; release gerçek drain sonrası generation fence ile gerçekleşir.

Gerçek acquisition session/provider portları01 haritasından; yeni aday CameraLeaseArbiter. SDK implementation değişikliği yalnız mevcut port gerektirirse dar kapsamda.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Native korelasyon engelini workaroundla kapatma veya gizli bekleme kuyruğu.

## Somut davranış kabulü

1. S1 kameraA lease tutar, S2 A CAMERA_BUSY, S3 B tamamlanır.
2. Ters multi-camera talepleri deadlock/kısmi lease bırakmaz; acquisition başarısızlığı rezervasyonu boşaltır.
3. Stop/Abort sonrası geç kare yeni cycle/samplea atanmaz; native unavailable fixture tarafından açılamaz.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Lease portu, sahiplik diagnostics ve kaynak yarış/cancel testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-10/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-10: …`** commit; push yok.
Sonraki: **COMM4-11**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
