#include "EdgePairService.hpp"
#include "CalibratedImageGeometry.hpp"
#include <limits>
#include <stdexcept>
namespace visionflow {
EdgePairResult EdgePairService::measure(const ImageFrame& image,const ToolRoi& roi,const EdgePairOptions& options,EdgePairMode mode,const EdgePairCalibration* calibration,const std::shared_ptr<std::atomic_bool>& cancel) {
    try {
        auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Edge pair measurement cancelled");};stop();
        if(mode!=EdgePairMode::PixelDebug && mode!=EdgePairMode::Calibrated)throw std::runtime_error("Invalid edge pair mode");
        if(mode==EdgePairMode::Calibrated && !calibration)throw std::runtime_error("Edge pair requires calibration");
        const auto search=[&]{
            if(mode==EdgePairMode::PixelDebug)return EdgePairFinder::find(image,roi,options,cancel);
            const auto map=CalibratedImageGeometry::bind(image,{std::make_shared<PlanarCalibration>(calibration->calibration),calibration->current,image.geometry().rootSize(),{}},cancel);
            if(roi.frame.id!=image.metadata().coordinateSystemId || !std::holds_alternative<ScanSegment>(roi.geometry))throw std::runtime_error("Edge pair scan local frame/type mismatch");
            return EdgePairFinder::findMapped(image,std::get<ScanSegment>(roi.geometry),[map](QPointF p){map.requireSamplingFootprint(p);return p;},options,cancel);
        }();
        // Validate calibrated context even for no-found, using real scan endpoints.
        auto metric=[&](QPointF a,QPointF b){
            const auto& m=image.metadata();ToolResult points;points.state=ExecutionState::Done;
            points.outputs["a"]={{"a",PortType::Point,"px","image"},a,true,m.frameId,m.cycleId,m};
            points.outputs["b"]={{"b",PortType::Point,"px","image"},b,true,m.frameId,m.cycleId,m};
            points.outputs["a"].imageGeometry=points.outputs["b"].imageGeometry=image.geometry();
            // Reuse all existing current/frame/hull/evidence checks. The internal
            // interval does not create an Edge Pair decision or tolerance result.
            const auto value=MeasurementService{}.distance(points,"a",points,"b",calibration->calibration,calibration->current,image.geometry().rootSize(),0,std::numeric_limits<double>::max());
            if(value.state!=ExecutionState::Done || !value.millimetres)throw std::runtime_error(value.error.toStdString());
            return *value.millimetres;
        };
        EdgePairResult r;r.origin=image.metadata();
        if(!search.selected) {
            if(mode==EdgePairMode::Calibrated)metric(search.edges.profile.scan.start,search.edges.profile.scan.end);
            r.diagnostic=FitDiagnostic{"PAIR_NOT_FOUND","pair_search",1,0,1,0,scanRejections(search.edges)};
            stop();r.state=ExecutionState::Done;return r;
        }
        const auto& pair=search.candidates[*search.selected];const auto a=search.edges.candidates[pair.firstEdge].point,b=search.edges.candidates[pair.secondEdge].point;
        if(mode==EdgePairMode::Calibrated){r.widthMm=metric(a,b);r.calibration=calibration->calibration.dependency();}
        stop();r.first=a;r.second=b;r.centre=(a+b)/2;r.widthPx=pair.widthPx;r.found=true;r.state=ExecutionState::Done;return r;
    }catch(const std::exception& e){EdgePairResult r;r.error=QString::fromUtf8(e.what());return r;}
}
}
