# CAM-OECS-01 — OECS SDK'lı build ve doğru px/readback sunumu

Durum: Uygulandı; hedefli ON/OFF geçti, tam suite koşulmadı.
[Kapanış](../../archive/2026-11-04-cam-oecs-01.md). Bağlı: [CAM-SDK-01](CAM-SDK-01.md).
Sonraki: [ICM-BAS-01](ICM-BAS-01.md).

## Oku

- [Ortak kabul](README.md), [kamera analizi](../../archive/2026-11-04-cam-analysis-01.md).
- [Ana plan](../camera-backend-activation-plan.md) §1/6;
  [CCUX sözleşmesi](../camera-setup/contract.md) C1/C2/C3.

## Davranış ve sahiplik

CMake/yerel SDK yapılandırması, scripts/run-local.ps1 ve ilgili kullanım rehberi;
CameraManager, OecsCameraSettings, CameraSettingsService ve ilgili testler.

1. Kurulu OECS header'ıyla optional bağlayıcıyı doğru derle; günlük başlatıcının
   SDK'lı gerçek EXE'yi kullandığını kaydet. Hardcoded kullanıcı yolu repoya taşıma.
2. Ayar erişimi gerçek neden ve backend desteğiyle açılır; pending optik kurulum
   salt ayar okumasını engellemez. Uygula ancak geçerli gerçek readback ile açılır.
3. Genişlik/yükseklik px olarak açıklanır; aktif raster, sensör maksimumu ve
   istenen değer farklıysa görünür. Binning 1×1 ile çözünürlük karıştırılmaz.
4. Yeni readback varken eski tamamlanmış 1×1 kayıt sessizce doğru gösterilmez.
   Gerçek/kayıtlı farkı sunulur ve düzeltme açık kayıt komutundan geçer; sadece
   okumak proje değiştirmez. Live uyum bilinmiyorsa fiziksel doğruluk iddia edilmez.
5. Yanlış geometriye kullanıcıya anlaşılır ret/düzeltme sağla; domain'i yalnız
   OECS sabitine daraltma. Lens/düzlem uydurma; geometri revizyonu/kalibrasyon
   invalidation ve no-op kuralları korunur. Form kaydı kamera ayarı uygulaması diye sunulmaz.

## Kabul

Regex: `^(camera_settings|oecs_camera_settings|ui_camera_manager|industrial_configuration|camera_setup_acceptance|industrial_oecs_experimental)$`.
Pending/tamamlanmış, readback öncesi/sonrası, kayıtlı 1×1/gerçek 2448×2048,
okumada disk no-op, açık düzeltme, iptal/stale/disk hatası ve eski model izni
negatiflerini genişlet. Fiziksel exact OECS readback ve mevcut scalar değerlerle
no-op apply/save yap; hata/rollback durumunu raporla. Scalar değiştirme gerekirse
önceki değerleri kaydet, geri yükle ve yeniden oku.
Ortak tam ON/OFF + iki boyut UI kabulü. Kanıt: `build/camera/cam-oecs-01/`.

Kapsam dışı: OECS native G1/pending-command çözümü, 2448×2047 canlı crop,
lens solver, fiziksel mm ve Basler edinimi. Önceki fiziksel test yeni koşum değildir.
