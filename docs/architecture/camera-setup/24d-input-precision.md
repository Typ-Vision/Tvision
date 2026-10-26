# CCUX-24D — üç ondalıklı giriş, kayıpsız mevcut değer

Durum: tamamlandı; [kapanış](../../archive/2026-10-26-ccux-24d.md),
[alan envanteri ve edit sözleşmesi](24d-input-inventory.md). [Ortak plan](24-usability-plan.md) ve
[analiz](24-usability-analysis.md) 6 okunur. Sonraki: 24E.

## Envanter ve sınır

Başlangıç `src/app/` dosyaları: MetrologyEditor, EdgePointEditor, RoiEditor,
PoseMatchEditor, MainWindowEditor, MainWindowTracking, CalibrationWizard.
Tüm dosyalarda aynı rakamı değiştirmek yerine birim bazlı alan envanteri çıkarılır.

| Alan sınıfı | Görünüm / kayıt kararı |
| --- | --- |
| mm/px uzunluk ve koordinat | Üç basamak, adım 0.001; değiştirilmemiş özgün double korunur |
| Pozitif uzunluk | Yeni düzenlenen değerde alan uygunsa minimum 0.001 |
| Negatif olabilen koordinat | Mevcut işaret/aralık korunur |
| mm/örnek gibi dönüşüm katsayısı | Boyut analiziyle ayrı değerlendir; minimum/kayıt kuralını körlemesine kopyalama |
| Adet/sıra | Tamsayı |
| Derece/yüzde/skor/birimsiz/exposure/gain | Bu kartın üç basamak dönüşümü dışında |

## Uygulama adımları

1. Her alan için widget, config anahtarı, birim, min/max, başlangıç, commit
   callback'i ve no-op davranışını kaydet. Mevcut MetrologyEditor korumasını incele.
2. Modelden gelen özgün double, başlangıç gösterim değeri ve gerçek kullanıcı
   düzenlemesini ayıran ortak yaklaşım kur. Yükleme/signal/refresh, edit sayılmasın.
3. UI hassasiyetini üçe indir. Başka alanın commit'i veya sayfa/araç değişimi
   dokunulmamış değeri spinbox'tan geri okumak yerine özgün modelden korusun.
4. Gerçek edit olduğunda gösterilen üç basamaklı değer kaydedilsin. Yazıp geri
   alma, aynı görünen değeri açıkça yazma, Enter/odak kaybı ve iptal davranışları
   testle tanımlansın; yalnız `valueChanged` ilk yüklemesi edit kanıtı sayılmasın.
5. 0.001 altındaki eski pozitif değer, dokunulmazsa aynen kalsın. 0.000 görünümü
   gerekiyorsa tooltip'te özgün değer gösterilsin; yüklemede min clamp modele yazılmasın.
6. JSON/solver/model hassasiyetine yuvarlama ekleme. Reçete no-op hash ve dirty
   politikası korunur; gerçekten değişen parametre normal invalidation uygular.

## Kabul matrisi

| İşlem | Beklenen |
| --- | --- |
| 1.23456789 yükle; başka alanı değiştir | Özgün 1.23456789 değişmez |
| Yükle/Save/reopen; sayfa/araç değiştir | No-op içerik/hash korunur |
| Değeri açıkça 1.235 yap | 1.235 kaydolur, ilgili dirty/invalidation oluşur |
| Küçük pozitif legacy ve minimumdaki yeni giriş | Eski değer korunur; yeni geçersiz sıfır kabul edilmez |
| Türkçe virgül, negatif koordinat, büyük değer | Locale/aralık doğru; basamak kaybı kontrollü |
| Adet/açı/pozlama alanı | Birim dışı alan davranışı değişmez |
| Kalibrasyon adayını iptal et | Önceki etkin model korunur |

Testler: `ui_geometry_smoke`, `ui_roi_tool`, `ui_pose_tracking_smoke`,
`ui_circle_smoke`; envantere göre `ui_calibration_wizard`. Anlamlı yeni senaryo:
yükle→başka alan commit→encode/hash ve açık edit→encode; yalnız decimals getter
assert'i yeterli değildir. Ortak Release/görsel/kapanış kuralları uygulanır.

Kanıt `build/ccux/ccux-24d/`; envanter ve no-op sonuçları kapanıştan bağlanır.
Commit: `CCUX-24D: show three decimals without losing untouched precision`.
