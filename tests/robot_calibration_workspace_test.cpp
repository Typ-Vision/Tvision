#include "application/RobotCalibrationWorkspace.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <thread>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);int checks=0;try{
 auto check=[&](bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);};using A=RobotCalibrationAction;
 RobotCalibrationWorkspace w;const auto station=QUuid::createUuid();auto v=w.view(station,HandEyeMode::EyeToHand);
 check(v.physical.contains("BLOCKED") && !v.actualPoseReady && v.active.isNull() && v.candidate.isNull(),"Unbound projection never fabricates readiness/pose/model");
 for(auto a:{A::Begin,A::SampleSolve,A::SampleHoldout,A::Solve,A::Validate,A::Activate,A::Abort,A::Recover})check(!w.disabled(v,a).isEmpty(),"Unbound commands unavailable");
 v.station=station;v.revision=1;v.connected=true;v.commands={{A::Begin,{}},{A::SampleSolve,{}},{A::SampleHoldout,{}},{A::Solve,{}},{A::Validate,{}},{A::Activate,{}},{A::Abort,{}},{A::Recover,{}}};int executed=0;
 w.bind([&](const QUuid&,HandEyeMode){return v;},[&](const RobotCalibrationCommand&){++executed;return QString("service result");});
 check(w.disabled(v,A::Begin).contains("PHYSICAL BLOCKED"),"Provider command availability alone cannot bypass physical gate");v.fixture=true;
 check(w.disabled(v,A::SampleSolve).contains("Actual pose") && w.disabled(v,A::SampleHoldout).contains("Actual pose"),"Both roles require actual pose");
 v.actualPoseReady=true;check(w.disabled(v,A::SampleSolve).isEmpty(),"Explicit actual readiness plus application adapter permits capture");
 check(w.command({A::Begin,v})=="service result" && executed==1,"Typed application command dispatched");
 auto old=v;++v.revision;bool stale=false;try{w.command({A::Activate,old});}catch(...){stale=true;}check(stale && executed==1,"Revision race rejected before execution");
 for(int n=0;n<5;++n){old=v;switch(n){case 0:old.session=QUuid::createUuid();break;case 1:old.candidate=QUuid::createUuid();break;case 2:old.candidateRevision=QUuid::createUuid();break;case 3:old.validation=QUuid::createUuid();break;case 4:old.activeRevision=QUuid::createUuid();break;}stale=false;try{w.command({A::Activate,old});}catch(...){stale=true;}check(stale,"Exact model/session identity fence");}
 v.recoveryRequired=true;for(auto a:{A::Begin,A::SampleSolve,A::Solve,A::Validate,A::Activate})check(w.disabled(v,a).contains("RECOVERY_REQUIRED"),"Recovery closes new work");check(w.disabled(v,A::Abort).isEmpty() && w.disabled(v,A::Recover).isEmpty(),"Recovery preserves explicit abort/recovery");
 v.recoveryRequired=false;v.draining=true;check(w.disabled(v,A::SampleHoldout).contains("DRAINING"),"Drain closes capture");v.draining=false;v.connected=false;check(!w.disabled(v,A::Solve).isEmpty() && w.disabled(v,A::Abort).isEmpty(),"Disconnect fences work but permits abort");
 v.connected=true;v.commands[A::Validate]="HOLDOUT_MISSING";check(w.disabled(v,A::Validate)=="HOLDOUT_MISSING","Application reject reason preserved");v.commands.remove(A::Solve);check(!w.disabled(v,A::Solve).isEmpty(),"Absent adapter unavailable");
 bool threadRejected=false;std::thread other([&]{try{w.view(station,v.mode);}catch(...){threadRejected=true;}});other.join();check(threadRejected,"Owner-thread boundary");
 bool context=false;try{w.view(QUuid::createUuid(),v.mode);}catch(...){context=true;}check(context,"Foreign station projection rejected");
 QTextStream(stdout)<<"PASS "<<checks<<" calibration application checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
