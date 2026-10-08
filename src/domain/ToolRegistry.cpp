#include "ToolRegistry.hpp"
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
QString builtinLabel(const QString& k) {if(k=="measurement")return "Measurement";if(k=="webcam")return "Webcam";if(k=="image_file")return "Image File";if(k=="template_match")return "Template Match";if(k=="blob")return "Blob";if(k=="edge")return "Edge";return "Calculate";}
QJsonObject builtinDefaults(const QString& k) {
 if(k=="measurement")return {{"lowerMm",0.0},{"upperMm",100.0}};
 if(k=="webcam")return {{"deviceIndex",0},{"backend","qt"},{"timeoutMs",5000}};
 if(k=="image_file")return {{"path",""}};
 if(k=="template_match")return {{"minScore",0.7},{"modelRoi",QJsonObject{}},{"searchRoi",QJsonObject{}},{"modelPng",""}};
 if(k=="blob")return {{"threshold",128},{"searchRoi",QJsonObject{}},{"polarity","bright"},{"connectivity",8},{"minArea",1},{"maxArea",QJsonValue::Null}};
 if(k=="edge")return {{"direction","left_to_right"},{"minContrast",20},{"searchRoi",QJsonObject{}},{"referenceOrigin",QJsonObject{}},{"referenceGeometry",""}};
 return {{"operation","distance"}};
}
QVector<PortDefinition> builtinInputs(const QString& k) {
 if(k=="webcam")return {};
 if(k=="image_file")return {};if(k=="calculate" || k=="measurement")return {{"p1",PortType::Point,"px","image"},{"p2",PortType::Point,"px","image"}};
 if(k=="edge")return {{"image",PortType::Image,"px","image"},{"region",PortType::Region,"px","image",false},{"reference",PortType::Point,"px","image",false}};
 return {{"image",PortType::Image,"px","image"},{"region",PortType::Region,"px","image",false}};
}
QVector<PortDefinition> builtinOutputs(const QString& k) {
 if(k=="measurement")return {{"value",PortType::Number,"mm","plane"},{"pixels",PortType::Number,"px","image"},{"pass",PortType::Boolean,"","plane"}};
 if(k=="webcam")return {{"image",PortType::Image,"px","image"}};
 if(k=="image_file")return {{"image",PortType::Image,"px","image"}};if(k=="calculate")return {{"value",PortType::Number,"px","image"}};
 if(k=="edge")return {{"point",PortType::Point,"px","image"},{"strength",PortType::Number,"gray8","image"},{"found",PortType::Boolean,"","image"}};
 if(k=="blob")return {{"candidateCount",PortType::Number,"count","image"},{"region",PortType::Region,"px","image"},{"point",PortType::Point,"px","image"},{"area",PortType::Number,"px2","image"},{"count",PortType::Number,"count","image"},{"found",PortType::Boolean,"","image"}};
 return {{"region",PortType::Region,"px","image"},{"point",PortType::Point,"px","image"},{"score",PortType::Number,"ratio","image"},{"found",PortType::Boolean,"","image"}};
}
void builtinValidation(const ToolDefinition& t) {
 auto p=t.parameters;
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
    QMutexLocker lock(&registryMutex);extensions.append(std::move(descriptor));
}
QVector<ToolDescriptor> ToolRegistry::entries() {
 static const QVector<ToolDescriptor> builtins=[] {QVector<ToolDescriptor> result;for(const QString kind:{"image_file","template_match","blob","edge","calculate","webcam","measurement"})result.append({kind,builtinLabel(kind),(kind=="blob" || kind=="measurement") ? 2 : 1,builtinDefaults(kind),builtinInputs(kind),builtinOutputs(kind),builtinValidation,kind=="image_file" || kind=="webcam"});return result;}();
    QMutexLocker lock(&registryMutex);auto result=builtins;result+=extensions;return result;
}
ToolDescriptor ToolRegistry::get(const QString& kind) {for(const auto& e:entries())if(e.kind==kind)return e;throw std::runtime_error(("Desteklenmeyen tool türü: "+kind).toUtf8().constData());}
void ToolRegistry::validate(const ToolDefinition& t) {auto e=get(t.kind);QSet<QString> allowed;for(auto it=e.defaults.begin();it!=e.defaults.end();++it)allowed.insert(it.key());keys(t.parameters,allowed);require(t.parameters.size()==e.defaults.size(),"Tool parametreleri eksik.");e.validate(t);}
}
