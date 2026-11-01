# Makineyle birlikte yazılım teslimi — lisans değerlendirmesi

LIC-01 · 2 Kasım 2026 · Uygulayıcı: Codex

## Sonuç ve kapsam

VisionFlow'u ayrıca ücretlendirmeden satılan makineye kurup müşteriye teslim
etmek, üçüncü kişiye yazılım dağıtımıdır. Ücretsiz teslim lisans yükümlülüklerini
kaldırmaz. Mevcut bağımlılık seçimi, koşulları sağlanarak kapalı kaynak ürün
teslimine elverişli görünüyor; sırf makine satıldığı için ücretli Qt lisansı
zorunlu sonucuna varılmıyor. Bu, tamamlanmış ürün paketi için uygunluk onayı
veya bağlayıcı hukuk görüşü değildir. İlk teslim öncesi gerçek paket ve müşteri
sözleşmesi üzerinden hukuk incelemesi gerekir.

İnceleme: kök CMake, bağımlılık manifesti, OpenCV hazırlama betiği, yerel Qt
başlıkları/SHARED hedefi/Multimedia yapılandırması ve mevcut kamera bağlaması.
Nihai kurulum paketi, dosya bazlı tam SBOM ve SDK EULA'sı doğrulanmadı.

## Mevcut bileşenler

| Bileşen | Projede görülen kullanım | Değerlendirme |
| --- | --- | --- |
| Qt 6.12.0 Core, Gui, Widgets, Concurrent, Network, Multimedia | CMake ürün hedefleri; yerel modül başlıklarında LGPL-3.0 seçeneği; Qt Core SHARED IMPORTED | LGPL yolu mümkün. DLL bağlantısı tek başına bütün koşulları karşılamaz. |
| Qt Test | Test hedefleri | Ürün paketine test araçları eklenmedikçe geliştirme/test kapsamında kalır. |
| OpenCV 5.0.0 | core/imgproc/imgcodecs/objdetect/calib/geometry/features; shared hazırlama betiği | Apache-2.0. Kapalı kaynak dağıtım mümkün; codec ve diğer gömülü bileşen bildirimleri ayrıca korunmalı. |
| ZXing-C++ 2.3.0 | Seçenek açıkken statik ürün okuyucusu | Apache-2.0; statik kullanım kendi başına uygulamayı açmayı gerektirmez. Manifestteki libzueci ve UTF-8 DFA bildirimleri de teslim edilir. |
| Zint 2.15.0 | Manifestte yalnız bağımsız test fixture üreticisi; BSD backend, GPL frontend hariç | Mevcut ürün bağımlılığı olarak değerlendirilmez. Test klasörlerini topluca dağıtmamak gerekir. |
| GenTL.h | third_party/gentl | EMVA koşulları: değiştirmeden kullanım/dağıtım, telif/koşul/sorumluluk reddini koruma. Bu izin üretici SDK runtime'ına otomatik geçmez. |
| MinGW GCC 13.1 runtime | Manifest: libgcc_s_seh-1, libstdc++-6, libwinpthread-1 | GCC runtime exception kapalı kaynak uygulamaya izin verir; dağıtılan runtime'ın kendi lisans/kaynak koşulları sürer. winpthread lisansı ayrıca envantere alınır. |
| CMake, Ninja, Python, Qt geliştirme araçları | Hazırlama/derleme/test araçları | Aracı kullanmak uygulamayı o aracın lisansına dönüştürmez. Araçları müşteriye dağıtmak ayrı inceleme gerektirir. |
| OECS MvCameraControl.dll, sürücüler ve üretici GenTL .cti dosyaları | OECS DLL'si QLibrary ile dinamik yükleniyor; SDK başlıkları yerel | Yeniden dağıtım/cihaz kapsamı EULA veya üretici yazısı olmadan onaylanamaz. Dosya adı lisans kanıtı değildir. |

Kaynak kanıtı: [CMake](../CMakeLists.txt), [ZXing çözümlemesi](../cmake/VisionDependencies.cmake),
[manifest](../third_party/manifest.json), [GenTL koşulları](../third_party/gentl/license.txt),
[OpenCV betiği](../scripts/build-opencv.ps1), [OECS bağlaması](../src/hardware/OecsCApi.cpp),
[iş istasyonu](windows-workstation.md). Yerel Qt kanıtları:
`C:/Qt/6.12.0/mingw_64/include/{QtCore,QtGui,QtWidgets,QtConcurrent,QtNetwork,QtMultimedia}`,
`lib/cmake/Qt6Core/Qt6CoreTargets.cmake`, `config_qtmultimedia.summary` ve `bin/`.
Qt paketinde başka modüllerin kurulu olması bunların üründe kullanıldığını göstermez.

OpenCV 4.5 ve sonrası için [yayıncının lisans açıklaması](https://opencv.org/license/),
[Apache-2.0 yeniden dağıtım koşulları](https://www.apache.org/licenses/LICENSE-2.0),
[ZXing 2.3.0 lisansı](https://github.com/zxing-cpp/zxing-cpp/blob/v2.3.0/LICENSE),
[GCC runtime exception](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html) ve
[EMVA lisansı, bölüm 4](https://www.emva.org/wp-content/uploads/GenICam_License_20231009.pdf)
kontrol edildi.

## Qt LGPL ile kapalı kaynak teslim

Uygulama kodunun gizli kalabilmesi için kullanılacak yol:

1. Qt'yi uygun shared/DLL mekanizmasıyla bağla; arayüz uyumlu değiştirilmiş Qt
   sürümüyle çalışmayı engelleme. Sırf DLL dosyası bulunması yeterli kanıt değildir.
2. Kullanılan Qt'yi belirgin bildir; LGPLv3 ve GPLv3 metinlerini teslim et.
3. Dağıtılan Qt ikililerine karşılık gelen tam kaynak, değişiklik ve gerekli
   derleme girdileri için geçerli teslim yöntemi kur. Genel Qt ana sayfası
   bağlantısıyla yetinme. Pratik tercih kaynak arşivini teslim medyasına koymak;
   yazılı teklif seçilirse yasal süre ve erişim yükümlülüğünü işletmek.
4. EULA'da LGPL kapsamındaki değişiklikleri ve bu değişikliklerin hata ayıklaması
   için gereken tersine mühendisliği yasaklama. Bu izin uygulamanın bütün koduna
   sınırsız erişim hakkı anlamına gelmez.

Bu yol kendi VisionFlow C++ kaynaklarını genel erişime açmayı otomatik olarak
gerektirmez. Qt üzerinde yapılan değişikliklerin kaynağı ise ilgili lisansa
göre sağlanır. Statik Qt de kendiliğinden yasak değildir; yeniden bağlamaya
uygun uygulama nesne kodu/girdileri gibi ek yükümlülükler nedeniyle mevcut DLL
yolu daha kolay yönetilir. Bkz. [LGPLv3 §3–4](https://doc.qt.io/qt-6/lgpl.html) ve
[Qt'nin yükümlülük açıklaması](https://www.qt.io/development/open-source-lgpl-obligations).

**Endüstriyel makine ayrıntısı:** LGPLv3 §4(e)'deki kurulum bilgisi yükümlülüğü,
GPLv3 §6'daki “User Product” tanımına bağlıdır. Yalnız endüstriyel kullanımı
olan makine otomatik tüketici ürünü sayılmaz; genel amaçlı PC'nin makineyle
teslimi ayrıca değerlendirilmelidir. Buradan bütün makinelerde mutlaka yönetici
şifresi verilmesi gerektiği veya endüstriyel cihazlarda her türlü kilitlemenin
serbest olduğu sonucu çıkmaz. Shared mekanizma, sözleşme ve gerektiğinde
kurulum/çalıştırma koşulları birlikte incelenir. Bkz.
[GPLv3 §6](https://doc.qt.io/qt-6/gpl.html).

LGPL koşullarıyla bağdaşmayan kilitli kurulum veya GPL-only Qt modülü istenirse
uygun ticari lisans alternatifi değerlendirilir. Ticari sözleşmede cihaz/runtime
dağıtım hakkı ve kapsamı yazılı netleştirilmelidir; yalnız geliştirici lisansı
alınmasının her cihaz kullanımını kapsadığı varsayılmaz.
[Qt lisans seçenekleri](https://doc.qt.io/qt-6/licensing.html).

## Multimedia ve kamera SDK açıkları

OpenCV hazırlama betiğinde FFmpeg/GStreamer kapalı. Buna karşılık yerel
`config_qtmultimedia.summary` FFmpeg'i açık gösteriyor; Qt bin dizininde
`avcodec-63.dll`, `avformat-63.dll`, `avutil-61.dll` mevcut. Dolayısıyla OpenCV
ayarından bütün ürünün FFmpeg kullanmadığı sonucu çıkarılamaz. Bu bulgu
kurulum paketinde bu dosyaların bulunduğunu veya belirli bir codec'in çalışma
anında kullanıldığını kanıtlamaz.

Teslim edilen Qt Multimedia backend'i, FFmpeg yapılandırması ve ek kütüphaneler
belirlenmeli. GPL/nonfree özellikli rastgele FFmpeg paketleri eklenmemeli;
ilgili lisans, kaynak ve bildirimler tamamlanmalı. H.264/H.265 gibi codec'lerin
patent durumu, etkin özellikler ve satış ülkeleri üzerinden ayrıca ele alınır.
Qt ticari lisansı bu üçüncü taraf/patent yükümlülüklerini ortadan kaldırmaz.
[Qt Multimedia lisansları](https://doc.qt.io/qt-6/qtmultimedia-index.html),
[FFmpeg hukuk/lisans açıklaması](https://ffmpeg.org/legal.html).

OECS için kurulumdaki EULA/redistributable listesi henüz bulunup doğrulanmadı.
Üreticiden şu üç konu yazılı teyit edilmeli: makineyle runtime/sürücü dağıtımı,
izin verilen kamera modelleri/markaları ve dağıtılabilecek dosyaların tam listesi.
Resmî kurulum programı kullanılması da bu kullanım hakkının yerini tutmaz.
Basler/Daheng veya başka backend eklendiğinde kendi sözleşmesi ayrıca incelenir.

## İlk müşteri tesliminden önce

- Gerçek EXE/DLL/eklenti/codec/font/model/ikon listesini sürüm ve hash ile çıkar;
  kurulu geliştirme klasörünün tamamını kopyalama. Tam dosya bazlı lisans/SBOM
  denetimi bu değerlendirmenin devam işidir.
- Ürün bildirimleri ve lisans metinlerini teslim paketine ekle; Qt/FFmpeg ve
  dağıtılan runtime kaynaklarını seçilen geçerli yöntemle sağla.
- Temiz makinede kurulum ve arayüz uyumlu Qt DLL değiştirme kontrolü yap.
- SDK yeniden dağıtım hakkını, Windows makine/PC lisansını ve EULA'yı doğrula.
- Son paketi ve kilitleme tasarımını lisans uzmanına incelet. Mevcut geliştirme
  kabulü dağıtım kabulü değildir; lisans bakımından “teslime hazır” kararı yoktur.

Öneri: mevcut Qt DLL + LGPL yolunu aday olarak koru; Qt değişimi veya satın
alma kararını bu aşamada zorunlu sayma. Önce SDK izni ve gerçek paket envanterini
tamamla, sonra LGPL uyum maliyetiyle ticari lisans teklifini karşılaştır.
