#pragma once
#include "Project.hpp"
#include <QUuid>
#include <QByteArray>
#include <cstdint>

namespace visionflow {
// Frozen W7 state names. Selection never grants Run/Auto permission.
enum class StationMode { Stopped, Preparing, ArmedManual, ArmedAuto, Running, Draining, Blocked };
struct StationRuntimeState {
    StationMode mode=StationMode::Stopped;
    bool deliveryPending=false;
    QString health;
};
struct StationDefinition {
    QUuid id;
    std::uint32_t key=0;
    QString name;
    bool operator==(const StationDefinition&) const = default;
};
struct StationCameraBinding {
    QString sourceId;
    QUuid camera;
    std::uint32_t cameraKey=0;
    bool operator==(const StationCameraBinding&) const = default;
};
struct StationRecipeSelection {
    QUuid revision;
    std::uint32_t recipeKey=0;
    Recipe recipe;
    QString projectDirectory;
    QVector<StationCameraBinding> cameras;
};
struct StationSnapshot {
    StationDefinition station;
    QUuid activation;
    StationRecipeSelection selection;
};
// Append-only activation history preserves old terminal/cycle provenance.
struct StationActivationCatalogue {
    QUuid installation, revision;
    QVector<StationSnapshot> history;
};
QByteArray stationRecipeBytes(const StationRecipeSelection&);
QByteArray encodeStationCatalogue(const StationActivationCatalogue&);
StationActivationCatalogue decodeStationCatalogue(const QByteArray&);
void validateStationCatalogue(const StationActivationCatalogue&);
void validateStationTransition(const StationActivationCatalogue&,const StationActivationCatalogue&);
}
