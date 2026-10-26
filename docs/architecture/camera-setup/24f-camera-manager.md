# CCUX-24F — görünür kurulum, geniş kamera önizlemesi

Durum: tamamlandı; [26 Ekim kapanışı](../../archive/2026-10-26-ccux-24f.md). [Ortak plan](24-usability-plan.md), contract
C1/C2/C2a ve [analiz](24-usability-analysis.md) 8 okunur. Sonraki: 24G.

## Hedef ve dosya haritası

Kamera listesi ve ayarlar solda, büyüyebilen önizleme sağda yer alır. Optik /
Geometri temel ayarlar yanında görünür olur. 24G'de Ayrıntılar'dan kaldırılacak
yönetim düğmelerinin uygun hedefleri bu kartta hazırlanır.

Başlangıç: `src/app/CameraManager.cpp/.hpp`, ortak `ImageViewer`, Camera Manager
test hedefinin CMake tanımı ve mevcut measurement workspace yönetim komutları.
Bu kart settings/edinim servislerini yeniden yazmaz; taşıma aynı application
komutlarını ve mutation/çalışan işlem korumalarını kullanır.

## Uygulama adımları

1. Eski kamera/kurulum yönetimi ile mevcut Camera Manager eylemlerini eşleştir.
   Ekle, düzenle, yeni kurulum, operasyonel seçim, kamera/kurulum arşivleme ve
   optik değişikliği bildir komutları için hedef konum ve eksik UI'ı belirle.
2. Sol ayar alanı scroll edebilsin; sağ viewer kalan alanda esnesin. Bütün sayfa
   scroll'unun preview'ı küçük bir şeride sıkıştırmasını kaldır. Splitter tercihi
   yerel INI'de saklanabilir; dar pencerede düğmeler erişilir kalmalı.
3. Ortak ImageViewer'ı preview için kullan; gerekiyorsa CMake test/library
   bağımlılığını ekle. Ölçekli pixmap'i raw frame yerine saklama. Gerçek raster,
   en-boy oranı ve frame metadata'sı korunur; ölçüm/ROI düzenleme modları kapalıdır.
4. Fit, actualSize, pan/zoom ve aynı geometride yeni karede görünüm koruma
   davranışı 24E ile aynı olsun. Kaynak değişimi eski görüntüyü yeni kamera
   görüntüsü gibi sunmasın; boş/hata/Stop durumları açık olsun.
5. Optik/Geometri kartında eksik/tamamlandı durumu, düzenleme ve optik değişimi
   birlikte erişilir olsun. “Taslak” yalnız pending durumunda kullanılır.
   Kamera fiziksel değişimini bildirmek mevcut invalidation komutunu çalıştırır.
6. Çok kurulum/profil yönetimi ile operasyonel seçimi anlaşılır düzenle. Tek
   varsayılanı gereksiz seçime zorlama; legacy belirsizliğini ilk öğeyle çözme.
   24G henüz uygulanmadığı için mevcut Kalibrasyon girişlerini bu kartta silme.

## Kabul

| Durum | Beklenen |
| --- | --- |
| Proje/kamera yok, kayıtlı offline kamera | Anlaşılır boş durum; yalnız ekran açmak cihaz açmaz |
| Eksik/tek/çok kurulum ve legacy belirsizliği | Doğru durum, görünür düzenleme, gerektiğinde açık seçim |
| Kamera A/B seçimi; ekle/düzenle/arşivle | Immutable bağlar korunur; diğer kamera bozulmaz |
| Exposure/gain kaydet ve ret | Gerçek readback kullanılır; başarısız işlem başarılı görünmez |
| Tek/canlı görüntü, zoom, yeniden boyutlandırma | Büyük preview, doğru aspect/raster, sınırlı tampon |
| Stop/çıkış/kopma ve lease çakışması | Drain/close korunur; eski completion yayınlanmaz |
| Optik değiştir, sonra eski değerlere dön | A→B→A eski ölçüm iznini canlandırmaz |

Testler: `ui_camera_manager`, `camera_settings`,
`camera_acquisition_sessions_contract`; taşınan komutların mevcut yönetim/
invalidation testleri kapsam doğrulamasıyla eklenir. 1400×900/850×600'de preview,
sol scroll, klavye odağı ve durum metinleri kontrol edilir. Fiziksel readback
yoksa fixture sonucu gerçek cihaz kabulü diye sunulmaz.

Kanıt `build/ccux/ccux-24f/`; 24G için işlev→hedef→test tablosu kapanışta bırakılır.
Ortak kapanış uygulanır. Commit: `CCUX-24F: enlarge preview and expose setup management`.
