#include "EditorCameraSessions.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <map>

namespace visionflow {
namespace {
QString physical(const IndustrialCameraRequest& r){
    if(!r.camera.physical)throw std::runtime_error("Industrial camera identity missing");
    const auto& p=*r.camera.physical;
    return QString::fromLatin1(QJsonDocument(QJsonArray{p.vendor,p.model,p.serial}).toJson(QJsonDocument::Compact).toHex());
}
template<class T>bool ready(const std::future<T>& f){return f.valid() && f.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;}
QByteArray signature(const QVector<EditorCameraTarget>& targets,const IndustrialExecutionInputs& in){
    QJsonArray rows;
    for(const auto& t:targets){auto c=t.request.camera.toJson(),s=t.request.setup.toJson(),p=t.request.profile.toJson();c.remove("name");s.remove("name");p.remove("name");
        rows.append(QJsonObject{{"source",t.request.sourceId},{"camera",c},{"setup",s},{"profile",p},{"experimental",t.experimental},
            {"route",in.routes.contains(t.request.camera.id.value)?in.routes[t.request.camera.id.value].toJson():QJsonObject{}}});}
    return QJsonDocument(rows).toJson(QJsonDocument::Compact);
}
}
struct EditorCameraSessions::Impl {
    struct Group {QString context,error;QByteArray signature;QVector<EditorCameraTarget> targets;CameraProviderFactory factory;
        std::future<AcquisitionContextStatus> preparing;};
    std::shared_ptr<CameraAcquisitionSessions> service=std::make_shared<CameraAcquisitionSessions>();
    IndustrialExecutionInputs inputs;
    QString recipe;
    bool enabled=false;
    QVector<EditorCameraTarget> desired;
    std::map<QString,Group> groups;
    std::vector<std::future<AcquisitionContextClose>> closing;
    QString retry;
    void stop(){enabled=false;desired.clear();retry.clear();for(auto& [key,g]:groups)closing.push_back(service->close(g.context));groups.clear();}
    void apply(){
        if(!enabled || !closing.empty())return;
        std::map<QString,QVector<EditorCameraTarget>> wanted;
        for(const auto& t:desired)wanted[physical(t.request)].append(t);
        for(auto it=groups.begin();it!=groups.end();)if(!wanted.contains(it->first)){closing.push_back(service->close(it->second.context));it=groups.erase(it);}else ++it;
        if(!closing.empty())return;
        for(const auto& [key,targets]:wanted){auto& g=groups[key];const auto sig=signature(targets,inputs);
            bool force=false;for(const auto& t:targets)if(retry=="*" || retry==t.request.sourceId)force=true;
            if(g.context.isEmpty())g.context="editor/"+QUuid::createUuid().toString(QUuid::WithoutBraces);
            force=force && !g.preparing.valid() && !service->status(g.context).allReady();
            if(sig==g.signature && g.factory.sameComposition(inputs.factory) && !force)continue;
            g.signature=sig;g.targets=targets;g.factory=inputs.factory;g.error.clear();
            QVector<IndustrialCameraRequest> requests;for(const auto& t:targets)requests.append(t.request);
            try{g.preparing=service->prepare(g.context,inputs,requests,targets.first().experimental);}
            catch(const std::exception& e){g.error=QString::fromUtf8(e.what());}
        }
        retry.clear();
    }
};
EditorCameraSessions::EditorCameraSessions():impl_(std::make_unique<Impl>()){}
EditorCameraSessions::~EditorCameraSessions(){impl_->stop();}
void EditorCameraSessions::reconcile(const QString& recipe,const IndustrialExecutionInputs& in,QVector<EditorCameraTarget> targets,const QString& retry){
    if(impl_->enabled && impl_->recipe!=recipe)impl_->stop();
    impl_->enabled=true;impl_->recipe=recipe;impl_->inputs=in;impl_->inputs.sessions.reset();
    impl_->desired=std::move(targets);impl_->retry=retry;impl_->apply();
}
void EditorCameraSessions::stop(){impl_->stop();}
bool EditorCameraSessions::poll(){
    bool changed=false;
    for(auto it=impl_->closing.begin();it!=impl_->closing.end();)if(ready(*it)){try{it->get();}catch(...){}it=impl_->closing.erase(it);changed=true;}else ++it;
    for(auto& [key,g]:impl_->groups)if(ready(g.preparing)){try{g.preparing.get();}catch(const std::exception& e){g.error=QString::fromUtf8(e.what());}changed=true;}
    if(changed && impl_->closing.empty())impl_->apply();return changed;
}
bool EditorCameraSessions::active() const{return impl_->enabled;}
bool EditorCameraSessions::hasTargets() const{return !impl_->desired.isEmpty();}
bool EditorCameraSessions::draining() const{return !impl_->closing.empty();}
bool EditorCameraSessions::pending(const QString& source) const{
    if(draining())return true;
    for(const auto& [key,g]:impl_->groups)if(g.preparing.valid())for(const auto& t:g.targets)if(source.isEmpty() || t.request.sourceId==source)return true;
    return false;
}
std::optional<AcquisitionSourceStatus> EditorCameraSessions::source(const QString& id) const{
    for(const auto& [key,g]:impl_->groups)for(const auto& t:g.targets)if(t.request.sourceId==id){
        if(!g.error.isEmpty()){AcquisitionSourceStatus failed;failed.request=t.request;failed.state=AcquisitionSessionState::Fault;failed.error=g.error;return failed;}
        auto status=impl_->service->status(g.context);if(status.sources.contains(id))return status.sources[id];
        AcquisitionSourceStatus missing;missing.request=t.request;missing.state=AcquisitionSessionState::Fault;missing.error=g.error;return missing;}
    return {};
}
IndustrialExecutionInputs EditorCameraSessions::executionInputs() const{
    auto in=impl_->inputs;in.sessions=impl_->service;in.sessionContext="editor/not-prepared";in.sessionGeneration=0;
    in.sourceSessionContexts.clear();in.sourceSessionGenerations.clear();
    for(const auto& [key,g]:impl_->groups){const auto status=impl_->service->status(g.context);for(const auto& t:g.targets){in.sourceSessionContexts[t.request.sourceId]=g.context;in.sourceSessionGenerations[t.request.sourceId]=status.generation;}}
    return in;
}
}
