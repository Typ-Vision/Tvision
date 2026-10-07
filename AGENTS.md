# VisionFlow çalışma sürekliliği

Her yeni oturumda önce `docs/progress.md`, `README.md` ve ilgili gereksinim belgelerini oku. Tamamlanmış adımları yeniden başlatma; progress dosyasındaki sıradaki adımdan devam et. Kullanıcının güncel yönlendirmesi önceliklidir.

Her tamamlanan adımda `docs/progress.md` dosyasına kapsam, değişen dosyalar, doğrulama sonuçları, bilinen sınırlamalar ve sonraki adımı kaydet. README ve mimari açıklamaları uygulanan durumla uyumlu tut. Hazırlık ekranlarını çalışan ürün özelliği gibi sunma.

C++20 / Qt Widgets / OpenCV; UI algoritma veya donanım SDK'sını doğrudan çağırmaz. Üç applicationType tek Recipe Editor / Runtime kullanır. Reçete, execution ve parça karar durumları ayrı kalır. Gerçek sonuç olmadan PASS, skor veya robot pose gösterme.

Yerel toolchain/build komutları README'dedir. UI değişikliklerinde Release derleme, ilgili CTest ve görsel kontrol yap. Makineye özel panel ayarları `config/local.ui.ini`; test ayarları ve görseller `build/` altında tutulur. Kurulum, dağıtım ve CUDA hızının doğrulandığını kanıtsız beyan etme.
