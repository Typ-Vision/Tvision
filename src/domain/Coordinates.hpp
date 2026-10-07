#pragma once
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>
#include <QJsonObject>
namespace visionflow {
enum class LengthUnit { Pixel, Millimetre };
enum class AxisConvention { XRightYDown, XRightYUp };
// Geometric frame, distinct from acquisition frameId/cycleId. Integer pixel centres.
struct CoordinateFrame {
    QString id;
    LengthUnit unit=LengthUnit::Pixel;
    AxisConvention axes=AxisConvention::XRightYDown;
    bool operator==(const CoordinateFrame&) const = default;
    void validate() const;
};
struct Point2D { QPointF position; CoordinateFrame frame; };
struct Pose2D {
    QPointF position;
    double angleRadians=0;
    CoordinateFrame frame;
    void validate() const;
};
// T_target_source: p_target = R(angle) * p_source + translation.
// Positive angle turns +X toward +Y: clockwise in images, CCW on Y-up planes.
struct Transform2D {
    CoordinateFrame source,target;
    QPointF translation;
    double angleRadians=0;
    void validate() const;
    Point2D map(const Point2D&) const;
    Pose2D map(const Pose2D&) const;
    Transform2D inverse() const;
    Transform2D then(const Transform2D& next) const;
};
Transform2D poseTransform(const Pose2D&,const CoordinateFrame& localFrame);
struct TrackingRoi {
    CoordinateFrame localFrame,referenceFrame;
    QVector<QPointF> localCorners;
    static TrackingRoi teach(const QVector<Point2D>& corners,const Pose2D& reference,const CoordinateFrame& local);
    QVector<Point2D> resolve(const Pose2D& reference) const;
};
// Schema 3-5 point anchor adapter: round displacement once and preserve size.
// No angle accepted; rotated polygons must never become AABB silently.
QRect translationTrackingRoi(const QRect& taught,const Point2D& origin,const Point2D& current,const QSize& imageSize);
// Dependency record for calibration persistence/activation in 6C, not calibration math.
struct CalibrationDependency {
    QString calibrationId;
    int version=0;
    QString contentId,cameraId,lensId;
    QSize resolution;
    QString workingPlaneId;
    CoordinateFrame source,target;
    bool operator==(const CalibrationDependency&) const = default;
    void validate() const;
    void requireCurrent(const CalibrationDependency& current) const;
    QJsonObject toJson() const;
    static CalibrationDependency fromJson(const QJsonObject&);
};
}
