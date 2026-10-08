#pragma once
#include "ChessboardService.hpp"
#include <QJsonObject>
#include <array>
namespace visionflow {
// Raw pixel -> target-local mm. No lens correction or production acceptance.
// Area is limited to the convex hull of observed inner corners.
class PlaneMapping {
public:
    const std::array<double,9>& matrix() const {return h_;}
    const ChessboardObservation& observation() const {return observation_;}
    const QVector<QPointF>& regionPixels() const {return hull_;}
    double fitRmsMillimetres() const {return fitRms_;}
    double inverseRmsPixels() const {return inverseRms_;}
    double maxInversePixels() const {return inverseMax_;}
private:
    friend class PlaneMappingService;
    PlaneMapping()=default;
    std::array<double,9> h_{};
    ChessboardObservation observation_;
    QVector<QPointF> hull_;
    double fitRms_=0,inverseRms_=0,inverseMax_=0,fitLimit_=0;
};
struct PlaneDistance {QPointF firstMillimetres,secondMillimetres;double pixels=0,millimetres=0;};
class PlaneMappingService {
public:
    static PlaneMapping fit(const ChessboardObservation&,double maxInverseErrorPixels=1.0);
    static QPointF map(const PlaneMapping&,QPointF rawPixel);
    static PlaneDistance distance(const PlaneMapping&,QPointF,QPointF);
    static QJsonObject encode(const PlaneMapping&);
    // Bounded evidence replay; saved fit flags/matrix are never trusted.
    static PlaneMapping decode(const QJsonObject&);
};
}
