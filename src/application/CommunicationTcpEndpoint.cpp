#include "CommunicationTcpEndpoint.hpp"
#include "ProductionResultCodec.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrentRun>
namespace visionflow {
namespace {QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}}
CommunicationTcpEndpoint::CommunicationTcpEndpoint(RuntimeService& r,RuntimeCoordinator& c,TriggerRouter& router,InputSnapshotStore& i,DeliveryStore& d,QString archive,QString catalog,QObject* parent)
    :QObject(parent),runtime_(r),coordinator_(c),router_(router),inputs_(i),ledger_(d),archive_(std::move(archive)),catalog_(std::move(catalog)) {
    clock_.start();connect(&server_,&QTcpServer::newConnection,this,[this]{acceptPeer();});
    timer_.setInterval(50);connect(&timer_,&QTimer::timeout,this,[this]{try{framer_.tick(clock_.elapsed());}catch(...){disconnectPeer();}});
}
CommunicationTcpEndpoint::~CommunicationTcpEndpoint(){server_.close();timer_.stop();disconnectPeer();if(cancel_)cancel_->store(true);if(watcher_){watcher_->disconnect(this);watcher_->waitForFinished();}}
void CommunicationTcpEndpoint::owner() const {require(thread()==QThread::currentThread(),"TCP endpoint owner thread required");}
void CommunicationTcpEndpoint::listen(const CommunicationTcpOptions& o){
    owner();require(!listening() && !watcher_ && !pending_ && outstanding_.isEmpty(),"TCP endpoint has active work");
    for(const auto& id:{o.workcell,o.project,o.endpoint,o.peer})require(!id.isEmpty() && id.size()<=128,"Explicit endpoint/peer identity required");
    require(o.inputs.size()<=64,"Input profile too large");for(const auto& input:o.inputs)require(input.maxAgeMs>0,"Input maxAge required");
    // No auto listen during construction/Load. An occupied v2/v3 profile is an error.
    require(server_.listen(o.address,o.port),qPrintable(server_.errorString()));options_=o;restoreDelivery();timer_.start();
}
QVector<DeliveryRecovery> CommunicationTcpEndpoint::inspectRecovery() const {
    owner();QVector<DeliveryRecovery> found;for(const auto& item:ledger_.recovery()){
        const auto& r=item.request;if(r.workcell==options_.workcell && r.project==options_.project && r.endpoint==options_.endpoint && r.peer==options_.peer)found.append(item);
    }return found;
}
void CommunicationTcpEndpoint::restoreDelivery(){
    recoveryState_.clear();bool pending=false;
    try{for(const auto& item:inspectRecovery()){
        const auto& d=item.request;const auto& s=item.status;
        if(s.state!="ACK_COMMITTED"){pending=true;if(s.state=="RECOVERY_REQUIRED")recoveryState_="RECOVERY_REQUIRED";}
        if(s.payload.isEmpty())continue;
        const auto o=QJsonDocument::fromJson(s.payload).object();
        TriggerRequest r{d.workcell,d.project,d.endpoint,d.peer,d.requestId,o["recipeId"].toString(),o["configId"].toString(),o["inputSnapshotId"].toString(),o["sequence"].toString().toULongLong(),o["inputRevision"].toString().toULongLong()};
        require(TriggerRouter::deliveryRequest(r).fingerprint==d.fingerprint,"RECOVERY_REQUIRED: payload request mismatch");
        // Tombstones remain on disk; only pending deliveries need resident state.
        if(s.state!="ACK_COMMITTED")entries_[d.requestId]={r,s.cycleId};
    }}catch(...){recoveryState_="RECOVERY_REQUIRED";pending=true;}
    coordinator_.setDeliveryPending(pending);
}
void CommunicationTcpEndpoint::resumeDelivery(const TriggerRequest& r){
    owner();require(!watcher_ && !pending_ && !runtime_.busy(),"Recovery requires idle execution");require(r.workcell==options_.workcell && r.project==options_.project && r.endpoint==options_.endpoint && r.peer==options_.peer,"Foreign recovery namespace");
    const auto d=TriggerRouter::deliveryRequest(r);auto s=ledger_.inspect(d);require(s.state!="ABSENT","Unknown request requires reconciliation");
    if(s.payload.isEmpty()){
        const auto t=ledger_.recoveryTerminal(d);require(!t.isEmpty(),"RECOVERY_REQUIRED: no committed terminal/bundle");
        const auto c=t["communication"].toObject();
        require(t["recipeId"]==r.recipeId && t["configId"]==r.configId && c["protocol"]=="vf.socket/3" && c["endpoint"]==r.endpoint && c["peer"]==r.peer && c["requestId"]==r.requestId && c["sequence"]==QString::number(r.sequence) && c["inputSnapshotId"]==r.inputSnapshotId && c["inputRevision"]==QString::number(r.inputRevision),"RECOVERY_REQUIRED: foreign terminal identity");
        const auto sealed=result({r,s.cycleId},t);ledger_.commit(d,t,CommunicationWireCodec::resultPayload(sealed));
    }
    entries_[r.requestId]={r,s.cycleId};restoreDelivery();
    // A durable ACK receipt can finish its commit without inventing a new ACK.
    s=ledger_.inspect(d);if(s.state=="ACK_RECEIVED"){ledger_.commitAcknowledgement(d);restoreDelivery();return;}
    if(s.dispatchable())replay(entries_[r.requestId]);
}
void CommunicationTcpEndpoint::stop(){owner();server_.close();timer_.stop();disconnectPeer();coordinator_.stop();}
QString CommunicationTcpEndpoint::state() const {
    owner();if(!listening())return "STOPPED";if(!fault_.isEmpty())return fault_;if(!recoveryState_.isEmpty())return recoveryState_;
    if(pending_ || coordinator_.recordingPending())return "RECORDING_PENDING";
    if(watcher_ || runtime_.busy())return "BUSY";
    if(!outstanding_.isEmpty() || coordinator_.deliveryPending())return "WAIT_ACK";
    if(!peer_ || peer_->state()!=QAbstractSocket::ConnectedState || peer_->bytesToWrite())return "NOT_READY";
    if(entries_.size()>=128)return "QUEUE_FULL";
    if(coordinator_.state()!=RuntimeState::Running || coordinator_.mode()!=RuntimeTrigger::SoftwareTrigger || runtime_.experimental())return "NOT_READY";
    try{runtime_.requireReady();}catch(...){return "NOT_READY";}return "READY";
}
QJsonObject CommunicationTcpEndpoint::status(const QString& type)const {const auto active=runtime_.active();const auto s=state();return {{"type",type},{"state",s},{"ready",s=="READY"},{"busy",bool(watcher_)||runtime_.busy()},{"recipeId",active?active->recipe().id:QString{}},{"configId",active?QString::fromLatin1(active->validation().contentId):QString{}}};}
void CommunicationTcpEndpoint::disconnectPeer(){if(peer_){auto* p=peer_.data();peer_.clear();p->disconnect(this);p->abort();p->deleteLater();}framer_.reset();}
void CommunicationTcpEndpoint::acceptPeer(){while(server_.hasPendingConnections()){
    auto* p=server_.nextPendingConnection();if(peer_){p->abort();p->deleteLater();continue;}
    peer_=p;session_=uuid();framer_.reset();p->setReadBufferSize(CommunicationWireCodec::maxLineBytes+1);
    connect(p,&QTcpSocket::readyRead,this,[this,p]{if(peer_==p)readPeer();});
    connect(p,&QTcpSocket::disconnected,this,[this,p]{if(peer_==p){peer_.clear();framer_.reset();}p->deleteLater();});
    send(status("HELLO"));
}}
void CommunicationTcpEndpoint::send(QJsonObject o){
    if(!peer_ || peer_->state()!=QAbstractSocket::ConnectedState)return;
    o["v"]=3;o["sessionId"]=session_;const auto bytes=CommunicationWireCodec::encode(o);
    if(peer_->bytesToWrite()+bytes.size()>2*(CommunicationWireCodec::maxLineBytes+1) || peer_->write(bytes)!=bytes.size())disconnectPeer();
}
void CommunicationTcpEndpoint::error(const QString& code,const QString& id){send({{"type","ERROR"},{"requestId",id},{"code",code}});}
void CommunicationTcpEndpoint::readPeer(){
    while(peer_ && peer_->bytesAvailable()){
        QVector<QJsonObject> batch;
        try{const auto bytes=peer_->read(CommunicationWireCodec::maxLineBytes+1);const auto lines=framer_.feed(bytes,clock_.elapsed());for(const auto& line:lines)batch.append(CommunicationWireCodec::decode(line));}
        catch(...){disconnectPeer();return;}
        for(const auto& o:batch){try{receive(o);}catch(const std::exception&){error("SERVICE_ERROR",o["requestId"].toString());}if(!peer_)return;}
    }
}
void CommunicationTcpEndpoint::receive(const QJsonObject& o){
    if(o["sessionId"]!=session_){error("STALE_SESSION",o["requestId"].toString());return;}
    const auto type=o["type"].toString();if(type=="STATUS"){send(status("STATUS"));return;}
    if(type=="INPUT"){
        const auto active=runtime_.active();require(active && o["recipeId"]==active->recipe().id && o["configId"]==QString::fromLatin1(active->validation().contentId),"INPUT config mismatch");
        InputSnapshot s{options_.endpoint,options_.peer,o["snapshotId"].toString(),o["recipeId"].toString(),o["configId"].toString(),o["sourceTime"].toString(),o["clockId"].toString(),o["revision"].toString().toULongLong()};
        for(const auto& field:o["fields"].toArray()){auto f=field.toObject();const auto name=f["name"].toString();require(options_.inputs.contains(name),"INPUT field not in configured profile");auto value=CommunicationWireCodec::decodeField(f);if(auto scalar=std::get_if<ValueEnvelope>(&value))CommunicationValues::validate(*scalar,options_.enums);s.fields[name]=std::move(value);}
        inputs_.put(s);send(status("STATUS"));return;
    }
    if(type=="TRIGGER"){trigger(o);return;}if(type=="ACK"){acknowledge(o);return;}error("BAD_DIRECTION",o["requestId"].toString());
}
void CommunicationTcpEndpoint::trigger(const QJsonObject& o){
    TriggerRequest r{options_.workcell,options_.project,options_.endpoint,options_.peer,o["requestId"].toString(),o["recipeId"].toString(),o["configId"].toString(),o["inputSnapshotId"].toString(),o["sequence"].toString().toULongLong(),o["inputRevision"].toString().toULongLong()};
    const auto d=TriggerRouter::deliveryRequest(r);
    if(entries_.contains(r.requestId)){
        const auto& e=entries_[r.requestId];if(TriggerRouter::deliveryRequest(e.trigger).fingerprint!=d.fingerprint){error("REQUEST_CONFLICT",r.requestId);return;}
        replay(e);return;
    }
    DeliveryStatus durable;try{durable=ledger_.inspect(d);}catch(const std::exception& ex){
        const auto reason=QString::fromUtf8(ex.what());if(reason.startsWith("CONFLICT")){error("REQUEST_CONFLICT",r.requestId);return;}throw;
    }
    if(durable.state!="ABSENT"){
        if(durable.payload.isEmpty()){send({{"type","INVALID"},{"requestId",r.requestId},{"reason","RECOVERY_REQUIRED"}});return;}
        const Entry e{r,durable.cycleId};replay(e);return;
    }
    if(!recoveryState_.isEmpty()){send({{"type","INVALID"},{"requestId",r.requestId},{"reason",recoveryState_}});return;}
    if(!fault_.isEmpty() || runtime_.experimental()){send({{"type","INVALID"},{"requestId",r.requestId},{"reason",runtime_.experimental()?QString("EXPERIMENTAL_OUTPUT_BLOCKED"):fault_}});return;}
    if(!options_.outputGroupId.isEmpty()) {
        const auto active=runtime_.active();bool mapped=false;
        if(active && active->publication()) {
            const auto profile=active->publication()->profiles.value(options_.endpoint);
            const bool transportMatches=profile.enabled && profile.protocol=="tcp" && profile.role=="server" && profile.port==port() && QHostAddress(profile.address)==options_.address;
            for(const auto& group:recipeOutputGroups(active->recipe().definition))if(transportMatches && group.id==options_.outputGroupId && group.profileId==options_.endpoint && group.payloadSchema=="vf.fields/1")mapped=true;
        }
        if(!mapped){send({{"type","INVALID"},{"requestId",r.requestId},{"reason","MAPPING_INVALID"}});return;}
    }
    const auto reserved=uuid();bool journalStarted=false;
    auto admission=router_.admit(r,options_.inputs,[&]{
        // Every durable boundary precedes worker entry/acquisition. A failed
        // begin remains recovery evidence, never an automatic second execution.
        try{journalStarted=true;ledger_.request(d);ledger_.begin(d,reserved);}catch(...){fault_="RECOVERY_REQUIRED";throw;}
    },"vf.socket/3");
    if(!admission.accepted()){
        // Durable admission evidence never authorizes a second execution.
        if(journalStarted)fault_="RECOVERY_REQUIRED";
        if(admission.existingState=="RECOVERY_REQUIRED" || admission.reason.startsWith("RECOVERY_REQUIRED"))fault_="RECOVERY_REQUIRED";
        const auto reason=admission.existingState=="RECOVERY_REQUIRED" ? QString("RECOVERY_REQUIRED"):admission.reason;
        send({{"type","INVALID"},{"requestId",r.requestId},{"reason",reason.startsWith("CONFLICT")?QString("REQUEST_CONFLICT"):!QRegularExpression("^[A-Za-z0-9_.:-]{1,128}$").match(reason).hasMatch()?QString("ADMISSION_FAILED"):reason}});return;
    }
    entries_.insert(r.requestId,{r,reserved});outstanding_=r.requestId;cycle_=*admission.cycle;cycle_.cycleId=reserved;
    coordinator_.setDeliveryPending(true);cancel_=coordinator_.cancellation();
    watcher_=new QFutureWatcher<CycleResult>(this);
    connect(watcher_,&QFutureWatcher<CycleResult>::finished,this,[this]{auto* done=watcher_;watcher_=nullptr;try{pending_=done->result();if(cancel_ && cancel_->load())RuntimeService::suppressCancelled(cycle_,*pending_);}catch(...){pending_.reset();fault_="EXECUTION_FAULT";}done->deleteLater();if(pending_)retryRecording();});
    const auto cycle=cycle_;const auto catalog=catalog_;const auto cancel=cancel_;
    watcher_->setFuture(QtConcurrent::run([cycle,catalog,cancel]{return RuntimeService::execute(cycle,catalog,cancel);}));
    send({{"type","ACCEPTED"},{"requestId",r.requestId},{"sequence",QString::number(r.sequence)},{"cycleId",reserved}});if(started)started();
}
QJsonObject CommunicationTcpEndpoint::result(const Entry& e,const QJsonObject& t)const {
    const auto b=t["resultBundle"].toObject();QJsonArray fields;bool found=options_.outputGroupId.isEmpty();
    for(const auto& group:b["groups"].toArray()){const auto g=group.toObject();if(g["groupId"]!=options_.outputGroupId)continue;require(g["profileId"]==options_.endpoint && g["payloadSchema"]=="vf.fields/1","Output group profile mismatch");found=true;const auto values=g["fields"].toObject();for(auto v=values.begin();v!=values.end();++v)fields.append(CommunicationWireCodec::encodeField(v.key(),decodeAssignmentValue(v->toObject())));}
    require(found,"Output group absent from committed bundle");const auto& r=e.trigger;
    return CommunicationWireCodec::sealResult({{"v",3},{"type","RESULT"},{"sessionId",session_.isEmpty()?QString("recovery"):session_},{"requestId",r.requestId},{"sequence",QString::number(r.sequence)},{"recipeId",r.recipeId},{"configId",r.configId},{"inputSnapshotId",r.inputSnapshotId},{"inputRevision",QString::number(r.inputRevision)},{"resultId",b["bundleId"]},{"cycleId",t["cycleId"]},{"execution",t["state"]},{"decision",b["decision"]},{"resultValid",b["decision"]!="NOT_EVALUATED"},{"fields",fields},{"error",t["error"]}});
}
void CommunicationTcpEndpoint::retryRecording(){owner();if(!pending_)return;
    try{
        if(runtime_.busy())coordinator_.finish(cycle_.ticket,*pending_);
        if(terminal_.isEmpty()){for(const auto& t:runtime_.records())if(t["cycleId"]==cycle_.cycleId){terminal_=t;break;}require(!terminal_.isEmpty(),"Committed terminal missing");ProductionResultCodec::validate(terminal_);}
        const auto& e=entries_[outstanding_];if(sealed_.isEmpty())sealed_=result(e,terminal_);
        ledger_.commit(TriggerRouter::deliveryRequest(e.trigger),terminal_,CommunicationWireCodec::resultPayload(sealed_));
        const auto completedResult=*pending_;pending_.reset();terminal_={};sealed_={};fault_.clear();replay(e);if(completed)completed(completedResult);
    }catch(...){fault_="RECORDING_PENDING";}
}
void CommunicationTcpEndpoint::replay(const Entry& e){const auto d=TriggerRouter::deliveryRequest(e.trigger);const auto s=ledger_.inspect(d);
    if(s.payload.isEmpty()){send({{"type","BUSY"},{"requestId",e.trigger.requestId},{"sequence",QString::number(e.trigger.sequence)},{"cycleId",e.cycleId}});return;}
    if(s.state=="ACK_RECEIVED"){error("ACK_COMMIT_PENDING",e.trigger.requestId);return;}
    if(s.dispatchable())ledger_.attempt(d);
    // Payload comes exclusively from durable bytes, including acknowledged replay.
    auto o=QJsonDocument::fromJson(s.payload).object();o["type"]="RESULT";o["payloadHash"]=s.payloadHash;send(o);
}
void CommunicationTcpEndpoint::acknowledge(const QJsonObject& o){const auto id=o["requestId"].toString();
    std::optional<DeliveryRequest> request;
    if(entries_.contains(id))request=TriggerRouter::deliveryRequest(entries_[id].trigger);
    else for(const auto& item:inspectRecovery())if(item.request.requestId==id && !item.status.payload.isEmpty())request=item.request;
    require(request.has_value(),"Unknown ACK");const auto d=*request;const auto s=ledger_.inspect(d);
    require(!s.payload.isEmpty() && QJsonDocument::fromJson(s.payload).object()["resultId"]==o["resultId"] && s.payloadHash==o["payloadHash"],"ACK exact result/hash mismatch");
    ledger_.acknowledge(d,s.payloadHash);ledger_.commitAcknowledgement(d);
    if(outstanding_==id){outstanding_.clear();fault_.clear();}restoreDelivery();
    send({{"type","ACKED"},{"requestId",id},{"resultId",o["resultId"]},{"payloadHash",s.payloadHash}});
}
}
