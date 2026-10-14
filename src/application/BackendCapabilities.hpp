#pragma once
#include <QString>
namespace visionflow {
struct BackendCapability {
    QString id,version,contentId,reason;
    bool available=false;
};
class BackendCapabilities {
public:
    static BackendCapability get(const QString& id);
    static void require(const QString& id,const QString& version,const QString& contentId);
};
}
