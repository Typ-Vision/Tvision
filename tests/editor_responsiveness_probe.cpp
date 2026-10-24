#include "app/MainWindow.hpp"
#include "app/ImageViewer.hpp"
#include "application/EditorObservation.hpp"
#include "application/PlaneMappingService.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QSaveFile>
#include <QStackedWidget>
#include <QTextStream>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <stdexcept>

namespace visionflow {
namespace {
void requireProbe(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QJsonObject readObject(const QString& path){QFile file(path);requireProbe(file.open(QIODevice::ReadOnly),"read fixture");return QJsonDocument::fromJson(file.readAll()).object();}
void writeBytes(const QString& path,const QByteArray& bytes){QSaveFile file(path);requireProbe(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size() && file.commit(),"write fixture/report");}
void writeObject(const QString& path,const QJsonObject& object){writeBytes(path,QJsonDocument(object).toJson());}
class PaintObservation : public QObject {
public:
    QWidget* page;
    QElapsedTimer timer;
    bool armed=false;
    qint64 first=-1;
    explicit PaintObservation(QWidget* target):page(target){}
    bool eventFilter(QObject* object,QEvent* event) override {
        if(armed && first<0 && event->type()==QEvent::Paint)
            if(auto* widget=qobject_cast<QWidget*>(object);widget && (widget==page || page->isAncestorOf(widget)))first=timer.nsecsElapsed();
        return false;
    }
};
struct CountingProvider : IndustrialCameraProvider {
    mutable std::atomic_int capabilitiesCalls=0,creates=0;
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice&) const override {++capabilitiesCalls;return {};}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest&) const override {++creates;throw std::runtime_error("offline probe attempted camera create");}
};
double median(QVector<double> values){std::sort(values.begin(),values.end());const auto n=values.size();return n%2 ? values[n/2]:(values[n/2-1]+values[n/2])/2;}
double p95(QVector<double> values){std::sort(values.begin(),values.end());return values[static_cast<int>(std::ceil(.95*values.size()))-1];}
}

bool MainWindow::verifyEditorResponsiveness(const QString& artifacts){
    using namespace editor_observation;
    smokeMode_=true;
    const bool contract=QCoreApplication::arguments().contains("--probe-contract");
    const int repetitions=contract ? 2:20;
    const QStringList cases{"framed_7x7_multi","legacy_23a_multi","uncalibrated_single","uncalibrated_multi5"};
    requireProbe(QDir().mkpath(artifacts),"create probe evidence directory");
    QFile csv(QDir(artifacts).filePath("raw.csv")),failures(QDir(artifacts).filePath("failures.csv"));
    requireProbe(csv.open(QIODevice::WriteOnly|QIODevice::Truncate) && failures.open(QIODevice::WriteOnly|QIODevice::Truncate),"open CSV");
    QTextStream raw(&csv),failed(&failures);
    raw<<"case,operation,iteration,warmup,success,operation_ms,first_editor_paint_ms,queued_event_ms,tools_before,tools_after,dirty_before,dirty_after,provider_capabilities,provider_creates,discoveries";
    for(int i=0;i<Count;++i)raw<<','<<names[i]<<"_calls,"<<names[i]<<"_inclusive_ms";
    raw<<'\n';failed<<"case,operation,iteration,error\n";
    QJsonArray summaries,manifest;
    // Deterministic observer contract: nested bindings restore the previous sink.
    Sample outer,inner;
    {Binding binding(outer);{Scope scope(Context);}{Binding nested(inner);Scope scope(Context);}{Scope scope(Context);}}
    requireProbe(!active && outer.calls[Context]==2 && inner.calls[Context]==1,"observation binding isolation");
    int failureCount=0;
    for(const auto& name:cases){
        const auto directory=QDir(artifacts).filePath(name);requireProbe(QDir().mkpath(directory),"case directory");
        const auto path=QDir(directory).filePath("fixture.vfproject");
        const bool old=name.startsWith("legacy") || name.startsWith("framed");
        service_.discardChanges();service_=ProjectService{};measurementWorkspace_.reset();
        if(old){
            for(const auto suffix:{"",".active-calibrations.json",".measurement-environment.json"}){
                QFile input(QString(VISIONFLOW_PROBE_FIXTURE)+"/copy.vfproject"+suffix);requireProbe(input.open(QIODevice::ReadOnly),"tracked legacy fixture available");writeBytes(path+suffix,input.readAll());
            }
            if(name.startsWith("framed")){
                auto catalog=readObject(path+".active-calibrations.json");auto records=catalog["records"].toArray();
                auto seed=CalibrationModelRecord::decode(records[0].toObject());
                auto grid=PlaneMappingService::decode(MeasurementService{}.encode(seed.model())["mapping"].toObject()).observation();
                grid.pixelHash=seed.image().pixelHash;
                if(seed.image().experimental)grid.source.experimentalAcquisition=seed.image().experimental->acquisition;
                grid.target={"probe-framed-7x7",7,7,12,CalibrationGridPattern::FramedCircles,FramedPlateGeometry{}};
                grid.pixels.clear();grid.worldMillimetres.clear();QVector<PlateCentre> centres;
                for(int r=0;r<7;++r)for(int c=0;c<7;++c){QPointF point(200+200*c,200+200*r);grid.pixels.append(point);grid.worldMillimetres.append({12.*c,12.*r});centres.append({r,c,point,{}});}
                auto observation=RawPlateObservation::create(grid,centres,{PlateOrientation::TriangleCorner,0,false,QPointF(100,100)},"probe-structural/1");
                auto record=CalibrationModelRecord::create(MeasurementService{}.teachHomography(seed.model().dependency(),PlaneMappingService::fit(observation)),seed.setup(),seed.image());
                records[0]=record.encode();catalog["records"]=records;writeObject(path+".active-calibrations.json",catalog);
                auto environment=readObject(path+".measurement-environment.json");auto selections=environment["selections"].toArray();
                for(int i=0;i<selections.size();++i){auto selection=selections[i].toObject();if(selection["contentId"]==seed.contentId()){selection["contentId"]=record.contentId();selections[i]=selection;}}
                environment["selections"]=selections;writeObject(path+".measurement-environment.json",environment);
            }
            service_.openProject(path);service_.selectRecipe(service_.project().recipes.first().id);
        }else{
            // A fresh filename on rerun is written atomically through the repository below.
            ProjectService fresh;const auto temp=path+".new";QFile::remove(temp);
            fresh.createProject(name,"offline responsiveness fixture",temp,"");fresh.createRecipe("Measurement","",ApplicationType::Measurement);
            fresh.enableMultiSource();for(int i=0;i<(name.endsWith("multi5") ? 5:1);++i)fresh.addTool("image_file");fresh.save();
            QFile source(temp);requireProbe(source.open(QIODevice::ReadOnly),"read fresh project");writeBytes(path,source.readAll());source.close();QFile::remove(temp);
            service_.openProject(path);service_.selectRecipe(service_.project().recipes.first().id);
        }
        const auto pre=service_.addTool("preprocess"),roi=service_.addTool("code_read");
        selectedToolId_=pre;measurementWorkspace();
        auto provider=std::make_shared<CountingProvider>();int discoveries=0;
        IndustrialExecutionInputs composition;composition.contract=measurementWorkspace_->environment().current;composition.factory=CameraProviderFactory::forTests(provider);
        composition.discover=[&discoveries](int,const std::shared_ptr<std::atomic_bool>&){++discoveries;return IndustrialDiscoverySnapshot{};};
        editorCameraComposition_=composition;
        refreshWorkspace();selectPage("editor");selectTool(pre);requireProbe(applyToolProperties(),"normalize initial Properties");service_.save();
        const Project baseline=service_.project();const auto recipeId=service_.activeRecipe()->id;
        manifest.append(QJsonObject{{"case",name},{"project",path},{"sources",name.endsWith("multi5") ? 5:old ? 2:1},{"models",old ? 2:0},{"framedStructuralModel",name.startsWith("framed")},{"cameraAcquisition",false}});
        PaintObservation paint(pages_->widget(pageIds_.indexOf("editor")));qApp->installEventFilter(&paint);
        const QStringList operations{"save_dirty","save_noop","parameter","add","roi"};
        for(const auto& operation:operations){
            QVector<double> opTimes,paintTimes,eventTimes;
            for(int iteration=0;iteration<=repetitions;++iteration){
                // File copy/normalization and equivalent recipe restoration are never timed.
                service_.discardChanges();ProjectRepository{}.save(path,baseline);service_=ProjectService{};service_.openProject(path);service_.selectRecipe(recipeId);
                selectedToolId_=operation=="roi" ? roi:pre;editorSession_.frame.reset();pendingDrawTool_.clear();pendingDrawRecipe_.clear();
                refreshWorkspace();selectTool(selectedToolId_);
                if(operation=="save_dirty"){auto* edit=propertiesBody_->findChild<QLineEdit*>("toolName");requireProbe(edit,"name field");edit->setText("Dirty sample");requireProbe(applyToolProperties() && service_.dirty(),"dirty Save preparation");}
                if(operation=="add")for(int i=0;i<toolbox_->count();++i)if(toolbox_->item(i)->data(Qt::UserRole)=="preprocess"){toolbox_->setCurrentRow(i);break;}
                if(operation=="roi"){
                    QImage image(640,480,QImage::Format_Grayscale8);image.fill(123);editorSession_.frame=ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),image.width(),image.height(),image.bytesPerLine(),PixelFormat::Gray8);
                    drawingTool_=roi;modelDrawing_=false;
                }
                QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();
                requireProbe(operation=="save_dirty" || !service_.dirty(),"clean equivalent initial state");
                const auto before=service_.tools().size();const bool dirtyBefore=service_.dirty();const auto definitionBefore=service_.activeRecipe()->definition;
                const auto capabilitiesBefore=provider->capabilitiesCalls.load(),createsBefore=provider->creates.load(),discoveriesBefore=discoveries;
                Sample sample;paint.first=-1;paint.armed=true;paint.timer.start();qint64 queued=-1;bool waiting=true;
                QTimer::singleShot(0,&paint,[&]{if(waiting)queued=paint.timer.nsecsElapsed();});
                bool success=true;QString error;
                qint64 operationNs=0;
                {Binding binding(sample);try{
                    if(operation.startsWith("save"))saveProject();
                    else if(operation=="parameter"){auto* channel=propertiesBody_->findChild<QComboBox*>("preprocessChannel");requireProbe(channel && channel->count()>1,"parameter field");channel->setCurrentIndex((channel->currentIndex()+1)%channel->count());}
                    else if(operation=="add")addSelectedTool();
                    else commitRoi(QRect(20,30,120,90));
                }catch(const std::exception& e){success=false;error=QString::fromUtf8(e.what());}operationNs=paint.timer.nsecsElapsed();}
                QElapsedTimer dispatch;dispatch.start();
                while((queued<0 || paint.first<0) && dispatch.elapsed()<5000)QApplication::processEvents();
                waiting=false;paint.armed=false;
                try{
                    requireProbe(queued>=0 && paint.first>=0,"missing queued callback or relevant editor paint");
                    requireProbe(provider->creates==createsBefore && provider->capabilitiesCalls==capabilitiesBefore && discoveries==discoveriesBefore,"ordinary edit touched hardware factory");
                    requireProbe(operation=="add" ? service_.tools().size()==before+1:service_.tools().size()==before,"equivalent tool count / actual Add");
                    if(operation.startsWith("save")){
                        requireProbe(!service_.dirty(),"Save completed");requireProbe(sample.calls[ProjectSave]==1,"one service Save");
                        requireProbe(sample.calls[AtomicWrite]==(operation=="save_dirty" ? 1u:0u),"dirty/no-op atomic write semantics");
                        requireProbe(ProjectRepository{}.load(path).recipes.first().definition==service_.project().recipes.first().definition,"saved recipe roundtrip");
                    }else requireProbe(service_.dirty() && service_.activeRecipe()->definition!=definitionBefore,"edit actually changed serialized recipe");
                    requireProbe(sample.calls[Calibration]==0 && sample.calls[Runtime]==0,"hidden pages are not rebuilt");
                    requireProbe(sample.calls[Context]>0 && sample.calls[Workspace]==1 && sample.calls[Editor]==1,"one refresh per Editor action");
                    requireProbe(old ? sample.calls[CatalogRead]>0 && sample.calls[CatalogDecode]>0 && sample.calls[ModelDecode]>0 && sample.calls[PlaneReplay]>0 : sample.calls[ModelDecode]==0 && sample.calls[PlaneReplay]==0,"catalog/decode/replay counter semantics");
                }catch(const std::exception& e){success=false;error+=QString::fromUtf8(e.what());}
                raw<<name<<','<<operation<<','<<iteration<<','<<(iteration==0)<<','<<success<<','<<operationNs/1e6<<','<<paint.first/1e6<<','<<queued/1e6<<','<<before<<','<<service_.tools().size()<<','<<dirtyBefore<<','<<service_.dirty()<<','<<provider->capabilitiesCalls-capabilitiesBefore<<','<<provider->creates-createsBefore<<','<<discoveries-discoveriesBefore;
                for(int i=0;i<Count;++i)raw<<','<<sample.calls[i]<<','<<sample.nanoseconds[i]/1e6;
                raw<<'\n';raw.flush();
                if(!success){++failureCount;failed<<name<<','<<operation<<','<<iteration<<",\""<<error.replace('"',"'")<<"\"\n";failed.flush();}
                else if(iteration){opTimes.append(operationNs/1e6);paintTimes.append(paint.first/1e6);eventTimes.append(queued/1e6);}
            }
            QJsonObject summary{{"case",name},{"operation",operation},{"successfulMeasuredSamples",opTimes.size()},{"requestedSamples",repetitions}};
            if(!opTimes.isEmpty())summary.insert("timings",QJsonObject{{"operationMedianMs",median(opTimes)},{"operationP95Ms",p95(opTimes)},{"paintMedianMs",median(paintTimes)},{"paintP95Ms",p95(paintTimes)},{"queuedEventMedianMs",median(eventTimes)},{"queuedEventP95Ms",p95(eventTimes)}});
            summaries.append(summary);QTextStream(stdout)<<name<<' '<<operation<<" samples="<<opTimes.size()<<'\n'<<Qt::flush;
        }
        qApp->removeEventFilter(&paint);
        for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();requireProbe(grab().save(QDir(directory).filePath(QString("editor-%1.png").arg(size.width()))),"probe screenshot");}
        service_.discardChanges();editorCameraComposition_.reset();
    }
    writeObject(QDir(artifacts).filePath("summary.json"),{{"contractOnly",contract},{"warmupsPerCaseOperation",1},{"repetitions",repetitions},{"p95Method","nearest rank ceil(0.95*n)"},{"paintMeaning","first Paint event in visible Editor subtree; offscreen is not physical presentation"},{"queuedMeaning","zero-delay callback posted before synchronous action; elapsed time includes action and queue dispatch"},{"scopeMeaning","owner UI thread, inclusive durations overlap; catalog_read counts calls including missing-file checks; atomic_write covers QSaveFile open/write/commit"},{"failures",failureCount},{"cases",manifest},{"results",summaries}});
    service_.discardChanges();return failureCount==0;
}
}
