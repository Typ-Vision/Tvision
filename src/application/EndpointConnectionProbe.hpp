#pragma once
#include "CommunicationProfiles.hpp"
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <functional>
namespace visionflow {
// Connection-only probe: never routes input, triggers, or production data.
class EndpointConnectionProbe final : public QObject {
public:
    explicit EndpointConnectionProbe(QObject* parent=nullptr);
    void connectProfile(const EndpointProfile&, bool testOnly=false);
    void disconnectProfile();
    bool active() const;
    QString state() const {return state_;}
    std::function<void()> changed;
private:
    QTcpServer server_; QTcpSocket socket_; QTimer timer_;
    QString state_="DISCONNECTED"; bool testOnly_=false;
    void report(const QString&);
};
}
