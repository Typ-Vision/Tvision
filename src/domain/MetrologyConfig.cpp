#include "MetrologyConfig.hpp"
#include "EdgePointConfig.hpp"
#include "ToolRoi.hpp"
#include <QStringList>
#include <cmath>
#include <stdexcept>
namespace visionflow {
bool isMetrology(const QString& k){return k=="edge_pair" || k=="line_fit";}
QJsonObject metrologyDefaults(const QString& kind){
    auto p=edgePointDefaults();p["mode"]="pixel";p["lower"]=0.;p["upper"]=65536.;
    if(kind=="edge_pair"){p["polarity"]="bright";p["minWidthPx"]=0.;p["maxWidthPx"]=65536.;}
    else {p["scanCount"]=21;p["scanWidthPx"]=2.;p["minPoints"]=6;p["minCoverage"]=.6;p["minSpanCoverage"]=.5;p["minSpanPx"]=10.;p["inlierDistancePx"]=1.;p["maxRmsPx"]=.35;p["maxResidualPx"]=1.;p["minSpanMm"]=1.;p["inlierDistanceMm"]=.1;p["maxRmsMm"]=.035;p["maxResidualMm"]=.1;p["ransac"]=true;p["angleLower"]=-180.;p["angleUpper"]=180.;}
    return p;
}
void validateMetrology(const QString& kind,const QJsonObject& p){
    auto require=[](bool ok){if(!ok)throw std::runtime_error("Invalid metrology configuration");};
    require(isMetrology(kind));const auto d=metrologyDefaults(kind);require(p.size()==d.size());for(auto i=p.begin();i!=p.end();++i)require(d.contains(i.key()));
    auto n=[&](const char* key,double low,double high){const auto v=p[key];require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=low && v.toDouble()<=high);};
    n("stepPx",.05,16);n("widthStepPx",.25,16);n("sigmaPx",0,16);n("minContrast",1e-9,5100);n("lower",0,1e9);n("upper",p["lower"].toDouble(),1e9);
    require(p["mode"]=="pixel" || p["mode"]=="calibrated");require(p["subpixel"].isBool());require(QStringList{"first","last","strongest"}.contains(p["selection"].toString()));
    require(p["scanRoi"].isObject());if(!p["scanRoi"].toObject().isEmpty()){auto r=ToolRoi::fromJson(p["scanRoi"].toObject());require(r.frame.unit==LengthUnit::Pixel);require(kind=="edge_pair" ? std::holds_alternative<ScanSegment>(r.geometry):std::holds_alternative<RotatedRectangle>(r.geometry));if(kind=="edge_pair")require(std::get<ScanSegment>(r.geometry).width<=1024);}
    if(kind=="edge_pair"){require(QStringList{"bright","dark","any"}.contains(p["polarity"].toString()));n("minWidthPx",0,65536);n("maxWidthPx",p["minWidthPx"].toDouble(),65536);require(p["maxWidthPx"].toDouble()>0);}
    else {
        require(QStringList{"rising","falling","any"}.contains(p["polarity"].toString()));require(p["ransac"].isBool());n("scanCount",3,128);n("minPoints",3,p["scanCount"].toDouble());for(auto k:{"scanCount","minPoints"})require(std::floor(p[k].toDouble())==p[k].toDouble());
        n("scanWidthPx",1e-9,1024);n("minCoverage",1e-9,1);n("minSpanCoverage",1e-9,1);
        for(auto k:{"minSpanPx","inlierDistancePx","maxRmsPx","maxResidualPx","minSpanMm","inlierDistanceMm","maxRmsMm","maxResidualMm"})n(k,1e-9,1e9);
        n("angleLower",-180,180);n("angleUpper",p["angleLower"].toDouble(),180);
    }
}
}
