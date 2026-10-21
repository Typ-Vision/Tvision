# CCUX-15 — Gerçek kamera ve bağımsız mm doğrulaması

Durum: planlandı. Önkoşul: CCUX-14 + erişilebilir kamera ve bilinen fiziksel referans. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C5; [kamera tanı rehberi](../../windows-camera-diagnostics.md); [MUI kabul matrisi](../measurement-usability-acceptance.md) gerçek raster; [kalibrasyon kabulü](../calibration-acceptance.md).
Kanıt kaynağı: önceki EXP-01d/02/03b yalnız karşılaştırma gerektiğinde; yeni koşum diye sunulmaz.

## Uygula

1. Exact kamera/lens/çalışma düzlemi, ışık/odak, görüntü geometrisi ve bağımsız referans ölçüsünü kaydet. Yeni ölçüm öğretiminde kullanılan aynı mesafeyi tek doğrulama referansı yapma.
2. Kamera ekle/tek/canlı/Stop/yeniden aç ve kontrollü USB kopma akışını test et. Canlı testi önerilen başlangıç 60 saniye; süre/FPS/kare sayısı ve hata raporlanır, hedef FPS iddiası yok.
3. Bilinen ölçü ve plaka akışında gerçek aday/deneysel etkinlik → Editor mm → Runtime mm → Logs/CSV'yi doğrula. Lens/plaka bilinmiyorsa fiziksel doğruluk maddesi açık kalsın.
4. Merkez/kenar ve farklı yönlerde bağımsız referansla hata/tekrarlanabilirliği ölç. Kabul toleransı iş ihtiyacından gelir; kullanıcı toleransı bilinmiyorsa yalnız ölçüm raporu, PASS yok.
5. Restart ve exposure/gain güncellemesinde kayıt/uyum, yanlış geometri/kamera negatiflerini denetle. Çok fiziksel kamera yoksa 5 kamera donanım kabulü açık, UI fixture sonucu ayrı.
6. Kullanıcı devreye alma akışını deneyerek menü/işlem kabulünü verir; gerekli düzeltmeleri ilgili karta bağla.

## Kabul

Yeni tarihli raw görüntü/sidecar/reçete/model/CSV ve test raporu birbirine bağlı. Deneysel mm doğruluk sonucu native tetik güveni veya production kabulü değildir; özgün MUI-04 problem görüntüsü yoksa o madde açık kalır.

**CTest:** İlgili düzeltme varsa onun suite'i; sırf fiziksel deneme için önceki yazılım suite'lerini yeniden koşma.

## Sınır ve kapanış

Donanım/ref/tolerans eksikse kartı fiziksel kabul bekliyor bırak; tamamlandı sayma.
Kurulum/dağıtım, CUDA, robot/PLC ve native G1/G3/G4 topluca kapanmaz.
Kanıt: `build/ccux/ccux-15/`. AGENTS ve ortak sözleşme C6 ile kapat.

