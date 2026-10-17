# VisionFlow çalışma sürekliliği

Her yeni oturumda `docs/progress.md` ve `README.md` dosyalarını oku; bu AGENTS ile birlikte kısa başlangıç paketidir. Kullanıcının güncel yönlendirmesi önceliklidir. Tamamlanmış adımları yeniden başlatma; güncel devam noktası progress'tedir.

Ardından seçili görev kartını ve görevle ilgili gereksinim/sözleşme bölümlerini oku. Büyük belgelerde önce `rg -n` ile başlık veya görev ID'sini bul, gerekli bölümü aç. Bütün gereksinim, mimari veya arşiv dosyalarını başlangıçta topluca okuma; belirsizlik, çelişki veya katmanlar arası değişiklik varsa okumayı bağımlılıklara genişlet. Eski kanıt gerektiğinde progress'teki arşiv dizininden ilgili kaydı bul. Tarihsel “sıradaki görev” ifadeleri güncel sıra değildir.

Her tamamlanan adımda progress'in güncel özetini güncelle; alta günlük ekleme. Kapsam, değişen dosyalar, doğrulama/kanıt yolları, bilinen sınırlamalar ve sonraki görevi `docs/archive/YYYY-MM-DD-gorev-id.md` kapanış kaydına yazıp progress ve `docs/archive/README.md` dizininden bağla. Yeni tarihte/görevde dosya oluştur; mevcut tarihsel kaydı değiştirme. Açık engelleri ve kalıcı kararları özetten kaybetme. Önceki test başarısını yeni koşum gibi sunma.

README giriş/kullanım/build rehberidir; gereksinimler ürün kurallarını, mimari belgeleri teknik sözleşmeleri taşır. Bunları yalnız ilgili davranış/sözleşme değiştiğinde güncelle; teslim paragraflarını birden çok dosyaya kopyalama. Hazırlık ekranlarını çalışan ürün özelliği gibi sunma.

UTF-8 boyut sınırları: progress 8.000, README 8.000, AGENTS 4.000; başlangıç toplamı en fazla 20.000 bayt. Kapanışta `python scripts/check-context-docs.py` çalıştır. Sınır aşılırsa kritik bilgiyi silmek yerine ayrıntıyı ilgili belgeye/arşive taşı ve bağlantısını koru.

C++20 / Qt Widgets / OpenCV; UI algoritma veya donanım SDK'sını doğrudan çağırmaz. Üç applicationType tek Recipe Editor / Runtime kullanır. Reçete, execution ve parça karar durumları ayrı kalır. Gerçek sonuç olmadan PASS, skor veya robot pose gösterme.

Yerel toolchain/build komutları README'dedir. UI değişikliklerinde Release derleme, ilgili CTest ve görsel kontrol yap. Makineye özel panel ayarları `config/local.ui.ini`; test ayarları ve görseller `build/` altında tutulur. Kurulum, dağıtım ve CUDA hızının doğrulandığını kanıtsız beyan etme.

## Ajanlar arası çalışma

Codex yönetici ajandır; Claude Code kullanıcının veya Codex'in atadığı alt görevleri uygular. Ajanlar paralel çalışmaz. Devir yalnız repo belgeleriyle yapılır; proje kararını ajana özel hafızaya yazma. Kapanış kaydına `Uygulayıcı: Codex` veya `Uygulayıcı: Claude Code` satırı ekle; her tamamlanan görevi görev ID'li tek commit olarak kaydet (`ICM-03a: …`). Push yalnız kullanıcı isteğiyle.
