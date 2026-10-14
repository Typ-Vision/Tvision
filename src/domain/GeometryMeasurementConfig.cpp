#include "GeometryMeasurementConfig.hpp"
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
const QStringList operations{"point_distance","x_offset","y_offset","point_line_distance","point_segment_distance","line_angle","line_intersection","parallel_distance","circle_centre_distance","circle_radius","circle_diameter"};
}
QJsonObject geometryMeasurementDefaults(){
    return {{"mode","calibrated"},{"operation","point_distance"},{"offset","absolute"},{"angle","acute_undirected"},
        {"fitMinPoints",6},{"fitInlierDistance",.1},{"fitMaxRms",.035},{"fitMaxResidual",.1},{"fitRansac",true},
        {"lineMinSpan",.1},{"lineMinVarianceRatio",25.},{"circleMinAngularSpan",90.},{"circleMaxCondition",1000.},{"toleranceEnabled",false},{"nominal",0.},{"lower",0.},{"upper",1000000.}};
}
void validateGeometryMeasurement(const QJsonObject& p){
    const auto d=geometryMeasurementDefaults();const bool legacy=!p.contains("toleranceEnabled") && !p.contains("nominal") && !p.contains("lower") && !p.contains("upper");
    require(p.size()==d.size()-(legacy ? 4:0),"Geometry parameters missing/extra");
    for(auto i=p.begin();i!=p.end();++i)require(d.contains(i.key()),"Unknown geometry parameter");
    require(p["mode"]=="pixel" || p["mode"]=="calibrated","Geometry mode must be pixel/calibrated");
    require(p["operation"].isString() && operations.contains(p["operation"].toString()),"Unknown geometry operation");
    require(p["offset"]=="signed" || p["offset"]=="absolute","Unknown geometry offset convention");
    require(p["angle"]=="signed_directed" || p["angle"]=="unsigned_directed" || p["angle"]=="acute_undirected","Unknown geometry angle convention");
    auto n=[&](const char* key,double lo,double hi){const auto v=p[key];require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=lo && v.toDouble()<=hi,"Geometry fit parameter outside bounds");};
    n("fitMinPoints",6,128);require(p["fitMinPoints"].toDouble()==std::floor(p["fitMinPoints"].toDouble()),"Geometry point count must be integral");
    for(auto key:{"fitInlierDistance","fitMaxRms","fitMaxResidual","lineMinSpan"})n(key,1e-9,1e9);
    n("lineMinVarianceRatio",1.000000001,1e12);n("circleMinAngularSpan",1e-9,360);n("circleMaxCondition",1,1e12);
    require(p["fitRansac"].isBool(),"Geometry RANSAC must be Boolean");
    if(!legacy){
        require(p["toleranceEnabled"].isBool(),"Geometry tolerance must be Boolean");
        for(auto key:{"nominal","lower","upper"})n(key,-1e9,1e9);
        require(p["lower"].toDouble()<=p["nominal"].toDouble() && p["nominal"].toDouble()<=p["upper"].toDouble(),"Geometry requires lower <= nominal <= upper");
        require(!p["toleranceEnabled"].toBool() || p["operation"]!="line_intersection","Intersection has no scalar tolerance");
    }
}
QVector<PortDefinition> geometryMeasurementInputs(){
    QVector<PortDefinition> p{{"image",PortType::Image,"px","image"}};
    for(auto id:{"p1","p2"})p.append({id,PortType::Point,"px","image",false});
    for(auto id:{"line1","line2"})p.append({id,PortType::Line2D,"px","image",false});
    for(auto id:{"circle1","circle2"})p.append({id,PortType::Circle2D,"px","image",false});
    for(auto id:{"samples1","samples2"})p.append({id,PortType::PointSet,"px","image",false});
    return p;
}
QVector<PortDefinition> geometryMeasurementOutputs(){return {
    {"valuePx",PortType::Number,"px","image"},{"valueMm",PortType::Number,"mm","plane"},
    {"anglePx",PortType::Number,"deg","image"},{"angleMm",PortType::Number,"deg","plane"},
    {"intersectionPx",PortType::Point,"px","image"},{"intersectionMm",PortType::Point,"mm","plane"},
    {"accepted",PortType::Boolean,"","plane"},{"status",PortType::Text,"","image"}};}
QStringList geometryMeasurementRequiredInputs(const QJsonObject& p){
    validateGeometryMeasurement(p);const auto op=p["operation"].toString();QStringList ports{"image"};
    if(op=="point_distance" || op=="x_offset" || op=="y_offset")ports<<"p1"<<"p2";
    else if(op=="point_line_distance" || op=="point_segment_distance")ports<<"p1"<<"line1";
    else if(op=="line_angle" || op=="line_intersection" || op=="parallel_distance")ports<<"line1"<<"line2";
    else {ports<<"circle1";if(op=="circle_centre_distance")ports<<"circle2";}
    if(p["mode"]=="calibrated")for(auto index:{QString("1"),QString("2")})
        if(ports.contains("line"+index) || ports.contains("circle"+index))ports<<"samples"+index;
    return ports;
}
}
