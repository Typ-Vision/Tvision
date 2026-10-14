#include "CircleFitter.hpp"
#include <opencv2/core.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <set>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool finite(QPointF p){return std::isfinite(p.x()) && std::isfinite(p.y());}
double norm(QPointF p){return std::hypot(p.x(),p.y());}
struct Model {QPointF centre;double radius;};
}
FittedCircle CircleFitter::fit(const QVector<CircleFitPoint>& scans,const CircleFitOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Circle fit cancelled");};stop();
    auto positive=[](double v){return std::isfinite(v) && v>0;};
    require(o.minPoints>=6 && o.minPoints<=4096 && positive(o.minCoverage) && o.minCoverage<=1 &&
        positive(o.minAngularSpanDegrees) && o.minAngularSpanDegrees<360 && positive(o.maxCondition) && o.maxCondition>1 &&
        positive(o.inlierDistance) && positive(o.maxRms) && positive(o.maxResidual) &&
        o.ransacIterations>=1 && o.ransacIterations<=1024,"Invalid circle fit options");
    require(scans.size()>=o.minPoints && scans.size()<=4096,"Invalid circle support size");
    QVector<QPointF> points;QVector<int> ids;std::set<std::pair<double,double>> unique;
    for(int i=0;i<scans.size();++i){stop();if(scans[i].point){auto p=*scans[i].point;
        require(finite(p) && std::abs(p.x())<=1e12 && std::abs(p.y())<=1e12,"Invalid circle point");
        require(unique.emplace(p.x(),p.y()).second,"Duplicate circle observation");points.append(p);ids.append(i);}}
    require(points.size()>=o.minPoints,"Insufficient found circle points");
    // Difference accumulation avoids summing large absolute coordinates.
    QPointF offset=points.front(),shift;for(auto p:points)shift+=p-offset;offset+=shift/points.size();
    double scale=0;for(auto p:points)scale=std::max(scale,norm(p-offset));
    require(positive(scale) && scale>1e-12 && positive(o.inlierDistance/scale) && o.inlierDistance/scale>=1e-12,"Degenerate circle scale/threshold");
    QVector<QPointF> local;for(auto p:points)local.append((p-offset)/scale);
    // Each consensus gets its own normalization: rejected far-away points must
    // not consume precision in the final SVD or geometric refinement.
    auto solve=[&](const QVector<int>& indices,bool refine)->std::optional<Model>{
        stop();const auto base=points[indices.front()];QPointF mean;
        for(auto i:indices){stop();mean+=points[i]-base;}mean=base+mean/indices.size();
        double spread=0;for(auto i:indices)spread=std::max(spread,norm(points[i]-mean));if(!positive(spread) || spread<=1e-12)return {};
        QVector<QPointF> data;cv::Mat a(int(indices.size()),3,CV_64F),b(int(indices.size()),1,CV_64F);
        for(int j=0;j<indices.size();++j){stop();auto p=(points[indices[j]]-mean)/spread;data.append(p);
            a.at<double>(j,0)=2*p.x();a.at<double>(j,1)=2*p.y();a.at<double>(j,2)=1;b.at<double>(j)=p.x()*p.x()+p.y()*p.y();}
        cv::SVD svd(a,cv::SVD::NO_UV);stop();if(svd.w.at<double>(2)<=svd.w.at<double>(0)*1e-10)return {};
        cv::Mat x;if(!cv::solve(a,b,x,cv::DECOMP_SVD))return {};stop();
        Model m{{x.at<double>(0),x.at<double>(1)},0};const double rr=x.at<double>(2)+m.centre.x()*m.centre.x()+m.centre.y()*m.centre.y();
        if(!finite(m.centre) || !positive(rr))return {};m.radius=std::sqrt(rr);
        auto objective=[&](const Model& model){double sum=0;for(auto p:data){stop();double e=norm(p-model.centre)-model.radius;sum+=e*e;}return sum;};
        if(refine){double lambda=1e-3,cost=objective(m);bool converged=false;
            for(int iteration=0;iteration<64;++iteration){stop();cv::Matx33d h=cv::Matx33d::zeros();cv::Vec3d gradient(0,0,0);
                for(auto p:data){stop();auto d=m.centre-p;double length=norm(d);if(length<=1e-12)return {};
                    cv::Vec3d j(d.x()/length,d.y()/length,-1);double e=length-m.radius;gradient+=j*e;
                    for(int row=0;row<3;++row)for(int col=0;col<3;++col)h(row,col)+=j[row]*j[col];}
                if(cv::norm(gradient)<=1e-12*data.size()){converged=true;break;}
                auto damped=h;for(int k=0;k<3;++k)damped(k,k)+=lambda*std::max(h(k,k),1e-12);
                cv::Vec3d delta;if(!cv::solve(damped,-gradient,delta,cv::DECOMP_CHOLESKY))return {};stop();
                Model next{m.centre+QPointF(delta[0],delta[1]),m.radius+delta[2]};
                const double nextCost=finite(next.centre) && positive(next.radius) ? objective(next):INFINITY;
                if(nextCost<=cost){const double improvement=cost-nextCost;m=next;cost=nextCost;lambda=std::max(1e-12,lambda*.25);
                    if(cv::norm(delta)<=1e-11*(1+norm(m.centre)+m.radius) || improvement<=1e-15*std::max(1.,cost)){converged=true;break;}}
                else {lambda*=10;if(lambda>1e15)return {};}
            }
            if(!converged)return {};
        }
        return Model{(mean-offset)/scale+m.centre*(spread/scale),m.radius*(spread/scale)};
    };
    auto members=[&](const Model& m){QVector<int> list;for(int i=0;i<local.size();++i){stop();if(std::abs(norm(local[i]-m.centre)-m.radius)*scale<=o.inlierDistance)list.append(i);}return list;};
    auto error=[&](const Model& m,const QVector<int>& list){double sum=0;for(auto i:list){stop();double e=norm(local[i]-m.centre)-m.radius;sum+=e*e;}return sum;};
    QVector<int> all;for(int i=0;i<points.size();++i)all.append(i);
    std::optional<Model> best;QVector<int> inliers;double bestError=INFINITY;
    auto consider=[&](const std::optional<Model>& candidate){if(!candidate)return;auto list=members(*candidate);double e=error(*candidate,list);
        if(list.size()>inliers.size() || (list.size()==inliers.size() && e<bestError)){best=candidate;inliers=std::move(list);bestError=e;}};
    consider(solve(all,true));
    if(o.ransac){std::mt19937 rng(0x56464332u);for(int k=0;k<o.ransacIterations;++k){stop();
        int a=int(rng()%points.size()),b=int(rng()%points.size()),c=int(rng()%points.size());if(a==b || a==c || b==c)continue;
        consider(solve(QVector<int>{a,b,c},false));}}
    require(best && inliers.size()>=o.minPoints,"Insufficient circle consensus");
    bool stable=false;for(int pass=0;pass<8;++pass){auto model=solve(inliers,true);require(bool(model),"Circle refinement failed");best=model;
        auto next=members(*best);require(next.size()>=o.minPoints,"Insufficient circle refit consensus");if(next==inliers){stable=true;break;}inliers=std::move(next);}
    require(stable,"Unstable circle consensus");
    FittedCircle r;r.centre=offset+best->centre*scale;r.radius=best->radius*scale;r.scans=scans;
    require(finite(r.centre) && positive(r.radius),"Invalid fitted circle");
    for(auto& s:r.scans){s.used=false;s.signedResidual.reset();s.reason=s.point ? "outlier":"not_found";}
    for(int i=0;i<local.size();++i){stop();r.scans[ids[i]].signedResidual=(norm(local[i]-best->centre)-best->radius)*scale;}
    cv::Mat jacobian(int(inliers.size()),3,CV_64F);QVector<double> angles;double sum=0;
    for(int j=0;j<inliers.size();++j){stop();int i=inliers[j];auto d=local[i]-best->centre;double length=norm(d);require(length>1e-12,"Singular circle Jacobian");
        jacobian.at<double>(j,0)=d.x()/length;jacobian.at<double>(j,1)=d.y()/length;jacobian.at<double>(j,2)=-1;
        angles.append(std::atan2(d.y(),d.x()));auto& s=r.scans[ids[i]];s.used=true;s.reason.clear();double e=*s.signedResidual;sum+=e*e;r.maxResidual=std::max(r.maxResidual,std::abs(e));}
    cv::SVD svd(jacobian,cv::SVD::NO_UV);stop();const double lo=svd.w.at<double>(2),hi=svd.w.at<double>(0);
    require(lo>hi*1e-10,"Rank deficient circle Jacobian");r.condition=(hi/lo)*(hi/lo);
    std::sort(angles.begin(),angles.end());double gap=angles.front()+2*std::numbers::pi-angles.back();
    for(int i=1;i<angles.size();++i)gap=std::max(gap,angles[i]-angles[i-1]);
    r.angularSpanDegrees=360-gap*180/std::numbers::pi;r.rms=std::sqrt(sum/inliers.size());
    r.usedCount=inliers.size();r.rejectedCount=points.size()-inliers.size();r.missingCount=scans.size()-points.size();r.coverage=double(r.usedCount)/scans.size();
    require(std::isfinite(r.condition) && r.condition<=o.maxCondition,"Ill-conditioned circle support");
    require(r.angularSpanDegrees>=o.minAngularSpanDegrees,"Insufficient circle angular span");
    require(r.coverage>=o.minCoverage,"Insufficient circle scan coverage");
    require(std::isfinite(r.rms) && r.rms<=o.maxRms && r.maxResidual<=o.maxResidual,"Circle residual limits exceeded");
    stop();return r;
}
}
