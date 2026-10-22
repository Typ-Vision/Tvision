#include "OecsExperimentalFixture.hpp"
#include "application/IndependentCalibrationCapture.hpp"
#include "application/MeasurementEnvironment.hpp"
#include "application/SystemCalibration.hpp"
#include "chessboard_fixture.hpp"
#include <cmath>
using namespace oecs_experimental_fixture;

namespace {
void calibrationCaptureChecks(){
    const auto r=request();auto deviceState=state();
    MeasurementEnvironment environment;environment.projectId="fixture";environment.current={{r.camera},{r.setup},{r.profile}};
    environment.history={r.setup};environment.registrations[r.camera.id.value]={"CAM_01",r.setup.id,r.profile.id};
    auto capture=IndependentCaptureRequest::fromDefaults(environment,r.camera.id,CalibrationCapturePolicy::ExperimentalOecs);capture.timeoutMs=3000;
    auto frozen=capture;environment.current.profiles[0].requested.exposureTimeUs=6000;
    check(frozen.profile->requested.exposureTimeUs==5000,"capture freezes CCUX defaults by value");
    auto ambiguous=environment;ambiguous.registrations.clear();auto second=r.profile;second.id={"second-profile"};ambiguous.current.profiles.append(second);
    rejects([&]{IndependentCaptureRequest::fromDefaults(ambiguous,r.camera.id,CalibrationCapturePolicy::ExperimentalOecs);});
    IndependentCaptureBackend backend;backend.industrialDiscover=[r]{return IndustrialDiscoverySnapshot{{r.device},{},false};};
    backend.experimentalApi=[deviceState]{return std::make_unique<Api>(deviceState);};
    auto stripe=[&]{deviceState->pixels.fill(char(20));for(int y=0;y<2048;++y)for(int x=120;x<520;++x)deviceState->pixels[y*2448+x]=char(220);};stripe();
    IndependentCalibrationSession session;
    auto acquire=[&]{return IndependentCalibrationSession::run(session.begin(capture),backend);};
    {auto held=ExperimentalOecsAcquisition::open(r,QDir::temp().absoluteFilePath("experimental.dll"),backend.experimentalApi);auto denied=acquire();check(!session.accept(denied),"calibration respects existing Editor lease");}
    auto live=acquire();check(live.error().isEmpty() && session.accept(live),"experimental recipe-free live calibration fixture");
    const auto liveProof=*session.evidence();const auto liveImage=*session.math().frame();
    check(liveProof.experimental && liveProof.origin==SourceOrigin::CameraAcquired && liveProof.toJson()["schemaVersion"]==2 &&
        MeasurementSourceEvidence::fromJson(liveProof.toJson())==liveProof,"typed experimental source roundtrip");
    auto& math=session.math();
    auto pair=CalibrationWizardSession::run(math.scanPair("experimental-pair",{{"image"},ScanSegment{{20,100},{620,100},4}}));
    check(pair.error().isEmpty() && math.accept(pair) && std::abs(*math.distancePixels()-400)<.01,"existing caliper on experimental real fixture raster");
    math.setKnownMillimetres(10);const auto candidate=math.calculate();
    check(candidate.image().origin=="experimental_acquired" && !candidate.activatable() && candidate.encode()["schemaVersion"]==6 &&
        !candidate.controlAccepted() && std::abs(candidate.model().millimetresPerPixel()-.025)<1e-9,"experimental candidate has math without physical/production acceptance");
    const auto encoded=candidate.named("Experimental fixture").encode();check(CalibrationModelRecord::decode(encoded).encode()==encoded,"schema6 candidate immutable roundtrip");
    auto stripped=encoded;stripped.remove("experimental");rejects([&]{CalibrationModelRecord::decode(stripped);});
    stripped=encoded;stripped["schemaVersion"]=2;stripped["policy"]="model-ready/1";stripped.remove("experimental");rejects([&]{CalibrationModelRecord::decode(stripped);});
    rejects([&]{math.useFrame(liveImage,candidate.setup(),"acquired");});
    rejects([&]{CalibrationModelRecord::mapLegacy(candidate,candidate.setup(),"Unsafe migration",QDateTime::currentDateTimeUtc());});
    auto clearEvidence=candidate.image();clearEvidence.experimental.reset();rejects([&]{CalibrationModelRecord::create(candidate.model(),candidate.setup(),clearEvidence);});
    QTemporaryDir directory(QDir::currentPath()+"/ccux07-calibration-XXXXXX");check(directory.isValid(),"build-only experimental calibration fixtures");
    ActiveCalibrationService catalog(directory.filePath("catalog.json"),"fixture",[]{return true;});catalog.save(candidate);
    check(catalog.records().front().encode()==candidate.encode(),"catalog candidate persistence");
    rejects([&]{catalog.activate(candidate.model().dependency().calibrationId,1,candidate.setup());});
    ActiveCalibrationSnapshot unsafe{candidate,{candidate.model().dependency().calibrationId,candidate.contentId(),1,1}};
    rejects([&]{SystemCalibration::evidence(unsafe,"source","image");});
    const auto path=directory.filePath("capture.pgm");const auto meta=liveImage.metadata();
    ExperimentalOecsAcquisition::save({liveImage,meta.sourceId,meta.cycleId,meta.frameId,meta.completedAtUtc},path);
    auto replay=capture;replay.origin=SourceOrigin::SetupFile;replay.filePath=path;
    auto replayResult=[&]{return IndependentCalibrationSession::run(session.begin(replay),backend);};
    check(session.accept(replayResult()) && session.evidence()->origin==SourceOrigin::SetupFile &&
        session.math().frame()->contentId()==liveImage.contentId() && session.evidence()->experimental->origin==SourceOrigin::SetupFile,"replay retains file identity and exact raster");
    pair=CalibrationWizardSession::run(math.scanPair("replay-pair",{{"image"},ScanSegment{{20,100},{620,100},4}}));check(math.accept(pair),"replay caliper");math.setKnownMillimetres(10);
    check(math.calculate().image().origin=="experimental_setup_file" && !math.calculate().activatable(),"replay math remains experimental");
    auto wrong=replay;wrong.policy=CalibrationCapturePolicy::Trusted;check(!session.accept(IndependentCalibrationSession::run(session.begin(wrong),backend)),"opt-in off rejects marked raster");
    wrong=capture;wrong.policy=CalibrationCapturePolicy::Trusted;check(!session.accept(IndependentCalibrationSession::run(session.begin(wrong),backend)),"production factory remains unavailable");
    wrong=replay;wrong.camera->physical->serial="OTHER";check(!session.accept(IndependentCalibrationSession::run(session.begin(wrong),backend)),"foreign serial replay rejected");
    wrong=replay;wrong.setup->geometry.reverse=GeometryMirror{true,false};check(!session.accept(IndependentCalibrationSession::run(session.begin(wrong),backend)),"wrong geometry replay rejected");
    wrong=replay;wrong.profile->revision=2;check(!session.accept(IndependentCalibrationSession::run(session.begin(wrong),backend)),"wrong profile replay rejected");
    QFile sidecar(path+".json");check(sidecar.open(QIODevice::ReadOnly),"read replay proof");const auto bytes=sidecar.readAll();sidecar.close();
    const auto original=QJsonDocument::fromJson(bytes).object();
    for(const auto& field:QStringList{"serial","contentId","readback","profileSnapshot","productionAdmission","triggerAssociationVerified"}){
        auto bad=original;bad[field]=true;check(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"mutate sidecar");sidecar.write(QJsonDocument(bad).toJson());sidecar.close();
        check(!session.accept(replayResult()) && !session.math().frame() && !session.evidence(),"changed sidecar cannot retain cached draft");
    }
    check(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"invalid sidecar");sidecar.write("{broken");sidecar.close();check(!session.accept(replayResult()),"broken JSON rejected");
    check(sidecar.remove(),"remove sidecar");check(!session.accept(replayResult()),"missing JSON rejected");
    check(sidecar.open(QIODevice::WriteOnly),"restore sidecar");sidecar.write(bytes);sidecar.close();
    auto job=session.begin(replay);auto done=IndependentCalibrationSession::run(job,backend);session.cancel();check(!session.accept(done),"Stop suppresses experimental callback");
    job=session.begin(replay);done=IndependentCalibrationSession::run(job,backend);auto newer=session.begin(replay);check(!session.accept(done) && session.accept(IndependentCalibrationSession::run(newer,backend)),"new generation suppresses old experimental callback");
    stripe();job=session.begin(capture);deviceState->waitHook=[&]{job.cancel();};auto stopped=IndependentCalibrationSession::run(job,backend);check(!session.accept(stopped),"cancel during experimental SDK wait suppresses owned raster");deviceState->waitHook={};
    QImage plate(2448,2048,QImage::Format_Grayscale8);plate.fill(255);const auto board=chessboardFixture();for(int y=0;y<board.height();++y)std::copy_n(board.constScanLine(y),board.width(),plate.scanLine(y));
    deviceState->pixels=QByteArray(reinterpret_cast<const char*>(plate.constBits()),plate.sizeInBytes());
    check(session.accept(acquire()),"experimental chessboard fixture acquired");
    const auto observation=CalibrationWizardSession::run(math.scanPlate({"grid",9,6,10}));check(observation.error().isEmpty() && math.accept(observation),"existing chessboard detector on experimental raster");math.confirmPlate();
    const auto mapping=math.calculatePlate();check(mapping.model().mapping().has_value() && mapping.image().experimental && !mapping.activatable(),"experimental plate candidate preserves evidence");
    check(CalibrationModelRecord::decode(mapping.encode()).encode()==mapping.encode(),"experimental plate record roundtrip");
    auto normalImage=candidate.image();normalImage.origin="acquired";normalImage.experimental.reset();const auto old=CalibrationModelRecord::create(candidate.model(),candidate.setup(),normalImage).encode();
    check(old["schemaVersion"]==1 && !old.contains("experimental") && CalibrationModelRecord::decode(old).encode()==old,"old schema1 no-op codec/hash unchanged");
    session.cancel();
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    calibrationCaptureChecks();
    check(makeOecsCApi()->softwareTriggerCorrelation()==OecsTriggerCorrelation::Unavailable,"native trusted capability stays unavailable");
    CameraProviderFactory native;check(!native.capabilities(request().device).softwareTrigger && !native.capabilities(request().device).preview,"production factory unchanged");
    {auto r=request();r.profile.requested.trigger.selector="FrameStart";rejects([&]{ExperimentalOecsAcquisition::open(r,"C:/fake.dll");});}
    {auto s=state();CameraPreviewSession preview;
        preview.start([s]{return open(s);},true,3000);
        until([&]{preview.poll();return preview.stats().completed>=2 || preview.state()==PreviewState::Error;});
        auto frame=preview.takeLatest();
        check(frame && frame->kind==PreviewAcquisitionKind::SequentialSingleFrame,"OECS preview remains sequential");
        const auto evidence=frame->frame.image.metadata().experimentalAcquisition;
        check(evidence["productionAdmission"]==false && evidence["triggerAssociationVerified"]==false &&
              evidence["profileSnapshot"].toObject()==request().profile.toJson(),"preview preserves explicit FrameBurstStart and experimental provenance");
        preview.stop();until([&]{preview.poll();return preview.drained();});
        check(!preview.takeLatest() && preview.state()==PreviewState::Idle && !s->running && !s->held,"OECS preview Stop drains resources without late raster");
    }
    {auto s=state();auto r=request();r.profile.requested.exposureAuto=AutoMode::Continuous;rejects([&]{ExperimentalOecsAcquisition::open(r,"C:/fake.dll",[s]{return std::make_unique<Api>(s);});});}
    {auto s=state();auto worker=open(s);rejects([&]{open(s);});start(*worker);check(worker->lifecycle().state()==CameraState::Streaming,"experimental configured");
        command(*worker,CameraOperation::Grab,CameraState::Streaming);auto frame=worker->take();check(frame.has_value(),"owned experimental frame");
        const auto evidence=frame->image.metadata().experimentalAcquisition;
        check(evidence["triggerAssociationVerified"]==false && evidence["productionAdmission"]==false && !evidence.contains("arm"),"old/fake proof never upgraded");
        IndustrialExecutionEvidence completion;completion.request=request();completion.acquired=frame->image.metadata();
        rejects([&]{industrialCompletionEvidence(completion);});
        check(frame->image.pixelText(0,0).contains("17"),"owned pixels survive buffer release");
        check(s->calls==QStringList{"start","clear","command","wait","release","clear","stop"},"Clear while running then Stop");
        QTemporaryDir directory(QDir::currentPath()+"/experimental-save-XXXXXX");check(directory.isValid(),"evidence temp directory");const auto path=directory.filePath("frame.pgm");
        close(*worker);ExperimentalOecsAcquisition::save(*frame,path);
        rejects([&]{ImageFileService{}.load(path);}); // No silent replay as an ordinary source.
        QFile file(path+".json");check(file.open(QIODevice::ReadOnly),"sidecar exists");auto saved=QJsonDocument::fromJson(file.readAll()).object();
        check(saved["contentId"]==QString::fromLatin1(frame->image.contentId().toHex()) && saved["triggerAssociationVerified"]==false,"saved hash and unverified provenance");
        file.close();
        const auto replay=ImageFileService{}.load(path,true);
        check(replay.contentId()==frame->image.contentId() && replay.metadata().experimentalAcquisition==saved,"explicit replay preserves exact owned raster and evidence");
        int replayIndex=0;
        for(const auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}){
            ProjectService project;project.createProject("Experimental","",directory.filePath(QString("replay-%1.vfproject").arg(replayIndex++)),"file");project.createRecipe("Replay","",type,"file");
            const auto source=project.setImageFile(path);project.enableMultiSource();const auto blob=project.addTool("blob");project.setConnections({{source,"image",blob,"image"}});
            project.save();const auto recipe=project.activeRecipe()->id;const auto version=project.activeRecipe()->recipeVersion;const auto definition=project.activeRecipe()->definition;
            project.openProject(project.path());project.selectRecipe(recipe);project.save();check(project.activeRecipe()->recipeVersion==version && project.activeRecipe()->definition==definition,"experimental replay no-op reopen");
            {
                const auto archive=project.path()+".production";RuntimeService runtime(archive,{},RuntimeIdentity::localEngineer());runtime.setExperimental(true);RuntimeCoordinator coordinator(runtime);
                rejects([&]{coordinator.load(*project.activeRecipe(),directory.path(),RuntimeTrigger::SoftwareTrigger);});
                coordinator.load(*project.activeRecipe(),directory.path());check(!runtime.active() && coordinator.state()==RuntimeState::LoadedStopped,"experimental Load grants no admission");coordinator.start();
                check(runtime.active()->validation().scope=="experimental_runtime/1","separate experimental admission");
                rejects([&]{runtime.validate(*project.activeRecipe(),directory.path(),replay);});rejects([&]{runtime.activate(*project.activeRecipe(),directory.path());});rejects([&]{runtime.rollback("foreign-production-history");});
                {PlcRuntimeSocket server(runtime,archive,{});PlcSocketOptions options;options.port=0;server.listen(options);QTcpSocket peer;peer.connectToHost(QHostAddress::LocalHost,server.port());QByteArray input;until([&]{QCoreApplication::processEvents();input+=peer.readAll();return input.contains('\n');},"experimental socket HELLO timeout");auto hello=QJsonDocument::fromJson(input.left(input.indexOf('\n'))).object();check(hello["ready"]==false,"experimental socket never READY");input.clear();peer.write(QJsonDocument(QJsonObject{{"v",2},{"sessionId",server.sessionId()},{"type","TRIGGER"},{"requestId","experimental-block"},{"recipeId",recipe},{"configId",QString::fromLatin1(runtime.active()->validation().contentId)}}).toJson(QJsonDocument::Compact)+'\n');until([&]{QCoreApplication::processEvents();input+=peer.readAll();return input.contains('\n');},"experimental socket rejection timeout");check(input.contains("EXPERIMENTAL_OUTPUT_BLOCKED"),input.constData());check(!server.working() && !server.blocking() && !runtime.busy() && !input.contains("\"type\":\"DONE\""),"PLC trigger cannot acquire/publish experimental scalar");server.stop();}
                rejects([&]{runtime.begin({{"plc",true}});});const auto lease=coordinator.begin(RuntimeTrigger::Manual);
                auto result=RuntimeService::execute(lease,{},coordinator.cancellation());check(result.state==ExecutionState::Done && result.industrialEvidence.isEmpty(),"experimental Runtime actual replay");
                rejects([&]{coordinator.begin(RuntimeTrigger::Manual);});check(coordinator.state()==RuntimeState::Running,"busy rejection preserves Running");rejects([&]{runtime.setExperimental(false);});
                auto changed=result;changed.frames[0].experimentalAcquisition["productionAdmission"]=true;rejects([&]{runtime.finish(lease.ticket,changed);});
                auto editorResult=ExecutionService{}.withExperimentalEditorReplay().run(definition,directory.path(),recipe,lease.snapshot->validation().contentId,{});rejects([&]{runtime.finish(lease.ticket,editorResult);});
                QDir().mkpath(archive);QLockFile lock(archive+"/archive.lock");check(lock.tryLock(0),"fixture archive lock");rejects([&]{coordinator.finish(lease.ticket,result);});check(coordinator.recordingPending() && runtime.busy(),"experimental persistence failure blocks admission");lock.unlock();
                changed=result;changed.error="changed pending completion";rejects([&]{coordinator.finish(lease.ticket,changed);});coordinator.finish(lease.ticket,result);check(!runtime.busy(),"retry uses unchanged completion");
                const auto records=ProductionStore(archive).recent("experimental_cycles");check(records.size()==1 && ProductionStore(archive).recent("cycles").isEmpty(),"experimental archive isolated from trusted terminal");
                const auto record=records.front();ExperimentalRuntime::validateRecord(record);rejects([&]{ProductionResultCodec::validate(record);});check(ExperimentalRuntime::csv(records).contains("BLOCKED_EXPERIMENTAL") && ExperimentalRuntime::summary(record).contains(ExperimentalOecsAcquisition::notice()),"offline Logs/CSV preserve experimental state");
                for(const auto& field:{"productionAdmission","triggerAssociationVerified","communicationState","schema","configId"}){auto bad=record;bad[field]=true;rejects([&]{ExperimentalRuntime::validateRecord(bad);});}
                auto bad=record;auto frames=bad["frames"].toArray();auto frame=frames[0].toObject();frame["experimentalAcquisition"]=QJsonObject{};frames[0]=frame;bad["frames"]=frames;rejects([&]{ExperimentalRuntime::validateRecord(bad);});
                const auto stopLease=coordinator.begin(RuntimeTrigger::Manual);auto stopped=RuntimeService::execute(stopLease,{},coordinator.cancellation());coordinator.stop();RuntimeService::suppressCancelled(stopLease,stopped);check(stopped.frames.isEmpty(),"late Stop clears experimental frames");coordinator.finish(stopLease.ticket,stopped);check(coordinator.state()==RuntimeState::LoadedStopped,"Stop returns passive");
                RuntimeService restarted(archive,{},RuntimeIdentity::localEngineer());check(!restarted.experimental() && !restarted.active() && ProductionStore(archive).recent("experimental_cycles").size()==2,"restart reads history without permission");rejects([&]{restarted.begin();});
                coordinator.start();const auto errorLease=coordinator.begin(RuntimeTrigger::Manual);check(QFile::rename(path,path+".hidden"),"hide source after begin");auto failed=RuntimeService::execute(errorLease,{},coordinator.cancellation());check(QFile::rename(path+".hidden",path),"restore source");check(failed.state==ExecutionState::Error,"real acquisition error");coordinator.finish(errorLease.ticket,failed);check(coordinator.state()==RuntimeState::Fault,"experimental failure closes admission");rejects([&]{coordinator.begin(RuntimeTrigger::Manual);});
            }
            EditorSession editor;const EditorContext ctx{project.project().id,project.path(),recipe,definition,{}};const auto signature=editor.signature(ctx);
            auto execution=ExecutionService{}.withExperimentalEditorReplay();const auto ticket=editor.begin(SessionJob::Execution);
            auto cycle=execution.run(definition,directory.path(),recipe,signature,{});check(cycle.state==ExecutionState::Done && cycle.industrialEvidence.isEmpty() && editor.acceptCycle(cycle,signature,ctx),"three application types share experimental flow");editor.finish(ticket);
            auto target=execution.runTool(definition,directory.path(),recipe,signature,{},blob,editor.evaluation(ctx,blob));
            check(target.state==ExecutionState::Done && target.acquisitions==0 && target.tools.front().outputs["area"].origin.experimentalAcquisition==saved,"TargetOnly retains experimental origin without acquisition");
            {QFile sidecar(path+".json");check(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"change cached evidence");auto changed=saved;changed["notice"]="changed fixture evidence";sidecar.write(QJsonDocument(changed).toJson());sidecar.close();
                check(editor.evaluation(ctx,blob).cache.isEmpty(),"JSON change invalidates source and dependent TargetOnly cache");
                check(sidecar.open(QIODevice::WriteOnly|QIODevice::Truncate),"restore cached evidence");sidecar.write(QJsonDocument(saved).toJson());}
            auto calibrationContext=ctx;calibrationContext.systemCalibration={{source,QJsonObject{{"revision",2}}}};
            check(editor.evaluation(calibrationContext,blob).cache.isEmpty(),"calibration identity change invalidates experimental cache");
            check(ExecutionService{}.runTool(definition,directory.path(),recipe,signature,{},blob,editor.evaluation(ctx,blob)).state==ExecutionState::Error,"default policy denies experimental cached input");
            const auto pending=editor.begin(SessionJob::Execution);editor.stop(SessionJob::Execution);check(editor.acceptCycle(cycle,signature,ctx) && !editor.frame && editor.cycle->state==ExecutionState::Skipped,"Stop suppresses late experimental cycle");editor.finish(pending);
            check(ExecutionService{}.run(definition,directory.path(),recipe,signature,{}).state==ExecutionState::Error,"default execution denies marked replay");
            ExecutionService injected([&](const ToolDefinition& t)->std::unique_ptr<AcquisitionAdapter>{if(t.kind=="image_file")return std::make_unique<ImageFileAdapter>(path,true);return {};});
            check(injected.run(definition,directory.path(),recipe,signature,{}).state==ExecutionState::Error,"injected experimental pixels cannot bypass default execution policy");
        }
        {
            ProjectService project;project.createProject("Two source experimental","",directory.filePath("two-source.vfproject"),"file");project.createRecipe("Two files","",ApplicationType::Measurement,"file");
            const auto first=project.setImageFile(path);project.enableMultiSource();const auto second=project.addTool("image_file");for(auto t:project.tools())if(t.id==second){t.parameters["path"]=path;project.updateTool(t);}const auto blob=project.addTool("blob");project.setConnections({{second,"image",blob,"image"}});project.save();
            RuntimeService runtime(project.path()+".production",{},RuntimeIdentity::localEngineer());runtime.setExperimental(true);runtime.prepare(*project.activeRecipe(),directory.path());const auto lease=runtime.begin();const auto result=RuntimeService::execute(lease,{},{});check(result.state==ExecutionState::Done && result.frames.size()==2 && result.frameId.isEmpty(),"experimental Runtime has distinct N source frames");runtime.finish(lease.ticket,result);const auto record=ProductionStore(project.path()+".production").recent("experimental_cycles").front();
            auto bad=record;auto frames=bad["frames"].toArray();frames[1]=frames[0];bad["frames"]=frames;rejects([&]{ExperimentalRuntime::validateRecord(bad);});
            bad=record;auto tools=bad["tools"].toArray();for(int i=0;i<tools.size();++i){auto t=tools[i].toObject();if(t["toolId"]!=blob)continue;auto outputs=t["outputs"].toObject();auto area=outputs["area"].toObject();area["value"]=123456.;outputs["area"]=area;t["outputs"]=outputs;tools[i]=t;}bad["tools"]=tools;rejects([&]{ExperimentalRuntime::validateRecord(bad);});
            runtime.setExperimental(false);check(!runtime.active(),"policy change discards admission");rejects([&]{runtime.begin();});
            runtime.setExperimental(true);const auto circle=project.addTool("circle_fit");for(auto t:project.tools())if(t.id==circle){t.parameters["mode"]="calibrated";project.updateTool(t);}project.setConnections({{second,"image",blob,"image"},{first,"image",circle,"image"}});project.save();rejects([&]{runtime.prepare(*project.activeRecipe(),directory.path());});
        }
        {IndustrialExecutionInputs inputs;inputs.leaseUse=CameraLeaseUse::Runtime;rejects([&]{acquireIndustrial(inputs,request(),"cycle","frame",1000,{},true);});}
        auto writeEvidence=[&](const QJsonObject& e){QFile f(path+".json");check(f.open(QIODevice::WriteOnly|QIODevice::Truncate),"rewrite fixture evidence");f.write(QJsonDocument(e).toJson());};
        for(const auto& field:QStringList{"triggerAssociationVerified","productionAdmission","contentId","width","serial","schema","readback","profileSnapshot"}){
            auto bad=saved;if(field=="triggerAssociationVerified" || field=="productionAdmission")bad[field]=true;else if(field=="width")bad[field]=1;else if(field=="readback" || field=="profileSnapshot")bad[field]=QJsonObject{{"schemaVersion",1}};else bad[field]=field=="serial"?QString{}:QString("tampered");
            writeEvidence(bad);rejects([&]{ImageFileService{}.load(path,true);});
        }
        writeEvidence(saved);check(QFile::rename(path+".json",path+".backup"),"hide fixture evidence");rejects([&]{ImageFileService{}.load(path,true);});check(QFile::rename(path+".backup",path+".json"),"restore fixture evidence");
        QFile raster(path);check(raster.open(QIODevice::ReadOnly),"read fixture raster");auto bytes=raster.readAll();raster.close();bytes[bytes.size()-1]=char(bytes.back()^1);check(raster.open(QIODevice::WriteOnly|QIODevice::Truncate),"tamper fixture raster");raster.write(bytes);raster.close();rejects([&]{ImageFileService{}.load(path,true);});
        rejects([&]{ExperimentalOecsAcquisition::save(*frame,path);});
        auto next=open(s);start(*next);check(next->lifecycle().state()==CameraState::Streaming,"manual reopen after shutdown");close(*next);
    }
    for(const auto& failure:QStringList{"command","timeout","geometry","incomplete","stop"}){auto s=state();s->failure=failure;auto w=open(s);start(*w);command(*w,CameraOperation::Grab,CameraState::Streaming);check(w->lifecycle().state()==CameraState::Fault && !w->take(),"failed grab yields no frame");rejects([&]{w->submit(CameraOperation::Grab);});close(*w);}
    for(int scenario=0;scenario<4;++scenario){auto s=state();if(scenario==0)s->metadata=true;if(scenario==1)s->wrongIdentity=true;if(scenario==2)s->nodes["TriggerSelector"].value=QString("FrameStart");if(scenario==3)s->nodes["Gain"].unit="unknown";
        auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Fault && !s->calls.contains("command"),"invalid configuration/identity/metadata rejected");close(*w);
    }
    {auto s=state();s->nodes["TriggerDelay"].value=500000.;s->nodes["AcquisitionBurstFrameCount"].value=qint64(3);s->nodes["TriggerCacheEnable"].value=true;
        auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Streaming && s->nodes["TriggerDelay"].value==FeatureValue(0.) && s->nodes["AcquisitionBurstFrameCount"].value==FeatureValue(qint64(1)) && s->nodes["TriggerCacheEnable"].value==FeatureValue(false),"delay/burst/cache writes and readbacks");close(*w);
    }
    for(bool valid:{true,false}){auto s=state();for(const auto& n:QStringList{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}){
            auto& f=s->nodes[n];f.type=FeatureType::Enumeration;f.value=n+"2";f.entries={{n+"1",valid?1u:0u},{n+"2",2}};
        }auto w=open(s);start(*w);check(w->lifecycle().state()==(valid?CameraState::Streaming:CameraState::Fault),"documented enum factor semantics; wrong raw value rejected");close(*w);
    }
    for(int scenario=0;scenario<3;++scenario){auto s=state();for(const auto& n:QStringList{"BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical"}){
            s->nodes[n].access=scenario==1?FeatureAccess::Unavailable:FeatureAccess::Missing;s->nodes[n].type=FeatureType::Unsupported;
        }if(scenario==2)s->nodes["BinningVertical"].access=FeatureAccess::Unavailable;
        auto w=open(s);start(*w);check(w->lifecycle().state()==(scenario==0?CameraState::Streaming:CameraState::Fault),"explicit NI pair is not-applicable; NA/mixed never default");
        if(scenario==0)check(w->readback()->geometry.binningEvidence==GeometryEvidence::NotApplicable,"NI is never reported factor readback");close(*w);
    }
    {auto s=state();s->dynamicAuto=true;s->nodes["TriggerMode"].value=QString("On");auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Streaming,"auto controls verified Off before dynamic trigger gating");close(*w);}
    for(const auto& node:QStringList{"AcquisitionBurstFrameCount","TriggerCacheEnable","TriggerDelay"}){
        auto s=state();s->nodes["TriggerDelay"].value=500000.;s->nodes["AcquisitionBurstFrameCount"].value=qint64(3);s->nodes["TriggerCacheEnable"].value=true;
        s->failure="configure:"+node;auto w=open(s);start(*w);check(w->lifecycle().state()==CameraState::Fault && s->writeFailed && s->nodes["TriggerDelay"].value==FeatureValue(500000.) && s->nodes["AcquisitionBurstFrameCount"].value==FeatureValue(qint64(3)) && s->nodes["TriggerCacheEnable"].value==FeatureValue(true),"partial experimental write rolls back original values");close(*w);
    }
    {auto s=state();std::promise<void> entered,release;auto released=release.get_future().share();s->waitHook=[&]{entered.set_value();released.wait();};
        auto w=open(s);start(*w);w->submit(CameraOperation::Grab,3000);entered.get_future().wait();w->requestStop();w->requestShutdown();check(!w->take(),"Stop clears preview immediately");
        rejects([&]{open(s);});release.set_value();until([&]{return w->shutdownState()==ShutdownState::Ready;});check(!w->take(),"late/cancelled frame suppressed");
    }
    {auto s=state();s->waitHook=[] {QThread::msleep(20);};auto w=open(s);start(*w);w->submit(CameraOperation::Grab,1);until([&]{w->poll();return w->lifecycle().state()==CameraState::Fault;});check(!w->take(),"expired frame suppressed");close(*w);}
    QTextStream(stdout)<<"PASS: experimental configuration/identity/metadata, owned delayed-old raster stays unverified, save/hash, lease, Stop/late/cancel, timeout/failure, manual reopen, native isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
