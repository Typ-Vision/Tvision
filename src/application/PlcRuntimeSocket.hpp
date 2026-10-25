#pragma once
#include "RuntimeCoordinator.hpp"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QFutureWatcher>

namespace visionflow {
struct PlcSocketOptions {
    QHostAddress address=QHostAddress::LocalHost;
    quint16 port=5000;
    int ackTimeoutMs=2000;
    int retryCount=3;
    QString measurementToolId;
};
// One owner thread, one PLC peer, one outstanding request. Workers use Runtime snapshots.
class PlcRuntimeSocket : public QObject {
public:
    explicit PlcRuntimeSocket(QObject* parent=nullptr);
    void bind(RuntimeService* runtime,RuntimeCoordinator* coordinator,QString archive={},QString catalog={},QString mapping={});
    void setMapping(QString mapping);
    void setUnavailable(std::function<QString()> callback){owner();unavailable_=std::move(callback); }
    PlcRuntimeSocket(RuntimeService& runtime,QString archive,QString catalog,
                     std::function<QString()> unavailable={},QObject* parent=nullptr);
    ~PlcRuntimeSocket() override;
    void listen(const PlcSocketOptions&);
    void stop(); // Refuses to discard an unacknowledged or unrecorded cycle.
    void cancel();
    void retryRecording();
    bool listening() const {return server_.isListening();}
    bool connected() const {return peer_ && peer_->state()==QAbstractSocket::ConnectedState;}
    bool blocking() const {return !outstanding_.isEmpty();}
    bool working() const {return watcher_!=nullptr;}
    bool recordingPending() const {return pending_.has_value() && fault_=="RECORDING_PENDING";}
    quint16 port() const {return server_.serverPort();}
    QString sessionId() const {return session_;}
    QString state() const;
    QStringList diagnostics() const {return diagnostics_;}
    std::function<void()> changed;
    std::function<void(const CycleResult&)> completed;
    std::function<void()> started;
private:
    struct Entry {QJsonObject request,response;bool acknowledged=false;QString ackEventId;QJsonObject ackEvent;};
    RuntimeService* runtime_=nullptr;
    RuntimeCoordinator* coordinator_=nullptr;
    std::unique_ptr<ProductionStore> store_;
    QString catalog_;
    std::function<QString()> unavailable_;
    QTcpServer server_;
    QPointer<QTcpSocket> peer_;
    QTimer timer_;
    QElapsedTimer ackClock_,partialClock_;
    QByteArray input_;
    PlcSocketOptions options_;
    QString session_,outstanding_,fault_;
    QMap<QString,Entry> entries_;
    QStringList diagnostics_;
    ProductionCycle cycle_;
    QFutureWatcher<CycleResult>* watcher_=nullptr;
    std::shared_ptr<std::atomic_bool> cancel_;
    std::optional<CycleResult> pending_;
    QString resultEventId_;
    QJsonObject resultEvent_;
    int retries_=0;
    QString unavailableReason() const;
    void owner() const;
    void log(const QString&);
    void acceptPeer();
    void readPeer();
    void receive(const QJsonObject&);
    void send(QJsonObject);
    QJsonObject status(const QString& type="STATUS") const;
    void error(const QString& code,const QString& request={});
    void trigger(const QJsonObject&);
    void acknowledge(const QString&);
    void tick();
    void disconnectPeer();
    QJsonObject event(const QString& action,const QString& request) const;
};
}
