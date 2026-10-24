# ICM-04d — pending command cancellation belge incelemesi

Tarih: 18 Ekim 2026. Uygulayıcı: Codex.
Durum: statik inceleme tamam; ICM-04d açık, native Unavailable.

## Kapsam ve bulgu

[8/8 gecikmiş eski kare karşı örneği](2026-10-18-icm-04d-pending.md)
sonrasında aynı fiziksel deney tekrarlanmadı. Önceden alınmış exact cihaz XML'i
ve kurulu OECS Development/Includes altındaki sekiz header incelendi.
18 Command içinde AcquisitionAbort yok. AcquisitionStop açıklaması mevcut karenin
sonunda durmayı söylüyor; API dönüşünün pending command completion garantisi değil.
DeviceReset var, ancak yalnız başlangıç durumu/varsayılan ayarlara dönüş anlatılıyor;
pending komut iptali, transport drain, tamamlanma sinyali veya counter epoch garantisi
yok. DeviceReset çalıştırılmadı; kamera ayarları değiştirilmedi.

Header API adlarında abort/cancel/drain bulunmadı. StopGrabbing açıklaması yalnız
stop; ClearImageBuffer açıklaması edinim sürerken buffer temizliği. Bu sınırlı
statik bulgu, firmware'in hiçbir iptal yolu olmadığı anlamına gelmez. SDK CHM'i
hh.exe ile iki kez decompile edilmeye çalışıldı, çıktı oluşmadı; içeriği okunamadı.

Yeni üretici alan adı araması exact modele ait
[veri sayfasını](https://www.opto-e.com/discontinued/COE-U%20series/Datasheets/COE-050-M-USB-050-IR-C_EN_v02.pdf)
ve [2019 kılavuzunu](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual-20190412.pdf)
buldu; pending cancellation/completion için kullanılabilir garanti bulunmadı.
Veri sayfasındaki software trigger veya Chunk data desteği exact firmware bariyer
kanıtı değildir. Başka modelin firmware'i bu kameraya uygulanamaz.

## Dosyalar ve doğrulama

- `industrial-camera-vendor-questions.md`: engelleyici tek soru en başa alındı;
  DeviceReset iptal/tamamlanma/epoch semantiği soruldu. Taslak gönderilmedi.
- Progress ve arşiv dizini güncellendi. Ürün, test kaynakları, UI ve local INI değişmedi.
- `build/icm/04d-cancel-audit/audit.py` ve `audit.json`: yeni statik koşum;
  XML SHA256, sekiz header yolu/hash'i, 18 Command ve lifecycle API envanteri.
  XML hash, AcquisitionAbort yokluğu, DeviceReset/AcquisitionStop varlığı ve
  abort/cancel/drain API adı yokluğu assertions geçti. Yeni command/kare: 0/0.
- `build/icm/04d-cancel-audit/context-check.json`: bağlam kontrolü.
  Build/CTest/görsel/donanım testi koşulmadı; önceki başarılar yeni koşum değildir.

## Devam noktası

Üreticiden exact firmware için belgeli cancel veya completion/drain/reset
prosedürü alınmalı. Yanıt exact API/node sırası, completion koşulu, hata/recovery
ve yeni epoch koşulunu açıklamalı. Sonra o prosedüre yönelik opt-in karşı deney
yazılıp eski gecikmiş command, timeout ve restart sınırları sınanabilir.
Yanıt veya eşdeğer resmi belge gelmeden aynı sessizlik matrisini tekrarlamak
engeli kapatmaz; 04e başlamaz. Native admission, ilk satır ürün geometri/kalibrasyon
entegrasyonu ve mevcut fiziksel kabul sınırları açık kalır.

Bu adım yalnız incelemeyi kapatır, ICM-04d'yi tamamlamaz. Yerel tek görev commit'i;
push yok. Üreticiye mesaj gönderme yetkisi verilmedi; gönderim yapılmadı.
