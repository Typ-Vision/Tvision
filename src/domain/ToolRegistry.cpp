#include "PoseMatchConfig.hpp"
#include "PoseTrackingConfig.hpp"
#include "CircleConfig.hpp"
#include "CircleConfig.hpp"
#include "ToolRegistry.hpp"
#include "IndustrialSourceReference.hpp"
#include "ReferenceConfig.hpp"
#include "PreprocessConfig.hpp"
#include "CodeReadConfig.hpp"
#include "EdgePointConfig.hpp"
#include "MetrologyConfig.hpp"
#include "GeometryMeasurementConfig.hpp"
#include <QJsonArray>
#include <QSet>
#include <QStringList>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <QMutex>
#include <QMutexLocker>
namespace visionflow {
namespace {
void require(bool ok,const QString& message) {if(!ok)throw std::runtime_error(message.toUtf8().constData());}
void keys(const QJsonObject& o,const QSet<QString>& allowed) {for(auto it=o.begin();it!=o.end();++it)require(allowed.contains(it.key()),"Desteklenmeyen editor alanı: "+it.key());}
void roi(const QJsonValue& v) {
 require(v.isObject(),"ROI nesne olmalı.");auto o=v.toObject();keys(o,{"x","y","width","height"});if(o.isEmpty())return;
 require(o.size()==4,"ROI alanları eksik.");for(const auto key:{"x","y","width","height"})require(o[key].isDouble() && o[key].toDouble()==o[key].toInt() && o[key].toDouble()>=0 && o[key].toDouble()<=100000,"ROI koordinatı geçersiz.");
 require(o["width"].toInt()>0 && o["height"].toInt()>0,"ROI boyutu pozitif olmalı.");
}
}
QString builtinLabel(const QString& k) {if(k=="industrial_camera")return "Industrial Camera";if(k=="shape_pose")return "Shape Pose";if(k=="pattern_pose")return "Pattern Pose";if(k=="geometry_measurement")return "Geometry Measurement";if(k=="circle_fit")return "Circle / Arc";if(k=="circle_fit")return "Circle / Arc";if(k=="edge_pair")return "Edge Pair / Caliper";if(k=="line_fit")return "Line Fit";if(k=="edge_point")return "Edge Point";if(k=="preprocess")return "Preprocess";if(k=="code_read")return "Code Read";if(k=="measurement")return "Measurement";if(k=="webcam")return "Webcam";if(k=="image_file")return "Image File";if(k=="template_match")return "Template Match";if(k=="blob")return "Blob";if(k=="edge")return "Edge";return "Calculate";}
QJsonObject builtinDefaults(const QString& k) {
 if(k=="industrial_camera")return {{"sourceReference",QJsonValue::Null}};
 if(k=="pattern_pose" || k=="shape_pose")return poseMatchDefaults();
 if(k=="geometry_measurement")return geometryMeasurementDefaults();
 if(k=="circle_fit")return circleDefaults();
 if(k=="circle_fit")return circleDefaults();
 if(isMetrology(k))return metrologyDefaults(k);
 if(k=="edge_point")return edgePointDefaults();
 if(k=="preprocess")return {{"channel","gray"},{"steps",QJsonArray{}}};
 if(k=="code_read")return codeReadDefaults();
 if(k=="measurement")return {{"lowerMm",0.0},{"upperMm",100.0}};
 if(k=="webcam")return {{"deviceIndex",0},{"backend","qt"},{"timeoutMs",5000}};
 if(k=="image_file")return {{"path",""}};
 if(k=="template_match")return {{"minScore",0.7},{"modelRoi",QJsonObject{}},{"searchRoi",QJsonObject{}},{"modelPng",""}};
 if(k=="blob")return {{"rejectBorderTouching",false},{"threshold",128},{"searchRoi",QJsonObject{}},{"polarity","bright"},{"connectivity",8},{"minArea",1},{"maxArea",QJsonValue::Null}};
 if(k=="edge")return {{"direction","left_to_right"},{"minContrast",20},{"searchRoi",QJsonObject{}},{"referenceOrigin",QJsonObject{}},{"referenceGeometry",""}};
 return {{"operation","distance"}};
}
QVector<PortDefinition> builtinInputs(const QString& k) {
 if(k=="industrial_camera")return {};
 if(k=="pattern_pose" || k=="shape_pose")return {{"image",PortType::Image,"px","image"}};
 if(k=="geometry_measurement")return geometryMeasurementInputs();
 if(k=="blob")return {{"image",PortType::Image,"px","image"},{"region",PortType::Region,"px","image",false},{"reference",PortType::Point,"px","image",false},{"poseReference",PortType::Pose2D,"px","image",false}};
 if(supportsPoseTracking(k))return {{"image",PortType::Image,"px","image"},{"reference",PortType::Point,"px","image",false},{"poseReference",PortType::Pose2D,"px","image",false}};
 if(k=="preprocess" || k=="code_read")return {{"image",PortType::Image,"px","image"}};
 if(k=="webcam")return {};
 if(k=="image_file")return {};if(k=="calculate" || k=="measurement")return {{"p1",PortType::Point,"px","image"},{"p2",PortType::Point,"px","image"}};
 if(k=="edge")return {{"image",PortType::Image,"px","image"},{"region",PortType::Region,"px","image",false},{"reference",PortType::Point,"px","image",false},{"poseReference",PortType::Pose2D,"px","image",false}};
 return {{"image",PortType::Image,"px","image"},{"region",PortType::Region,"px","image",false}};
}
QVector<PortDefinition> builtinOutputs(const QString& k) {
 if(k=="industrial_camera")return {{"image",PortType::Image,"px","image"}};
 if(k=="pattern_pose" || k=="shape_pose")return {{"pose",PortType::Pose2D,"px","image"},{"scale",PortType::Number,"ratio","image"},{"score",PortType::Number,"ratio","image"},{"found",PortType::Boolean,"","image"},{"ambiguous",PortType::Boolean,"","image"},{"truncated",PortType::Boolean,"","image"},{"candidates",PortType::Text,"","image"}};
 if(k=="geometry_measurement")return geometryMeasurementOutputs();
 if(k=="edge_pair")return {{"first",PortType::Point,"px","image"},{"second",PortType::Point,"px","image"},{"centre",PortType::Point,"px","image"},{"widthPx",PortType::Number,"px","image"},{"widthMm",PortType::Number,"mm","plane"},{"found",PortType::Boolean,"","image"},{"accepted",PortType::Boolean,"","plane"}};
 if(k=="circle_fit"){
  QVector<PortDefinition> out{{"found",PortType::Boolean,"","image"},{"qualified",PortType::Boolean,"","plane"},{"accepted",PortType::Boolean,"","plane"},{"diameterAccepted",PortType::Boolean,"","plane"},{"deviationAccepted",PortType::Boolean,"","plane"},{"scope",PortType::Text,"","plane"},{"quality",PortType::InspectionResults,"","plane"},{"slots",PortType::InspectionResults,"","plane"},{"scans",PortType::InspectionResults,"","plane"},{"coverage",PortType::Number,"ratio","image"},{"maxGapDegrees",PortType::Number,"deg","image"},{"circularityIndex",PortType::Number,"ratio","plane"}};
  for(const auto suffix:{QString("Px"),QString("Mm")}){const bool px=suffix=="Px";const QString u=px ? "px":"mm",f=px ? "image":"plane";out.append({"observations"+suffix,PortType::PointSet,u,f});out.append({"circle"+suffix,PortType::Circle2D,u,f});out.append({"arc"+suffix,PortType::Arc2D,u,f});out.append({"centre"+suffix,PortType::Point,u,f});for(auto k:{"diameter","radius","fitRms","fitMaxResidual","radialMin","radialMax","maxInward","maxOutward","observedDeviation","deviation"})out.append({QString(k)+suffix,PortType::Number,u,f});}return out;
 }
 if(k=="circle_fit"){
  QVector<PortDefinition> out{{"found",PortType::Boolean,"","image"},{"qualified",PortType::Boolean,"","plane"},{"accepted",PortType::Boolean,"","plane"},{"diameterAccepted",PortType::Boolean,"","plane"},{"deviationAccepted",PortType::Boolean,"","plane"},{"scope",PortType::Text,"","plane"},{"quality",PortType::InspectionResults,"","plane"},{"slots",PortType::InspectionResults,"","plane"},{"scans",PortType::InspectionResults,"","plane"},{"coverage",PortType::Number,"ratio","image"},{"maxGapDegrees",PortType::Number,"deg","image"},{"circularityIndex",PortType::Number,"ratio","plane"}};
  for(const auto suffix:{QString("Px"),QString("Mm")}){const bool px=suffix=="Px";const QString u=px ? "px":"mm",f=px ? "image":"plane";out.append({"observations"+suffix,PortType::PointSet,u,f});out.append({"circle"+suffix,PortType::Circle2D,u,f});out.append({"arc"+suffix,PortType::Arc2D,u,f});out.append({"centre"+suffix,PortType::Point,u,f});for(auto k:{"diameter","radius","fitRms","fitMaxResidual","radialMin","radialMax","maxInward","maxOutward","observedDeviation","deviation"})out.append({QString(k)+suffix,PortType::Number,u,f});}return out;
 }
 if(k=="line_fit"){
  QVector<PortDefinition> out{{"found",PortType::Boolean,"","image"},{"accepted",PortType::Boolean,"","plane"},{"coverage",PortType::Number,"ratio","image"},{"spanCoverage",PortType::Number,"ratio","image"},{"usedCount",PortType::Number,"count","image"},{"outlierCount",PortType::Number,"count","image"},{"missingCount",PortType::Number,"count","image"},{"scans",PortType::InspectionResults,"","image"}};
  out.append({"observationsPx",PortType::PointSet,"px","image"});
  for(const auto suffix:{QString("Px"),QString("Mm")}){const bool px=suffix=="Px";const QString unit=px ? "px":"mm",frame=px ? "image":"plane";
   out.append({"line"+suffix,PortType::Line2D,unit,frame});out.append({"origin"+suffix,PortType::Point,unit,frame});out.append({"tangent"+suffix,PortType::Point,"unit-vector",frame});
   for(const auto key:{"angle","span","rms","maxResidual"})out.append({QString(key)+suffix,PortType::Number,QString(key)=="angle" ? "deg":unit,frame});
  }return out;
 }

 if(k=="edge_point")return {{"point",PortType::Point,"px","image"},{"strength",PortType::Number,"gray8/px","image"},{"signedStrength",PortType::Number,"gray8/px","image"},{"found",PortType::Boolean,"","image"},{"refined",PortType::Boolean,"","image"},{"pointMm",PortType::Point,"mm","plane"}};
 if(k=="preprocess")return {{"image",PortType::Image,"px","image"}};
 if(k=="code_read")return {{"codes",PortType::CodeResults,"px","image"},{"count",PortType::Number,"count","image"},{"found",PortType::Boolean,"","image"},{"accepted",PortType::Boolean,"","image"},{"checks",PortType::InspectionResults,"","image"}};
 if(k=="measurement")return {{"value",PortType::Number,"mm","plane"},{"pixels",PortType::Number,"px","image"},{"pass",PortType::Boolean,"","plane"}};
 if(k=="webcam")return {{"image",PortType::Image,"px","image"}};
 if(k=="image_file")return {{"image",PortType::Image,"px","image"}};if(k=="calculate")return {{"value",PortType::Number,"px","image"}};
 if(k=="edge")return {{"point",PortType::Point,"px","image"},{"strength",PortType::Number,"gray8","image"},{"found",PortType::Boolean,"","image"}};
 if(k=="blob")return {{"candidateCount",PortType::Number,"count","image"},{"region",PortType::Region,"px","image"},{"point",PortType::Point,"px","image"},{"area",PortType::Number,"px2","image"},{"count",PortType::Number,"count","image"},{"found",PortType::Boolean,"","image"}};
 return {{"region",PortType::Region,"px","image"},{"point",PortType::Point,"px","image"},{"score",PortType::Number,"ratio","image"},{"found",PortType::Boolean,"","image"}};
}
void builtinValidation(const ToolDefinition& t) {
 if(t.kind=="industrial_camera"){
  const auto reference=t.parameters["sourceReference"];
  require(reference.isNull() || reference.isObject(),"Industrial source reference must be null (draft) or an object");
  if(reference.isObject())IndustrialSourceReference::fromJson(reference.toObject());
  require(t.dependencies.isEmpty(),"Industrial source backend is resolved locally, not from tool dependencies");
  return;
 }
 if(t.kind=="pattern_pose" || t.kind=="shape_pose"){validatePoseMatch(t.parameters);return;}
 if(t.kind=="geometry_measurement"){validateGeometryMeasurement(t.parameters);return;}
 if(t.kind=="circle_fit"){validateCircle(t.parameters);return;}
 if(t.kind=="circle_fit"){validateCircle(t.parameters);return;}
 if(isMetrology(t.kind)){validateMetrology(t.kind,t.parameters);return;}
 if(t.kind=="edge_point"){validateEdgePoint(t.parameters);return;}
 auto p=t.parameters;
 if(t.kind=="preprocess"){validatePreprocess(p);return;}
 if(t.kind=="code_read"){validateCodeRead(p);return;}
 if(t.kind=="webcam") {
  auto index=p["deviceIndex"],timeout=p["timeoutMs"];
  require(index.isDouble() && index.toDouble()==index.toInt() && index.toInt()>=0 && index.toInt()<=63,"Webcam index 0–63 olmalı.");
  require(timeout.isDouble() && timeout.toDouble()==timeout.toInt() && timeout.toInt()>=1 && timeout.toInt()<=60000,"Webcam timeout 1–60000 ms olmalı.");
  require(p["backend"].isString() && QStringList{"qt"}.contains(p["backend"].toString()),"Webcam backend qt olmalı.");
 } else if(t.kind=="measurement") {
  const auto lower=p["lowerMm"],upper=p["upperMm"];require(lower.isDouble() && upper.isDouble() && std::isfinite(lower.toDouble()) && std::isfinite(upper.toDouble()) && lower.toDouble()>=0 && upper.toDouble()>=lower.toDouble(),"Tolerans 0 ≤ alt ≤ üst ve sonlu mm olmalı.");
 } else if(t.kind=="image_file")require(p["path"].isString() && p["path"].toString().size()<=4096,"Görüntü yolu hatalı.");
 else if(t.kind=="template_match") {auto v=p["minScore"];require(v.isDouble() && v.toDouble()>=0 && v.toDouble()<=1,"Min Score 0–1 aralığında olmalı.");roi(p["modelRoi"]);roi(p["searchRoi"]);require(p["modelPng"].isString() && p["modelPng"].toString().size()<=4*1024*1024,"Model PNG sınırı 4 MB base64.");}
 else if(t.kind=="blob") {auto v=p["threshold"];require(v.isDouble() && v.toDouble()>=0 && v.toDouble()<=255 && v.toDouble()==v.toInt(),"Threshold 0–255 tamsayı olmalı.");roi(p["searchRoi"]);
  require(p["rejectBorderTouching"].isBool(),"Blob sınır filtresi boolean olmalı.");
  require(p["polarity"]=="bright" || p["polarity"]=="dark","Blob polarity bright/dark olmalı.");
  require(p["connectivity"]==4 || p["connectivity"]==8,"Blob connectivity 4/8 olmalı.");
  auto area=[](const QJsonValue& v){return v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=1 && v.toDouble()<=2147483647 && std::floor(v.toDouble())==v.toDouble();};
  require(area(p["minArea"]),"Min Area pozitif tamsayı px² olmalı.");
  require(p["maxArea"].isNull() || (area(p["maxArea"]) && p["maxArea"].toDouble()>=p["minArea"].toDouble()),"Max Area ≥ Min Area olan pozitif tamsayı px² veya sınırsız olmalı.");
 }
 else if(t.kind=="edge") {
  auto v=p["minContrast"];require(v.isDouble() && v.toDouble()>=1 && v.toDouble()<=255,"Min Contrast 1–255 olmalı.");roi(p["searchRoi"]);
  require(p["direction"].isString() && QStringList{"left_to_right","right_to_left","top_to_bottom","bottom_to_top"}.contains(p["direction"].toString()),"Edge yönü hatalı.");
  auto identity=p["referenceGeometry"].toString();require(p["referenceGeometry"].isString() && (identity.isEmpty() || (identity.size()==64 && std::all_of(identity.begin(),identity.end(),[](QChar c){return (c>='0' && c<='9') || (c>='a' && c<='f');}))),"Referans geometri kimliği geçersiz.");
  require(p["referenceOrigin"].isObject(),"Referans noktası nesne olmalı.");auto o=p["referenceOrigin"].toObject();keys(o,{"x","y"});if(!o.isEmpty()) {require(o.size()==2,"Referans koordinatları eksik.");for(const auto key:{"x","y"})require(o[key].isDouble() && std::isfinite(o[key].toDouble()) && o[key].toDouble()>=0 && o[key].toDouble()<=100000,"Referans koordinatı geçersiz.");}
 } else require(p["operation"].isString() && p["operation"].toString()=="distance","Calculate işlemi hatalı.");
}
namespace { QMutex registryMutex; QVector<ToolDescriptor> extensions; }
void ToolRegistry::registerType(ToolDescriptor descriptor) {
    for(const auto& entry:entries())if(entry.kind==descriptor.kind)throw std::runtime_error("Tool türü zaten kayıtlı.");
    if(descriptor.kind.isEmpty() || !descriptor.validate || descriptor.version<1)throw std::runtime_error("Geçersiz tool kaydı.");
    if(descriptor.minimumEditorSchema<6 || descriptor.minimumEditorSchema>7)throw std::runtime_error("Unsupported registry editor schema");
    if(!descriptor.requiredBackend.isEmpty() && descriptor.minimumEditorSchema<7)throw std::runtime_error("Backend dependency requires schema 7");
    for(const auto& ports:{descriptor.inputs,descriptor.outputs}) {
        QSet<QString> ids;
        for(const auto& p:ports) {
            if(p.id.isEmpty() || p.id.size()>128 || ids.contains(p.id) || portTypeName(p.type).isEmpty() || p.unit.size()>32 || p.frame.isEmpty() || p.frame.size()>128)throw std::runtime_error("Invalid registry port");
            if(int(p.type)>int(PortType::Boolean) && descriptor.minimumEditorSchema<7)throw std::runtime_error("Typed ports require editor schema 7");
            ids.insert(p.id);
        }
    }
    QMutexLocker lock(&registryMutex);extensions.append(std::move(descriptor));
}
QVector<ToolDescriptor> ToolRegistry::entries() {
 static const QVector<ToolDescriptor> builtins=[] {QVector<ToolDescriptor> result;for(const QString kind:{"image_file","template_match","blob","edge","calculate","webcam","measurement"})result.append({kind,builtinLabel(kind),kind=="blob" ? 4 : kind=="measurement" ? 2 : 1,builtinDefaults(kind),builtinInputs(kind),builtinOutputs(kind),builtinValidation,kind=="image_file" || kind=="webcam",kind=="measurement"});result.append({"preprocess",builtinLabel("preprocess"),1,builtinDefaults("preprocess"),builtinInputs("preprocess"),builtinOutputs("preprocess"),builtinValidation,false,false,{},7});result.append({"code_read",builtinLabel("code_read"),2,builtinDefaults("code_read"),builtinInputs("code_read"),builtinOutputs("code_read"),builtinValidation,false,false,"zxing-reader",7});result.append({"edge_point",builtinLabel("edge_point"),3,builtinDefaults("edge_point"),builtinInputs("edge_point"),builtinOutputs("edge_point"),builtinValidation,false,false,{},7});result.append({"geometry_measurement",builtinLabel("geometry_measurement"),2,builtinDefaults("geometry_measurement"),builtinInputs("geometry_measurement"),builtinOutputs("geometry_measurement"),builtinValidation,false,false,{},7});for(const QString k:{"edge_pair","line_fit","circle_fit"})result.append({k,builtinLabel(k),(k=="line_fit" || k=="circle_fit") ? 3:2,builtinDefaults(k),builtinInputs(k),builtinOutputs(k),builtinValidation,false,false,{},7});result.append({"pattern_pose",builtinLabel("pattern_pose"),1,builtinDefaults("pattern_pose"),builtinInputs("pattern_pose"),builtinOutputs("pattern_pose"),builtinValidation,false,false,{},7});result.append({"shape_pose",builtinLabel("shape_pose"),1,builtinDefaults("shape_pose"),builtinInputs("shape_pose"),builtinOutputs("shape_pose"),builtinValidation,false,false,{},7});result.append({"industrial_camera",builtinLabel("industrial_camera"),1,builtinDefaults("industrial_camera"),builtinInputs("industrial_camera"),builtinOutputs("industrial_camera"),builtinValidation,true,false,{},9});return result;}();
    QMutexLocker lock(&registryMutex);auto result=builtins;result+=extensions;return result;
}
ToolDescriptor ToolRegistry::get(const QString& kind) {for(const auto& e:entries())if(e.kind==kind)return e;throw std::runtime_error(("Desteklenmeyen tool türü: "+kind).toUtf8().constData());}
void ToolRegistry::validate(const ToolDefinition& t) {
 auto e=get(t.kind);encodeToolDependencies(t.dependencies);auto base=t;
 if(t.kind=="blob" && !base.parameters.contains("rejectBorderTouching"))base.parameters["rejectBorderTouching"]=false;
 if(t.parameters.contains("referenceBinding")){if(toolReferenceBinding(t).world)base.parameters["scanRoi"]=QJsonObject{};base.parameters.remove("referenceBinding");}
 if(t.parameters.contains("poseTracking")){
     if(isWorldPoseTracking(t)){worldPoseTracking(t);base.parameters["scanRoi"]=QJsonObject{};}
     else poseTrackingBinding(t);
     base.parameters.remove("poseTracking");
 }
 QSet<QString> allowed;for(auto it=e.defaults.begin();it!=e.defaults.end();++it)allowed.insert(it.key());
 if(t.kind=="circle_fit")allowed.insert("worldCalibration");
 keys(base.parameters,allowed);require(t.kind=="geometry_measurement" || base.parameters.size()==e.defaults.size()+((t.kind=="circle_fit" && base.parameters.contains("worldCalibration")) ? 1:0),"Tool parametreleri eksik.");e.validate(base);
}
}
