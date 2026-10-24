#include "application/EditorObservation.hpp"
#include "ActiveCalibrationService.hpp"
#include "IndependentCalibrationCapture.hpp"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QMap>
#include <QSaveFile>
#include <QSet>
#include <cmath>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void keys(const QJsonObject& o,const QSet<QString>& k){require(o.size()==k.size(),"Missing/unknown calibration contract fields");for(auto i=o.begin();i!=o.end();++i)require(k.contains(i.key()),"Unknown calibration contract field");}
QString text(const QJsonObject& o,const char* k){require(o[k].isString(),"Expected calibration text");const auto s=o[k].toString();require(!s.trimmed().isEmpty() && s.size()<=512,"Invalid calibration text");return s;}
int integer(const QJsonObject& o,const char* k){require(o[k].isDouble() && o[k].toDouble()==o[k].toInt() && o[k].toInt()>0 && o[k].toInt()<=1000000000,"Invalid calibration integer");return o[k].toInt();}
double number(const QJsonObject& o,const char* k){require(o[k].isDouble() && std::isfinite(o[k].toDouble()),"Invalid calibration number");return o[k].toDouble();}
bool hash(const QString& s){if(s.size()!=64)return false;for(auto c:s)if(!((c>='0' && c<='9') || (c>='a' && c<='f')))return false;return true;}
QString digest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
QJsonObject frameJson(const CoordinateFrame& f){f.validate();return {{"id",f.id},{"unit",f.unit==LengthUnit::Pixel ? "px" : "mm"},{"axes",f.axes==AxisConvention::XRightYDown ? "down" : "up"}};}
CoordinateFrame frame(const QJsonValue& v){require(v.isObject(),"Expected coordinate frame");const auto o=v.toObject();keys(o,{"id","unit","axes"});const auto u=text(o,"unit"),a=text(o,"axes");require((u=="px" || u=="mm") && (a=="down" || a=="up"),"Unknown coordinate convention");return {text(o,"id"),u=="px" ? LengthUnit::Pixel : LengthUnit::Millimetre,a=="down" ? AxisConvention::XRightYDown : AxisConvention::XRightYUp};}
QJsonObject sampleJson(const CalibrationSample& s){return {{"id",s.id},{"x1",s.first.position.x()},{"y1",s.first.position.y()},{"x2",s.second.position.x()},{"y2",s.second.position.y()},{"knownMm",s.knownMillimetres}};}
bool same(const CalibrationSample& a,const CalibrationSample& b){return (a.first.position==b.first.position && a.second.position==b.second.position) || (a.first.position==b.second.position && a.second.position==b.first.position);}
CalibrationSample sample(const QJsonObject& o,const CoordinateFrame& f){keys(o,{"id","x1","y1","x2","y2","knownMm"});return {text(o,"id"),{{number(o,"x1"),number(o,"y1")},f},{{number(o,"x2"),number(o,"y2")},f},number(o,"knownMm")};}
QJsonObject selectionJson(const ActiveCalibrationSelection& s){return {{"id",s.id},{"version",s.version},{"contentId",s.contentId},{"revision",s.revision}};}
ActiveCalibrationSelection selection(const QJsonValue& v){require(v.isObject(),"Invalid active selection");const auto o=v.toObject();keys(o,{"id","version","contentId","revision"});ActiveCalibrationSelection s{text(o,"id"),text(o,"contentId"),integer(o,"version"),integer(o,"revision")};require(hash(s.contentId),"Invalid active content identity");return s;}
CalibrationModelRecord find(const QVector<CalibrationModelRecord>& records,const QString& id,int version){for(const auto& r:records)if(r.model().dependency().calibrationId==id && r.model().dependency().version==version)return r;throw std::runtime_error("Calibration model not found");}
QVector<CalibrationModelRecord> decodeRecords(const QJsonObject& o){
    editor_observation::Scope observation(editor_observation::CatalogDecode);QVector<CalibrationModelRecord> result;QMap<QString,int> versions;const auto a=o["records"].toArray();require(a.size()<=128,"Calibration history full");for(const auto& v:a){require(v.isObject(),"Invalid model record");auto r=CalibrationModelRecord::decode(v.toObject());const auto& d=r.model().dependency();require(d.version==versions.value(d.calibrationId)+1,"Nonconsecutive calibration history");versions[d.calibrationId]=d.version;result.append(r);}return result;}
}
void MeasurementSetup::validate() const {
    for(const auto& s:{id,adapter,lensId,planeId})require(!s.trimmed().isEmpty() && s.size()<=512,"Missing measurement setup identity");
    require(adapter=="image_file" || adapter=="webcam" || adapter=="gentl","Unsupported acquisition setup adapter");require(hash(configId),"Invalid acquisition configuration hash");
    source.validate();target.validate();require(resolution.width()>0 && resolution.height()>0 && source.unit==LengthUnit::Pixel && source.axes==AxisConvention::XRightYDown && target.unit==LengthUnit::Millimetre,"Invalid measurement setup geometry");
}
QJsonObject MeasurementSetup::toJson() const {validate();return {{"schemaVersion",1},{"id",id},{"adapter",adapter},{"configId",configId},{"lensId",lensId},{"planeId",planeId},{"width",resolution.width()},{"height",resolution.height()},{"source",frameJson(source)},{"target",frameJson(target)}};}
MeasurementSetup MeasurementSetup::fromJson(const QJsonObject& o){keys(o,{"schemaVersion","id","adapter","configId","lensId","planeId","width","height","source","target"});require(integer(o,"schemaVersion")==1,"Unsupported measurement setup schema");MeasurementSetup s{text(o,"id"),text(o,"adapter"),text(o,"configId"),text(o,"lensId"),text(o,"planeId"),{integer(o,"width"),integer(o,"height")},frame(o["source"]),frame(o["target"])};s.validate();return s;}

CalibrationModelRecord::CalibrationModelRecord(PlanarCalibration m,MeasurementSetup s,CalibrationImageEvidence i,CalibrationControl c):model_(std::move(m)),setup_(std::move(s)),image_(std::move(i)),control_(std::move(c)){}
CalibrationModelRecord CalibrationModelRecord::create(const PlanarCalibration& m,const MeasurementSetup& s,const CalibrationImageEvidence& image,const CalibrationControl& control){
    MeasurementService math;const auto encoded=math.encode(m);auto replay=math.decode(encoded);s.validate();const auto& d=replay.dependency();
    require(d.lensId==s.lensId && d.workingPlaneId==s.planeId && d.resolution==s.resolution && d.source==s.source && d.target==s.target,"Model/measurement setup mismatch");
    require(image.sourceToolId==d.cameraId && !image.frameId.trimmed().isEmpty() && image.frameId.size()<=512 && hash(image.pixelHash),"Invalid image provenance");
    require(image.origin=="acquired" || image.origin=="setup_file" || image.origin=="trial_file" || image.origin=="experimental_acquired" || image.origin=="experimental_setup_file","Unknown image evidence origin");
    require(image.experimental.has_value()==image.origin.startsWith("experimental_"),"Experimental model evidence cannot be removed");
    if(image.experimental){const auto& e=*image.experimental;e.validate();require(s==IndependentCaptureRequest::installation(e.camera,e.setup) && image.pixelHash==e.acquisition["contentId"] && image.origin==(e.origin==SourceOrigin::CameraAcquired ? "experimental_acquired":"experimental_setup_file"),"Experimental model installation/raster/origin mismatch");}
    require(!d.cameraId.startsWith("calibration-test:") || image.origin!="acquired","Test image cannot become acquired evidence");
    if(m.mapping())require(m.mapping()->observation().source.frameId==image.frameId,"Mapping image frame mismatch");
    if(m.mapping() && m.mapping()->observation().target.pattern!=CalibrationGridPattern::Chessboard)
        require(m.mapping()->observation().pixelHash==image.pixelHash,"Circle mapping raster evidence mismatch");
    if(m.mapping() && m.mapping()->observation().target.pattern==CalibrationGridPattern::FramedCircles){
        require(image.experimental.has_value()==!m.mapping()->observation().source.experimentalAcquisition.isEmpty(),"Framed acquisition evidence cannot be removed");
        if(image.experimental)require(m.mapping()->observation().source.experimentalAcquisition==image.experimental->acquisition,"Framed acquisition sidecar mismatch");
    }
    if(m.manualProvenance())require(m.manualProvenance()->frameId==image.frameId && (m.manualProvenance()->origin!="file_test" || image.origin!="acquired"),"Manual image evidence mismatch");
    require(control.samples.size()<=256 && (!control.limitMillimetres || (!control.samples.isEmpty() && std::isfinite(*control.limitMillimetres) && *control.limitMillimetres>=0)),"Invalid independent control limit/count");
    CalibrationModelRecord r(std::move(replay),s,image,control);QSet<QString> ids;double maximum=0;
    for(int n=0;n<control.samples.size();++n){const auto& c=control.samples[n];require(!c.id.trimmed().isEmpty() && c.id.size()<=128 && !ids.contains(c.id) && std::isfinite(c.knownMillimetres) && c.knownMillimetres>0,"Invalid independent control sample");ids.insert(c.id);
        for(const auto& p:{c.first,c.second})require(p.frame==s.source && std::isfinite(p.position.x()) && std::isfinite(p.position.y()) && p.position.x()>=0 && p.position.y()>=0 && p.position.x()<s.resolution.width() && p.position.y()<s.resolution.height() && p.position.x()<=s.resolution.width()-1 && p.position.y()<=s.resolution.height()-1,"Control outside image frame");
        require(c.first.position!=c.second.position,"Degenerate control segment");for(int j=0;j<n;++j)require(!same(c,control.samples[j]),"Repeated control geometry");
        if(encoded.contains("teaching")){auto t=sample(encoded["teaching"].toObject(),s.source);require(t.id!=c.id && !same(t,c),"Teaching is not independent control");}
        double mm=0;if(m.mapping()){for(const auto& p:m.mapping()->observation().pixels)for(const auto& q:{c.first.position,c.second.position})require(std::hypot(p.x()-q.x(),p.y()-q.y())>0.5,"Fit point is not independent control");mm=PlaneMappingService::distance(*m.mapping(),c.first.position,c.second.position).millimetres;}else mm=std::hypot(c.first.position.x()-c.second.position.x(),c.first.position.y()-c.second.position.y())*m.millimetresPerPixel();
        const auto residual=std::abs(mm-c.knownMillimetres);require(std::isfinite(mm) && mm>0 && std::isfinite(residual),"Control distance overflow");r.residuals_.append(residual);maximum=std::max(maximum,residual);
    }
    if(control.limitMillimetres)r.accepted_=maximum<=*control.limitMillimetres;
    auto o=r.encode();o.remove("contentId");r.contentId_=digest(o);return r;
}
bool CalibrationModelRecord::activatable() const {return !image_.experimental && image_.origin!="trial_file" && (!accepted_ || *accepted_);}
bool CalibrationModelRecord::experimentallyActivatable() const {return image_.experimental.has_value() && (!accepted_ || *accepted_);}
QJsonObject CalibrationModelRecord::encode() const {
    if(migration_)return {{"schemaVersion",5},{"policy","setup-migration/1"},{"original",original_},{"derived",derived_},{"contentId",contentId_}};
    if(legacy_)return {{"schemaVersion",3},{"policy","legacy-validated/1"},{"modelReady",true},{"model",MeasurementService{}.encode(model_)},{"setup",setup_.toJson()},
        {"legacyEvidence",QJsonObject{{"modelContentId",model_.dependency().contentId},{"rasterEvidence","not_recorded"},{"independentControl","original_legacy_holdout"}}},{"name",name_},{"recordedAtUtc",recordedAt_.toString(Qt::ISODateWithMs)},{"contentId",contentId_}};
    QJsonArray samples,residuals;for(const auto& s:control_.samples)samples.append(sampleJson(s));for(double r:residuals_)residuals.append(r);
    QJsonObject result{{"schemaVersion",name_.isEmpty() ? 1:2},{"policy","model-ready/1"},{"modelReady",true},{"model",MeasurementService{}.encode(model_)},{"setup",setup_.toJson()},
        {"image",QJsonObject{{"origin",image_.origin},{"sourceToolId",image_.sourceToolId},{"frameId",image_.frameId},{"pixelHash",image_.pixelHash}}},
        {"control",QJsonObject{{"samples",samples},{"limitMm",control_.limitMillimetres ? QJsonValue(*control_.limitMillimetres) : QJsonValue()},{"residualsMm",residuals},{"accepted",accepted_ ? QJsonValue(*accepted_) : QJsonValue()}}},{"contentId",contentId_}};
    if(image_.experimental){result["schemaVersion"]=6;result["policy"]="experimental-model-ready/1";result["experimental"]=image_.experimental->toJson();}
    if(!original_.isEmpty()){result["schemaVersion"]=4;result["original"]=original_;result["policy"]="setup-association/1";}
    if(!name_.isEmpty()){result["name"]=name_;result["recordedAtUtc"]=recordedAt_.toString(Qt::ISODateWithMs);}return result;
}
CalibrationModelRecord CalibrationModelRecord::named(const QString& name,QDateTime at) const {
    require(!migration_,"Mapped records are immutable; create a new version");
    require(!name.trimmed().isEmpty() && name==name.trimmed() && name.size()<=128,"Invalid calibration name");
    require(at.isValid() && at.offsetFromUtc()==0,"UTC record date required");auto r=*this;r.name_=name;r.recordedAt_=at.toUTC();auto o=r.encode();o.remove("contentId");r.contentId_=digest(o);return r;
}
CalibrationModelRecord CalibrationModelRecord::adoptLegacy(const PlanarCalibration& model,const MeasurementSetup& setup,const QString& name,QDateTime at){
    const auto m=MeasurementService{}.decode(MeasurementService{}.encode(model));const auto& d=m.dependency();setup.validate();require(m.validated() && d.lensId==setup.lensId && d.workingPlaneId==setup.planeId && d.resolution==setup.resolution && d.source==setup.source && d.target==setup.target,"Legacy adoption requires original accepted evidence and matching declared setup");
    CalibrationModelRecord r(m,setup,{"legacy_record",d.cameraId,{},{}},{});r.legacy_=true;r.residuals_=m.residualHoldoutMillimetres();r.accepted_=true;return r.named(name,at);
}
CalibrationModelRecord CalibrationModelRecord::associate(const CalibrationModelRecord& original,const MeasurementSetup& setup,const QString& name,QDateTime at){
    require(original.original_.isEmpty() && !original.legacy_ && original.image_.origin=="trial_file","Only original raster-backed trial may be associated");
    setup.validate();require(setup.resolution==original.setup_.resolution && setup.source==original.setup_.source && setup.target.axes==original.setup_.target.axes,"Association geometry/axes mismatch");
    auto d=original.model_.dependency();d.calibrationId="association:"+digest({{"original",original.contentId()},{"setup",setup.toJson()}});d.version=1;d.contentId.clear();d.lensId=setup.lensId;d.workingPlaneId=setup.planeId;d.target=setup.target;
    MeasurementService math;const auto encoded=math.encode(original.model_);
    auto model=[&]{if(original.model_.mapping())return math.teachHomography(d,*original.model_.mapping());
        if(original.model_.manualProvenance())return math.manualScale(d,original.model_.pixelsPerMillimetre(),*original.model_.manualProvenance());
        return math.teach(d,sample(encoded["teaching"].toObject(),setup.source));}();
    QVector<CalibrationSample> holdout;for(const auto& v:encoded["holdout"].toArray())holdout.append(sample(v.toObject(),setup.source));
    if(original.model_.validated())model=math.validate(model,holdout,encoded["maxErrorMm"].toDouble());
    auto image=original.image_;image.origin="setup_file";auto r=create(model,setup,image,original.control_);r.original_=original.encode();return r.named(name,at);
}
CalibrationModelRecord CalibrationModelRecord::mapLegacy(const CalibrationModelRecord& original,const MeasurementSetup& setup,const QString& name,QDateTime at){
    require(!original.image_.experimental && original.original_.isEmpty() && original.image_.origin!="trial_file","Original legacy/global record required; associate trials separately");
    setup.validate();require(setup.resolution==original.setup_.resolution && setup.lensId==original.setup_.lensId && setup.planeId==original.setup_.planeId && setup.source==original.setup_.source && setup.target.axes==original.setup_.target.axes,"Legacy optical/plane/raster/coordinate declaration differs; recalibrate explicitly");
    auto d=original.model_.dependency();d.calibrationId="migration:"+digest({{"originalId",d.calibrationId},{"setup",setup.toJson()}});d.contentId.clear();d.target=setup.target;
    MeasurementService math;const auto encoded=math.encode(original.model_);
    auto model=[&]{if(original.model_.mapping())return math.teachHomography(d,*original.model_.mapping());if(original.model_.manualProvenance())return math.manualScale(d,original.model_.pixelsPerMillimetre(),*original.model_.manualProvenance());return math.teach(d,sample(encoded["teaching"].toObject(),setup.source));}();
    QVector<CalibrationSample> holdout;for(const auto& v:encoded["holdout"].toArray())holdout.append(sample(v.toObject(),setup.source));if(original.model_.validated())model=math.validate(model,holdout,encoded["maxErrorMm"].toDouble());
    auto r=original.legacy_ ? adoptLegacy(model,setup,name,at):create(model,setup,original.image_,original.control_).named(name,at);
    r.derived_=r.encode();r.original_=original.encode();r.migration_=true;auto envelope=r.encode();envelope.remove("contentId");r.contentId_=digest(envelope);return r;
}
CalibrationModelRecord CalibrationModelRecord::decode(const QJsonObject& o){
    editor_observation::Scope observation(editor_observation::ModelDecode);
    const int schema=integer(o,"schemaVersion");
    if(schema==5){keys(o,{"schemaVersion","policy","original","derived","contentId"});require(o["original"].isObject() && o["derived"].isObject() && o["original"].toObject()["schemaVersion"].toInt()<=3 && o["derived"].toObject()["schemaVersion"].toInt()<=3,"Invalid migration wrapper");const auto original=decode(o["original"].toObject()),derived=decode(o["derived"].toObject());auto r=mapLegacy(original,derived.setup(),derived.name(),derived.recordedAt());require(r.encode()==o,"Migration replay/content mismatch");return r;}
    if(schema==4){keys(o,{"schemaVersion","policy","modelReady","model","setup","image","control","contentId","name","recordedAtUtc","original"});require(o["original"].isObject() && o["original"].toObject()["schemaVersion"].toInt()<=3,"Invalid association origin");auto r=associate(decode(o["original"].toObject()),MeasurementSetup::fromJson(o["setup"].toObject()),text(o,"name"),QDateTime::fromString(text(o,"recordedAtUtc"),Qt::ISODateWithMs));require(r.encode()==o,"Association replay/content mismatch");return r;}
    if(schema==3){keys(o,{"schemaVersion","policy","modelReady","model","setup","legacyEvidence","name","recordedAtUtc","contentId"});require(o["policy"]=="legacy-validated/1" && o["modelReady"].isBool() && o["modelReady"].toBool() && o["model"].isObject() && o["setup"].isObject(),"Invalid legacy readiness policy");auto r=adoptLegacy(MeasurementService{}.decode(o["model"].toObject()),MeasurementSetup::fromJson(o["setup"].toObject()),text(o,"name"),QDateTime::fromString(text(o,"recordedAtUtc"),Qt::ISODateWithMs));require(r.encode()==o,"Legacy adoption evidence/hash mismatch");return r;}
    if(schema==6){QSet<QString> fields{"schemaVersion","policy","modelReady","model","setup","image","control","contentId","experimental"};if(o.contains("name")){fields.insert("name");fields.insert("recordedAtUtc");}keys(o,fields);}
    else if(schema==2)keys(o,{"schemaVersion","policy","modelReady","model","setup","image","control","contentId","name","recordedAtUtc"});
    else keys(o,{"schemaVersion","policy","modelReady","model","setup","image","control","contentId"});
    require((schema==1 || schema==2 || schema==6) && text(o,"policy")== (schema==6 ? "experimental-model-ready/1":"model-ready/1") && o["modelReady"].isBool() && o["modelReady"].toBool(),"Unsupported calibration readiness policy");
    for(const auto* k:{"model","setup","image","control"})require(o[k].isObject(),"Expected calibration object");
    auto s=MeasurementSetup::fromJson(o["setup"].toObject());const auto i=o["image"].toObject(),c=o["control"].toObject();keys(i,{"origin","sourceToolId","frameId","pixelHash"});keys(c,{"samples","limitMm","residualsMm","accepted"});require(c["samples"].isArray() && c["samples"].toArray().size()<=256 && c["residualsMm"].isArray() && (c["accepted"].isNull() || c["accepted"].isBool()),"Invalid control evidence types");
    CalibrationControl control;if(!c["limitMm"].isNull())control.limitMillimetres=number(c,"limitMm");for(const auto& v:c["samples"].toArray()){require(v.isObject(),"Invalid control sample");control.samples.append(sample(v.toObject(),s.source));}
    std::optional<ExperimentalCalibrationEvidence> experimental;if(schema==6){require(o["experimental"].isObject(),"Experimental model evidence required");experimental=ExperimentalCalibrationEvidence::fromJson(o["experimental"].toObject());}
    auto r=create(MeasurementService{}.decode(o["model"].toObject()),s,{text(i,"origin"),text(i,"sourceToolId"),text(i,"frameId"),text(i,"pixelHash"),experimental},control);if(schema==2 || (schema==6 && o.contains("name")))r=r.named(text(o,"name"),QDateTime::fromString(text(o,"recordedAtUtc"),Qt::ISODateWithMs));require(r.encode()==o,"Calibration evidence/content mismatch");return r;
}

ActiveCalibrationService::ActiveCalibrationService(QString path,QString projectId,ChangeGate gate):path_(std::move(path)),projectId_(std::move(projectId)),gate_(std::move(gate)){require(QFileInfo(path_).isAbsolute() && !projectId_.trimmed().isEmpty() && projectId_.size()<=512 && bool(gate_),"Active calibration requires project/path/change gate");read();}
QJsonObject ActiveCalibrationService::read() const {
    editor_observation::Scope observation(editor_observation::CatalogRead);
    if(!QFileInfo::exists(path_))return {{"format","visionflow.active-calibration"},{"schemaVersion",1},{"projectId",projectId_},{"records",QJsonArray{}},{"selection",QJsonValue()},{"revision",0}};
    QFile f(path_);require(f.open(QIODevice::ReadOnly) && f.size()<=4*1024*1024,"Cannot read bounded active calibration catalog");auto bytes=f.read(4*1024*1024+1);require(f.error()==QFileDevice::NoError && bytes.size()<=4*1024*1024,"Active calibration catalog read failed");QJsonParseError error;const auto doc=QJsonDocument::fromJson(bytes,&error);require(error.error==QJsonParseError::NoError && doc.isObject(),"Invalid active calibration JSON");const auto o=doc.object();keys(o,{"format","schemaVersion","projectId","records","selection","revision"});require(text(o,"format")=="visionflow.active-calibration" && integer(o,"schemaVersion")==1 && text(o,"projectId")==projectId_ && o["records"].isArray(),"Active calibration project/schema mismatch");
    require(o["revision"].isDouble() && o["revision"].toDouble()==o["revision"].toInt() && o["revision"].toInt()>=0 && o["revision"].toInt()<=1000000000,"Invalid activation revision");const auto records=decodeRecords(o);
    if(!o["selection"].isNull()){const auto s=selection(o["selection"]);require(s.revision==o["revision"].toInt() && find(records,s.id,s.version).contentId()==s.contentId,"Selection record/revision mismatch");}return o;
}
void ActiveCalibrationService::write(const QJsonObject& o) const {const auto b=QJsonDocument(o).toJson();require(b.size()<=4*1024*1024,"Active calibration catalog full");QSaveFile f(path_);f.setDirectWriteFallback(false);require(f.open(QIODevice::WriteOnly) && f.write(b)==b.size() && f.commit(),"Cannot commit active calibration catalog");}
QVector<CalibrationModelRecord> ActiveCalibrationService::records() const {return decodeRecords(read());}
void ActiveCalibrationService::save(const CalibrationModelRecord& r) const {
    const auto record=CalibrationModelRecord::decode(r.encode());QLockFile lock(path_+".lock");require(lock.tryLock(0),"Active calibration catalog busy");auto o=read();auto all=decodeRecords(o);require(all.size()<128,"Calibration history full");int latest=0;for(const auto& v:all)if(v.model().dependency().calibrationId==record.model().dependency().calibrationId)latest=v.model().dependency().version;require(record.model().dependency().version==latest+1,"Calibration version must append consecutively");auto a=o["records"].toArray();a.append(record.encode());o["records"]=a;write(o);
}
std::optional<ActiveCalibrationSelection> ActiveCalibrationService::remembered() const {const auto o=read();const auto v=o.value("selection");return v.isNull() ? std::nullopt : std::optional(selection(v));}
ActiveCalibrationSnapshot ActiveCalibrationService::snapshot() const {require(active_.has_value(),"Aktif sistem kalibrasyonu yok; ana sayfadan etkinleştirin.");const auto r=find(records(),active_->id,active_->version);return resolve(r.setup());}
void ActiveCalibrationService::activate(const QString& id,int version,const MeasurementSetup& current){
    require(gate_(),"Stop Runtime/workers and finish pending records/ACK before changing calibration");current.validate();QLockFile lock(path_+".lock");require(lock.tryLock(0),"Active calibration catalog busy");auto o=read();const auto r=find(decodeRecords(o),id,version);require(r.setup()==current && r.activatable(),"Calibration setup/control/trial evidence prevents activation");require(o["revision"].toInt()<1000000000,"Activation revision exhausted");ActiveCalibrationSelection next{id,r.contentId(),version,o["revision"].toInt()+1};o["selection"]=selectionJson(next);o["revision"]=next.revision;write(o);active_=next;
}
void ActiveCalibrationService::deactivate(){require(gate_(),"Stop Runtime/workers and finish pending records/ACK before changing calibration");QLockFile lock(path_+".lock");require(lock.tryLock(0),"Active calibration catalog busy");auto o=read();require(o["revision"].toInt()<1000000000,"Activation revision exhausted");o["selection"]=QJsonValue();o["revision"]=o["revision"].toInt()+1;write(o);active_.reset();}
ActiveCalibrationSnapshot ActiveCalibrationService::resolve(const MeasurementSetup& current) const {require(active_.has_value(),"No active calibration");current.validate();auto o=read();require(!o["selection"].isNull() && selection(o["selection"])==*active_,"Active calibration selection changed");auto r=find(decodeRecords(o),active_->id,active_->version);require(r.setup()==current && r.contentId()==active_->contentId && r.activatable(),"Active calibration installation/evidence invalid");return {r,*active_};}
}
