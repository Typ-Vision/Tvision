# CCUX-03 — Setup Kameralar ekranını sadeleştirme

Durum: planlandı. Önkoşul: CCUX-02. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C1–C2; [UI gereksinimleri](../../requirements/02_machine_vision_ui_gereksinimleri.md) kamera/Setup bölümleri.
Kaynak: `src/app/CameraManager.hpp/.cpp`, `src/app/MainWindowCamera.cpp`, `tests/camera_manager_test.cpp`.

## Uygula

1. Tek kayıt listesi ve seçili kamera düzenlemesi kur: kısa ID, açıklama/ad, model/seri, bağlantı durumu, exposure/gain.
2. Tara → seç → Ekle akışını CCUX-01/02 komutlarına bağla. Varsayılan setup/profile otomatik; normal ekranda “Kurulum / acquisition profili” ve taslak düğmesi kalksın.
3. Producer/rota/timeout/geometri gibi bilgiler Ayrıntılar içinde kalsın. Legacy belirsizliği yalnız gerektiğinde göster; gizli zorunlu alan bırakma.
4. Aynı kaydedilmiş proje/registry'yi Kalibrasyon ile paylaş. Kamera ekleme için reçete isteme; proje yoksa tek proje oluştur/aç yönlendirmesi.
5. Deneysel OECS'nin oturum onayı ve etiketi görünür kalsın. Mevcut tek önizleme çalışsın; canlı düğmesini CCUX-05'e kadar çalışan özellik gibi sunma.

## Kabul

0/1/5/10 kamera, uzun ID/ad, offline/ambiguous/SDK missing, tekrar tarama seçimi, duplicate kayıt, hatalı ayar ve busy durumu. 1400×900 ve 850×600 ekranlarda ana işlemler erişilebilir. UI testleri iç alan sayısı yerine kullanıcı akışını doğrulasın.

**CTest:** ui_camera_manager, camera_transport, industrial_configuration

## Sınır ve kapanış

Algoritma/SDK UI'ya taşınmaz. İlgili Release build ve iki boyutta görsel zorunlu. Kullanım değiştiği ölçüde kamera rehberini güncelle.
Kanıt: `build/ccux/ccux-03/`. AGENTS ve ortak sözleşme C6 ile kapat.

