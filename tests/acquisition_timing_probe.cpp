// Software baseline and trace contract. Physical baseline uses oecs_experimental_probe.
#include "IndustrialProviderFixture.hpp"
#include "application/IndustrialExecution.hpp"
#include "application/IndependentCalibrationCapture.hpp"
#include <QJsonArray>
#include <QElapsedTimer>
#include <QSet>
using namespace industrial_provider_fixture;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto original=AcquisitionTiming::current();
    {AcquisitionTimingBinding outer({"outer",{},"binding-test",AcquisitionTiming::Clock::now()});
        {AcquisitionTimingBinding inner({"inner",{},"binding-test",AcquisitionTiming::Clock::now()});check(AcquisitionTiming::current().request=="inner","nested timing binding");}
        check(AcquisitionTiming::current().request=="outer","restore outer timing binding");}
    check(AcquisitionTiming::current().request==original.request,"restore caller timing binding");
    const auto base=argc>1?QString::fromLocal8Bit(argv[1]):QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("timing-fixture");
    check(QDir().mkpath(base),"evidence directory");const auto trace=QDir(base).absoluteFilePath("trace.jsonl");QFile::remove(trace);
    const auto req=request();auto state=std::make_shared<State>();IndustrialExecutionInputs inputs;inputs.contract={{req.camera},{req.setup},{req.profile}};
    inputs.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(state));inputs.discover=[req](int,const auto&){return IndustrialDiscoverySnapshot{{req.device},{},false};};
    IndependentCaptureRequest capture;capture.origin=SourceOrigin::CameraAcquired;capture.camera=req.camera;capture.setup=req.setup;capture.profile=req.profile;capture.policy=CalibrationCapturePolicy::Trusted;
    IndependentCaptureBackend backend;backend.factory=inputs.factory;backend.industrialDiscover=[req]{return IndustrialDiscoverySnapshot{{req.device},{},false};};
    QJsonArray samples;
    // Interleave off/on blocks so file instrumentation cost is visible. No timing threshold.
    for(int round=0;round<3;++round)for(bool enabled:{false,true})for(const auto& flow:QStringList{"editor","runtime","calibration"}){
        if(enabled)qputenv("VISIONFLOW_ACQUISITION_TIMING",trace.toUtf8());else qunsetenv("VISIONFLOW_ACQUISITION_TIMING");
        IndependentCalibrationSession calibration;
        for(int sample=0;sample<20;++sample){
            const auto context=AcquisitionTiming::request(flow);AcquisitionTimingBinding binding(context);
            QElapsedTimer timer;timer.start();
            if(flow=="calibration"){const auto result=IndependentCalibrationSession::run(calibration.begin(capture),backend);check(result.error().isEmpty() && calibration.accept(result),"calibration sample");}
            else {inputs.leaseUse=flow=="runtime"?CameraLeaseUse::Runtime:CameraLeaseUse::Editor;const auto result=acquireIndustrial(inputs,req,"cycle",QString::number(sample),3000,{});check(result.evidence && result.image.size()==QSize(3,2),"owned execution sample");}
            samples.append(QJsonObject{{"flow",flow},{"round",round},{"sample",sample},{"timingEnabled",enabled},{"durationMs",timer.nsecsElapsed()/1e6},{"outcome","success"}});
        }
    }
    qputenv("VISIONFLOW_ACQUISITION_TIMING",trace.toUtf8());
    {AcquisitionTimingBinding binding(AcquisitionTiming::request("negative-error"));state->settings=true;reject([&]{acquireIndustrial(inputs,req,"cycle","bad",3000,{});});state->settings=false;}
    {AcquisitionTimingBinding binding(AcquisitionTiming::request("negative-cancel"));auto cancel=std::make_shared<std::atomic_bool>(false);state->hardware->hook=[cancel](const QString& call){if(call=="wait")cancel->store(true);};reject([&]{acquireIndustrial(inputs,req,"cycle","cancel",3000,cancel);});state->hardware->hook={};}
    {AcquisitionTimingBinding binding(AcquisitionTiming::request("negative-cancel-before-discovery"));auto cancel=std::make_shared<std::atomic_bool>(true);reject([&]{acquireIndustrial(inputs,req,"cycle","cancel-before-discovery",3000,cancel);});}
    {AcquisitionTimingBinding binding(AcquisitionTiming::request("negative-timeout"));state->delayMs=20;reject([&]{acquireIndustrial(inputs,req,"cycle","timeout",10,{});});state->delayMs=0;}
    qunsetenv("VISIONFLOW_ACQUISITION_TIMING");
    QFile data(QDir(base).absoluteFilePath("samples.json"));check(data.open(QIODevice::WriteOnly),"samples write");data.write(QJsonDocument(QJsonObject{{"kind","software-fixture-only"},{"runtimeScope","acquireIndustrial with Runtime lease; coordinator/persistence excluded"},{"geometry","3x2 Mono8"},{"exposureUs",100},{"gainDb",1},{"samples",samples}}).toJson());data.close();
    QFile file(trace);check(file.open(QIODevice::ReadOnly),"trace read");QSet<QString> sessions;QMap<QString,int> opens,configs,closes;bool error=false,timeout=false;int cancelled=0;
    while(!file.atEnd()){const auto row=QJsonDocument::fromJson(file.readLine()).object();check(row["inclusive"].toBool() && !row["requestId"].toString().isEmpty(),"trace identity");check(row["durationMs"].toDouble()>=0,"monotonic span");
        const auto flow=row["flow"].toString(),stage=row["stage"].toString();if(flow.startsWith("negative")){if(stage=="industrial.acquire"){error|=row["outcome"]=="error";cancelled+=row["outcome"]=="cancelled"?1:0;timeout|=row["outcome"]=="timeout";}continue;}
        if(stage=="sdk.open-total"){++opens[flow];sessions.insert(row["sessionId"].toString());}if(stage=="configure.readback")++configs[flow];if(stage=="sdk.close")++closes[flow];
    }
    check(!sessions.contains("") && sessions.size()==180,"distinct session per baseline frame");for(const auto& flow:QStringList{"editor","runtime","calibration"})check(opens[flow]==60 && configs[flow]==60 && closes[flow]==60,"baseline repeated open/configure/close counts");check(error && cancelled==2 && timeout,"failure/cancel/timeout samples separate");
    QTextStream(stdout)<<"PASS: 3 x 20 per flow off/on; 180 traced open/configure/close, nested binding restoration, error/cancel/timeout isolation\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
