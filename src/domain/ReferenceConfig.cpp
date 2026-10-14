#include "ReferenceConfig.hpp"
#include "PoseTrackingConfig.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}}
bool supportsReference(const QString& k){return k=="edge" || supportsPoseTracking(k);}
bool referenceSourcePort(const QString& k,const QString& p){
    if(k=="pattern_pose" || k=="shape_pose")return p=="pose";
    if(k=="template_match" || k=="edge" || k=="edge_point" || k=="blob")return p=="point";
    if(k=="edge_pair")return p=="centre" || p=="first" || p=="second";
    if(k=="circle_fit")return p=="centrePx";
    return k=="line_fit" && p=="originPx";
}
QString referenceGeometryIdentity(const ToolDefinition& t,const QString& port){
    require(referenceSourcePort(t.kind,port),"Unsupported reference source port");
    QJsonObject geometry;
    for(const auto key:{"modelRoi","modelPng","anchor","scanRoi","searchRoi","direction","poseTracking","referenceBinding"})
        if(t.parameters.contains(key))geometry[key]=t.parameters[key];
    // Threshold/score/fit quality settings affect current cache, not teaching geometry.
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject{{"kind",t.kind},{"port",port},{"geometry",geometry},{"dependencies",encodeToolDependencies(t.dependencies)}}).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
ToolRoi referenceNominalRoi(const ToolDefinition& t,const CoordinateFrame& f){
    if(t.kind!="edge")return nominalTrackingRoi(t,f);
    auto proxy=t;proxy.kind="blob";return nominalTrackingRoi(proxy,f);
}
QString referenceInput(const ReferenceBinding& b){return b.mode==ReferenceMode::Rigid ? "poseReference":"reference";}
ReferenceBinding toolReferenceBinding(const ToolDefinition& t){
    require(supportsReference(t.kind),"Unsupported reference consumer");
    require(t.parameters["referenceBinding"].isObject(),"Reference binding must be an object");
    require(!t.parameters.contains("poseTracking"),"Duplicate reference bindings");
    const auto b=ReferenceBinding::fromJson(t.parameters["referenceBinding"].toObject());
    if(b.world){
        require(t.kind=="edge_point" || t.kind=="edge_pair" || t.kind=="line_fit" || t.kind=="circle_fit","World reference consumer unsupported");
        require(t.kind=="edge_point" || t.parameters["mode"]=="calibrated","World reference requires calibrated mode");
        const auto& g=b.nominal.geometry;
        require((t.kind=="edge_point" || t.kind=="edge_pair") ? std::holds_alternative<ScanSegment>(g):t.kind=="line_fit" ? std::holds_alternative<RotatedRectangle>(g):(std::holds_alternative<Annulus>(g) || std::holds_alternative<ArcRoi>(g)),"World reference ROI type mismatch");
        if(t.kind=="circle_fit")require(t.parameters["unitsPerSample"]==b.mmPerSample,"World reference sampling unit changed; reteach");
    }else {
        require(!b.calibration,"Raw reference cannot carry a world calibration dependency");
        require(!t.parameters.contains("mode") || t.parameters["mode"]=="pixel","Raw reference requires pixel mode");
    }
    require(referenceNominalRoi(t,b.nominal.frame).toJson()==b.nominal.toJson(),"Nominal ROI changed; reteach reference");
    return b;
}
}
