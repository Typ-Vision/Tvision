# Adım 4: ROI / teaching / eşleştirme ve oturum

## Güncel durum — A1–A7 çekirdek tamamlandı, sıradaki A8

A7 koordinat ve tracking ROI sözleşmesi [coordinates.md](coordinates.md) içindedir. Edge translation-only point anchor ortak dönüşümü kullanır; kutu boyutu, delta yuvarlama, migration/invalidation ve kayıt şeması korunur. Pose2D/dönmüş dört köşe ROI yalnız domain geometri çekirdeğidir; Match açı tespiti/Pose portu ve dönmüş crop henüz yoktur. A6 lifecycle/deadline/kuyruk çekirdeği SDK stream'e bağlı değildir; fiziksel kabul 6B'de açık kalır.

[6A/A1–A2](development-plan.md) tamamlandı: Run Match mevcut düğmesiyle ortak engine üzerinden bağlı girdileri çözer; aynı frame/config altında Run Flow ile tutarlı sonuç verir. Her iki UI komutu kaynağı yeniden edinir; Run Match yalnız gerekli upstream alt grafiğini çalıştırır. Stop ortak cooperative iptal uygular. Sonuçlar cycle/frame kimlikleriyle gösterilir. Schema 5 referans kimliği model ROI ve PNG içeriğine bağlıdır; değişim Edge ERROR/downstream SKIPPED üretir. Search ROI yeniden çizimi ilişkiyi günceller. Save/reopen ve legacy baseline migration kabulü tamamlandı; ayrıntılar [tool-execution.md](tool-execution.md) içindedir. A3 registry/ayrı yürütücüler, A4 EditorSession worker/cache/reference teaching ve A5 owned frame/metadata kabulü tamamlandı. GUI görünümü ve menüler korunur. Aşağıdaki Adım 4 servis/thread açıklamaları tarihsel kayıttır.


Güncel Adım 5 sözleşmesi: [tool-execution.md](tool-execution.md). Typed DAG, Blob/Edge/Calculate ve worker acquisition uygulanmıştır; aşağıdaki Adım 3/4 açıklamaları tarihsel teslimi anlatır.

7 Ekim 2026. Tek Recipe Editor korunur. Template Match gerçek hesap yapar; Blob/Edge/Calculate configuration aşamasındadır. Genel Runtime çevrimi çalıştırılmaz; recipe DRAFT ve parça PASS/FAIL ayrı tutulur.

## Kullanım

Görüntüyü açın, Template Match ekleyip seçin. Properties içindeki **Model ROI çiz / öğret** ile ayırt edici dokuyu sürükleyerek seçin; sarı ROI ve model oluşturulur. **Search ROI çiz** mavi arama alanını belirler; **Tam kare ara** arama ROI'sini temizler. ROI yeniden çizilerek değişir. Escape çizimi/seçimi iptal eder; normal modda pan/zoom devam eder.

**Run Match** Results sekmesine geçer. Ham normalize skor, en iyi adayın sol üst X/Y koordinatı ve servis süresi gösterilir. Eşik üstü aday yeşil; eşik altı en iyi aday kırmızıdır. Skor olasılık değildir. Tek sonuç, sabit açı/ölçek, `TM_CCOEFF_NORMED`; maske, angle/scale ve NMS yok. Sabit renkli model reddedilir.

## Servis ve thread sözleşmesi

`MatchService`, ImageFrame'in private ham verisini vision adapter'e bağlar; UI OpenCV çağırmaz. Gray8 doğrudan, BGR/BGRA grayscale dönüşümüyle (alpha dikkate alınmaz), Gray16 sabit 0–65535 → 0–255 ölçeğiyle kullanılır. Öğretme ve eşleştirme QtConcurrent worker üzerinde; girdiler immutable frame/config snapshot'ıdır.

Tamamlanan iş yalnız proje/reçete/tool kimliği, tüm recipe definition ve frame generation hâlâ aynıysa yayımlanır. Source reload/replace/disable, reçete/tool seçimi veya configuration değişikliği eski sonucu geçersiz kılar. Save sonuçları kalıcılaştırmaz; Run Match recipe'yi VALID veya dirty yapmaz. Worker çalışırken tekrar run/ROI ve kapanış engellenir; kaynak/config değişirse bekleyen sonuç sessizce atılır. İş hard timeout/cancel desteklemez. Dosya decode hâlâ senkrondur; genel engine Adım 5'tedir.

## Kayıt ve migration

Root/recipe schemaVersion 1 kalır; `definition.editor.schemaVersion` artık 2:

```json
{
  "minScore": 0.7,
  "modelRoi": {"x": 700, "y": 650, "width": 64, "height": 64},
  "searchRoi": {"x": 650, "y": 600, "width": 512, "height": 512},
  "modelPng": "<Gray8 PNG base64>"
}
```

Boş ROI `{}`; boş model `""`. Boş search ROI tam kare demektir. ROI piksel koordinatında tamsayıdır, başlangıç (0,0); sürükleme yönünden bağımsız ve görüntü sınırına clamp edilir. Çizim minimum 2×2; model en fazla 1024×1024, base64 en fazla 4 MB. Eşleştirme PNG/Gray8/boyut/ROI sınırı/modelden küçük arama/eksik model durumlarını doğrular. Model PNG header boyutu decode öncesi kontrol edilir. Dış kaynak görüntü değişince model korunur, sonuç temizlenir; yeni görüntüye karşı tekrar arama yapılabilir.

Editor schema 1 dosyaları okunurken mevcut ID/minScore korunur, yeni model/ROI alanları boş olarak sunulur. Sadece açma ve no-op Properties uygulaması dirty yapmaz. İlk gerçek tool değişikliğinde schema 2 yazılır. Desteklenmeyen şema ve alanlar reddedilir; editor dışı definition alanları korunur.

Model PNG proje JSON'una gömülür. Model ve metadata aynı QSaveFile commit'inde saklanır; ayrı asset yazımında oluşabilecek yarım kayıt yoktur. Proje toplam 16 MB sınırını aşarsa kayıt başarısız olur ve dirty veri bellekte kalır. Üretimde çok sayıda büyük model için paket/asset düzeni ayrıca tasarlanmalıdır. Görüntü dosyası gömülmez, relative kaynak yolu korunur.

## Oturum

Başarılı normal kapanışta `config/local.ui.ini` içinde `session/project`, `session/recipe`, `session/page` kaydedilir. Son kaydedilmiş proje sonraki açılışta ProjectService üzerinden okunur; mevcut aktif reçete/ekran restore edilir. Artık var olmayan reçetede editor yerine Recipes açılır. Proje eksik/bozuksa nonmodal hata gösterilir, Open Project kullanılabilir; son yol silinmez. Hiç kaydedilmemiş yeni proje, Discard ile bırakılınca restore edilmez. Crash recovery veya autosave değildir. Smoke test üretim INI'sini kullanmaz ve başlangıç restore'u atlar; ayrı pencerelerle restore doğrular.

## Kabul ve performans

Release CTest: session project/recipe/page restore ve missing file, gerçek viewport ROI gesture, reverse drag ve sınır clamp, teaching/save/reopen/Run Match, worker sırasında kapanış ve stale-result reddi. Core testler 5 MP Gray8 pozitif/orijinal ve taşınmış model konumu, bağımsız random negatif görüntü, eksik/bozuk model, flat template, dışarı taşan/küçük search ROI ve Gray16 dönüşümünü kapsar. Fixture'lar gerçek decode ve OpenCV hesabından geçer; sentetiktir, üretim false accept/reject doğrulaması değildir.

`editor_services_test.exe --benchmark`: 2592×1944 Gray8, 64×48 model, 512×512 search ROI; 10 warmup + 100 ölçüm, model PNG decode ve hazırlık + match + minMaxLoc. 100/100 doğru konum; yerel median 7.1516 ms, p95 7.4798 ms. CSV `docs/performance/roi_match_service.csv`. Acquisition, teaching, worker dispatch/queue ve UI çizimi bu süreye dahil değildir; RGB grayscale dönüşümü ek süre getirir. Tam 5 MP aramanın <10 ms olduğu iddia edilmez. Mevcut tam kare ölçümleri `opencv_matching.md` içindedir.

Görsel kabul: `build/step4-editor.png`, ROI/aday overlay, Results ve kaydırılabilir Properties incelendi. Native yüksek DPI/dosya seçici/kamera senaryoları ayrıca denenecek.
