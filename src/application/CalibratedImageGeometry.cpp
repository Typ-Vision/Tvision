#include "CalibratedImageGeometry.hpp"
#include "MeasurementService.hpp"
#include <limits>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}}
void CalibratedImageGeometry::stop() const {require(!cancel_ || !cancel_->load(),"Calibrated mapping cancelled");}
CalibratedImageGeometry CalibratedImageGeometry::bind(const ImageFrame& image,const MeasurementExecutionContext& ctx,const std::shared_ptr<std::atomic_bool>& cancel){
    CalibratedImageGeometry r;r.image_=image;r.context_=ctx;r.cancel_=cancel;r.stop();
    require(ctx.calibration && ctx.current,"Mapping requires current calibration");
    const auto& d=ctx.calibration->dependency();d.requireCurrent(*ctx.current);
    require(ctx.calibration->measurementReady(),"Mapping requires validated calibration");
    require(ctx.rawSize==image.geometry().rootSize() && d.resolution==ctx.rawSize,"Mapping actual root resolution mismatch");
    require(!image.metadata().frameId.isEmpty() && !image.metadata().cycleId.isEmpty() && image.metadata().completedAtUtc.isValid(),"Mapping acquisition provenance missing");
    require(d.cameraId==image.metadata().sourceId && d.source.id==image.geometry().rootCoordinateSystemId(),"Mapping root source/frame mismatch");
    return r;
}
void CalibratedImageGeometry::requireRoot(QPointF raw) const {
    stop();auto m=image_.metadata();m.coordinateSystemId=image_.geometry().rootCoordinateSystemId();
    ToolResult t;t.state=ExecutionState::Done;t.outputs["p"]={{"p",PortType::Point,"px","image"},raw,true,m.frameId,m.cycleId,m};
    const auto checked=MeasurementService{}.distance(t,"p",t,"p",*context_.calibration,*context_.current,context_.rawSize,0,std::numeric_limits<double>::max());
    if(checked.state!=ExecutionState::Done)throw std::runtime_error(checked.error.toStdString());
}
QPointF CalibratedImageGeometry::imageToWorld(QPointF p) const {
    return rootToWorld(image_.geometry().toRoot(p));
}
QPointF CalibratedImageGeometry::rootToWorld(QPointF raw) const {
    requireRoot(raw);const auto& c=*context_.calibration;
    if(c.mapping())return PlaneMappingService::map(*c.mapping(),raw);
    auto world=raw*c.millimetresPerPixel();if(c.dependency().target.axes==AxisConvention::XRightYUp)world.setY(-world.y());return world;
}
QPointF CalibratedImageGeometry::worldToImage(QPointF world) const {
    return image_.geometry().fromRoot(worldToRoot(world));
}
QPointF CalibratedImageGeometry::worldToRoot(QPointF world) const {
    stop();const auto& c=*context_.calibration;QPointF raw;
    if(c.mapping())raw=PlaneMappingService::inverseMap(*c.mapping(),world);
    else {raw=world/c.millimetresPerPixel();if(c.dependency().target.axes==AxisConvention::XRightYUp)raw.setY(-raw.y());}
    requireRoot(raw);return raw;
}
void CalibratedImageGeometry::requireSamplingFootprint(QPointF p) const {
    for(double x:{std::floor(p.x()),std::ceil(p.x())})for(double y:{std::floor(p.y()),std::ceil(p.y())})imageToWorld({x,y});
}
void CalibratedImageGeometry::requireImage(const ImageFrame& a) const {
    stop();const auto& b=image_;const auto& x=a.metadata();const auto& y=b.metadata();
    require(a.geometry()==b.geometry() && a.size()==b.size() && a.format()==b.format() && a.contentId()==b.contentId() && a.processingId()==b.processingId() && x.sourceId==y.sourceId && x.coordinateSystemId==y.coordinateSystemId && x.frameId==y.frameId && x.cycleId==y.cycleId && x.completedAtUtc==y.completedAtUtc && x.exposureAtUtc==y.exposureAtUtc && x.deviceTimestampTicks==y.deviceTimestampTicks && x.deviceClockId==y.deviceClockId,"Mapping image differs from immutable snapshot");
}
}
