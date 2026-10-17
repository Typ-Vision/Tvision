# KMC-10c1 — Çok kaynaklı immutable Runtime ve completion çekirdeği

17 Ekim KMC-10c2/10d güncellemesi: [kalıcı Runtime, terminal7 ve N-kamera UI](multi-source-runtime-terminal.md) uygulandı. Aşağıdaki kapalı production/yalnız bellek ifadeleri bu belgenin önceki paket kapanış durumudur; Runtime editor9 bütün-set provider ile açılır. [Birleşik kabul/devir](multi-source-release-acceptance.md).

17 Ekim 2026. **Programatik, yalnız bellekte kabul**. `MultiSourceRuntimeService` kaynak setini kabul eder; ortak `RuntimeService::execute` worker'ı gerçek edinim yapar ve trusted completion üretir. Kullanıcının kalıcı Runtime/Production yolu editor9'u KMC-10c2 kabulüne kadar açıkça reddeder. Yeni kullanıcı ekranı veya üretim izni yoktur.

## Tek tutarlı kaynak/model seti

`MeasurementWorkspace::productionSources(recipe)` enabled required source kümesini strict DAG'den çözer. Her kaynak kendi exact sourceSetup bağına, acquisition tanımına ve gerekiyorsa scoped model/evidence'ına sahiptir. Source UUID anahtardır; dosya/raster içeriği, görünen ad ve CameraId aynı kavram değildir. Raw-only dal kullanılmayan bir kalibrasyonun aktivasyonunu istemez. Metric/world dal explicit kurulum ve aktif exact model gerektirir; global model fallback'i yoktur.

`ScopedActiveCalibrationService::snapshots` registry görünümünü, permission setini ve bütün gerekli model kayıtlarını bir batch içinde yakalar. Katalog en çok 4 MiB olarak okunur. Ön/son katalog SHA256, registry belgesinin tamamı ve permission seti karşılaştırılır. Aradaki değişim bütün okumayı reddeder; çağıran bütün admission'ı yeniden almalıdır. Bir kaynak eski, diğeri yeni seçimle sessizce kabul edilmez. Son registry karşılaştırması acquisition tanımlarıyla aynı epoch'u da denetler. Test için isteğe bağlı `beforeVerify` bariyeri registry/model/permission değişimini deterministik olarak araya sokar.

Bu optimistic tutarlı görünüm kontrolüdür; çok dosyalı transaction, cross-process çalışma lease'i veya power-loss kabulü değildir. Snapshot dönüşünden sonraki değişim eski immutable snapshot'ı değiştirmez; bir sonraki begin güncelliği yeniden denetler.

## Admission ve lease

`MultiSourceRuntimeService::prepare` önce bütün definition için mevcut 16 MiB snapshot sınırını, provider okumalarından önce denetler; sonra tek candidate set kurar. Bütün dal doğrulamaları başarılı olmadan aktif set değiştirilmez. Kaynak anahtarları tam olmalı; missing/foreign set ve farklı engine, source/model/acquisition scope reddedilir. Aynı fiziksel CameraId'nin iki acquisition tool üzerinden tekrar talebi konservatif olarak reddedilir; yeniden kullanım politikası sonraki UX işidir. Legacy raw webcam index'leri de aynı backend/index için iki gizli capture açmaz.

Her dal mevcut ProductionRecipeService validation kurallarından geçer. İç, tek kaynaklı schema8 projection yalnız bu doğrulama için kullanılır; reçete dosyası/schema9 definition değiştirilmez. Whitelist, backend/asset/reference/model/calibration, geometri ve tolerans kapıları atlanmaz. Üst ProductionSnapshot özgün editor9 reçeteyi ve bütün branch dependency setini taşır. İçerik kimliği aynı canonical recipe sözleşmesiyle, dependency kimliği bütün sourceSet ile hesaplanır.

Scope `multi_source_in_memory/1`'dir. Prepare Validate/Activate, begin Run yetkisini ve owner thread'i denetler. Begin setin tamamını yeniden alıp içerik/dependency kimliklerini karşılaştırır; değişimde yeniden prepare gerekir. Başlayan lease kaynak/model sürümlerini sabitler; admission sonrası seçim değişimi çalışan çevrimi değiştirmez. Aynı anda tek lease vardır. Begin eski completion'ı temizler. Servis yalnız aktif snapshot, devam eden lease ve son gerçek completion/failure'ı tutar; restart izin veya aktif çalışma geri yüklemez.

## Ortak worker ve trusted completion

`RuntimeService::execute` immutable `sourceSet` üzerinden frame.sourceId'ye uygun model/acquisition snapshot'ını kullanır; worker canlı registry/model seçimi okumaz. Gerçek dosya/Qt webcam acquisition ve KMC-10b sıralı süre/bellek/Stop kapıları korunur. N raw frame ortak cycle, farklı source/frame/coordinate kimlikleri taşır; N>1 compatibility frameId boştur. Aynı pikseller kaynak veya model paylaşımı anlamına gelmez.

`ProductionRecipeService::completionDigest` mevcut private lease/snapshot capability'sini bütün N frame, tool, typed port, metadata ve owned image content/processing kimlikleriyle mühürler. Yeni public completion imzalama API'si eklenmedi. Editor sonuçları, değiştirilmiş trusted sonuç, başka lease/ticket ve partial sonuç kabul edilmez.

Yeni private completion set doğrulaması önce tüm acquired frame indeksini ve her port kökenini SourceFrameIndex/ToolResultCodec ile denetler. DONE bütün required source ve tool sonuçlarını ister. Sonra mevcut ProductionRecipeService semantik replay kapılarını exact immutable dal snapshot'larıyla uygular: Measurement tolerans/model/kararı, typed Geometry replay, metrology, Circle ve reference sözleşmeleri korunur. İç replay için oluşturulan tek-source terminal nesneleri geçicidir; persist callback yoktur, çok-source terminal kaydı gibi yayımlanmaz.

ERROR'da edinilmiş kaynak alt kümesi olabilir; eksik kaynaktan sonuç çıkmaz. Bağımsız başarılı dalın gerçek tanısal ölçümü kalabilir; genel parça PASS/aggregate karar yoktur. Stop bütün frame/çıktı/overlay/kararları bastırır. Geç Stop yalnız değiştirilmemiş aynı lease completion'ını bastırıp yeniden mühürleyebilir. Worker completion üretmeden hata verirse `abandon(ticket, reason)` lease'i yalnız hata mesajıyla kapatır; frame/result uydurmaz. Reddedilmiş completion lease'i temizlemez; aynı gerçek sonuç acquisition yapmadan yeniden sunulabilir. Disk record failure/retry KMC-10c2 işidir.

## Kabul ve sınırlar

`multi_source_production_contract`: 2/10 kaynak × üç applicationType, gerçek farklı dosya rasterları ve farklı scoped modeller, same pixels/different source/model, registry/katalog/permission okuma çatışması ve bütün set retry, eksik/yabancı/swapped set ve engine, tekrarlı fiziksel hedef, yetki/owner thread/busy, admission sonrası model değişimi, sonraki begin ret/reprepare, wrong/missing/duplicate/swapped frame, partial ve değiştirilmiş/Editor/foreign lease completion, erken/geç Stop, partial acquisition ERROR, worker abandonment ve default persistent Runtime/schema9 kapısının kapalı kalması. Raw mixed file/webcam ve geç kare adapter fixture'dır; fiziksel kamera kabulü değildir.

Terminal schema, restart/Logs/CSV ve vf.socket2 için N-source disk kanıtı bu pakette eklenmedi. KMC-10c2 tamamlanmadan kullanıcıya çok-source production açılmaz. Runtime N-kamera UI KMC-10d, B birleşik kabul/devir KMC-10e'dir. Fiziksel kamera/seri sürekliliği/ölçüm/PLC/robot, native/KUI-6/T8.4.2b.3c/lens/CUDA/kurulum/dağıtım kabulü açık kalır. [Doğrulama ve devir](../progress.md).
