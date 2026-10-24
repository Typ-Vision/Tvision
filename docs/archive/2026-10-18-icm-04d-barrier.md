# ICM-04d — Clear/Stop/Start bariyer karşı deneyi

Tarih: 18 Ekim 2026. Uygulayıcı: Codex.
Durum: **tanı alt adımı tamamlandı; ICM-04d/native admission açık**.

## Kapsam ve sonuç

Mevcut devam noktasındaki epoch/bariyer adayı sınandı. Exact cihaz
Opto Engineering Srl / COE-050-M-USB-050-IR-C / 02A2220000627.
Üç tur × dört senaryo = 12 senaryo, 36 software command, 27 alınan kare.
Her senaryoda kontrol command/kare, tüketilmeyen command, fence sonrası yeni
command olmadan gözlem ve son olarak ayrı command/kare vardır.

- Clear hemen: **3/3 eski command karesi** fence sonrası geldi. Kontrole göre
  frame/trigger sayaçları +1; yeni command henüz gönderilmemişti.
- Clear 150 ms sonra: 0/3; buffer temizliği gözlendi, exposure bariyeri kanıtlanmadı.
- Clear→Stop→Start hemen veya 150 ms sonra: 0/6; sonraki command'ın sayaçları
  altı kez 1/1 ve frameNumber=0 oldu. Aynı açık handle üzerinde stream restart
  sayaç dönemini değiştiriyor; yalnız reopen generation yeterli varsayılamaz.
- Son binary'nin kare gelmeyen dokuz gözlem penceresi 362–379 ms sürdü.
  Üç sızıntı yaklaşık 33 ms'de teslim edildi. Kısa sessizlik quiescence değildir.

ExposureTime=5000 µs ve TriggerDelay=0 yalnız okundu; ExposureAuto erişimi
Unavailable (1). Exposure/gain/ROI/UserSet/firmware yazılmadı. Trigger/source/
burst/cache ve Timestamp/Framecounter/ExtTriggerCount geçici ayarlandı; sekiz
metadata bankasının eski değerleri ve selector restore/readback ile doğrulandı.
Her alınan karenin Mono8 boyutu/payload ve embedded byte 4/8 sayaçları doğrulandı.
Görüntü ürün edinimine verilmedi, PGM üretilmedi; ilk satır dışlama izni korunur.
Clear stream çalışırken uygulandı; stop sonrasında Clear çağrılmadı. Restart
sonrası Clear yapılmadı, olası eski teslimi saklamaması sağlandı.

İlk fiziksel ön deneme ExposureAuto enum unavailable hatasıyla tetik göndermeden
durdu; cleanup/restore başarılıydı. Erişim kontrolü NI/NA için düzeltildi.
Sonraki ön koşumda SDK Clear sonrası NODATA'yı bazen beklemeden döndürdü.
Final binary monoton bütçe içinde kalan süreyi tekrar bekler, 1 ms ara verir,
actual süre/attempt sayısını kaydeder. Ön sonuçlar final koşum diye sunulmaz.

## Değişen dosyalar

- `tests/industrial-camera-probe/trigger.cpp`: opt-in `--stream-barrier`, 12 senaryo,
  monoton gözlem penceresi, metadata layout doğrulaması, sayaç ve host zaman kanıtı.
  Ürüne linklenmez, CTest değildir; mevcut tanı seçenekleri korunur.
- `industrial-camera-trigger-correlation.md`: Clear karşı örneği, aynı handle
  restart/reset gözlemi, geçerli kanıt sınırı.
- `industrial-camera-vendor-questions.md`: exact yeni karşı örnek ve reset sorusu.
  Taslak **gönderilmedi**.
- Progress ve arşiv dizini. Ürün/UI/kalıcı şema/local INI değişmedi.

## Yeni doğrulama ve kanıt

`build/icm/04d-barrier/` altında:

- `build-final.log`: opt-in diagnostic Release build exit 0.
- `sandbox.json`: exact cihaz görünmedi; fiziksel cihaz yokluğu değildir.
- `native.json`: ExposureAuto unavailable ön denemesi, 0 tetik, başarılı restore.
- `native-preliminary.json`: tek NODATA ön koşumu; final pencere kanıtı değil.
- `native-final.json`: son binary, 12 senaryo/36 command/27 kare, cleanup/restore
  başarılı, exitCode=0; productionAdmission/ready/counterEpoch/barrier tümü false.
- `inventory-after.json`: ayrı salt okunur process, 1 cihaz; open/readback/close/
  destroy başarılı. Eski trigger/source/burst/cache, aktif metadata false ve
  5000 µs exposure doğrulandı. Sekiz bankanın tamamının readback'i tanı cleanup'ında.
- `validate.py`, `validated-summary.json`: JSON assertions, embedded byte/sayaç
  eşleşmesi, pencere süreleri, altı reset ve source/binary/report SHA256 doğrulandı.
- `invalid-cli.json`: relative DLL path ret, exit 1, command/frame 0.
- `ctest.log`, `ctest.xml`: mevcut SDK build'inde ilgili iki CTest **2/2 yeni koşum**.
  Ürün kaynakları yeniden derlenmedi; tam suite/UI/görsel/stress koşulmadı.
- `context-check.json`: bağlam boyut/link kontrolü.

Yerel SDK header StopGrabbing açıklaması yalnız stop çağrısını, Clear açıklaması
acquisition sürerken buffer temizlemeyi anlatıyor; pending exposure/USB completion
garantisi eklemiyor. [Üretici 2023 kılavuzu](https://www.opto-e.cn/repository/_shared/COE-U-series-USB3.0-Camera-User-Manual_1.2.0_20230428.pdf)
için yeni web araması barrier garantisi sağlayan ek kaynak bulmadı. Başka üretici
semantiği exact cihaz kanıtına dönüştürülmedi.

## Açık sınırlar ve sonraki görev

Clear-only adayının yetersizliği artık somut. Clear/Stop/Start adayı henüz
çürütülmedi fakat belgeli başlangıç/recovery bariyeri yok. Uzun exposure, trigger
delay, command failure/cancel/unplug/reset/duplicate teslim garantileri ve fiziksel
uzun stress bu koşuma dahil değil. 36 command'ın hepsi ürün kabulü değildir;
9 command için kare tüketilmemesi gözlem senaryosunun parçasıdır.

Sıradaki adım exact firmware için üretici dokümanı/yanıtıyla stop completion,
gecikmiş command ve stream sayaç reset semantiğini kapatmak; gerekirse bu garantiye
yönelik uzun exposure karşı deneyi. Gönderim yetkisi bu görevde verilmedi.
Bu semantik kapanmadan 04e başlamaz. İlk satır dışlama ürün geometri/kalibrasyon
kimliğine henüz işlenmedi. Native Unavailable; G0–G4, H01–H08, sahne/lens/ölçüm,
dağıtım ve CUDA sınırları korunur. ICM-04d görev ID'li tek commit; push yok.
