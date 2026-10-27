# Değişken oluşturma ve değerini değiştirme

Bu rehber 28 Ekim 2026 tarihindeki VisionFlow arayüzünü anlatır.
Değişkenler reçeteye aittir; değerleri her çalıştırma çevriminde yeniden üretilir.

## 1. Değişken oluştur

1. Projeyi ve düzenleyeceğin reçeteyi Editor'da aç.
2. **Variables → Ekle** düğmesine bas.
3. Örnek olarak **Ad:** `ParcaAdedi`, **Tip:** `Float64`,
   **Birim:** `unitless` seç. İstersen açıklama ekle ve **Save/Kaydet** ile onayla.

Bu işlem değişkenin tanımını oluşturur. Henüz değer atanmadığı için tabloda
`Missing` görülmesi normaldir. **Düzenle** ad/tip/birim/açıklama içindir;
tablodaki **Değer** hücresine elle yazılmaz.

## 2. Sabit değer ata veya değiştir

1. **Toolbox → Set Variables** aracını akışa ekle ve seç.
2. **Properties** panelinde **+** ile atama satırı ekle.
3. **Hedef** alanından `ParcaAdedi` değişkenini seç.
4. **Uyumlu kaynak** alanını **Typed constant** yap.
5. **Typed constant (JSON)** alanına `5` yaz.
6. **Atamaları uygula** düğmesine bas, projeyi kaydet ve **Run Flow** çalıştır.
7. **Variables** sekmesinde değerin `5`, geçerliliğin `Valid` olduğunu kontrol et.

Değeri `10` yapmak için aynı atama satırındaki sabiti `10` olarak değiştir,
**Atamaları uygula → Kaydet → Run Flow** adımlarını tekrarla.
**Taslağı geri al**, uygulanmamış atama değişikliklerini iptal eder.

Sabit alanı JSON biçimi ister:

| Değişken tipi | Yazılacak örnek |
| --- | --- |
| Boolean | `true` veya `false` |
| Int32 / UInt32 | `5` |
| Float64 | `12.5` (ondalık ayırıcı nokta) |
| String | `"Parca A"` (tırnaklı) |
| Int64 | `"123456789"` (tırnaklı) |

## 3. Değeri program çalışırken araç sonucundan al

Her çevrimde gerçek sonucu kullanmak için sabit yerine bir araç çıkışı seç:

1. Örneğin akışta görüntü kaynağına bağlı bir **Blob** aracı hazırla.
2. Set Variables satırında hedefi `ParcaAdedi` bırak.
3. **Uyumlu kaynak** listesinden ilgili Blob aracının **count** çıkışını seç.
   Bu çıkış için hedefin `Float64 / unitless` olması gerekir.
4. **Atamaları uygula → Kaydet → Run Flow** çalıştır.

Böylece `ParcaAdedi`, o çevrimde Blob'un bulduğu nesne sayısını alır.
Boolean değişken için uyumlu `found` çıkışı da seçilebilir. Listede yalnız
uyumlu tip/birimde kaynaklar gösterilir. Kaynak seçimi yürütme bağımlılığını kurar.

## Bilinmesi gerekenler

- Değerler çevrim başında **unset** olur; önceki çevrimden taşınmaz ve reçeteye
  sonuç değeri olarak kaydedilmez. Kalıcı sayaç gibi kullanılmaz.
- Bir Set Variables aracındaki bütün satırlar birlikte uygulanır. Bir satır
  başarısızsa grubun tamamı uygulanmaz; aynı hedefe iki satır yazılamaz.
- Satırları yukarı/aşağı taşımak sıralı hesaplama oluşturmaz. Sonraki işlem,
  öncekinin yeni değerini okuyacaksa ikinci bir Set Variables aracı ekle ve
  kaynak olarak ilkinin **bundle** alanını açıkça seç.
- `ParcaAdedi + 1` gibi serbest ifadeler sabit alanına yazılamaz. Hesaplama
  için uygun **Calculate** aracı çıktısı kullanılabilir.
- `System.*` satırları salt okunurdur. Editor'da **Run Tool / Run Flow**
  hesaplama ve önizlemedir; PLC/robota veri göndermez.
- Runtime'da kayıtlı yeni tanımı kullanmak için çalışan oturumu **Stop** ile
  durdurup reçeteyi yeniden **Load** et; ardından **Run** ve çevrim başlat.

Teknik ayrıntı: [değişken ve atama sözleşmesi](architecture/communication/contract.md#c2--variable-tanımı-ve-atama).
