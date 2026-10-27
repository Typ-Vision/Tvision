#include "application/TriggerRouter.hpp"
#include "application/ProjectService.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QTextStream>
#include <future>
#include <QUuid>
#include <QLockFile>
using namespace visionflow;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(...){failed=true;}check(failed,"expected rejection");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/router-XXXXXX");check(dir.isValid(),"fixture");
 ProjectService project;project.createProject("Router","",dir.filePath("p.vfproject"),"file");project.createRecipe("Saved","",ApplicationType::DefectInspection,"file");
 QImage image(32,24,QImage::Format_Grayscale8);image.fill(255);check(image.save(dir.filePath("frame.png")),"image");project.setImageFile(dir.filePath("frame.png"));project.save();
 RuntimeService runtime(dir.filePath("production"),dir.filePath("catalog.json"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);
 coordinator.load(*project.activeRecipe(),dir.path(),RuntimeTrigger::SoftwareTrigger);coordinator.start();
 qint64 now=100;InputSnapshotStore inputs([&]{return now;});DeliveryStore ledger(dir.filePath("delivery"));bool editorBusy=false;
 TriggerRouter router(coordinator,runtime,inputs,ledger,[&]{return editorBusy?QString("NOT_READY"):QString{};});
 auto active=runtime.active();ValueEnvelope value;value.value=false;value.validity=ValueValidity::Valid;
 InputSnapshot s{"a","peer","snapshot",active->recipe().id,QString::fromLatin1(active->validation().contentId),"remote time","remote clock",1,{{"ready",value}}};inputs.put(s);
 QMap<QString,InputRequirement> required{{"ready",{value,20}}};
 TriggerRequest request{"cell","project","a","peer","r1",s.recipeId,s.configId,s.id,1,1};
 auto admit=[&](const TriggerRequest& r){return router.admit(r,required);};
 auto wrong=request;wrong.configId="wrong";check(admit(wrong).reason=="SNAPSHOT_MISMATCH" && !runtime.busy(),"config rejection before acquisition");wrong=request;wrong.recipeId="wrong";check(admit(wrong).reason=="SNAPSHOT_MISMATCH","recipe mismatch");
 auto partial=s;partial.id="partial";partial.fields.clear();inputs.put(partial);wrong=request;wrong.inputSnapshotId=partial.id;check(admit(wrong).reason=="INPUT_PARTIAL","partial rejected");
 auto bad=s;bad.id="bad";auto missing=value;missing.value.reset();missing.validity=ValueValidity::Missing;missing.reason="not sampled";bad.fields["ready"]=missing;inputs.put(bad);wrong.inputSnapshotId=bad.id;check(admit(wrong).reason=="INPUT_QUALITY","bad quality rejected");
 auto typed=s;typed.id="type";auto number=value;number.type=ScalarType::Int32;number.value=qint32(2);typed.fields["ready"]=number;inputs.put(typed);wrong.inputSnapshotId=typed.id;check(admit(wrong).reason=="INPUT_TYPE","typed profile enforced");
 SpatialTransform pose;pose.source={"camera","","",QUuid::createUuid().toString(QUuid::WithoutBraces),1};pose.target={"","robot","",QUuid::createUuid().toString(QUuid::WithoutBraces),1};
 SpatialAssignmentValue spatial;spatial.value=pose;spatial.validity=ValueValidity::Valid;auto robot=typed;robot.id="robot";robot.fields={{"pose",spatial}};inputs.put(robot);
 auto robotRequest=request;robotRequest.inputSnapshotId=robot.id;QMap<QString,InputRequirement> poseRequired{{"pose",{spatial,20}}};
 auto partialPose=robot;partialPose.id="partial-pose";partialPose.fields={{"x",number}};inputs.put(partialPose);robotRequest.inputSnapshotId=partialPose.id;
 check(router.admit(robotRequest,poseRequired).reason=="INPUT_PARTIAL","scalar x is not a complete pose");
 auto broken=robot;broken.id="broken-pose";auto invalidSpatial=spatial;invalidSpatial.value->rotation.w=2;broken.fields["pose"]=invalidSpatial;rejects([&]{inputs.put(broken);});
 auto wrongFrame=spatial;wrongFrame.value->source.revision=2;robotRequest.inputSnapshotId=robot.id;check(router.admit(robotRequest,{{"pose",{wrongFrame,20}}}).reason=="INPUT_TYPE","frame revision checked");
 now=121;check(admit(request).reason=="INPUT_STALE","stale rejected");inputs.put(s);check(admit(request).reason=="INPUT_STALE","duplicate input never refreshes age");now=100;
 auto conflict=s;conflict.fields["ready"]=number;rejects([&]{inputs.put(conflict);});s.fields.clear();check(inputs.get("a","peer","snapshot",1)->fields.size()==1,"immutable deep copy");
 wrong=request;wrong.inputRevision=2;check(admit(wrong).reason=="INPUT_MISMATCH","exact revision required");
 editorBusy=true;check(admit(request).reason=="NOT_READY" && !runtime.busy(),"Editor acquisition guard");editorBusy=false;
 coordinator.setDeliveryPending(true);check(admit(request).reason=="DELIVERY_PENDING","pending gate");coordinator.setDeliveryPending(false);
 check(runtime.records().isEmpty() && !runtime.busy(),"all rejections acquire zero and create no terminal");
 auto accepted=admit(request);check(accepted.accepted() && accepted.inputs && !std::get<bool>(*std::get<ValueEnvelope>(accepted.inputs->fields["ready"]).value),"valid false accepted");
 auto second=request;second.endpoint="b";second.requestId="r2";check(admit(second).reason=="BUSY","simultaneous second endpoint BUSY");
 check(admit(request).reason=="DUPLICATE" && !admit(request).cycle,"duplicate cannot issue lease");auto changed=request;changed.sequence++;check(admit(changed).reason=="CONFLICT","sequence fingerprint conflict");
 auto result=RuntimeService::execute(*accepted.cycle,dir.filePath("catalog.json"),coordinator.cancellation());check(result.acquisitions==1,"exactly one acquisition");QLockFile recordLock(dir.filePath("production/archive.lock"));check(recordLock.tryLock(),"record lock");rejects([&]{coordinator.finish(accepted.cycle->ticket,result);});
 check(admit(second).reason=="RECORDING_PENDING","record pending reason preserved");recordLock.unlock();coordinator.retryRecording();
 check(admit(request).reason=="DUPLICATE","completed duplicate no acquisition");
 // Durable evidence supersedes process state and never becomes a new lease.
 auto durable=request;durable.requestId="durable";ledger.request(TriggerRouter::deliveryRequest(durable));check(admit(durable).existingState=="RECOVERY_REQUIRED","durable intent requires recovery");
 auto changedDurable=durable;changedDurable.sequence++;check(admit(changedDurable).reason.startsWith("CONFLICT"),"durable fingerprint conflict");
 DeliveryStore reopened(dir.filePath("delivery"));TriggerRouter fresh(coordinator,runtime,inputs,reopened,[]{return QString{};});check(fresh.admit(durable,required).existingState=="RECOVERY_REQUIRED","reopen refuses automatic acquisition");
 coordinator.stop();check(admit(request).reason=="DUPLICATE","duplicate prior to Stop gate");check(admit(second).reason=="NOT_RUNNING","Stop gate");
 check(std::async(std::launch::async,[&]{try{admit(second);}catch(const ProductionException& e){return e.code()==ProductionError::WrongThread;}return false;}).get(),"owner serialization enforced");
 coordinator.load(*project.activeRecipe(),dir.path(),RuntimeTrigger::Manual);coordinator.start();check(admit(second).reason=="TRIGGER_SOURCE_DISABLED","manual preserved");auto manual=coordinator.begin(RuntimeTrigger::Manual);coordinator.finish(manual.ticket,RuntimeService::execute(manual,dir.filePath("catalog.json"),coordinator.cancellation()));
 check(runtime.records().size()==2,"manual and routed cycles only");
 QTextStream(stdout)<<"PASS: immutable typed inputs, quality/age/revision/config, shared guard, two endpoint arbitration, duplicate/conflict, durable recovery, Stop/manual/owner gates\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
