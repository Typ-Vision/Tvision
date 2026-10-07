# 6B — yerel kamera SDK kurulumu

## Güncel yeniden kontrol — 7 Ekim 2026, keşif bütçesi teslimi

Önceki kurulum/ABI kanıtları aşağıda korunur. Bu oturumda Galaxy GxGVTL.cti yeni toplam/scan bütçe ayrımıyla 1000 ve 3000 ms toplam bütçede, 250 ms scan cap ile DONE/0 cihaz verdi. JSON: build/6b-galaxy-gige-default-budget.json ve build/6b-galaxy-gige-budget.json. GALAXY_* Machine değişkenleri yalnız probe process scope'a alındı; global environment veya kurulum değiştirilmedi.

Önceki `C:/Program Files/Basler/pylon/Runtime/x64/ProducerGEV.cti` ve standart Basler klasörleri bu oturumda erişilebilir bulunmadı. Probe ERROR/eksik producer: build/6b-basler-gige-budget.json. SDK'nın kaldırıldığı veya taşındığı kanıtlanmadı; güncel Basler kurulum/yol kabulü açık. Aşağıdaki installer exit 0 geçmiş kanıtı güncel erişilebilirlik anlamına gelmez. Yeni indirme/kurulum yapılmadı. Fiziksel grab kabulü yok.

7 Ekim 2026, Windows x64. Kullanıcı bağlı kamera olmadığını, hedeflerin Basler ve Daheng olduğunu ve SDK indirme/kurulumunu açıkça yetkilendirdi. GigE bağlantı standardıdır; ayrı bir kamera markası olarak modellenmez. Mevcut Qt/OpenCV/MinGW toolchain değiştirilmedi.

## Kurulan paketler ve kaynakları

| Paket | Kurulum | Doğrulanan kapsam |
| --- | --- | --- |
| Basler pylon Runtime 26.09.2 | `C:/Program Files/Basler/pylon` | GigE ve USB runtime/sürücüler, x64/x86 GenTL producer, camera emulator transport. Installer exit 0, restart None. Tam Developer SDK değildir; PylonC DLL/header ve Development dizini bu kurulumda bulunmadı. |
| Daheng Galaxy SDK 2.6.2608.9131 | `C:/Program Files/Daheng Imaging/GalaxySDK` | Tam SDK: VC/C++ header ve samples, GxIAPI x64/x86, GenICam, GigE/USB/CXP GenTL ve SDK varsayılan sürücüleri. Installer exit 0; günlükte restart No. |

Basler Runtime resmi [pylon indirme sayfasından](https://www.baslerweb.com/en/downloads/software/3200606064/?downloadCategory.values.label.data=pylon) ve `downloads-ctf.baslerweb.com` bağlantısından alındı. Authenticode Valid, imzalayan Basler AG. Paket SHA-256: `520AB1CE28A32D1FF9D449D32565866AF2689E1A272C2377916C6EE6C3BEE7D7`.

Daheng'in [resmi sayfası](https://en.daheng-imaging.com/index.php?a=lists&c=index&catid=59&czxt=8&m=content) aynı sürümü listeler, ancak indirme login ister. Hesap oluşturulmadı. Galaxy paketi [VA Imaging müşteri indirme sayfasının](https://va-imaging.com/en-us/pages/customerdownloads) açık CSV katalog bağlantısından indirildi: `https://hs.va-imaging.com/hubfs/Supportshare/Daheng%20SDK/Galaxy%20Windows%20SDK_2.6.2608.zip`. Dış installer Authenticode imzasızdır; üretici yayınıyla hash eşitliği kanıtlanmadı. ZIP SHA-256: `B199617316A9CCC1BC56B00D655D6F80B919A7E56C915520023CEEFCDB1A5BEA`. EXE sürümü 2.6.2608.9131.

Installer'lar ve günlükler `build/sdk-install/` altındadır ve Git'e eklenmez. Galaxy Inno Setup `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP- /LOG=...` ile kuruldu. Basler `/quiet` denemesi Apply öncesinde `0x80070057: BA passed NULL hwndParent to Apply` hatasında takıldı; yalnız bu kurulumun süreçleri kapatılıp [desteklenen](https://docs.baslerweb.com/knowledge/redistributable-pylon-runtime-installation-options) `/passive /norestart` kipinde tamamlandı. Basler seçimi C++ runtime, GigE runtime/filter/producer ve USB runtime/driver/producer ile emulator transport idi. İstenen eski `CRuntime` feature adı C API dosyalarını sağlamadı; kurulu C API var diye kabul edilmez. Üretici zinciri CodeMeter bağımlılığını da kurdu. Galaxy varsayılan kurulumu sürücü, NIC jumbo ayar yardımcıları ve log servisini çalıştırdı; her alt çağrının exit 0 kaydı günlükte bulunur. İşletim sistemi yeniden başlatılmadı.

## MinGW ile gerçek SDK sınırı doğrulaması

- Galaxy C API: `build/sdk-install/galaxy-abi-probe.cpp` MinGW GCC 13.1/C++20 x64 ile derlendi. `GXInitLib`, `GXUpdateDeviceList(1000 ms)`, `GXCloseLib` gerçek kurulu GxIAPI DLL'inden dinamik çözüldü; üçünün sonucu **0**, cihaz sayısı **0**, probe exit **0**. Sonuç `galaxy-abi-probe.json`. Sadece init/discovery/close; configure/grab/trigger kabulü değildir.
- Galaxy `GxIAPI.h` GCC'de `enum GX_AUTO_CONNECTION_STATUS_ENTRY;` bildirimi nedeniyle derlenmiyor; `-fms-extensions` de çözmedi. Vendor dosyaları değiştirilmedi. Probe yalnız `GXErrorList.h` ve kurulu GXDef/GxIAPILegacy'deki dar C ABI function-pointer imzalarını kullanır. Bu kanıt MSVC C++ ABI uyumu anlamına gelmez.
- DLL araması için probe `LoadLibraryExW` ve `AddDllDirectory` ile Galaxy APIDll/Win64 ve GenICam/bin/Win64_x64 bağımlılıklarını kullanır. Yeni kurulum ortam değişkenleri mevcut Codex sürecinde yoktu; Galaxy init -1 verdi. Machine'deki `GALAXY_*` ve GenTL değişkenleri sadece probe sürecine alınca başarılı oldu. Global PATH elle değiştirilmedi.
- VisionFlow'un mevcut MinGW GenTL probe'u gerçek `GxU3VTL.cti` ve Basler `ProducerU3V.cti` ile **DONE**, boş cihaz listesi verdi. Basler DLL bağımlılıkları için sadece probe PATH'ine `C:/Program Files/Basler/pylon/Runtime/x64` eklendi. Sonuçlar `galaxy-usb-probe.json`, `basler-usb-probe.json`.
- Gerçek `GxGVTL.cti` (3000 ms) ve `ProducerGEV.cti` (10000 ms) yüklendi; keşif mevcut toplam deadline kontrolünde **ERROR / Kamera keşfi zaman aşımı** oldu. GenTL discovery kodunun vendor scan bekleme bütçesi ile toplam bütçeyi ayırması sonraki 6B işidir. Bu raporlar cihaz yok sonucunu başarılı discovery diye sunmaz; `galaxy-gige-probe.json`, `basler-gige-probe.json`.
- Release build güncel; tool_execution, acquisition_contract, owned_frame_contract, camera_lifecycle_contract ve camera_transport **5/5**. Ürün kaynakları/GUI değişmedi, yeni görsel ürün kabulü yapılmadı.

Machine `GENICAM_GENTL64_PATH` kurulumlar tarafından şu iki dizine ayarlandı:

```text
C:\Program Files\Daheng Imaging\GalaxySDK\GenTL\Win64
C:\Program Files\Basler\pylon\Runtime\x64\
```

Kurulum öncesinde açık terminal/Codex bu değişkenleri otomatik miras almaz. Yeni oturumda ortamı kontrol edin; gerekli SDK değişkenlerini process scope'a alın ve Basler runtime bağımlılıklarını process DLL arama yoluna ekleyin. Producer seçicisine doğrudan `.cti` yolu da girilebilir. SDK dosyaları repository içine kopyalanmaz.

## Açık işler

Basler tam Software Suite/Developer indirmesi zorunlu kişisel/şirket formu açtı. Ad/soyad, şirket, posta kodu/şehir, ülke, e-posta, görev ve bölüm uydurulmadı veya kullanıcı adına gönderilmedi. Form kullanıcıya açık bırakıldı; görsel `build/sdk-install/basler-developer-form.png`. Bu nedenle **Basler tam Developer SDK kurulumu henüz tamamlanmadı**. Runtime üretici dosyalarıyla genel GenTL yolu ilerleyebilir; Basler C API/header/örnek yolu için Developer teslimi gerekir.

6B'nin sonraki uygulaması: gerçek GigE keşif deadline davranışı ve dar vendor C ABI adapter seçimi; feature/trigger/stream/owned frame → mevcut DAG, lifecycle cleanup ve kamera yapılandırmasının kaydı. Bağlı kamera yok; fiziksel trigger/grab, unplug/reconnect, Stop, buffer/event cleanup ve üretim latency kabulü açık. Emulator transport dosyası bulunması emulated capture çalıştırıldığı anlamına gelmez. Başka marka GigE kameranın bu producer'larla desteklendiği de kanıtlanmadı.
