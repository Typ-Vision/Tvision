# CCUX-09 — Kamera merkezli kalibrasyon ekranı ve sihirbazlar

Durum: planlandı. Önkoşul: CCUX-05, CCUX-08. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C3; [sistem özeti](../calibration-overview.md).
Kaynak: `src/app/CalibrationOverview.hpp/.cpp`, `src/app/CalibrationWizard.hpp/.cpp`, `src/app/MainWindowCalibration.cpp`; ilgili UI testleri.

## Uygula

1. Aynı registry'den kamera listesi; seçili kameranın aktif durum/yöntem/tarih/değerlerini alt bölümde göster. “Kalibrasyon yok” ile “kayıt var/bağlantı yok” ayrımını koru.
2. Kalibre Et seçiminde Bilinen ölçü ve Kalibrasyon plakası; robot mevcutsa açıklamalı disabled. Varsayılan kamera/setup/profile sihirbaza otomatik ve immutable geçsin.
3. Görüntü → kenar/plaka → fiziksel mesafe/aralık → hesapla/kontrol → Kaydet ve kullan akışını mevcut matematik üzerinden bağla. Adımda yalnız ilgili alanlar görünsün.
4. Tek komutun kısmi başarı/ret durumunu anlaşılır göster; başarısız etkinleştirmede eski aktif model kaybolmasın. Deneysel etiket ana sonuçta görünür.
5. Geçmiş/dosya denemesi/legacy ilişkilendirme ve optik değişim bildirimi Ayrıntılar'da erişilebilir; normal kullanıcıya profil/kurulum seçtirme. Kamera değişimi veya pencere kapanışı eski işleri iptal etsin.

## Kabul

Reçetesiz 0/1/5/10 kamera; iki yöntem; kaydet/iptal/tekrar aç; A sonucu B altında görünmez; offline/experimental/no calibration; önceki etkin model korunması. 1400×900 ve 850×600 ekranlar ve klavye erişimi.

**CTest:** ui_calibration_overview, ui_independent_calibration_wizard, ui_scoped_calibration_activation, ui_industrial_calibration_activation, calibration_plate_contract

## Sınır ve kapanış

Release + CTest + görsel. Yeni robot/lens/ChArUco yok. Mevcut kullanıcı rehberi ve calibration-overview yalnız uygulanmış yeni akışla güncellenir.
Kanıt: `build/ccux/ccux-09/`. AGENTS ve ortak sözleşme C6 ile kapat.

