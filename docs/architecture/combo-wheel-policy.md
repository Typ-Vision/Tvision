# Combobox wheel politikası — EFR-02

`main.cpp`, QApplication oluşturulduktan sonra tek `ComboWheelPolicy` kurar.
Uygulama filtresi bütün QComboBox'ları ve çocuklarını olay geldiğinde tanır;
sonradan oluşturulan modal/dinamik kontroller için panel kaydı gerekmez.
Kapalı combo wheelEvent'i çağrılmaz. Uygun QAbstractScrollArea çubuğuna tek
iletim yapılır; içeride sınır varsa dış alana bakılır. Alan yoksa olay
ignore edilip filtrede tüketilir. İletilen olay accept edilir; Qt'nin tekrar
üst widget'a yayması engellenir. Açık popup sınırı alttaki formu kaydırmaz.

Açık popup'ta olay yalnız view scrollbar'ına gider; view seçimi ve combo
index'i wheel ile değiştirilmez. Angle delta Qt scrollbar adım/birikimini,
pixel delta piksel adımını kullanır. Yatay angle giriş scrollbar'ın dikey
wheel yoluna normalize edilir; sınır yönü pixel girişle aynı kalır. İletim
guard'ı filtrenin kendi ürettiği olayı yeniden işlemesini engeller.
Klavye, mouse seçimi, accessibility ve editable text olaylarına dokunulmaz.
Viewer/spinbox veya combo dışı alanlar filtreden geçer.

## Kaynak envanteri

28 Ekim kaynak taraması (`build/efr/efr-02-combo-inventory.txt`):

| Yüzey | Combo oluşturan dosyalar (src/app) |
| --- | --- |
| Workspace / Editor / Runtime | MainWindowWorkspace, MainWindowEditor, MainWindowRuntime, MainWindowTracking |
| Kamera / Setup / Calibration | CameraManager, CalibrationOverview, CalibrationWizard |
| Modal iletişim/değişken | MainWindowCommunication, MainWindowVariables, EndpointProfileEditor |
| Dinamik Properties | CodeReadEditor, EdgePointEditor, MetrologyEditor, PoseMatchEditor, PreprocessEditor, PublishDataEditor, PropertySections, ResultEditor, SetVariablesEditor, RoiEditor |

Envanter kaynak literal taramasıdır; graph completeness iddiası değildir.
Kamera routes/profiles, kalibrasyon ve Properties ayarları mevcut
currentIndexChanged/activated işleyicilerini kullanır. Wheel bu sinyalleri
üretmediğinden bu seçim kaynaklı yazma/readback/bağlantı yolu tetiklenmez.
Programatik seçim ve gerçek kullanıcı seçimi mevcut işleyicilere devam eder.

## Kabul matrisi

`ui_combo_wheel`, Qt olayları ile aşağıdaki sonuçları ölçer. Aynı test
offscreen ve Windows platformunda çalıştırıldı. Sinyal sayacı doğrudan ayar
yazma callback'ine bağlıdır; gerçek kamera veya disk ayar servisi fixture'ı değildir.

| Durum | Ölçülen sonuç |
| --- | --- |
| Odaklı/odaksız kapalı | index=5, text korunur; indexChanged/activated/yazma=0; en yakın scrollbar bir kez değişir, dış alan 0 kez |
| İç içe alan / iç sınır | İç alan sınırdayken dış scrollbar ilerler; combo değişmez |
| Dikey/yatay angle ve pixel | Doğru eksen kayar; pixel -17/-13 için scrollbar +17/+13 |
| Disabled / üst alan yok | index korunur, seçim sinyali yok |
| Editable çocuk line edit | Wheel metni değiştirmez; gerçek yazma `typed` metnini üretir |
| Açık liste scroll→Escape | Liste kayar, index/yazma değişmez; Escape index=5 tutar |
| Açık liste scroll→click | index=8, activated ve yazma birer kez |
| Açık liste scroll→ok/Enter | Açıkça odaklanan 8 satırından 9'a seçim; ikinci yazma/activated |
| Kapalı klavye oku | 9→10; üçüncü yazma |
| Sonradan oluşturulan modal | index=5, indexChanged=0 |
| Combo dışı regresyon | Spinbox wheel değeri ve normal scroll area hareketi sürer |

Görseller 1000×720 ve 650×480 form, popup bağlamı ve popup listesi için
`build/efr/efr-02/` ve `native-qt/` altındadır. Windows Qt koşumu sentetik
QWheelEvent kullanır; fiziksel mouse/trackpad ve OS input yönlendirme kabulü
açıktır. Kamera/readback fiziksel kabulü bu yazılım testiyle kapanmaz.
