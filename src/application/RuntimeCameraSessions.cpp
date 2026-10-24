#include "RuntimeCameraSessions.hpp"
#include "ScopedActiveCalibration.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>

namespace visionflow {
namespace {
void require(bool b,const char* why){if(!b)throw ProductionException(ProductionError::DependencyChanged,why);}
template<class T>bool available(const std::future<T>& f){return f.valid() && f.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;}
QByteArray identity(const Recipe& recipe,const MultiSourceDependencies& set){
    QJsonObject sources,models,requests;
    for(auto it=set.sources.cbegin();it!=set.sources.cend();++it){const auto& d=it.value();QJsonObject assets;
        for(auto a=d.assetHashes.cbegin();a!=d.assetHashes.cend();++a)assets[a.key()]=a.value();
        sources[it.key()]=QJsonObject{{"engine",d.engineVersion},{"assets",assets},{"calibration",d.calibration?d.calibration->toJson():QJsonObject{}},
            {"system",d.systemCalibration},{"measurement",d.measurementSource},{"industrial",d.industrialSource}};}
    for(auto it=set.experimentalCalibrations.cbegin();it!=set.experimentalCalibrations.cend();++it){require(bool(it.value()),"Null experimental model");const auto& s=it.value()->snapshot;const auto& c=s.calibration;
        models[it.key()]=QJsonObject{{"target",s.target.toJson()},{"device",s.device.toJson()},{"record",c.record.encode()},
            {"selection",QJsonObject{{"id",c.selection.id},{"contentId",c.selection.contentId},{"version",c.selection.version},{"revision",c.selection.revision}}},{"scope",c.scope},{"acquisition",c.acquisition}};}
    if(set.industrial)for(const auto& t:editorTools(recipe.definition))if(t.enabled && t.kind=="industrial_camera"){
        const auto r=industrialExecutionRequest(*set.industrial,t);
        requests[t.id]=QJsonObject{{"camera",r.camera.toJson()},{"setup",r.setup.toJson()},{"profile",r.profile.toJson()},
            {"route",set.industrial->routes.contains(r.camera.id.value)?set.industrial->routes[r.camera.id.value].toJson():QJsonObject{}}};}
    return QJsonDocument(QJsonObject{{"sources",sources},{"models",models},{"requests",requests}}).toJson(QJsonDocument::Compact);
}
}
struct RuntimeCameraSessions::Impl {
    std::shared_ptr<CameraAcquisitionSessions> service=std::make_shared<CameraAcquisitionSessions>();
    Recipe recipe;
    MultiSourceDependencies frozen;
    QByteArray identity;
    QString context,error;
    bool experimental=false,prepared=false,closeFailed=false;
    std::future<AcquisitionContextStatus> preparing;
    std::future<AcquisitionContextClose> closing;
    void stop(){prepared=false;if(!context.isEmpty() && !closing.valid())closing=service->close(context);}
};
RuntimeCameraSessions::RuntimeCameraSessions():impl_(std::make_unique<Impl>()){}
RuntimeCameraSessions::~RuntimeCameraSessions(){impl_->stop();}
void RuntimeCameraSessions::start(const Recipe& recipe,MultiSourceDependencies frozen,bool experimental,std::shared_ptr<const AcquisitionOecsBackend> backend){
    require(!pending() && !impl_->prepared,"Previous Runtime cameras must drain before Run");require(bool(frozen.industrial),"Industrial execution composition required");
    impl_->recipe=recipe;impl_->frozen=std::move(frozen);impl_->identity=identity(recipe,impl_->frozen);impl_->experimental=experimental;impl_->error.clear();impl_->closeFailed=false;
    impl_->context="runtime/"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto inputs=*impl_->frozen.industrial;inputs.sessions.reset();inputs.admittedDevices.clear();inputs.leaseUse=experimental?CameraLeaseUse::Editor:CameraLeaseUse::Runtime;
    QVector<IndustrialCameraRequest> requests;for(const auto& t:editorTools(recipe.definition))if(t.enabled && t.kind=="industrial_camera")requests.append(industrialExecutionRequest(inputs,t));
    try{impl_->preparing=impl_->service->prepare(impl_->context,inputs,requests,experimental,5000,std::move(backend));}
    catch(...){impl_->stop();throw;}
}
bool RuntimeCameraSessions::poll(){
    bool changed=false;
    if(available(impl_->preparing)){
        try{const auto result=impl_->preparing.get();if(!result.allReady()){
            QStringList failures;for(const auto& s:result.sources)if(s.state!=AcquisitionSessionState::Ready)failures<<s.request.sourceId+": "+s.error;
            throw std::runtime_error(failures.join("\n").toStdString());}
            if(!impl_->closing.valid())impl_->prepared=true;
        }catch(const std::exception& e){impl_->error=QString::fromUtf8(e.what());impl_->stop();}changed=true;
    }
    if(available(impl_->closing)){const auto result=impl_->closing.get();if(!result.clean){impl_->closeFailed=true;impl_->error=result.error.isEmpty()?"Camera close failed; lease quarantined":result.error;}impl_->context.clear();changed=true;}
    return changed;
}
void RuntimeCameraSessions::stop(){impl_->stop();}
bool RuntimeCameraSessions::pending() const{return impl_->preparing.valid() || impl_->closing.valid();}
bool RuntimeCameraSessions::draining() const{return impl_->closing.valid();}
bool RuntimeCameraSessions::ready() const{return impl_->prepared && !pending() && impl_->service->status(impl_->context).allReady();}
QString RuntimeCameraSessions::error() const{return impl_->error;}
bool RuntimeCameraSessions::closeFailed() const{return impl_->closeFailed;}
MultiSourceDependencies RuntimeCameraSessions::compose(MultiSourceDependencies current) const{
    require(ready() && current.industrial,"Runtime camera context not Ready");
    require(current.industrial->factory.sameComposition(impl_->frozen.industrial->factory) && identity(impl_->recipe,current)==impl_->identity,"Runtime source/profile/model changed during camera preparation");
    auto in=*current.industrial;const auto status=impl_->service->status(impl_->context);
    in.sessions=impl_->service;in.sessionContext=impl_->context;in.sessionGeneration=status.generation;in.sourceSessionContexts.clear();in.sourceSessionGenerations.clear();
    in.leaseUse=impl_->experimental?CameraLeaseUse::Editor:CameraLeaseUse::Runtime;in.admittedDevices.clear();
    if(!impl_->experimental)for(auto it=status.sources.cbegin();it!=status.sources.cend();++it)in.admittedDevices[it.key()]=it->request.device;
    current.industrial=std::make_shared<const IndustrialExecutionInputs>(std::move(in));return current;
}
}
