#include "EdgePointService.hpp"
#include "MeasurementDiagnostic.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/metrology/EdgeFinder.hpp"
#include <stdexcept>
#include <algorithm>
namespace visionflow {
ToolResult EdgePointService::execute(const ToolDefinition& tool,const QMap<QString,PortValue>& inputs,const QString&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel) {
    ToolRegistry::validate(tool);const auto p=tool.parameters;
    if(p.contains("poseTracking"))throw std::runtime_error("Pose tracking requires DAG resolution");
    if(p["scanRoi"].toObject().isEmpty())throw std::runtime_error("Edge Point requires a taught scan ROI");
    const auto image=std::get<ImageFrame>(inputs.value("image").data);
    EdgeOptions options;options.profile={p["stepPx"].toDouble(),p["widthStepPx"].toDouble(),p["sigmaPx"].toDouble()};
    options.polarity=p["polarity"]=="rising" ? EdgePolarity::Rising:p["polarity"]=="falling" ? EdgePolarity::Falling:EdgePolarity::Any;
    options.selection=p["selection"]=="first" ? EdgeSelection::First:p["selection"]=="last" ? EdgeSelection::Last:EdgeSelection::Strongest;
    options.minContrast=p["minContrast"].toDouble();options.subpixel=p["subpixel"].toBool();
    const auto result=EdgeFinder::find(image,ToolRoi::fromJson(p["scanRoi"].toObject()),options,cancel);
    ToolResult r;r.toolId=tool.id;r.state=ExecutionState::Done;r.message=result.selected ? "FOUND":"NOT_FOUND";
    auto put=[&](const QString& id,PortData data,bool valid=true){for(const auto& port:outputPorts(tool.kind))if(port.id==id){PortValue v{port,std::move(data),valid,frameId,cycleId,image.metadata()};v.method="profile-edge";v.methodVersion=1;if(!valid)v.invalidReason="NOT_FOUND";r.outputs[id]=std::move(v);}};
    put("found",bool(result.selected));
    if(result.selected){const auto& c=result.candidates[*result.selected];put("point",c.point);put("strength",c.strength);put("signedStrength",c.signedStrength);put("refined",c.refined);r.points.append(c.point);r.message+=" • "+c.refinement;}
    else {put("point",QPointF{},false);put("strength",0.0,false);put("signedStrength",0.0,false);put("refined",false,false);}
    EdgeDiagnostics d;d.scan={result.profile.scan.start,result.profile.scan.end};d.width=result.profile.scan.width;d.length=d.scan.length();
    d.sampleCount=result.profile.samples.size();d.derivativeCount=result.profile.derivatives.size();d.candidateCount=result.candidates.size();
    // Evenly spaced display samples only; selection always uses the complete profile.
    auto indices=[](int count){QVector<int> out;const int shown=std::min(count,2048);for(int i=0;i<shown;++i)out.append(shown==1 ? 0:int(qint64(i)*(count-1)/(shown-1)));return out;};
    for(auto i:indices(d.sampleCount)){const auto& s=result.profile.samples[i];d.profile.append({s.distancePx,s.intensity,s.smoothed});}
    for(auto i:indices(d.derivativeCount)){const auto& s=result.profile.derivatives[i];d.derivative.append({s.distancePx,s.value});}
    auto shown=indices(d.candidateCount);if(result.selected && !shown.contains(*result.selected)){if(shown.size()==2048)shown.removeLast();shown.append(*result.selected);std::sort(shown.begin(),shown.end());}
    for(auto i:shown){const auto& c=result.candidates[i];d.candidates.append({c.point,c.distancePx,c.signedStrength,c.accepted,result.selected && *result.selected==i,c.reason,c.refinement});}
    if(!result.selected){r.fitDiagnostic=FitDiagnostic{"EDGE_NOT_FOUND","edge_search",1,0,1,0,scanRejections(result)};r.message=presentMeasurementDiagnostic(*r.fitDiagnostic);}
    r.edgeDiagnostics=std::move(d);
    return r;
}
}
