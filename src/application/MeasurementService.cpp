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
void MeasurementService::identify(PlanarScaleCalibration& c) {
    auto setup=c.dependency_.toJson();setup.remove("contentId");QJsonArray holdout;for(const auto& s:c.holdout_)holdout.append(sampleJson(s));
    const QJsonObject object{{"model","uniform_planar_scale/1"},{"setup",setup},{"mmPerPx",c.scale_},{"teaching",sampleJson(c.teaching_)},{"holdout",holdout},{"validated",c.validated_},{"maxErrorMm",c.maxError_},{"rmsMm",c.rms_}};
    c.dependency_.contentId=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(object).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
PlanarScaleCalibration MeasurementService::teach(CalibrationDependency setup,const CalibrationSample& sample) const {
    setup.contentId="pending";setup.validate();
    require(setup.source.unit==LengthUnit::Pixel && setup.source.axes==AxisConvention::XRightYDown && setup.target.unit==LengthUnit::Millimetre,"Uniform scale requires image pixels to millimetres");
    PlanarScaleCalibration c;c.dependency_=std::move(setup);c.teaching_=sample;c.scale_=sample.knownMillimetres/sampleLength(sample,c.dependency_);
    require(std::isfinite(c.scale_) && c.scale_>0,"Invalid scale");identify(c);return c;
}
PlanarScaleCalibration MeasurementService::validate(const PlanarScaleCalibration& candidate,const QVector<CalibrationSample>& holdout,double maxError) const {
    require(std::isfinite(maxError) && maxError>=0 && holdout.size()>=2 && holdout.size()<=256,"Validation requires 2–256 independent samples and finite nonnegative error bound");
    auto c=candidate;QSet<QString> ids{c.teaching_.id};double squareSum=0;
    for(int i=0;i<holdout.size();++i) {
        const auto& s=holdout[i];require(!ids.contains(s.id) && !sameSegment(s,c.teaching_),"Teaching/repeated sample cannot be holdout evidence");ids.insert(s.id);
        for(int j=0;j<i;++j)require(!sameSegment(s,holdout[j]),"Repeated holdout geometry");
        const double residual=std::abs(sampleLength(s,c.dependency_)*c.scale_-s.knownMillimetres);
        require(std::isfinite(residual) && residual<=maxError,"Holdout residual exceeds limit");squareSum+=residual*residual;
    }
    require(std::isfinite(squareSum),"Residual overflow");c.rms_=std::sqrt(squareSum/holdout.size());c.holdout_=holdout;c.maxError_=maxError;c.validated_=true;identify(c);return c;
}
QJsonObject MeasurementService::encode(const PlanarScaleCalibration& c) const {
    QJsonArray evidence;for(const auto& s:c.holdout_)evidence.append(sampleJson(s));
    return {{"schemaVersion",1},{"model","uniform_planar_scale/1"},{"dependency",c.dependency_.toJson()},
        {"teaching",sampleJson(c.teaching_)},{"holdout",evidence},{"validated",c.validated_},
        {"maxErrorMm",c.maxError_},{"rmsMm",c.rms_},{"mmPerPx",c.scale_}};
}
PlanarScaleCalibration MeasurementService::decode(const QJsonObject& o) const {
    const QSet<QString> fields{"schemaVersion","model","dependency","teaching","holdout","validated","maxErrorMm","rmsMm","mmPerPx"};
    require(o.size()==fields.size(),"Missing/unknown calibration fields");
    for(auto it=o.begin();it!=o.end();++it)require(fields.contains(it.key()),"Unknown calibration field");
    require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["model"].toString()=="uniform_planar_scale/1","Unsupported calibration schema/model");
    require(o["dependency"].isObject() && o["teaching"].isObject() && o["holdout"].isArray() && o["validated"].isBool(),"Invalid calibration record types");
    const auto dependency=CalibrationDependency::fromJson(o["dependency"].toObject());
    auto number=[](const QJsonObject& j,const char* key){require(j[key].isDouble() && std::isfinite(j[key].toDouble()),"Invalid calibration number");return j[key].toDouble();};
    auto readSample=[&](const QJsonValue& value){
        require(value.isObject(),"Invalid calibration sample");const auto j=value.toObject();
        const QSet<QString> keys{"id","x1","y1","x2","y2","knownMm"};require(j.size()==keys.size(),"Missing/unknown sample fields");
        for(auto it=j.begin();it!=j.end();++it)require(keys.contains(it.key()),"Unknown sample field");
        require(j["id"].isString(),"Invalid sample id");
        return CalibrationSample{j["id"].toString(),{{number(j,"x1"),number(j,"y1")},dependency.source},{{number(j,"x2"),number(j,"y2")},dependency.source},number(j,"knownMm")};
    };
    auto c=teach(dependency,readSample(o["teaching"]));const auto evidence=o["holdout"].toArray();
    require(evidence.size()<=256,"Too many calibration samples");QVector<CalibrationSample> samples;
    for(const auto& value:evidence)samples.append(readSample(value));
    if(o["validated"].toBool())c=validate(c,samples,number(o,"maxErrorMm"));
    else require(samples.isEmpty() && number(o,"maxErrorMm")==0 && number(o,"rmsMm")==0,"Candidate contains validation evidence");
    require(c.scale_==number(o,"mmPerPx") && c.rms_==number(o,"rmsMm") && c.dependency_==dependency,"Calibration content/evidence identity mismatch");
    return c;
}
MeasurementResult MeasurementService::distance(const ToolResult& first,const QString& firstPort,const ToolResult& second,const QString& secondPort,const PlanarScaleCalibration& c,const CalibrationDependency& current,const QSize& size,double lower,double upper) const {
    MeasurementResult r;QString code="CALIBRATION_INVALID";
    try {
        require(c.validated(),"Calibration has no accepted independent validation");c.dependency().requireCurrent(current);
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
        const auto px=length(std::get<QPointF>(a.data),std::get<QPointF>(b.data)),mm=px*c.scale_;
        require(std::isfinite(px) && std::isfinite(mm),"Measurement overflow");
        r.pixels=px;r.millimetres=mm;r.origin=a.origin;r.calibration=c.dependency();r.state=ExecutionState::Done;
        r.decision=mm>=lower && mm<=upper ? PartDecision::Pass : PartDecision::Fail;
    }catch(const std::exception& e){r.errorCode=code;r.error=QString::fromUtf8(e.what());}
    return r;
}
}
