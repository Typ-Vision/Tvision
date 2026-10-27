#include "app/MainWindow.hpp"
#include "app/PublishDataEditor.hpp"
#include "app/EndpointProfileEditor.hpp"
#include "application/PublicationService.hpp"
#include "application/DeliveryStore.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QTextStream>
#include <QKeyEvent>
#include <QStatusBar>
#include <QLineEdit>
#include <QProcess>
#include <QElapsedTimer>
#include <QThread>
#include <stdexcept>
namespace visionflow {
bool MainWindow::verifyPublicationUi(const QString& artifacts){
    auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    QTemporaryDir dir(QDir(artifacts).filePath("publication-ui-XXXXXX"));check(dir.isValid(),"fixture");
    MainWindow w(dir.filePath("panels.ini"));w.smokeMode_=true;w.resize(1400,900);w.show();
    w.service_.createProject("Publish","",dir.filePath("part.vfproject"),"");const auto recipe=w.service_.createRecipe("Part","",ApplicationType::DefectInspection);
    QImage image(24,24,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("part.png")),"image");w.service_.setImageFile(dir.filePath("part.png"));
    QVector<VariableDefinition> variables;QVector<AssignmentRow> rows;
    for(int i=0;i<3;++i){VariableDefinition v;v.name="Accepted"+QString::number(i);v.id=w.service_.addVariable(v);variables.append(v);ValueEnvelope value;value.type=ScalarType::Boolean;value.validity=ValueValidity::Valid;value.value=bool(i%2);rows.append({v.id,ConstantAssignmentSource{value}});}
    const auto producer=w.service_.addTool("set_variables");for(auto t:w.service_.tools())if(t.id==producer){t.parameters=encodeAssignmentConfig({producer,1,rows});w.service_.updateTool(t);}
    QTcpServer observer;check(observer.listen(QHostAddress::LocalHost,0),"observer");EndpointProfile profile;profile.id=QUuid::createUuid().toString(QUuid::WithoutBraces);profile.enabled=true;profile.port=observer.serverPort();
    CommunicationProfiles store(dir.filePath("communication-profiles.json"));store.save({{profile.id,profile}});
    // Configure physical mapping through the actual profile panel; duplicate names must not collapse in QMap.
    EndpointProfileEditor panel(dir.filePath("communication-profiles.json"),[]{return false;});
    auto* physical=panel.findChild<QTableWidget*>("endpointMapping");
    for(int i=0;i<3;++i){panel.findChild<QPushButton*>("endpointAddMapping")->click();physical->item(i,0)->setText("field"+QString::number(i));physical->item(i,1)->setText("json"+QString::number(i));}
    panel.findChild<QPushButton*>("endpoint_save")->click();check(store.load()[profile.id].fieldMapping.size()==3,"physical mapping saved");
    physical->item(1,0)->setText("field0");panel.findChild<QPushButton*>("endpoint_save")->click();check(panel.findChild<QLabel*>("endpointStatus")->text().contains("Duplicate") && store.load()[profile.id].fieldMapping.size()==3,"duplicate mapping atomic rejection");panel.findChild<QPushButton*>("endpoint_cancel")->click();
    const auto publisher=w.service_.addTool("publish_data");w.service_.save();w.refreshWorkspace();w.selectPage("editor");w.selectTool(publisher);
    const auto untouched=w.service_.activeRecipe()->definition;check(w.commitToolProperties()==PropertyCommit::Unchanged && w.service_.activeRecipe()->definition==untouched,"new disabled draft no-op");
    w.findChild<QPushButton*>("publishAddGroup")->click();w.findChild<QComboBox*>("publishProducer")->setCurrentIndex(1);w.findChild<QComboBox*>("publishEndpoint")->setCurrentIndex(1);
    auto* fields=w.findChild<QTableWidget*>("publishFields");for(int i=0;i<3;++i){w.findChild<QPushButton*>("publishAddField")->click();fields->item(i,0)->setText("field"+QString::number(i));static_cast<QComboBox*>(fields->cellWidget(i,1))->setCurrentIndex(i+1);}
    w.findChild<QPushButton*>("publishPreview")->click();check(w.findChild<QLabel*>("publishStatus")->text().startsWith("RESOLVED"),"three field resolved preview");
    check(w.applyToolProperties(),"group and publisher atomic apply");w.showToolProperties();w.findChild<QCheckBox*>("toolEnabled")->setChecked(true);check(w.applyToolProperties(),"enable publisher");w.showToolProperties();w.findChild<QCheckBox*>("publishRequired")->setChecked(false);check(w.applyToolProperties() && !w.service_.outputGroups().first().required,"optional binding UI");w.showToolProperties();w.findChild<QCheckBox*>("publishRequired")->setChecked(true);check(w.applyToolProperties() && w.service_.outputGroups().first().required,"required binding UI");w.service_.save();const auto saved=w.service_.activeRecipe()->definition;const auto group=w.service_.outputGroups().first();
    w.service_.openProject(w.service_.path());w.service_.selectRecipe(recipe);w.refreshWorkspace();w.selectTool(publisher);check(saved==w.service_.activeRecipe()->definition && w.commitToolProperties()==PropertyCommit::Unchanged,"UUID save reopen no-op");
    fields=w.findChild<QTableWidget*>("publishFields");fields->item(1,0)->setText("field0");check(w.commitToolProperties()==PropertyCommit::Failed && saved==w.service_.activeRecipe()->definition,"duplicate logical field rejected atomically");w.showToolProperties();
    fields=w.findChild<QTableWidget*>("publishFields");fields->item(1,0)->setText("missingMapping");w.findChild<QPushButton*>("publishPreview")->click();check(w.findChild<QLabel*>("publishStatus")->text().contains("unresolved"),"missing physical mapping visible");
    for(auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();auto* scroll=w.findChild<QScrollArea*>("propertiesScroll");scroll->ensureWidgetVisible(w.findChild<QLabel*>("publishStatus"));QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("publish-error-%1.png").arg(size.width()))),"error visual");}
    w.showToolProperties();w.workspaceError_->clear();w.workspaceError_->hide();w.statusBar()->clearMessage();
    auto* sourcePicker=w.findChild<QComboBox*>("publishProducer");sourcePicker->setFocus();QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(sourcePicker,&tab);check(QApplication::focusWidget()!=sourcePicker,"Publish keyboard traversal");
    for(auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();w.findChild<QPushButton*>("publishPreview")->click();auto* scroll=w.findChild<QScrollArea*>("propertiesScroll");scroll->verticalScrollBar()->setValue(0);QApplication::processEvents();check(scroll->horizontalScrollBar()->maximum()==0 && sourcePicker->height()>=27,"Publish panel fits without horizontal clipping");check(w.grab().save(QDir(artifacts).filePath(QString("publish-editor-%1.png").arg(size.width()))),"Editor visual");scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("publish-controls-%1.png").arg(size.width()))),"controls visual");}
    auto incompatible=[&](const QJsonObject& definition,const QString& reason){
        ToolDefinition tool;for(const auto& t:editorTools(definition))if(t.id==publisher)tool=t;
        PublishDataEditor editor(tool,definition,dir.filePath("communication-profiles.json"));bool rejected=false;
        try{editor.validateDraft(tool);}catch(const std::exception& e){rejected=QString::fromUtf8(e.what()).contains(reason);}
        check(rejected,"incompatible type/unit/frame preview rejected");
        editor.findChild<QPushButton*>("publishPreview")->click();check(!editor.findChild<QLabel*>("publishStatus")->text().isEmpty(),"incompatible preview error visible");
    };
    auto changeSource=[&](QJsonObject definition,const VariableDefinition& variable,const AssignmentValue& value){
        auto vars=recipeVariables(definition);vars[0]=variable;definition=withRecipeVariables(definition,vars);auto tools=editorTools(definition);
        for(auto& t:tools)if(t.id==producer){auto config=decodeAssignmentConfig(t.parameters,t.id);config.rows[0].source=ConstantAssignmentSource{value};t.parameters=encodeAssignmentConfig(config);}
        return withEditorTools(definition,tools);
    };
    auto v=variables[0];ValueEnvelope scalar;scalar.type=ScalarType::Float64;scalar.validity=ValueValidity::Valid;scalar.value=1.0;
    incompatible(changeSource(saved,v,scalar),"type mismatch");v.type=VariableType::Float64;v.unit=ScalarUnit::Millimetre;scalar.unit=ScalarUnit::Metre;incompatible(changeSource(saved,v,scalar),"unit mismatch");
    FrameRef source{"","robot","cell",QUuid::createUuid().toString(QUuid::WithoutBraces),1};auto target=source;target.frameUuid=QUuid::createUuid().toString(QUuid::WithoutBraces);v.type=VariableType::SpatialTransform;v.frameConstraint=VariableFrameConstraint{source,target};SpatialAssignmentValue pose;pose.validity=ValueValidity::Valid;auto wrong=source;wrong.frameUuid=QUuid::createUuid().toString(QUuid::WithoutBraces);pose.value=SpatialTransform{wrong,target,{1,2,3},{}};incompatible(changeSource(saved,v,pose),"frame mismatch");
    auto snapshot=PublicationService::capture(saved,store);const auto signature=w.flowSignature();auto execution=ExecutionService{}.withPublicationSnapshot(snapshot);auto c=execution.run(saved,dir.path(),recipe,signature,{});check(c.state==ExecutionState::Done && c.publicationIntents.size()==1 && c.publicationIntents.first().fields.size()==3,"three fields one intent");check(w.editorSession_.acceptCycle(c,signature,w.editorContext()),"Editor accepts cycle");auto partial=execution.runTool(saved,dir.path(),recipe,signature,{},publisher,w.editorSession_.evaluation(w.editorContext(),publisher));check(partial.state==ExecutionState::Done && partial.publicationIntents.size()==1,"Run Tool intent");QApplication::processEvents();check(!observer.hasPendingConnections(),"preview and Run Tool network zero");
    ProductionRecipeService production([](const QString&,ProductionAction){return true;},[&](const Recipe& r){ProductionDependencies deps;deps.publication=PublicationService::capture(r.definition,store);return deps;});const auto frozen=*w.service_.activeRecipe();auto validation=production.validate("engineer",frozen,dir.path());production.activate("engineer",validation.id,frozen,dir.path());auto lease=production.beginCycle("engineer");auto result=RuntimeService::execute(lease,dir.filePath("catalog.json"),{});production.finishCycle(lease.ticket,result);const auto terminal=production.cycleRecords().last();ProductionResultCodec::validate(terminal);
    DeliveryStore delivery(dir.filePath("delivery"));DeliveryRequest request{"cell",w.service_.project().id,profile.id,"peer","request","fingerprint"};delivery.request(request);delivery.begin(request,terminal["cycleId"].toString());const auto payload=QJsonDocument(terminal["resultBundle"].toObject()).toJson(QJsonDocument::Compact);delivery.commit(request,terminal,payload);int writes=0;check(delivery.dispatch(request,[&](const QByteArray& bytes){check(bytes==payload,"persisted exact bytes");++writes;}),"terminal dispatch");check(writes==1 && terminal["resultBundle"].toObject()["groups"].toArray().size()==1,"one terminal publish");
    if(QCoreApplication::arguments().contains("--tcp-acceptance-smoke")){
        observer.close();profile=store.load()[profile.id];profile.role="server";++profile.revision;store.save({{profile.id,profile}});
        w.service_.enableMultiSource();w.service_.setRuntimeOptions("SoftwareTrigger",{});w.service_.save();w.ensureRuntime();
        w.runtimeCoordinator_->load(*w.service_.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);
        check(!w.tcpEndpoint_,"Load creates no endpoint");w.runtimeAction("run");check(w.runtimeCoordinator_->state()==RuntimeState::Running && !w.tcpEndpoint_,"unchecked Run creates no endpoint");w.runtimeAction("stop");
        w.findChild<QCheckBox*>("runtimeTcpV3")->setChecked(true);w.findChild<QLineEdit*>("runtimeTcpPeer")->setText("fixture-generation-1");
        w.runtimeAction("run");check(w.tcpEndpoint_ && w.tcpEndpoint_->listening(),"explicit Run creates production v3 endpoint");
        auto wait=[&](auto f){QElapsedTimer timer;timer.start();while(!f() && timer.elapsed()<15000){QApplication::processEvents();QThread::msleep(1);}check(f(),"production simulator timeout");};
        QFile input(dir.filePath("empty-input.json"));check(input.open(QIODevice::WriteOnly),"input file");input.write(QJsonDocument(QJsonObject{{"snapshotId","fixture"},{"revision","1"},{"fields",QJsonArray{}},{"sourceTime","fixture"},{"clockId","fixture"}}).toJson());input.close();
        QProcess simulator;simulator.start(QCoreApplication::applicationDirPath()+"/visionflow_plc_simulator.exe",{"--v3","--port",QString::number(w.tcpEndpoint_->port()),"--input",input.fileName(),"--expect-decision","NOT_EVALUATED","--expect-field","field0","--expect-value","false"});wait([&]{return simulator.state()==QProcess::NotRunning;});
        if(simulator.exitCode()!=0)QTextStream(stderr)<<simulator.readAllStandardOutput()<<simulator.readAllStandardError();check(simulator.exitCode()==0,"production endpoint actual simulator");
        check(w.runtime_->records().size()==1 && !w.runtimeCoordinator_->deliveryPending(),"production terminal and ACK committed");
        w.runtimeAction("stop");check(w.tcpEndpoint_->state()!="READY","Stop closes admission");
        w.selectPage("runtime");for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("runtime-v3-%1x%2.png").arg(size.width()).arg(size.height()))),"Runtime v3 visual");}
        w.findChild<QPushButton*>("runtime_tcp_stop")->click();check(!w.tcpEndpoint_,"drained endpoint safely released");
        MainWindow restarted(dir.filePath("panels.ini"));check(!restarted.tcpEndpoint_ && !restarted.findChild<QCheckBox*>("runtimeTcpV3")->isChecked(),"restart does not restore production network permission");
    }
    check(group.producerId==producer && group.fields[0].fieldId==variables[0].id,"UUID binding preserved");QTextStream(stdout)<<"PASS Publish UI: three fields, mapping errors, UUID reopen, Editor network=0, Runtime terminal dispatch=1\n";return true;
}
}
