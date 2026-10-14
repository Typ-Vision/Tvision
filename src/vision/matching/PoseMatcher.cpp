#include "PoseMatcher.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace visionflow {
namespace {
constexpr double rad=std::numbers::pi/180.;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void stop(const std::shared_ptr<std::atomic_bool>& c){if(c && c->load())throw std::runtime_error("Pose search cancelled");}
std::vector<double> grid(double lo,double hi,double step){std::vector<double> v;for(int i=0;i<=64;++i){double x=lo+i*step;if(x>=hi-1e-10)break;v.push_back(x);}v.push_back(hi);return v;}
double angleDistance(double a,double b){return std::abs(std::remainder(a-b,2*std::numbers::pi));}
bool same(const PoseCandidate& a,const PoseCandidate& b,const PoseSearch& o){return cv::norm(a.anchor-b.anchor)<=o.nmsDistance && angleDistance(a.angleRadians,b.angleRadians)<=std::clamp(o.angleStep*1.5,12.,20.)*rad;}
void prune(std::vector<PoseCandidate>& v,const PoseSearch& o,int limit,bool* truncated=nullptr){
    std::stable_sort(v.begin(),v.end(),[](const auto& a,const auto& b){return a.score>b.score;});
    std::vector<PoseCandidate> keep;
    for(const auto& c:v)if(std::none_of(keep.begin(),keep.end(),[&](const auto& k){return same(c,k,o);})){
        if(int(keep.size())==limit){if(truncated)*truncated=true;break;}keep.push_back(c);
    }v=std::move(keep);
}
struct Warped {cv::Mat image,mask;cv::Point2d anchor;};
Warped warp(const cv::Mat& model,cv::Point2d anchor,double angle,double scale){
    const double c=std::cos(angle)*scale,s=std::sin(angle)*scale;
    auto rotate=[&](cv::Point2d p){return cv::Point2d(c*p.x-s*p.y,s*p.x+c*p.y);};
    double left=0,top=0,right=0,bottom=0;
    for(auto p:{cv::Point2d(0,0),cv::Point2d(model.cols-1,0),cv::Point2d(0,model.rows-1),cv::Point2d(model.cols-1,model.rows-1)}){
        const auto q=rotate(p);left=std::min(left,q.x);top=std::min(top,q.y);right=std::max(right,q.x);bottom=std::max(bottom,q.y);
    }
    left=std::floor(left);top=std::floor(top);
    cv::Mat affine(cv::Matx23d(c,-s,-left,s,c,-top));
    const cv::Size size(int(std::ceil(right)-left)+1,int(std::ceil(bottom)-top)+1);
    Warped w;cv::warpAffine(model,w.image,affine,size,cv::INTER_LINEAR,cv::BORDER_CONSTANT);
    // Eroded support excludes interpolation against padding from the score.
    cv::Mat support(model.size(),CV_8U,cv::Scalar(255)),weight;
    cv::warpAffine(support,weight,affine,size,cv::INTER_LINEAR,cv::BORDER_CONSTANT);
    cv::compare(weight,254,w.mask,cv::CMP_GE);
    w.anchor=rotate(anchor)-cv::Point2d(left,top);return w;
}
std::vector<PoseCandidate> search(const cv::Mat& image,const cv::Mat& model,cv::Point2d anchor,double angle,double scale,
    cv::Rect roi,int peaks,double factor,const std::shared_ptr<std::atomic_bool>& cancel){
    stop(cancel);auto w=warp(model,anchor,angle,scale);std::vector<PoseCandidate> out;
    if(w.image.cols>roi.width || w.image.rows>roi.height || cv::countNonZero(w.mask)<16)return out;
    cv::Scalar mean,dev;cv::meanStdDev(w.image,mean,dev,w.mask);if(dev[0]<1)return out;
    cv::Mat scores;cv::matchTemplate(image(roi),w.image,scores,cv::TM_CCOEFF_NORMED,w.mask);
    for(int y=0;y<scores.rows;++y)for(int x=0;x<scores.cols;++x){auto& v=scores.at<float>(y,x);if(!std::isfinite(v) || v>1.0001f || v< -1.0001f)v=-2;}
    for(int i=0;i<peaks;++i){stop(cancel);double score;cv::Point p;cv::minMaxLoc(scores,nullptr,&score,nullptr,&p);if(score<=-1)break;
        out.push_back({(cv::Point2d(p+roi.tl())+w.anchor)*factor,angle,scale,std::clamp(score,-1.,1.)});
        const int radius=std::max(2,std::min(model.cols,model.rows)/3);
        scores(cv::Rect(p.x-radius,p.y-radius,2*radius+1,2*radius+1)&cv::Rect(0,0,scores.cols,scores.rows)).setTo(-2);
    }return out;
}
}
void PoseSearch::validate() const {
    for(double v:{angleMin,angleMax,angleStep,scaleMin,scaleMax,scaleStep,minScore,ambiguityGap,nmsDistance})require(std::isfinite(v),"Nonfinite pose configuration");
    require(angleMin>=-180 && angleMax<=180 && angleMin<=angleMax && angleStep>=1 && angleStep<=90,"Pose angle range/step invalid");
    require(scaleMin>=.5 && scaleMax<=2 && scaleMin<=scaleMax && scaleStep>=.02 && scaleStep<=.5,"Pose scale range/step invalid");
    require(grid(angleMin,angleMax,angleStep).size()<=37 && grid(scaleMin,scaleMax,scaleStep).size()<=17 && grid(angleMin,angleMax,angleStep).size()*grid(scaleMin,scaleMax,scaleStep).size()<=255,"Pose search grid exceeds budget");
    require(minScore>=0 && minScore<=1 && ambiguityGap>=0 && ambiguityGap<=1 && nmsDistance>=2 && nmsDistance<=1024,"Pose score/NMS invalid");
    require(pyramidLevels>=1 && pyramidLevels<=3 && maxCandidates>=2 && maxCandidates<=32,"Pose pyramid/candidate budget invalid");
}
PoseSearchResult PoseMatcher::find(const cv::Mat& gray,const cv::Mat& model,cv::Point2d anchor,const cv::Rect& roi,const PoseSearch& o,const std::shared_ptr<std::atomic_bool>& cancel){
    o.validate();stop(cancel);
    require(!gray.empty() && gray.type()==CV_8UC1 && !model.empty() && model.type()==CV_8UC1,"Pose matching requires Gray8 images");
    require(gray.total()<=16*1024*1024 && model.cols>=16 && model.rows>=16 && model.cols<=256 && model.rows<=256,"Pose image/model size exceeds budget");
    require(std::isfinite(anchor.x) && std::isfinite(anchor.y) && anchor.x>=0 && anchor.y>=0 && anchor.x<=model.cols-1 && anchor.y<=model.rows-1,"Teaching anchor outside model");
    require(roi.width>0 && roi.height>0 && (roi & cv::Rect(0,0,gray.cols,gray.rows))==roi,"Pose search ROI outside image");
    cv::Scalar mean,dev;cv::meanStdDev(model,mean,dev);require(dev[0]>=1,"Pose model lacks texture");
    std::vector<cv::Mat> images{gray(roi).clone()};
    for(int i=1;i<o.pyramidLevels;++i){cv::Mat next;cv::pyrDown(images.back(),next);images.push_back(next);}
    const int coarse=o.pyramidLevels-1;const double factor=double(1<<coarse);
    std::vector<PoseCandidate> candidates;
    // Coarse template uses area reduction; all final scores use full-resolution raw pixels.
    // Coarse images use pixel-centre origins (pyrDown samples 2*x).
    cv::Mat smallModel;cv::resize(model,smallModel,{},1/factor,1/factor,cv::INTER_AREA);
    require(smallModel.cols>=4 && smallModel.rows>=4,"Pose model too small for pyramid");
    for(double a:grid(o.angleMin,o.angleMax,o.angleStep))for(double s:grid(o.scaleMin,o.scaleMax,o.scaleStep)){
        auto found=search(images.back(),smallModel,anchor/factor,a*rad,s,{0,0,images.back().cols,images.back().rows},8,factor,cancel);
        candidates.insert(candidates.end(),found.begin(),found.end());
    }
    // Bound refinement work; candidate list is a search hypothesis list, not total object count.
    prune(candidates,o,128);
    for(int level=coarse-1;level>=0;--level){
        const double f=double(1<<level);cv::resize(model,smallModel,{},1/f,1/f,cv::INTER_AREA);
        std::vector<PoseCandidate> next;
        for(const auto& c:candidates){
            auto w=warp(smallModel,anchor/f,c.angleRadians,c.scale);const auto tl=c.anchor/f-w.anchor;
            const int margin=4;
            cv::Rect area(int(std::floor(tl.x))-margin,int(std::floor(tl.y))-margin,w.image.cols+2*margin+1,w.image.rows+2*margin+1);
            area &= cv::Rect(0,0,images[level].cols,images[level].rows);
            auto found=search(images[level],smallModel,anchor/f,c.angleRadians,c.scale,area,1,f,cancel);next.insert(next.end(),found.begin(),found.end());
        }candidates=std::move(next);prune(candidates,o,128);
    }
    // Two bounded angular/scale refinement passes; score always recomputed on full raw pixels.
    for(int pass=0;pass<2;++pass){std::vector<PoseCandidate> next;
        for(const auto& c:candidates){
            for(int da=-1;da<=1;++da)for(int ds=-1;ds<=1;++ds){
                const double a=c.angleRadians/rad+da*o.angleStep/std::pow(2.,pass+1),s=c.scale+ds*o.scaleStep/std::pow(2.,pass+1);
                if(a<o.angleMin-1e-9 || a>o.angleMax+1e-9 || s<o.scaleMin-1e-9 || s>o.scaleMax+1e-9)continue;
                auto w=warp(model,anchor,a*rad,s);auto tl=c.anchor-w.anchor;
                cv::Rect area(int(std::floor(tl.x))-3,int(std::floor(tl.y))-3,w.image.cols+7,w.image.rows+7);area &= cv::Rect(0,0,images[0].cols,images[0].rows);
                auto found=search(images[0],model,anchor,a*rad,s,area,1,1,cancel);next.insert(next.end(),found.begin(),found.end());
            }
        }candidates=std::move(next);prune(candidates,o,128);
    }
    std::erase_if(candidates,[&](const auto& c){return c.score<o.minScore;});
    PoseSearchResult result;prune(candidates,o,o.maxCandidates,&result.truncated);
    for(auto& c:candidates)c.anchor+=cv::Point2d(roi.tl());result.candidates=std::move(candidates);
    result.ambiguous=result.truncated || (result.candidates.size()>1 && result.candidates.front().score-result.candidates[1].score<=o.ambiguityGap);
    if(!result.candidates.empty() && (!result.ambiguous || o.selectBest))result.selected=result.candidates.front();
    stop(cancel);return result;
}
}
