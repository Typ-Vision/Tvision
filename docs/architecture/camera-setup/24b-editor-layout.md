# CCUX-24B — kalıcı Flow ve sade sonuç alanları

Durum: tamamlandı — [kapanış](../../archive/2026-10-26-ccux-24b.md). Önce [ortak plan](24-usability-plan.md), contract ve
[analiz](24-usability-analysis.md) 1, 3–5 okunur. Sonraki kart: 24C.

## Hedef ve değişiklik alanı

Run Flow/Run Tool veya Results/Profil sekmesine geçişte sağdaki Flow listesi
görünür kalır. Sonuçlar iş değerlerini sunar; teknik kimlik satırları normal
sonuç panelinden kalkar. Measurement ve Circle/Line/Edge Pair Properties'teki
kalibrasyon özeti/aç düğmesi kaldırılır. Model geçerlilik kontrolü korunur.

Başlangıç dosyaları: `src/app/MainWindowEditor.cpp`, `MainWindowExecution.cpp`,
`MainWindowRuntime.cpp`, `MainWindow.cpp/.hpp`. Ortak kaynak özeti üreticisi
değişirse `src/application/ExperimentalRuntime.cpp` tüketicileri ayrıca doğrulanır.

## Uygulama adımları

1. Mevcut alt panel, selection bağlantıları, sonuç formatter tüketicileri ve
   yerel splitter ayarlarını doğrula; mevcut iki boyutta başlangıç görselini al.
2. Alt alanı yatay splitter'a ayır: sol Results/Profil/Bağlantılar sekmeleri,
   sağ tek Flow listesi. Eski Flow sekmesindeki DAG özeti ve Validate DAG
   “Bağlantılar”da kalsın. Aynı listeyi kopyalama veya ikinci selection modeli kurma.
3. Sağ listeye kullanılabilir minimum genişlik ver; panel daralınca üst viewer/
   Properties işlevlerini kaybetme. Yeni splitter için sürümlü yerel ayar kullan;
   eski/bozuk/sıfır genişlikli ayar güvenli başlangıca dönsün.
4. Tool ID ile seçim, scroll, Properties, ROI, klavye ve sağ tık sıralamayı koru.
   Çalıştırma yalnız sol Results sekmesini değiştirebilir.
5. Sonuç panelindeki Kaynak/Frame/Cycle kimlik satırlarını kaldır. Çok kaynak
   görüntü seçicisi, değer/birim, süre, execution durumu ve gerçek hata kalsın.
   Kalıcı metadata, Logs/CSV ve hata tanısı aynı veriyi taşımaya devam etsin.
6. Measurement kalibrasyon bloğu ve ortak metrologi bloğunu kaldır. Tolerans,
   input bağlantıları ve ölçüm servisi dokunulmaz; yeni Circle varsayılanı 24C'dir.

## Kabul matrisi

| Senaryo | Beklenen |
| --- | --- |
| Run Flow/Tool başarı, hata ve Stop | Flow görünür; seçim/scroll korunur; sahte sonuç yok |
| Results/Profil/Bağlantılar seçimi | Sağ liste kalır; DAG doğrulama erişilir |
| Tool seç/sırala/sil, klavye ile gezin | Properties ve ROI aynı tool ID'ye bağlı |
| Eski/yeni INI, restart ve reçete açma | Liste sıfır genişlikte veya ekran dışında kalmaz |
| İki kaynak ve çıktı görünürlük tercihi | Doğru görüntü seçilir; tercih gerçek sonuç/Logs'u değiştirmez |
| Kalibrasyonsuz metrik araç | Gereksiz panel kalksa da gerçek model hatası görünür; sahte mm yok |

İlgili testler: `ui_editor_usability`, `ui_shell_smoke`, `ui_runtime_visibility`,
`output_visibility_policy`. Liste görünürlüğü ve selection devamlılığı için
etkileşim senaryoları genişletilir. Logs/CSV kalıcılığı etkilenirse ilgili mevcut
regresyon da eklenir. Her iki pencere boyutunda okunabilirlik ve splitter kontrolü.

## Kapanış

Ortak planın Release/test/görsel/kapanış adımları zorunlu. Kanıt
`build/ccux/ccux-24b/`. Gereksinimlerin Editor/Runtime sunum bölümlerinde teknik
kimliklerin normal panelden çıkarıldığı, tanı/kayıtta korunduğu açıklanır.
Commit: `CCUX-24B: keep Flow visible and simplify result panels`.
