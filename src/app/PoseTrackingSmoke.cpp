#include "MainWindow.hpp"
#include "CameraFlowSmokeAudit.hpp"
#include "ImageViewer.hpp"
#include "PoseMatchEditor.hpp"
#include "domain/PoseTrackingConfig.hpp"
#include "domain/ReferenceConfig.hpp"
#include "application/RuntimeService.hpp"
#include "application/CalibrationService.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QTableWidget>
#include <QTextEdit>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QElapsedTimer>
#include <QThread>
#include <QDir>
#include <QTextStream>
#include <QUuid>
#include <cmath>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMouseEvent>
namespace visionflow {
bool MainWindow::verifyPoseTracking(const QString& artifacts) {
    smokeMode_=true;int checks=0;
    auto check=[&](bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);};
    auto wait=[&]{QApplication::processEvents();QElapsedTimer timer;timer.start();while((editorSession_.busy() || runtimeJobs_.busy()) && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(!editorSession_.busy() && !runtimeJobs_.busy(),"pose UI worker deadline");};
    const auto directory=QDir(artifacts).filePath("pose-ui-fixtures/"+QUuid::createUuid().toString(QUuid::WithoutBraces));QDir().mkpath(directory);
    QImage pixels(256,256,QImage::Format_Grayscale8);pixels.fill(30);
    for(int y=0;y<24;++y)for(int x=0;x<24;++x)pixels.scanLine(46+y)[51+x]=uchar(40+(x*37+y*71+x*y*13)%190);
    for(int y=46;y<=74;++y)for(int x=88;x<=132;++x)pixels.scanLine(y)[x]=uchar(std::lround(128+100*std::erf((y-60)/1.4)));
    for(int y=81;y<=109;++y)for(int x=88;x<=102;++x)pixels.scanLine(y)[x]=uchar(std::lround(28+100*(std::erf((y-90)/1.4)-std::erf((y-100)/1.4))));
    for(int y=108;y<=142;++y)for(int x=78;x<=112;++x)pixels.scanLine(y)[x]=uchar(std::lround(128+100*std::erf((std::hypot(x-95.,y-125.)-10)/1.4)));
    for(int y=173;y<181;++y)for(int x=92;x<99;++x)pixels.scanLine(y)[x]=220;
    const auto imagePath=QDir(directory).filePath("teaching.png");check(pixels.save(imagePath),"pose UI independent actual PNG");
    for(const QString kind:{"pattern_pose","shape_pose"})for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}) {
        const auto path=QDir(directory).filePath(kind+QString::number(int(type))+".vfproject");
        service_.createProject("Pose teaching","",path,"file");const auto recipeId=service_.createRecipe("Part","",type,"file");refreshWorkspace();selectPage("editor");
        check(loadEditorImage(imagePath),"pose UI source load");wait();const auto source=selectedToolId_;
        auto add=[&](const QString& k){
            // Legacy Edge is a compatibility fixture, never a new catalog entry.
            if(k=="edge"){const auto id=service_.addTool(k);refreshWorkspace();selectTool(id);return id;}
            auto* search=findChild<QLineEdit*>("toolboxSearch");search->setText(k);
            QListWidgetItem* entry=nullptr;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)==k)entry=toolbox_->item(i);
            check(entry && !entry->isHidden(),"pose UI searchable Toolbox entry");toolbox_->setCurrentItem(entry);findChild<QPushButton*>("addTool")->click();search->clear();return selectedToolId_;
        };
        auto bind=[&](const QString& target){selectTool(target);auto* input=propertiesBody_->findChild<QComboBox*>("input_image");check(input && input->findData(source+"/image")>0,"pose UI image choice");input->setCurrentIndex(input->findData(source+"/image"));};
        const auto poseId=add(kind);bind(poseId);
        for(const auto& kv:QVector<QPair<QString,double>>{{"angleMin",0},{"angleMax",90},{"angleStep",90},{"pyramidLevels",1},{"minScore",.95}})propertiesBody_->findChild<QDoubleSpinBox*>("pose_"+kv.first)->setValue(kv.second);
        propertiesBody_->findChild<QDoubleSpinBox*>("pose_anchor_x")->setValue(9);propertiesBody_->findChild<QDoubleSpinBox*>("pose_anchor_y")->setValue(14);
        check(applyToolProperties(),"pose UI search settings");findChild<QPushButton*>("modelRoi")->click();commitRoi({51,46,24,24});imageViewer_->drawRegion(false);wait();
        auto configured=[&](const QString& id){for(const auto& t:service_.tools())if(t.id==id)return t;throw std::runtime_error("UI tool missing");};
        check(!configured(poseId).parameters["modelPng"].toString().isEmpty() && configured(poseId).parameters["anchor"].toObject()["x"]==9,"pose UI actual teaching with explicit anchor");
        runExecution(poseId);wait();auto e=editorSession_.evaluation(editorContext());check(e.cache.contains(poseId) && e.cache[poseId].result.outputs["pose"].valid,"pose UI actual matcher result");
        const auto beforeInvalidModel=service_.activeRecipe()->definition;
        findChild<QPushButton*>("modelRoi")->click();commitRoi({51,46,8,8});imageViewer_->drawRegion(false);wait();check(service_.activeRecipe()->definition==beforeInvalidModel,"invalid model never mutates recipe");
        findChild<QPushButton*>("modelRoi")->click();commitRoi({52,46,24,24});imageViewer_->drawRegion(false);
        auto replaced=pixels;replaced.scanLine(0)[0]=31;check(replaced.save(imagePath),"replace source while model teaching is pending");wait();check(service_.activeRecipe()->definition==beforeInvalidModel,"changed image asset suppresses late model commit");
        check(pixels.save(imagePath),"restore model teaching source");selectTool(source);runExecution(source);wait();selectTool(poseId);runExecution(poseId);wait();
        const auto pointSource=add("template_match");bind(pointSource);findChild<QPushButton*>("modelRoi")->click();commitRoi({51,46,24,24});imageViewer_->drawRegion(false);wait();runExecution(pointSource);wait();
        check(editorSession_.evaluation(editorContext()).cache.contains(pointSource),"real Point producer current cache");
        QStringList consumers;
        for(const QString consumerKind:{"edge_point","edge_pair","line_fit","circle_fit","blob","code_read","edge"}) {
            if(consumerKind=="code_read"){bool available=false;for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)==consumerKind && !(toolbox_->item(i)->flags()&Qt::ItemIsEnabled))available=false;else if(toolbox_->item(i)->data(Qt::UserRole)==consumerKind)available=true;if(!available)continue;}
            const auto id=add(consumerKind);consumers.append(id);bind(id);
            auto t=configured(id);if(QStringList{"edge_pair","line_fit","circle_fit"}.contains(consumerKind)){check(t.parameters["mode"]=="calibrated","UI creation defaults: circle/line/pair calibrated");propertiesBody_->findChild<QComboBox*>("mt_mode")->setCurrentText("pixel");t=configured(id);}const CoordinateFrame raw{editorSession_.frame->metadata().coordinateSystemId};
            ToolRoi roi{raw,ScanSegment{{95,50},{95,70},2}};
            if(consumerKind=="edge_pair")roi.geometry=ScanSegment{{95,85},{95,105},2};
            if(consumerKind=="line_fit")roi.geometry=RotatedRectangle{{115,60},{24,20},0};
            if(consumerKind=="circle_fit")roi.geometry=Annulus{{95,125},5,15};
            if(consumerKind=="edge"){t.parameters["searchRoi"]=MatchService::rectangle({88,46,44,29});t.parameters["direction"]="top_to_bottom";}
            else if(consumerKind=="blob" || consumerKind=="code_read")t.parameters["searchRoi"]=MatchService::rectangle({87,168,18,18});else t.parameters["scanRoi"]=roi.toJson();
            if(consumerKind=="edge_point")t.parameters["minContrast"]=10.;
            if(consumerKind=="blob"){t.parameters["minArea"]=1;t.parameters["threshold"]=100;}
            service_.updateTool(t);refreshWorkspace();selectTool(id);
            auto* picker=propertiesBody_->findChild<QComboBox*>("poseReferencePicker");check(picker && picker->findData(poseId+"/pose")>0 && !propertiesBody_->findChild<QComboBox*>("input_poseReference"),"one common pose picker");
            const auto beforeSelection=service_.activeRecipe()->definition;picker->setCurrentIndex(picker->findData(poseId+"/pose"));check(service_.activeRecipe()->definition==beforeSelection,"picker selection remains pending until teach");
            propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();t=configured(id);
            if(!t.parameters.contains("referenceBinding"))QTextStream(stderr)<<"Teaching failure "<<consumerKind<<": "<<workspaceError_->text()<<'\n';
            check(t.parameters.contains("referenceBinding") && toolReferenceBinding(t).sourceTool==poseId,"UI teaches actual pose relationship");
            check(validateGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition)).valid(),"UI atomic binding leaves valid graph");
            const auto bound=service_.activeRecipe()->definition;propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();check(service_.activeRecipe()->definition==bound,"deterministic no-op reteaching");
            const auto nominal=toolReferenceBinding(configured(id)).nominal.toJson();
            picker=propertiesBody_->findChild<QComboBox*>("poseReferencePicker");
            propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetAngle")->setValue(12);
            picker->setCurrentIndex(picker->findData(pointSource+"/point"));
            auto* angle=propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetAngle");check(!angle->isEnabled() && angle->value()==0 && service_.activeRecipe()->definition==bound,"Point selection resets pending angle without committing");
            propertiesBody_->findChild<QPushButton*>("cancelReferenceEdit")->click();wait();check(service_.activeRecipe()->definition==bound && propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetAngle")->isEnabled(),"cancel restores saved Pose relationship");
            picker=propertiesBody_->findChild<QComboBox*>("poseReferencePicker");picker->setCurrentIndex(picker->findData(pointSource+"/point"));propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();
            check(toolReferenceBinding(configured(id)).mode==ReferenceMode::Translation && toolReferenceBinding(configured(id)).nominal.toJson()==nominal,"all seven tools bind actual Point without changing nominal ROI");
            check(!propertiesBody_->findChild<QComboBox*>("input_reference"),"only one reference selector");
            auto* dx=propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetX");dx->setValue(.25);QMetaObject::invokeMethod(dx,"editingFinished",Qt::DirectConnection);wait();
            check(toolReferenceBinding(configured(id)).offsetPx==QPointF(.25,0) && editorSession_.evaluation(editorContext()).cache.contains(pointSource),"offset edit commits atomically and preserves upstream current cache");
            check(imageViewer_->roi().has_value(),"offset edit updates current Viewer ROI");
            const auto view=*imageViewer_->roi();const auto base=toolReferenceBinding(configured(id)).nominal;
            auto location=[](const ToolRoi& r){return std::visit([](const auto& g)->QPointF{using T=std::decay_t<decltype(g)>;if constexpr(std::is_same_v<T,ScanSegment>)return g.start;else if constexpr(std::is_same_v<T,RotatedRectangle> || std::is_same_v<T,Annulus>)return g.centre;else if constexpr(std::is_same_v<T,ArcRoi>)return g.ring.centre;else return {};},r.geometry);};
            check(QLineF(location(view),location(base)+QPointF(.25,0)).length()<1e-6,"Viewer offset applies exactly once and leaves nominal relation intact");
            const auto beforeDrag=service_.activeRecipe()->definition;const auto at=location(view);
            auto mouse=[&](QEvent::Type event,QPointF p,Qt::MouseButton button,Qt::MouseButtons buttons){const auto point=imageViewer_->mapFromScene(p);QMouseEvent ev(event,QPointF(point),QPointF(imageViewer_->viewport()->mapToGlobal(point)),button,buttons,Qt::NoModifier);QApplication::sendEvent(imageViewer_->viewport(),&ev);};
            mouse(QEvent::MouseButtonPress,at,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,at+QPointF(5,5),Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,at+QPointF(5,5),Qt::LeftButton,Qt::NoButton);
            check(imageViewer_->roi() && imageViewer_->roi()->toJson()==view.toJson() && service_.activeRecipe()->definition==beforeDrag && selectedToolId_==id,"resolved Viewer ignores geometry drag and preserves tool selection");
            if(type==ApplicationType::Measurement){
                auto* scroll=findChild<QScrollArea*>("propertiesScroll");resize(1400,900);QApplication::processEvents();scroll->ensureWidgetVisible(propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetX"));QApplication::processEvents();grab().save(QDir(artifacts).filePath("r5-"+kind+"-"+consumerKind+"-point-1400.png"));
                resize(850,600);QApplication::processEvents();scroll->ensureWidgetVisible(propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetX"));QApplication::processEvents();grab().save(QDir(artifacts).filePath("r5-"+kind+"-"+consumerKind+"-point-850.png"));if(scroll->horizontalScrollBar()->maximum()!=0){QTextStream(stderr)<<consumerKind<<" Properties overflow="<<scroll->horizontalScrollBar()->maximum()<<" viewport="<<scroll->viewport()->width()<<'\n';for(auto* widget:propertiesBody_->findChildren<QWidget*>())if(widget->minimumSizeHint().width()>scroll->viewport()->width()-20)QTextStream(stderr)<<widget->objectName()<<" minHint="<<widget->minimumSizeHint().width()<<'\n';}
                check(scroll->horizontalScrollBar()->maximum()==0,"all seven reference panels fit narrow Properties");resize(1400,900);
            }
            const auto offsetDefinition=service_.activeRecipe()->definition;dx=propertiesBody_->findChild<QDoubleSpinBox*>("referenceOffsetX");dx->setValue(10000);QMetaObject::invokeMethod(dx,"editingFinished",Qt::DirectConnection);wait();check(service_.activeRecipe()->definition==offsetDefinition && dx->value()==.25,"invalid offset restores saved value");
            picker=propertiesBody_->findChild<QComboBox*>("poseReferencePicker");picker->setCurrentIndex(picker->findData(poseId+"/pose"));dx->setValue(0);propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();
            check(toolReferenceBinding(configured(id)).mode==ReferenceMode::Rigid && toolReferenceBinding(configured(id)).offsetPx.isNull(),"Point to Pose transition explicitly taught");
            // A conflict is rejected without leaving a new tool or connection behind.
            if(consumerKind=="blob"){auto links=editorConnections(service_.activeRecipe()->definition);links.append({id,"region",id,"region"});bool rejected=false;try{service_.updateToolConnections(configured(id),links);}catch(const std::exception&){rejected=true;}check(rejected && service_.activeRecipe()->definition==bound,"graph conflict edit is atomic");}
        }
        const auto edgeId=consumers.first();selectTool(edgeId);runExecution(edgeId);wait();e=editorSession_.evaluation(editorContext());check(e.cache.contains(edgeId) && e.cache[edgeId].result.outputs["point"].valid,"UI tracked edge TargetOnly runs");
        check(QLineF(std::get<QPointF>(e.cache[edgeId].result.outputs["point"].data),{95,60}).length()<.3 && editorSession_.cycle->acquisitions==0,"UI actual edge oracle and acquisition0");
        const auto rawHash=editorSession_.frame->contentId();
        // Changing the model invalidates the cached producer; teach cannot fall back.
        selectTool(poseId);propertiesBody_->findChild<QDoubleSpinBox*>("pose_anchor_x")->setValue(10);check(applyToolProperties(),"UI anchor edit");selectTool(edgeId);const auto stale=service_.activeRecipe()->definition;
        propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();check(service_.activeRecipe()->definition==stale,"missing current pose cache rejects teaching atomically");
        selectTool(poseId);runExecution(poseId);wait();selectTool(edgeId);propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();check(configured(edgeId).parameters["referenceBinding"]!=configured(consumers[1]).parameters["referenceBinding"],"UI explicit reteacher uses new model identity");
        // Restore anchor and re-teach every consumer using fresh actual results.
        selectTool(poseId);propertiesBody_->findChild<QDoubleSpinBox*>("pose_anchor_x")->setValue(9);check(applyToolProperties(),"restore UI anchor");runExecution(poseId);wait();
        for(const auto& id:consumers){selectTool(id);propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();}
        selectTool(edgeId);runExecution(edgeId);wait();check(editorSession_.frame->contentId()==rawHash,"teaching preserves owned raw pixels");
        QImage moved(256,256,QImage::Format_Grayscale8);moved.fill(30);
        for(int y=0;y<256;++y)for(int x=0;x<256;++x){const int sx=y-120+60,sy=60-(x-140);if(sx>=0 && sx<256 && sy>=0 && sy<256)moved.scanLine(y)[x]=pixels.constScanLine(sy)[sx];}
        check(moved.save(imagePath),"UI independent inverse raster rotation");runFlow();wait();e=editorSession_.evaluation(editorContext());
        check(e.cache[edgeId].result.outputs["point"].valid && QLineF(std::get<QPointF>(e.cache[edgeId].result.outputs["point"].data),{140,155}).length()<.3,"UI rotated actual edge oracle");
        check(imageViewer_->roi().has_value() && QLineF(std::get<ScanSegment>(imageViewer_->roi()->geometry).start,{150,155}).length()<.3,"UI displays current resolved ROI rather than nominal rectangle");
        if(type==ApplicationType::Measurement){imageViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-rotated-tracking.png"));}
        check(!propertiesBody_->findChild<QPushButton*>("edgeDraw")->isEnabled(),"tracked display cannot overwrite nominal ROI with current geometry");
        QImage repeated=pixels;for(int y=0;y<24;++y)for(int x=0;x<24;++x)repeated.scanLine(200+y)[180+x]=pixels.constScanLine(46+y)[51+x];check(repeated.save(imagePath),"UI repeated candidate raster");runFlow();wait();e=editorSession_.evaluation(editorContext());
        check(std::get<bool>(e.cache[poseId].result.outputs["ambiguous"].data) && e.cache[edgeId].result.state==ExecutionState::Skipped && e.cache[edgeId].result.outputs.isEmpty() && !imageViewer_->roi(),"UI actual ambiguous reference clears result and resolved ROI");
        const auto ambiguousDefinition=service_.activeRecipe()->definition;propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();check(service_.activeRecipe()->definition==ambiguousDefinition,"UI ambiguous teaching preserves recipe");
        check(pixels.save(imagePath),"restore UI teaching raster");runFlow();wait();
        service_.save();const auto definition=service_.activeRecipe()->definition;const auto revision=service_.activeRecipe()->recipeVersion;service_.openProject(path);service_.selectRecipe(recipeId);service_.save();check(service_.activeRecipe()->definition==definition && service_.activeRecipe()->recipeVersion==revision && !service_.dirty(),"UI teaching save/reopen/no-op");
        // Remove the relationship through the UI; nominal ROI remains editable.
        refreshWorkspace();selectTool(edgeId);propertiesBody_->findChild<QPushButton*>("clearPoseReference")->click();wait();check(!configured(edgeId).parameters.contains("referenceBinding"),"UI clears binding and reference together");
        check(validateGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition)).valid(),"clear reference leaves valid graph");
        // Reopening the saved project restores the original teaching, never an active permission.
        service_.discardChanges();service_.openProject(path);service_.selectRecipe(recipeId);refreshWorkspace();wait();selectTool(poseId);runExecution(poseId);wait();selectTool(edgeId);runExecution(edgeId);wait();
        RuntimeService runtime(QDir(directory).filePath("runtime-"+kind+QString::number(int(type))),path+".calibrations.json",RuntimeIdentity::localEngineer());
        runtime.checkConfiguration(*service_.activeRecipe(),directory);check(!runtime.active(),"Configuration check alone grants no active permission");
        auto runtimeUi=[&](const QString& label){
            check(moved.save(imagePath),"Runtime independent rotated raster");
            service_.save();selectPage("recipes");refreshRecipes();
            for(int row=0;row<recipeTable_->rowCount();++row)if(recipeTable_->item(row,0)->data(Qt::UserRole)==recipeId)recipeTable_->selectRow(row);
            findChild<QPushButton*>("loadRecipe")->click();wait();findChild<QPushButton*>("runtime_run")->click();wait();
            findChild<QPushButton*>("runtime_cycle")->click();wait();
            if(!runtimeResult_)QTextStream(stderr)<<runtimeSummary_->text()<<"\n";
            check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"Reference UI actual Runtime persisted");
            runtimeTools_->setCurrentIndex(runtimeTools_->findData(edgeId));showRuntimeResult();const auto runtimeScan=CameraFlowSmokeAudit::runtimeScan(*runtimeViewer_,edgeId);check(!runtimeScan.isEmpty(),"Runtime current resolved reference scan layer");
            const auto edgeResult=std::find_if(runtimeResult_->tools.cbegin(),runtimeResult_->tools.cend(),[&](const auto& r){return r.toolId==edgeId;});
            check(edgeResult!=runtimeResult_->tools.cend() && edgeResult->outputs["point"].valid && QLineF(std::get<QPointF>(edgeResult->outputs["point"].data),{140,155}).length()<.3,"Runtime actual rotated edge oracle");
            if(label=="raw")check(QLineF((runtimeScan[0]+runtimeScan[3])/2,{150,155}).length()<.3,"Runtime scan current position independent oracle");
            setWindowState(Qt::WindowNoState);
            for(int w:{1400,850}){resize(w,w==1400 ? 900:600);QApplication::processEvents();runtimeViewer_->fitImage();QApplication::processEvents();grab().save(QDir(artifacts).filePath("r7-"+kind+"-"+label+"-runtime-"+QString::number(w)+".png"));}
            const auto records=ProductionStore(service_.path()+".production").recent("cycles");
            check(records.first()["schemaVersion"]==4 && ProductionResultCodec::csv(records).contains("referenceBinding"),"Reference UI restart CSV");
            findChild<QPushButton*>("runtime_stop")->click();wait();check(!runtimeResult_ && !runtimeViewer_->roi() && !runtimeViewer_->hasImage(),"Reference Runtime Stop clears");
            selectPage("logs");refreshLogs();logTable_->selectRow(0);QApplication::processEvents();
            auto* detail=findChild<QTextEdit*>("logDetail");check(detail->toPlainText().contains("Referans") && detail->toPlainText().contains("referenceBindings"),"Logs reference identity");
            for(int w:{1400,850}){resize(w,w==1400 ? 900:600);QApplication::processEvents();grab().save(QDir(artifacts).filePath("r7-"+kind+"-"+label+"-logs-"+QString::number(w)+".png"));}
            check(pixels.save(imagePath),"Restore Runtime teaching raster");resize(1400,900);selectPage("editor");refreshWorkspace();runFlow();wait();selectTool(edgeId);
        };
        if(type==ApplicationType::Measurement)runtimeUi("raw");
        if(type==ApplicationType::Measurement){
            resize(1400,900);horizontal_->setSizes({155,750,300});vertical_->setSizes({540,240});findChild<QTabWidget*>("editorTabs")->setCurrentIndex(1);imageViewer_->fitImage();QApplication::processEvents();
            auto* scroll=findChild<QScrollArea*>("propertiesScroll");scroll->ensureWidgetVisible(propertiesBody_->findChild<QPushButton*>("teachPoseReference"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-tracking.png"));
            resize(850,600);QApplication::processEvents();imageViewer_->fitImage();scroll->ensureWidgetVisible(propertiesBody_->findChild<QComboBox*>("poseReferencePicker"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-tracking-small.png"));if(scroll->horizontalScrollBar()->maximum()!=0)QTextStream(stderr)<<"Small UI: "<<width()<<'x'<<height()<<" overflow="<<scroll->horizontalScrollBar()->maximum()<<'\n';check(width()==850 && height()==600 && scroll->horizontalScrollBar()->maximum()==0,"pose picker small window layout");
            selectTool(poseId);QApplication::processEvents();scroll->ensureWidgetVisible(propertiesBody_->findChild<QDoubleSpinBox*>("pose_angleMin"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-teaching-small.png"));scroll->ensureWidgetVisible(propertiesBody_->findChild<QDoubleSpinBox*>("pose_anchor_x"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-anchor-small.png"));resize(1400,900);
            if(auto* toggle=propertiesBody_->findChild<QPushButton*>("circle_fit/referenceDetailsToggle"))toggle->setChecked(true);
            const auto circleId=consumers[3];selectTool(circleId);propertiesBody_->findChild<QPushButton*>("clearPoseReference")->click();wait();
            service_.setCalibrationSetup("lens","plane");const auto frame=*editorSession_.frame;const auto setup=CalibrationWorkflow::installation(*service_.activeRecipe(),frame);
            auto sample=[&](QString name,QPointF a,QPointF b,double mm){return CalibrationSample{name,{a,setup.source},{b,setup.source},mm};};
            const auto scale=MeasurementService{}.validate(CalibrationWorkflow::teach(*service_.activeRecipe(),frame,"pose-ui-scale",1,sample("teach",{0,0},{100,0},10)),{sample("x",{10,20},{110,20},10),sample("y",{30,10},{30,110},10)},.001);
            const auto catalog=service_.path()+".calibrations.json";CalibrationService(catalog).save(scale);CalibrationWorkflow::bind(service_,frame,catalog,"pose-ui-scale");
            auto worldCircle=configured(circleId);worldCircle.parameters["mode"]="calibrated";worldCircle.parameters["scanRoi"]=ToolRoi{scale.dependency().target,Annulus{{9.5,-12.5},.5,1.5}}.toJson();worldCircle.parameters["unitsPerSample"]=.1;worldCircle.parameters["scanWidth"]=.1;service_.updateTool(worldCircle);refreshWorkspace();runFlow();wait();selectTool(circleId);
            auto* picker=propertiesBody_->findChild<QComboBox*>("poseReferencePicker");picker->setCurrentIndex(picker->findData(poseId+"/pose"));propertiesBody_->findChild<QDoubleSpinBox*>("trackingMmPerSample")->setValue(.1);propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();wait();
            if(!toolReferenceBinding(configured(circleId)).world)QTextStream(stderr)<<"World teaching failure: "<<workspaceError_->text()<<'\n'<<QJsonDocument(configured(circleId).parameters).toJson(QJsonDocument::Compact)<<'\n';check(toolReferenceBinding(configured(circleId)).world,"UI nominal mm circle teaches separate world envelope");
            runExecution(circleId);wait();e=editorSession_.evaluation(editorContext());check(e.cache.contains(circleId) && e.cache[circleId].result.outputs["circleMm"].valid,"UI world circle feature executes through shared map");
            check(std::abs(std::get<Circle2D>(e.cache[circleId].result.outputs["circleMm"].data).radius-1)<.03 && e.cache[circleId].result.outputs["accepted"].valid && std::get<bool>(e.cache[circleId].result.outputs["accepted"].data),"UI world metric has actual radius and evaluated tool tolerance");
            propertiesBody_->findChild<QPushButton*>("circle_fit/referenceDetailsToggle")->setChecked(true);QApplication::processEvents();
            scroll->ensureWidgetVisible(propertiesBody_->findChild<QDoubleSpinBox*>("trackingMmPerSample"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-world-tracking.png"));
            resize(850,600);QApplication::processEvents();imageViewer_->fitImage();scroll->ensureWidgetVisible(propertiesBody_->findChild<QDoubleSpinBox*>("trackingMmPerSample"));QApplication::processEvents();grab().save(QDir(artifacts).filePath(kind+"-world-tracking-small.png"));check(scroll->horizontalScrollBar()->maximum()==0,"expanded world reference details fit small Properties");resize(1400,900);
            // Author a new nominal mm ROI without ever saving an unbound world
            // recipe. The same dialog also performs explicit world reteaching.
            for(const auto& id:QVector<QString>{edgeId,consumers[1],consumers[2],circleId}){
                selectTool(id);auto tool=configured(id);
                if(tool.kind=="line_fit"){tool.parameters["polarity"]="falling";service_.updateTool(tool);refreshWorkspace();selectTool(id);}
                auto* choice=propertiesBody_->findChild<QComboBox*>("poseReferencePicker");choice->setCurrentIndex(choice->findData(poseId+"/pose"));
                const auto beforeAuthor=service_.activeRecipe()->definition;
                propertiesBody_->findChild<QPushButton*>("editWorldTrackingRoi")->click();QApplication::processEvents();auto* dialog=findChild<QDialog*>("worldTrackingRoiDialog");check(dialog && dialog->isVisible(),"nominal mm authoring dialog opens");
                auto set=[&](const char* key,double value){auto* n=dialog->findChild<QDoubleSpinBox*>(QString("world_roi_")+key);check(n,"world author field exists");n->setValue(value);};
                if(tool.kind=="edge_point" || tool.kind=="edge_pair"){set("ax",9.5);set("bx",9.5);set("ay",tool.kind=="edge_point" ? -5:-8.5);set("by",tool.kind=="edge_point" ? -7:-10.5);set("width",.2);}
                else if(tool.kind=="line_fit"){set("cx",11.5);set("cy",-6);set("width",2.4);set("height",2);set("angle",0);}
                else {set("cx",9.5);set("cy",-12.5);set("inner",.5);set("outer",1.5);set("start",30);set("sweep",-220);}
                set("mmPerSample",.1);
                check(service_.activeRecipe()->definition==beforeAuthor,"pending world coordinates never mutate recipe");
                if(tool.kind=="edge_point"){
                    set("width",-1);check(dialog->findChild<QDoubleSpinBox*>("world_roi_width")->value()==.001,"world extent minimum is .001");set("by",-5);dialog->findChild<QPushButton*>("teachWorldRoi")->click();check(dialog->isVisible() && !dialog->findChild<QLabel*>("worldRoiError")->text().isEmpty() && service_.activeRecipe()->definition==beforeAuthor,"invalid world geometry rejected atomically");set("by",-7);set("width",.2);
                    dialog->grab().save(QDir(artifacts).filePath(kind+"-world-authoring.png"));
                }
                dialog->findChild<QPushButton*>("teachWorldRoi")->click();wait();check(toolReferenceBinding(configured(id)).world && toolReferenceBinding(configured(id)).sourceTool==poseId && validateGraph(service_.tools(),editorConnections(service_.activeRecipe()->definition)).valid(),"world authoring atomically teaches geometry and graph");
                runExecution(id);wait();e=editorSession_.evaluation(editorContext());check(e.cache.contains(id) && e.cache[id].result.state==ExecutionState::Done,"new nominal world ROI executes");
                const auto output=e.cache[id].result;
                if(tool.kind=="edge_point")check(output.outputs["point"].valid && QLineF(std::get<QPointF>(output.outputs["point"].data),{95,60}).length()<.3,"authored world edge actual raster oracle");
                else check(output.outputs["accepted"].valid && std::get<bool>(output.outputs["accepted"].data) && output.decision==PartDecision::NotEvaluated,"authored mm tool tolerance separate from part decision");
                if(tool.kind=="circle_fit")check(output.outputs["arcMm"].valid && std::get<Arc2D>(output.outputs["arcMm"].data).sweepDegrees<0,"authored negative world arc uses actual observations");
                const auto taughtDefinition=service_.activeRecipe()->definition;
                propertiesBody_->findChild<QPushButton*>("editWorldTrackingRoi")->click();QApplication::processEvents();dialog=findChild<QDialog*>("worldTrackingRoiDialog");check(dialog,"world reauthor opens");dialog->reject();wait();check(service_.activeRecipe()->definition==taughtDefinition,"cancel world reauthor preserves taught recipe");
            }
            service_.save();const auto worldDefinition=service_.activeRecipe()->definition;service_.openProject(path);service_.selectRecipe(recipeId);service_.save();check(service_.activeRecipe()->definition==worldDefinition && !service_.dirty(),"authored world ROI save reopen no-op");refreshWorkspace();runFlow();wait();selectTool(circleId);
            runtimeUi("world");
            // Stop must suppress even a queued successful teacher.
            const auto beforeStop=service_.activeRecipe()->definition;propertiesBody_->findChild<QPushButton*>("teachPoseReference")->click();findChild<QPushButton*>("stopFlow")->click();wait();check(service_.activeRecipe()->definition==beforeStop,"UI Stop prevents late teaching commit");
        }
        service_.save();
    }
    QTextStream(stdout)<<checks<<" pose tracking UI checks passed\n";return true;
}
}
