#include "HandEyeValidationService.hpp"
#include "ProductionStore.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QSaveFile>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
QString id(const QUuid& u){return u.toString(QUuid::WithoutBraces);}
bool uuid(const QJsonValue& v){return v.isString() && !QUuid(v.toString()).isNull() && id(QUuid(v.toString()))==v.toString();}
double angle(Rotation3D a,Rotation3D b){
    a=a.normalized();b=b.normalized();
    // atan2 form avoids acos loss near zero and is invariant to q/-q.
    const double w=a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;
    const double x=a.w*b.x-a.x*b.w-a.y*b.z+a.z*b.y;
    const double y=a.w*b.y+a.x*b.z-a.y*b.w-a.z*b.x;
    const double z=a.w*b.z-a.x*b.y+a.y*b.x-a.z*b.w;
    return 2*std::atan2(std::hypot(x,y,z),std::abs(w));
}
QJsonObject error(const SpatialTransform& a,const SpatialTransform& b){
    require(a.source==b.source && a.target==b.target,"REFERENCE_FRAME_MISMATCH");
    const auto& x=a.translationMm;const auto& y=b.translationMm;
    const double t=std::hypot(x[0]-y[0],x[1]-y[1],x[2]-y[2]);
    const double r=angle(a.rotation,b.rotation);require(std::isfinite(t)&&std::isfinite(r),"NONFINITE_ERROR");
    return {{"mm",t},{"rad",r}};
}
QJsonObject metrics(std::vector<double> t,std::vector<double> r){
    auto summary=[](std::vector<double> v){double sum=0;for(double x:v)sum+=x*x;std::sort(v.begin(),v.end());require(std::isfinite(sum),"ERROR_OVERFLOW");return QJsonObject{{"max",v.back()},{"rms",std::sqrt(sum/v.size())},{"p95",v[size_t(std::ceil(.95*v.size()))-1]}};};
    return {{"mm",summary(t)},{"rad",summary(r)}};
}
QJsonObject toleranceJson(const HandEyeValidationTolerance& t){
    require(!t.revision.isNull(),"MISSING_TOLERANCE_REVISION");
    for(double v:{t.maxMm,t.rmsMm,t.p95Mm,t.maxRad,t.rmsRad,t.p95Rad})require(std::isfinite(v)&&v>0,"INVALID_TOLERANCE");
    return {{"revision",id(t.revision)},{"mm",QJsonObject{{"max",t.maxMm},{"rms",t.rmsMm},{"p95",t.p95Mm}}},{"rad",QJsonObject{{"max",t.maxRad},{"rms",t.rmsRad},{"p95",t.p95Rad}}}};
}
bool within(const QJsonObject& m,const QJsonObject& t){for(const QString unit:{"mm","rad"})for(const QString key:{"max","rms","p95"})if(m[unit].toObject()[key].toDouble()>t[unit].toObject()[key].toDouble())return false;return true;}
QJsonObject candidate(const ProductionStore& store,const QUuid& c,const QUuid& revision,const QUuid& session){
    const auto o=store.get("hand_eye_candidate",id(c));
    require(o["format"]=="visionflow.hand-eye-candidate/1" && o["state"]=="CANDIDATE" && o["evidence"]=="synthetic-fixture" && o["physicalEligible"]==false,"INVALID_CANDIDATE");
    require(o["candidate"]==id(c) && uuid(o["candidateRevision"]) && o["candidateRevision"]==id(revision) && o["session"]==id(session),"STALE_CANDIDATE");
    require(o["dependencyDigest"]==handEyeDigest(o["dependencies"].toObject()) && o["sampleSetDigest"]==handEyeDigest(o["dataset"].toObject()),"CANDIDATE_DIGEST_MISMATCH");return o;
}
QString activePath(const QString& dir,const QUuid& station){require(!station.isNull(),"MISSING_STATION");return dir+"/hand_eye_active/"+id(station)+".json";}
void safeRoot(const QString& dir){require(QDir(dir).isAbsolute() && !QFileInfo(dir).isSymLink() && !QFileInfo(dir+"/hand_eye_active").isSymLink(),"INVALID_ACTIVE_DIRECTORY");}
}
QString handEyeDigest(const QJsonObject& o){return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
QJsonObject handEyeDependencyJson(const CalibrationDependencySnapshot& d){
    return {{"mode",int(d.mode)},{"cameraKey",double(d.cameraKey)},{"cameraRevision",id(d.cameraRevision)},
      {"intrinsicKey",double(d.intrinsicRevisionKey)},{"intrinsicRevision",id(d.intrinsicRevision)},
      {"plateKey",double(d.plateRevisionKey)},{"plateRevision",id(d.plateRevision)},
      {"tcpKey",double(d.flangeTcpRevisionKey)},{"tcpRevision",id(d.flangeTcpRevision)},
      {"adapterKey",double(d.robotAdapterRevisionKey)},{"adapterRevision",id(d.robotAdapterRevision)},
      {"acquisitionKey",double(d.acquisitionProfileRevisionKey)},{"acquisitionRevision",id(d.acquisitionProfileRevision)},
      {"modelKey",double(d.modelRevisionKey)},{"modelRevision",id(d.modelRevision)},
      {"cameraId",d.cameraId},{"physical",d.physical.toJson()},
      {"targetFrame",id(d.pose.targetFrame)},{"targetRevision",id(d.pose.targetFrameRevision)},
      {"sourceFrame",id(d.pose.sourceFrame)},{"sourceRevision",id(d.pose.sourceFrameRevision)},{"convention",int(d.pose.convention)}};
}
HandEyeValidationService::HandEyeValidationService(QString directory):directory_(QDir::cleanPath(directory)){}
HandEyeValidationResult HandEyeValidationService::validateFixture(const HandEyeValidationRequest& input) const {
    const auto request=input;HandEyeValidationResult result;auto& out=result.report;
    out={{"format","visionflow.hand-eye-validation/1"},{"evidence","synthetic-fixture"},{"physicalEligible",false},{"productionPermitted",false},{"state","FAILED"}};
    try{
        require(request.evidence==EvidenceState::Synthetic,"PHYSICAL_GATE_BLOCKED");
        require(!request.session.isNull()&&!request.candidate.isNull()&&!request.candidateRevision.isNull(),"MISSING_CANDIDATE_IDENTITY");
        require(!request.referenceRevision.isNull()&&!request.referenceSource.trimmed().isEmpty()&&request.referenceSource.toUtf8().size()<=256,"MISSING_INDEPENDENT_REFERENCE");
        const auto tolerance=toleranceJson(request.tolerance);ProductionStore store(directory_);
        const auto c=candidate(store,request.candidate,request.candidateRevision,request.session);
        require(request.holdout.size()>=5&&request.holdout.size()<=256,"HOLDOUT_COUNT");
        require(request.references.size()==request.holdout.size(),"REFERENCE_COUNT");
        const auto x=SpatialTransform::fromJson(c["transform"].toObject());const auto fixed=SpatialTransform::fromJson(c["fixedTransform"].toObject());
        QSet<QUuid> samples,frames;for(auto v:c["dataset"].toObject()["samples"].toArray()){auto s=v.toObject();samples.insert(QUuid(s["sample"].toString()));frames.insert(QUuid(s["frame"].toString()));}
        QMap<QUuid,SpatialTransform> refs;QJsonArray referenceRows;
        for(const auto& r:request.references){require(!r.sample.isNull()&&!refs.contains(r.sample),"DUPLICATE_REFERENCE");r.expectedPlate.validate();refs.insert(r.sample,r.expectedPlate);referenceRows.append(QJsonObject{{"sample",id(r.sample)},{"expectedPlate",r.expectedPlate.toJson()}});}
        QJsonArray rows;std::vector<double> t,r,ct,cr;
        const auto policy=FixturePolicyV1::create();
        for(const auto& s:request.holdout){
            require(s.holdout,"TRAINING_SAMPLE_IN_HOLDOUT");
            require(!s.sample.isNull()&&!s.frame.isNull()&&!samples.contains(s.sample)&&!frames.contains(s.frame),"TRAINING_HOLDOUT_OVERLAP_OR_DUPLICATE");samples.insert(s.sample);frames.insert(s.frame);
            require(refs.contains(s.sample),"MISSING_SAMPLE_REFERENCE");
            require(std::isfinite(s.rmsPx)&&std::isfinite(s.maxPx)&&std::isfinite(s.ambiguityMarginPx)&&s.rmsPx>=0&&s.maxPx>=s.rmsPx&&s.rmsPx<=policy.maxPlateRmsPx&&s.maxPx<=policy.maxPlateMaxPx&&s.ambiguityMarginPx>=policy.minAmbiguityMarginPx,"HOLDOUT_PLATE_QUALITY");
            const auto g=s.baseTFlange;
            const bool inHand=c["dependencies"].toObject()["mode"].toInt()==int(HandEyeMode::EyeInHand);
            // Explicit equality for the otherwise unused endpoint of G: in the
            // eye-to-hand case composition alone would not check base identity.
            require(g.target==FrameRef::fromJson(c["dataset"].toObject()["base"].toObject()) && g.source==FrameRef::fromJson(c["dataset"].toObject()["flange"].toObject()),"HOLDOUT_ROBOT_FRAME_MISMATCH");
            const auto predicted=s.cameraTPlate.then(x).then(inHand?g:g.inverse());
            const auto independent=error(predicted,refs[s.sample]);const auto closure=error(predicted,fixed);
            t.push_back(independent["mm"].toDouble());r.push_back(independent["rad"].toDouble());ct.push_back(closure["mm"].toDouble());cr.push_back(closure["rad"].toDouble());
            rows.append(QJsonObject{{"sample",id(s.sample)},{"frame",id(s.frame)},{"baseTFlange",g.toJson()},{"cameraTPlate",s.cameraTPlate.toJson()},{"predictedPlate",predicted.toJson()},{"rmsPx",s.rmsPx},{"maxPx",s.maxPx},{"ambiguityMarginPx",s.ambiguityMarginPx},{"independentError",independent},{"closureError",closure}});
        }
        const auto m=metrics(t,r),closure=metrics(ct,cr);const bool pass=within(m,tolerance)&&within(closure,tolerance);
        QJsonObject held{{"samples",rows}},reference{{"revision",id(request.referenceRevision)},{"source",request.referenceSource},{"samples",referenceRows}};
        out["report"]=id(QUuid::createUuid());out["session"]=id(request.session);out["candidate"]=id(request.candidate);out["candidateRevision"]=id(request.candidateRevision);out["candidateDigest"]=handEyeDigest(c);
        out["dependencies"]=c["dependencies"];out["dependencyDigest"]=c["dependencyDigest"];out["trainingSetDigest"]=c["sampleSetDigest"];
        out["holdout"]=held;out["holdoutSetDigest"]=handEyeDigest(held);out["reference"]=reference;out["referenceSetDigest"]=handEyeDigest(reference);out["tolerance"]=tolerance;out["toleranceDigest"]=handEyeDigest(tolerance);
        out["metrics"]=m;out["closureMetrics"]=closure;out["percentileMethod"]="nearest-rank ceil(0.95*n)-1";out["rotationMethod"]="quaternion geodesic 2*atan2(norm(vector),abs(dot))";
        out["state"]=pass?"VALIDATED":"FAILED";out["reason"]=pass?"SOFTWARE_VERIFIED":"TOLERANCE_EXCEEDED";
        store.append("hand_eye_validation",out["report"].toString(),out);result.accepted=pass;result.reason=out["reason"].toString();
    }catch(const std::exception& e){result.reason=QString::fromUtf8(e.what());out["state"]="FAILED";out["reason"]=result.reason;}
    return result;
}
HandEyeActivationService::HandEyeActivationService(QString directory):directory_(QDir::cleanPath(directory)){safeRoot(directory_);}
HandEyeActivationResult HandEyeActivationService::activateProduction(const HandEyeActivationRequest&) const{return {false,"PHYSICAL_GATE_BLOCKED",{}};}
QJsonObject HandEyeActivationService::remembered(const QUuid& station) const {
    safeRoot(directory_);const auto path=activePath(directory_,station);require(!QFileInfo(path).isSymLink(),"ACTIVE_SYMLINK");if(!QFileInfo::exists(path))return {};
    QFile file(path);require(file.open(QIODevice::ReadOnly)&&file.size()<=64*1024,"ACTIVE_READ_FAILED");QJsonParseError error;
    const auto doc=QJsonDocument::fromJson(file.readAll(),&error);const auto envelope=doc.object();const auto o=envelope["active"].toObject();
    require(error.error==QJsonParseError::NoError&&doc.isObject()&&envelope.size()==2&&envelope["sha256"]==handEyeDigest(o),"ACTIVE_CORRUPT");
    require(o["format"]=="visionflow.hand-eye-active/1"&&o["station"]==id(station)&&uuid(o["revision"])&&uuid(o["candidateRevision"])&&o["evidence"]=="synthetic-fixture"&&o["productionPermitted"]==false,"ACTIVE_SCHEMA");
    ProductionStore store(directory_);const auto c=candidate(store,QUuid(o["candidate"].toString()),QUuid(o["candidateRevision"].toString()),QUuid(o["session"].toString()));
    const auto v=store.get("hand_eye_validation",o["validationReport"].toString());
    require(o["candidateDigest"]==handEyeDigest(c)&&o["validationDigest"]==handEyeDigest(v)&&v["state"]=="VALIDATED"&&v["candidateDigest"]==handEyeDigest(c),"ACTIVE_PROVENANCE");
    return o; // history only, deliberately no lease or Run/Auto state
}
HandEyeActivationResult HandEyeActivationService::activateFixture(const HandEyeActivationRequest& request,HandEyeActivationBoundary& boundary,Fault fault) const {
    HandEyeActivationResult result;
    try{
        safeRoot(directory_);require(QDir().mkpath(directory_+"/hand_eye_active"),"ACTIVE_DIRECTORY_FAILED");
        QLockFile lock(directory_+"/hand_eye_active/activation.lock");lock.setStaleLockTime(0);require(lock.tryLock(0),"ACTIVATION_BUSY");
        int callbacks=0;
        boundary.withExclusiveSnapshot([&](const HandEyeActivationSnapshot& current){
            require(++callbacks==1,"INVALID_BOUNDARY_CALLBACK");
            require(current.idle&&current.cameraDrained,"STATION_BUSY_OR_CAMERA_NOT_DRAINED");
            require(!request.station.isNull()&&current.station==request.station&&current.session==request.session&&current.candidate==request.candidate&&current.candidateRevision==request.candidateRevision,"STALE_CANDIDATE");
            require(current.dependencies.complete()&&request.dependenciesDigest==handEyeDigest(handEyeDependencyJson(current.dependencies)),"STALE_DEPENDENCIES");
            ProductionStore store(directory_);const auto c=candidate(store,request.candidate,request.candidateRevision,request.session);
            require(c["dependencyDigest"]==request.dependenciesDigest,"STALE_DEPENDENCIES");
            const auto v=store.get("hand_eye_validation",id(request.validationReport));
            require(v["format"]=="visionflow.hand-eye-validation/1"&&v["report"]==id(request.validationReport)&&v["state"]=="VALIDATED"&&v["evidence"]=="synthetic-fixture"&&v["physicalEligible"]==false&&v["productionPermitted"]==false,"VALIDATION_NOT_ACCEPTED");
            require(v["session"]==id(request.session)&&v["candidate"]==id(request.candidate)&&v["candidateRevision"]==id(request.candidateRevision)&&v["candidateDigest"]==handEyeDigest(c)&&v["dependencyDigest"]==request.dependenciesDigest,"STALE_VALIDATION");
            require(!current.referenceRevision.isNull()&&!current.toleranceRevision.isNull()&&v["reference"].toObject()["revision"]==id(current.referenceRevision)&&v["tolerance"].toObject()["revision"]==id(current.toleranceRevision),"STALE_REFERENCE_OR_TOLERANCE");
            const auto old=remembered(request.station);require(old.isEmpty()?request.expectedActiveRevision.isNull():old["revision"]==id(request.expectedActiveRevision),"STALE_ACTIVE_REVISION");
            const QJsonObject next{{"format","visionflow.hand-eye-active/1"},{"station",id(request.station)},{"session",id(request.session)},{"revision",id(QUuid::createUuid())},{"previousRevision",old.isEmpty()?QString():old["revision"].toString()},
                {"candidate",id(request.candidate)},{"candidateRevision",id(request.candidateRevision)},{"candidateDigest",handEyeDigest(c)},{"validationReport",id(request.validationReport)},{"validationDigest",handEyeDigest(v)},{"dependencyDigest",request.dependenciesDigest},{"evidence","synthetic-fixture"},{"productionPermitted",false}};
            const auto data=QJsonDocument(QJsonObject{{"active",next},{"sha256",handEyeDigest(next)}}).toJson(QJsonDocument::Compact);
            const auto path=activePath(directory_,request.station);require(!QFileInfo(path).isSymLink(),"ACTIVE_SYMLINK");
            QSaveFile file(path);file.setDirectWriteFallback(false);require(file.open(QIODevice::WriteOnly),"ACTIVE_OPEN_FAILED");if(fault)fault("after-open");
            const auto half=data.size()/2;
            require(file.write(data.constData(),half)==half,"ACTIVE_WRITE_FAILED");if(fault)fault("after-partial-write");
            require(file.write(data.constData()+half,data.size()-half)==data.size()-half,"ACTIVE_WRITE_FAILED");if(fault)fault("after-write");if(fault)fault("before-commit");
            require(file.commit(),"ACTIVE_COMMIT_FAILED");result.active=next;result.accepted=true;result.reason="SOFTWARE_ACTIVE";if(fault)fault("after-commit");
        });
        require(callbacks==1,"MISSING_EXCLUSIVE_BOUNDARY");
    }catch(const std::exception& e){
        // Once commit succeeded a postcommit notification failure cannot turn
        // a durable new selection into a reported rejected transaction.
        if(!result.accepted)result.reason=QString::fromUtf8(e.what());
    }
    return result;
}
}
