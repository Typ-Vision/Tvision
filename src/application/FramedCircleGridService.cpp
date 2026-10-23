#include "FramedCircleGridService.hpp"
#include "vision/ImageAccess.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <QElapsedTimer>
#include <QJsonArray>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
constexpr auto version="framed_circle_grid/1";
struct Cancelled {};
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
QPointF qp(cv::Point2f p){return {p.x,p.y};}
double distance(cv::Point2f a,cv::Point2f b){return cv::norm(a-b);}
cv::Point2f mapped(const cv::Mat& h,cv::Point2f p){
    double w=h.at<double>(2,0)*p.x+h.at<double>(2,1)*p.y+h.at<double>(2,2);
    return {float((h.at<double>(0,0)*p.x+h.at<double>(0,1)*p.y+h.at<double>(0,2))/w),float((h.at<double>(1,0)*p.x+h.at<double>(1,1)*p.y+h.at<double>(1,2))/w)};
}
struct Region {std::vector<cv::Point2f> corners;cv::Point2f marker;int corner=-1;cv::Mat toUnit;};
struct Dot {cv::Point2f p,unit;double radius=0,edge=0;};
// Subpixel iso-intensity contour from the ORIGINAL raster. No warp coordinates
// enter the measurement; bilinear gradient interpolates each threshold crossing.
double sample(const cv::Mat& gray,cv::Point2f p){
    int x=int(std::floor(p.x)),y=int(std::floor(p.y));
    if(x<0 || y<0 || x+1>=gray.cols || y+1>=gray.rows)return 0;
    double dx=p.x-x,dy=p.y-y;
    return (1-dy)*((1-dx)*gray.at<uchar>(y,x)+dx*gray.at<uchar>(y,x+1))+dy*((1-dx)*gray.at<uchar>(y+1,x)+dx*gray.at<uchar>(y+1,x+1));
}
std::optional<Dot> refine(const cv::Mat& gray,const std::vector<cv::Point>& contour,double level){
    if(contour.size()<12)return {};
    const double area=std::abs(cv::contourArea(contour));if(area<20)return {};
    auto ellipse=cv::fitEllipse(contour);double a=ellipse.size.width,b=ellipse.size.height;
    if(std::min(a,b)<5 || std::max(a,b)/std::min(a,b)>2.5 || area/(CV_PI*a*b/4)<.86 || area/(CV_PI*a*b/4)>1.12)return {};
    std::vector<cv::Point2f> points;double edge=0;
    for(auto p:contour){cv::Point2f q(p);double gx=(sample(gray,q+cv::Point2f(1,0))-sample(gray,q-cv::Point2f(1,0)))/2;
        double gy=(sample(gray,q+cv::Point2f(0,1))-sample(gray,q-cv::Point2f(0,1)))/2,g2=gx*gx+gy*gy;
        if(g2<1)continue;double shift=(level-sample(gray,q))/g2;
        cv::Point2f delta(float(shift*gx),float(shift*gy));if(cv::norm(delta)>1.5)continue;
        points.push_back(q+delta);edge+=std::sqrt(g2);
    }
    if(points.size()<contour.size()*.75)return {};
    auto fit=cv::fitEllipse(points);if(distance(fit.center,ellipse.center)>1.5)return {};
    // Reject flat/saturated or grossly blurred edges, relative to dot contrast.
    double inside=sample(gray,fit.center),outside=0;
    for(int k=0;k<16;++k){double theta=k*CV_PI/8;outside+=sample(gray,fit.center+cv::Point2f(float(std::cos(theta)*std::max(a,b)),float(std::sin(theta)*std::max(a,b))))/16;}
    double contrast=outside-inside,meanEdge=edge/points.size();
    if(contrast<8 || meanEdge<contrast*.12)return {};
    // Radial ellipse residual rejects dirt and partial/occluded disks.
    double residual=0,angle=fit.angle*CV_PI/180;
    for(auto p:points){auto d=p-fit.center;double x=(d.x*std::cos(angle)+d.y*std::sin(angle))/(fit.size.width/2),y=(-d.x*std::sin(angle)+d.y*std::cos(angle))/(fit.size.height/2);residual+=std::pow(std::hypot(x,y)-1,2);}
    if(std::sqrt(residual/points.size())>.045)return {};
    return Dot{fit.center,{},std::sqrt(a*b)/2,meanEdge};
}
}
PlateDetectionResult FramedCircleGridService::observe(const ImageFrame& frame,const CalibrationGridTarget& target,const std::shared_ptr<std::atomic_bool>& cancel,const std::optional<QRect>& searchRoi){
    PlateDetectionDiagnostics diag;diag.algorithmVersion=version;
    diag.parameters={{"searchMaxDimension",1200},{"searchScales",QJsonArray{1,.5}},{"thresholdLevels",15},{"frameBudget",16},{"candidateBudget",512},
        {"contourBudgetPerThreshold",20000},{"minFrameAreaRatio",.015},{"holeAreaRatio",QJsonArray{.55,.95}},{"outerApproxPerimeterRatio",.012},{"holeApproxPerimeterRatio",.006},
        {"markerDiagonalRange",QJsonArray{.035,.22}},{"maxMarkerCornerDistance",.23},{"clusterTolerance",.035},{"clusterMinSupport",5},{"assignmentTolerance",.045},
        {"gridSpacingRange",QJsonArray{.07,.16}},{"maxSpacingDeviationRatio",.22},{"maxEllipseAxisRatio",2.5},{"maxEllipseResidual",.045},{"minContrast",8},{"minEdgeContrastRatio",.12}};
    diag.parameters["radiusMedianRange"]=QJsonArray{.65,1.5};
    diag.parameters["axesConvention"]="clockwise_columns_from_marked_corner_in_raw_raster";
    diag.parameters["mirrorEvidence"]="raw_cell_winding_only; physical_reflection_not_observable_with_symmetric_marker";
    QElapsedTimer timer;timer.start();qint64 previous=0;
    auto stage=[&](const char* name){auto now=timer.nsecsElapsed();diag.stageMilliseconds[name]=(now-previous)/1e6;previous=now;};
    auto stop=[&]{if(cancel && cancel->load())throw Cancelled{};};
    auto fail=[&](PlateDetectionStatus s,const QString& why){if(diag.assignedCandidates.isEmpty())diag.rejectedCandidates=diag.partialCandidates;diag.stageMilliseconds["total"]=timer.nsecsElapsed()/1e6;return PlateDetectionResult::failure(s,why,diag);};
    try{
        stop();target.validate();require(target.pattern==CalibrationGridPattern::FramedCircles,"Framed circle target required");
        CalibrationGridObservation grid{target,frame.metadata(),frame.size(),{},{},QString::fromLatin1(frame.contentId().toHex())};
        const auto& m=frame.metadata();
        require(!frame.size().isEmpty() && qint64(frame.size().width())*frame.size().height()<=16000000 && frame.processing().isEmpty() && !m.sourceId.isEmpty() && !m.frameId.isEmpty() && !m.cycleId.isEmpty() && m.completedAtUtc.isValid() && (m.coordinateSystemId=="image" || m.coordinateSystemId==m.sourceId+"/image"),"Raw frame/provenance required, limit 16 MP");
        QRect roi=searchRoi.value_or(QRect(QPoint(0,0),frame.size()));
        require(!roi.isEmpty() && QRect(QPoint(0,0),frame.size()).contains(roi),"Search ROI outside raw raster");
        cv::Mat raw=ImageAccess(frame).ownedCopy(),gray;
        switch(frame.format()){
        case PixelFormat::Gray8:gray=raw;break;case PixelFormat::Gray16:raw.convertTo(gray,CV_8U,1./257);break;
        case PixelFormat::Bgr8:cv::cvtColor(raw,gray,cv::COLOR_BGR2GRAY);break;case PixelFormat::Bgra8:cv::cvtColor(raw,gray,cv::COLOR_BGRA2GRAY);break;
        case PixelFormat::Rgb8:cv::cvtColor(raw,gray,cv::COLOR_RGB2GRAY);break;case PixelFormat::Rgba8:cv::cvtColor(raw,gray,cv::COLOR_RGBA2GRAY);break;}
        cv::Rect cr(roi.x(),roi.y(),roi.width(),roi.height());double scale=std::min(1.,1200./std::max(roi.width(),roi.height()));cv::Mat search;
        cv::resize(gray(cr),search,{},scale,scale,cv::INTER_AREA);double low,high;cv::minMaxLoc(search,&low,&high);stage("input");stop();
        if(high-low<8)return fail(PlateDetectionStatus::NotFound,"Insufficient contrast");
        std::vector<Region> regions;bool frameSeen=false;
        const std::vector<cv::Point2f> unit{{0,0},{1,0},{1,1},{0,1}};
        const auto fullSearch=search;const double fullScale=scale;
        for(double searchFactor:{1.,.5}){scale=fullScale*searchFactor;cv::resize(fullSearch,search,{},searchFactor,searchFactor,cv::INTER_AREA);
        for(int step=1;step<=15;++step){stop();cv::Mat mask;cv::threshold(search,mask,low+(high-low)*step/16.,255,cv::THRESH_BINARY_INV);
            std::vector<std::vector<cv::Point>> contours;std::vector<cv::Vec4i> hierarchy;cv::findContours(mask,contours,hierarchy,cv::RETR_TREE,cv::CHAIN_APPROX_SIMPLE);
            if(contours.size()>20000)continue;
            for(int i=0;i<int(contours.size());++i){if((i%128)==0)stop();double area=std::abs(cv::contourArea(contours[i]));int child=hierarchy[i][2];
                if(area<search.total()*.015 || child<0)continue;
                std::vector<cv::Point> outer;cv::approxPolyDP(contours[i],outer,cv::arcLength(contours[i],true)*.012,true);
                if(outer.size()!=4 || !cv::isContourConvex(outer))continue;
                bool clipped=false;for(auto p:outer)if(p.x<=1 || p.y<=1 || p.x>=search.cols-2 || p.y>=search.rows-2)clipped=true;if(clipped)continue;
                std::vector<cv::Point2f> corners;for(auto p:outer)corners.emplace_back(p);
                if(cv::contourArea(corners,true)<0)std::reverse(corners.begin(),corners.end());
                auto first=std::min_element(corners.begin(),corners.end(),[](auto a,auto b){return a.x+a.y<b.x+b.y;});std::rotate(corners.begin(),first,corners.end());
                auto transform=cv::getPerspectiveTransform(corners,unit);
                // The frame must enclose a bright hole. The marker is an actual
                // diagonal cut in that hole, attached to BOTH adjacent borders.
                for(int h=child;h>=0;h=hierarchy[h][0]){
                    double ratio=std::abs(cv::contourArea(contours[h]))/area;if(ratio<.55 || ratio>.95)continue;frameSeen=true;
                    diag.frameBoundary.clear();for(auto p:corners)diag.frameBoundary.append({p.x/scale+roi.x(),p.y/scale+roi.y()});
                    std::vector<cv::Point> hole;cv::approxPolyDP(contours[h],hole,cv::arcLength(contours[h],true)*.006,true);
                    if(hole.size()!=5 || !cv::isContourConvex(hole))continue;
                    std::vector<cv::Point2f> hp;for(auto p:hole)hp.push_back(mapped(transform,cv::Point2f(p)));
                    int diagonal=-1;bool valid=true;
                    for(int k=0;k<5;++k){auto d=hp[(k+1)%5]-hp[k];if(std::min(std::abs(d.x),std::abs(d.y))>.025){if(diagonal>=0)valid=false;diagonal=k;}}
                    if(!valid || diagonal<0)continue;
                    auto a=hp[diagonal],b=hp[(diagonal+1)%5],middle=(a+b)*.5f;int corner=0;
                    for(int k=1;k<4;++k)if(distance(middle,unit[k])<distance(middle,unit[corner]))corner=k;
                    auto delta=a-b;
                    if(distance(middle,unit[corner])>.23 || std::abs(delta.x)<.035 || std::abs(delta.y)<.035 || std::abs(delta.x)>.22 || std::abs(delta.y)>.22)continue;
                    auto marker=mapped(transform.inv(),middle);
                    bool duplicate=false;cv::Point2f centre;for(auto p:corners)centre+=p*.25f;
                    for(const auto& r:regions){cv::Point2f rc;for(auto p:r.corners)rc+=(p-cv::Point2f(float(roi.x()),float(roi.y())))*float(scale*.25);auto rm=(r.marker-cv::Point2f(float(roi.x()),float(roi.y())))*float(scale);
                        if(distance(centre,rc)<std::sqrt(area)*.08 && distance(marker,rm)<std::sqrt(area)*.04){duplicate=true;break;}}
                    if(duplicate)continue;
                    require(regions.size()<16,"Frame search budget exceeded");Region r;r.corner=corner;
                    r.marker={float(marker.x/scale+roi.x()),float(marker.y/scale+roi.y())};
                    for(auto p:corners)r.corners.push_back({float(p.x/scale+roi.x()),float(p.y/scale+roi.y())});
                    r.toUnit=cv::getPerspectiveTransform(r.corners,unit);regions.push_back(r);
                }
            }
        }
        }
        stage("frame_marker");stop();
        if(regions.empty())return fail(PlateDetectionStatus::NotFound,frameSeen ? "Frame found; triangle missing, clipped or ambiguous":"No complete framed plate; include the entire frame in the search ROI");
        std::vector<RawPlateObservation> observations;PlateDetectionDiagnostics best=diag;QString reason="Incomplete or ambiguous 7x7 topology";
        for(const auto& region:regions){stop();diag.frameBoundary.clear();for(auto p:region.corners)diag.frameBoundary.append(qp(p));
            auto bounds=cv::boundingRect(region.corners)&cv::Rect(0,0,gray.cols,gray.rows);cv::Mat patch=gray(bounds);double lo,hi;cv::minMaxLoc(patch,&lo,&hi);
            std::vector<Dot> dots;
            for(int step=1;step<=15;++step){stop();double threshold=lo+(hi-lo)*step/16.;cv::Mat mask;cv::threshold(patch,mask,threshold,255,cv::THRESH_BINARY_INV);
                std::vector<std::vector<cv::Point>> contours;cv::findContours(mask,contours,cv::RETR_LIST,cv::CHAIN_APPROX_NONE, bounds.tl());
                if(contours.size()>20000)continue;
                for(int i=0;i<int(contours.size());++i){if(i%128==0)stop();auto box=cv::boundingRect(contours[i]);double area=std::abs(cv::contourArea(contours[i]));
                    if(area<20 || area>bounds.area()*.025 || box.width<5 || box.height<5)continue;
                    auto refined=refine(gray,contours[i],threshold);if(!refined)continue;auto dot=*refined;dot.unit=mapped(region.toUnit,dot.p);
                    if(dot.unit.x<.04 || dot.unit.y<.04 || dot.unit.x>.96 || dot.unit.y>.96)continue;
                    bool duplicate=false;for(auto& existing:dots)if(distance(existing.p,dot.p)<std::min(existing.radius,dot.radius)*.6){if(dot.edge>existing.edge)existing=dot;duplicate=true;break;}
                    if(!duplicate){require(dots.size()<512,"Candidate budget exceeded");dots.push_back(dot);}
                }
            }
            stage("raw_candidates");stop();
            std::sort(dots.begin(),dots.end(),[](auto a,auto b){return a.p.y==b.p.y ? a.p.x<b.p.x:a.p.y<b.p.y;});
            diag.partialCandidates.clear();for(auto dot:dots)diag.partialCandidates.append(qp(dot.p));
            if(diag.partialCandidates.size()>=best.partialCandidates.size())best=diag;
            if(dots.empty())continue;
            std::vector<double> sizes;for(auto dot:dots)sizes.push_back(dot.radius);std::sort(sizes.begin(),sizes.end());const double medianRadius=sizes[sizes.size()/2];
            auto eligible=[&](const Dot& d){return d.radius>=medianRadius*.65 && d.radius<=medianRadius*1.5;};
            // Separate 1D neighbour clusters in frame-normalized search space.
            // The actual raw centres are neither projected nor fitted to H.
            auto clusters=[&](bool x){std::vector<double> values;for(auto d:dots)if(eligible(d))values.push_back(x ? d.unit.x:d.unit.y);std::sort(values.begin(),values.end());
                std::vector<std::pair<double,int>> groups;for(auto v:values){if(groups.empty() || v-groups.back().first/groups.back().second>.035)groups.push_back({v,1});else{groups.back().first+=v;++groups.back().second;}}
                std::vector<double> result;for(auto [sum,n]:groups)if(n>=5)result.push_back(sum/n);return result;};
            auto xs=clusters(true),ys=clusters(false);if(xs.size()!=7 || ys.size()!=7)continue;
            bool regular=true;for(auto values:{xs,ys}){double spacing=(values[6]-values[0])/6;if(spacing<.07 || spacing>.16)regular=false;
                for(int i=1;i<7;++i)if(std::abs(values[i]-values[i-1]-spacing)>.22*spacing)regular=false;}
            if(!regular)continue;
            std::array<int,49> assignment;assignment.fill(-1);bool ambiguous=false;
            for(int k=0;k<int(dots.size());++k){if(!eligible(dots[k]))continue;auto p=dots[k].unit;
                int c=int(std::min_element(xs.begin(),xs.end(),[&](auto a,auto b){return std::abs(a-p.x)<std::abs(b-p.x);})-xs.begin());
                int r=int(std::min_element(ys.begin(),ys.end(),[&](auto a,auto b){return std::abs(a-p.y)<std::abs(b-p.y);})-ys.begin());
                if(std::abs(p.x-xs[c])>.045 || std::abs(p.y-ys[r])>.045)continue;
                if(assignment[r*7+c]>=0)ambiguous=true;assignment[r*7+c]=k;
            }
            if(ambiguous || std::find(assignment.begin(),assignment.end(),-1)!=assignment.end())continue;
            std::vector<double> radii;for(int k:assignment)radii.push_back(dots[k].radius);std::sort(radii.begin(),radii.end());
            if(radii.front()<radii[24]*.65 || radii.back()>radii[24]*1.5)continue;
            auto observed=grid;QVector<PlateCentre> centres;diag.assignedCandidates.clear();diag.rejectedCandidates.clear();
            for(int r=0;r<7;++r)for(int c=0;c<7;++c){int u=c,v=r;
                if(region.corner==1){u=6-r;v=c;}if(region.corner==2){u=6-c;v=6-r;}if(region.corner==3){u=r;v=6-c;}
                auto p=qp(dots[assignment[v*7+u]].p);observed.pixels.append(p);observed.worldMillimetres.append({c*target.squareMillimetres,r*target.squareMillimetres});centres.append({r,c,p,{}});diag.assignedCandidates.append(p);
            }
            for(int k=0;k<int(dots.size());++k)if(std::find(assignment.begin(),assignment.end(),k)==assignment.end())diag.rejectedCandidates.append(qp(dots[k].p));
            try{observations.push_back(RawPlateObservation::create(observed,centres,{PlateOrientation::TriangleCorner,region.corner,false,qp(region.marker)},version));best=diag;}catch(const std::exception& e){reason=QString::fromUtf8(e.what());}
        }
        stage("topology_orientation");best.stageMilliseconds=diag.stageMilliseconds;diag=best;stop();
        if(observations.size()!=1){diag.assignedCandidates.clear();if(observations.empty() && diag.partialCandidates.isEmpty())reason="Frame and triangle found; no dots meet raw edge contrast/ellipse quality gates";
            return fail(PlateDetectionStatus::NotFound,observations.empty() ? reason:"Multiple plates or orientation interpretations; narrow the search ROI");}
        diag.message=QString("%1 candidates, 49 assigned raw centres").arg(diag.partialCandidates.size());diag.stageMilliseconds["total"]=timer.nsecsElapsed()/1e6;
        return PlateDetectionResult::ready(std::move(observations.front()),diag);
    }catch(const Cancelled&){return fail(PlateDetectionStatus::Cancelled,"Plate detection cancelled");}
    catch(const std::exception& e){return fail(previous==0 ? PlateDetectionStatus::InvalidInput:PlateDetectionStatus::NotFound,QString::fromUtf8(e.what()));}
}
}
