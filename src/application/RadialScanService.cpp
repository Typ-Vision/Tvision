#include "CalibratedImageGeometry.hpp"
#include "RadialScanService.hpp"
#include <limits>
#include <stdexcept>
namespace visionflow {
CalibratedRadialSearch RadialScanService::scan(const ImageFrame& image,const ArcRoi& roi,const RadialScanOptions& options,const RadialScanCalibration& context,const std::shared_ptr<std::atomic_bool>& cancel) {
    const auto& c=context.calibration;
    const auto map=CalibratedImageGeometry::bind(image,{std::make_shared<PlanarCalibration>(c),context.current,image.geometry().rootSize(),{}},cancel);
    auto guard=[&](QPointF p){map.imageToWorld(p);};
    auto inverse=[&](QPointF p){const auto q=map.worldToImage(p);map.requireSamplingFootprint(q);return q;};
    guard(inverse(roi.ring.centre));
    const auto search=RadialScanner::findMapped(image,roi,inverse,options,cancel);
    for(const auto& s:search.scans){guard(s.rawStart);guard(s.rawEnd);if(s.rawPoint)guard(*s.rawPoint);}
    return {search,c.dependency()};
}
}
