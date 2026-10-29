#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "application/RuntimeOverlay.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QJsonArray>
#include <QGraphicsScene>
#include <QPainter>
#include <QMouseEvent>
#include <QSettings>
#include <QTextStream>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTableWidget>
#include <QLabel>
#include <QThread>
namespace visionflow {
bool MainWindow::verifyRuntimeOverlays(const QString& artifacts){
    int checks=0;auto check=[&](bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);};
    const auto path=QDir(artifacts).filePath("ccux-24h");QDir().mkpath(path);
    QImage image(800,500,QImage::Format_Grayscale8);image.fill(12);{QPainter p(&image);p.setPen(Qt::NoPen);p.setBrush(Qt::white);p.drawEllipse(QPoint(200,200),80,80);p.drawRect(430,100,200,280);}check(image.save(path+"/mixed.png"),"Mixed actual raster");
    QTemporaryDir fixture(path+"/project-XXXXXX");check(fixture.isValid(),"Unique project fixture");fixture.setAutoRemove(false);
    service_.createProject("24H layers","",fixture.filePath("mixed.vfproject"),"file");service_.createRecipe("Circle Line Edge Blob","",ApplicationType::DefectInspection);service_.enableMultiSource();QVector<ToolConnection> links;QStringList sourceIds,preprocessIds;
    for(int i=0;i<2;++i){const auto source=service_.addTool("image_file");service_.setImageFile(path+"/mixed.png",source);sourceIds<<source;const auto prep=service_.addTool("preprocess");preprocessIds<<prep;for(auto t:service_.tools())if(t.id==prep){t.parameters["steps"]=QJsonArray{QJsonObject{{"op","threshold"},{"size",3},{"value",128.}}};service_.updateTool(t);}links<<ToolConnection{source,"image",prep,"image"};
        for(const auto& kind:QStringList{"circle_fit","line_fit","edge_point","blob"}){const auto id=service_.addTool(kind);for(auto t:service_.tools())if(t.id==id){t.parameters["mode"]="pixel";if(kind=="circle_fit"){t.parameters=defaultParameters(kind);t.parameters["scanRoi"]=ToolRoi{{source+"/image"},Annulus{{200,200},65,95}}.toJson();t.parameters["maxRms"]=1.;t.parameters["maxResidual"]=2.;}else if(kind=="line_fit")t.parameters["scanRoi"]=ToolRoi{{source+"/image"},RotatedRectangle{{520,100},{160,50},0}}.toJson();else if(kind=="edge_point"){t.parameters.remove("mode");t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{350,220},{700,220},4}}.toJson();}else t.parameters.remove("mode");service_.updateTool(t);}links<<ToolConnection{prep,"image",id,"image"};}
    }
    service_.setConnections(links);service_.save();refreshWorkspace();ensureRuntime();runtimeCoordinator_->load(*service_.activeRecipe(),QFileInfo(service_.path()).absolutePath());selectPage("runtime");runtimeAction("run");runtimeAction("cycle");
    auto wait=[&]{QElapsedTimer t;t.start();while(runtimeJobs_.busy() && t.elapsed()<30000){QApplication::processEvents();QThread::msleep(1);}QApplication::processEvents();check(!runtimeJobs_.busy(),"Runtime finished");};wait();
    if(runtimeResult_ && runtimeResult_->state!=ExecutionState::Done)for(const auto& t:runtimeResult_->tools)if(t.state!=ExecutionState::Done)QTextStream(stderr)<<t.toolId<<": "<<t.message<<'\n';
    check(runtimeResult_ && runtimeResult_->state==ExecutionState::Done,"Mixed real tools completed");
    auto* all=findChild<QCheckBox*>("runtimeAllTools");all->setChecked(true);showRuntimeResult();const auto before=*runtimeResult_;const auto snapshot=runtime_->active();const auto content=ProductionRecipeService::recipeContent(snapshot->recipe(),snapshot->projectDirectory());
    auto ids=[&]{QSet<QString> result;for(const auto* item:runtimeViewer_->scene()->items())if(item->data(0)=="runtimeOverlay")result.insert(item->data(1).toString());return result;};
    for(int i=0;i<2;++i){runtimeSources_->setCurrentIndex(i);showRuntimeResult();check(ids().size()==4,"Four independent layers per source");check(!runtimeViewer_->roi() && !runtimeViewer_->roiGestureActive(),"Read only collection has no editable ROI");
        const auto graph=validateSourceGraph(editorTools(snapshot->recipe().definition),editorConnections(snapshot->recipe().definition));for(const auto& id:ids())check(graph.toolSources.value(id)==sourceIds[i],"No cross source layer");
    }
    runtimeSources_->setCurrentIndex(0);all->setChecked(false);runtimeTools_->setCurrentIndex(2);showRuntimeResult();check(ids().size()==1 && ids().contains(runtimeTools_->currentData().toString()),"Selected tool focus");
    auto background=[&]{for(const auto* item:runtimeViewer_->scene()->items())if(item->data(0)!="runtimeOverlay")if(const auto* pix=qgraphicsitem_cast<const QGraphicsPixmapItem*>(item))return pix->pixmap().toImage().pixelColor(0,0).red();return -1;};
    runtimeTools_->setCurrentIndex(runtimeTools_->findData(preprocessIds[0]));showRuntimeResult();check(background()==0 && ids().isEmpty(),"Selected processed preview and overlay swap together");all->setChecked(true);check(background()==12 && ids().size()==4,"All-tools switches atomically to raw with all layers");all->setChecked(false);
    {QSettings settings(settingsFile_,QSettings::IniFormat);check(!settings.value("runtime/allToolOverlays",true).toBool(),"Local preference persisted");}
    {MainWindow restored(settingsFile_);check(!restored.findChild<QCheckBox*>("runtimeAllTools")->isChecked(),"Restart reads local preference");}
    all->setChecked(true);showRuntimeResult();check(RuntimeService::sameCompletion(before,*runtimeResult_) && before.acquisitions==runtimeResult_->acquisitions && content==ProductionRecipeService::recipeContent(snapshot->recipe(),snapshot->projectDirectory()) && !service_.dirty(),"Toggle preserves completion hash acquisitions and dirty state");
    int edits=0;runtimeViewer_->onRoiEdited=[&]{++edits;};
    const QPointF press=runtimeViewer_->mapFromScene(QPointF(120,200)),release=press+QPointF(20,10);
    QMouseEvent down(QEvent::MouseButtonPress,press,press,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(runtimeViewer_->viewport(),&down);
    QMouseEvent move(QEvent::MouseMove,release,release,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(runtimeViewer_->viewport(),&move);
    QMouseEvent up(QEvent::MouseButtonRelease,release,release,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(runtimeViewer_->viewport(),&up);check(edits==0 && !runtimeViewer_->roiGestureActive() && !service_.dirty(),"Runtime drag cannot edit recipe geometry");runtimeViewer_->onRoiEdited={};
    for(const auto size:{QSize(1400,900),QSize(850,600)}){setWindowState(Qt::WindowNoState);resize(size);QApplication::processEvents();runtimeViewer_->fitImage();check(this->size()==size,"Requested fixture geometry");check(grab().save(path+QString("/mixed-%1.png").arg(size.width())),"Mixed screenshot");}
    const auto count=runtimeViewer_->scene()->items().size();QElapsedTimer timer;timer.start();for(int i=0;i<20;++i)showRuntimeResult();const auto elapsed=timer.nsecsElapsed()/1e6;check(runtimeViewer_->scene()->items().size()==count,"No item accumulation across refreshes");
    runtimeAction("cycle");check(ids().isEmpty() && !runtimeViewer_->hasImage(),"New cycle clears old presentation");wait();check(runtimeResult_ && runtimeResult_->cycleId!=before.cycleId && ids().size()==4 && runtimeViewer_->scene()->items().size()==count,"Second cycle replaces layers");
    const auto graph=validateSourceGraph(editorTools(snapshot->recipe().definition),editorConnections(snapshot->recipe().definition));ImageFrame raw;for(const auto& t:runtimeResult_->tools)if(t.toolId==sourceIds[0])raw=std::get<ImageFrame>(t.outputs["image"].data);
    const auto collection=runtimeOverlays(*runtimeResult_,editorTools(snapshot->recipe().definition),graph,raw,snapshot);check(collection.layers.size()==4,"Application collection");auto foreignSnapshot=std::make_shared<ProductionSnapshot>(*snapshot);check(runtimeOverlays(*runtimeResult_,editorTools(snapshot->recipe().definition),graph,raw,foreignSnapshot).layers.isEmpty(),"Different immutable snapshot with equal content rejected");auto stale=*runtimeResult_;stale.cycleId="other";check(runtimeOverlays(stale,editorTools(snapshot->recipe().definition),graph,raw,snapshot).layers.isEmpty(),"Stale cycle rejected");check(runtimeOverlays(*runtimeResult_,editorTools(snapshot->recipe().definition),graph,raw,{}).layers.isEmpty(),"Foreign snapshot rejected");
    stale=*runtimeResult_;for(auto& t:stale.tools)if(t.toolId==collection.layers.first().toolId)for(auto& v:t.outputs)v.origin.frameId="other";check(runtimeOverlays(stale,editorTools(snapshot->recipe().definition),graph,raw,snapshot).layers.size()==3,"Foreign frame result rejected");
    stale=*runtimeResult_;for(auto& t:stale.tools)t.state=ExecutionState::Skipped;check(runtimeOverlays(stale,editorTools(snapshot->recipe().definition),graph,raw,snapshot).layers.isEmpty(),"Skipped results never invent geometry");
    auto sameNames=collection;for(auto& layer:sameNames.layers)layer.name="Aynı etiket";runtimeViewer_->setRuntimeOverlays(sameNames);check(ids().size()==4,"Repeated presentation labels preserve tool ID separation");
    auto dense=collection;dense.layers.clear();for(int k=0;k<4;++k){RuntimeOverlayLayer layer;layer.toolId=QString::number(k);layer.name="Dense "+layer.toolId;layer.color=QColor::fromHsv(k*80,170,255);for(int i=0;i<2048;++i)layer.points<<QPointF(20+i%740,20+(i/740)*30+k*60);dense.layers<<layer;}
    QVector<QPointF> baselinePoints;for(const auto& layer:dense.layers)baselinePoints+=layer.points;
    QElapsedTimer benchmark;benchmark.start();runtimeViewer_->setToolOverlay({},baselinePoints,{}, {},{},{},{},{},true);QApplication::processEvents();const auto baseline=benchmark.nsecsElapsed()/1e6;
    benchmark.restart();runtimeViewer_->setRuntimeOverlays(dense);QApplication::processEvents();const auto denseMs=benchmark.nsecsElapsed()/1e6;const auto denseItems=runtimeViewer_->scene()->items().size();runtimeViewer_->setRuntimeOverlays(dense);QApplication::processEvents();check(runtimeViewer_->scene()->items().size()==denseItems,"8192 points collection remains bounded");
    for(const auto* item:runtimeViewer_->scene()->items())if(item->data(0)=="runtimeOverlay")check(item->type()!=QGraphicsLineItem::Type,"Independent points never joined into distance lines");
    showRuntimeResult();
    runtimeAction("stop");check(ids().isEmpty() && !runtimeViewer_->hasImage(),"Stop clears layers");
    selectPage("logs");refreshLogs();check(logTable_->rowCount()>0,"Historical results available");logTable_->selectRow(0);QApplication::processEvents();
    check(findChild<QLabel*>("logRasterNotice")->text().contains("Görüntü mevcut değil") && findChild<QTextEdit*>("logDetail")->toPlainText().contains("Görüntü mevcut değil"),"Logs explicitly reports unavailable historical raster");
    check(!runtimeViewer_->hasImage() && ids().isEmpty() && !runtimeResult_,"Selecting historical result cannot restore raster or overlays");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){resize(size);QApplication::processEvents();check(grab().save(path+QString("/logs-no-raster-%1.png").arg(size.width())),"Historical raster notice screenshot");}
    QFile report(path+"/render.txt");check(report.open(QIODevice::WriteOnly),"Render report");QTextStream(&report)<<"800x500; 4 tools/source; 2 sources; scene items="<<count<<"; 20 refresh ms="<<elapsed<<"; legacy single 8192-point presentation ms="<<baseline<<"; collection 4x2048 points ms="<<denseMs<<"; dense scene items="<<denseItems<<"; checks="<<checks<<'\n';QTextStream(stdout)<<checks<<" Runtime overlay checks passed\n";return true;
}
}
