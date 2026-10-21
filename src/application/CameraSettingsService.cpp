#include "CameraSettingsService.hpp"
#include <cmath>
#include <exception>
#include <QFileInfo>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
FeatureValue number(const CameraFeature& f,double value){
    require(std::isfinite(value),"Non-finite setting");
    if(f.type==FeatureType::Float)return value;
    require(f.type==FeatureType::Integer && std::trunc(value)==value && std::abs(value)<=9007199254740991.,"Nonrepresentable setting");
    return qint64(value);
}
void numeric(const CameraFeature& f,const QStringList& units){
    require(f.readable() && (f.type==FeatureType::Float || f.type==FeatureType::Integer),"Numeric readback unsupported");
    require(units.contains(f.unit),"Unproven setting unit");
    const double v=f.type==FeatureType::Float?std::get<double>(f.value):double(std::get<qint64>(f.value));
    require(std::isfinite(v) && std::isfinite(f.minimum) && std::isfinite(f.maximum) && f.minimum<=v && v<=f.maximum,"Invalid setting limits/readback");
}
CameraSettingsSnapshot inspect(IndustrialCameraBackend& backend){
    CameraSettingsSnapshot s;
    s.exposure=backend.setting("ExposureTime");s.gain=backend.setting("Gain");
    s.exposureAuto=backend.setting("ExposureAuto");s.gainAuto=backend.setting("GainAuto");
    s.actual=backend.readback();s.actual.validate();return s;
}
void checkLive(const CameraTicket& t){require(!t.cancel->load() && !t.deadline.expired(),"Settings cancelled/expired");}
}
CameraSettingsSession::~CameraSettingsSession(){if(applying_)invalidate_();}
void CameraSettingsSession::connect(int timeout){require(gate_(),"Workspace busy");worker_->submit(CameraOperation::Connect,timeout);}
void CameraSettingsSession::apply(CameraSettingsValues values,int timeout){
    require(gate_(),"Workspace busy");
    require(repository_->load().encode()==expected_.encode(),"Stale settings registry; reopen required");
    require(worker_->lifecycle().state()==CameraState::Connected,"Reopen settings session before applying again");
    {std::lock_guard lock(shared_->mutex);shared_->desired=values;}
    worker_->submit(CameraOperation::Configure,timeout);applying_=true;
}
bool CameraSettingsSession::poll(){
    const bool accepted=worker_->poll();const auto current=state();
    if(applying_ && (current==CameraState::Configured || current==CameraState::Fault)){
        applying_=false;const auto result=snapshot();
        // The lease remains held across owner-thread invalidation. A true no-op
        // keeps availability; failed/changed settings retain conservative policy.
        if(current==CameraState::Fault || result.deviceChanged || result.profileChanged || result.geometryChanged)invalidate_();
    }
    return accepted;
}
CameraSettingsSnapshot CameraSettingsSession::snapshot() const {
    (void)worker_->lifecycle().state(); // owner-thread assertion
    std::lock_guard lock(shared_->mutex);return shared_->snapshot;
}
CameraState CameraSettingsSession::state() const{return worker_->lifecycle().state();}
QString CameraSettingsSession::error() const{return worker_->lifecycle().error();}
void CameraSettingsSession::stop(){if(applying_){applying_=false;invalidate_();}worker_->requestShutdown();}

std::unique_ptr<CameraSettingsSession> CameraSettingsService::open(MeasurementEnvironmentRepository repo,
    const CameraId& id,int revision,const IndustrialDiscoverySnapshot& discovery,const CameraLocalConfiguration& local,
    const CameraProviderFactory& factory,std::function<bool()> gate,std::function<void()> invalidate,
    MeasurementEnvironmentRepository::Fault fault){
    require(bool(gate) && bool(invalidate) && gate(),"Workspace busy / missing invalidation gate");
    require(QFileInfo(repo.projectPath()).isFile(),"Saved project required");
    const auto e=repo.load();require(e.revision==revision,"Stale registry revision");
    const auto defaults=e.defaults(id);
    require(defaults.setup && defaults.profile && defaults.status!=CameraDefaultsStatus::Archived && defaults.status!=CameraDefaultsStatus::Ambiguous,"Camera defaults unavailable/ambiguous");
    IndustrialCameraRequest r;r.sourceId="camera-settings:"+id.value;
    for(const auto& c:e.current.cameras)if(c.id==id)r.camera=c;
    for(const auto& s:e.current.setups)if(s.id==*defaults.setup)r.setup=s;
    for(const auto& p:e.current.profiles)if(p.id==*defaults.profile)r.profile=p;
    require(r.camera.physical.has_value(),"Industrial settings backend required; webcam settings unsupported");
    // Exact remembered route wins. A stale hint may fall back only to a single exact route.
    const auto routes=resolveRoutes(*r.camera.physical,discovery.devices);
    auto hint=local.route(*r.camera.physical);bool matches=false;
    for(auto i:routes)if(hint && discovery.devices[i].route==*hint)matches=true;
    if(!matches)hint.reset();
    r.device=CameraService::resolveIndustrial(r.camera,discovery,hint);
    const auto caps=factory.capabilities(r.device);
    require(factory.injected_ && caps.open && caps.configure,"Native settings backend unavailable / unsupported");
    auto session=std::unique_ptr<CameraSettingsSession>(new CameraSettingsSession);
    session->shared_=std::make_shared<CameraSettingsSession::Shared>();
    session->expected_=e;session->repository_=repo;session->gate_=std::move(gate);session->invalidate_=std::move(invalidate);
    // Local class has access to Shared without exposing a backend or SDK to the UI.
    class Transaction final:public CameraWorkerBackend {
        std::unique_ptr<IndustrialCameraBackend> backend_;
        IndustrialCameraRequest request_;
        MeasurementEnvironmentRepository repo_;
        MeasurementEnvironment expected_;
        std::shared_ptr<CameraSettingsSession::Shared> shared_;
        MeasurementEnvironmentRepository::Fault fault_;
        void publish(CameraSettingsSnapshot s){std::lock_guard lock(shared_->mutex);shared_->snapshot=std::move(s);}
        void identity(){require(backend_->actualIdentity()==request_.device.identity,"Actual settings identity mismatch");}
    public:
        Transaction(std::shared_ptr<const IndustrialCameraProvider> provider,IndustrialCameraRequest r,
            MeasurementEnvironmentRepository repo,MeasurementEnvironment e,std::shared_ptr<CameraSettingsSession::Shared> shared,
            MeasurementEnvironmentRepository::Fault fault):backend_(provider->createSettings(r.camera,r.device)),request_(std::move(r)),repo_(std::move(repo)),expected_(std::move(e)),shared_(std::move(shared)),fault_(fault){require(bool(backend_),"Settings backend missing");}
        CameraWorkerResult execute(const CameraTicket& t) override {
            if(t.operation==CameraOperation::Connect){auto result=backend_->execute(t);if(!result.success)return result;
                identity();publish(inspect(*backend_));return {true,{},{}};}
            require(t.operation==CameraOperation::Configure,"Settings session cannot acquire frames");
            identity();checkLive(t);require(repo_.load().encode()==expected_.encode(),"Stale registry before settings write");
            auto before=inspect(*backend_);auto output=before;
            CameraSettingsValues desired;{std::lock_guard lock(shared_->mutex);desired=shared_->desired;}
            numeric(before.exposure,{"us",QString::fromUtf8("µs")});numeric(before.gain,{"dB"});
            QVector<QPair<QString,FeatureValue>> plan;
            const auto add=[&](const CameraFeature& f,const FeatureValue& value,bool dynamicAccess=false){
                require(f.readable(),"Setting readback unsupported");
                if(!featureEquivalent(f,value)){
                    auto preflight=f;
                    if(dynamicAccess && f.access==FeatureAccess::ReadOnly)preflight.access=FeatureAccess::ReadWrite;
                    validateFeatureWrite(preflight,value);plan.append({f.name,value});}
            };
            // Manual-only profile contract; never guess an auto-off capability.
            add(before.exposureAuto,QString("Off"));add(before.gainAuto,QString("Off"));
            if(desired.exposureUs){numeric(before.exposure,{"us",QString::fromUtf8("µs")});
                add(before.exposure,number(before.exposure,*desired.exposureUs),!featureEquivalent(before.exposureAuto,QString("Off")));}
            if(desired.gainDb){numeric(before.gain,{"dB"});add(before.gain,number(before.gain,*desired.gainDb),!featureEquivalent(before.gainAuto,QString("Off")));}
            QVector<QString> touched;
            try {
                for(const auto& w:plan){checkLive(t);const auto live=backend_->setting(w.first);validateFeatureWrite(live,w.second);
                    touched.append(w.first);backend_->writeSetting(w.first,w.second);}
                identity();checkLive(t);output=inspect(*backend_);identity();
                for(const auto& w:plan)require(featureEquivalent(backend_->setting(w.first),w.second),"Settings final readback mismatch");
                require(output.actual.geometry==before.actual.geometry,"Unexpected geometry mutation during basic settings write");
                require(output.actual.settings.pixelFormat==before.actual.settings.pixelFormat && output.actual.settings.trigger==before.actual.settings.trigger,
                    "Unexpected pixel format/trigger mutation during basic settings write");
                require(featureEquivalent(output.exposure,number(output.exposure,desired.exposureUs.value_or(before.actual.settings.exposureTimeUs))) &&
                    featureEquivalent(output.gain,number(output.gain,desired.gainDb.value_or(before.actual.settings.gain))),"Final scalar readback mismatch");
                auto profile=request_.profile;profile.pending=false;profile.requested=output.actual.settings;
                output.requested=desired;
                if(desired.exposureUs)profile.requested.exposureTimeUs=*desired.exposureUs;
                else if(!request_.profile.pending && featureEquivalent(output.exposure,number(output.exposure,request_.profile.requested.exposureTimeUs)))
                    profile.requested.exposureTimeUs=request_.profile.requested.exposureTimeUs;
                if(desired.gainDb)profile.requested.gain=*desired.gainDb;
                else if(!request_.profile.pending && featureEquivalent(output.gain,number(output.gain,request_.profile.requested.gain)))
                    profile.requested.gain=request_.profile.requested.gain;
                // Preserve operator illumination: it is not a device readback node.
                if(!request_.profile.pending)profile.requested.illumination=request_.profile.requested.illumination;
                profile=request_.profile.revised(profile);profile.validate();
                require(featureEquivalent(output.exposure,number(output.exposure,output.actual.settings.exposureTimeUs)) &&
                    featureEquivalent(output.gain,number(output.gain,output.actual.settings.gain)),"Inventory/acquisition readback mismatch");
                auto next=expected_;output.profileChanged=profile!=request_.profile;
                for(auto& p:next.current.profiles)if(p.id==profile.id)p=profile;
                // Pending optical declarations remain pending; do not manufacture lens/plane.
                if(!request_.setup.pending){auto setup=request_.setup;setup.geometry=output.actual.geometry.descriptor();setup=request_.setup.revised(setup);
                    output.geometryChanged=setup.geometry!=request_.setup.geometry;
                    for(auto& s:next.current.setups)if(s.id==setup.id)s=setup;
                    if(setup.revision!=request_.setup.revision)next.history.append(setup);
                }
                checkLive(t);repo_.commit(expected_,next,fault_);output.saved=true;output.deviceChanged=!touched.isEmpty();publish(output);
                return {true,{},{},output.actual};
            }catch(const std::exception& failure){
                const QString error=QString::fromUtf8(failure.what());bool restored=true;
                // Even a failed SDK write can have changed a register.
                if(!touched.isEmpty()){
                    for(const auto& f:{before.exposure,before.gain,before.exposureAuto,before.gainAuto}){
                        try{if(!featureEquivalent(backend_->setting(f.name),f.value))backend_->writeSetting(f.name,f.value);}catch(...){restored=false;}}
                    try{identity();const auto actual=inspect(*backend_);require(actual.actual==before.actual,"Rollback readback mismatch");
                        for(const auto& f:{before.exposure,before.gain,before.exposureAuto,before.gainAuto})require(featureEquivalent(backend_->setting(f.name),f.value),"Rollback inventory mismatch");}catch(...){restored=false;}
                }
                before.unusable=!restored;before.error=restored?error:error+"; rollback unverified; camera settings unusable, reopen/recheck required";
                publish(before);return {false,before.error,{}};
            }
        }
        void shutdown() override {backend_->shutdown();}
    };
    session->worker_=std::make_unique<CameraWorkerSession>(r.device.identity,CameraLeaseUse::Editor,r.sourceId,FrameQueueMode::ProductionFifo,
        [provider=factory.injected_,r,repo,e,shared=session->shared_,fault]{return std::make_unique<Transaction>(provider,r,repo,e,shared,fault);});
    return session;
}
}
