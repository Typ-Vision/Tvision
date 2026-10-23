#include "CameraAcquisitionFixture.hpp"
#include "application/IndependentCalibrationCapture.hpp"
using namespace persistent_fixture;
IndependentCaptureRequest target(const Fixture& f){const auto& r=f.requests.first();IndependentCaptureRequest t;t.origin=SourceOrigin::CameraAcquired;t.camera=r.camera;t.setup=r.setup;t.profile=r.profile;t.policy=CalibrationCapturePolicy::Trusted;return t;}
IndependentCaptureBackend backend(const Fixture& f){IndependentCaptureBackend b;b.factory=f.inputs.factory;b.industrialDiscover=[discover=f.inputs.discover]{return discover(5000,{});};return b;}
void ready(IndependentCalibrationSession& s){until([&]{return !s.cameraStatus().preparing;});check(s.cameraStatus().allReady(),"camera ready");}
void drained(IndependentCalibrationSession& s){until([&]{return s.cameraStatus().drained;});}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    Fixture f("calibration");const auto r=target(f);const auto b=backend(f);auto p=f.first();IndependentCalibrationSession s;
    check(p->opens==0,"list/session construction offline");s.prepareCamera(r,b);ready(s);check(p->opens==1 && p->configs==1 && p->grabs==0,"entry prepares without acquisition");
    QSet<QString> frames;
    for(int n=0;n<20;++n){s.prepareCamera(r,b);check(s.accept(IndependentCalibrationSession::run(s.begin(r),b)),"prepared capture");frames.insert(s.evidence()->frameId);}
    check(frames.size()==20 && p->opens==1 && p->configs==1 && p->grabs==20 && *f.discoveries==1 && p->closes==0,"twenty new frames, one connection/discovery");
    {auto competing=std::make_shared<CameraAcquisitionSessions>();auto in=f.inputs;in.leaseUse=CameraLeaseUse::Runtime;const auto out=take(competing->prepare("runtime",in,f.requests,false));check(!out.allReady(),"calibration lease excludes Runtime");competing->shutdown().get();}
    QTemporaryDir tmp(QDir::currentPath()+"/calibration-session-XXXXXX");check(tmp.isValid(),"build fixture directory");QImage image(3,2,QImage::Format_Grayscale8);image.fill(17);const auto path=tmp.filePath("trial.png");check(image.save(path),"fixture file");auto file=r;file.origin=SourceOrigin::SetupFile;file.filePath=path;
    check(s.accept(IndependentCalibrationSession::run(s.begin(file),b)),"file capture after live");drained(s);check(p->closes==1 && s.evidence()->origin==SourceOrigin::SetupFile,"file switch releases camera and retains origin");
    auto changed=r;changed.profile->requested.exposureTimeUs=200;s.prepareCamera(changed,b);ready(s);check(p->opens==2 && p->configs==2,"new context rereads changed profile");s.cancel();drained(s);
    {auto runtime=std::make_shared<CameraAcquisitionSessions>();auto in=f.inputs;in.leaseUse=CameraLeaseUse::Runtime;check(take(runtime->prepare("runtime",in,f.requests,false)).allReady(),"Runtime takes released camera");s.prepareCamera(r,b);until([&]{return !s.cameraStatus().preparing;});check(!s.cameraStatus().allReady(),"busy Runtime cannot be stolen");s.cancel();drained(s);runtime->shutdown().get();}
    s.prepareCamera(r,b);ready(s);p->failGrab=true;check(!s.accept(IndependentCalibrationSession::run(s.begin(r),b)),"hardware fault rejects result");drained(s);p->failGrab=false;const int opens=p->opens;QThread::msleep(20);check(p->opens==opens,"no automatic reconnect");s.prepareCamera(r,b);ready(s);check(s.accept(IndependentCalibrationSession::run(s.begin(r),b)),"explicit retry");s.cancel();drained(s);
    for(bool opening:{true,false}){
        Fixture slow(opening?"late-open":"late-frame");auto q=slow.first();std::promise<void> release;auto gate=release.get_future().share();if(opening)q->openGate=gate;else q->grabGate=gate;
        IndependentCalibrationSession pending;auto req=target(slow);auto back=backend(slow);pending.prepareCamera(req,back);
        std::future<IndependentCaptureResult> capture;
        if(opening)until([&]{return q->openEntered.load();});else{ready(pending);auto job=pending.begin(req);capture=std::async(std::launch::async,[job,back]{return IndependentCalibrationSession::run(job,back);});until([&]{return q->grabEntered.load();});}
        pending.cancel();check(!pending.cameraStatus().drained,"Stop waits actual SDK exit");release.set_value();if(capture.valid())check(!pending.accept(capture.get()),"late frame retired");drained(pending);check(q->closes==1 && !q->wrongThread,"fixed worker and single close");
    }
    {Fixture faulty("uncertain-close");faulty.first()->failClose=true;IndependentCalibrationSession uncertain;uncertain.prepareCamera(target(faulty),backend(faulty));ready(uncertain);uncertain.cancel();drained(uncertain);check(!uncertain.cameraStatus().clean,"uncertain close remains fault");}
    {Fixture transfer("editor-calibration-runtime");auto sessions=std::make_shared<CameraAcquisitionSessions>();
        check(take(sessions->prepare("editor",transfer.inputs,transfer.requests,false)).allReady(),"Editor prepares camera");
        check(take(sessions->close("editor")).clean,"accepted Editor exit drains");
        IndependentCalibrationSession calibration;calibration.prepareCamera(target(transfer),backend(transfer));ready(calibration);calibration.cancel();drained(calibration);
        auto in=transfer.inputs;in.leaseUse=CameraLeaseUse::Runtime;check(take(sessions->prepare("runtime",in,transfer.requests,false)).allReady(),"Runtime opens after calibration drain");sessions->shutdown().get();
        check(transfer.first()->opens==3 && transfer.first()->closes==3,"Editor calibration Runtime deterministic handoff");}
    check(!p->wrongThread,"fixed owner thread");QTextStream(stdout)<<"PASS: 20 frames/one open; file release; profile readback; Runtime exclusion/handoff; explicit retry; delayed open/frame Stop; close quarantine\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
