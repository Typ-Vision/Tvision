#pragma once
#include "CalibrationCapture.hpp"
#include "MeasurementSourceIdentity.hpp"
#include "domain/Project.hpp"
namespace visionflow {
// Immutable value captured on the owner thread. No worker accesses the catalog.
// The original model/evidence is never relabelled or written to legacy codecs.
class SystemCalibration {
public:
    static std::optional<SetupReference> setupReference(const Recipe&);
    static QJsonObject sourceBinding(const QString& sourceToolId,const SetupReference&);
    static ToolDefinition source(const Recipe&);
    static bool requestsCalibration(const Recipe&,bool includeOptionalPoints=true);
    static CalibrationDependency binding(const ActiveCalibrationSnapshot&,const QString& sourceToolId,const QString& rawCoordinates);
    static QJsonObject evidence(const ActiveCalibrationSnapshot&,const QString& sourceToolId,const QString& rawCoordinates);
    static ActiveCalibrationSnapshot decodeEvidence(const QJsonObject&);
    static MeasurementExecutionContext resolve(const ActiveCalibrationSnapshot&,const Recipe&,const QString& directory,const ImageFrame&);
    static Recipe prepareRecipe(const ActiveCalibrationSnapshot&,const Recipe&,const QString& directory);
    static QStringList impact(const ActiveCalibrationSnapshot&,const Recipe&,const QString& directory);
    static void checkInstallation(const ActiveCalibrationSnapshot& s,const Recipe& r,const QString& directory,const QSize& size){compatible(s,r,directory,size);}
private:
    static void compatible(const ActiveCalibrationSnapshot&,const Recipe&,const QString& directory,const QSize&);
    static PlanarCalibration executionModel(const ActiveCalibrationSnapshot&,const QString&,const QString&);
};
}
