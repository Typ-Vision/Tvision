#pragma once
#include "Coordinates.hpp"
#include "SpatialTransform.hpp"
#include <QByteArray>
#include <QJsonObject>
#include <variant>
namespace visionflow {
struct CodeResult {QString text,symbology;QByteArray bytes;QVector<QPointF> corners;};
struct CodeResults {QVector<CodeResult> items;};
struct PointSet {QVector<QPointF> points;CoordinateFrame frame;};
struct ProfileSample {QPointF position;double value=0,derivative=0;};
struct Profile {QVector<ProfileSample> samples;};
struct Line2D {QPointF start,end;CoordinateFrame frame;};
struct Circle2D {QPointF centre;double radius=0;CoordinateFrame frame;};
struct Arc2D {Circle2D circle;double startDegrees=0,sweepDegrees=0;};
struct InspectionResult {QString name,reason;bool accepted=false;};
struct InspectionResults {QVector<InspectionResult> items;};
using ToolValueData=std::variant<QString,CodeResults,PointSet,Profile,Line2D,Circle2D,Arc2D,Pose2D,InspectionResults,SpatialTransform>;
// Strict bounded payload format, independent of OpenCV and acquisition storage.
// Payloads never imply a shared camera: their PortValue envelope supplies the
// source/frame/cycle origin. SourceFrameIndex validates it per acquired branch;
// a shared world frame or equal scalar/pixels cannot authorize cross-source use.
class ToolValues {
public:
    static constexpr int maxItems=8192,maxCodes=128,maxText=65536,maxJsonBytes=2*1024*1024;
    static QJsonObject encode(const ToolValueData&);
    static ToolValueData decode(const QJsonObject&);
};
}
