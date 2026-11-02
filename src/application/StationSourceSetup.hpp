#pragma once
#include "StationIdentityRegistry.hpp"
#include <QSize>
namespace visionflow {
class StationSourceSetup final {
public:
    explicit StationSourceSetup(StationIdentityStore& store):store_(store){}
    static QSize inspectImage(const QString& path);
    // A single identity CAS commits name, keys and complete source together.
    StationFileSource save(const QUuid& expectedCatalogue,const QUuid& station,
        const QString& name,const QUuid& camera,const QString& cameraName,const QString& path);
private:
    StationIdentityStore& store_;
};
}
