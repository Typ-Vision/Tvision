#include "CameraAcquisitionFixture.hpp"
#include <barrier>
using namespace persistent_fixture;
namespace {
int assertions=0;
void verify(bool ok,const char* message){++assertions;check(ok,message);}
void atomicRaces(){
    auto a=request("arbiter-A"),b=request("arbiter-B"),c=request("arbiter-C");
    const CameraLeaseTarget A{a.camera.id.value,*a.camera.physical},B{b.camera.id.value,*b.camera.physical},C{c.camera.id.value,*c.camera.physical};
    for(int n=0;n<50;++n){
        std::barrier gate(2);quint32 left=99,right=99;
        auto run=[&](bool reverse,quint32& reason){gate.arrive_and_wait();auto r=CameraLeaseArbiter::reserve(reverse?"reverse":"forward",CameraLeaseUse::Runtime,reverse?QVector<CameraLeaseTarget>{B,A}:QVector<CameraLeaseTarget>{A,B});reason=r.reason;gate.arrive_and_wait();};
        std::thread first(run,false,std::ref(left)),second(run,true,std::ref(right));first.join();second.join();
        verify((left==0 && right==3)||(left==3 && right==0),"reverse camera requests have exactly one winner without wait/deadlock");
        auto all=CameraLeaseArbiter::reserve("after-race",CameraLeaseUse::Runtime,{A,B});verify(!all.reason && all.leases.size()==2,"race leaves no partial ownership");
    }
    auto held=CameraLeaseArbiter::reserve("S1",CameraLeaseUse::Runtime,{A});
    auto denied=CameraLeaseArbiter::reserve("S2",CameraLeaseUse::Runtime,{C,A});
    auto independent=CameraLeaseArbiter::reserve("S3",CameraLeaseUse::Calibration,{C});
    verify(held.reason==0 && denied.reason==3 && denied.leases.isEmpty() && independent.reason==0,"failed atomic reservation does not retain first free camera");
    verify(CameraLeaseArbiter::reserve("alias",CameraLeaseUse::Preview,{{"other-UUID",*a.camera.physical}}).reason==3,"physical alias cannot bypass UUID ownership");
    verify(CameraLeaseArbiter::reserve("rebind",CameraLeaseUse::Editor,{{a.camera.id.value,*b.camera.physical}}).reason==3,"UUID cannot be rebound during ownership");
    quint64 old=held.leases.first()->generation();held.leases.clear();
    auto next=CameraLeaseArbiter::reserve("S2",CameraLeaseUse::Runtime,{A});verify(next.leases.first()->generation()>old,"new owner gets a monotonically newer lease generation");
    bool owner=false;for(const auto& d:CameraLeaseArbiter::diagnostics())if(d.owner=="S2" && d.cameraId==A.cameraId && d.use==CameraLeaseUse::Runtime && d.generation==next.leases.first()->generation())owner=true;
    verify(owner,"diagnostics expose exact owner/use/UUID/generation");
}
void stationAndDrain(){
    Fixture f("station-arbiter",2);auto runtime=f.inputs;runtime.leaseUse=CameraLeaseUse::Runtime;
    CameraAcquisitionSessions s1,s2,s3;auto a=f.requests[0],b=f.requests[1];
    auto ready=take(s1.prepare("S1",runtime,{a},false));verify(ready.allReady(),"S1 owns A");
    auto busy=take(s2.prepare("S2",runtime,{a},false));verify(!busy.allReady() && busy.resourceReason==3,"S2 receives nonterminal CAMERA_BUSY");
    auto other=take(s3.prepare("S3",runtime,{b},false));auto result=take(s3.acquire("S3",other.generation,b,"cycle-B","frame-B",3000));
    verify(result.image.metadata().cycleId=="cycle-B" && result.evidence,"S3 independently completes B while S1 holds A");
    auto preview=a;preview.profile.requested.trigger.enabled=false;
    rejects([&]{f.inputs.factory.open(preview,CameraLeaseUse::Preview,FrameQueueMode::PreviewLatest,1);});
    rejects([&]{f.inputs.factory.open(a,CameraLeaseUse::Calibration,FrameQueueMode::ProductionFifo,1);});
    verify(f.first()->opens==1,"preview/calibration cannot open the station camera through another registry");
    auto probe=f.first();std::promise<void> gate;probe->grabGate=gate.get_future().share();
    auto oldFrame=s1.acquire("S1",ready.generation,a,"old-cycle","old-sample",3000);until([&]{return probe->grabEntered.load();});
    rejects([&]{s1.prepare("S1",runtime,{a},false);});
    auto closing=s1.close("S1",1);until([&]{return s1.status("S1").closeBudgetExceeded;});
    auto blocked=take(s2.prepare("S2",runtime,{a},false));bool stillDraining=closing.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready;
    gate.set_value();rejects([&]{take(std::move(oldFrame));});verify(take(std::move(closing)).clean,"Abort/Stop waits real drain");
    verify(stillDraining && blocked.resourceReason==3,"late hardware completion keeps ownership after close budget");
    probe->grabGate={};auto next=take(s2.prepare("S2",runtime,{a},false));verify(next.allReady(),"S2 admission succeeds after real drain");
    rejects([&]{s1.acquire("S1",ready.generation,a,"stale","stale",3000);});
    auto fresh=take(s2.acquire("S2",next.generation,a,"new-cycle","new-sample",3000));
    verify(fresh.image.metadata().frameId=="new-sample" && fresh.evidence->acquired.frameId.endsWith("hardware-2"),"late old sample is never assigned to the new request");
    verify(take(s2.close("S2")).clean && take(s3.close("S3")).clean,"station drains");
    verify(s1.shutdown().get().clean && s2.shutdown().get().clean && s3.shutdown().get().clean,"station shutdown");
}
void rollbackAndNativeGate(){
    Fixture f("rollback-group",3);auto probe=f.first();probe->failConfigure=true;CameraAcquisitionSessions failed,other;
    auto status=take(failed.prepare("failed",f.inputs,f.requests,false));verify(!status.allReady(),"prepare failure cannot leave partial Ready");
    check(take(failed.close("failed")).clean,"failed group real drain");probe->failConfigure=false;
    auto reversed=f.requests;std::reverse(reversed.begin(),reversed.end());auto ready=take(other.prepare("retry-reverse",f.inputs,reversed,false));verify(ready.allReady(),"all cameras become available after failed acquisition preparation drains");
    f.probes[f.requests[1].device.identity.serial]->failGrab=true;
    rejects([&]{take(other.acquire("retry-reverse",ready.generation,f.requests[1],"fail","fail",3000));});
    check(take(other.close("retry-reverse")).clean,"failed capture group drain");
    for(const auto& p:f.probes)verify(p->closes==p->opens,"acquisition failure releases every group camera after drain");
    auto native=f.inputs;native.factory=CameraProviderFactory{};CameraAcquisitionSessions unavailable;
    auto closed=take(unavailable.prepare("native",native,{f.requests[0]},false));verify(!closed.allReady() && closed.resourceReason==0 && f.first()->opens==2,"lease and fixture history do not promote native unavailable");
    check(take(unavailable.close("native")).clean,"unavailable cleanup");
    verify(failed.shutdown().get().clean && other.shutdown().get().clean && unavailable.shutdown().get().clean,"rollback shutdown");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    atomicRaces();stationAndDrain();rollbackAndNativeGate();
    QJsonObject report{{"status","PASS"},{"assertions",assertions},{"reverseRaces",50},{"physicalAcceptance",false},{"nativeAdmission",false},{"fixture","CameraAcquisitionFixture / exact VF SESSION synthetic 3x2 Mono8"}};
    if(argc>1){QSaveFile output(QString::fromLocal8Bit(argv[1]));check(output.open(QIODevice::WriteOnly),"report open");output.write(QJsonDocument(report).toJson());check(output.commit(),"report commit");}
    QTextStream(stdout)<<QJsonDocument(report).toJson();return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1;}}
