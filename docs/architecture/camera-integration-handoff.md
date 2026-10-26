# Birleşik kamera sürümü — kullanım ve fiziksel devir

25 Ekim 2026. INT-01–09, yerel hedef dal `CameraIntegration`.
[INT planı](camera-integration-a0-e2-plan.md),
[yazılım/görsel kabul](camera-integration-int08-acceptance.md),
[kapanış ve başlatıcı kanıtı](../archive/2026-10-25-int-09.md).

Repo kökünde `./VisionFlow.cmd` veya Explorer çift tıklaması kullanılır.
Başlatıcı `scripts/run-local.ps1` üzerinden `build/reference-ui/visionflow.exe`
açar; DLL yolları yalnız çocuk süreçte ayarlanır. Eksik EXE hatası tam yolu ve
README derleme yönlendirmesini gösterir. `./VisionFlow.cmd -SmokeTest` offscreen
fixture testidir; fiziksel kamera testi değildir. Dal değiştirmek EXE derlemez;
başka kaynak değişikliğinde README Release build komutunu yeniden çalıştırın.
Bu eşleme yerel geliştirme ortamı içindir; temiz makine dağıtım kabulü değildir.

## Sözleşmeler ve aktarımın son eşlemesi

| Aktarılan kapsam | Güncel dayanak |
| --- | --- |
| Kamera merkezli GUI, 7×7/49 merkez, iki yöntem | [Kullanım](../camera-runtime-usage.md), [kalibrasyon](calibration-overview.md), [edinim ömrü](camera-setup/acquisition-sessions.md) |
| A1 tanı / C1 frozen retry ve ilk hata | [Tanı](runtime-diagnostics.md), [kurtarma](runtime-error-recovery.md) |
| B0–B4 exact kayıt/izin ve bütün-set Verify | [Scoped model](scoped-active-calibration.md), [kaynak bağı](scoped-calibration-integration.md), [Verify](source-verification.md) |
| D1–D4 yerel çıktı seçimi | [Görünürlük](output-visibility.md); payload/DAG/Logs/CSV/izinli PLC verisi filtreden bağımsız |
| E1 birleşik yazılım/görsel/performance | [INT-08 matrisi](camera-integration-int08-acceptance.md); eski E1 sonuçları ayrı tarihsel kanıt |
| E2 fiziksel restart | Aşağıdaki operatör kaydı; henüz kabul edilmedi |
| A2 / C2 | A2 kaldırılmıştır; C2 aralıklı süreklilik sorunu yeni rapor/failing regression koşuluyla ertelenmiştir, çözülmüş sayılmaz |

`build/int09/transfer-final-map.json`, özgün 64 yol/185 hunk için güncel dosya
hashlerini ve korunmuş Git karşılıklarını taşır. INT-02 envanteri ve INT-08
manifesti tarihsel kanıttır; yeniden yazılmaz. Snapshot içindeki
`runtime-calibration-output-acceptance.md`, `runtime-calibration-output-fix-plan.md`
ve `runtime-calibration-output-handover.md` eski tabanın kayıtları olarak
`230cc72` altında korunur (`git show 230cc72:docs/architecture/<dosya>`).
İçlerindeki “industrial grab yok” türü ifadeler eski kapsamı anlatır.
Güncel OECS yolu normal kamera komutlarıyla edinim yapar; native production
Unavailable ve experimental PLC/robot engeli devam eder.

## Fiziksel restart gözlemi için operatör kaydı

Mevcut kullanıcı projesi/kalibrasyonu değiştirilmeden önce gerçek hedefi kaydedin.
Fixture model/kimliklerini fiziksel projeye taşımayın. Kaynak/reçete belirtilmediği
için INT-09 gerçek proje seçmedi ve kamera çekimi başlatmadı.

| Kaydedilecek bilgi | Gerekli içerik |
| --- | --- |
| Sürüm | Aktif Git commit/dal, EXE tam yolu/SHA-256; `build/int09/manifest.json` ile karşılaştırma |
| Proje/reçete | Tam proje yolu, project/recipe UUID, version/schema/applicationType; kaydedilmiş immutable sürüm |
| Kaynaklar | Her source UUID/kind, kamera immutable ID/provider/vendor/model/seri, setup ID/revision, profil ID/revision |
| Geometri/optik | Raster, origin/ROI/binning/reverse/pixel format, gerçek lens/düzlem/ışık/odak |
| Model | Hedefe bağlı model ID/content/version, kapsam/köken ve kayıtlı seçim; 7×7 aralık ve yön teyidi |
| Ortam | Yerel INI, katalog/terminal yolları, trigger ve Communication/ACK durumu; sırf gözlem için donanım çıkışı açılmaz |

Eldeki cihaz beyanı Opto Engineering COE-050-M-USB-050-IR-C,
seri **02A2220000627**; Device ID=1 seri değildir. Kullanıcıdan gerçek proje/reçete
yolu ve yukarıdaki exact kaynak/model bilgisi gerekir; cihaz beyanı izin oluşturmaz.

1. Uygulamayı kapatıp aynı başlatıcıyla açın; proje/Load kamera veya Run başlatmamalı.
2. Metrik kaynakta kayıtlı model görünebilir; oturum izni kendiliğinden dönmemeli.
   **Kaynağı doğrula** seçin; gerçek frame/kimlik ve kapanış sonrası bütün-set
   Ready durumunu, kaynak hatalarını ve tanı kaydını saklayın. Fit/çevrim başlamamalı.
3. **Run → Tek çevrim** ile gerçek source/frame/cycle/model/birim ve kalıcı terminal
   kaydını Logs/CSV ile eşleyin. OECS NOT_EVALUATED ve PLC/robot engelini korumalı.
4. **Stop** gerçek drain beklemeli. Çıktı seçimi yalnız görünümü değiştirmeli;
   disk retry varsa aynı sonucu yeniden yazmalı, yeni çekim yapmamalı.
5. Log/PNG/CSV, gözlem tarihi/operatör ve gerçek sonucu `build/e2-evidence/`
   altında kaydedin. Başarısızlıkta ilk hata ve recovery bilgisi korunur.

Bağımsız mm referansı/tolerans/tekrarlanabilirlik, lens/kurulum, CCUX-15j/22f,
native OECS pending-command/G0–G4, PLC/robot fiziksel kabulü, MUI-04,
native DPI/çoklu monitör, CUDA ve dağıtım açıktır. Kullanıcının önceki plaka
kalibrasyon başarısı bağımsız mm veya bu restart gözlemi yerine geçmez.
