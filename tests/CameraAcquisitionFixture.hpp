#pragma once
#include "application/CameraAcquisitionSessions.hpp"
#include "application/ProjectService.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QThread>
#include <QJsonDocument>
#include <QTextStream>
#include <QSaveFile>
#include <set>
using namespace visionflow;
namespace persistent_fixture {
inline void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
template<class F> void until(F f){Deadline d(5000);while(!f()){check(!d.expired(),"fixture timeout");QThread::msleep(1);}}
template<class T> T take(std::future<T> future){check(future.wait_for(std::chrono::seconds(6))==std::future_status::ready,"future timeout");return future.get();}
struct Probe {
    std::atomic_int created=0,opens=0,configs=0,grabs=0,closes=0,destroyed=0;
    std::atomic_bool wrongThread=false,openEntered=false,grabEntered=false,closeEntered=false;
    bool failConfigure=false,failGrab=false,failClose=false;
    std::shared_future<void> openGate,grabGate,closeGate;
};
inline IndustrialCameraRequest request(QString serial="persistent"){
    IndustrialCameraRequest r;
    r.device={{"VF","SESSION",serial,DeviceIdentityConfidence::SerialReported},{"oecs",{},"fixture","exact"},{"oecs","fixture",QString(64,'a'),1}};
    r.camera={{serial+"/camera"},"Synthetic camera",{"gentl",{},DeviceIdentityConfidence::Unavailable,1,{}},r.device.identity};
    GeometryDescriptor g;g.resolution={3,2};g.sensorRoi=QRect(0,0,3,2);g.binning=QSize(1,1);g.decimation=QSize(1,1);g.reverse=GeometryMirror{};g.complete=true;
    r.setup={{serial+"/setup"},r.camera.id,"Synthetic setup","lens","plane",1,1,g};
    r.profile={{serial+"/profile"},r.camera.id,r.setup.id,"Synthetic profile",1,{"Mono8",{true,"FrameStart","Software"},AutoMode::Off,100,AutoMode::Off,1,{}}};
    r.sourceId="source-A";r.validate();return r;
}
class Backend final:public IndustrialCameraBackend {
    IndustrialCameraRequest r_;
    std::shared_ptr<Probe> p_;
    std::thread::id thread_=std::this_thread::get_id();
    bool opened_=false;
    void thread() const {if(thread_!=std::this_thread::get_id())p_->wrongThread=true;}
public:
    Backend(IndustrialCameraRequest r,std::shared_ptr<Probe> p):r_(std::move(r)),p_(std::move(p)){++p_->created;}
    ~Backend() override {thread();++p_->destroyed;}
    PhysicalDeviceIdentity actualIdentity() const override {thread();return r_.device.identity;}
    AcquisitionReadback readback() const override {
        thread();GeometryReadback g;g.resolution=r_.setup.geometry.resolution;g.offset=r_.setup.geometry.sensorRoi->topLeft();g.binning=*r_.setup.geometry.binning;g.decimation=*r_.setup.geometry.decimation;g.reverse=*r_.setup.geometry.reverse;
        g.offsetEvidence=g.binningEvidence=g.decimationEvidence=g.reverseEvidence=GeometryEvidence::Reported;return {g,r_.profile.requested};
    }
    CameraWorkerResult execute(const CameraTicket& t) override {
        thread();if(t.operation==CameraOperation::Connect){p_->openEntered=true;if(p_->openGate.valid())p_->openGate.wait();opened_=true;++p_->opens;}
        if(t.operation==CameraOperation::Configure){++p_->configs;if(p_->failConfigure)throw std::runtime_error("fixture configuration failure");}
        if(t.operation==CameraOperation::Grab){p_->grabEntered=true;if(p_->grabGate.valid())p_->grabGate.wait();if(p_->failGrab)throw std::runtime_error("fixture capture failure");
            const int n=++p_->grabs;const unsigned char pixels[6]={static_cast<unsigned char>(n),2,3,4,5,6};FrameMetadata m;
            m.sourceId=r_.sourceId;m.frameId=r_.sourceId+"/hardware-"+QString::number(n);m.cycleId="backend-cycle-"+QString::number(n);m.coordinateSystemId=r_.sourceId+"/image";m.completedAtUtc=QDateTime::currentDateTimeUtc();
            auto image=ImageFrame::fromBuffer(pixels,6,3,2,3,PixelFormat::Gray8,m);return {true,{},AcquiredFrame{image,m.sourceId,m.cycleId,m.frameId,m.completedAtUtc}};
        }return {true,{},{}};
    }
    void shutdown() override {thread();p_->closeEntered=true;if(p_->closeGate.valid())p_->closeGate.wait();if(opened_){++p_->closes;opened_=false;}if(p_->failClose)throw std::runtime_error("fixture uncertain close");}
};
class Provider final:public IndustrialCameraProvider {
    QMap<QString,std::shared_ptr<Probe>> probes_;
public:
    explicit Provider(QMap<QString,std::shared_ptr<Probe>> p):probes_(std::move(p)){}
    IndustrialCameraCapabilities capabilities(const DiscoveredDevice& d) const override {return {probes_.contains(d.identity.serial),true,true,true,true,true,{}};}
    std::unique_ptr<IndustrialCameraBackend> create(const IndustrialCameraRequest& r) const override {check(probes_.contains(r.device.identity.serial),"unknown fixture serial");return std::make_unique<Backend>(r,probes_[r.device.identity.serial]);}
};
struct Fixture {
    QVector<IndustrialCameraRequest> requests;
    QMap<QString,std::shared_ptr<Probe>> probes;
    IndustrialExecutionInputs inputs;
    std::shared_ptr<std::atomic_int> discoveries=std::make_shared<std::atomic_int>(0);
    explicit Fixture(QString serial,int cameras=1){
        QVector<DiscoveredDevice> devices;
        for(int i=0;i<cameras;++i){auto r=request(serial+QString::number(i));r.sourceId="source-"+QString::number(i);requests.append(r);devices.append(r.device);probes[r.device.identity.serial]=std::make_shared<Probe>();
            inputs.contract.cameras.append(r.camera);inputs.contract.setups.append(r.setup);inputs.contract.profiles.append(r.profile);}
        inputs.factory=CameraProviderFactory::forTests(std::make_shared<Provider>(probes));inputs.discover=[devices,count=discoveries](int,const auto&){++*count;return IndustrialDiscoverySnapshot{devices,{},false};};
    }
    std::shared_ptr<Probe> first(){return probes[requests.first().device.identity.serial];}
};
}
