#include "EdgePointConfig.hpp"
#include "ToolRoi.hpp"
#include <QStringList>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace visionflow {
QJsonObject edgePointDefaults(){return {{"scanRoi",QJsonObject{}},{"stepPx",.5},{"widthStepPx",1},{"sigmaPx",1},{"polarity","any"},{"selection","strongest"},{"minContrast",20},{"subpixel",true}};}
void validateEdgePoint(const QJsonObject& p) {
    auto require=[](bool ok){if(!ok)throw std::runtime_error("Invalid Edge Point configuration");};
    const auto defaults=edgePointDefaults();require(p.size()==defaults.size());for(auto i=p.begin();i!=p.end();++i)require(defaults.contains(i.key()));
    auto number=[&](const char* key,double low,double high){const auto v=p[key];require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=low && v.toDouble()<=high);};
    number("stepPx",.05,16);number("widthStepPx",.25,16);number("sigmaPx",0,16);number("minContrast",std::numeric_limits<double>::min(),5100);
    require(p["polarity"].isString() && QStringList{"rising","falling","any"}.contains(p["polarity"].toString()));
    require(p["selection"].isString() && QStringList{"first","last","strongest"}.contains(p["selection"].toString()));require(p["subpixel"].isBool());
    require(p["scanRoi"].isObject());const auto json=p["scanRoi"].toObject();
    if(!json.isEmpty()){const auto r=ToolRoi::fromJson(json);require(r.frame.unit==LengthUnit::Pixel && std::holds_alternative<ScanSegment>(r.geometry) && std::get<ScanSegment>(r.geometry).width<=1024);}
}
}
