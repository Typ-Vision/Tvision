# ICM-BAS-02a — Basler ayar/readback ve rollback

Durum: Planlandı. Bağlı: [ICM-BAS-01](ICM-BAS-01.md)/G0.
Sonraki: [ICM-BAS-02b](ICM-BAS-02b.md).

## Oku

- [Ortak kabul](README.md), önceki kartın kapanışı ve gerçek node envanteri.
- [CCUX sözleşmesi](../camera-setup/contract.md) C1/C2/C3/C4;
  [ana plan](../camera-backend-activation-plan.md) §5/6.

## Davranış ve sahiplik

Basler hardware binding/session, application settings adaptörü,
CameraSettingsService/MeasurementWorkspace ve gerekli CameraProviderFactory
ürün composition noktaları; CameraManager ve settings/UI testleri.

1. Ürün ayar servisi backend'i exact kimlik/capability ile seçer; Basler pylon C
   ve OECS ayar yolları birlikte çalışır. Test enjeksiyonu ürüne açılmaz.
2. Pending kamera kaydında setup/profile geometrisi tamam olmadan gerçek ayar
   okunabilir. Width/Height/offset/ROI/binning/decimation/reverse ve değerlerin
   reported/not-applicable/unknown kanıtı korunur. Lens/düzlem üretilmez.
3. Exposure/gain/auto birimi ve yazılabilirliği canlı node'lardan gelir;
   GainRaw doğrudan dB sayılmaz. Belgelenmemiş dönüşüm/yeni alias tahmini yok.
4. Scalar apply: preflight → desteklenen yazma → gerçek readback → atomik proje
   kaydı. Stale/disk/SDK/readback hatasında rollback/uyumsuzluk görünürdür.
   Auto/trigger geçici değişirse önceki durum doğrulanarak geri getirilir.
5. Yalnız okuma no-op; isim/no-op hash kuralları korunur. Değişiklik ilgili kaynak
   cache/izinlerini yeniler; başka kamera/model etkilenmez. Settings yolu grab yapamaz.

## Kabul

Regex: `^(camera_settings|oecs_camera_settings|ui_camera_manager|industrial_provider_contract|camera_setup_acceptance)$`; yeni Basler settings fixture testini ekle.
Gerçek/yanlış kimlik, eksik/read-only node, aralık/NaN/birim, auto, aynı değer,
pending, gerçek geometri, stale/CAS/disk hatası, partial write ve rollback failure
negatifleri. Fiziksel read/no-op save ve aralıkta scalar değiştir/geri yükle/readback;
önceki değerler kanıta alınır, rollback belirsizse kabul kapanmaz.
Ortak tam ON/OFF + UI iki boyut. Kanıt: `build/icm/bas-02a/`.

Kapsam dışı: ROI/trigger editörü genişletme, Start/Grab/production capability,
kalibrasyon aktivasyonu ve G1. Başarılı ayar kaydı görüntü edinimi değildir.
