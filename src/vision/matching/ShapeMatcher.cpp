#include "ShapeMatcher.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace visionflow {
namespace {
constexpr double pi=std::numbers::pi,rad=pi/180.;
constexpr int bins=8;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void stop(const std::shared_ptr<std::atomic_bool>& c){if(c && c->load())throw std::runtime_error("Shape search cancelled");}
struct Edge {cv::Point2d point;double normal;};
struct Gradient {cv::Mat magnitude;std::vector<Edge> edges;};
Gradient gradient(const cv::Mat& m,const std::shared_ptr<std::atomic_bool>& cancel={}){
    require(!m.empty() && m.type()==CV_8UC1,"Shape matching requires Gray8");
    cv::Mat blurred,dx,dy,mag;cv::GaussianBlur(m,blurred,{3,3},.8,.8,cv::BORDER_REFLECT_101 | cv::BORDER_ISOLATED);
    cv::Sobel(blurred,dx,CV_32F,1,0);cv::Sobel(blurred,dy,CV_32F,0,1);
    cv::magnitude(dx,dy,mag);Gradient g;mag.convertTo(g.magnitude,CV_8U,.25);
    // Keep all strong gradient support, including both sides of equal ridges.
    // Canny's one-sided tie breaking shifts symmetric contours under 90/180 rotation.
    // Ignore crop-border gradients. Teaching must contain the actual contour.
    for(int y=2;y<m.rows-2;++y){if(y%32==0)stop(cancel);for(int x=2;x<m.cols-2;++x)if(mag.at<float>(y,x)>=60){
        double a=std::atan2(dy.at<float>(y,x),dx.at<float>(y,x));if(a<0)a+=pi;if(a>=pi)a-=pi;
        g.edges.push_back({{double(x),double(y)},a});
    }}stop(cancel);return g;
}
int bin(double a){a=std::fmod(a+4*pi,pi);return int(std::floor(a*bins/pi+.5))%bins;}
double sample(const cv::Mat& m,cv::Point2d p){
    int x=int(p.x),y=int(p.y);int xx=std::min(x+1,m.cols-1),yy=std::min(y+1,m.rows-1);
    double fx=p.x-x,fy=p.y-y;
    return (1-fy)*((1-fx)*m.at<float>(y,x)+fx*m.at<float>(y,xx))+fy*((1-fx)*m.at<float>(yy,x)+fx*m.at<float>(yy,xx));
}
bool same(const PoseCandidate& a,const PoseCandidate& b,const PoseSearch& o){return cv::norm(a.anchor-b.anchor)<=o.nmsDistance && std::abs(std::remainder(a.angleRadians-b.angleRadians,2*pi))<=std::clamp(o.angleStep*1.5,12.,20.)*rad;}
}
void ShapeMatcher::validateModel(const cv::Mat& m){
    require(m.cols>=16 && m.rows>=16 && m.cols<=256 && m.rows<=256,"Shape model must be 16..256 px");
    require(gradient(m).edges.size()>=16,"Shape model needs at least 16 interior edge samples");
}
PoseSearchResult ShapeMatcher::find(const cv::Mat& gray,const cv::Mat& model,cv::Point2d anchor,const cv::Rect& roi,const PoseSearch& o,const std::shared_ptr<std::atomic_bool>& cancel){
    o.validate();stop(cancel);validateModel(model);
    require(gray.type()==CV_8UC1 && !gray.empty() && gray.total()<=16*1024*1024,"Shape image exceeds Gray8/16Mi budget");
    require(roi.width>0 && roi.height>0 && (roi & cv::Rect(0,0,gray.cols,gray.rows))==roi,"Shape search ROI outside image");
    require(std::isfinite(anchor.x) && std::isfinite(anchor.y) && anchor.x>=0 && anchor.y>=0 && anchor.x<=model.cols-1 && anchor.y<=model.rows-1,"Shape anchor outside model");
    auto taught=gradient(model,cancel),scene=gradient(gray(roi),cancel);
    if(scene.edges.empty())return {};
    // Share the bounded pyramid proposal mechanism, operating on gradient magnitude.
    // Proposal correlation is discarded; final score always comes from oriented chamfer.
    auto proposals=o;proposals.minScore=0;proposals.maxCandidates=32;proposals.selectBest=true;
    auto seeds=PoseMatcher::find(scene.magnitude,taught.magnitude,anchor,{0,0,roi.width,roi.height},proposals,cancel);
    std::array<cv::Mat,bins> distance;
    for(int b=0;b<bins;++b){stop(cancel);cv::Mat mask(roi.size(),CV_8U,cv::Scalar(255));
        for(const auto& e:scene.edges)if(bin(e.normal)==b)mask.at<uchar>(int(e.point.y),int(e.point.x))=0;
        cv::distanceTransform(mask,distance[b],cv::DIST_L2,cv::DIST_MASK_PRECISE);
        cv::min(distance[b],6.,distance[b]);
    }
    std::vector<Edge> samples;const auto count=taught.edges.size();
    for(size_t i=0;i<std::min<size_t>(count,256);++i)samples.push_back(taught.edges[i*count/std::min<size_t>(count,256)]);
    auto score=[&](const PoseCandidate& c){
        const double co=std::cos(c.angleRadians)*c.scale,si=std::sin(c.angleRadians)*c.scale;
        auto map=[&](cv::Point2d p){p-=anchor;return c.anchor+cv::Point2d(co*p.x-si*p.y,si*p.x+co*p.y);};
        for(auto p:{cv::Point2d(0,0),cv::Point2d(model.cols-1,0),cv::Point2d(0,model.rows-1),cv::Point2d(model.cols-1,model.rows-1)}){auto q=map(p);if(q.x<0 || q.y<0 || q.x>roi.width-1 || q.y>roi.height-1)return -1.;}
        double cost=0;
        for(const auto& e:samples){auto p=map(e.point);int expected=bin(e.normal+c.angleRadians);double best=1;
            for(int b=0;b<bins;++b){int d=std::abs(b-expected);d=std::min(d,bins-d);best=std::min(best,sample(distance[b],p)/6.+.5*d/(bins/2.));}cost+=best;
        }return 1-cost/samples.size();
    };
    std::vector<PoseCandidate> refined;
    for(auto c:seeds.candidates){c.score=score(c);
        for(int pass=0;pass<4;++pass){stop(cancel);auto best=c;const double step=std::pow(.5,pass);
            // Coordinate descent with bounded translation/angle/scale proposals.
            for(int axis=0;axis<4;++axis){best=c;
                for(int sign:{-1,1}){auto p=c;if(axis==0)p.anchor.x+=sign*step;if(axis==1)p.anchor.y+=sign*step;
                    if(axis==2)p.angleRadians+=sign*o.angleStep*rad*step/2;if(axis==3)p.scale+=sign*o.scaleStep*step/2;
                    if(p.angleRadians<o.angleMin*rad-1e-10 || p.angleRadians>o.angleMax*rad+1e-10 || p.scale<o.scaleMin-1e-10 || p.scale>o.scaleMax+1e-10)continue;
                    p.score=score(p);if(p.score>best.score)best=p;
                }c=best;
            }
        }if(c.score>=o.minScore)refined.push_back(c);
    }
    std::stable_sort(refined.begin(),refined.end(),[](const auto& a,const auto& b){return a.score>b.score;});
    PoseSearchResult out;
    for(auto c:refined)if(std::none_of(out.candidates.begin(),out.candidates.end(),[&](const auto& k){auto local=k;local.anchor-=cv::Point2d(roi.tl());return same(c,local,o);})){
        if(int(out.candidates.size())==o.maxCandidates){out.truncated=true;break;}c.anchor+=cv::Point2d(roi.tl());out.candidates.push_back(c);
    }
    out.ambiguous=out.truncated || (out.candidates.size()>1 && out.candidates[0].score-out.candidates[1].score<=o.ambiguityGap);
    if(!out.candidates.empty() && (!out.ambiguous || o.selectBest))out.selected=out.candidates[0];
    stop(cancel);return out;
}
}
