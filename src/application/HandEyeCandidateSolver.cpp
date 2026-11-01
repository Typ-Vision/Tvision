#include "HandEyeCandidateSolver.hpp"
#include "ProductionStore.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <opencv2/calib.hpp>
#include <opencv2/geometry.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace visionflow {
namespace {
using M = cv::Matx44d;
constexpr double rad = std::numbers::pi / 180;
void require(bool b, const char* why) { if (!b) throw std::runtime_error(why); }
QString id(const QUuid& u) { return u.toString(QUuid::WithoutBraces); }
QString digest(const QJsonObject& o) { return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex()); }
cv::Mat rotation(const M& m) { cv::Mat r(3,3,CV_64F); for(int i=0;i<3;++i)for(int j=0;j<3;++j)r.at<double>(i,j)=m(i,j); return r; }
cv::Mat translation(const M& m) { cv::Mat t(3,1,CV_64F);for(int i=0;i<3;++i)t.at<double>(i)=m(i,3);return t; }
M matrix(const SpatialTransform& s) {
    s.validate(); auto q=s.rotation.normalized(); const double w=q.w,x=q.x,y=q.y,z=q.z;
    return M(1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),s.translationMm[0],
        2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),s.translationMm[1],
        2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),s.translationMm[2],0,0,0,1);
}
M compose(const cv::Mat& r,const cv::Mat& t) { M m=M::eye(); for(int i=0;i<3;++i){m(i,3)=t.at<double>(i);for(int j=0;j<3;++j)m(i,j)=r.at<double>(i,j);} return m; }
cv::Vec3d logR(const cv::Mat& r) { cv::Mat v;cv::Rodrigues(r,v);return {v.at<double>(0),v.at<double>(1),v.at<double>(2)}; }
SpatialTransform spatial(const M& m,FrameRef source,FrameRef target) {
    auto v=logR(rotation(m)); double a=cv::norm(v);Rotation3D q;
    if(a>1e-14){double k=std::sin(a/2)/a;q={std::cos(a/2),k*v[0],k*v[1],k*v[2]};}
    SpatialTransform s{source,target,{m(0,3),m(1,3),m(2,3)},q.normalized()};s.validate();return s;
}
QJsonArray singular(const cv::Mat& a) { cv::Mat w;cv::SVD::compute(a,w);QJsonArray values;for(int i=0;i<w.rows;++i)values.append(w.at<double>(i));return values; }
QJsonObject dependencies(const CalibrationDependencySnapshot& d) {
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
M mean(const std::vector<M>& values) {
    cv::Mat r=rotation(values.front()); bool converged=false;
    for(int n=0;n<100;++n){cv::Vec3d delta{};for(const auto& v:values)delta+=logR(r.t()*rotation(v));delta*=1.0/values.size();
        if(cv::norm(delta)<1e-12){converged=true;break;}cv::Mat step;cv::Rodrigues(delta,step);r=r*step;}
    require(converged,"MEAN_DID_NOT_CONVERGE");cv::Mat t=cv::Mat::zeros(3,1,CV_64F);for(const auto& v:values)t+=translation(v);t/=double(values.size());return compose(r,t);
}
}
HandEyeSolveResult HandEyeCandidateSolver::solveFixture(const HandEyeFixtureRequest& input,const QString& directory) const {
    const auto request=input; // owned snapshot; no mutable source data retained
    HandEyeSolveResult result; auto& report=result.report; QJsonArray rejected;
    report={{"format","visionflow.hand-eye-candidate/1"},{"evidence","synthetic-fixture"},{"physicalEligible",false},
      {"requestedEvidence",int(request.evidence)},{"algorithm","OpenCV CALIB_HAND_EYE_PARK"},{"openCV",CV_VERSION},{"session",id(request.session)}};
    try {
        require(request.evidence==EvidenceState::Synthetic,"PHYSICAL_GATE_BLOCKED");
        require(!request.session.isNull() && request.dependencies.complete(),"INCOMPLETE_PROVENANCE");
        require(request.policy.valid(),"INVALID_POLICY");
        require(request.dependencies.mode==HandEyeMode::EyeInHand || request.dependencies.mode==HandEyeMode::EyeToHand,"INVALID_MODE");
        request.base.validate();request.flange.validate();request.camera.validate();request.plate.validate();
        const auto& binding=request.dependencies.pose;
        require(QUuid(request.base.frameUuid)==binding.targetFrame && QUuid(request.flange.frameUuid)==binding.sourceFrame,"FRAME_CONTRACT_MISMATCH");
        QSet<QString> frames{request.base.frameUuid,request.flange.frameUuid,request.camera.frameUuid,request.plate.frameUuid};require(frames.size()==4,"FRAME_ALIAS");
        require(std::isfinite(request.plateDiagonalMm)&&request.plateDiagonalMm>0,"INVALID_PLATE_DIAGONAL");
        require(request.samples.size()<=256,"DATASET_LIMIT");
        const auto dep=dependencies(request.dependencies);report["dependencies"]=dep;report["dependencyDigest"]=digest(dep);
        report["policyRevision"]=request.policy.revision;report["policyDigest"]=request.policy.digest;
        std::vector<M> g,c;std::vector<cv::Mat> rg,tg,rc,tc;QSet<QUuid> sampleIds,frameIds;QJsonArray samples;
        for(const auto& s:request.samples) {
            require(!s.sample.isNull()&&!s.frame.isNull()&&!sampleIds.contains(s.sample)&&!frameIds.contains(s.frame),"DUPLICATE_OR_MISSING_SAMPLE_FRAME");sampleIds.insert(s.sample);frameIds.insert(s.frame);
            require(s.baseTFlange.target==request.base && s.baseTFlange.source==request.flange && s.cameraTPlate.target==request.camera && s.cameraTPlate.source==request.plate,"SAMPLE_FRAME_DIRECTION_MISMATCH");
            s.baseTFlange.validate();s.cameraTPlate.validate();
            if(s.holdout)continue;
            samples.append(QJsonObject{{"sample",id(s.sample)},{"frame",id(s.frame)},{"baseTFlange",s.baseTFlange.toJson()},{"cameraTPlate",s.cameraTPlate.toJson()},
              {"rmsPx",s.rmsPx},{"maxPx",s.maxPx},{"ambiguityMarginPx",s.ambiguityMarginPx}});
            if(!std::isfinite(s.rmsPx)||!std::isfinite(s.maxPx)||!std::isfinite(s.ambiguityMarginPx)||s.rmsPx<0||s.maxPx<s.rmsPx||s.rmsPx>request.policy.maxPlateRmsPx||s.maxPx>request.policy.maxPlateMaxPx||s.ambiguityMarginPx<request.policy.minAmbiguityMarginPx)
                rejected.append(QJsonObject{{"sample",id(s.sample)},{"reason","PLATE_QUALITY_OR_AMBIGUITY"}});
            auto gm=matrix(s.baseTFlange);g.push_back(gm);c.push_back(matrix(s.cameraTPlate));
            if(request.dependencies.mode==HandEyeMode::EyeToHand)gm=gm.inv();
            rg.push_back(rotation(gm));tg.push_back(translation(gm));rc.push_back(rotation(c.back()));tc.push_back(translation(c.back()));
        }
        QJsonObject dataset{{"samples",samples},{"plateDiagonalMm",request.plateDiagonalMm},{"base",request.base.toJson()},{"flange",request.flange.toJson()},{"camera",request.camera.toJson()},{"plate",request.plate.toJson()}};report["dataset"]=dataset;report["sampleSetDigest"]=digest(dataset);report["rejectedSamples"]=rejected;
        require(rejected.isEmpty(),"PLATE_QUALITY_OR_AMBIGUITY");require(int(g.size())>=request.policy.minSolveSamples,"INSUFFICIENT_SOLVE_SAMPLES");
        cv::Mat logs,translationSystem;double extent=0;std::vector<cv::Vec3d> axes;int ignored=0;
        for(size_t i=0;i<g.size();++i)for(size_t j=i+1;j<g.size();++j) {
            extent=std::max(extent,cv::norm(translation(g[i])-translation(g[j])));
            M a=request.dependencies.mode==HandEyeMode::EyeInHand ? g[j].inv()*g[i] : g[j]*g[i].inv();
            auto r=rotation(a);cv::Mat block=r-cv::Mat::eye(3,3,CV_64F);translationSystem.push_back(block);
            auto v=logR(r);auto angle=cv::norm(v);if(angle+1e-14<request.policy.minRotationSpreadDeg*rad){++ignored;continue;}
            cv::Mat row(1,3,CV_64F);for(int k=0;k<3;++k)row.at<double>(0,k)=v[k];logs.push_back(row);axes.push_back(v/angle);
        }
        report["ignoredSmallRotationPairs"]=ignored;report["translationCoverage"]=extent/request.plateDiagonalMm;
        require(logs.rows>=2,"ROTATION_DIVERSITY");
        const auto rs=singular(logs),ts=singular(translationSystem);report["rotationSingularValues"]=rs;report["translationSingularValues"]=ts;
        double separation=0;for(const auto& a:axes)for(const auto& b:axes)separation=std::max(separation,std::acos(std::clamp(std::abs(a.dot(b)),0.0,1.0))/rad);
        report["axisSeparationDeg"]=separation;report["rotationConditionRatio"]=rs[1].toDouble()/rs[0].toDouble();report["translationConditionRatio"]=ts[2].toDouble()/ts[0].toDouble();
        require(separation>=request.policy.minAxisSeparationDeg && rs[1].toDouble()/rs[0].toDouble()>=request.policy.minRotationConditionRatio,"ROTATION_DEGENERATE");
        require(ts[2].toDouble()>=request.policy.minTranslationCondition && ts[2].toDouble()/ts[0].toDouble()>=request.policy.minTranslationRatio,"TRANSLATION_DEGENERATE");
        require(extent/request.plateDiagonalMm>=request.policy.minTranslationCoverage,"TRANSLATION_COVERAGE");
        cv::Mat xr,xt;cv::calibrateHandEye(rg,tg,rc,tc,xr,xt,cv::CALIB_HAND_EYE_PARK);
        require(cv::checkRange(xr)&&cv::checkRange(xt)&&std::abs(cv::determinant(xr)-1)<1e-8 && cv::norm(xr.t()*xr-cv::Mat::eye(3,3,CV_64F))<1e-8,"SOLVER_NON_RIGID");
        const M x=compose(xr,xt);std::vector<M> constants;
        for(size_t i=0;i<g.size();++i)constants.push_back(request.dependencies.mode==HandEyeMode::EyeInHand ? g[i]*x*c[i] : g[i].inv()*x*c[i]);
        const M fixed=mean(constants);double maxT=0,maxR=0,sumT=0,sumR=0;
        for(const auto& value:constants){double t=cv::norm(translation(value)-translation(fixed));double r=cv::norm(logR(rotation(fixed).t()*rotation(value)));maxT=std::max(maxT,t);maxR=std::max(maxR,r);sumT+=t*t;sumR+=r*r;}
        report["maxClosureMm"]=maxT;report["maxClosureRad"]=maxR;report["rmsClosureMm"]=std::sqrt(sumT/g.size());report["rmsClosureRad"]=std::sqrt(sumR/g.size());
        // Synthetic oracle only; these limits never become physical tolerances.
        require(maxT<=1e-6 && maxR<=1e-7,"INCONSISTENT_CONVENTION_OR_DATASET");
        const bool inHand=request.dependencies.mode==HandEyeMode::EyeInHand;
        report["transform"]=spatial(x,request.camera,inHand?request.flange:request.base).toJson();
        report["fixedTransform"]=spatial(fixed,request.plate,inHand?request.base:request.flange).toJson();
        report["candidate"]=id(QUuid::createUuid());report["state"]="CANDIDATE";
        // Append-only evidence store; archive failure never returns accepted.
        ProductionStore(directory).append("hand_eye_candidate",report["candidate"].toString(),report);
        result.accepted=true;result.reason="CANDIDATE";
    }catch(const std::exception& e){result.reason=QString::fromUtf8(e.what());report["state"]="FAILED";report["reason"]=result.reason;}
    return result;
}
}
