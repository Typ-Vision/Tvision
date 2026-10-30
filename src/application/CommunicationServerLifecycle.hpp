#pragma once
#include "StationRunController.hpp"
#include <QTcpServer>
#include <QTcpSocket>
#include <QPointer>
#include <QElapsedTimer>
#include <optional>

namespace visionflow {
class CommunicationPeerSessionService;
struct CommunicationServerProfile {
    bool enabled=false;
    QString address="127.0.0.1";
    quint16 port=5000;
    QString peerPolicy="single-trusted-local-network";
    int parserDeadlineMs=1000,ackTimeoutMs=2000,retries=3;
    int maxStations=32,maxQueuedResponses=64,writeHighWaterBytes=65536,maxReadBatch=4;
};
// Separate explicit v4 machine file; legacy endpoint profiles are never read/written.
class CommunicationServerProfiles final {
public:
    explicit CommunicationServerProfiles(QString path):path_(std::move(path)){}
    std::optional<CommunicationServerProfile> load() const;
    void save(const CommunicationServerProfile&) const;
    static void validate(const CommunicationServerProfile&);
private:QString path_;
};
enum class CommunicationServerState {NotConfigured,Disabled,Listening,PeerConnected,Fault,Closing,Closed};
// Construct once at process startup. Run/Stop and recipe activation never touch listener.
// Owner must outlive registered station controllers and explicitly shutdown before destroying them.
class CommunicationServerLifecycle final {
public:
    CommunicationServerLifecycle(const CommunicationServerProfiles&,QString checkpointPath,CommunicationPeerSessionService* sessions=nullptr,QString startupError={});
    ~CommunicationServerLifecycle();
    CommunicationServerLifecycle(const CommunicationServerLifecycle&)=delete;
    CommunicationServerLifecycle& operator=(const CommunicationServerLifecycle&)=delete;
    void addStation(StationRunController&);
    CommunicationServerState state() const;
    QString error() const {owner();return error_;}
    quint16 port() const {owner();return server_.serverPort();}
    QUuid listenerGeneration() const {owner();return generation_;}
    QTcpSocket* peer() const {owner();return peer_;}
    // Nonblocking bounded shutdown: caller polls on its normal event loop, controls remain online.
    void beginShutdown(int deadlineMs);
    bool pollShutdown(); // true after checkpoint attempt + socket close; false while draining
    bool recoveryRequired() const {owner();return recovery_;}
private:
    void owner() const;
    void acceptPeer();
    void checkpoint();
    QTcpServer server_;
    QPointer<QTcpSocket> peer_;
    QPointer<QTcpSocket> rejectedPeer_;
    CommunicationPeerSessionService* sessions_=nullptr; // outlives lifecycle
    QMap<QUuid,StationRunController*> stations_;
    QString checkpointPath_,error_;
    QUuid generation_;
    CommunicationServerState state_=CommunicationServerState::NotConfigured;
    QThread* owner_=QThread::currentThread();
    QElapsedTimer shutdownTimer_;
    int deadlineMs_=0,maxStations_=32;
    bool recovery_=false;
};
}
