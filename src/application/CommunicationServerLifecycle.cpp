#include "CommunicationServerLifecycle.hpp"
#include "CommunicationListenerOwnership.hpp"
#include "CommunicationPeerSessionService.hpp"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QCoreApplication>

namespace visionflow {
namespace {
QPointer<QTcpServer> v4Selection;
QVector<QPointer<QTcpServer>> legacyListeners;
void compositionOwner(){if(!QCoreApplication::instance() || QThread::currentThread()!=QCoreApplication::instance()->thread())throw std::runtime_error("Process listener composition requires application thread");}
void atomicWrite(const QString& path,const QJsonObject& object) {
    if(!QDir().mkpath(QFileInfo(path).absolutePath()))throw std::runtime_error("Cannot create v4 configuration directory");
    QSaveFile file(path);const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())throw std::runtime_error("V4 atomic write failed");
}
}
void CommunicationListenerOwnership::selectV4(QTcpServer& server) {
    compositionOwner();
    if(v4Selection)throw std::runtime_error("V4 server already composed in this process");
    for(const auto& legacy:legacyListeners)if(legacy && legacy->isListening())throw std::runtime_error("Stop legacy listeners before selecting v4; no automatic migration");
    v4Selection=&server;
}
void CommunicationListenerOwnership::releaseV4(QTcpServer& server){if(v4Selection==&server)v4Selection.clear();}
void CommunicationListenerOwnership::requireLegacyAllowed() {
    compositionOwner();
    if(v4Selection)throw std::runtime_error("V4 process profile selected; legacy listener disabled");
}
void CommunicationListenerOwnership::observeLegacy(QTcpServer& server) {
    compositionOwner();
    legacyListeners.removeIf([](const auto& p){return p.isNull();});
    if(!legacyListeners.contains(&server))legacyListeners.append(&server);
}
void CommunicationListenerOwnership::requireMigrationAllowed() {
    compositionOwner();
    if(v4Selection && v4Selection->isListening())throw std::runtime_error("Stop v4 listener before migration/rollback");
    for(const auto& legacy:legacyListeners)if(legacy && legacy->isListening())throw std::runtime_error("Stop legacy listeners before migration/rollback");
}
void CommunicationServerProfiles::validate(const CommunicationServerProfile& p) {
    if(QHostAddress(p.address).isNull() || !p.port || p.peerPolicy!="single-trusted-local-network"
       || p.parserDeadlineMs<10 || p.parserDeadlineMs>60000 || p.ackTimeoutMs<10 || p.ackTimeoutMs>60000
       || p.retries<0 || p.retries>3 || p.maxStations<1 || p.maxStations>32
       || p.maxQueuedResponses<1 || p.maxQueuedResponses>64 || p.writeHighWaterBytes<1024 || p.writeHighWaterBytes>65536
       || p.maxReadBatch<1 || p.maxReadBatch>4)throw std::runtime_error("Invalid v4 server profile");
}
std::optional<CommunicationServerProfile> CommunicationServerProfiles::load() const {
    QFile file(path_);if(!file.exists())return {};
    if(!file.open(QIODevice::ReadOnly) || file.size()>16384)throw std::runtime_error("Cannot read bounded v4 server profile");
    QJsonParseError parse;const auto doc=QJsonDocument::fromJson(file.readAll(),&parse);const auto o=doc.object();
    if(parse.error!=QJsonParseError::NoError || !doc.isObject() || o["schema"]!="visionflow.communication-server.v4/1" || !o["enabled"].isBool())throw std::runtime_error("Invalid v4 server profile schema");
    auto number=[&](const char* key){const auto v=o[key];const auto n=v.toInt(-1);if(!v.isDouble() || v.toDouble()!=n)throw std::runtime_error("Invalid v4 integer");return n;};
    const auto port=number("port");if(port<1 || port>65535)throw std::runtime_error("Invalid v4 port");
    CommunicationServerProfile p;p.enabled=o["enabled"].toBool();p.address=o["address"].toString();p.port=quint16(port);p.peerPolicy=o["peerPolicy"].toString();
    p.parserDeadlineMs=number("parserDeadlineMs");p.ackTimeoutMs=number("ackTimeoutMs");p.retries=number("retries");p.maxStations=number("maxStations");p.maxQueuedResponses=number("maxQueuedResponses");p.writeHighWaterBytes=number("writeHighWaterBytes");p.maxReadBatch=number("maxReadBatch");validate(p);return p;
}
void CommunicationServerProfiles::save(const CommunicationServerProfile& p) const {
    validate(p);atomicWrite(path_,{{"schema","visionflow.communication-server.v4/1"},{"enabled",p.enabled},{"address",p.address},{"port",p.port},{"peerPolicy",p.peerPolicy},
        {"parserDeadlineMs",p.parserDeadlineMs},{"ackTimeoutMs",p.ackTimeoutMs},{"retries",p.retries},{"maxStations",p.maxStations},{"maxQueuedResponses",p.maxQueuedResponses},{"writeHighWaterBytes",p.writeHighWaterBytes},{"maxReadBatch",p.maxReadBatch}});
}
CommunicationServerLifecycle::CommunicationServerLifecycle(const CommunicationServerProfiles& profiles,QString checkpoint,CommunicationPeerSessionService* sessions,QString startupError):sessions_(sessions),checkpointPath_(std::move(checkpoint)) {
    QObject::connect(&server_,&QTcpServer::newConnection,&server_,[this]{acceptPeer();});server_.setMaxPendingConnections(1);
    if(!startupError.isEmpty()){state_=CommunicationServerState::Fault;error_=std::move(startupError);return;}
    try {const auto p=profiles.load();if(!p)return;maxStations_=p->maxStations;if(!p->enabled){state_=CommunicationServerState::Disabled;return;}
        CommunicationListenerOwnership::selectV4(server_);
        if(!server_.listen(QHostAddress(p->address),p->port))throw std::runtime_error(server_.errorString().toStdString());
        generation_=QUuid::createUuid();state_=CommunicationServerState::Listening;
    }catch(const std::exception& e){error_=QString::fromUtf8(e.what());state_=CommunicationServerState::Fault;}
}
CommunicationServerLifecycle::~CommunicationServerLifecycle(){if(peer_)peer_->abort();if(rejectedPeer_)rejectedPeer_->abort();server_.close();CommunicationListenerOwnership::releaseV4(server_);}
void CommunicationServerLifecycle::owner() const {if(owner_!=QThread::currentThread())throw std::runtime_error("Server owner thread required");}
CommunicationServerState CommunicationServerLifecycle::state() const {owner();if(state_==CommunicationServerState::Listening && peer_ && peer_->state()==QAbstractSocket::ConnectedState)return CommunicationServerState::PeerConnected;return state_;}
void CommunicationServerLifecycle::acceptPeer() {
    while(server_.hasPendingConnections()){auto* socket=server_.nextPendingConnection();if(peer_ || state_==CommunicationServerState::Closed){
            if(sessions_ && peer_ && !rejectedPeer_ && state_!=CommunicationServerState::Closed){rejectedPeer_=socket;QObject::connect(socket,&QTcpSocket::disconnected,&server_,[this,socket]{if(rejectedPeer_==socket)rejectedPeer_.clear();socket->deleteLater();});attachCommunicationPeer(*socket,*sessions_);}
            else{socket->abort();socket->deleteLater();}continue;}
        peer_=socket;socket->setReadBufferSize(65536);
        QObject::connect(socket,&QTcpSocket::disconnected,&server_,[this,socket]{if(peer_==socket)peer_.clear();socket->deleteLater();});
        if(sessions_)attachCommunicationPeer(*socket,*sessions_);
    }
}
void CommunicationServerLifecycle::addStation(StationRunController& station) {
    owner();if(state_==CommunicationServerState::Closing || state_==CommunicationServerState::Closed || stations_.contains(station.station()) || stations_.size()>=maxStations_ || !station.drained())throw std::runtime_error("Invalid station lifecycle registration");
    stations_.insert(station.station(),&station);
}
void CommunicationServerLifecycle::beginShutdown(int deadlineMs) {
    owner();if(state_==CommunicationServerState::Closing || state_==CommunicationServerState::Closed)return;
    if(deadlineMs<0 || deadlineMs>60000)throw std::runtime_error("Invalid shutdown deadline");
    state_=CommunicationServerState::Closing;deadlineMs_=deadlineMs;shutdownTimer_.start();
    for(auto* station:stations_)station->stop();
}
void CommunicationServerLifecycle::checkpoint() {
    QJsonArray states;for(auto* station:stations_){const auto s=station->state();states.append(QJsonObject{{"station",station->station().toString()},{"activation",station->activation().toString()},{"mode",int(s.mode)},{"deliveryPending",s.deliveryPending},{"drained",station->drained()}});}
    atomicWrite(checkpointPath_,{{"schema","visionflow.communication-shutdown.v4/1"},{"listenerGeneration",generation_.toString()},{"recoveryRequired",recovery_},{"stations",states}});
}
bool CommunicationServerLifecycle::pollShutdown() {
    owner();if(state_==CommunicationServerState::Closed)return true;
    if(state_!=CommunicationServerState::Closing)throw std::runtime_error("Shutdown not begun");
    bool drained=true;for(auto* station:stations_){station->poll();drained=station->drained() && drained;}
    if(!drained && shutdownTimer_.elapsed()<deadlineMs_)return false;
    recovery_=!drained;
    try{checkpoint();}catch(const std::exception& e){recovery_=true;error_=QString::fromUtf8(e.what());}
    if(peer_)peer_->abort();if(rejectedPeer_)rejectedPeer_->abort();server_.close();CommunicationListenerOwnership::releaseV4(server_);state_=CommunicationServerState::Closed;return true;
}
}
