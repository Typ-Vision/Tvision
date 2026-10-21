# CCUX-05 — Tek görüntü, canlı görüntü ve durdurma

Durum: planlandı. Önkoşul: CCUX-03, CCUX-04. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C2; önceki kartların yeni API adları.
Kaynak: `src/app/CameraManager.hpp/.cpp`, `src/app/MainWindowCamera.cpp`, `tests/camera_manager_test.cpp`.

## Uygula

1. Kamera ekranına Tek görüntü / Canlı görüntü / Durdur komutlarını application session üzerinden bağla.
2. Durum ve gerçek FPS'yi göster; video dosyası kaydı vaat etme. Görüntü alınamıyorsa nedenini göster, eski görüntüyü yeni kare gibi bırakma.
3. Canlı sırasında uyumsuz ayar/kamera değişikliğinde önce Stop tamamlanmalı; tablo seçimiyle eski kare başka kameraya taşınmasın.
4. Sayfadan/projeden çıkış ve pencere kapanışında session yaşam döngüsünü tamamla. Kalibrasyona geçerken kamera lease'i bırakılmış olsun.
5. Kanıtla PGM kaydı yalnız geçerli görünen kareden; Stop/late frame kaydedilemesin. Deneysel nitelik korunur.

## Kabul

Tek görüntü, canlı aç/durdur/tekrar aç, canlıdan kalibrasyona geçiş, kopma, kapatırken pending callback, busy kamera. İki pencere boyutunda görüntü ve kontrol erişimi; UI donmadan ilerleme.

**CTest:** ui_camera_manager, camera_worker_lease_contract, industrial_oecs_experimental

## Sınır ve kapanış

Release + ilgili CTest + görsel. Performans garantisi veya gerçek video donanım kabulü bu kartın tamamlanma şartı değildir; yapılamayan fiziksel test raporlanır.
Kanıt: `build/ccux/ccux-05/`. AGENTS ve ortak sözleşme C6 ile kapat.

