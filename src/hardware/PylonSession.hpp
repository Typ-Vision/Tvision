#pragma once
#include <QJsonObject>
#include <QStringList>
#include <memory>
#include "CameraFeatures.hpp"
#include <functional>
#include <QSize>
#include <QPoint>

namespace visionflow {
struct PylonSelector { QString vendor,model,serial; };
struct PylonFrame {
    QByteArray pixels;
    QSize size;
    QPoint offset;
    quint64 blockId=0,timestamp=0;
    size_t stride=0;
};
// Owner-thread, exclusive native session. Caller holds the common physical lease.
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
    CameraFeature feature(const QString&) const;
    void writeFeature(const QString&,const FeatureValue&);
    // Untrusted diagnostic/preview acquisition; BlockID is not trigger evidence.
    void start(bool softwareTrigger,const std::function<int()>& budget);
    PylonFrame grab(const std::function<int()>& budget);
    void stop();
    void close();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
