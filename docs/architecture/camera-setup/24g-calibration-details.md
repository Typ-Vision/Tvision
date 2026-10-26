# CCUX-24G — yalnız Ayrıntılar düğmelerini sadeleştirme

Durum: yazılım/görsel kabul tamamlandı; [kapanış](../../archive/2026-10-26-ccux-24g.md).
[Ortak plan](24-usability-plan.md), contract C3/C3a ve
[analiz](24-usability-analysis.md) 7'nin **güncel düzeltmesi** okunur. Sonraki: 24H.

## Dokunulmayacak çalışan akış

Kullanıcının 26 Ekim açık düzeltmesi: “Kalibre Et…” ile açılan ve devam eden
özellikler silinmeyecek. Bilinen ölçü, Kalibrasyon plakası, mevcut plaka türleri,
kamera/dosya kaynağı, görüntü alma, hesaplama, bağımsız kontrol ve Kaydet ve kullan
korunur. Oluşturma yöntemlerini 7×7'ye indirme; mevcut solver, şema veya eski
kayıt desteğini kaldırma. Bu kart sihirbaz refactor'u değildir.

Yöntem seçimindeki robot öğesi `calibrateRobot` görünür ve devre dışı kalır;
metni **Robot Kalibrasyonu (Sonradan devreye alınacak)** olur. Tıklama/klavye
etkinleştirmesi wizard, kamera edinimi veya robot işlemi başlatmaz. Ayrıntılar
altındaki ikinci robot düğmesi kaldırılır; bu iki konum karıştırılmaz.

## Ayrıntılar eylem kararları

| Mevcut öğe | Planlanan işlem / işlevin erişim yeri |
| --- | --- |
| Kamera / kurulum ekle | Ayrıntılar'dan kaldır; Camera Manager'daki yönetim |
| Kurulumu düzenle / Yeni ölçüm kurulumu | Ayrıntılar'dan kaldır; Camera Manager kurulum alanı |
| Cihazları keşfet | Kopyayı kaldır; Camera Manager keşif |
| Operasyonel seç | Ayrıntılar'dan kaldır; Camera Manager çok kurulum yönetimi |
| Bilinen ölçü / Kalibrasyon plakası | Yalnız Ayrıntılar kopyalarını kaldır; Kalibre Et seçenekleri aynen kalır |
| Dosyadan deneme başlat | Ayrı kopyayı kaldır; mevcut sihirbaz içi dosya seçimi kalır |
| Robot kalibrasyonu | Yalnız Ayrıntılar kopyasını kaldır; Kalibre Et'teki pasif seçenek kalır |
| Etkinleştir | Kalıcı genel düğmeyi kaldır; geçmişte seçili uygun kaydın koşullu “Bu kaydı kullan” eylemi |
| Devre dışı bırak | Ayrıntılar'dan çıkar; seçili etkin modelin durum alanında koşullu “Kalibrasyonu devre dışı bırak” |
| Eski kayıtları kurulumla eşle | Normal Ayrıntılar'dan kaldır; eksik eski bağ saptanınca koşullu uyumluluk akışı |
| Denemeyi bu kurulumla ilişkilendir | Normal Ayrıntılar'dan kaldır; mevcut dosya denemesi ilişkilendirmesine ihtiyaç halinde açık eylem |
| Kamerayı / Kurulumu arşivle | Ayrıntılar'dan kaldır; Camera Manager yönetimi |
| Optik / kamera konumu değişti | Ayrıntılar'dan kaldır; görünür Camera Manager Optik/Geometri kartı |
| Dışa aktar / boş projeye al / bağımlılıklarıyla farklı kaydet | Ayrıntılar'dan kaldır; proje yönetimi altında kalibrasyon bağımlılıkları eylemleri |
| Kurulum/profil seçicileri | Düğme değildir; tek varsayılanda sadeleştir, belirsiz/çoklu bağda açık seçim korunur |
| Kayıt/sürüm/seçim/kontrol tablosu | Salt okunur geçmiş korunur; seçili kayıt eylemi yalnız gerekli olduğunda sunulur |

Genel işlev taşıma kararları uygulama tasarımıdır; yeni özellik veya onay akışı
icat edilmez. Mevcut projeyi açmak otomatik migration/yazma/probe yapmaz.
Dosya kaynağına ait açık ilişkilendirme kamera edinim kanıtı üretmez. Proje
transferinde mevcut bağımlılık bütünlüğü, çakışma ve boş hedef kontrolleri korunur.

## Uygulama adımları

1. `src/app/CalibrationOverview.cpp/.hpp` üzerinde mevcut objectName/callback
   envanteri çıkar. Kaynakta `chooseMethod` ve `wizard` sınırını işaretle.
   Bu plan hazırlanırken constructor ve chooseMethod doğrudan doğrulandı;
   coverage metadata_match/no_recorded_issue idi, uygulamada tekrar doğrulanır.
2. Ana akışın Bilinen ölçü ve plaka seçenekleri için başlangıç etkileşim kaydını
   al. Dosya/live, bağımsız kontrol, SaveUse ve ret/iptal yollarını listele.
3. 24F kapanışındaki yönetim hedeflerini doğrula; proje transferi, geçmiş kaydı
   kullanma ve koşullu legacy/ilişkilendirme için aynı servis komutlarına erişim kur.
   Çalışan işlem/dirty proje/revision ve hatada eski seçim koruması devam etsin.
4. Tabloya göre yalnız Ayrıntılar kopyalarını kaldır. Servisleri/solver'ları
   unused sanarak silme. Kalibrasyon ekranının boş durum/yönlendirme metinlerini
   yeni erişim yollarına göre düzelt; kullanıcıyı kaldırılmış düğmeye gönderme.
5. Ayrıntılar geçmiş ve gerekli seçicilerden oluşabilir; checkbox'un kendisinin
   kaldırılması istenmedi. Ana sayfa seçili kamera/durum/Kalibre Et odağını korur.
6. Robot ana seçenek metnini güncelle; disable durumunu ve callback olmadığını
   davranışla doğrula. Genel yeni yöntem veya izin mekanizması ekleme.

## Kabul matrisi — koruma ve kaldırma birlikte

| Senaryo | Beklenen |
| --- | --- |
| Kalibre Et aç | Bilinen ölçü ve Kalibrasyon plakası erişilir; robot görünür/pasif |
| Bilinen ölçü canlı/dosya, hesapla, kontrol, Kaydet ve kullan | Önceki çalışan özellikler mevcut; uygun aday etkinleşir |
| Plaka mevcut türleri ve 7×7; canlı/dosya | Tür/ayar/hesap/overlay/kontrol/kayıt akışları korunur |
| SaveUse ret, stale revision, yazma hatası, iptal | Önceki etkin model korunur; bağlantı/drain sözleşmesi sürer |
| Ayrıntılar checkbox aç/kapat | Tekrarlanan yönetim/yöntem/robot/transfer düğmeleri yok; geçmiş okunabilir |
| Geçmiş kaydı kullan/devre dışı bırak | Doğru kurulum için mevcut gerçek probe/uyum işlemi; başarısızlıkta eski seçim korunur |
| Tek varsayılan/çoklu/legacy/profilsiz kayıt | Sessiz yanlış bağ yok; gerekli açık seçim ve uyumluluk erişilir |
| Import/export/Save As ve dosya ilişkilendirme | Yeni konumdan aynı veri/kimlik koruması; no-op eski kayıt bozulmaz |
| Robot öğesine tık/Enter/Space, ekran aç/restart | Kamera açma, robot bağlantısı, pose veya kayıt üretmez |

Mevcut testler: `ui_calibration_overview`, `ui_calibration_wizard`,
`ui_scoped_calibration_activation`, `measurement_setup_migration`,
`framed_calibration_plate_contract`, `calibration_camera_sessions_contract`.
`calibration_save_use` ve proje transfer testleri gerçek etkiye göre eklenir.
Ana yöntemlerin hâlâ çalıştığı olumlu testler, yalnız kaldırılan widget yokluğu
testlerinden daha önceliklidir. 1400×900/850×600 yöntem seçimi, Ayrıntılar ve
yeni yönetim erişimi görsel kontrol edilir.

## Kapanış ve sınır

Kanıt `build/ccux/ccux-24g/`. README kullanımında Bilinen ölçü/plaka kalır;
yalnız değişen yönetim erişim yolu güncellenir. Ortak kapanış uygulanır.
Erişimi taşınmamış gerçek işlev varsa kart tamamlanmış sayılmaz. Robotun ileride
devreye alınması ayrı gereksinim, tasarım ve fiziksel kabul kartı ister; bu
paketin sonraki otomatik adımı değildir.
Commit: `CCUX-24G: simplify details while preserving calibration workflows`.
