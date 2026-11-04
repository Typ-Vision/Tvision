#include "app/MainWindow.hpp"
#include "app/PublishDataEditor.hpp"
#include "app/SetVariablesEditor.hpp"
#include "application/MachineSetup.hpp"
#include "application/PublicationPublisher.hpp"
#include "application/CommunicationWorkspace.hpp"
#include "persistence/StationIdentityStore.hpp"
#include "domain/PublicationTemplate.hpp"
#include "app/EndpointProfileEditor.hpp"
#include "app/DeliveryDiagnosticsWidget.hpp"
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
    check(!w.findChild<QComboBox*>("publishGroup") && !w.findChild<QPushButton*>("publishPreview") && !w.findChild<QCheckBox*>("toolEnabled") && !w.findChild<QPushButton*>("runTool")->isVisible(),"V4 properties contain no legacy editing/execution controls");
    check(w.findChild<QPushButton*>("publishApply")->text()=="Atamaları uygula","V4 apply label");
    // Historical data remains readable/executable through the existing backend.
    OutputGroup group;group.id=QUuid::createUuid().toString(QUuid::WithoutBraces);group.producerId=producer;group.producerRevision=1;group.profileId=profile.id;group.contractRevision=profile.contractRevision;group.required=true;
    for(int i=0;i<3;++i)group.fields.append({"field"+QString::number(i),variables[i].id});
    for(auto tool:w.service_.tools())if(tool.id==publisher){tool.enabled=true;tool.parameters=encodePublishConfig({{group.id}});w.service_.updatePublication(tool,{group});}
    w.service_.save();const auto saved=w.service_.activeRecipe()->definition;
    w.service_.openProject(w.service_.path());w.service_.selectRecipe(recipe);w.refreshWorkspace();w.selectTool(publisher);
    check(saved==w.service_.activeRecipe()->definition && w.commitToolProperties()==PropertyCommit::Unchanged,"Legacy UUID/parameters/groups open and save no-op");
    auto incompatible=[&](const QJsonObject& definition,const QString& reason){bool rejected=false;try{for(const auto& tool:editorTools(definition))if(tool.kind=="set_variables")SetVariablesEditor::validateConfiguration(tool,recipeVariables(definition),editorTools(definition));PublicationService::capture(definition,store);}catch(const std::exception& e){rejected=QString::fromUtf8(e.what()).contains(reason);}check(rejected,"Legacy incompatible type/unit/frame still rejected");};
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
        // Legacy diagnostics are an explicit acceptance fixture, not a page
        // owned by the normal Communication surface.
        check(!w.deliveryDiagnostics_,"normal Communication has no legacy diagnostics owner");
        w.deliveryDiagnostics_=new DeliveryDiagnosticsWidget([&w]{return w.service_.path()+".production";},&w);
        observer.close();profile=store.load()[profile.id];profile.role="server";++profile.revision;store.save({{profile.id,profile}});
        const bool multiple=QCoreApplication::arguments().contains("--multi-endpoint-smoke");
        if(multiple){
            QTcpServer portProbe;check(portProbe.listen(QHostAddress::LocalHost,0),"second simulator port");
            auto secondary=profile;secondary.id=QUuid::createUuid().toString(QUuid::WithoutBraces);secondary.port=portProbe.serverPort();secondary.revision=1;portProbe.close();
            store.save({{profile.id,profile},{secondary.id,secondary}});
            auto groups=recipeOutputGroups(w.service_.activeRecipe()->definition);auto second=groups.first();second.id=QUuid::createUuid().toString(QUuid::WithoutBraces);second.profileId=secondary.id;groups.append(second);
            w.service_.setOutputGroups(groups);for(auto tool:w.service_.tools())if(tool.id==publisher){tool.parameters=encodePublishConfig({{groups[0].id,groups[1].id}});w.service_.updateTool(tool);}
        }
        w.service_.enableMultiSource();w.service_.setRuntimeOptions("SoftwareTrigger",{});w.service_.save();w.ensureRuntime();
        w.runtimeCoordinator_->load(*w.service_.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);
        const auto loadedRecipe=w.runtimeCoordinator_->loaded()->id;
        w.service_.createRecipe("Selected draft only","",ApplicationType::Measurement);
        check(w.service_.activeRecipe()->id!=loadedRecipe,"selected recipe differs from loaded snapshot");
        check(!w.tcpEndpoint_,"Load creates no endpoint");w.runtimeAction("run");check(w.runtimeCoordinator_->state()==RuntimeState::Running && !w.tcpEndpoint_,"unchecked Run creates no endpoint");w.runtimeAction("stop");
        w.findChild<QCheckBox*>("runtimeTcpV3")->setChecked(true);w.findChild<QLineEdit*>("runtimeTcpPeer")->setText("fixture-generation-1");
        w.runtimeAction("run");check(w.tcpEndpoint_ && w.tcpEndpoint_->listening(),"explicit Run creates production v3 endpoint");
        QTcpSocket secondaryPeer;QByteArray secondaryBuffer;QString secondarySession;int secondaryResults=0;
        if(multiple){
            check(w.tcpPublicationTargets_.size()==1,"Run opens second loaded output target");
            QObject::connect(&secondaryPeer,&QTcpSocket::readyRead,[&]{
                secondaryBuffer+=secondaryPeer.readAll();for(;;){auto n=secondaryBuffer.indexOf('\n');if(n<0)break;const auto message=CommunicationWireCodec::decode(secondaryBuffer.left(n));secondaryBuffer.remove(0,n+1);
                    if(message["type"]=="HELLO")secondarySession=message["sessionId"].toString();
                    if(message["type"]=="RESULT"){++secondaryResults;secondaryPeer.write(CommunicationWireCodec::encode({{"v",3},{"type","ACK"},{"sessionId",secondarySession},{"requestId",message["requestId"]},{"resultId",message["resultId"]},{"payloadHash",message["payloadHash"]}}));}
                }
            });secondaryPeer.connectToHost(QHostAddress::LocalHost,w.tcpPublicationTargets_.front()->port());
            QElapsedTimer connected;connected.start();while(secondarySession.isEmpty() && connected.elapsed()<5000){QApplication::processEvents();QThread::msleep(1);}check(!secondarySession.isEmpty(),"second real simulator connected before trigger");
        }
        check(w.runtime_->active()->recipe().id==loadedRecipe,"v3 Run uses loaded recipe despite selected draft");
        w.deliveryDiagnostics_->refresh();check(w.findChild<QLabel*>("deliveryStatus")->text().contains(profile.id),"diagnostics bound to production endpoint");
        auto wait=[&](auto f){QElapsedTimer timer;timer.start();while(!f() && timer.elapsed()<15000){QApplication::processEvents();QThread::msleep(1);}check(f(),"production simulator timeout");};
        QFile input(dir.filePath("empty-input.json"));check(input.open(QIODevice::WriteOnly),"input file");input.write(QJsonDocument(QJsonObject{{"snapshotId","fixture"},{"revision","1"},{"fields",QJsonArray{}},{"sourceTime","fixture"},{"clockId","fixture"}}).toJson());input.close();
        QProcess simulator;simulator.start(QCoreApplication::applicationDirPath()+"/visionflow_plc_simulator.exe",{"--v3","--port",QString::number(w.tcpEndpoint_->port()),"--input",input.fileName(),"--expect-decision","NOT_EVALUATED","--expect-field","field0","--expect-value","false"});wait([&]{return simulator.state()==QProcess::NotRunning;});
        if(simulator.exitCode()!=0)QTextStream(stderr)<<simulator.readAllStandardOutput()<<simulator.readAllStandardError();check(simulator.exitCode()==0,"production endpoint actual simulator");
        if(multiple)wait([&]{return secondaryResults==1 && !w.runtimeCoordinator_->deliveryPending();});
        check(w.runtime_->records().size()==1 && !w.runtimeCoordinator_->deliveryPending(),"production terminal and ACK committed");
        if(multiple){const auto deliveries=w.tcpLedger_->recovery();check(deliveries.size()==2 && deliveries[0].status.state=="ACK_COMMITTED" && deliveries[1].status.state=="ACK_COMMITTED","UI uses one committed bundle and two independent ACK journals");}
        w.runtimeAction("stop");check(w.tcpEndpoint_->state()!="READY","Stop closes admission");
        w.selectPage("runtime");for(const auto size:{QSize(1400,900),QSize(850,600)}){w.resize(size);QApplication::processEvents();check(w.grab().save(QDir(artifacts).filePath(QString("runtime-v3-%1x%2.png").arg(size.width()).arg(size.height()))),"Runtime v3 visual");}
        w.findChild<QPushButton*>("runtime_tcp_stop")->click();check(!w.tcpEndpoint_,"drained endpoint safely released");
        w.deliveryDiagnostics_->refresh();check(w.findChild<QLabel*>("deliveryStatus")->text().contains("no bound production endpoint"),"diagnostics releases closed endpoint");
        MainWindow restarted(dir.filePath("panels.ini"));check(!restarted.tcpEndpoint_ && !restarted.findChild<QCheckBox*>("runtimeTcpV3")->isChecked(),"restart does not restore production network permission");
    }
    // TCP acceptance intentionally selected another draft and the Runtime page.
    // Restore the publisher's recipe/tool before exercising the slot editor.
    w.service_.selectRecipe(recipe);
    w.refreshWorkspace();
    w.selectPage("editor");
    w.selectTool(publisher);
    check(w.service_.activeRecipe()->id==recipe && w.selectedToolId_==publisher,
          "slot fixture restores publisher recipe and tool");
    // Explicitly retire the fixture's legacy execution before editing V4 bindings.
    for(auto tool:w.service_.tools())if(tool.id==publisher){tool.enabled=false;w.service_.updateTool(tool);}
    w.showToolProperties();
    // ---- v4 fixed-slot assignment (COMM4-17) ----
    // Machine template manifests are an explicit machine artifact; this fixture
    // writes one revision and only edits the recipe-level source binding.
    const auto templateDir=QDir(dir.filePath("v4-machine")).filePath("publication-templates-v4");
    check(QDir().mkpath(templateDir),"template fixture dir");
    PublicationPoseContract poseContract{QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid()};
    auto machineTemplate=PublicationTemplate::create(QUuid::createUuid(),QUuid::createUuid(),poseContract);
    machineTemplate.flags[0].name="AcceptedFlag";
    machineTemplate.flags[0].required=true;
    machineTemplate.flags[1].name="SecondFlag";
    {QFile file(QDir(templateDir).filePath("revision1.json"));check(file.open(QIODevice::WriteOnly),"template fixture write");file.write(QJsonDocument(machineTemplate.toJson()).toJson());}
    w.setCommunicationTemplateDirectory(templateDir);
    w.showToolProperties();
    auto* templateSelector=w.findChild<QComboBox*>("publishTemplateSelector");
    auto* slotTable=w.findChild<QTableWidget*>("publishSlots");
    auto* slotStatus=w.findChild<QLabel*>("publishSlotStatus");
    check(templateSelector && slotTable && slotStatus,"slot editor controls exist");
    check(templateSelector->count()==2,"one machine template revision offered");
    auto slotRow=[&](const QString& prefix){for(int r=0;r<slotTable->rowCount();++r)if(slotTable->item(r,0) && slotTable->item(r,0)->text().startsWith(prefix))return r;return -1;};
    auto pickSource=[&](int row,const QString& prefix){auto* picker=qobject_cast<QComboBox*>(slotTable->cellWidget(row,1));check(picker,"slot picker");for(int i=0;i<picker->count();++i)if(picker->itemText(i).startsWith(prefix)){picker->setCurrentIndex(i);return;}throw std::runtime_error("slot source option missing");};
    templateSelector->setCurrentIndex(1);
    check(slotTable->rowCount()==53,"fixed 16/32/4/1 slot capacity listed");
    ToolDefinition activeLegacy;for(const auto& t:w.service_.tools())if(t.id==publisher)activeLegacy=t;
    activeLegacy.enabled=true;auto* legacyGuard=dynamic_cast<PublishDataEditor*>(w.findChild<QWidget*>("publishDataEditor"));bool legacyRejected=false;
    try{legacyGuard->validateDraft(activeLegacy);}catch(const std::exception& e){legacyRejected=QString::fromUtf8(e.what()).contains("legacy");}
    check(legacyRejected,"Active legacy binding requires explicit transition without data deletion");
    check(slotRow("AcceptedFlag")>=0,"custom slot name from machine template shown");
    // Required slot missing blocks Apply with the field name; nothing is written.
    const auto beforeSlotDraft=w.service_.activeRecipe()->definition;
    check(!w.applyToolProperties() && beforeSlotDraft==w.service_.activeRecipe()->definition,"required missing blocks apply atomically");
    check(w.findChild<QLabel*>("publishSlotStatus")->text().contains("AcceptedFlag"),"required slot named in error");
    // Variable and direct source produce the same metadata preview path.
    pickSource(slotRow("AcceptedFlag"),"Accepted0 • Variable • Boolean");
    if(!w.applyToolProperties())QTextStream(stdout)<<"SLOT APPLY DEBUG: "<<w.findChild<QLabel*>("publishSlotStatus")->text()<<" | "<<w.findChild<QLabel*>("publishSlotStatus")->text()<<"\n";
    check(w.applyToolProperties(),"Boolean variable assigned to flag slot");
    // Apply rebuilds the properties panel; re-acquire the widgets.
    slotTable=w.findChild<QTableWidget*>("publishSlots");slotStatus=w.findChild<QLabel*>("publishSlotStatus");
    check(slotTable && slotStatus,"slot editor restored after apply");
    const auto savedV4=w.service_.activeRecipe()->definition;for(const auto& t:w.service_.tools())if(t.id==publisher)check(!t.enabled && t.parameters==activeLegacy.parameters,"V4 Apply preserves hidden enabled and parameters");
    const auto assignments=recipePublicationAssignments(savedV4);
    check(recipeOutputGroups(savedV4).size()==1 && recipeOutputGroups(savedV4).first().id==group.id,"Slot-only Apply preserves legacy groups");
    check(assignments && assignments->templateId==machineTemplate.id && assignments->templateRevision==machineTemplate.revision && assignments->rows.size()==1,"template revision + one assignment persisted");
    check(assignments->rows.first().kind==PublicationSlotKind::Flag && assignments->rows.first().index==0 && assignments->rows.first().source.kind==PublicationSourceKind::Variable,"variable slot source recorded");
    w.findChild<QPushButton*>("publishSlotPreview")->click();
    check(slotStatus->text().contains("SLOT ÖNİZLEMESİ") && (slotStatus->text().contains("Kontrat uyumlu") || slotStatus->text().contains("Geçerli")),"definition slot preview names the resolved slot");
    check(!slotStatus->text().contains("PASS"),"slot preview never reports a part PASS");
    // Incompatible port choice is a visible validation error, not a silent drop.
    pickSource(slotRow("Number0"),"Accepted0 • Variable • Boolean");
    check(w.findChild<QLabel*>("publishSlotStatus")->text().contains("sayısal") || (slotTable->cellWidget(slotRow("Number0"),1) && slotTable->cellWidget(slotRow("Number0"),1)->toolTip().contains("sayısal")),"incompatible Number assignment explained");
    check(!w.applyToolProperties() && savedV4==w.service_.activeRecipe()->definition,"incompatible slot assignment rejected atomically");
    // Cancel restores the last applied definition byte-for-byte.
    w.showToolProperties();slotTable=w.findChild<QTableWidget*>("publishSlots");pickSource(slotRow("SecondFlag"),"Accepted1 • Variable • Boolean");
    w.findChild<QPushButton*>("publishCancel")->click();QApplication::processEvents();
    check(savedV4==w.service_.activeRecipe()->definition && w.commitToolProperties()==PropertyCommit::Unchanged,"cancel leaves recipe bytes unchanged");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){
        w.resize(size);QApplication::processEvents();
        auto* scroll=w.findChild<QScrollArea*>("propertiesScroll");
        scroll->ensureWidgetVisible(w.findChild<QLabel*>("publishSlotStatus"));QApplication::processEvents();
        check(scroll->horizontalScrollBar()->maximum()==0,"slot panel fits without horizontal clipping");
        check(w.grab().save(QDir(artifacts).filePath(QString("publish-slots-%1.png").arg(size.width()))),"slot editor visual");
        w.findChild<QPushButton*>("publishSlotPreview")->click();
        QApplication::processEvents();
        auto* editorPanel=w.findChild<QWidget*>("publishDataEditor");
        check(editorPanel && editorPanel->grab().save(QDir(artifacts).filePath(QString("publish-slots-preview-%1.png").arg(size.width()))),"slot preview visual");
    }
    slotTable=w.findChild<QTableWidget*>("publishSlots");slotTable->setFocus();
    check(QApplication::focusWidget()==slotTable,"slot table accepts keyboard focus");
    check(group.producerId==producer && group.fields[0].fieldId==variables[0].id,"UUID binding preserved");QTextStream(stdout)<<"PASS Publish UI: three fields, mapping errors, UUID reopen, Editor network=0, Runtime terminal dispatch=1, machine template slot assignment/no-op\n";return true;
}
bool MainWindow::verifyVf03Publication(const QString& artifacts){
    auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    auto readBytes=[](const QString& path){QFile file(path);if(!file.open(QIODevice::ReadOnly))return QByteArray{};return file.readAll();};
    check(communicationWorkspace_!=nullptr,"Communication workspace bound by the normal launcher");
    // Same machine setup action the operator uses; repeated runs resume the
    // existing installation instead of replacing it.
    communicationWorkspace_->startSetup();
    const auto machine=communicationWorkspace_->machineDirectory();
    check(!machine.isEmpty(),"Machine directory available");
    JsonStationIdentityStore identities(QDir(machine).filePath("communication-identities-v4.json"));
    const auto catalogue=identities.load();
    const auto measurement=MachineSetup::defaults(catalogue.installation)[1];
    const auto stationsPath=QDir(machine).filePath("communication-stations-v4.json");
    const auto stationsBefore=readBytes(stationsPath);
    const auto recipesBefore=communicationWorkspace_->recipes().size();

    const QString root=QDir(artifacts).filePath("vf03-"+QUuid::createUuid().toString(QUuid::WithoutBraces));
    check(QDir().mkpath(root),"Isolated project directory");
    const QString imagePath=QDir(root).filePath("surface.png");
    QImage image(48,32,QImage::Format_Grayscale8);image.fill(210);check(image.save(imagePath),"Source raster");
    service_.createProject("VF03 kabul","",QDir(root).filePath("vf03.vfproject"),"file");
    const auto recipe=service_.createRecipe("Ölçüm A","",ApplicationType::Measurement,"file");
    service_.setImageFile(imagePath);
    VariableDefinition value;value.name="Value";value.type=VariableType::Int32;const auto valueId=service_.addVariable(value);
    VariableDefinition flag;flag.name="Flag";flag.type=VariableType::Boolean;const auto flagId=service_.addVariable(flag);
    ValueEnvelope number;number.type=ScalarType::Int32;number.validity=ValueValidity::Valid;number.value=qint32(42);
    ValueEnvelope boolean;boolean.type=ScalarType::Boolean;boolean.validity=ValueValidity::Valid;boolean.value=false;
    const auto producer=service_.addTool("set_variables");
    for(auto tool:service_.tools())if(tool.id==producer){tool.parameters=encodeAssignmentConfig({producer,1,{{valueId,ConstantAssignmentSource{number}},{flagId,ConstantAssignmentSource{boolean}}}});service_.updateTool(tool);}
    service_.setPublicationAssignments(PublicationAssignments{measurement.id,measurement.revision,
        {{PublicationSlotKind::Number,0,{PublicationSourceKind::Variable,valueId,{},0}},
         {PublicationSlotKind::Flag,0,{PublicationSourceKind::Variable,flagId,{},0}}}});
    service_.save();refreshWorkspace();selectPage("editor");
    auto* publish=findChild<QPushButton*>("publishRecipeV4");
    check(publish && publish->isVisible(),"Publish action visible in Recipe Editor");
    check(!service_.dirty(),"Recipe is saved before publishing");
    publish->click();
    check(workspaceError_==nullptr || workspaceError_->text().isEmpty() || !workspaceError_->isVisible(),"Publish succeeded without editor error");
    const auto recipesAfter=communicationWorkspace_->recipes().size();
    check(recipesAfter==recipesBefore+1,"Published revision added to station recipe list");
    const auto published=[&]{for(const auto& item:communicationWorkspace_->recipes())if(item.publicationOnly && item.selection.recipe.name=="Ölçüm A")return item;return StationSavedRecipe{};}();
    check(published.publicationOnly && published.selection.recipeKey>0 && !published.selection.revision.isNull(),"Exact published revision present");
    check(published.selection.cameras.isEmpty(),"Publishing does not invent a station camera binding");
    check(readBytes(stationsPath)==stationsBefore,"Publishing leaves station activation bytes unchanged");
    check(communicationWorkspace_->stations().isEmpty(),"Publishing does not load or activate a station");
    for(const auto size:{QSize(1400,900),QSize(850,600)}){
        resize(size);QApplication::processEvents();
        check(grab().save(QDir(artifacts).filePath(QString("vf03-editor-%1x%2.png").arg(size.width()).arg(size.height()))),"Editor visual");
        selectPage("communication");refreshCommunication();QApplication::processEvents();
        auto* picker=findChild<QComboBox*>("communicationSavedRecipe");
        check(picker && picker->count()>=1,"Communication recipe selector visible");
        bool listed=false;for(int i=0;i<picker->count();++i)listed|=picker->itemText(i).contains("Ölçüm A") && picker->itemText(i).contains("yayım");
        check(listed,"Published revision listed by the existing Communication selector");
        if(auto* scroll=findChild<QScrollArea*>("communicationScroll")){scroll->ensureWidgetVisible(picker);QApplication::processEvents();}
        check(grab().save(QDir(artifacts).filePath(QString("vf03-communication-%1x%2.png").arg(size.width()).arg(size.height()))),"Communication visual");
        if(picker){auto* panel=findChild<QWidget*>("communicationWorkspace");check(panel && panel->grab().save(QDir(artifacts).filePath(QString("vf03-recipe-list-%1x%2.png").arg(size.width()).arg(size.height()))),"Recipe list visual");}
        selectPage("editor");QApplication::processEvents();
    }
    // Unsaved editor changes are rejected explicitly and never publish.
    service_.editRecipeMetadata(recipe,"Ölçüm A","kaydedilmemiş taslak");
    publish=findChild<QPushButton*>("publishRecipeV4");
    publish->click();
    check(workspaceError_->isVisible() && workspaceError_->text().contains("Kaydedilmemiş"),"Unsaved draft rejected with explicit reason");
    check(communicationWorkspace_->recipes().size()==recipesAfter,"Rejected draft adds nothing");
    service_.discardChanges();refreshWorkspace();
    // Identical saved content stays the same immutable revision.
    publish=findChild<QPushButton*>("publishRecipeV4");publish->click();
    check(communicationWorkspace_->recipes().size()==recipesAfter,"Identical content is a no-op revision");
    check(statusBar()->currentMessage().contains("zaten") || statusBar()->currentMessage().contains("Yayım"),"Publish result reported");
    check(published.publishedAt.isValid(),"Published time reaches saved recipe DTO");
    const auto originalKey=published.selection.recipeKey;refreshDashboardStations();dashboardPublished_->setCurrentIndex(dashboardPublished_->findData(originalKey));
    for(int n=0;n<2;++n){service_.editRecipeMetadata(recipe,"Ölçüm A",QString("revision %1").arg(n));service_.save();publishEditingRecipe();}
    refreshDashboardStations();check(dashboardPublished_->currentData().toUInt()==originalKey,"Catalogue refresh preserves selected key");
    int sameRecipe=0,latest=0;for(int n=0;n<dashboardPublished_->count();++n){if(dashboardPublished_->itemText(n).startsWith("Ölçüm A ·")){++sameRecipe;latest+=dashboardPublished_->itemText(n).contains("Son yayımlanan");}check(dashboardPublished_->itemText(n).contains("#") && dashboardPublished_->itemData(n,Qt::ToolTipRole).toString().contains("UTC"),"Revision label includes key and full UTC tooltip");}
    check(sameRecipe==3 && latest==1,"Three immutable publications keep one latest label");
    const auto surface=MachineSetup::defaults(catalogue.installation)[0];
    service_.createProject("Duplicate name","",QDir(root).filePath("duplicate.vfproject"),"file");service_.createRecipe("Ölçüm A","",ApplicationType::DefectInspection,"file");service_.setImageFile(imagePath);service_.setPublicationAssignments(PublicationAssignments{surface.id,surface.revision,{}});service_.save();refreshWorkspace();publishEditingRecipe();refreshDashboardStations();
    latest=0;for(int n=0;n<dashboardPublished_->count();++n)latest+=dashboardPublished_->itemText(n).contains("Son yayımlanan");
    check(latest==2 && dashboardPublished_->count()==4 && dashboardPublished_->currentData().toUInt()==originalKey,"Same name with different recipeId keeps separate latest labels and selection");
    auto displayFixture=communicationWorkspace_->recipes();const auto sameTime=QDateTime::fromString("2026-11-04T12:00:00Z",Qt::ISODate);
    quint32 highest=0;for(auto& entry:displayFixture)if(entry.selection.recipe.id==published.selection.recipe.id){entry.publishedAt=entry.selection.recipeKey==originalKey?QDateTime{}:sameTime;highest=qMax(highest,entry.selection.recipeKey);}
    communicationWorkspace_->setRecipeControl([displayFixture]{return displayFixture;},{});refreshDashboardStations();
    const int unknown=dashboardPublished_->findData(originalKey);check(dashboardPublished_->itemText(unknown).contains("Tarih bilinmiyor") && !dashboardPublished_->itemText(unknown).contains("Son yayımlanan"),"Unknown legacy date never claims latest");
    check(dashboardPublished_->itemText(dashboardPublished_->findData(highest)).contains("Son yayımlanan"),"Equal date order is deterministic by key");
    const auto status=communicationWorkspace_->publicationStatus();
    check(status.contains("Yayımlanmış reçete"),"Machine catalogue status visible");
    QTextStream(stdout)<<"PASS VF-03 normal UI: editor publish + Communication recipe list + unsaved rejection\n";
    return true;
}
}
