#pragma once
#include "application/MeasurementWorkspace.hpp"
#include <cmath>
// Included after the shared real localhost peer harness.
QImage measurementStripe(){QImage im(640,200,QImage::Format_Grayscale8);im.fill(10);for(int y=0;y<200;++y)for(int x=100;x<400;++x)im.scanLine(y)[x]=220;return im;}
class MeasurementFixtureAdapter final:public AcquisitionAdapter {
public:
 AcquisitionCapabilities capabilities()const override{return {"webcam",true};}
 ImageFrame acquire(const AcquisitionRequest&)const override{const auto im=measurementStripe();return ImageFrame::fromBuffer(im.constBits(),im.sizeInBytes(),640,200,im.bytesPerLine(),PixelFormat::Gray8);}
};
int measurementAcceptance(ApplicationType applicationType){
 QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/tcp-measurement-XXXXXX");check(dir.isValid(),"measurement fixture");
 ProjectService p;p.createProject("TCP measurement","",dir.filePath("part.vfproject"),"");p.save();
 MeasurementWorkspace workspace(p.path(),p.project().id,[]{return true;});MeasurementSetupContract contract;
 contract.cameras.append({{"camera"},"Fixture camera",{"webcam","fixture",DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});
 contract.setups.append({{"setup"},contract.cameras.first().id,"Fixture setup","lens","plane",1,1,{{640,200},{},{},true}});workspace.replace(contract);
 const auto camera=contract.cameras.first();const auto setup=contract.setups.first();
 IndependentCaptureRequest req;req.origin=SourceOrigin::CameraAcquired;req.camera=camera;req.setup=setup;
 IndependentCaptureBackend backend{[&]{return QVector<DeviceBinding>{camera.device};},[](const DeviceBinding&,const GeometryDescriptor&){return std::make_unique<MeasurementFixtureAdapter>();}};
 IndependentCalibrationSession session;check(session.accept(IndependentCalibrationSession::run(session.begin(req),backend)),"measurement independent raster fixture");
 auto& math=session.math();check(math.accept(CalibrationWizardSession::run(math.scanPair("width",{{"image"},ScanSegment{{20,100},{620,100},4}}))),"real caliper");math.setKnownMillimetres(1.0);const auto model=math.calculate().named("Fixture scale");
 const SetupReference ref{camera.id,setup.id,1};workspace.calibrations().save(ref,model);workspace.calibrations().setOperational(camera.id,setup.id);workspace.calibrations().activate(ref,model.contentId(),session);
 p.createRecipe("Measured part","",applicationType);p.enableMultiSource();check(measurementStripe().save(dir.filePath("part.png")),"measurement image");p.setImageFile(dir.filePath("part.png"));
 QString source;for(const auto& t:p.tools())if(t.kind=="image_file")source=t.id;
 const auto left=p.addTool("edge_point"),right=p.addTool("edge_point"),metric=p.addTool("measurement"),decision=p.addTool("result");
 for(auto t:p.tools()){
  if(t.id==left || t.id==right){t.parameters["scanRoi"]=ToolRoi{{source+"/image"},ScanSegment{{20,100},{620,100},4}}.toJson();t.parameters["polarity"]=t.id==left?"rising":"falling";}
  if(t.id==metric){t.parameters["lowerMm"]=0.99;t.parameters["upperMm"]=1.01;}
  if(t.id==decision){t.enabled=true;t.parameters=encodeResultConfig({ResultOperator::All,{{metric,"pass",2}}});}p.updateTool(t);
 }
 p.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",metric,"p1"},{right,"point",metric,"p2"}});p.bindSourceSetup(source,ref.toJson());
 VariableDefinition value;value.name="Length";value.type=VariableType::Float64;value.unit=ScalarUnit::Millimetre;value.id=p.addVariable(value);VariableDefinition accepted;accepted.name="Accepted";accepted.type=VariableType::Boolean;accepted.id=p.addVariable(accepted);
 const auto producer=p.addTool("set_variables");for(auto t:p.tools())if(t.id==producer){t.parameters=encodeAssignmentConfig({producer,1,{{value.id,ToolOutputAssignmentSource{metric,"value",2}},{accepted.id,ToolOutputAssignmentSource{metric,"pass",2}}}});p.updateTool(t);}
 EndpointProfile profile;profile.id=uuid();profile.role="server";profile.enabled=true;QTcpServer reservation;check(reservation.listen(QHostAddress::LocalHost,0),"reserve measurement port");profile.port=reservation.serverPort();reservation.close();profile.fieldMapping={{"length","length"},{"accepted","accepted"}};CommunicationProfiles profiles(dir.filePath("profiles.json"));profiles.save({{profile.id,profile}});
 OutputGroup group{uuid(),profile.id,1,true,"vf.fields/1",producer,1,{{"length",value.id},{"accepted",accepted.id}}};p.setOutputGroups({group});const auto publisher=p.addTool("publish_data");for(auto t:p.tools())if(t.id==publisher){t.enabled=true;t.parameters=encodePublishConfig({{group.id}});p.updateTool(t);}p.save();
 const auto archive=dir.filePath("production");RuntimeService runtime(archive,"unused",RuntimeIdentity::localEngineer(),{},{},{},[&](const Recipe& r){auto d=workspace.productionSources(r);d.publication=PublicationService::capture(r.definition,profiles);return d;});
 RuntimeCoordinator coordinator(runtime);InputSnapshotStore inputs;DeliveryStore ledger(archive);TriggerRouter router(coordinator,runtime,inputs,ledger,[]{return QString{};});CommunicationTcpEndpoint server(runtime,coordinator,router,inputs,ledger,archive,"unused");
 coordinator.load(*p.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);check(!server.listening(),"measurement Load network zero");coordinator.start();CommunicationTcpOptions options;options.port=profile.port;options.workcell="cell";options.project=p.project().id;options.endpoint=profile.id;options.peer="fixture-generation-1";options.outputGroupId=group.id;server.listen(options);
 Peer peer;peer.connectTo(server.port());auto config=QString::fromLatin1(runtime.active()->validation().contentId);
 peer.send({{"type","INPUT"},{"snapshotId","empty"},{"revision","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"fields",QJsonArray{}},{"sourceTime","fixture"},{"clockId","fixture"}});peer.take("STATUS");
 auto request=[&](QString id){return QJsonObject{{"type","TRIGGER"},{"requestId",id},{"sequence","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"inputSnapshotId","empty"},{"inputRevision","1"}};};
 peer.send(request("measured"));peer.take("ACCEPTED");auto result=peer.take("RESULT","measured");check(result["execution"]=="DONE" && result["decision"]=="OK" && result["resultValid"]==true,"measurement decision OK");
 const auto fields=result["fields"].toArray();check(fields.size()==2 && fields[0].toObject()["value"]==true && fields[1].toObject()["unit"]=="mm" && std::abs(fields[1].toObject()["value"].toDouble()-1.0)<0.001,"real measured mm and accepted published");
 peer.ack(result);peer.take("ACKED");check(runtime.records().size()==1,"measurement committed once");
 // Saved/draft selection cannot mutate the loaded immutable snapshot.
 const auto saved=*p.activeRecipe();for(auto t:p.tools())if(t.id==metric){t.parameters["lowerMm"]=0.0;t.parameters["upperMm"]=0.5;p.updateTool(t);}check(p.dirty() && runtime.active()->recipe().definition==saved.definition,"dirty draft preserves loaded snapshot");
 peer.send(request("draft"));peer.take("ACCEPTED");result=peer.take("RESULT","draft");check(result["decision"]=="OK","draft tolerance does not affect active measurement");peer.ack(result);peer.take("ACKED");
 coordinator.stop();p.save();coordinator.load(*p.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();config=QString::fromLatin1(runtime.active()->validation().contentId);peer.disconnect();peer.connectTo(server.port());
 peer.send({{"type","INPUT"},{"snapshotId","empty-nok"},{"revision","1"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"fields",QJsonArray{}},{"sourceTime","fixture"},{"clockId","fixture"}});peer.take("STATUS");auto nok=request("measured-nok");nok["inputSnapshotId"]="empty-nok";peer.send(nok);peer.take("ACCEPTED");result=peer.take("RESULT","measured-nok");check(result["execution"]=="DONE" && result["decision"]=="NOK" && result["resultValid"]==true && result["fields"].toArray()[0].toObject()["value"]==false && result["fields"].toArray()[1].toObject()["value"]==1.0,"real measured tolerance NOK with false accepted");peer.ack(result);peer.take("ACKED");
 peer.send({{"type","INPUT"},{"snapshotId","empty"},{"revision","2"},{"recipeId",p.activeRecipe()->id},{"configId",config},{"fields",QJsonArray{}},{"sourceTime","fixture"},{"clockId","fixture"}});peer.take("STATUS");
 server.started=[&]{coordinator.stop();};auto stopped=request("stop");stopped["inputRevision"]="2";peer.send(stopped);peer.take("ACCEPTED");result=peer.take("RESULT","stop");check(result["decision"]=="NOT_EVALUATED" && result["resultValid"]==false,"measurement Stop suppresses old OK");peer.ack(result);peer.take("ACKED");server.stop();
 QTextStream(stdout)<<checks<<" real measurement TCP acceptance checks passed (fixture only; no physical calibration acceptance)\n";return 0;
}
