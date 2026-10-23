#pragma once
#include "vision/ImageFrame.hpp"
#include <QJsonObject>
#include <QPointF>
#include <QVector>
#include <optional>
namespace visionflow {
enum class CalibrationGridPattern { Chessboard, SymmetricCircles, FramedCircles };
enum class PlatePolarity { DarkOnLight };
enum class PlateOrientation { Unmarked, TriangleCorner };
struct FramedPlateGeometry {
    PlatePolarity polarity=PlatePolarity::DarkOnLight;
    PlateOrientation orientation=PlateOrientation::TriangleCorner;
    std::optional<double> circleDiameterMm,frameThicknessMm,innerWidthMm,innerHeightMm;
    void validate() const;
};
struct CalibrationGridTarget {
    QString id;
    int columns=9,rows=6;
    // Historical field name retained for source compatibility. For circles this
    // is centre-to-centre spacing, never diameter. Codec uses spacingMm + kind.
    double squareMillimetres=0;
    CalibrationGridPattern pattern=CalibrationGridPattern::Chessboard;
    std::optional<FramedPlateGeometry> framed;
    void validate() const;
    QJsonObject toJson() const;
    static CalibrationGridTarget fromJson(const QJsonObject&);
};
struct CalibrationGridObservation {
    CalibrationGridTarget target;
    FrameMetadata source;
    QSize resolution;
    QVector<QPointF> pixels,worldMillimetres;
    QString pixelHash;
    // Completeness, raw provenance and topology only; never fits a model.
    void validateRaw() const;
};
struct PlateCentre {
    int row=0,column=0;
    QPointF rawPixel;
    // Absent for legacy detectors that do not estimate localization uncertainty.
    std::optional<double> uncertaintyPixels;
};
struct PlateOrientationEvidence {
    PlateOrientation kind=PlateOrientation::Unmarked;
    int quarterTurns=0;
    bool mirrored=false;
    std::optional<QPointF> triangleRaw;
};
// Validated value object: no mutable access, no implicit model conversion.
class RawPlateObservation {
public:
    static RawPlateObservation create(CalibrationGridObservation, QVector<PlateCentre>,
        PlateOrientationEvidence, QString algorithmVersion, QString coordinateSpace="raw_pixel_centres/1");
    const CalibrationGridObservation& grid() const {return grid_;}
    const QVector<PlateCentre>& centres() const {return centres_;}
    const PlateOrientationEvidence& orientation() const {return orientation_;}
    const QString& algorithmVersion() const {return algorithm_;}
    const QString& coordinateSpace() const {return space_;}
private:
    RawPlateObservation()=default;
    CalibrationGridObservation grid_;
    QVector<PlateCentre> centres_;
    PlateOrientationEvidence orientation_;
    QString algorithm_,space_;
};
enum class PlateDetectionStatus { Ready, Unsupported, InvalidInput, NotFound, Cancelled };
struct PlateDetectionDiagnostics {
    PlateDetectionStatus status=PlateDetectionStatus::NotFound;
    QString message;
    QVector<QPointF> partialCandidates; // Never model input.
};
class PlateDetectionResult {
public:
    static PlateDetectionResult ready(RawPlateObservation);
    static PlateDetectionResult failure(PlateDetectionStatus,QString);
    const PlateDetectionDiagnostics& diagnostics() const {return diagnostics_;}
    const std::optional<RawPlateObservation>& observation() const {return observation_;}
private:
    PlateDetectionResult()=default;
    PlateDetectionDiagnostics diagnostics_;
    std::optional<RawPlateObservation> observation_;
};
// Existing callers and legacy chessboard codec keep their source/byte contract.
using ChessboardTarget=CalibrationGridTarget;
using ChessboardObservation=CalibrationGridObservation;
}
