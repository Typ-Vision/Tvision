#pragma once
#include "TriggerRouter.hpp"
#include "CommunicationWireCodec.hpp"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QFutureWatcher>
namespace visionflow {
struct CommunicationTcpOptions {
    QHostAddress address=QHostAddress::LocalHost;
    quint16 port=0;
    QString workcell,project,endpoint,peer,outputGroupId;
    QMap<QString,InputRequirement> inputs;
    EnumDefinitions enums;
    bool required=true;
    int ackTimeoutMs=2000,retryCount=3;
};
// Explicit development endpoint. All methods and Qt Network objects stay on the
// Runtime owner thread; the worker captures only an immutable production lease.
class CommunicationTcpEndpoint : public QObject {
public:
    CommunicationTcpEndpoint(RuntimeService&,RuntimeCoordinator&,TriggerRouter&,InputSnapshotStore&,DeliveryStore&,QString archive,QString catalog,QObject* parent=nullptr);
    ~CommunicationTcpEndpoint() override;
    void listen(const CommunicationTcpOptions&);
    void stop();
    void retryRecording();
    // Explicit owner-thread reconciliation. Never prepares/acquires/runs. Exact
    // stable request identity and committed terminal evidence are required.
    QVector<DeliveryRecovery> inspectRecovery() const;
    void resumeDelivery(const TriggerRequest&);
    QJsonObject diagnosticStatus() const;
    QVector<QJsonObject> diagnosticEvents() const;
    void retryDelivery(const DeliveryRequest&);
    quint16 port() const {return server_.serverPort();}
    bool listening() const {return server_.isListening();}
    QString state() const;
    bool recordingPending() const {return pending_.has_value();}
    std::function<void()> started;
    std::function<void(const CycleResult&)> completed;
private:
    RuntimeService& runtime_;RuntimeCoordinator& coordinator_;TriggerRouter& router_;InputSnapshotStore& inputs_;DeliveryStore& ledger_;
    QString archive_,catalog_,session_,fault_;CommunicationTcpOptions options_;
    QTcpServer server_;QPointer<QTcpSocket> peer_;QTimer timer_;QElapsedTimer clock_;CommunicationLineFramer framer_;
    QFutureWatcher<CycleResult>* watcher_=nullptr;std::optional<CycleResult> pending_;ProductionCycle cycle_;
    std::shared_ptr<std::atomic_bool> cancel_;
    struct Entry {TriggerRequest trigger;QString cycleId;};QMap<QString,Entry> entries_;
    QString recoveryState_;
    QString retryRequest_;qint64 retryDeadline_=0;int retries_=0;bool retryExhausted_=false;
    void tickDelivery();
    void restoreDelivery();
    QVector<QJsonObject> diagnosticEvents_;
    void recordDiagnostic(const QString&,const QJsonObject&);
    QString outstanding_;QJsonObject terminal_,sealed_;
    void owner() const;
    void acceptPeer();void disconnectPeer();void readPeer();void receive(const QJsonObject&);
    void send(QJsonObject);void error(const QString&,const QString& id={});
    QJsonObject status(const QString&) const;
    void trigger(const QJsonObject&);void acknowledge(const QJsonObject&);void replay(const Entry&);
    QJsonObject result(const Entry&,const QJsonObject&) const;
};
}
