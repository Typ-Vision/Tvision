# COMM4-24 — Kalibrasyon operatör UI akışı

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **16,23** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S5–S8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

İki mount modunu, frame/TCP/lens readiness, lease sahibini, sample pairing/reject nedenini, candidate/validate/activate ayrımını göster. BEGIN/SAMPLE/SOLVE/VALIDATE/ACTIVATE/ABORT application komutlarına bağla.

Gerçek CalibrationWizard/Overview.* ve Communication workspace01/18den; yeni aday robot calibration view model. UI solve/SDK çağırmaz.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Robot motion ekranı, sahte pose üretimi veya fiziksel gatei UI bypass etme.

## Somut davranış kabulü

1. Actual pose yokken capture kapalı/açık neden; candidate oluşunca active değişmez.
2. ABORT gerçek drain progressi, reconnect recovery ve physical BLOCKED görünür; PASS/score uydurulmaz.
3. Normal/küçük ekran görsel, klavye/disabled reason, station2 üretim durumu erişilebilir.

## Release ve teslim kabulü

Profil **U**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI Release ve normal/küçük ekranda gerçek görsel kontrol zorunlu; offscreen sonucu native/DPI kabulü değildir.

Teslim: Calibration UI ve iki viewport/transition görsel kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-24/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-24: …`** commit; push yok.
Sonraki: **COMM4-25**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
