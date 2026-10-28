#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
class CalibratedImageGeometry {
public:
    static CalibratedImageGeometry bind(const ImageFrame&,const MeasurementExecutionContext&,const std::shared_ptr<std::atomic_bool>& cancel={});
    QPointF imageToWorld(QPointF) const;
    QPointF worldToImage(QPointF) const;
    QPointF rootToWorld(QPointF) const;
    QPointF worldToRoot(QPointF) const;
    void requireSamplingFootprint(QPointF) const;
    void requireImage(const ImageFrame&) const;
private:
    void stop() const;
    void requireRoot(QPointF) const;
    ImageFrame image_;
    MeasurementExecutionContext context_;
    std::shared_ptr<std::atomic_bool> cancel_;
};
}
