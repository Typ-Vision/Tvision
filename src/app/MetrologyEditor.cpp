#include "MetrologyEditor.hpp"
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMap>
namespace visionflow {
namespace {
QString fieldLabel(const QString& key) {
    static const QMap<QString,QString> labels{
        {"mode","Birim modu"},{"direction","Tarama yönü"},{"selection","Kenar seçimi"},{"polarity","Kenar yönü"},
        {"minContrast","Min kontrast (gray8/px)"},{"stepPx","Tarama adımı (px)"},{"widthStepPx","Enine adım (px)"},{"sigmaPx","Gaussian sigma (px)"},{"subpixel","Subpixel inceltme"},
        {"scanCount","Tarama sayısı"},{"scanWidthPx","Tarama genişliği (px)"},{"scanWidth","Tarama genişliği"},
        {"minWidthPx","Min çift genişliği (px)"},{"maxWidthPx","Max çift genişliği (px)"},{"lower","Alt tolerans"},{"upper","Üst tolerans"},
        {"angleLower","Alt açı toleransı (deg)"},{"angleUpper","Üst açı toleransı (deg)"},
        {"diameterLower","Alt çap toleransı"},{"diameterUpper","Üst çap toleransı"},{"deviationLower","Alt sapma toleransı"},{"deviationUpper","Üst sapma toleransı"},
        {"ransac","Sağlam fit (RANSAC)"},{"minPoints","Min fit noktası"},{"minFoundPoints","Min bulunan nokta"},{"minCoverage","Min gözlem oranı"},{"minFitCoverage","Min fit oranı"},{"minSpanCoverage","Min destek oranı"},
        {"inlierDistancePx","Inlier mesafesi (px)"},{"inlierDistanceMm","Inlier mesafesi (mm)"},{"inlierDistance","Inlier mesafesi"},
        {"maxRmsPx","Max RMS (px)"},{"maxRmsMm","Max RMS (mm)"},{"maxRms","Max RMS"},{"maxResidualPx","Max residual (px)"},{"maxResidualMm","Max residual (mm)"},{"maxResidual","Max residual"},
        {"minSpanPx","Min destek (px)"},{"minSpanMm","Min destek (mm)"},{"minAngularSpan","Min açısal destek (deg)"},{"maxGapDegrees","Max açısal boşluk (deg)"},{"maxCondition","Max fit koşulu"},{"unitsPerSample","Örnek ölçeği"}};
    return labels.value(key,key);
}
}

MetrologyEditor::MetrologyEditor(const QString& kind,const QJsonObject& p,const QString& frameId,bool hasFrame,QWidget* parent):QWidget(parent),kind_(kind),frameId_(frameId),original_(p){
    setObjectName("metrologySettings");auto* f=new QFormLayout(this);f->setContentsMargins(0,0,0,0);f->setRowWrapPolicy(QFormLayout::WrapAllRows);
    auto help=new QLabel((p["poseTracking"].toObject()["schemaVersion"]==2 || p["referenceBinding"].toObject()["mapping"]=="world-reference-v1") ? "World tracking: gerçek raw gözlemler ve current mm refit. accepted yalnız dahil tool toleransıdır; genel karar NOT_EVALUATED. Circle radyal sapma fit dışı gözlemleri de içerir; açı/gap gerçek current world noktalarındandır. Nominal mm ROI öğret düğmesiyle yeniden düzenlenir. Profil, scanWidthPx ve min/maxWidthPx synthetic örnek birimindedir.":"Pixel: diagnostic px. Calibrated: kayıtlı güncel kalibrasyonla mm. accepted yalnız bu aracın dahil tolerans sonucudur; genel parça kararı değildir. Circle: çap ve sampled radyal sapma ayrı limitlerdir; kalite yoksa NOT_EVALUATED. Kalibre ROI koordinatları mm, scanWidth mm, unitsPerSample mm/synthetic px; fit eşikleri seçili birimde. Line açısı seçili koordinat düzleminde hesaplanır. Scan ve residual kanıtı Results / Logs içindedir.");help->setWordWrap(true);f->addRow(help);
    for(auto it=p.begin();it!=p.end();++it){const auto key=it.key();if(key=="scanRoi")continue;
        if(it->isDouble()){auto* s=new QDoubleSpinBox;s->setObjectName("mt_"+key);s->setDecimals(key=="scanCount" || key=="minPoints" || key=="minFoundPoints" || key=="minFoundPoints" ? 0:9);s->setRange(key.startsWith("angle") ? -180:0,key.startsWith("angle") ? 180:1e9);if(key=="stepPx")s->setRange(.05,16);else if(key=="widthStepPx")s->setRange(.25,16);else if(key=="sigmaPx")s->setRange(0,16);else if(key=="minContrast")s->setRange(1e-9,5100);else if(key=="scanCount" || key=="minPoints" || key=="minFoundPoints" || key=="minFoundPoints")s->setRange(3,128);else if(key=="scanWidthPx")s->setRange(1e-9,1024);else if(key=="minCoverage" || key=="minSpanCoverage" || key=="minFitCoverage" || key=="minFitCoverage")s->setRange(1e-9,1);else if(key=="minWidthPx")s->setRange(0,65536);else if(key=="maxWidthPx")s->setRange(1e-9,65536);else if(key.startsWith("minSpan") || key.startsWith("inlierDistance") || key.startsWith("maxRms") || key.startsWith("maxResidual"))s->setMinimum(1e-9);
            s->setValue(it->toDouble());s->setToolTip(key);QString label=fieldLabel(key);if((p["poseTracking"].toObject()["schemaVersion"]==2 || p["referenceBinding"].toObject()["mapping"]=="world-reference-v1") && (key.endsWith("Px") && !key.startsWith("minSpan") && !key.startsWith("inlierDistance") && !key.startsWith("maxRms") && !key.startsWith("maxResidual")))label+=" (synthetic samples)";if(key=="lower" || key=="upper")label+=" (width / line span; mode px/mm)";if(kind=="circle_fit" && (key.startsWith("diameter") || key.startsWith("deviation") || key=="scanWidth" || key=="inlierDistance" || key=="maxRms" || key=="maxResidual"))label+=" (mode px/mm)";if(kind=="circle_fit" && key=="unitsPerSample")label+=" (mode units / synthetic px)";f->addRow(label,s);connect(s,&QDoubleSpinBox::editingFinished,this,[this]{if(onChanged)onChanged();});}
        else if(it->isBool()){auto* c=new QCheckBox(fieldLabel(key));c->setObjectName("mt_"+key);c->setChecked(it->toBool());f->addRow(c);connect(c,&QCheckBox::toggled,this,[this]{if(onChanged)onChanged();});}
        else if(it->isString()){auto* c=new QComboBox;c->setObjectName("mt_"+key);const auto choices=key=="mode" ? QStringList{"pixel","calibrated"}:key=="direction" ? QStringList{"outward","inward"}:key=="direction" ? QStringList{"outward","inward"}:key=="selection" ? QStringList{"first","last","strongest"}:kind=="edge_pair" ? QStringList{"bright","dark","any"}:QStringList{"rising","falling","any"};c->addItems(choices);c->setCurrentText(it->toString());c->setToolTip(key);f->addRow(fieldLabel(key),c);connect(c,&QComboBox::currentIndexChanged,this,[this]{if(onChanged)onChanged();});}
    }
    auto* draw=new QPushButton(kind=="circle_fit" ? "Halka / yay çiz":kind=="edge_pair" ? "Ok çiz (A → B)":"Dönük kutu çiz");draw->setObjectName("metrologyDraw");draw->setEnabled(hasFrame);f->addRow(draw);connect(draw,&QPushButton::clicked,this,[this]{if(onDraw)onDraw();});
    QJsonObject coords;const auto saved=p["scanRoi"].toObject();
    if(kind=="circle_fit"){ArcRoi g{{{100,100},30,50},0,360};if(!saved.isEmpty()){const auto r=ToolRoi::fromJson(saved);g=std::holds_alternative<Annulus>(r.geometry) ? ArcRoi{std::get<Annulus>(r.geometry),0,360}:std::get<ArcRoi>(r.geometry);}coords={{"cx",g.ring.centre.x()},{"cy",g.ring.centre.y()},{"inner",g.ring.innerRadius},{"outer",g.ring.outerRadius},{"start",g.startDegrees},{"sweep",g.sweepDegrees}};}
    else if(kind=="edge_pair"){ScanSegment g{{10,20},{100,20},4};if(!saved.isEmpty())g=std::get<ScanSegment>(ToolRoi::fromJson(saved).geometry);coords={{"ax",g.start.x()},{"ay",g.start.y()},{"bx",g.end.x()},{"by",g.end.y()},{"width",g.width}};}
    else {RotatedRectangle g{{100,100},{100,40},0};if(!saved.isEmpty())g=std::get<RotatedRectangle>(ToolRoi::fromJson(saved).geometry);coords={{"cx",g.centre.x()},{"cy",g.centre.y()},{"width",g.size.width()},{"height",g.size.height()},{"angle",g.angleDegrees}};}
    for(auto i=coords.begin();i!=coords.end();++i){auto* s=new QDoubleSpinBox;s->setObjectName("mt_roi_"+i.key());s->setDecimals(9);s->setRange(-1e8,1e8);s->setValue(i->toDouble());f->addRow(i.key()+((i.key()=="angle" || i.key()=="start" || i.key()=="sweep") ? " (deg)":((kind=="circle_fit" && p["mode"]=="calibrated") || (p["poseTracking"].toObject()["schemaVersion"]==2 || p["referenceBinding"].toObject()["mapping"]=="world-reference-v1") ? " (nominal world mm)":" (raw px)")),s);}
    auto* apply=new QPushButton("Koordinatları uygula");apply->setObjectName("metrologyApplyRoi");apply->setEnabled(hasFrame);f->addRow(apply);
    connect(apply,&QPushButton::clicked,this,[this]{if(const auto roi=coordinateRoi();roi && onRoi)onRoi(*roi);});
    if(kind_=="circle_fit")for(const auto& key:{"start","sweep"}) {
        auto* spin=findChild<QDoubleSpinBox*>(QString("mt_roi_")+key);spin->setRange(-360,360);spin->setKeyboardTracking(true);
        connect(spin,&QDoubleSpinBox::valueChanged,this,[this]{if(!original_["scanRoi"].toObject().isEmpty())if(const auto roi=coordinateRoi();roi && onPreview)onPreview(*roi);});
        connect(spin,&QDoubleSpinBox::editingFinished,this,[this]{if(!original_["scanRoi"].toObject().isEmpty())if(const auto roi=coordinateRoi();roi && onRoi)onRoi(*roi);});
    }
    auto* hint=new QLabel(saved.isEmpty() ? "ROI öğretilmedi; çizim veya koordinatları uygulayın.":"ROI reçeteye bağlıdır. Tutamaçlar / Ctrl+Z / Escape desteklenir. Line genişlik ekseni destek yönü; yükseklik ekseni scan yönüdür.");hint->setWordWrap(true);f->addRow(hint);
}
std::optional<ToolRoi> MetrologyEditor::coordinateRoi() const {
    try {
        auto n=[&](const char* k){return findChild<QDoubleSpinBox*>(QString("mt_roi_")+k)->value();};
        ToolRoi roi;roi.frame.id=frameId_;
        if(!original_["scanRoi"].toObject().isEmpty())roi.frame=ToolRoi::fromJson(original_["scanRoi"].toObject()).frame;
        if(kind_=="circle_fit")roi.geometry=ArcRoi{{{n("cx"),n("cy")},n("inner"),n("outer")},n("start"),n("sweep")};
        else if(kind_=="edge_pair")roi.geometry=ScanSegment{{n("ax"),n("ay")},{n("bx"),n("by")},n("width")};
        else roi.geometry=RotatedRectangle{{n("cx"),n("cy")},{n("width"),n("height")},n("angle")};
        roi.validate();return roi;
    }catch(const std::exception&){return {};}
}
QJsonObject MetrologyEditor::parameters() const {auto p=original_;for(auto i=original_.begin();i!=original_.end();++i){const auto name="mt_"+i.key();if(auto* s=findChild<QDoubleSpinBox*>(name)){if(s->value()!=i->toDouble())p[i.key()]=s->value();}else if(auto* c=findChild<QCheckBox*>(name))p[i.key()]=c->isChecked();else if(auto* c=findChild<QComboBox*>(name))p[i.key()]=c->currentText();}if(kind_=="circle_fit" && p["mode"]!=original_["mode"]){p["scanRoi"]=QJsonObject{};p.remove("worldCalibration");p["unitsPerSample"]=p["mode"]=="pixel" ? 1.:.1;}if(kind_=="circle_fit" && p["mode"]!=original_["mode"]){p["scanRoi"]=QJsonObject{};p.remove("worldCalibration");p["unitsPerSample"]=p["mode"]=="pixel" ? 1.:.1;}return p;}
}
