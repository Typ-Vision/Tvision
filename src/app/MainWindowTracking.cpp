#include "PrecisionInput.hpp"
#include "MainWindow.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "application/PoseRoiService.hpp"
#include "application/ReferenceService.hpp"
#include "application/WorldTrackingService.hpp"
#include "PropertySections.hpp"
#include <numbers>
#include <QComboBox>
#include <QFormLayout>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QTimer>
#include <algorithm>
#include <QDialog>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QVBoxLayout>
namespace visionflow {
void MainWindow::addTrackingProperties(QFormLayout* host,const ToolDefinition& tool) {
    auto* section=new QWidget;section->setObjectName("referenceSection");auto* form=new QFormLayout(section);form->setContentsMargins(0,0,0,0);form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    host->addRow("Referans",section);
    auto* picker=new QComboBox;picker->setObjectName("poseReferencePicker");picker->addItem("Görüntüye sabit",QString{});picker->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);picker->setMinimumContentsLength(10);
    const QMap<QString,QString> labels{{"pose","Model referansı"},{"point","Nokta"},{"centre","Merkez"},{"first","İlk kenar noktası"},{"second","İkinci kenar noktası"},{"centrePx","Çember merkezi"},{"originPx","Çizgi referans noktası"}};
    for(const auto& source:service_.tools())if(source.enabled && source.id!=tool.id)for(const auto& port:outputPorts(source.kind))if(referenceSourcePort(source.kind,port.id))
        picker->addItem(source.name+" · "+(source.kind=="template_match" ? QString("Model merkezi"):labels.value(port.id))+" ("+port.id+")",source.id+"/"+port.id);
    QString selected;QPointF offset;double angle=0,unit=.1;bool world=false;
    try{
        if(tool.parameters.contains("referenceBinding")){const auto b=toolReferenceBinding(tool);selected=b.sourceTool+"/"+b.sourcePort;offset=b.offsetPx;angle=b.angleOffsetRadians*180/std::numbers::pi;world=b.world;unit=b.mmPerSample;}
        else if(tool.parameters.contains("poseTracking")){selected=trackingSourceTool(tool)+"/pose";world=isWorldPoseTracking(tool);if(world)unit=worldPoseTracking(tool).mmPerSample;}
        else for(const auto& c:editorConnections(service_.activeRecipe()->definition))if(c.targetTool==tool.id && c.targetPort=="reference")selected=c.sourceTool+"/"+c.sourcePort;
    }catch(const std::exception&){}
    const bool invalidated=tool.parameters["referenceBinding"].toObject()["invalidated"].toBool() || tool.parameters["poseTracking"].toObject()["invalidated"].toBool();
    if(invalidated){auto* warning=new QLabel("Görüntü / ROI geometrisi değişti. Referans ilişkisini güncel görüntü ve kaynak sonucuyla yeniden öğretin.");warning->setObjectName("referenceInvalidated");warning->setWordWrap(true);form->addRow(warning);}
    world=world || tool.parameters["mode"]=="calibrated";
    if(!selected.isEmpty() && picker->findData(selected)<0)picker->addItem("Eksik / devre dışı: "+selected,selected);
    for(int i=0;i<picker->count();++i)picker->setItemData(i,picker->itemText(i),Qt::ToolTipRole);
    picker->setCurrentIndex(selected.isEmpty() ? 0:picker->findData(selected));form->addRow("Kaynak / çıktı",picker);
    auto number=[&](const char* name,double value){auto* n=new QDoubleSpinBox;n->setObjectName(name);n->setDecimals(9);n->setRange(-1e8,1e8);loadPrecisionInput(n,value,QString(name)!="referenceOffsetAngle");return n;};
    auto* x=number("referenceOffsetX",offset.x());auto* y=number("referenceOffsetY",offset.y());auto* a=number("referenceOffsetAngle",angle);
    form->addRow("X ofseti [px]",x);form->addRow("Y ofseti [px]",y);form->addRow("Açı ofseti [°]",a);
    auto* help=new QLabel;help->setObjectName("poseTrackingStatus");help->setWordWrap(true);form->addRow(help);
    auto state=[picker,x,y,a,help,this]{const auto key=picker->currentData().toString();picker->setToolTip(picker->currentText());const bool pose=key.endsWith("/pose"),fixed=key.isEmpty();x->setEnabled(!fixed && !editorSession_.busy());y->setEnabled(!fixed && !editorSession_.busy());a->setEnabled(pose && !editorSession_.busy());if(!pose)a->setValue(0);
        help->setText(fixed ? "Referans hareketi uygulanmaz. Seçim yalnız Bağla ile kaydedilir.":pose ? "Model eksenlerinde X/Y; açı ofseti model referansı etrafında uygulanır. Güncel kaynak sonucu gerekir.":"Bu kaynak yalnız konum üretir. X/Y görüntü eksenlerindedir; ROI'nin kendi yönü korunur. Güncel kaynak sonucu gerekir.");};
    connect(picker,&QComboBox::currentIndexChanged,this,[state]{state();});state();
    auto* advanced=propertySection(form,"Referans ayrıntıları…",tool.kind+"/referenceDetails",settingsFile_);auto* details=static_cast<QFormLayout*>(advanced->layout());
    auto* sampling=new QDoubleSpinBox;sampling->setObjectName("trackingMmPerSample");sampling->setDecimals(9);sampling->setRange(1e-9,1e9);loadPrecisionInput(sampling,unit);if(world)details->addRow("Nominal mm / örnek",sampling);else sampling->setParent(advanced);
    auto* identity=new QLabel(selected.isEmpty() ? "Öğretilmiş ilişki yok.":"Kayıtlı ilişki. Model/ROI/port veya kalibrasyon değişimi yeniden öğretim gerektirir. Çözülen geometri salt okunurdur.");identity->setWordWrap(true);identity->setToolTip(selected);details->addRow(identity);
    if(tool.parameters.contains("referenceBinding")){const auto b=toolReferenceBinding(tool);auto* hashes=new QLabel("Kaynak geometri: "+b.geometryId.left(12)+"\nROI kimliği: "+b.roiIdentity().left(12));hashes->setObjectName("referenceTeachingIdentity");hashes->setWordWrap(true);hashes->setToolTip(b.geometryId+"\n"+b.roiIdentity());details->addRow(hashes);}
    if(tool.kind=="edge_point" || tool.kind=="edge_pair" || tool.kind=="line_fit" || tool.kind=="circle_fit"){
        auto* author=new QPushButton("Nominal mm ROI öğret…");author->setObjectName("editWorldTrackingRoi");author->setEnabled(editorSession_.frame.has_value() && !editorSession_.busy());details->addRow(author);
        connect(author,&QPushButton::clicked,this,[this,picker]{editWorldTrackingRoi(picker->currentData().toString());});
    }
    auto* teach=new QPushButton(selected.isEmpty() ? "Referansa bağla":"İlişkiyi yeniden kaydet");teach->setObjectName("teachPoseReference");teach->setEnabled(editorSession_.frame.has_value() && !editorSession_.busy());form->addRow(teach);
    connect(teach,&QPushButton::clicked,this,[this,picker,sampling]{teachPoseReference(picker->currentData().toString(),precisionValue(sampling));});
    auto* cancel=new QPushButton("İptal");cancel->setObjectName("cancelReferenceEdit");form->addRow(cancel);connect(cancel,&QPushButton::clicked,this,[this]{QTimer::singleShot(0,this,[this]{refreshWorkspace();});});
    auto* remove=new QPushButton(world && tool.kind!="circle_fit" ? "Referansı + mm ROI kaldır":"Referansı kaldır");remove->setObjectName("clearPoseReference");remove->setEnabled(!selected.isEmpty() && !editorSession_.busy());form->addRow(remove);connect(remove,&QPushButton::clicked,this,[this]{teachPoseReference({},0);});
    auto edit=[this,picker,x,y,a,selected,id=tool.id]{
        if(editorSession_.busy() || selectedToolId_!=id || picker->currentData().toString()!=selected)return;
        try{for(const auto& t:service_.tools())if(t.id==id && t.parameters.contains("referenceBinding")){
            auto b=toolReferenceBinding(t);b.offsetPx={precisionValue(x),precisionValue(y)};if(precisionInputEdited(a))b.angleOffsetRadians=precisionValue(a)*std::numbers::pi/180;
            const auto e=editorSession_.evaluation(editorContext());const auto image=editorSession_.toolImage(editorContext(),id);if(!e.cache.contains(b.sourceTool))throw std::runtime_error("Güncel kaynak sonucu yok. Önce kaynak aracını çalıştırın.");
            for(const auto& source:service_.tools())if(source.id==b.sourceTool){
                if(b.world){const auto ctx=calibrationResolver(*service_.activeRecipe())(image);const auto r=ReferenceService::resolveWorld(b,source,e.cache[b.sourceTool].result,image,ctx);if(!r.map)throw std::runtime_error(r.reason.toUtf8().constData());auto candidate=t;candidate.parameters["referenceBinding"]=b.toJson();WorldTrackingService::preflight(candidate,image,*r.map);}
                else {const auto r=ReferenceService::resolve(b,source,e.cache[b.sourceTool].result,image);if(!r.roi)throw std::runtime_error(r.reason.toUtf8().constData());}
            }
            service_.bindReference(id,b);QTimer::singleShot(0,this,[this]{refreshWorkspace();});return;
        }}catch(const std::exception& e){for(const auto& t:service_.tools())if(t.id==id && t.parameters.contains("referenceBinding")){const auto b=toolReferenceBinding(t);loadPrecisionInput(x,b.offsetPx.x(),true);loadPrecisionInput(y,b.offsetPx.y(),true);loadPrecisionInput(a,b.angleOffsetRadians*180/std::numbers::pi);}displayError(QString::fromUtf8(e.what()));}
    };
    for(auto* n:{x,y,a})connect(n,&QDoubleSpinBox::editingFinished,this,edit);
}
void MainWindow::editWorldTrackingRoi(const QString& producer){
    if(editorSession_.busy() || !service_.activeRecipe() || !applyToolProperties())return;
    try{
        if(producer.isEmpty())throw std::runtime_error("Önce referans kaynağını seçin.");
        for(const auto& tool:service_.tools())if(tool.id==selectedToolId_){
            const auto image=editorSession_.toolImage(editorContext(),tool.id);
            const auto ctx=calibrationResolver(*service_.activeRecipe())(image);
            if(!ctx.calibration || !ctx.current)throw std::runtime_error("Nominal mm ROI requires current validated calibration");
            const auto target=ctx.calibration->dependency().target;
            const auto signature=flowSignature();const auto id=tool.id;const auto kind=tool.kind;
            auto* dialog=new QDialog(this);dialog->setObjectName("worldTrackingRoiDialog");dialog->setWindowTitle("Nominal world ROI · "+tool.name);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowModality(Qt::WindowModal);dialog->resize(430,540);
            auto* layout=new QVBoxLayout(dialog);auto* help=new QLabel("Koordinatlar nominal dünya düzleminde mm'dir. Geometri, güncel referans ve kalibrasyonla birlikte öğretilince kaydedilir. İptal reçeteyi değiştirmez. Örnekleme ayarları synthetic örnek birimindedir.");help->setWordWrap(true);layout->addWidget(help);
            auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);auto* body=new QWidget;auto* fields=new QFormLayout(body);fields->setRowWrapPolicy(QFormLayout::WrapAllRows);scroll->setWidget(body);layout->addWidget(scroll);
            ToolRoi roi{target,ScanSegment{{1,1},{5,1},.2}};
            if(kind=="line_fit")roi.geometry=RotatedRectangle{{5,5},{4,2},0};
            if(kind=="circle_fit")roi.geometry=ArcRoi{{{5,5},1,2},0,360};
            if((tool.parameters.contains("referenceBinding") && toolReferenceBinding(tool).world) || isWorldPoseTracking(tool) || (kind=="circle_fit" && tool.parameters["mode"]=="calibrated" && !tool.parameters["scanRoi"].toObject().isEmpty()))roi=ToolRoi::fromJson(tool.parameters["scanRoi"].toObject());
            QJsonObject values;
            if(auto* s=std::get_if<ScanSegment>(&roi.geometry))values={{"ax",s->start.x()},{"ay",s->start.y()},{"bx",s->end.x()},{"by",s->end.y()},{"width",s->width}};
            else if(auto* b=std::get_if<RotatedRectangle>(&roi.geometry))values={{"cx",b->centre.x()},{"cy",b->centre.y()},{"width",b->size.width()},{"height",b->size.height()},{"angle",b->angleDegrees}};
            else {const auto a=std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);values={{"cx",a.ring.centre.x()},{"cy",a.ring.centre.y()},{"inner",a.ring.innerRadius},{"outer",a.ring.outerRadius},{"start",a.startDegrees},{"sweep",a.sweepDegrees}};}
            values["mmPerSample"]=tool.parameters.contains("referenceBinding") ? toolReferenceBinding(tool).mmPerSample:isWorldPoseTracking(tool) ? worldPoseTracking(tool).mmPerSample:.1;
            const QMap<QString,QString> labels{{"ax","Başlangıç X"},{"ay","Başlangıç Y"},{"bx","Bitiş X"},{"by","Bitiş Y"},{"cx","Merkez X"},{"cy","Merkez Y"},{"width","Genişlik"},{"height","Yükseklik"},{"inner","İç yarıçap"},{"outer","Dış yarıçap"},{"angle","Yön açısı"},{"start","Başlangıç açısı"},{"sweep","Tarama açısı"},{"mmPerSample","Nominal mm / örnek"}};
            fields->addRow("Dünya düzlemi",new QLabel(target.id+(target.axes==AxisConvention::XRightYUp ? " · +Y yukarı":" · +Y aşağı")));
            for(auto i=values.begin();i!=values.end();++i){auto* n=new QDoubleSpinBox;n->setObjectName("world_roi_"+i.key());n->setDecimals(9);n->setRange(-1e8,1e8);if(i.key()=="mmPerSample")n->setRange(1e-9,1e9);loadPrecisionInput(n,i->toDouble(),i.key()!="mmPerSample" && coordinateLength(i.key()),positiveExtent(i.key()));fields->addRow(labels.value(i.key())+(i.key()=="mmPerSample" ? "":i.key()=="angle" || i.key()=="start" || i.key()=="sweep" ? " (deg)":" (mm)"),n);}
            auto* error=new QLabel;error->setObjectName("worldRoiError");error->setWordWrap(true);layout->addWidget(error);
            auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Save)->setText("Öğret ve kaydet");buttons->button(QDialogButtonBox::Save)->setObjectName("teachWorldRoi");layout->addWidget(buttons);
            connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
            connect(buttons,&QDialogButtonBox::accepted,dialog,[this,dialog,error,target,id,kind,producer,signature]{try{
                if(id!=selectedToolId_ || signature!=flowSignature() || editorSession_.busy())throw std::runtime_error("Editör değişti; ROI ekranını yeniden açın.");
                auto n=[&](const char* key){return precisionValue(dialog->findChild<QDoubleSpinBox*>(QString("world_roi_")+key));};
                ToolRoi value;value.frame=target;
                if(kind=="circle_fit")value.geometry=ArcRoi{{{n("cx"),n("cy")},n("inner"),n("outer")},n("start"),n("sweep")};
                else if(kind=="line_fit")value.geometry=RotatedRectangle{{n("cx"),n("cy")},{n("width"),n("height")},n("angle")};
                else value.geometry=ScanSegment{{n("ax"),n("ay")},{n("bx"),n("by")},n("width")};
                value.validate();teachPoseReference(producer,n("mmPerSample"),value);dialog->accept();
            }catch(const std::exception& e){error->setText(QString::fromUtf8(e.what()));}});
            dialog->open();return;
        }
    }catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
}
void MainWindow::teachPoseReference(const QString& key,double mmPerSample,std::optional<ToolRoi> nominal) {
    if(editorSession_.busy() || !service_.activeRecipe() || !applyToolProperties())return;
    try {
        const auto producer=key.section('/',0,0),port=key.contains('/') ? key.section('/',1):QString("pose");
        for(auto tool:service_.tools())if(tool.id==selectedToolId_) {
            if(producer.isEmpty()){service_.clearReference(tool.id);refreshWorkspace();return;}
            QPointF offset;double angle=0;
            if(auto* x=propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetX"))offset.setX(precisionValue(x));
            if(auto* y=propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetY"))offset.setY(precisionValue(y));
            if(port=="pose")if(auto* a=propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetAngle"))angle=precisionValue(a)*std::numbers::pi/180;
            MeasurementExecutionContext calibration;
            if(nominal){tool.parameters.remove("poseTracking");tool.parameters.remove("referenceBinding");tool.parameters["scanRoi"]=nominal->toJson();if(tool.kind!="edge_point")tool.parameters["mode"]="calibrated";}
            const auto json=tool.parameters.value("scanRoi").toObject();
            if(!json.isEmpty() && ToolRoi::fromJson(json).frame.unit==LengthUnit::Millimetre)calibration=calibrationResolver(*service_.activeRecipe())(editorSession_.toolImage(editorContext(),tool.id));
            const auto input=editorSession_.trackingTeachingInput(editorContext(),tool,producer,mmPerSample,calibration);
            const auto sourceSignature=editorSession_.evaluation(editorContext()).cache.value(producer).signature;
            const auto signature=flowSignature();const auto generation=editorSession_.generation;
            auto* watcher=editorSession_.watch<TrackingTeaching>(this,SessionJob::Teaching);const auto cancel=editorSession_.token(SessionJob::Teaching);refreshFlow();
            connect(watcher,&QFutureWatcher<TrackingTeaching>::finished,this,[this,watcher,cancel,signature,generation,producer,sourceSignature]{
                const auto work=watcher->result();watcher->deleteLater();
                if(!cancel->load() && signature==flowSignature() && generation==editorSession_.generation) {
                    if(!work.tool)displayError(work.error);
                    else try{const auto current=editorSession_.evaluation(editorContext());if(current.cache.contains(producer) && current.cache[producer].signature==sourceSignature){
                        auto links=editorConnections(service_.activeRecipe()->definition);const auto b=toolReferenceBinding(*work.tool);
                        links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& c){return c.targetTool==work.tool->id && (c.targetPort=="reference" || c.targetPort=="poseReference");}),links.end());
                        links.append({b.sourceTool,b.sourcePort,work.tool->id,referenceInput(b)});service_.updateToolConnections(*work.tool,links);refreshWorkspace();
                    }}catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
                }refreshFlow();
            });
            editorSession_.launch(watcher,[input,port,offset,angle,cancel]{return EditorSession::teachReference(input,port,offset,angle,cancel);});return;
        }
    }catch(const std::exception& e){displayError(QString::fromUtf8(e.what()));}
}
}
