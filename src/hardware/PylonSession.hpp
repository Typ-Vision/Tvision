#pragma once
#include <QJsonObject>
#include <QStringList>
#include <memory>

namespace visionflow {
struct PylonSelector { QString vendor,model,serial; };
// Owner-thread, exclusive, read-only G0 session. Caller holds the common physical lease.
// One native session owns init/enumeration/terminate at a time; no SDK calls escape it.
class PylonSession final {
public:
    PylonSession();
    ~PylonSession();
    PylonSession(const PylonSession&)=delete;
    PylonSession& operator=(const PylonSession&)=delete;
    static bool available();
    static QJsonObject abi();
    QJsonObject open(const QString& runtime,const QStringList& dependencies,const PylonSelector&);
    void close();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
