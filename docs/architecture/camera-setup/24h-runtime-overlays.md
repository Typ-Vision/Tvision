# CCUX-24H — Runtime'da bütün araç overlay'leri

Durum: planlandı; 24G sonrası, 24E viewer ve 24B panel düzeni kullanılır.
[Ortak plan](24-usability-plan.md), contract C4/C5 ve
[analiz](24-usability-analysis.md) 10 okunur. Sonraki: 24I.

## Hedef ve başlangıç dosyaları

Görünür “Tüm araçları göster” seçeneği varsayılan açık olur. Tek çekim ve normal
çevrim aynı güncel kaynak/çevrim overlay sunumunu kullanır. Tool seçimi ayrıntı
ve kenar profilini yönetmeye devam eder; checkbox kapalıyken seçili araç odağı
korunur. Yerel tercih reçete/hash'i veya yürütme sonucunu değiştirmez.

Başlangıç: `src/app/MainWindowRuntime.cpp`, `ImageViewer.cpp/.hpp`,
`ImageViewerRoi.cpp`; gerçek sonuç ve referenceBinding/poseTracking verisini
üreten mevcut application sunum yolları kart başında graf/kaynakla doğrulanır.
Tek `setToolOverlay` çağrısını döngüye almak veya tek düzenlenebilir roi_ alanını
paylaştırmak yeterli kabul edilmez.

## Önce dar veri sözleşmesi

Uygulamadan önce aşağıdaki öneri mevcut domain tipleriyle eşlenerek küçük bir
tasarım notuna bağlanır; yeni adlar mevcut API varmış gibi kullanılmaz.

| Veri | Sorumluluk |
| --- | --- |
| Sunum anahtarı | Yüklü snapshot kimliği + source ID + frame/cycle + geometri/uzay |
| Araç katmanı | Tool ID, kısa ad, deterministik renk, execution durumu |
| İlkel öğe grupları | Tarama ROI/yönü, kabul/ret noktaları, sonuç çizgi/çemberi, maske/code köşeleri, etiket |
| Köken/uygunluk | Aynı çevrimin gerçek çözülmüş referansı/pose'u ve koordinat dönüşümü |
| Viewer girdisi | Atomik değiştirilen salt okunur katman koleksiyonu; recipe edit callback'i yok |

Tüm araçlar modunda ortak raw geometri kullanılır. İşlenmiş preview'dan gelen
öğe için doğrulanmış raw dönüşüm gerekir; yoksa yanlış konuma çizmek yerine öğe
çizilmez ve ayrıntıda neden erişilir olur. Mevcut tool sonuç tipi dönüşüm
sağlamıyorsa gereken application veri bağı bu kartta eklenir, UI tahmini yapılmaz.

## Uygulama adımları

1. Circle, Line, Edge/Measurement, Blob/Code için mevcut çıktı/overlay türlerini
   ve kaynak koordinatlarını envanterle. Her öğenin frame/geometri kaynağını belirle.
2. Bir çevrimin seçili kaynağına ait katmanları toplayan sunum yolunu oluştur.
   Snapshot/source/frame/cycle uyuşmazlığını filtrele; eski katmanları yeni
   çevrimin henüz bitmemiş öğeleriyle birleştirme.
3. Her tool için ayrı ilkel gruplar üret. Farklı araçların tek noktalarını
   otomatik mesafe çizgisiyle bağlama. Etiket/renk tool kimliğiyle kararlı olsun.
4. Tarama bölgesini nominal config'ten değil aynı çevrimde çözülmüş referenceBinding
   ve poseTracking sonucundan üret. Referans çözülemezse eski/nominal ROI gösterme.
   Failed/skipped araçta bulunmuş sonuç üretme; geçerli ayar ROI'si çizilirse
   sonucu olmayan tarama alanı olduğu açıkça ayırt edilsin.
5. Viewer'a toplu read-only set/clear yolu ekle; Editor'ın mevcut düzenleme
   davranışı bozulmasın. Runtime ROI tutamaçları/drag callback'leri kapalı kalsın.
6. Checkbox ve yerel kalıcılığı ekle. Tarama alanları, bulunan noktalar, sonuç
   şekilleri ve etiketler için filtreler aynı katman verisine uygulanabilir;
   yeni acquisition/execution başlatmaz. Çoklu mod raw, seçili mod mevcut
   işlenmiş preview kullanıyorsa geçiş görüntü/overlay'yi atomik değiştirir.
7. Stop, Unload/yeni snapshot, kaynak değişimi, yeni çevrim ve hata durumlarında
   eski katmanları temizle. Eksik yeni karede önceki sonuç yeniymiş gibi görünmesin.

## Kabul matrisi

- Tek kaynakta Circle + Line + Edge/Measurement + Blob/Code; çizgi/çember,
  maske/köşeler, kabul/ret noktaları, tarama yönleri birbirine karışmaz.
- İki kaynak, benzer koordinatlar ve iki ardışık çevrim: yalnız seçili kaynağın
  doğru kare katmanları çizilir. Aynı tool adları tool ID ayrımını bozmaz.
- ReferenceBinding ve poseTracking başarı/ret/kayıp referans: yalnız gerçek
  çözülmüş geometri çizilir. Skipped/failed sonuç uydurulmaz.
- Toggle/filtre/araç seçimi ve restart: doğru yerel tercih; acquisition sayacı,
  reçete hash'i ve sonuçlar değişmez. Tek çekim/normal çevrim eşdeğer sunulur.
- Stop, hata, kaynak/snapshot değişimi: stale overlay kalmaz. Raw/işlenmiş geçişte
  görüntü ve geometri uyumludur; viewer düzenleme başlatmaz.
- Çok noktalı sahnede kayıtlı fixture boyutu/sayısıyla render süresi ve UI
  etkileşimi ölçülür. Katman/item sayısı çevrimler boyunca birikmez; 24H başlangıç
  ölçümüne göre regresyon açıklanır. Kanıtsız FPS/hız garantisi verilmez.

Testler: `ui_multi_source_runtime`, `ui_runtime_visibility`, `ui_runtime_failures`,
`reference_world_contract`, `world_tracking_contract`; yeni koleksiyon/geometri
ve stale-frame negatifleri eklenir. Ortak viewer değişirse 24E ilgili regresyonları
tekrar edilir. İki boyutta görsel kontrol ve karışık araç fixture'ı zorunludur.

Kanıt `build/ccux/ccux-24h/`; dar veri sözleşmesi ve uygulanan API teknik belgede,
kanıt ve sınırlamalar kapanışta tutulur. Ortak kapanış uygulanır.
Commit: `CCUX-24H: render source-scoped multi-tool Runtime overlays`.
