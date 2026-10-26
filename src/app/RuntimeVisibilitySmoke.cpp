#include "MainWindow.hpp"
#include "ImageViewer.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <cmath>

namespace visionflow {
bool MainWindow::verifyRuntimeVisibility(const QString& artifacts){
    int checks=0;auto check=[&](bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);};
    auto until=[&](auto ready,const char* text){QElapsedTimer timer;timer.start();while(!ready() && timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}check(ready(),text);};
    QTemporaryDir dir(QDir(artifacts).filePath("d4-visibility-XXXXXX"));check(dir.isValid(),"D4 fixture directory");dir.setAutoRemove(false);
    auto shot=[&](const QString& name){
        auto* parent=runtimeSummary_->parentWidget();while(parent && !qobject_cast<QScrollArea*>(parent))parent=parent->parentWidget();auto* scroll=qobject_cast<QScrollArea*>(parent);check(bool(scroll),"Runtime results scroll area");scroll->verticalScrollBar()->setValue(0);
        setWindowState(Qt::WindowNoState);resize(1400,900);QApplication::processEvents();runtimeViewer_->fitImage();check(grab().save(dir.filePath(name+"-1400.png")),"D4 normal screenshot");resize(850,600);QApplication::processEvents();check(width()==850 && height()==600 && grab().save(dir.filePath(name+"-850.png")),"D4 small screenshot");
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());QApplication::processEvents();check(grab().save(dir.filePath(name+"-850-results.png")),"D4 small results accessible by scrolling");scroll->verticalScrollBar()->setValue(0);
    };
    QImage image(1000,600,QImage::Format_Grayscale8);image.fill(0);for(int y=100;y<400;++y)for(int x=200;x<600;++x)image.scanLine(y)[x]=255;
    const auto file=dir.filePath("part.png");check(image.save(file),"Actual PNG");
    for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
        service_.createProject("D4 visibility","",dir.filePath(typeKey(type)+".vfproject"),"file");service_.createRecipe("Immutable Runtime","",type,"file");
        const auto source=service_.setImageFile(file),left=service_.addTool("edge"),right=service_.addTool("edge"),metric=service_.addTool("measurement");
        for(auto tool:service_.tools()){if(tool.id==right)tool.parameters["direction"]="right_to_left";if(tool.id==metric){tool.parameters["lowerMm"]=9.97;tool.parameters["upperMm"]=9.98;}service_.updateTool(tool);}
        service_.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",metric,"p1"},{right,"point",metric,"p2"}});service_.setCalibrationSetup("lens","plane");
        const auto frame=RuntimeService::probe(*service_.activeRecipe(),dir.path(),{});const auto installation=CalibrationWorkflow::installation(*service_.activeRecipe(),frame);
        auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,installation.source},{b,installation.source},mm};};
        const auto catalog=service_.path()+".calibrations.json";
        const auto calibration=MeasurementService{}.validate(CalibrationWorkflow::teach(*service_.activeRecipe(),frame,"scale",1,sample("fit",{0,0},{400,0},10)),{sample("x",{10,30},{810,30},20),sample("y",{40,10},{40,510},12.5)},.001);
        CalibrationService(catalog).save(calibration);CalibrationWorkflow::bind(service_,frame,catalog,"scale");service_.save();refreshWorkspace();until([&]{return !editorSession_.busy();},"Editor idle");ensureRuntime();
        const auto projectId=service_.project().id;const auto recipe=*service_.activeRecipe();const auto tools=service_.tools();
        auto selection=[&](const QString& id,const OutputVisibilityOverride& value){check(outputPreferences_.save({projectId,recipe.id,id},value).ok(),"Persist local selection");};
        // These connected points are hidden before execution, not removed from the graph.
        for(const auto& tool:tools)selection(tool.id,QSet<QString>{});
        runtimeCoordinator_->load(recipe,dir.path());selectPage("runtime");runtimeAction("run");check(!runtimeResult_ && !runtimeViewer_->hasImage() && !runtimeSummary_->text().contains("PASS"),"No invented initial value/decision");
        runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);const auto actual=RuntimeService::execute(runtimeCycle_,catalog,runtimeCoordinator_->cancellation());
        check(actual.state==ExecutionState::Done,"Real calibrated execution with hidden connected ports");
        for(const auto& result:actual.tools)if(result.toolId==metric)check(result.outputs["value"].valid && std::abs(std::get<double>(result.outputs["value"].data)-9.975)<1e-9 && result.decision==PartDecision::Pass,"Hidden upstream point still yields actual downstream scalar");
        runtimeResult_=actual;showRuntimeResult();check(!runtimeSummary_->text().contains(actual.cycleId) && !runtimeSummary_->text().contains(actual.frameId),"CCUX24B routine result omits cycle/frame identities");runtimeTools_->setCurrentIndex(runtimeTools_->findData(metric));showRuntimeResult();
        check(runtimeSummary_->text().contains("Parça kararı: PASS") && !runtimeSummary_->text().contains("  value:") && !runtimeSummary_->text().contains("Ölçüm:") && runtimeViewer_->hasImage(),"Explicit empty retains decision and actual viewer, no scalar summary leak");
        const auto signature=flowSignature();const auto generation=editorSession_.generation;const auto dirty=service_.dirty();const auto content=ProductionRecipeService::recipeContent(recipe,dir.path());
        selection(metric,QSet<QString>{"value"});showRuntimeResult();check(runtimeSummary_->text().contains("9.975000 mm • PASS"),"Explicit value shown from actual result");
        selection(metric,QSet<QString>{});showRuntimeResult();
        // A different active Editor recipe must never supply Runtime identity/defaults.
        const auto editorState=service_;const auto editorOnly=service_.createRecipe("Editor only","",type,"file");check(outputPreferences_.showAll({projectId,editorOnly,metric},"measurement").ok(),"Different Editor recipe preference");
        showRuntimeResult();check(!runtimeSummary_->text().contains("  value:") && runtimeSummary_->text().contains("Parça kararı: PASS"),"Runtime uses immutable recipe identity independent of active Editor");service_=editorState;
        ProjectService foreign;foreign.createProject("Foreign Editor project","",dir.filePath(typeKey(type)+"-foreign.vfproject"),"file");service_=foreign;
        check(outputPreferences_.showAll({service_.project().id,recipe.id,metric},"measurement").ok(),"Different project preference");showRuntimeResult();
        check(runtimeProjectId_==projectId && !runtimeSummary_->text().contains("  value:") && runtimeSummary_->text().contains("Parça kararı: PASS"),"Runtime project identity survives different Editor project");service_=editorState;
        check(RuntimeService::sameCompletion(actual,*runtimeResult_) && signature==flowSignature() && generation==editorSession_.generation && content==ProductionRecipeService::recipeContent(*service_.activeRecipe(),dir.path()),"Same typed payload, provenance and trusted digest after presentation");
        // Restore the independent test edit; visibility itself did not dirty this recipe.
        check(!dirty && !service_.dirty() && !service_.recipeDirty(recipe.id),"Visibility preserves recipe dirty state");
        completeRuntimeResult(*runtimeResult_,runtimeCoordinator_->cancellation());check(runtime_->records().size()==1 && !runtime_->busy(),"Original trusted completion accepted after visibility changes");
        selection(metric,QSet<QString>{"value"});showRuntimeResult();shot(typeKey(type)+"-value");selection(metric,QSet<QString>{});showRuntimeResult();shot(typeKey(type)+"-empty");
        const auto records=runtime_->records();const auto csv=ProductionResultCodec::csv(records);refreshLogs();const auto log=logTable_->item(0,0)->data(Qt::UserRole).toString();
        for(const auto& tool:tools)selection(tool.id,std::nullopt);showRuntimeResult();for(const auto& tool:tools)selection(tool.id,QSet<QString>{});showRuntimeResult();refreshLogs();
        check(records==runtime_->records() && csv==ProductionResultCodec::csv(runtime_->records()) && log==logTable_->item(0,0)->data(Qt::UserRole).toString() && RuntimeService::sameCompletion(actual,*runtimeResult_),"Same terminal JSON, Logs, CSV and typed completion after reset/empty");
        QFile evidence(dir.filePath(typeKey(type)+"-terminal.json"));check(evidence.open(QIODevice::WriteOnly),"Terminal evidence file");evidence.write(QJsonDocument(records.first()).toJson());
        if(type==ApplicationType::Measurement){
            // Real localhost vf.socket/2 delivery of the selected, hidden scalar.
            PlcRuntimeSocket socket(*runtime_,runtimePath_+".production",catalog);PlcSocketOptions options;options.port=0;options.measurementToolId=metric;options.ackTimeoutMs=10000;socket.listen(options);
            QByteArray input;QVector<QJsonObject> messages;QTcpSocket peer;QObject::connect(&peer,&QTcpSocket::readyRead,[&]{input+=peer.readAll();while(input.contains('\n')){const auto n=input.indexOf('\n');messages.append(QJsonDocument::fromJson(input.left(n)).object());input.remove(0,n+1);}});
            auto take=[&](const QString& type){QJsonObject found;try{until([&]{if(!found.isEmpty())return true;for(int i=0;i<messages.size();++i)if(messages[i]["type"]==type){found=messages.takeAt(i);return true;}return false;},qPrintable("Socket response "+type));}catch(...){QTextStream(stderr)<<"Expected "<<type<<", peer "<<peer.errorString()<<", server "<<socket.state()<<", messages ";for(const auto& message:messages)QTextStream(stderr)<<QJsonDocument(message).toJson(QJsonDocument::Compact);QTextStream(stderr)<<'\n';throw;}return found;};
            socket.completed=[&](const CycleResult& result){runtimeResult_=result;showRuntimeResult();};peer.connectToHost(QHostAddress::LocalHost,socket.port());const auto hello=take("HELLO");
            auto send=[&](QJsonObject value){value["v"]=2;value["sessionId"]=hello["sessionId"];peer.write(QJsonDocument(value).toJson(QJsonDocument::Compact)+'\n');};
            const QJsonObject request{{"type","TRIGGER"},{"requestId","hidden-scalar"},{"recipeId",recipe.id},{"configId",QString::fromLatin1(runtime_->active()->validation().contentId)}};
            send(request);take("BUSY");const auto response=take("DONE");check(response["resultValid"]==true && response["decision"]=="PASS" && std::abs(response["valueMm"].toDouble()-9.975)<1e-9 && !runtimeSummary_->text().contains("  value:") && runtimeSummary_->text().contains("Parça kararı: PASS"),"Hidden scalar still delivered over real localhost socket with UI filtered");
            const auto delivered=*runtimeResult_;const auto count=runtime_->records().size();selection(metric,std::nullopt);showRuntimeResult();send(request);check(take("DONE")==response && count==runtime_->records().size() && RuntimeService::sameCompletion(delivered,*runtimeResult_),"Duplicate delivery unchanged after visibility reset, no new execution");
            send({{"type","ACK"},{"requestId","hidden-scalar"}});take("ACKED");peer.abort();socket.stop();selection(metric,QSet<QString>{});
        }
        runtimeAction("stop");runtimeAction("run");runtimeCycle_=runtimeCoordinator_->begin(RuntimeTrigger::Manual);const auto failed=RuntimeService::execute(runtimeCycle_,catalog,runtimeCoordinator_->cancellation(),[](const ToolDefinition&)->std::unique_ptr<AcquisitionAdapter>{throw std::runtime_error("INT07_SOURCE_READ_FAILED");});completeRuntimeResult(failed,runtimeCoordinator_->cancellation());
        {QFile diagnostic(dir.filePath(typeKey(type)+"-error-results.txt"));check(diagnostic.open(QIODevice::WriteOnly),"Error result evidence");diagnostic.write(("Actual state: "+executionStateName(failed.state)+"\n"+runtimeSummary_->text()).toUtf8());}
        if(!runtimeResult_ || !runtimeSummary_->text().contains("SKIPPED") || !runtimeSummary_->text().contains("kullanılamıyor"))QTextStream(stderr)<<"Actual state "<<executionStateName(failed.state)<<"\n"<<runtimeSummary_->text()<<'\n';
        check(runtimeResult_ && failed.state==ExecutionState::Error && runtimeSummary_->text().contains("ERROR") && runtimeSummary_->text().contains("SKIPPED") && runtimeSummary_->text().contains("kullanılamıyor") && !runtimeSummary_->text().contains("Ölçüm:") && !runtimeViewer_->hasImage(),"Empty selection preserves real acquisition error, downstream skipped and invalid reasons");shot(typeKey(type)+"-error");runtimeAction("stop");
    }
    QTextStream(stdout)<<checks<<" D4 Runtime visibility checks passed; artifacts "<<dir.path()<<'\n';return true;
}
}
