# CCUX-19 — Sade kamera akışının birleşik kabulü ve kullanım belgeleri

Durum: yazılım/görsel kabul tamamlandı. [22 Ekim kapanışı](../../archive/2026-10-22-ccux-19.md).
Önkoşul: CCUX-16–18. CCUX-15 fiziksel kabulü açık.
[Ortak sözleşme](contract.md); yalnız gerekirse önceki kartların kanıtları.

## Uygula ve kabul et

1. Yeni proje ve eski proje için Kamera → Kalibrasyon → Editor → Runtime →
   Logs/CSV zincirini normal komutlarla doğrula. Hiçbir aşamada deneysel seçenek,
   tetik eşleşmesi uyarısı veya eski kalibrasyon sayfasına geçiş gerekmesin.
   Bir ve beş kamera fixture'ı, üç applicationType, Raw/px ve kaynak başına mm,
   canlı edinim ve PGM+JSON replay kapsansın. Beş fixture kamera fiziksel kabul değildir.
2. Restart/proje değişimi/USB kopma simülasyonu, yanlış seri/geometri, bozuk
   sidecar, model/ayar değişimi, Stop/geç completion, lease çakışması ve kayıt
   hatası/retry negatiflerini birleştir. Restart otomatik capture/Run başlatmaz;
   aynı matematik güncel uyum/probe sonrası normal komutlarla kullanılabilir.
3. Eski normal ve OECS runtime1/2 kayıtları Logs'ta okunur; seçili kayıt CSV'si
   doğru exporter/şema/birim/model/kökeni korur. Yeni ekranlar eski kayıtları
   doğrulanmış production kaydına dönüştürmez. Sayfa değişimleri crash üretmez.
4. CCUX-16–18'in ilgili suite'lerini UI Release ve core-off Release üzerinde
   birleşik olarak bir kez çalıştır; UI olmayan build'e UI testleri bekleme.
   `ctest -N` hedefleri ve fixture'ları doğrulasın; sıfır test başarı sayılmaz.
   Önceki başarıları bu koşumun sonucu diye kopyalama.
5. Kameralar/Kalibrasyon/iki sihirbaz/Editor/Runtime/Logs için normal ve küçük
   boyut görsellerini incele. Görünür metin ve tooltip taramasını gerçek UI
   senaryolarıyla tamamla; yalnız kaynak metni araması görsel kabul değildir.
6. README, `docs/camera-runtime-usage.md`, `docs/editor-usage.md`,
   `docs/windows-camera-diagnostics.md` kullanım adımlarını yeni akışa göre
   güncelle; planlananı çalışıyor diye önceden yazma. İlgili eski mimari belgelerde
   opt-in UI açıklamalarını güncelle, kalıcı şema/kanıt sözleşmesini koru.

## Kapanış ve sonraki görev

Kanıt `build/ccux/ccux-19/`; Release/CTest komutları, exit kodları, görseller ve
senaryo matrisi tarihli kapanışta. `python scripts/check-context-docs.py` zorunlu.
Her tamamlanan kart tek görev ID'li commit; push yok.

Bu kart ürün akışının yazılım/görsel kabulüdür. Yeni fiziksel koşum yapılırsa
ayrı kanıt yazılır. [CCUX-15](15-physical-acceptance.md) bağımsız referans,
ışık/odak, tolerans/mm ve kullanıcı işlem kabulü için açık kalır; sonraki görevdir.
G1/üretim, robot/PLC, kurulum/dağıtım ve CUDA doğrulanmış sayılmaz.
