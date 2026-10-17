#include "MeasurementService.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double length(const QPointF& a,const QPointF& b){return std::hypot(a.x()-b.x(),a.y()-b.y());}
void point(const Point2D& p,const CalibrationDependency& d) {
    require(p.frame==d.source,"Point coordinate frame/unit/axes mismatch");
    require(std::isfinite(p.position.x()) && std::isfinite(p.position.y()) && p.position.x()>=0 && p.position.y()>=0 && p.position.x()<=d.resolution.width()-1 && p.position.y()<=d.resolution.height()-1,"Point outside raw calibration resolution");
}
double sampleLength(const CalibrationSample& s,const CalibrationDependency& d) {
    require(!s.id.trimmed().isEmpty() && s.id.size()<=128 && std::isfinite(s.knownMillimetres) && s.knownMillimetres>0,"Invalid calibration sample identity/known length");
    point(s.first,d);point(s.second,d);double n=length(s.first.position,s.second.position);require(std::isfinite(n) && n>0,"Degenerate calibration sample");return n;
}
QJsonObject sampleJson(const CalibrationSample& s){return {{"id",s.id},{"x1",s.first.position.x()},{"y1",s.first.position.y()},{"x2",s.second.position.x()},{"y2",s.second.position.y()},{"knownMm",s.knownMillimetres}};}
bool sameSegment(const CalibrationSample& a,const CalibrationSample& b){return (a.first.position==b.first.position && a.second.position==b.second.position) || (a.first.position==b.second.position && a.second.position==b.first.position);}
}
double PlanarCalibration::millimetresPerPixel() const {if(mapping_)throw std::runtime_error("Homography has no uniform mm/px scale");return scale_;}
double PlanarCalibration::pixelsPerMillimetre() const {require(manualProvenance_.has_value(),"Not a manual scale model");return pixelsPerMillimetre_;}
void MeasurementService::identify(PlanarCalibration& c) {
    if(c.mapping_ || c.manualProvenance_){auto j=MeasurementService{}.encode(c);auto d=c.dependency_.toJson();d.remove("contentId");j["dependency"]=d;c.dependency_.contentId=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(j).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());return;}
    auto setup=c.dependency_.toJson();setup.remove("contentId");QJsonArray holdout;for(const auto& s:c.holdout_)holdout.append(sampleJson(s));
    const QJsonObject object{{"model","uniform_planar_scale/1"},{"setup",setup},{"mmPerPx",c.scale_},{"teaching",sampleJson(c.teaching_)},{"holdout",holdout},{"validated",c.validated_},{"maxErrorMm",c.maxError_},{"rmsMm",c.rms_}};
    c.dependency_.contentId=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(object).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
PlanarCalibration MeasurementService::teach(CalibrationDependency setup,const CalibrationSample& sample) const {
    setup.contentId="pending";setup.validate();require(setup.modelType=="uniform_planar_scale","Scale teaching requires scale model");
    require(setup.source.unit==LengthUnit::Pixel && setup.source.axes==AxisConvention::XRightYDown && setup.target.unit==LengthUnit::Millimetre,"Uniform scale requires image pixels to millimetres");
    PlanarCalibration c;c.dependency_=std::move(setup);c.teaching_=sample;c.scale_=sample.knownMillimetres/sampleLength(sample,c.dependency_);
    require(std::isfinite(c.scale_) && c.scale_>0,"Invalid scale");identify(c);return c;
}
PlanarCalibration MeasurementService::manualScale(CalibrationDependency setup,double pxPerMm,const ManualScaleProvenance& provenance) const {
    setup.modelType="manual_planar_scale";setup.contentId="pending";setup.validate();
    require(setup.source.unit==LengthUnit::Pixel && setup.source.axes==AxisConvention::XRightYDown && setup.target.unit==LengthUnit::Millimetre,"Manual scale requires image pixels to millimetres");
    require(std::isfinite(pxPerMm) && pxPerMm>0,"px/mm must be finite and positive");
    const double inverse=1/pxPerMm;require(std::isfinite(inverse) && inverse>0,"mm/px reciprocal must be finite and positive");
    require((provenance.origin=="acquired" || provenance.origin=="file_test") && provenance.sourceId==setup.cameraId && !provenance.frameId.trimmed().isEmpty() && provenance.resolution==setup.resolution,"Manual evidence provenance/source/frame/resolution mismatch");
    require((provenance.origin=="file_test")==setup.cameraId.startsWith("calibration-test:"),"Manual evidence test source/origin mismatch");
    PlanarCalibration c;c.dependency_=std::move(setup);c.scale_=inverse;c.pixelsPerMillimetre_=pxPerMm;c.manualProvenance_=provenance;identify(c);return c;
}
double MeasurementService::trialDistance(const PlanarCalibration& c,const Point2D& a,const Point2D& b) const {
    require(c.dependency_.modelType=="manual_planar_scale","Trial requires manual scale");point(a,c.dependency_);point(b,c.dependency_);
    const double px=length(a.position,b.position),mm=px/c.pixelsPerMillimetre_;require(std::isfinite(mm) && (px==0 || mm>0),"Trial distance overflow/underflow");return mm;
}
PlanarCalibration MeasurementService::teachHomography(CalibrationDependency setup,const PlaneMapping& mapping) const {
    setup.modelType="raw_planar_homography";setup.modelVersion=1;setup.contentId="pending";setup.target.axes=AxisConvention::XRightYDown;setup.validate();
    require(setup.source==CoordinateFrame{mapping.observation().source.coordinateSystemId,LengthUnit::Pixel,AxisConvention::XRightYDown} && setup.target.unit==LengthUnit::Millimetre && setup.cameraId==mapping.observation().source.sourceId && setup.resolution==mapping.observation().resolution,"Mapping/source installation mismatch");
    PlanarCalibration c;c.dependency_=setup;c.mapping_=mapping;identify(c);return c;
}
PlanarCalibration MeasurementService::validate(const PlanarCalibration& candidate,const QVector<CalibrationSample>& holdout,double maxError) const {
    require(std::isfinite(maxError) && maxError>=0 && holdout.size()>=2 && holdout.size()<=256,"Validation requires 2–256 independent samples and finite nonnegative error bound");
    auto c=candidate;const bool teaching=c.dependency_.modelType=="uniform_planar_scale";QSet<QString> ids;if(teaching)ids.insert(c.teaching_.id);double squareSum=0;c.maxResidual_=0;c.measured_.clear();c.residuals_.clear();
    for(int i=0;i<holdout.size();++i) {
        const auto& s=holdout[i];require(!ids.contains(s.id) && (!teaching || !sameSegment(s,c.teaching_)),"Teaching/repeated sample cannot be holdout evidence");ids.insert(s.id);
        for(int j=0;j<i;++j)require(!sameSegment(s,holdout[j]),"Repeated holdout geometry");
        const double px=sampleLength(s,c.dependency_);double mm=c.manualProvenance_ ? px/c.pixelsPerMillimetre_ : px*c.scale_;
        if(c.mapping_){for(auto p:c.mapping_->observation().pixels)require(length(p,s.first.position)>0.5 && length(p,s.second.position)>0.5,"Fit corners cannot serve as independent holdout points");mm=PlaneMappingService::distance(*c.mapping_,s.first.position,s.second.position).millimetres;}
        require(std::isfinite(mm) && mm>0,"Validation distance overflow/underflow");
        const double residual=std::abs(mm-s.knownMillimetres);c.maxResidual_=std::max(c.maxResidual_,residual);
        c.measured_.append(mm);c.residuals_.append(residual);
        require(std::isfinite(residual) && residual<=maxError,"Holdout residual exceeds limit");squareSum+=residual*residual;
    }
    require(std::isfinite(squareSum),"Residual overflow");c.rms_=std::sqrt(squareSum/holdout.size());c.holdout_=holdout;c.maxError_=maxError;c.validated_=true;identify(c);return c;
}
QJsonObject MeasurementService::encode(const PlanarCalibration& c) const {
    require(c.readyRecordContentId_.isEmpty(),"Transient system binding must be persisted with its model-ready record, not legacy model codec");
    QJsonArray evidence;for(const auto& s:c.holdout_)evidence.append(sampleJson(s));
    if(c.manualProvenance_){const auto& p=*c.manualProvenance_;return {{"schemaVersion",3},{"model","manual_planar_scale/1"},{"dependency",c.dependency_.toJson()},{"pixelsPerMillimetre",c.pixelsPerMillimetre_},{"millimetresPerPixel",c.scale_},{"provenance",QJsonObject{{"origin",p.origin},{"sourceId",p.sourceId},{"frameId",p.frameId},{"width",p.resolution.width()},{"height",p.resolution.height()}}},{"holdout",evidence},{"validated",c.validated_},{"maxErrorMm",c.maxError_},{"rmsMm",c.rms_},{"maxResidualMm",c.maxResidual_}};}
    if(c.mapping_)return {{"schemaVersion",2},{"model","raw_planar_homography/1"},{"dependency",c.dependency_.toJson()},{"mapping",PlaneMappingService::encode(*c.mapping_)},{"lensPolicy","raw_assumed_negligible"},{"evidenceOrigin",c.dependency_.cameraId.startsWith("calibration-test:") ? "file_test" : "acquisition"},{"holdout",evidence},{"validated",c.validated_},{"maxErrorMm",c.maxError_},{"rmsMm",c.rms_},{"maxResidualMm",c.maxResidual_}};
    return {{"schemaVersion",1},{"model","uniform_planar_scale/1"},{"dependency",c.dependency_.toJson()},
        {"teaching",sampleJson(c.teaching_)},{"holdout",evidence},{"validated",c.validated_},
        {"maxErrorMm",c.maxError_},{"rmsMm",c.rms_},{"mmPerPx",c.scale_}};
}
PlanarCalibration MeasurementService::decode(const QJsonObject& o) const {
    const bool mapping=o["model"].toString()=="raw_planar_homography/1";
    const bool manual=o["model"].toString()=="manual_planar_scale/1";
    const QSet<QString> fields=manual ? QSet<QString>{"schemaVersion","model","dependency","pixelsPerMillimetre","millimetresPerPixel","provenance","holdout","validated","maxErrorMm","rmsMm","maxResidualMm"} : mapping ? QSet<QString>{"schemaVersion","model","dependency","mapping","lensPolicy","evidenceOrigin","holdout","validated","maxErrorMm","rmsMm","maxResidualMm"} : QSet<QString>{"schemaVersion","model","dependency","teaching","holdout","validated","maxErrorMm","rmsMm","mmPerPx"};
    require(o.size()==fields.size(),"Missing/unknown calibration fields");
    for(auto it=o.begin();it!=o.end();++it)require(fields.contains(it.key()),"Unknown calibration field");
    require(o["schemaVersion"].isDouble() && (manual ? o["schemaVersion"].toDouble()==3 : mapping ? o["schemaVersion"].toDouble()==2 : o["schemaVersion"].toDouble()==1 && o["model"].toString()=="uniform_planar_scale/1"),"Unsupported calibration schema/model");
    require(o["dependency"].isObject() && (manual ? o["provenance"].isObject() : mapping ? o["mapping"].isObject() : o["teaching"].isObject()) && o["holdout"].isArray() && o["validated"].isBool(),"Invalid calibration record types");
    const auto dependency=CalibrationDependency::fromJson(o["dependency"].toObject());
    require(dependency.modelType==(manual ? "manual_planar_scale" : mapping ? "raw_planar_homography" : "uniform_planar_scale"),"Record/dependency model mismatch");
    auto number=[](const QJsonObject& j,const char* key){require(j[key].isDouble() && std::isfinite(j[key].toDouble()),"Invalid calibration number");return j[key].toDouble();};
    auto readSample=[&](const QJsonValue& value){
        require(value.isObject(),"Invalid calibration sample");const auto j=value.toObject();
        const QSet<QString> keys{"id","x1","y1","x2","y2","knownMm"};require(j.size()==keys.size(),"Missing/unknown sample fields");
        for(auto it=j.begin();it!=j.end();++it)require(keys.contains(it.key()),"Unknown sample field");
        require(j["id"].isString(),"Invalid sample id");
        return CalibrationSample{j["id"].toString(),{{number(j,"x1"),number(j,"y1")},dependency.source},{{number(j,"x2"),number(j,"y2")},dependency.source},number(j,"knownMm")};
    };
    auto readManual=[&]{const auto p=o["provenance"].toObject();const QSet<QString> keys{"origin","sourceId","frameId","width","height"};require(p.size()==keys.size(),"Missing/unknown provenance fields");for(auto it=p.begin();it!=p.end();++it)require(keys.contains(it.key()),"Unknown provenance field");
        for(const auto* key:{"origin","sourceId","frameId"})require(p[key].isString(),"Invalid provenance text");
        for(const auto* key:{"width","height"})require(p[key].isDouble() && p[key].toDouble()==p[key].toInt() && p[key].toInt()>0,"Invalid provenance resolution");
        return manualScale(dependency,number(o,"pixelsPerMillimetre"),{p["origin"].toString(),p["sourceId"].toString(),p["frameId"].toString(),{p["width"].toInt(),p["height"].toInt()}});};
    auto c=manual ? readManual() : mapping ? teachHomography(dependency,PlaneMappingService::decode(o["mapping"].toObject())) : teach(dependency,readSample(o["teaching"]));const auto evidence=o["holdout"].toArray();
    require(evidence.size()<=256,"Too many calibration samples");QVector<CalibrationSample> samples;
    for(const auto& value:evidence)samples.append(readSample(value));
    if(o["validated"].toBool())c=validate(c,samples,number(o,"maxErrorMm"));
    else require(samples.isEmpty() && number(o,"maxErrorMm")==0 && number(o,"rmsMm")==0,"Candidate contains validation evidence");
    require((manual ? c.scale_==number(o,"millimetresPerPixel") && c.maxResidual_==number(o,"maxResidualMm") && encode(c)==o : mapping ? c.maxResidual_==number(o,"maxResidualMm") && encode(c)==o : c.scale_==number(o,"mmPerPx")) && c.rms_==number(o,"rmsMm") && c.dependency_==dependency,"Calibration content/evidence identity mismatch");
    return c;
}
MeasurementResult MeasurementService::distance(const ToolResult& first,const QString& firstPort,const ToolResult& second,const QString& secondPort,const PlanarCalibration& c,const CalibrationDependency& current,const QSize& size,double lower,double upper) const {
    MeasurementResult r;QString code="CALIBRATION_INVALID";
    try {
        require(c.measurementReady(),"Calibration has neither active model readiness nor accepted independent validation");
        require(!c.manualProvenance_ || c.manualProvenance_->origin=="acquired" || !c.readyRecordContentId_.isEmpty(),"File test calibration is not a production measurement");c.dependency().requireCurrent(current);
        require(size==current.resolution,"Camera raw resolution changed");
        code="TOLERANCE_INVALID";require(std::isfinite(lower) && std::isfinite(upper) && lower>=0 && lower<=upper,"Invalid millimetre tolerance interval");
        code="MEASUREMENT_INPUT_INVALID";
        require(first.state==ExecutionState::Done && second.state==ExecutionState::Done,"Upstream not DONE");
        const auto a=first.outputs.value(firstPort),b=second.outputs.value(secondPort);
        for(const auto& v:{a,b}) {
            require(v.valid && v.port.type==PortType::Point && v.port.unit=="px" && v.port.frame=="image" && std::holds_alternative<QPointF>(v.data),"Missing/invalid Point px image port");
            const auto& m=v.origin;
            require(!m.frameId.isEmpty() && !m.cycleId.isEmpty() && m.frameId==v.frameId && m.cycleId==v.cycleId && m.sourceId==current.cameraId && m.coordinateSystemId==current.source.id && m.completedAtUtc.isValid(),"Input source/frame/cycle/coordinate provenance mismatch");
            point({std::get<QPointF>(v.data),current.source},current);
        }
        require(a.frameId==b.frameId && a.cycleId==b.cycleId && a.origin.completedAtUtc==b.origin.completedAtUtc,"Inputs originate from different acquisition");
        const auto px=length(std::get<QPointF>(a.data),std::get<QPointF>(b.data));const double mm=c.mapping_ ? PlaneMappingService::distance(*c.mapping_,std::get<QPointF>(a.data),std::get<QPointF>(b.data)).millimetres : c.manualProvenance_ ? px/c.pixelsPerMillimetre_ : px*c.scale_;
        require(std::isfinite(px) && std::isfinite(mm) && (px==0 || mm>0),"Measurement overflow/underflow");
        r.pixels=px;r.millimetres=mm;r.origin=a.origin;r.calibration=c.dependency();r.state=ExecutionState::Done;
        r.decision=mm>=lower && mm<=upper ? PartDecision::Pass : PartDecision::Fail;
    }catch(const std::exception& e){r.errorCode=code;r.error=QString::fromUtf8(e.what());}
    return r;
}
}
