#include "PlaneMappingService.hpp"
#include <QJsonArray>
#include <QSet>
#include <opencv2/geometry.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double norm(QPointF p){return std::hypot(p.x(),p.y());}
bool finite(QPointF p){return std::isfinite(p.x()) && std::isfinite(p.y());}
double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}
QPointF project(const cv::Mat& h,QPointF p) {
    const auto x=h.at<double>(0,0)*p.x()+h.at<double>(0,1)*p.y()+h.at<double>(0,2);
    const auto y=h.at<double>(1,0)*p.x()+h.at<double>(1,1)*p.y()+h.at<double>(1,2);
    const auto w=h.at<double>(2,0)*p.x()+h.at<double>(2,1)*p.y()+h.at<double>(2,2);
    const auto bound=std::abs(h.at<double>(2,0)*p.x())+std::abs(h.at<double>(2,1)*p.y())+std::abs(h.at<double>(2,2));
    require(std::isfinite(w) && std::abs(w)>1e-10*std::max(bound,1e-12),"Homography horizon/invalid denominator");
    QPointF q{x/w,y/w};require(finite(q),"Nonfinite plane point");return q;
}
cv::Mat normalization(const QVector<QPointF>& points) {
    QPointF mean;for(auto p:points)mean+=p;mean/=points.size();double variance=0;for(auto p:points)variance+=norm(p-mean)*norm(p-mean);
    require(std::isfinite(variance) && variance>1e-15,"Degenerate point distribution");double scale=std::sqrt(2.0*points.size()/variance);
    cv::Mat h=cv::Mat::eye(3,3,CV_64F);h.at<double>(0,0)=h.at<double>(1,1)=scale;h.at<double>(0,2)=-scale*mean.x();h.at<double>(1,2)=-scale*mean.y();return h;
}
QJsonArray pointsJson(const QVector<QPointF>& points){QJsonArray a;for(auto p:points)a.append(QJsonArray{p.x(),p.y()});return a;}
double number(const QJsonObject& o,const char* key){require(o[key].isDouble() && std::isfinite(o[key].toDouble()),"Invalid plane number");return o[key].toDouble();}
void keys(const QJsonObject& o,const QSet<QString>& expected){require(o.size()==expected.size(),"Missing/unknown plane fields");for(auto it=o.begin();it!=o.end();++it)require(expected.contains(it.key()),"Unknown plane field");}
QVector<QPointF> readPoints(const QJsonValue& v){require(v.isArray(),"Invalid point array");auto a=v.toArray();require(a.size()<=512,"Plane point limit exceeded");QVector<QPointF> result;for(auto item:a){require(item.isArray(),"Invalid point");const auto p=item.toArray();require(p.size()==2 && p[0].isDouble() && p[1].isDouble(),"Invalid point components");QPointF q{p[0].toDouble(),p[1].toDouble()};require(finite(q),"Nonfinite point");result.append(q);}return result;}
}
PlaneMapping PlaneMappingService::fit(const ChessboardObservation& o,double limit) {
    const auto& t=o.target;const auto n=o.pixels.size();
    require(!t.id.trimmed().isEmpty() && t.id.size()<=128 && t.columns>=3 && t.rows>=3 && t.columns<=32 && t.rows<=32 && n==t.columns*t.rows && n<=512 && o.worldMillimetres.size()==n,"Invalid chessboard geometry/count");
    require(std::isfinite(t.squareMillimetres) && t.squareMillimetres>0 && t.squareMillimetres<=1000000,"Invalid target spacing");
    require(std::isfinite(limit) && limit>0 && limit<=100,"Fit pixel bound must be in (0,100]");
    require(o.resolution.width()>0 && o.resolution.height()>0 && qint64(o.resolution.width())*o.resolution.height()<=16000000 &&
        !o.source.sourceId.isEmpty() && !o.source.frameId.isEmpty() && !o.source.cycleId.isEmpty() && (o.source.coordinateSystemId=="image" || o.source.coordinateSystemId==o.source.sourceId+"/image") && o.source.completedAtUtc.isValid(),"Raw image provenance/resolution required");
    std::vector<cv::Point2d> image,world;std::vector<cv::Point2f> hullInput;
    for(int i=0;i<n;++i){auto p=o.pixels[i],q=o.worldMillimetres[i];require(finite(p) && p.x()>=0 && p.y()>=0 && p.x()<=o.resolution.width()-1 && p.y()<=o.resolution.height()-1,"Fit pixel outside raw image");
        require(q==QPointF((i%t.columns)*t.squareMillimetres,(i/t.columns)*t.squareMillimetres),"Target ordering/world grid mismatch");
        for(int j=0;j<i;++j)require(norm(p-o.pixels[j])>0.5,"Duplicate/unresolved fit corners");image.emplace_back(p.x(),p.y());world.emplace_back(q.x(),q.y());hullInput.emplace_back(p.x(),p.y());}
    // Every cell must remain convex and have consistent orientation: no fold/order swaps.
    double orientation=0;
    for(int r=0;r<t.rows-1;++r)for(int c=0;c<t.columns-1;++c){int i=r*t.columns+c;QVector<QPointF> cell{o.pixels[i],o.pixels[i+1],o.pixels[i+t.columns+1],o.pixels[i+t.columns]};
        for(int k=0;k<4;++k){double sign=cross(cell[(k+1)%4]-cell[k],cell[(k+2)%4]-cell[(k+1)%4]);require(std::abs(sign)>1,"Degenerate cell/insufficient distribution");if(orientation==0)orientation=sign;require(sign*orientation>0,"Folded/wrong corner ordering");}}
    std::vector<cv::Point2f> hull;cv::convexHull(hullInput,hull);require(hull.size()>=4 && std::abs(cv::contourArea(hull))>=64,"Insufficient calibration area");
    auto h=cv::findHomography(image,world,0);require(!h.empty() && cv::checkRange(h),"Homography fit failed");h.convertTo(h,CV_64F);
    auto normalized=normalization(o.worldMillimetres)*h*normalization(o.pixels).inv();cv::SVD svd(normalized);
    require(svd.w.at<double>(2)>0 && svd.w.at<double>(0)/svd.w.at<double>(2)<1e8,"Singular/unstable homography");
    require(std::abs(h.at<double>(2,2))>1e-12,"Homography cannot be normalized");h/=h.at<double>(2,2);const auto inverse=h.inv();
    PlaneMapping result;result.observation_=o;result.fitLimit_=limit;for(int i=0;i<9;++i)result.h_[i]=h.at<double>(i/3,i%3);
    double denominatorSign=0;
    for(auto p:hull){result.hull_.append({p.x,p.y});const double w=h.at<double>(2,0)*p.x+h.at<double>(2,1)*p.y+1;if(denominatorSign==0)denominatorSign=w;require(w*denominatorSign>0,"Horizon crosses calibration region");project(h,{p.x,p.y});}
    for(int i=0;i<n;++i){const double mm=norm(project(h,o.pixels[i])-o.worldMillimetres[i]),px=norm(project(inverse,o.worldMillimetres[i])-o.pixels[i]);require(px<=limit,"Fit inverse pixel error exceeds declared bound (check target/lens)");result.fitRms_+=mm*mm;result.inverseRms_+=px*px;result.inverseMax_=std::max(result.inverseMax_,px);}
    result.fitRms_=std::sqrt(result.fitRms_/n);result.inverseRms_=std::sqrt(result.inverseRms_/n);return result;
}
QPointF PlaneMappingService::map(const PlaneMapping& m,QPointF p) {
    require(finite(p),"Nonfinite raw point");const auto size=m.observation_.resolution;require(p.x()>=0 && p.y()>=0 && p.x()<=size.width()-1 && p.y()<=size.height()-1,"Point outside raw resolution");
    std::vector<cv::Point2f> hull;for(auto q:m.hull_)hull.emplace_back(q.x(),q.y());require(cv::pointPolygonTest(hull,cv::Point2f(p.x(),p.y()),true)>=-1e-4,"Point outside calibrated inner-corner region; extrapolation disabled");
    cv::Mat h(3,3,CV_64F,const_cast<double*>(m.h_.data()));return project(h,p);
}
PlaneDistance PlaneMappingService::distance(const PlaneMapping& m,QPointF a,QPointF b){auto x=map(m,a),y=map(m,b);return {x,y,norm(a-b),norm(x-y)};}
QJsonObject PlaneMappingService::encode(const PlaneMapping& m) {
    const auto& o=m.observation_;const auto& t=o.target;const auto& s=o.source;QJsonArray h;for(double v:m.h_)h.append(v);
    return {{"schemaVersion",1},{"model","raw_planar_homography/1"},{"solver","opencv5.direct_homography/1"},{"coordinateSpace","raw_pixel_centres"},
      {"target",QJsonObject{{"id",t.id},{"columns",t.columns},{"rows",t.rows},{"squareMm",t.squareMillimetres},{"origin","corner_1"},{"axes","columns_rows"}}},
      {"source",QJsonObject{{"sourceId",s.sourceId},{"frameId",s.frameId},{"cycleId",s.cycleId},{"coordinateSystemId",s.coordinateSystemId},{"completedAtUtc",s.completedAtUtc.toString(Qt::ISODateWithMs)}}},
      {"width",o.resolution.width()},{"height",o.resolution.height()},{"pixels",pointsJson(o.pixels)},{"worldMm",pointsJson(o.worldMillimetres)},{"regionPixels",pointsJson(m.hull_)},
      {"H",h},{"fitRmsMm",m.fitRms_},{"inverseRmsPx",m.inverseRms_},{"maxInversePx",m.inverseMax_},{"fitLimitPx",m.fitLimit_}};
}
PlaneMapping PlaneMappingService::decode(const QJsonObject& j) {
    keys(j,{"schemaVersion","model","solver","coordinateSpace","target","source","width","height","pixels","worldMm","regionPixels","H","fitRmsMm","inverseRmsPx","maxInversePx","fitLimitPx"});
    require(number(j,"schemaVersion")==1 && j["model"].toString()=="raw_planar_homography/1" && j["solver"].toString()=="opencv5.direct_homography/1" && j["coordinateSpace"].toString()=="raw_pixel_centres","Unsupported plane schema/solver/coordinate space");
    const auto t=j["target"].toObject(),s=j["source"].toObject();keys(t,{"id","columns","rows","squareMm","origin","axes"});keys(s,{"sourceId","frameId","cycleId","coordinateSystemId","completedAtUtc"});
    require(t["origin"].toString()=="corner_1" && t["axes"].toString()=="columns_rows","Unsupported target axes/origin");
    auto integer=[](const QJsonObject& o,const char* key){double v=number(o,key);require(v>=1 && v<=16000000 && std::floor(v)==v,"Invalid plane integer");return int(v);};
    ChessboardObservation o{{t["id"].toString(),integer(t,"columns"),integer(t,"rows"),number(t,"squareMm")},{},{integer(j,"width"),integer(j,"height")},readPoints(j["pixels"]),readPoints(j["worldMm"])};
    o.source.sourceId=s["sourceId"].toString();o.source.frameId=s["frameId"].toString();o.source.cycleId=s["cycleId"].toString();o.source.coordinateSystemId=s["coordinateSystemId"].toString();o.source.completedAtUtc=QDateTime::fromString(s["completedAtUtc"].toString(),Qt::ISODateWithMs);
    const auto replay=fit(o,number(j,"fitLimitPx"));require(encode(replay)==j,"Plane matrix/fit/evidence replay mismatch");return replay;
}
}
