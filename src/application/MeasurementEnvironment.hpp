#pragma once
#include "ActiveCalibrationService.hpp"
#include "MeasurementSourceIdentity.hpp"
#include <QSet>
#include <QThread>

namespace visionflow {
struct RememberedCalibration {
    SetupReference target;
    QString id, contentId;
    int version=1;
    bool operator==(const RememberedCalibration&) const = default;
};
// Persistent choices only. This type cannot grant acquisition/Runtime permission.
struct MeasurementEnvironment {
    QString projectId;
    int revision=0;
    MeasurementSetupContract current;
    QVector<MeasurementSetupDescriptor> history;
    QSet<QString> archivedCameras, archivedSetups;
    QMap<QString,QString> operational; // CameraId -> SetupId
    QVector<RememberedCalibration> selections;
    QJsonObject encode(int capacity=128) const;
    static MeasurementEnvironment decode(const QJsonObject&,int capacity=128);
};
class MeasurementEnvironmentRepository {
public:
    enum class Fault { None, BeforeWrite, BeforeCommit, AfterCandidate };
    MeasurementEnvironmentRepository(QString absoluteProjectPath,QString projectId,int capacity=128);
    MeasurementEnvironment load() const;
    // Compare the entire expected document under lock, including its revision.
    MeasurementEnvironment commit(const MeasurementEnvironment& expected,MeasurementEnvironment next,Fault=Fault::None) const;
    // Candidate first, selection second. Retry after interruption is idempotent.
    MeasurementEnvironment saveAndRemember(const MeasurementEnvironment& expected,const CalibrationModelRecord&,
        const SetupReference&,Fault=Fault::None) const;
    // Explicit dependency bundle for Save As/import/export; no legacy auto migration.
    QJsonObject exportBundle() const;
    void importBundle(const QJsonObject&) const; // fresh destination only
    QString path() const {return projectPath_+".measurement-environment.json";}
    QString catalogPath() const {return projectPath_+".active-calibrations.json";}
private:
    void validateDependencies(const MeasurementEnvironment&) const;
    QString projectPath_,projectId_;
    int capacity_;
};
// Application-owned, owner-thread state; no QWidget/recipe ownership.
class MeasurementEnvironmentRegistry {
public:
    explicit MeasurementEnvironmentRegistry(MeasurementEnvironmentRepository);
    const MeasurementEnvironment& snapshot() const;
    void reload();
    void replace(MeasurementSetupContract);
    void archiveCamera(const CameraId&);
    void archiveSetup(const SetupId&);
    void setOperational(const CameraId&,const SetupId&);
private:
    void owner() const;
    void store(MeasurementEnvironment);
    MeasurementEnvironmentRepository repository_;
    MeasurementEnvironment state_;
    QThread* thread_;
};
}
