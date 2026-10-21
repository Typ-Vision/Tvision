// Explicit manual hardware run; never an automatic hardware CTest.
#include "application/ExperimentalOecsAcquisition.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QThread>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);QJsonObject out{{"productionAdmission",false},{"triggerAssociationVerified",false}};int rc=0;
    try{
        if(argc!=5 && argc!=6)throw std::runtime_error("usage: oecs_experimental_probe absolute-dll absolute-evidence-directory exposure-us count [--stop-in-flight|--timeout]");
        const QString scenario=argc==6?QString::fromLocal8Bit(argv[5]):QString{};
        if(!scenario.isEmpty() && scenario!="--stop-in-flight" && scenario!="--timeout")throw std::runtime_error("unknown manual scenario");
        const QString directory=QString::fromLocal8Bit(argv[2]);QDir().mkpath(directory);
        bool ok=false;const double exposure=QString::fromLocal8Bit(argv[3]).toDouble(&ok);if(!ok)throw std::runtime_error("numeric exposure required");
        const int count=QString::fromLocal8Bit(argv[4]).toInt();if(count<1 || count>1000)throw std::runtime_error("count 1..1000 required");
        IndustrialCameraRequest r;
        r.device={{"Opto Engineering Srl","COE-050-M-USB-050-IR-C","02A2220000627",DeviceIdentityConfidence::SerialReported},
            {"oecs",{},"usb","exact"},{"oecs","experimental-native",QString(64,'a'),1}};
        r.camera={{"oecs-exp"},"Manual experimental camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},r.device.identity};
        GeometryDescriptor g;g.resolution={2448,2048};g.sensorRoi=QRect(0,0,2448,2048);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
        r.setup={{"scene"},r.camera.id,"Experimental scene","Lens unidentified","Scene reference unverified",1,1,g};
        r.profile={{"experimental"},r.camera.id,r.setup.id,"Manual experimental",1,{"Mono8",{true,"FrameBurstStart","Software"},AutoMode::Off,exposure,AutoMode::Off,0,{}}};r.sourceId="manual-experimental-preview";
        auto worker=ExperimentalOecsAcquisition::open(r,QString::fromLocal8Bit(argv[1]));
        const auto wait=[&](CameraState desired){Deadline deadline(32000);for(;;){worker->poll();const auto state=worker->lifecycle().state();if(state==CameraState::Fault)throw std::runtime_error(worker->lifecycle().error().toStdString());if(state==desired)return;if(deadline.expired())throw std::runtime_error("controller wait timeout");QThread::msleep(1);}};
        const auto command=[&](CameraOperation op,CameraState state){worker->submit(op,3000);wait(state);};
        command(CameraOperation::Connect,CameraState::Connected);command(CameraOperation::Configure,CameraState::Configured);command(CameraOperation::Start,CameraState::Streaming);
        QJsonArray samples;
        for(int i=0;i<count;++i){command(CameraOperation::Grab,CameraState::Streaming);auto f=worker->take();if(!f)throw std::runtime_error("missing owned frame");
            auto evidence=f->image.metadata().experimentalAcquisition;evidence["contentId"]=QString::fromLatin1(f->image.contentId().toHex());samples.append(evidence);
            if(i==0 || i==count-1)ExperimentalOecsAcquisition::save(*f,QDir(directory).absoluteFilePath(QString("frame-%1.pgm").arg(i)));
        }
        if(scenario=="--stop-in-flight"){
            worker->submit(CameraOperation::Grab,3000);QThread::msleep(10);worker->requestStop();
            out["stopRequestedDuringPendingGrab"]=true;
            out["physicalCommandInFlightProven"]=false;
        }else if(scenario=="--timeout"){
            worker->submit(CameraOperation::Grab,1);Deadline observation(5000);
            while(worker->lifecycle().state()!=CameraState::Fault){worker->poll();if(observation.expired())throw std::runtime_error("1ms deadline did not fault");QThread::msleep(1);}
            out["deadlineFaultObserved"]=true;out["deadlineFault"]=worker->lifecycle().error();out["physicalCommandInFlightProven"]=false;
        }
        worker->requestStop();worker->requestShutdown();Deadline cleanup(5000);while(worker->shutdownState()!=ShutdownState::Ready){if(cleanup.expired())throw std::runtime_error("cleanup wait timeout");QThread::msleep(1);}
        if(worker->take())throw std::runtime_error("frame escaped stopped/expired request");out["stoppedQueueEmpty"]=true;out["shutdownReady"]=true;
        out["samples"]=samples;out["count"]=count;out["status"]="EXPERIMENTAL_OWNED_FRAMES";
    }catch(const std::exception& e){rc=1;out["status"]="ERROR";out["error"]=QString::fromUtf8(e.what());}
    QTextStream(stdout)<<QJsonDocument(out).toJson();return rc;
}
