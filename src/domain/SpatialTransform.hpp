#pragma once
#include <QJsonObject>
#include <QString>
#include <array>
namespace visionflow {
// Geometric identity, never an acquisition frame/cycle ID. At least one owner ID.
struct FrameRef {
    QString deviceId,robotId,groupId,frameUuid;
    int revision=0;
    bool operator==(const FrameRef&) const = default;
    void validate() const;
    QJsonObject toJson() const;
    static FrameRef fromJson(const QJsonObject&);
};
struct Rotation3D {
    double w=1,x=0,y=0,z=0;
    // Reject corruption; normalize only floating point drift within this norm tolerance.
    static constexpr double normTolerance=1e-6;
    Rotation3D normalized() const;
    bool equivalent(const Rotation3D&,double tolerance=1e-12) const;
    QJsonObject toJson() const;
    static Rotation3D fromJson(const QJsonObject&);
};
// T_target_source: p_target = R * p_source + translationMm.
// Right-handed, column vectors, quaternion wxyz. No Euler/HALCON/matrix import.
struct SpatialTransform {
    FrameRef source,target;
    std::array<double,3> translationMm{};
    Rotation3D rotation;
    void validate() const;
    SpatialTransform inverse() const;
    // this T_B_C then next T_A_B produces T_A_C; exact frame/revision match.
    SpatialTransform then(const SpatialTransform& next) const;
    QJsonObject toJson() const;
    static SpatialTransform fromJson(const QJsonObject&);
};
}
