#include "PoseMatchConfig.hpp"
#include <QSet>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool ok){if(!ok)throw std::runtime_error("Invalid pattern_pose configuration");}}
QJsonObject poseMatchDefaults(){return {{"modelRoi",QJsonObject{}},{"modelPng",""},{"anchor",QJsonObject{}},{"searchRoi",QJsonObject{}},
    {"angleMin",-30.},{"angleMax",30.},{"angleStep",10.},{"scaleMin",1.},{"scaleMax",1.},{"scaleStep",.1},
    {"minScore",.8},{"ambiguityGap",.03},{"nmsDistance",8.},{"pyramidLevels",2},{"maxCandidates",16},{"selection","reject_ambiguous"}};}
void validatePoseMatch(const QJsonObject& p){
    const auto defaults=poseMatchDefaults();require(p.size()==defaults.size());for(auto it=p.begin();it!=p.end();++it)require(defaults.contains(it.key()));
    auto n=[&](const char* key,double lo,double hi){const auto v=p[key];require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=lo && v.toDouble()<=hi);return v.toDouble();};
    auto a=n("angleMin",-180,180),b=n("angleMax",-180,180),step=n("angleStep",1,90);require(a<=b);
    auto s=n("scaleMin",.5,2),t=n("scaleMax",.5,2),ss=n("scaleStep",.02,.5);require(s<=t);
    const auto ac=std::ceil((b-a)/step)+1,sc=std::ceil((t-s)/ss)+1;require(ac<=37 && sc<=17 && ac*sc<=255);
    n("minScore",0,1);n("ambiguityGap",0,1);n("nmsDistance",2,1024);
    for(auto key:{"pyramidLevels","maxCandidates"}){auto v=n(key,QString(key)=="pyramidLevels" ? 1:2,QString(key)=="pyramidLevels" ? 3:32);require(v==std::floor(v));}
    require(p["selection"]=="reject_ambiguous" || p["selection"]=="best");
    require(p["modelPng"].isString() && p["modelPng"].toString().size()<=4*1024*1024);
    for(auto key:{"modelRoi","searchRoi"}){require(p[key].isObject());const auto r=p[key].toObject();if(r.isEmpty())continue;require(r.size()==4);
        for(auto k:{"x","y","width","height"}){auto v=r[k];require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()==std::floor(v.toDouble()) && v.toDouble()>=0 && v.toDouble()<=100000);}
        require(r["width"].toInt()>0 && r["height"].toInt()>0);
    }
    require(p["anchor"].isObject());const auto anchor=p["anchor"].toObject();if(!anchor.isEmpty()){
        require(anchor.size()==2);for(auto k:{"x","y"})require(anchor[k].isDouble() && std::isfinite(anchor[k].toDouble()) && anchor[k].toDouble()>=0 && anchor[k].toDouble()<=255);
        const auto r=p["modelRoi"].toObject();require(!r.isEmpty() && anchor["x"].toDouble()<=r["width"].toDouble()-1 && anchor["y"].toDouble()<=r["height"].toDouble()-1);
    }
}
}
