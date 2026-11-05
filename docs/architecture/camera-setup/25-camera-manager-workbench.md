# CAM-UI-01 — Camera Manager iş istasyonu düzeni: uygulama planı

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Camera Manager'ı tek sütunlu formdan üç bölmeli iş istasyonuna taşımak — solda kamera kartları, ortada büyük önizleme + HUD, sağda sekmeli denetçi — işlev, koruma ve test sözleşmelerini bozmadan.

**Architecture:** `CameraManager` tek QWidget olarak kalır; numune `prototypes/newUI_prototypes/Camera/code.html` düzeninden ilham alır ama yalnız gerçekten okunabilen veriyi gösterir. `QTableWidget` (`cameraDevices`) veri kaynağı ve test erişim yüzeyi olarak korunur; kart görünümü bir `QStyledItemDelegate` ile çizilir, böylece `item(row,col)`, `selectRow`, `rowCount` çağrıları kırılmaz. Exposure/gain/kurulum/sürücü alanları sağdaki `QTabWidget` içinde üç sekmeye taşınır; objectName'ler ve durum metinleri aynen korunur.

**Tech Stack:** C++20, Qt 6 Widgets, mevcut `ImageViewer`, `resources/theme.qss`, mevcut CTest hedefleri.

**Spec:** Bu belgenin “Tasarım” bölümü + `prototypes/newUI_prototypes/Camera/DESIGN.md` (yalnız dil/yoğunluk referansı). Prototip görselleri: `currentOne.png` (mevcut), `screen.png` / `code.html` (hedef düzen).

## Tasarım

### Karar
Prototipin **yapısı** alınır, **verisi** alınmaz. Açık tema zorunlu değil; koyu `theme.qss` korunur, teal `#66e3c4` + mavi `#49a8ff` vurguları kullanılır.

```
┌ Kamera Yöneticisi ──────────────────────────────── (cameraTitle)
├ [Tara] [Ekle] [ID/adı kaydet]                     (cameraActionBar, 1. satır)
├ [Ayarları oku] [Uygula] [Tek] [Canlı] [Durdur] [Kaydet]  (2. satır)
├───────────────┬──────────────────────┬────────────┐
│ ara             │ cameraViewHeader     │ Temel      │
│ kamera kartları │ zoom strip           │ Kurulum    │
│ (cameraDevices) │ cameraPreviewImage   │ Sürücü     │
│ cameraProject   │ HUD (X/Y/piksel/zoom)│            │
│ cameraDetail    │ cameraPreviewStatus  │            │
├───────────────┴──────────────────────┴────────────┤
└ cameraStatus (durum satırı)
```

### Gerçek veri eşlemesi (yalnız bunlar gösterilir)
| Prototip öğesi | Kaynak |
|---|---|
| Kamera kartı adı/model/seri/transport | `rows_`, `MeasurementCamera`, `CameraDiscoveryEntry` |
| Kart durumu (Canlı/Hazır/Keşfedildi/Çevrimdışı) | `MeasurementWorkspace::availability` + `previewActive_` |
| Viewport, Sığdır/1:1/±, pan/zoom | Mevcut `ImageViewer` |
| HUD X/Y/piksel | `ImageViewer::onPixel` + `ImageFrame::pixelText` |
| HUD zoom | `ImageViewer::onZoom` / `zoomFactor()` |
| Üst bilgi mod/raster/FPS | `liveMode_`, `lastFrame_->image.size()`, `previewSession_.stats().fps` |
| Temel: exposure/gain + min–max/birim | `CameraSettingsSnapshot.exposure/gain` |
| Kurulum: profil/kurulum/optik/düzlem/operasyonel/arşiv | `MeasurementEnvironment` (mevcut düğmeler) |
| Sürücü: producer/runtime/timeout/rota | `CameraLocalConfiguration`, mevcut alanlar |

### Gösterilmeyecekler (veri yok, uydurma yasak)
IP adresi, sıcaklık, dropped frames, bant genişliği, PLC senkron hattı, görüntü işleme ms, mm/px, “3 Aktif / 4 Bulunan”, ROI/defect overlay, histogram (bu kartta yok), pixel format/FPS/trigger yazımı (backend sözleşmesi yok), “Tümünü Doğrula”.

### Korunacak sözleşmeler
- Bütün objectName'ler: `cameraManager`, `cameraSplitter`, `cameraSettingsScroll`, `cameraTitle`, `cameraStatus`, `cameraScan`, `cameraRegister`, `cameraRename`, `cameraProject`, `cameraDevices`, `cameraDetail`, `cameraShortId`, `cameraName`, `cameraAdvanced`, `cameraProducer`, `cameraChoose`, `cameraProducers`, `cameraProducerEnable/Disable/Remove`, `cameraRoutes`, `cameraProfiles`, `cameraProfilesLabel`, `cameraDefaultsSave`, `cameraProfileEdit`, `cameraSetupAdd`, `cameraSetupStatus`, `cameraSetupCard`, `cameraOperational`, `cameraArchiveSetup`, `cameraArchive`, `cameraOpticalChange`, `cameraRememberRoute`, `cameraExposure`, `cameraGain`, `cameraTimeout`, `cameraExperimentalRuntime`, `cameraBaslerRuntime`, `cameraRuntimeDependencies_*`, `cameraRuntimeStatus_*`, `cameraRuntimeChoose_*`, `cameraRuntimeSave_*`, `cameraSettingsRead`, `cameraApply`, `cameraPreview`, `cameraLive`, `cameraStop`, `cameraCheck`, `cameraReadback`, `cameraPreviewStatus`, `cameraSaveFrame`, `cameraFit`, `cameraActualSize`, `cameraZoomOut`, `cameraZoomIn`, `cameraPreviewImage`, `cameraProfileDialog`, `draft*`.
- Durum metinleri: `"Tek görüntü"`, `"Canlı"`, `"Sıralı tek kare"`, `"Continuous"`, `"kare"`, `"FPS"`, `"Görüntü alınamadı"`, `"doğrulandı ve kaydedildi"`, `"Sayısal"`, `"tamamlanmadı"`, `"Çevrimdışı"`, `"yerel rota seçili"`. Metin değiştirme.
- `findChild<QScrollArea*>()` ilk eşleşme `cameraSettingsScroll` olmak zorunda (testler niteliksiz çağırıyor). Yeni scroll alanları ondan **sonra** kurulur.

## Global Constraints

- C++20 / Qt 6 Widgets; yeni bağımlılık, yeni font/ikon/asset yok. Yeni renkler yalnız `theme.qss` içinde `#cameraManager` kapsamlı seçicilerle.
- UI SDK çağırmaz; mevcut `CameraSession`, `CameraSettingsSession`, `CameraPreviewSession`, `MeasurementWorkspace` komutları kullanılır.
- Gerçek olmayan telemetri, PASS/skor/pose, sahte “hazır” durumu yok.
- `CameraManager::controls()` içinde görünürlük (`isVisible`) kararı verilmez; yalnız `setEnabled`/`setText`. Sekmede gizli alanlar da programatik testte çalışır.
- Release derleme zorunlu: `build/reference-ui`. Testler `QT_QPA_PLATFORM=offscreen`.
- Her görev tek commit; mesaj `CAM-UI-01a: …` biçiminde; push yok.
- `docs/progress.md` şu an 8.000 bayt sınırında: ara görevlerde buraya satır **eklenmez**; devir özeti yalnız Task 6'da, yer açılarak yazılır. Ara görev notları commit mesajında ve `docs/archive/2026-11-05-cam-ui-01.md` taslağında tutulur.

## Review Focus

Aşağıdakiler kullanıcıyı en olası vuran durumlar; her biri ilgili görevin testinde sabitlenir.

1. **850×600 dar pencere:** üç panel + iki satır eylem çubuğu varken önizleme `height > h/2`, `width > w/3` eşiğinin altına düşmemeli.
2. **Sekmede gizli alan:** kullanıcı “Sürücü” sekmesindeyken `Ayarları oku` çalışmalı; `controls()` gizli widget'a dokunurken çökmemeli veya yanlış enable üretmemeli.
3. **Uzun model/seri adı (60+ karakter):** 210 px sol panelde yatay kaydırma çubuğu oluşmamalı, metin elide edilmeli.
4. **Çok satırlı durum metni:** `cameraStatus` `\n` içeren mesajlarda kırpılmadan okunabilmeli (sabit yükseklik yok).
5. **Yeniden tarama sırasında seçim:** kamera listesi yeniden kurulurken seçili kamera ve arama filtresi korunmalı; kart gizliyken `currentRow` bozulmamalı.
6. **Stop/çıkış sonrası:** HUD ve üst bilgi eski kareyi göstermeye devam etmemeli.

---

## Kontrol noktaları

| Nokta | Nerede | Ne görülür | Kapı |
|---|---|---|---|
| **C1 — kabuk** | Task 3 sonrası (Task 4) | Gerçek `VisionFlow.exe`; proje açmadan Kamera sayfası, üç panel, Temel/Kurulum/Sürücü sekmeleri, 1400×900 + 850×600 | Kullanıcı **ONAY** demeden Task 5 başlamaz |
| **C2 — kapanış** | Task 6 | HUD'lu tam sayfa, test görselleri, arşiv kaydı | Fiziksel kabul ayrı ve açık kalır |

C1 “işlevsiz” kabuktur: proje yok, kamera yok, hiçbir düğme cihaz açmaz, sekmeler gezinir. Bu amaçla ürün koduna demo bayrağı veya placeholder panel eklenmez.

## Dosya Yapısı

| Dosya | Sorumluluk |
|---|---|
| `src/app/CameraCardDelegate.hpp` / `.cpp` | (yeni) Kart çizimi ve satır yüksekliği; veri okumaz, `QModelIndex` kardeş sütunlarından okur |
| `src/app/CameraManager.hpp` | Yeni üye işaretçileri (`search_`, `filter_`, `inspector_`, `context_`, `viewHeader_`, `hudPixel_`, `hudZoom_`, `hudStats_`), `applyFilter()` |
| `src/app/CameraManager.cpp` | Sayfa iskeleti, panel kurulumu, filtre, HUD bağlantıları |
| `resources/theme.qss` | `#cameraManager` kapsamlı kart/çubuk/HUD/sekme stilleri |
| `tests/camera_manager_test.cpp` | Yeni kabul kontrolleri (kart, arama, çubuk, sekmeler, HUD) |
| `CMakeLists.txt` | Delegate kaynağı (ürün + `camera_manager_test`) |
| `docs/progress.md`, `docs/archive/2026-11-05-cam-ui-01.md` | Devir kaydı (son görev) |

---

### Task 1: Kamera kartı listesi ve arama

**Files:**
- Create: `src/app/CameraCardDelegate.hpp`, `src/app/CameraCardDelegate.cpp`
- Modify: `src/app/CameraManager.hpp`, `src/app/CameraManager.cpp:63-66,157-171,193-196`
- Modify: `CMakeLists.txt:191` (ürün) ve `CMakeLists.txt:771` (test hedefi)
- Modify: `resources/theme.qss` (sonunda `#cameraManager` bloğu)
- Test: `tests/camera_manager_test.cpp` (130. satırdan sonra)

**Interfaces:**
- Consumes: `QTableWidget cameraDevices` sütun sırası `{Kamera, Model, Seri/kimlik, Transport, Durum}`; `refresh()` dolduruyor.
- Produces: `visionflow::CameraCardDelegate` (`QStyledItemDelegate` alt sınıfı, 74 px `sizeHint`); `CameraManager::applyFilter()`; `QLineEdit* search_` (objectName `cameraSearch`); `QString filter_`.

- [ ] **Step 1: Delegate başlığını ve gövdesini yaz**

`src/app/CameraCardDelegate.hpp`:

```cpp
#pragma once
#include <QStyledItemDelegate>
namespace visionflow {
// Draws one camera card from the five data columns of the row; no model changes.
class CameraCardDelegate final:public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter*,const QStyleOptionViewItem&,const QModelIndex&) const override;
    QSize sizeHint(const QStyleOptionViewItem&,const QModelIndex&) const override;
};
}
```

`src/app/CameraCardDelegate.cpp` kuralları (gövdeyi implementer yazar):
- `sizeHint` → `QSize(0,74)`.
- `paint`: `option.rect.adjusted(2,2,-2,-2)` içine 8 px yuvarlatılmış kart. Renkler sabit: kart `#111720`, hover `#151d27`, seçili `#17314a`, kenar `#263242`, seçili kenar/vurgu `#66e3c4`, başlık metni `#edf4fb`, ikincil `#8fa0b5`.
- Seçili satırda sol kenara 3 px teal şerit.
- Satırlar: üstte `sibling(0)` kalın 12 px sağ üstte `sibling(3)` küçük rozet (yuvarlatılmış, `#263242` zemin); ortada `sibling(1)` 11 px; altta `sibling(2)` `Consolas` 10 px sol ve `sibling(4)` 10 px sağ.
- Her metin `QFontMetrics::elidedText(...,Qt::ElideRight,alanGenişliği)` ile kırpılır; taşma yok.
- `painter->save()/restore()`; `State_Selected` ve `State_MouseOver` dışında durum yok sayılır.

- [ ] **Step 2: CMake'e ekle**

`CMakeLists.txt:191` yanına `target_sources(visionflow PRIVATE src/app/CameraCardDelegate.cpp)`; `:771` satırını `add_executable(camera_manager_test tests/camera_manager_test.cpp src/app/CameraManager.cpp src/app/CameraCardDelegate.cpp)` yap.

- [ ] **Step 3: Tabloyu karta çevir ve aramayı ekle (`CameraManager.cpp`)**

`table_` kurulumunu (`:63-65`) şu hale getir (sütun sayısı ve başlık metinleri **korunur**):

```cpp
table_=new QTableWidget(0,5);table_->setObjectName("cameraDevices");
table_->setHorizontalHeaderLabels({"Kamera","Model","Seri / kimlik","Transport","Durum"});
table_->horizontalHeader()->hide();table_->verticalHeader()->hide();
table_->verticalHeader()->setDefaultSectionSize(74);
table_->setShowGrid(false);table_->setWordWrap(false);
table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
table_->setSelectionBehavior(QAbstractItemView::SelectRows);
table_->setSelectionMode(QAbstractItemView::SingleSelection);
table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
table_->setItemDelegate(new CameraCardDelegate(table_));
for(int col=1;col<5;++col)table_->setColumnHidden(col,true);
table_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
table_->setMinimumHeight(180);table_->setMaximumHeight(QWIDGETSIZE_MAX);
```

`search_` satırını tablodan **önce** ekle: `search_=new QLineEdit;search_->setObjectName("cameraSearch");search_->setPlaceholderText("Ad, model veya seri ara…");search_->setClearButtonEnabled(true);layout->addWidget(search_);` ve `connect(search_,&QLineEdit::textChanged,this,[this](const QString& t){filter_=t.trimmed();applyFilter();});`

Tabloyu yerleşimde esnet: `layout->addWidget(table_,1);` (eski `setMaximumHeight(210)` kaldırılır). `cameraProject` ve `cameraDetail` tablodan sonra kalır; Task 2'de `cameraProject` listenin en üstüne alınır.

`CameraManager::refresh()` sonunda (`select()` çağrısından hemen önce) `applyFilter();` ekle. Yeni metot:

```cpp
void CameraManager::applyFilter(){
    for(int i=0;i<rows_.size() && i<table_->rowCount();++i){
        bool match=filter_.isEmpty();
        for(int c=0;c<table_->columnCount() && !match;++c)
            if(auto* item=table_->item(i,c))match=item->text().contains(filter_,Qt::CaseInsensitive);
        table_->setRowHidden(i,!match);
    }
}
```

`applyFilter()` içinde `currentRow`/seçim değiştirilmez. `refresh()` zaten seçimi `selectedKey_` ile geri kuruyor; filtre gizlese bile seçim korunur.

`controls()` içine `search_->setEnabled(idle);` ekle.

- [ ] **Step 4: QSS bloğunu ekle (`resources/theme.qss` sonu)**

```css
QWidget#cameraManager { background: #0b0f14; }
QWidget#cameraHeader, QWidget#cameraActionBar { background: #0e141c; border-bottom: 1px solid #263242; }
QLabel#cameraTitle { font-size: 16px; font-weight: 600; }
QLabel#cameraContext { color: #8fa0b5; }
QLineEdit#cameraSearch { padding: 6px 9px; }
QTableWidget#cameraDevices { background: transparent; border: none; outline: none; }
QTableWidget#cameraDevices::item { border: none; padding: 0; }
QTabWidget#cameraInspector::pane { border: none; border-left: 1px solid #263242; }
QTabWidget#cameraInspector QTabBar::tab { padding: 7px 10px; }
QScrollArea#cameraInspectorScroll { background: transparent; border: none; }
QLabel#cameraViewHeader { background: #111720; border-bottom: 1px solid #263242; color: #8fa0b5; font-family: 'Consolas'; padding: 4px 8px; }
QWidget#cameraHudBar { background: #111720; border-top: 1px solid #263242; }
QLabel#cameraPixelHud, QLabel#cameraHudZoom, QLabel#cameraHudStats { color: #66e3c4; font-family: 'Consolas'; padding: 3px 8px; background: transparent; }
QLabel#cameraStatus { color: #b9c7d8; background: #0e141c; border-top: 1px solid #263242; padding: 5px 8px; }
QLabel#cameraSectionTitle { color: #66e3c4; font-weight: 600; }
```

- [ ] **Step 5: Kabul testlerini yaz (`tests/camera_manager_test.cpp`, satır 130 sonrası, `count=10` iken)**

```cpp
auto* cardTable=view.findChild<QTableWidget*>("cameraDevices");
check(cardTable->horizontalHeader()->isHidden(),"card list hides column headers");
check(cardTable->verticalHeader()->defaultSectionSize()==74,"card row height");
check(qobject_cast<CameraCardDelegate*>(cardTable->itemDelegate()),"card delegate installed");
for(int col=1;col<5;++col)check(cardTable->isColumnHidden(col),"card data column hidden");
check(cardTable->horizontalScrollBarPolicy()==Qt::ScrollBarAlwaysOff,"card list never scrolls sideways");
QPixmap cardShot(cardTable->viewport()->size());cardTable->viewport()->render(&cardShot);check(!cardShot.isNull(),"card list renders");
auto* search=view.findChild<QLineEdit*>("cameraSearch");
search->setText("no-such-camera");QApplication::processEvents();
for(int row=0;row<cardTable->rowCount();++row)check(cardTable->isRowHidden(row),"search hides non-matching cards");
search->clear();QApplication::processEvents();check(!cardTable->isRowHidden(0),"search reset restores cards");
search->setText(request().device.identity.serial);QApplication::processEvents();view.refresh();QApplication::processEvents();
check(cardTable->isRowHidden(1) && !cardTable->isRowHidden(0),"refresh keeps the active filter");search->clear();QApplication::processEvents();
view.resize(850,600);QApplication::processEvents();
check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"long identity fits narrow card list");
view.resize(1400,900);
```

Başlığa `#include "app/CameraCardDelegate.hpp"` ekle.

- [ ] **Step 6: Derle ve çalıştır**

```
cmake --build build/reference-ui -j 6 --target camera_manager_test
ctest --test-dir build/reference-ui -R ui_camera_manager --output-on-failure
```

Beklenen: PASS. Mevcut `cameraDevices` tabanlı kontroller (`item(row,2)`, `selectRow`, `rowCount`) değişmeden geçer.

- [ ] **Step 7: Commit**

```
git add src/app/CameraCardDelegate.hpp src/app/CameraCardDelegate.cpp src/app/CameraManager.hpp src/app/CameraManager.cpp resources/theme.qss tests/camera_manager_test.cpp CMakeLists.txt docs/progress.md
git commit -m "CAM-UI-01a: draw camera cards and filter the device list"
```

---

### Task 2: Sayfa iskeleti — başlık, iki satır eylem çubuğu, durum satırı

**Files:**
- Modify: `src/app/CameraManager.hpp` (yeni `QLabel* context_`)
- Modify: `src/app/CameraManager.cpp:46-50,70-77,126-133,141-153`
- Test: `tests/camera_manager_test.cpp`

**Interfaces:**
- Consumes: Task 1 sonrası mevcut widget'lar ve objectName'ler.
- Produces: `QWidget#cameraHeader`, `QWidget#cameraActionBar`, `QLabel#cameraStatus` alt şeritte, `QLabel#cameraContext`.

- [ ] **Step 1: Failing testi yaz (dosya sonundaki mevcut blokların önüne, `view.resize(1400,900);` sonrası)**

```cpp
check(view.findChild<QWidget*>("cameraHeader") && view.findChild<QWidget*>("cameraActionBar"),"page chrome present");
auto* statusStrip=view.findChild<QLabel*>("cameraStatus");
check(statusStrip && statusStrip->wordWrap() && statusStrip->maximumHeight()>1000,"status strip wraps multi-line messages without a height cap");
for(const auto size:{QSize(1400,900),QSize(850,600)}){
    view.resize(size);QApplication::processEvents();
    for(const auto& name:QStringList{"cameraScan","cameraRename","cameraSettingsRead","cameraApply","cameraPreview","cameraLive","cameraStop","cameraSaveFrame"}){
        auto* action=view.findChild<QPushButton*>(name);
        check(action && view.rect().contains(action->mapTo(&view,action->rect().center())),qPrintable("primary action reachable without scrolling: "+name));
    }
}
view.resize(1400,900);
```

- [ ] **Step 2: Çalıştır, FAIL gör** (`cameraHeader` yok)

- [ ] **Step 3: İskeleti kur (`CameraManager.cpp`)**

Kurucuda `outer` düzenini şu sırayla kur:

```cpp
auto* outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,0);outer->setSpacing(0);
auto* header=new QWidget;header->setObjectName("cameraHeader");
auto* hl=new QHBoxLayout(header);hl->setContentsMargins(10,7,10,7);
hl->addWidget(text("Kamera yöneticisi","cameraTitle"));
context_=text({},"cameraContext");hl->addWidget(context_,1);
outer->addWidget(header);
auto* bar=new QWidget;bar->setObjectName("cameraActionBar");
auto* bl=new QVBoxLayout(bar);bl->setContentsMargins(8,6,8,6);bl->setSpacing(6);
auto* row1=new QHBoxLayout;auto* row2=new QHBoxLayout;bl->addLayout(row1);bl->addLayout(row2);outer->addWidget(bar);
```

Eylem yerleşimi (objectName korunur, mantık değişmez):

| Satır | Düğmeler |
|---|---|
| row1 | `cameraScan`, `cameraRegister`, `cameraRename`, `row1->addStretch()` |
| row2 | `cameraSettingsRead`, `cameraApply`, `cameraPreview`, `cameraLive`, `cameraStop`, `cameraSaveFrame`, `row2->addStretch()` |

`cameraProject` action bar'a taşınmaz: mevcut yeri olan sol listenin **en üstünde** kalır (test `:280` onu `cameraSettingsScroll` viewport'unda kaydırmadan görmek istiyor). Görünürlüğü `controls()` içindeki `setVisible(!context_())` satırında kalır.

`status_` etiketi splitter'ın **altına** `outer->addWidget(status_)` ile alınır; `status_->setWordWrap(true)` zaten var, maksimum yükseklik verilmez.

`this` layout'unda artık yalnız `header`, `bar`, `split`, `status_` bulunur. `layout->addWidget(text("Kamera yönetimi"...))` ve eski `layout->addWidget(status_)` satırlarını kaldır; `cameraTitle` metni `"Kamera yöneticisi"` olur (yalnız boşluk farkı, objectName sabit).

`select()` içinde `context_->setText(...)` güncelle: seçili satır varsa `row.camera ? shortId + " • " + model : "Keşfedildi • kayıtlı değil"`, yoksa boş. Metin yalnız bilgi amaçlı; test bağımlılığı yok.

- [ ] **Step 4: Testi geçir, tüm `ui_camera_manager`'ı çalıştır**

```
cmake --build build/reference-ui -j 6 --target camera_manager_test
ctest --test-dir build/reference-ui -R ui_camera_manager --output-on-failure
```

Beklenen: PASS; `:277` satırındaki “small screen main action reachable by scrolling” kontrolü de artık doğrudan geçer.

- [ ] **Step 5: Commit**

```
git commit -am "CAM-UI-01b: add camera page header and persistent action bars"
```

---

### Task 3: Üç bölmeli splitter ve sekmeli denetçi

**Files:**
- Modify: `src/app/CameraManager.hpp` (`QTabWidget* inspector_`)
- Modify: `src/app/CameraManager.cpp:42-44,51-56,78-135,175,185-192,204-206`
- Test: `tests/camera_manager_test.cpp`

**Interfaces:**
- Consumes: Mevcut alanlar; `cameraAdvanced` (checkable `QGroupBox`) ve `advancedBody_` davranışı.
- Produces: `QTabWidget#cameraInspector` — sekme 0 `Temel`, 1 `Kurulum`, 2 `Sürücü`; sekmelerin içinde `QScrollArea#cameraInspectorScroll` (yalnız sekme 0).

- [ ] **Step 1: Failing testi yaz (Task 2 testinden hemen sonra)**

```cpp
auto* tabs=view.findChild<QTabWidget*>("cameraInspector");
check(tabs && tabs->count()==3,"inspector has three tabs");
check(tabs->tabText(0)=="Temel" && tabs->tabText(1)=="Kurulum" && tabs->tabText(2)=="Sürücü","inspector tab labels");
check(view.findChild<QScrollArea*>() && view.findChild<QScrollArea*>()->objectName()=="cameraSettingsScroll","first scroll area remains the left panel");
check(tabs->widget(0)->findChild<QLineEdit*>("cameraExposure") && tabs->widget(0)->findChild<QLabel*>("cameraReadback"),"base tab owns exposure/gain/readback");
check(tabs->widget(1)->findChild<QLabel*>("cameraSetupStatus") && tabs->widget(1)->findChild<QComboBox*>("cameraProfiles"),"setup tab owns setup management");
check(tabs->widget(2)->findChild<QGroupBox*>("cameraAdvanced") && tabs->widget(2)->findChild<QLineEdit*>("cameraExperimentalRuntime"),"driver tab owns producer/runtime");
tabs->setCurrentIndex(1);QApplication::processEvents();check(view.findChild<QLabel*>("cameraSetupStatus")->isVisible(),"setup tab content visible when selected");
tabs->setCurrentIndex(2);view.findChild<QGroupBox*>("cameraAdvanced")->setChecked(true);QApplication::processEvents();
check(view.findChild<QLineEdit*>("cameraExperimentalRuntime")->isVisible(),"advanced body visible inside driver tab");
tabs->setCurrentIndex(0);
view.resize(850,600);QApplication::processEvents();check(view.findChild<QScrollArea*>()->horizontalScrollBar()->maximum()==0,"narrow inspector keeps left panel overflow-free");view.resize(1400,900);
```

- [ ] **Step 2: Çalıştır, FAIL gör**

- [ ] **Step 3: Panelleri kur**

`split` üç pane alır; sıralama **korunur** (settings scroll önce):

```cpp
auto* scroll=new QScrollArea;scroll->setObjectName("cameraSettingsScroll");scroll->setWidgetResizable(true);
scroll->setMinimumWidth(210);split->addWidget(scroll);
split->addWidget(previewBody);                    // mevcut
inspector_=new QTabWidget;inspector_->setObjectName("cameraInspector");inspector_->setMinimumWidth(300);
split->addWidget(inspector_);
split->setStretchFactor(0,0);split->setStretchFactor(1,1);split->setStretchFactor(2,0);
split->setSizes({250,760,330});
```

`previewBody` şu an `:44`'te oluşturuluyor; kesme sırası `scroll` → `previewBody` → `inspector_` olmalı. Denetçi scroll'u (`cameraInspectorScroll`) en sonda oluşturulur.

Sekme 0 `Temel` (QScrollArea `cameraInspectorScroll` içinde): `cameraReadback`, `cameraExposure` + yeni `QLabel#cameraExposureRange`, `cameraGain` + `QLabel#cameraGainRange`, `cameraShortId`, `cameraName`, `cameraDetail` (wordWrap), `cameraSectionTitle` başlıkları (`"Pozlama", "Kazanç", "Kimlik", "Readback"`).
Sekme 1 `Kurulum`: `cameraSetupCard` içeriği (`cameraSetupStatus`, `cameraProfiles`, `cameraDefaultsSave`, `cameraProfileEdit`, `cameraSetupAdd`, `cameraOperational`, `cameraArchiveSetup`, `cameraArchive`, `cameraOpticalChange`) — mevcut bağlantılar taşınır, yeniden yazılmaz.
Sekme 2 `Sürücü`: `cameraAdvanced` grubu (producer, producers, runtime alanları, `cameraRoutes`, `cameraTimeout`, `cameraRememberRoute`, `cameraCheck`) ve `advancedBody_` toggle davranışı aynen.

Taşıma listesi (kopya oluşturma, taşı):
- `exposure_`, `gain_` → Temel (eski `form` satırlarından çıkar).
- `readback_` → Temel (eski `layout` satırından çıkar).
- `shortId_`, `name_`, `detail_` → Temel.
- `setupCard`/`setupStatus_`, `profiles_`, `defaults`, `draft`, `cameraOperational`, `cameraArchiveSetup`, `cameraArchive`, `cameraOpticalChange` → Kurulum.
- `producers`, `producer_`, `choose`, producer enable/disable/remove, runtime alanları, `timeout_`, `runtime_`, `routes_`, `cameraRememberRoute`, `check_` → Sürücü (`cameraAdvanced` içinde).

`settingsRead_`/`apply_` Task 2'de action bar'da; sekmede kopyası oluşturulmaz.

`settingsReadback()` sonuna aralık etiketleri ekle:

```cpp
auto range=[](const CameraFeature& f){return f.readable()?QString("[%1 – %2 %3]").arg(f.minimum).arg(f.maximum).arg(f.unit):QString("Desteklenmiyor");};
findChild<QLabel*>("cameraExposureRange")->setText(range(s.exposure));
findChild<QLabel*>("cameraGainRange")->setText(range(s.gain));
```

`select()`/`clearFrame()` sırasında bu iki etiket boşaltılır (`""`), çünkü değer bilinmiyor.

- [ ] **Step 4: Testleri geçir**

```
cmake --build build/reference-ui -j 6 --target camera_manager_test
ctest --test-dir build/reference-ui -R ui_camera_manager --output-on-failure
```

Beklenen: PASS. Özellikle `:117-118` (runtime kontrolü gizli sekmede), `:150,164,307,312` (`cameraAdvanced` toggle), `:231-232` (enable mantığı), `:284` (legacy profil seçimi) geçmeli.

- [ ] **Step 5: Commit**

```
git commit -am "CAM-UI-01c: split the camera page into three panes with a tabbed inspector"
```

---

### Task 4: Ara kontrol noktası C1 — gerçek EXE ile kabuk incelemesi (onay kapısı)

**Files:**
- Create: `build/camera-shell-review.md` (inceleme notları; `build/` altında, commit edilmez)
- Create: `build/camera-shell/` (ekran görüntüleri; commit edilmez)
- Modify: `docs/architecture/camera-setup/25-camera-manager-workbench.md` (yalnız “Onay” bölümü)

**Interfaces:**
- Consumes: Task 1–3 (kart listesi, başlık/eylem çubukları, üç panel + Temel/Kurulum/Sürücü sekmeleri).
- Produces: insan onayı. Onay gelmeden Task 5 başlamaz. Onay notu ve varsa düzeltme listesi bu dosyaya yazılır.

Bu görevde **yeni ürün kodu yazılmaz**. Amaç, gerçek `VisionFlow.exe` ile kabuğu gözle görmek. Sahte demo modu, placeholder panel veya `--camera-shell` bayrağı **eklenmez**: mevcut widget'lar taşındığında sinyal bağlantıları kendiliğinden korunur; placeholder kurup sonra gerçek paneli koymak iki kat iş olur ve uydurma veri üretir.

Boş durum zaten “işlevsiz”dir: proje açılmadan kamera sayfası yalnız `cameraProject` yönlendirmesini, boş kart listesini ve üç sekmeyi gösterir; hiçbir düğme cihaz açmaz.

- [ ] **Step 1: Ürünü derle**

```powershell
cmake --build build/reference-ui -j 6 --target visionflow
```

Beklenen: hata yok.

- [ ] **Step 2: EXE'yi başlat ve sekmeleri gez**

```powershell
.\VisionFlow.cmd
```

Yapılacaklar (proje açma, cihaz tarama, ayar okuma **yok**):
1. Sol menüden **System → Camera Manager**.
2. Sağ denetçide sırayla **Temel → Kurulum → Sürücü** sekmelerine geç; her sekmenin içeriği görünüyor mu?
3. Sol kart listesine tıkla; seçim görsel olarak değişiyor mu (kamera yoksa boş kalır)?
4. Pencereyi 850×600'e küçült; üç panel, iki eylem satırı ve durum satırı okunur mu, önizleme alanı ezilmiş mi?
5. Pencereyi 1400×900'e büyüt; tekrar kontrol et.

Beklenen: sekmeler arasında gezinilir, hiçbir panel boş/çökmüş görünmez, hiçbir düğme cihaz açmaz, durum satırı “Henüz arama yapılmadı…” veya proje yönlendirmesini gösterir.

- [ ] **Step 3: Kanıt al**

Her iki boyutta üç sekme için ekran görüntüsü al (`Win+Shift+S`), `build/camera-shell/` altına `temel-1400.png`, `kurulum-1400.png`, `surucu-1400.png`, `temel-850.png`, `kurulum-850.png`, `surucu-850.png` adlarıyla kaydet. Altına `proje-yok.png` ekle.

- [ ] **Step 4: İnceleme notunu yaz**

`build/camera-shell-review.md`:

```text
# CAM-UI-01 C1 kabuk incelemesi
Tarih:
Build: build/reference-ui
Karar: ONAY | DÜZELTME
Düzeltilecekler (görev no + tek satır):
-
Görseller: build/camera-shell/
```

- [ ] **Step 5: Onay kapısı**

Kullanıcı **ONAY** demeden Task 5'e geçilmez. **DÜZELTME** gelirse ilgili Task 1–3 adımına dönülür, düzeltme aynı görev numarasıyla tek commit olur, C1 tekrar edilir. Plan dosyasındaki bu bölümün sonuna `Onay: <tarih> — <karar>` satırı eklenir.

- [ ] **Step 6: Commit (yalnız onay satırı)**

```bash
git add docs/architecture/camera-setup/25-camera-manager-workbench.md
git commit -m "CAM-UI-01d: record camera shell review approval"
```

---

### Task 5: Viewport üst bilgisi ve piksel/zoom HUD'ı

**Files:**
- Modify: `src/app/CameraManager.hpp` (`QLabel *viewHeader_,*hudPixel_,*hudZoom_,*hudStats_`)
- Modify: `src/app/CameraManager.cpp` (previewBody kurulumu, `clearFrame()`, `pollJobs()`, `select()`)
- Test: `tests/camera_manager_test.cpp`

**Interfaces:**
- Consumes: `lastFrame_->image.pixelText(int,int)`, `lastFrame_->image.size()`, `previewSession_.stats()`, `ImageViewer::onPixel/onZoom`.
- Produces: `QLabel#cameraViewHeader`, `QLabel#cameraPixelHud`, `QLabel#cameraHudZoom`, `QLabel#cameraHudStats`, `QWidget#cameraHudBar`.

- [ ] **Step 1: Failing testi yaz (Task 3 testinden hemen sonra)**

```cpp
auto* hudPixel=view.findChild<QLabel*>("cameraPixelHud");
auto* hudZoom=view.findChild<QLabel*>("cameraHudZoom");
auto* viewHeader=view.findChild<QLabel*>("cameraViewHeader");
check(hudPixel && hudZoom && viewHeader,"viewport header and HUD present");
check(viewHeader->text().contains("Görüntü yok"),"header starts empty");
check(hudPixel->text().contains("Piksel:"),"HUD exposes pixel value field");
```

Ve mevcut tek görüntü bloğunda, `image` tanımlandıktan sonra (`:194` “same source single recapture preserves manual zoom” kontrolünün hemen altına) ekle:

```cpp
check(viewHeader->text().contains("Tek görüntü") && viewHeader->text().contains("3×2"),qPrintable(viewHeader->text()));
image->onPixel(QPoint(1,1));QApplication::processEvents();
check(hudPixel->text().contains("X: 1 px") && hudPixel->text().contains("Piksel:"),qPrintable(hudPixel->text()));
image->zoomBy(2.);QApplication::processEvents();check(hudZoom->text().contains("Zoom:"),qPrintable(hudZoom->text()));
```

Ve `view.stop();` sonrası (`:196`) ekle:

```cpp
check(viewHeader->text().contains("Görüntü yok") && hudPixel->text().contains("X: –"),"Stop clears viewport header and HUD");
```

- [ ] **Step 2: Çalıştır, FAIL gör**

- [ ] **Step 3: Görünümü ve bağlantıları yaz**

`previewBody` düzeni:

```cpp
viewHeader_=new QLabel("Görüntü yok");viewHeader_->setObjectName("cameraViewHeader");pv->addWidget(viewHeader_);
auto* zoom=new QHBoxLayout;pv->addLayout(zoom);
// mevcut cameraFit/cameraActualSize/cameraZoomOut/cameraZoomIn düğmeleri buraya
pv->addWidget(image_,1);
auto* hudBar=new QWidget;hudBar->setObjectName("cameraHudBar");
auto* hud=new QHBoxLayout(hudBar);hud->setContentsMargins(0,0,0,0);
hudPixel_=new QLabel("X: –    Y: –    Piksel: –");hudPixel_->setObjectName("cameraPixelHud");
hudZoom_=new QLabel("Zoom: 100.0%");hudZoom_->setObjectName("cameraHudZoom");
hudStats_=new QLabel;hudStats_->setObjectName("cameraHudStats");
hud->addWidget(hudPixel_,1);hud->addWidget(hudZoom_);hud->addWidget(hudStats_);
pv->addWidget(hudBar);
pv->addWidget(previewStatus_);
```

Bağlantılar (kurucuda, `image_` oluşturulduktan sonra):

```cpp
image_->onPixel=[this](QPoint p){
    hudPixel_->setText(QString("X: %1 px    Y: %2 px    Piksel: %3").arg(p.x()).arg(p.y())
        .arg(lastFrame_?lastFrame_->image.pixelText(p.x(),p.y()):QString("–")));
};
image_->onZoom=[this](double z){hudZoom_->setText(QString("Zoom: %1%").arg(z*100,0,'f',1));};
```

`pollJobs()` kare dalında, `const auto stats=previewSession_.stats();` satırından sonra:

```cpp
const auto size=lastFrame_->image.size();
viewHeader_->setText(QString("%1 • %2×%3 • %4 FPS").arg(liveMode_?"Canlı":"Tek görüntü")
    .arg(size.width()).arg(size.height()).arg(stats.fps,0,'f',1));
hudStats_->setText(QString("%1 kare").arg(stats.completed));
```

`clearFrame()` içine: `viewHeader_->setText("Görüntü yok");hudPixel_->setText("X: –    Y: –    Piksel: –");hudStats_->clear();` (zoom değeri korunur; görünüm sıfırlanmıyor).

- [ ] **Step 4: Testleri geçir**

```
cmake --build build/reference-ui -j 6 --target camera_manager_test
ctest --test-dir build/reference-ui -R ui_camera_manager --output-on-failure
```

Beklenen: PASS; `:194` manuel zoom korunumu ve `:201-204` canlı metinleri bozulmadan geçer.

- [ ] **Step 5: Commit**

```
git commit -am "CAM-UI-01e: add viewport header and pixel/zoom HUD"
```

---

### Task 6: Release regresyonu, görsel kanıt ve devir kaydı

**Files:**
- Modify: `docs/progress.md`
- Create: `docs/archive/2026-11-05-cam-ui-01.md`
- Modify: `docs/archive/README.md` (dizin bağlantısı)

**Interfaces:**
- Consumes: Task 1–5.
- Produces: kapanış kanıtı ve devir özeti.

- [ ] **Step 1: Tam Release derlemesi**

```
cmake --build build/reference-ui -j 6
```

Beklenen: hata yok, uyarı artışı yok.

- [ ] **Step 2: UI testlerini çalıştır**

```
ctest --test-dir build/reference-ui -R "ui_camera_manager|ui_shell_smoke|ui_calibration_overview|ui_calibration_wizard" --output-on-failure
```

Beklenen: hepsi PASS. `ui_shell_smoke` `MainWindow::verifyCamera` yolunu kullanır; `cameraDevices`/`cameraStatus`/`cameraProducer` objectName'leri korunduğu için geçmeli.

- [ ] **Step 3: Görselleri üret ve incele**

```
$env:VISIONFLOW_TEST_ARTIFACTS="D:/TVision/build/ccux/ccux-25-camera-workbench"
ctest --test-dir build/reference-ui -R ui_camera_manager --output-on-failure
```

İncelenecek dosyalar (testin ürettiği adlarla): `camera-0-normal.png`, `camera-10-small.png`, `camera-preview.png`, `live-0-1400.png`, `live-0-850.png`, `camera-small-actions.png`, `camera-no-project.png`, `camera-ambiguous.png`, `camera-legacy-ambiguous.png`. `product-settings-850.png` yalnız OECS SDK derlenmişse üretilir; yoksa eksikliği sınırlama olarak kayda yaz.

Kontrol listesi: kartlarda taşma yok; seçili kart teal şeritli; üç panel 850×600'de okunur; önizleme yarıdan büyük; HUD kırpılmıyor; durum satırı çok satırlı metinde tam görünüyor; hiçbir uydurma telemetri yok.

- [ ] **Step 4: Devir kaydını yaz**

`docs/progress.md` “Devam noktası”na tek satır: `CAM-UI-01: Camera Manager üç bölmeli düzen; fiziksel kabul açık.` (bağlantı: `archive/2026-11-05-cam-ui-01.md`). **progress şu an tam 8.000 bayt sınırında**; satırı eklemeden önce eşdeğer ayrıntıyı arşive taşıyıp yer aç, sonra `check-context-docs.py` ile doğrula.

`docs/archive/2026-11-05-cam-ui-01.md` içeriği: kapsam, değişen dosyalar, doğrulama komutları ve sonuçları, görsel kanıt dizini, bilinen sınırlamalar (histogram yok; pixel format/FPS/trigger yazımı yok; fiziksel readback koşulmadı), sonraki görev önerisi. `Uygulayıcı:` satırı zorunlu. `docs/archive/README.md` dizinine bağlantı ekle.

- [ ] **Step 5: Belge sınırlarını doğrula**

```
python scripts/check-context-docs.py
```

Beklenen: PASS (progress 8.000, README 8.000, AGENTS 4.000 bayt).

- [ ] **Step 6: Commit**

```
git add docs/progress.md docs/archive/2026-11-05-cam-ui-01.md docs/archive/README.md
git commit -m "CAM-UI-01f: record camera workbench acceptance"
```

---

## Kabul matrisi

| Durum | Beklenen | Kanıt |
|---|---|---|
| 0/1/5/10 kamera | Kart listesi doğru satır sayısı, taşma yok | `camera-{0,1,5,10}-normal/small.png` |
| Uzun model/seri (60+) | Elide, yatay scroll yok | Task 1 testi + `camera-long-small.png` |
| Arama | Eşleşmeyen kartlar gizlenir, seçim bozulmaz | Task 1 testi |
| Dar pencere 850×600 | Ana eylemler görünür, önizleme eşik üstü | Task 2/5 testleri + `live-0-850.png` |
| Sekmede gizli ayar | Oku/Uygula programatik ve görünür akışta çalışır | Task 3 testi + mevcut `:110-186` |
| Tek/canlı/Stop | Eski kare HUD'da kalmaz, durum metni aynı | Task 5 testi + `:196,214` |
| Busy/lease/kopma | Mevcut davranış değişmez | Mevcut `:180,203,221,226,242-265` |
| Proje yok | `cameraProject` kaydırmadan görünür | Mevcut `:280` |
| Fiziksel kamera | Bu kart kapsam dışı; fixture sonucu fiziksel kabul sayılmaz | — |

## Kapsam dışı (bilinçli)

Histogram, IP/sıcaklık/bant genişliği/PLC telemetrisi, ROI/defect overlay, genel GenICam düğüm tarayıcısı, pixel format/trigger/FPS yazımı, açık tema, ikon fontu, yeni bağımlılık. Bunlardan biri istenirse ayrı kart açılır; `CameraSettingsService` sözleşmesi genişletilmeden yapılmaz.
