# KUI-4 — Kalibrasyon ana sayfası ve sihirbazlar

15 Ekim 2026. KUI-1–KUI-3 servisleri üzerine Qt Widgets sunumu eklendi. **KUI-5 ortak Editor/Runtime resolver henüz bağlı değildir**; ana sayfadaki etkinlik bu aşamada yalnız yeni katalog seçimini yönetir. Üç applicationType aynı MainWindow ve aynı kalibrasyon sayfasını kullanır.

Setup → Calibration üç yöntem kartını açar. Bilinen ölçü ve plaka ayrı yeniden boyutlandırılabilir CalibrationWizard pencereleridir. Robot kartı yalnız Eye-in-Hand / Eye-to-Hand açıklaması ve Kapat düğmesi içerir. Eski kayıt/kanıt ekranı ayrı uyumluluk düğmesiyle korunur; eski binding ve `.vfcalstudy` akışı dönüştürülmez.

## Uygulanan akış

1. Reçetenin çalışan Image File/Webcam kaynağından worker ile kare alın veya dosya açın. Kurulum adı, lens/kamera kurulumu ve çalışma düzlemi beyanını girin. Dosya kökeni korunur: Sadece deneme veya açık hedef kaynakla Bu kurulumdan kaydedilmiş görüntü. Deneme etkinleştirilemez. Industrial grab eklenmedi.
2. Bilinen ölçüde görüntüye tarama oku çizin. Tek kenar veya Kaliper gerçek Edge/EdgePair servislerini kullanır. Aday seçici ve gerçek adayın üstüne tıklama, nokta adı düzenleme, silme, yeniden bulma, tarama yönü ve kapalı örnekleme/sigma/şerit/seçim/subpixel ayrıntıları bulunur. Çizim uçları ölçüm ucu sayılmaz. Kaliper çifti hazır gelir; başka iki nokta seçilebilir.
3. Seçili çiftin px mesafesi ile gerçek mm girilir; yalnız o çift uniform model oluşturur. Plaka akışında chessboard iç köşeleri veya simetrik daire merkezleri/adet/aralık girilir; tam grid bulunur, numaralı noktalar onaylanır, raw H hesaplanır. Fit RMS/inverse px istatistikleri bağımsız fiziksel hata değildir. Hull overlay korunur.
4. Son adımda ad otomatik önerilir ve değiştirilebilir; ID/sürüm kullanıcıdan istenmez. İsteğe bağlı başka bir mesafeyle kontrol gerçek yeni taramaları kullanır. Sınır girilmezse yalnız residual gösterilir; sahte kabul verilmez. Kaydet ve bitir yalnız aday kaydeder. Etkinleştirme ana sayfadadır ve actual source acquisition/config/resolution ile yeniden denetlenir; Runtime/worker/pending record/ACK kapısı korunur.

Geri girdileri korur. Yeni raster/kurulum, plaka tanımı veya yeniden arama ilgili taslak sonucunu temizler. Stop/İptal generation/ticket iptaliyle geç worker sonucunu bastırır; destructor worker sahipliğini bırakmaz. Kaydetme hatası pencereyi kapatmaz. Yeni proje veya restart seçim iznini geri getirmez; son seçim yalnız hatırlatılır.

## Kayıt ve kaynaklar

- CalibrationCapture application katmanı çalışan acquisition adapter'larını çağırır. Kaynak tool ID, dosya yolu ve decoded pixel kimliği korunur; yalnız ortak geometrik raw koordinat adı `image` olarak belirlenir. UI SDK/OpenCV çağırmaz.
- Projeye ait `<proje>.active-calibrations.json` KUI-1 katalog sözleşmesindedir. Yeni adlandırılmış wrapper schema2 `name` ve `recordedAtUtc` alanlarını içerik hash'ine katar; schema1 byte/field yolu korunur. İç uniform/manual/H codec ve engine execution/22 değişmez. Yeni sürümler yine ardışık olmalıdır.
- Yeniden kullanılabilen adlandırılmış plaka tanımları proje klasöründeki `calibration-targets.json` dosyasına CalibrationTargetLibrary ile atomik/lock kontrollü yazılır: strict schema1, 1 MiB/64 tanım. Bu dosya fiziksel cihaz kalibrasyonu veya aktif seçim değildir.
- Kaynak dosya ilişkilendirmesi kullanıcı beyanıdır; fiziksel kamera/lens kimliği otomatik doğrulanmaz. Aynı boyut tek başına etkinleştirme için yeterli değildir; kaynak adapter/config ve kurulum sözleşmesi karşılaştırılır.

## Doğrulama

Release `build/reference-core` ve `build/reference-ui` başarılı. İlgili core CTest **20/20 (12,20 sn)**; UI **23/23 (49,35 sn)**, UI shell/circle dahil. İlk sandbox UI shell koşumu Communication localhost timeout verdi; aynı test ağ erişimiyle tam ilgili koşumda geçti. Kapı gevşetilmedi. `kui4-release-ctest.log` / `kui4-release-tests.xml` iki build altında bulunur. Son UI-only kaynak düzeltmeleri ayrıca `kui4-final-ui-*` ile sınanır.

Yeni `ui_calibration_wizard` gerçek Qt mouse drag/click ile 400 px stripe/10 mm hesabını, bağımsız yeni taramadan kontrolü, dosya kökenini, named strict replay/tamper, actual acquisition sonrası aktivasyonu, busy gate, robot no-op, proje/restart, gerçek chessboard/circle rasterlarını, Stop/geç completion ve İptal'i sınar. Fixture ve PNG'ler build altındadır. 1400×900 ve 850×600 ana sayfa, bilinen ölçü ve iki plaka ekranı görselleri üretilip küçük ekranlarda metin/alt eylemler incelendi. Offscreen kabul native DPI/çoklu monitör kabulü değildir.

UI testinde düz circle rasterında OpenCV 5 SimpleBlobDetector son merkezi yanlış verdi. Yalnız tam ve eksene paralel düzenli grid için raster connected-component centroid yolu eklendi; adet/alan/yuvarlak biçim/row-column düzeni ve aynı H fit kapılarından geçer. Güçlü perspektif ret ve bağımsız KUI-3 testleri korunur. Bu geniş perspektif/kısmi grid/clutter desteği değildir.

## Sıradaki

**KUI-5:** ortak etkin modeli bütün metrik araçlar, Editor cache/TargetOnly, immutable Runtime, terminal/Logs/CSV ve eski px tolerans/world ROI etki incelemesine bağlamak. **KUI-6:** birleşik tam Release kabulü. Fiziksel K4, lens/robot, CUDA, kurulum/dağıtım ve T8.4.2b.3c bu teslimle kapanmaz.
