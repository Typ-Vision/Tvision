#include "PlcRuntimeSocket.hpp"
#include "EditorSession.hpp"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>

namespace visionflow {
namespace {
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
bool token(const QJsonValue& v){return v.isString() && QRegularExpression("^[A-Za-z0-9_.:-]{1,64}$").match(v.toString()).hasMatch();}
bool fields(const QJsonObject& o,const QStringList& keys){if(o.size()!=keys.size())return false;for(const auto& k:keys)if(!o.contains(k))return false;return true;}
}
PlcRuntimeSocket::PlcRuntimeSocket(QObject* parent):QObject(parent) {
    connect(&server_,&QTcpServer::newConnection,this,[this]{acceptPeer();});
    timer_.setInterval(50);connect(&timer_,&QTimer::timeout,this,[this]{tick();});
}
PlcRuntimeSocket::PlcRuntimeSocket(RuntimeService& runtime,QString archive,QString catalog,
    std::function<QString()> unavailable,QObject* parent):PlcRuntimeSocket(parent) {
    unavailable_=std::move(unavailable);bind(&runtime,nullptr,std::move(archive),std::move(catalog));
}
void PlcRuntimeSocket::bind(RuntimeService* runtime,RuntimeCoordinator* coordinator,QString archive,QString catalog,QString mapping) {
    owner();if(blocking() || working())throw std::runtime_error("Pending PLC cycle/delivery prevents rebinding.");
    runtime_=runtime;coordinator_=coordinator;catalog_=std::move(catalog);
    store_=runtime ? std::make_unique<ProductionStore>(std::move(archive)) : nullptr;
    options_.measurementToolId=std::move(mapping);
}
void PlcRuntimeSocket::setMapping(QString mapping) {
    owner();if(blocking())throw std::runtime_error("Pending delivery prevents mapping changes.");
    options_.measurementToolId=std::move(mapping);
}
QString PlcRuntimeSocket::unavailableReason() const {
    if(!runtime_ || (coordinator_ && !coordinator_->loaded()))return "NO_RECIPE_LOADED";
    if(runtime_->experimental())return "EXPERIMENTAL_OUTPUT_BLOCKED";
    if(coordinator_) {
        if(coordinator_->recordingPending())return "RECORDING_PENDING";
        if(coordinator_->deliveryPending())return "DELIVERY_PENDING";
        const auto state=coordinator_->state();
        if(state==RuntimeState::Starting || state==RuntimeState::Stopping)return "NOT_READY";
        if(state==RuntimeState::Fault)return "RUNTIME_FAULT";
        if(state!=RuntimeState::Running)return "NOT_RUNNING";
        if(coordinator_->mode()!=RuntimeTrigger::SoftwareTrigger)return "TRIGGER_SOURCE_DISABLED";
    }
    if(unavailable_ && !unavailable_().isEmpty())return "NOT_READY";
    if(!runtime_->active())return "NOT_RUNNING";
    try{runtime_->requireReady();}catch(...){return "DEPENDENCY_INVALID";}
    return {};
}
PlcRuntimeSocket::~PlcRuntimeSocket(){server_.disconnect(this);server_.close();timer_.stop();if(peer_){peer_->disconnect(this);peer_->abort();}if(cancel_)cancel_->store(true);if(watcher_){watcher_->disconnect(this);watcher_->waitForFinished();}}
void PlcRuntimeSocket::owner() const {if(QThread::currentThread()!=thread())throw std::runtime_error("PLC controller requires owner thread.");}
void PlcRuntimeSocket::log(const QString& message){diagnostics_.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)+" "+message);if(diagnostics_.size()>128)diagnostics_.removeFirst();if(changed)changed();}
QString PlcRuntimeSocket::state() const {
    owner();if(!listening())return "STOPPED";
    if(pending_)return fault_.isEmpty() ? "BUSY":fault_;
    if(watcher_)return "BUSY";
    if(blocking())return fault_.isEmpty() ? "WAIT_ACK" : fault_;
    if(entries_.size()>=128)return "SESSION_FULL";
    if(!unavailableReason().isEmpty() || runtime_->busy())return "NOT_READY";
    try{runtime_->requireReady();}catch(...){return "NOT_READY";}
    return "READY";
}
QJsonObject PlcRuntimeSocket::status(const QString& type) const {
    const auto active=runtime_ ? runtime_->active() : nullptr;const auto s=state();
    const auto loadedId=coordinator_ ? (coordinator_->loaded() ? coordinator_->loaded()->id : QString{}) : (active ? active->recipe().id : QString{});
    const bool current=active && (!coordinator_ || coordinator_->state()==RuntimeState::Running);
    return {{"type",type},{"state",s},{"ready",s=="READY"},{"busy",working() || recordingPending()},
        {"requestId",outstanding_},{"recipeId",loadedId},
        {"configId",current ? QString::fromLatin1(active->validation().contentId) : QString{}},
        {"measurementToolId",options_.measurementToolId}};
}
void PlcRuntimeSocket::listen(const PlcSocketOptions& options) {
    owner();if(listening() || blocking() || (runtime_ && runtime_->busy()))throw std::runtime_error("PLC session/cycle already active.");
    if(options.ackTimeoutMs<100 || options.ackTimeoutMs>60000 || options.retryCount<0 || options.retryCount>10)throw std::runtime_error("ACK timeout 100..60000 ms; retries 0..10.");
    options_=options;if(!server_.listen(options.address,options.port))throw std::runtime_error(server_.errorString().toUtf8().constData());
    session_=uuid();entries_.clear();fault_.clear();timer_.start();log("LISTEN "+options.address.toString()+":"+QString::number(port())+" session="+session_);
}
void PlcRuntimeSocket::stop(){owner();if(blocking() || working())throw std::runtime_error("Record the cycle and receive ACK before stopping the PLC session.");server_.close();timer_.stop();disconnectPeer();log("STOPPED");}
void PlcRuntimeSocket::cancel(){owner();if(cancel_)cancel_->store(true);log("STOP requested; execution cancellation is cooperative.");}
void PlcRuntimeSocket::disconnectPeer(){if(peer_){auto* socket=peer_.data();peer_.clear();socket->abort();socket->deleteLater();}input_.clear();partialClock_.invalidate();}
void PlcRuntimeSocket::acceptPeer() {
    while(server_.hasPendingConnections()) {
        auto* socket=server_.nextPendingConnection();
        if(peer_ && peer_->state()!=QAbstractSocket::UnconnectedState){socket->abort();socket->deleteLater();continue;}
        disconnectPeer();peer_=socket;socket->setReadBufferSize(8192);
        connect(socket,&QTcpSocket::readyRead,this,[this,socket]{if(peer_==socket)readPeer();});
        connect(socket,&QTcpSocket::disconnected,this,[this,socket]{if(peer_==socket){peer_.clear();input_.clear();partialClock_.invalidate();log("DISCONNECTED; request retained in session.");}socket->deleteLater();});
        connect(socket,&QTcpSocket::errorOccurred,this,[this,socket](QAbstractSocket::SocketError){if(peer_==socket)log("SOCKET "+socket->errorString());});
        send(status("HELLO"));log("CONNECTED "+socket->peerAddress().toString());
        if(blocking() && !entries_[outstanding_].response.isEmpty()){if(fault_=="ACK_TIMEOUT")fault_.clear();send(entries_[outstanding_].response);ackClock_.restart();retries_=0;}
    }
}
void PlcRuntimeSocket::send(QJsonObject message) {
    message["v"]=2;message["sessionId"]=session_;
    if(!peer_ || peer_->state()!=QAbstractSocket::ConnectedState)return;
    const auto bytes=QJsonDocument(message).toJson(QJsonDocument::Compact)+'\n';
    if(bytes.size()>65536 || peer_->bytesToWrite()+bytes.size()>65536){log("SEND_LIMIT; peer disconnected.");disconnectPeer();return;}
    if(peer_->write(bytes)!=bytes.size()){log("WRITE_FAILED");disconnectPeer();return;}
    log("TX "+message["type"].toString()+" request="+message["requestId"].toString());
}
void PlcRuntimeSocket::error(const QString& code,const QString& id){send({{"type","ERROR"},{"code",code},{"requestId",id}});log("ERROR "+code);}
void PlcRuntimeSocket::readPeer() {
    while(peer_ && peer_->bytesAvailable()) {
        input_+=peer_->read(2048);
        for(;;){const auto end=input_.indexOf('\n');if(end<0)break;
            if(end>2048){log("FRAME_TOO_LARGE");disconnectPeer();return;}
            const auto line=input_.left(end);input_.remove(0,end+1);partialClock_.invalidate();QJsonParseError parse;
            const auto doc=QJsonDocument::fromJson(line,&parse);
            if(parse.error!=QJsonParseError::NoError || !doc.isObject()){error("BAD_JSON");disconnectPeer();return;}
            try{receive(doc.object());}catch(const std::exception& e){error("SERVICE_ERROR");log(QString::fromUtf8(e.what()));}
            if(!peer_)return;
        }
        if(input_.size()>2048){log("FRAME_TOO_LARGE");disconnectPeer();return;}
        if(input_.isEmpty())partialClock_.invalidate();else if(!partialClock_.isValid())partialClock_.start();
    }
}
void PlcRuntimeSocket::receive(const QJsonObject& o) {
    if(o["v"]!=2){error("UNSUPPORTED_VERSION");return;}
    if( o["sessionId"]!=session_ || !o["type"].isString()){error("PROTOCOL_OR_SESSION");return;}
    const auto type=o["type"].toString();log("RX "+type+" request="+o["requestId"].toString().left(64));
    if(type=="STATUS" && fields(o,{"v","sessionId","type"})){send(status());return;}
    if(type=="TRIGGER" && fields(o,{"v","sessionId","type","requestId","recipeId","configId"}) && token(o["requestId"]) && (token(o["recipeId"]) || o["recipeId"]==QString{}) && (token(o["configId"]) || o["configId"]==QString{})) {trigger(o);return;}
    if(type=="ACK" && fields(o,{"v","sessionId","type","requestId"}) && token(o["requestId"])) {acknowledge(o["requestId"].toString());return;}
    error("BAD_MESSAGE");
}
QJsonObject PlcRuntimeSocket::event(const QString& action,const QString& id) const {
    return {{"schemaVersion",1},{"action",action},{"sessionId",session_},{"requestId",id},
        {"atUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
}
void PlcRuntimeSocket::trigger(const QJsonObject& o) {
    const auto id=o["requestId"].toString();
    if(entries_.contains(id)) {
        const auto& entry=entries_[id];if(entry.request!=o){error("REQUEST_CONFLICT",id);return;}
        if(!entry.response.isEmpty())send(entry.response);
        else send({{"type","BUSY"},{"requestId",id},{"recordingPending",recordingPending()}});
        return;
    }
    if(watcher_){send({{"type","BUSY"},{"requestId",id}});return;}
    const auto reason=pending_ ? (fault_.isEmpty() ? QString("BUSY"):fault_) : blocking() ? QString("DELIVERY_PENDING") : entries_.size()>=128 ? QString("SESSION_FULL") : unavailableReason();
    if(!reason.isEmpty()){send({{"type","INVALID"},{"reason",reason},{"requestId",id},{"resultValid",false},{"decision","NOT_EVALUATED"},{"valueMm",QJsonValue(QJsonValue::Null)}});return;}
    if(runtime_->busy()){send({{"type","BUSY"},{"requestId",id}});return;}
    const auto active=runtime_->active();
    if(o["recipeId"]!=active->recipe().id || o["configId"]!=QString::fromLatin1(active->validation().contentId)){error("SNAPSHOT_MISMATCH",id);return;}
    if(!options_.measurementToolId.isEmpty()) {
        bool found=false;for(const auto& t:editorTools(active->recipe().definition))if(t.id==options_.measurementToolId && t.kind=="measurement" && t.enabled)found=true;
        if(!found){send({{"type","INVALID"},{"reason","MAPPING_INVALID"},{"requestId",id},{"resultValid",false},{"decision","NOT_EVALUATED"},{"valueMm",QJsonValue(QJsonValue::Null)}});return;}
    }
    // Persist request intent before acquisition; a failed begin may leave an intent without a result.
    auto intent=event("PLC_REQUEST",id);intent["request"]=o;intent["measurementToolId"]=options_.measurementToolId;
    const auto intentId=uuid();intent["eventId"]=intentId;store_->append("events",intentId,intent);
    const QJsonObject communication{{"protocol","vf.socket/2"},{"sessionId",session_},{"requestId",id},{"measurementToolId",options_.measurementToolId}};
    cycle_=coordinator_ ? coordinator_->begin(RuntimeTrigger::SoftwareTrigger,communication) : runtime_->begin(communication);entries_.insert(id,Entry{o,{},{},{},{}});outstanding_=id;fault_.clear();
    if(coordinator_)coordinator_->setDeliveryPending(true);
    cancel_=coordinator_ ? coordinator_->cancellation() : std::make_shared<std::atomic_bool>(false);const auto cycle=cycle_;const auto catalog=catalog_;const auto cancel=cancel_;
    watcher_=new QFutureWatcher<CycleResult>(this);
    connect(watcher_,&QFutureWatcher<CycleResult>::finished,this,[this]{
        auto* done=watcher_;watcher_=nullptr;
        pending_=done->result();done->deleteLater();
        try {if(cancel_->load())RuntimeService::suppressCancelled(cycle_,*pending_);retryRecording();}
        catch(const ProductionException& e){fault_="INVALID_COMPLETION";log(fault_+" "+QString::fromUtf8(e.what()));}
        catch(const std::exception& e){fault_="COMPLETION_ERROR";log(fault_+" "+QString::fromUtf8(e.what()));}
    });
    if(started)started();
    watcher_->setFuture(QtConcurrent::run([cycle,catalog,cancel]{
        try{return RuntimeService::execute(cycle,catalog,cancel);}
        catch(const std::exception& e){CycleResult r;r.cycleId=uuid();r.recipeId=cycle.snapshot->recipe().id;r.configId=cycle.snapshot->validation().contentId;r.state=ExecutionState::Error;r.error=QString::fromUtf8(e.what());return r;}
    }));
    send({{"type","BUSY"},{"requestId",id}});log("ACCEPTED ticket="+QString::number(cycle_.ticket));
}
void PlcRuntimeSocket::retryRecording() {
    owner();if(!pending_ || fault_=="INVALID_COMPLETION" || fault_=="COMPLETION_ERROR")return;
    try {
        if(runtime_->busy()){if(coordinator_)coordinator_->finish(cycle_.ticket,*pending_);else runtime_->finish(cycle_.ticket,*pending_); }
        if(resultEvent_.isEmpty()) {
            const auto& r=*pending_;QJsonObject response{{"type","DONE"},{"requestId",outstanding_},
                {"cycleId",r.cycleId},{"recipeId",r.recipeId},{"configId",QString::fromLatin1(r.configId)},
                {"executionState",executionStateName(r.state)},{"error",r.error},
                {"measurementToolId",options_.measurementToolId},{"resultValid",false},{"decision","NOT_EVALUATED"},
                {"valueMm",QJsonValue(QJsonValue::Null)}};
            if(r.state==ExecutionState::Done)for(const auto& t:r.tools)if(t.toolId==options_.measurementToolId && t.state==ExecutionState::Done && t.decision!=PartDecision::NotEvaluated) {
                const auto v=t.outputs.value("value");if(v.valid && v.port.unit=="mm" && std::holds_alternative<double>(v.data) && std::isfinite(std::get<double>(v.data))) {
                    response["resultValid"]=true;response["valueMm"]=std::get<double>(v.data);response["decision"]=t.decision==PartDecision::Pass ? "PASS" : "FAIL";
                }
            }
            resultEventId_=uuid();resultEvent_=event("PLC_RESULT",outstanding_);resultEvent_["eventId"]=resultEventId_;
            resultEvent_["ticket"]=QString::number(cycle_.ticket);resultEvent_["response"]=response;
        }
        try {store_->append("events",resultEventId_,resultEvent_);}
        catch(const std::exception& e){throw ProductionException(ProductionError::PersistenceFailure,QString::fromUtf8(e.what()));}
        auto result=std::move(*pending_);pending_.reset();entries_[outstanding_].response=resultEvent_["response"].toObject();
        resultEvent_={};resultEventId_.clear();cycle_={};fault_.clear();retries_=0;ackClock_.restart();
        send(entries_[outstanding_].response);if(completed)completed(result);log("RESULT_RECORDED; WAIT_ACK");
    }catch(const ProductionException& e){fault_=e.code()==ProductionError::PersistenceFailure ? "RECORDING_PENDING":"INVALID_COMPLETION";log(fault_+" "+QString::fromUtf8(e.what()));}
    catch(const std::exception& e){fault_="COMPLETION_ERROR";log("COMPLETION_ERROR "+QString::fromUtf8(e.what()));}
}
void PlcRuntimeSocket::acknowledge(const QString& id) {
    auto it=entries_.find(id);if(it==entries_.end()){error("UNKNOWN_REQUEST",id);return;}
    if(it->response.isEmpty()){error("RESULT_NOT_READY",id);return;}
    if(!it->acknowledged) {
        if(it->ackEvent.isEmpty()){it->ackEventId=uuid();it->ackEvent=event("PLC_ACK",id);it->ackEvent["eventId"]=it->ackEventId;it->ackEvent["cycleId"]=it->response["cycleId"];}
        store_->append("events",it->ackEventId,it->ackEvent);it->acknowledged=true;
        if(outstanding_==id){outstanding_.clear();if(coordinator_)coordinator_->setDeliveryPending(false);ackClock_.invalidate();fault_.clear();}
    }
    send({{"type","ACKED"},{"requestId",id}});log("ACKED "+id);
}
void PlcRuntimeSocket::tick() {
    if(partialClock_.isValid() && partialClock_.elapsed()>=2000){log("FRAME_TIMEOUT");disconnectPeer();}
    if(!outstanding_.isEmpty() && ackClock_.isValid() && ackClock_.elapsed()>=options_.ackTimeoutMs && fault_!="ACK_TIMEOUT") {
        if(retries_<options_.retryCount){++retries_;send(entries_[outstanding_].response);ackClock_.restart();log("RESULT_RETRY "+QString::number(retries_));}
        else {fault_="ACK_TIMEOUT";log("ACK_TIMEOUT; no new acquisition; late ACK/reconnect accepted.");}
    }
}
}
