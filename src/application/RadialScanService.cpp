#include "RadialScanService.hpp"
#include <limits>
#include <stdexcept>
namespace visionflow {
CalibratedRadialSearch RadialScanService::scan(const ImageFrame& image,const ArcRoi& roi,const RadialScanOptions& options,const RadialScanCalibration& context,const std::shared_ptr<std::atomic_bool>& cancel) {
    const auto& c=context.calibration;const auto& m=image.metadata();
    auto guard=[&](QPointF raw){
        ToolResult t;t.state=ExecutionState::Done;t.outputs["p"]={{"p",PortType::Point,"px","image"},raw,true,m.frameId,m.cycleId,m};
        const auto checked=MeasurementService{}.distance(t,"p",t,"p",c,context.current,image.size(),0,std::numeric_limits<double>::max());
        if(checked.state!=ExecutionState::Done)throw std::runtime_error(checked.error.toStdString());
    };
    // Validate acceptance/current/provenance even if every radial scan is missing.
    auto inverse=[&](QPointF world){
        if(cancel && cancel->load())throw std::runtime_error("Calibrated radial scan cancelled");
        if(c.mapping())return PlaneMappingService::inverseMap(*c.mapping(),world);
        auto p=world/c.millimetresPerPixel();if(c.dependency().target.axes==AxisConvention::XRightYUp)p.setY(-p.y());return p;
    };
    guard(inverse(roi.ring.centre));
    const auto search=RadialScanner::findMapped(image,roi,inverse,options,cancel);
    for(const auto& s:search.scans){guard(s.rawStart);guard(s.rawEnd);if(s.rawPoint)guard(*s.rawPoint);}
    return {search,c.dependency()};
}
}
