#pragma once
#include "application/RuntimeService.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QTextStream>
#include <QImage>
#include <QFile>
#include <QDir>

// Reuse the independently checked raster corpus, then exercise actual immutable
// Runtime acquisition, completion, atomic recording and restart readers.
inline visionflow::CycleResult productionAcceptance(const visionflow::Recipe& recipe,const QString& directory,
    const QString& archive,const QString& catalog) {
    using namespace visionflow;
    auto require=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    auto reject=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}require(caught,"Production rejection missing");};
    RuntimeService runtime(archive,catalog,RuntimeIdentity::localEngineer());runtime.prepare(recipe,directory);
    const auto lease=runtime.begin();
    auto editor=ExecutionService{}.run(recipe.definition,directory,recipe.id,lease.snapshot->validation().contentId,{}, {},{},
        [&](const ImageFrame& f){return CalibrationWorkflow::resolve(recipe,f,catalog);});
    reject([&]{runtime.finish(lease.ticket,editor);});
    const auto actual=RuntimeService::execute(lease,catalog,{});
    if(actual.state!=ExecutionState::Done)for(const auto& t:actual.tools)QTextStream(stderr)<<t.toolId<<" "<<executionStateName(t.state)<<" "<<t.message<<"\n";
    require(actual.state==ExecutionState::Done,"Actual reference production DAG failed");
    auto tamper=[&](auto edit){auto changed=actual;edit(changed);reject([&]{runtime.finish(lease.ticket,changed);});require(runtime.busy(),"Rejected completion lost lease");};
    tamper([](auto& r){r.evaluationId="foreign";});
    tamper([](auto& r){r.frames.first().completedAtUtc=r.frames.first().completedAtUtc.addMSecs(1);});
    tamper([](auto& r){r.tools.last().points.append({1,2});});
    tamper([](auto& r){r.tools.last().state=ExecutionState::Error;r.tools.last().outputs.clear();});
    tamper([](auto& r){r.partial=true;});
    for(int i=0;i<actual.tools.size();++i)for(const auto& key:actual.tools[i].outputs.keys()) {
        if(actual.tools[i].outputs[key].valid && actual.tools[i].outputs[key].port.type==PortType::Number)
            tamper([&](auto& r){r.tools[i].outputs[key].data=std::get<double>(r.tools[i].outputs[key].data)+.125;});
        if(actual.tools[i].outputs[key].valid && actual.tools[i].outputs[key].port.type==PortType::Boolean)
            tamper([&](auto& r){r.tools[i].outputs[key].data=!std::get<bool>(r.tools[i].outputs[key].data);});
        if(actual.tools[i].outputs[key].valid && actual.tools[i].outputs[key].port.type==PortType::Point)
            tamper([&](auto& r){r.tools[i].outputs[key].data=std::get<QPointF>(r.tools[i].outputs[key].data)+QPointF(.125,0);});
    }
    auto changedDraft=recipe;auto tools=editorTools(recipe.definition);
    for(auto& t:tools)if(t.parameters.contains("referenceBinding")) {auto b=toolReferenceBinding(t);b.offsetPx+=QPointF(.25,0);t.parameters["referenceBinding"]=b.toJson();break;}
    changedDraft.definition=withEditorTools(recipe.definition,tools);
    require(lease.snapshot->recipe().definition==recipe.definition,"Draft changed active snapshot");
    reject([&]{runtime.activate(changedDraft,directory);});
    {QLockFile lock(archive+"/archive.lock");lock.setStaleLockTime(0);require(lock.tryLock(),"Lock fixture");
        reject([&]{runtime.finish(lease.ticket,actual);});require(runtime.busy(),"Record failure lost lease");}
    runtime.finish(lease.ticket,actual);reject([&]{runtime.activate(changedDraft,directory);});reject([&]{runtime.finish(lease.ticket,actual);});
    const auto records=runtime.records();require(records.size()==1,"Atomic retry duplicated record");
    const auto record=records.first();require(record["schemaVersion"]==4,"New reference terminal schema");
    require(record["configId"]==QString::fromLatin1(lease.snapshot->validation().contentId),"Recorded immutable config");
    const auto refs=record["referenceBindings"].toObject();
    for(const auto& t:editorTools(recipe.definition))if(t.parameters.contains("referenceBinding"))
        require(refs[t.id]==t.parameters["referenceBinding"],"Exact used binding/offset identity lost");
    const auto summary=ProductionResultCodec::summary(record);const auto csv=ProductionResultCodec::csv(records);
    require(summary.contains("Referans") && csv.contains("referenceBinding") && csv.contains("offset"),"Logs/CSV missing reference evidence");
    auto broken=record;auto wrongRefs=refs;auto first=wrongRefs.begin().value().toObject();first["sourcePort"]="foreign";wrongRefs[wrongRefs.begin().key()]=first;broken["referenceBindings"]=wrongRefs;
    reject([&]{ProductionResultCodec::validate(broken);});
    RuntimeService restarted(archive,catalog,RuntimeIdentity::localEngineer());
    require(restarted.records()==records && ProductionResultCodec::csv(restarted.records())==csv,"Restart replay differs");
    require(!restarted.active(),"Restart restored production permission");reject([&]{restarted.begin();});
    auto next=runtime.begin();reject([&]{runtime.finish(next.ticket,actual);});
    auto stop=RuntimeService::execute(next,catalog,std::make_shared<std::atomic_bool>(true));
    require(stop.state==ExecutionState::Skipped,"Stop terminal state");
    for(const auto& t:stop.tools)require(t.outputs.isEmpty() && t.points.isEmpty() && t.decision==PartDecision::NotEvaluated,"Stop retained results");
    runtime.finish(next.ticket,stop);
    next=runtime.begin();auto lateStop=RuntimeService::execute(next,catalog,{});
    RuntimeService::suppressCancelled(next,lateStop);require(lateStop.state==ExecutionState::Skipped,"Late Stop suppression");
    runtime.finish(next.ticket,lateStop);
    QString sourcePath;for(const auto& t:editorTools(recipe.definition))if(t.enabled && t.kind=="image_file")sourcePath=QDir(directory).absoluteFilePath(t.parameters["path"].toString());
    require(!sourcePath.isEmpty(),"Production image fixture");QImage original(sourcePath);require(!original.isNull(),"Read actual source");
    QImage blank(original.size(),QImage::Format_Grayscale8);blank.fill(30);require(blank.save(sourcePath),"Blank source fixture");
    next=runtime.begin();auto missing=RuntimeService::execute(next,catalog,{});
    for(const auto& t:missing.tools)if(refs.contains(t.toolId))require(t.state==ExecutionState::Skipped && t.outputs.isEmpty(),"Not-found reference left stale output");
    runtime.finish(next.ticket,missing);require(original.save(sourcePath),"Restore actual source");
    next=runtime.begin();require(QFile::rename(sourcePath,sourcePath+".offline"),"Hide source fixture");
    auto failed=RuntimeService::execute(next,catalog,{});require(QFile::rename(sourcePath+".offline",sourcePath),"Restore hidden source");
    require(failed.state==ExecutionState::Error,"Missing acquisition must be ERROR");runtime.finish(next.ticket,failed);
    return actual;
}
