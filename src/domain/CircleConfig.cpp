#include "CircleConfig.hpp"
#include "EdgePointConfig.hpp"
#include "ToolRoi.hpp"
#include "Coordinates.hpp"
#include <cmath>
#include <stdexcept>
namespace visionflow {
QJsonObject circleDefaults(){
    auto p=edgePointDefaults();
    p["mode"]="pixel";p["direction"]="outward";p["scanCount"]=72;p["scanWidth"]=1.;p["unitsPerSample"]=1.;
    p["minPoints"]=6;p["minFitCoverage"]=.6;p["minAngularSpan"]=90.;p["maxCondition"]=1000.;p["ransac"]=true;
    p["inlierDistance"]=1.;p["maxRms"]=.35;p["maxResidual"]=1.;
    p["minFoundPoints"]=6;p["minCoverage"]=.9;p["maxGapDegrees"]=15.;
    p["diameterLower"]=0.;p["diameterUpper"]=65536.;p["deviationLower"]=0.;p["deviationUpper"]=1.;
    return p;
}
void validateCircle(const QJsonObject& p){
    auto require=[](bool ok){if(!ok)throw std::runtime_error("Invalid Circle configuration");};
    const auto d=circleDefaults();require(p.size()==d.size()+(p.contains("worldCalibration") ? 1:0));for(auto i=p.begin();i!=p.end();++i)require(d.contains(i.key()) || i.key()=="worldCalibration");
    if(p.contains("worldCalibration")){require(p["worldCalibration"].isObject());CalibrationDependency::fromJson(p["worldCalibration"].toObject());require(p["mode"]=="calibrated");}
    auto n=[&](const char* k,double lo,double hi){require(p[k].isDouble() && std::isfinite(p[k].toDouble()) && p[k].toDouble()>=lo && p[k].toDouble()<=hi);};
    require(p["mode"]=="pixel" || p["mode"]=="calibrated");require(p["direction"]=="outward" || p["direction"]=="inward");
    require(p["selection"]=="first" || p["selection"]=="last" || p["selection"]=="strongest");require(p["polarity"]=="rising" || p["polarity"]=="falling" || p["polarity"]=="any");
    require(p["subpixel"].isBool() && p["ransac"].isBool());
    n("stepPx",.05,16);n("widthStepPx",.25,16);n("sigmaPx",0,16);n("minContrast",1e-9,5100);
    n("scanCount",6,128);n("minPoints",6,p["scanCount"].toDouble());n("minFoundPoints",6,p["scanCount"].toDouble());
    for(auto k:{"scanCount","minPoints","minFoundPoints"})require(p[k].toDouble()==std::floor(p[k].toDouble()));
    n("scanWidth",1e-9,1024);n("unitsPerSample",1e-9,1e9);if(p["mode"]=="pixel")require(p["unitsPerSample"]==1.);
    for(auto k:{"minFitCoverage","minCoverage"})n(k,1e-9,1);
    n("minAngularSpan",1e-9,360);n("maxCondition",1,1e12);n("maxGapDegrees",1e-9,360);
    for(auto k:{"inlierDistance","maxRms","maxResidual"})n(k,1e-9,1e9);
    n("diameterLower",0,1e9);n("diameterUpper",p["diameterLower"].toDouble(),1e9);
    n("deviationLower",0,1e9);n("deviationUpper",p["deviationLower"].toDouble(),1e9);
    require(p["scanRoi"].isObject());if(!p["scanRoi"].toObject().isEmpty()){
        auto r=ToolRoi::fromJson(p["scanRoi"].toObject());require(std::holds_alternative<Annulus>(r.geometry) || std::holds_alternative<ArcRoi>(r.geometry));
        require(r.frame.unit==(p["mode"]=="pixel" ? LengthUnit::Pixel:LengthUnit::Millimetre));
    }
}
}
