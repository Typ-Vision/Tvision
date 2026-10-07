# Recipe Editor / Image File sözleşmesi — Adım 3 temeli

## Güncel mimari çekirdek ve devam işleri

A7 [coordinates.md](coordinates.md) ortak Pose2D/Transform2D/TrackingRoi ve kalibrasyon bağımlılık kaydını tanımlar. Edge aynı çekirdekle yalnız öteleme yapar; editor schema 5 ve legacy migration korunur. Genel pose/dört köşe ROI henüz editor JSON alanı değildir. Kalibrasyon dependency schema 1 JSON ayrı kayıt sözleşmesidir; reçete/kalibrasyon uygulaması 6C'de kalır.

[6A planı](development-plan.md) kapsamında A3 tool tanımları/portlar/parametre doğrulaması ToolRegistry üzerinden sunulur; A4 öğretim ve worker/cache kuralları EditorSession/CameraSession bileşenlerindedir. A5 ImageFrame dosya servisinden ayrılmış, ortak owned buffer oluşturma ve metadata sözleşmesi kazanmıştır. Mevcut Properties, Toolbox, Flow ve Image Viewer görünümü, panel yerleri ve menüler korunur. A2 tamamlandı: güncel editor schema 5, Edge referenceGeometry kimliği ve schema 1–4 baseline migration sözleşmesi [tool-execution.md](tool-execution.md) içindedir. Sonraki migration/paketleme çalışmaları A9'da kalır.


Adım 4 güncellemesi: editor schema 2, ROI/model ve gerçek match artık uygulanmıştır. Güncel parametre/migration/thread/oturum sözleşmesi [matching-editor.md](matching-editor.md) içindedir. Aşağıdaki schema 1 örneği eski dosya formatını açıklar.

Güncel Adım 5 sözleşmesi: [tool-execution.md](tool-execution.md). Typed DAG, Blob/Edge/Calculate ve worker acquisition uygulanmıştır; aşağıdaki Adım 3/4 açıklamaları tarihsel teslimi anlatır.

7 Ekim 2026. Üst proje/reçete şeması hâlâ 1'dir; mevcut `.vfproject` dosyaları korunur. Yeni yapı `recipe.definition.editor` alanında ayrı `schemaVersion: 1` taşır. Editor alanı olmayan Adım 2 reçeteleri boş tool listesiyle açılır; yalnız açmak reçeteyi dirty yapmaz.

## Bileşenler

| Bileşen | Sorumluluk |
| --- | --- |
| domain/ToolDefinition | Tool ID/kind/name/enabled/parameters, editor şema doğrulaması |
| application/ProjectService | Add/update/remove tool, tek dosya kaynağı, path çözümleme, DRAFT/dirty/revision |
| application/ImageFileService | QFile ile Unicode yol okuma, OpenCV imdecode, orijinal piksel verisi ve bağımsız QImage preview |
| app/ImageViewer | Qt Graphics View üzerinde görüntüleme, fit/zoom/pan, view→image piksel eşleme |
| app/MainWindowEditor | Toolbar, Toolbox, Flow listesi, dinamik Properties, servis komutları |

UI OpenCV algoritması veya dosya okuma API'sini çağırmaz. `ImageFrame` orijinal cv::Mat'ı private tutar; UI preview ve piksel bilgisi API'sini kullanır. QImage preview deep-copy ile decode tamponundan bağımsızdır. Domain/project library Qt Core; acquisition/session Qt Gui/Concurrent + OpenCV; viewer Qt Widgets kullanır.

## Minimal kayıt yapısı

```json
{
  "editor": {
    "schemaVersion": 1,
    "tools": [
      {
        "id": "70c27dcd-9e31-451c-9552-ad7dba2583a6",
        "kind": "image_file",
        "name": "Image_File_1",
        "description": "Test görüntüsü",
        "enabled": true,
        "parameters": { "path": "images/part.png" }
      },
      {
        "id": "8df11767-0b3c-4f7b-82f6-e689f866a4a0",
        "kind": "template_match",
        "name": "Match_Part",
        "description": "",
        "enabled": true,
        "parameters": { "minScore": 0.7 }
      }
    ]
  }
}
```

Bu nesne `definition` içine yazılır. `definition` içindeki editor dışı alanlar korunur. Editor/tool içinde bilinmeyen alan, kind veya schema reddedilir; gelecekte yeni parametre/port eklemek açık migration kararı gerektirir. Kalıcı UUID, tool adı değişince korunur. Tool adları reçetede case-fold karşılaştırmayla benzersizdir; 1–64 yazdırılabilir karakter, description en fazla 2048 karakter; en fazla 256 tool.

| kind | parameters | Şimdiki davranış |
| --- | --- | --- |
| image_file | path: string | Gerçek dosya görüntüsü edinme; reçetede en fazla bir kaynak |
| template_match | minScore: number 0–1 | Parametre düzenleme; UI yüzde gösterir, match yürütülmez |
| blob | threshold: integer 0–255 | Parametre düzenleme; segmentation yürütülmez |
| edge | direction: left_to_right/right_to_left/top_to_bottom/bottom_to_top | Parametre düzenleme; edge araması yürütülmez |
| calculate | operation: distance | İşlem gösterimi; input bağlantısı/hesaplama henüz yok |

Tool ekleme/silme, ad/description/enabled ve geçerli parametre değişikliği DRAFT/dirty yapar. Revizyon başarılı Save sırasında bir artar; no-op değişiklik revizyonu artırmaz. Yeni reçeteye otomatik yürütülebilir şablon/tool grafiği yüklenmez. Flow şu an tool listesidir; sıra yürütme veya typed DAG anlamına gelmez.

## Dosya kaynakları ve viewer

Open Image önce decode eder; başarılıysa Image File tool'u oluşturur/günceller. Aynı kaynağı ikinci kez eklemek mevcut ID'yi seçer. Dosya yolu proje dosyasının klasörüne göre relative yazılır; dış klasör için `../` içerebilir. Image dosyası paket içine gömülmez veya kopyalanmaz. Projeyi taşırken relative görüntü varlıklarını da taşıyın. Kamera metadata referansı ile Image File path ayrı alanlardır.

Reçete seçilince kayıtlı kaynak çözülür ve görüntü yüklenir. Reçete değişimi, kaynak silme veya source disabled durumunda eski frame temizlenir. Bozuk yeni dosya seçiminde önceki çalışan kaynak/preview korunur. Kayıtlı kaynak Reload sırasında bozuk/eksikse eski preview gösterilmez; viewer boş/hata durumuna geçer. Dosya içerik değişimi otomatik izlenmez; Reload açıkça tekrar okur. Reload/Fit/Zoom/Pan reçete config'i değişmediği için dirty yapmaz.

Görüntü formatları: Gray8, Gray16, RGB8, RGBA8. Gray16 preview sabit 0–65535 → 0–255 ölçeğiyle hazırlanır; piksel metni orijinal 16-bit değeri gösterir. BGR/BGRA RGB/RGBA'ya yalnız görüntüleme için dönüştürülür. Diğer depth/channel türleri reddedilir. Dosya codec'i kurulu OpenCV imgcodecs desteğine bağlıdır; testler PNG formatındadır. EXIF auto-rotate uygulanmaz (IMREAD_UNCHANGED); image pixel koordinatları decoded veriyle aynıdır.

Tek dosya en fazla 256 MB, decoded görüntü en fazla 100 MP. Decode şu aşamada senkron çalışır; background acquisition/engine yürütmesi Adım 5 kapsamında tasarlanacaktır. Kamera SDK, canlı trigger, frame/cycle/result engine sözleşmesi henüz yoktur.

Viewer: Fit, +/−, 1:1, tekerlek zoom, sol tuşla sürükleyerek pan. Zoom kullanıcı komutları 0.001–32x aralığındadır. Piksel koordinatı (x,y), sol üst (0,0), birim px; preview scale ham görüntü koordinatını değiştirmez. Hover değerleri sınır kontrolüyle orijinal frame'den alınır. Zoom değişiminde eski hover metni temizlenir. Image'e tıklamak Image File tool'unu Flow/Properties'de seçer; cyan sınır yalnız kaynak seçimini gösterir, match/ROI sonucu değildir. Escape tool seçimini temizler.

Properties metin değişiklikleri geçerli olduğunda uygulanır; numeric/checkbox değişiklikleri doğrudan uygulanır. Hatalı alan config'i değiştirmez ve Save reddedilir. Tool seçimi sırasında doğrulama yapılır. Henüz Undo/Redo veya genel draft form recovery yoktur.

## Testler ve sonraki kapsam

CTest 3/3: mevcut project_persistence; genişletilmiş ui_shell_smoke; yeni editor_services. PNG 8-bit/16-bit/color/alpha, Unicode yol, odd-row-stride, ham piksel/kanal düzeni, bozuk/eksik/unsupported veri, ID/parametre/source/dirty/revision roundtrip, source disable/discard, tool silme, zoom/pan ve koordinatlar kontrol edilir. UI smoke 2592×1944 sentetik PNG dosyasını gerçek decode hattından geçirir; Properties/Flow, valid/invalid alanlar, save/reopen, reçete görüntü izolasyonu ve bozuk Reload sonrası toparlanmayı doğrular.

Görsel kanıt: `build/step3-editor.png`. Test görüntüsü `build/görüntü_ölçüm.png`; kamera fotoğrafı değildir. Native dosya seçici, gerçek mouse wheel/high-DPI/multi-monitor deneyimi ayrıca manuel denenebilir.

Adım 4: ROI çizimi, model/search ROI teaching, gerçek template match ve overlay. Bu aşama mevcut editor schema 1 için açık migration ve tool/frame/sonuç invalidation tasarımı gerektirir. Blob/Edge/Calculate yürütmesi, typed port/DAG ve genel engine Adım 5'te kalır. Bu adım PASS/FAIL, angle/pose veya gerçek match skoru üretmez.
