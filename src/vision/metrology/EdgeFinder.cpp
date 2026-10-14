#include "EdgeFinder.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void validate(const EdgeOptions& o) {
    if(!std::isfinite(o.minContrast) || o.minContrast<=0 || o.minContrast>5100 ||
       (o.polarity!=EdgePolarity::Rising && o.polarity!=EdgePolarity::Falling && o.polarity!=EdgePolarity::Any) ||
       (o.selection!=EdgeSelection::First && o.selection!=EdgeSelection::Last && o.selection!=EdgeSelection::Strongest))throw std::runtime_error("Invalid edge options");
}
}

EdgeSearch EdgeFinder::find(const ImageFrame& image,const ToolRoi& roi,const EdgeOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    validate(o);return select(ProfileSampler::sample(image,roi,o.profile,cancel),o,cancel);
}
EdgeSearch EdgeFinder::findMapped(const ImageFrame& image,const ScanSegment& scan,const ProfileRawMap& map,const EdgeOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    validate(o);return select(ProfileSampler::sampleMapped(image,scan,map,o.profile,cancel),o,cancel);
}
EdgeSearch EdgeFinder::select(SampledProfile profile,const EdgeOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Edge search cancelled");};
    stop();EdgeSearch r;r.profile=std::move(profile);
    const auto& values=r.profile.derivatives;
    if(values.size()<3)throw std::runtime_error("Insufficient edge peak support: three derivative samples required");
    const auto same=[](double a,double b){return std::abs(a-b)<=1e-9*std::max({1.0,std::abs(a),std::abs(b)});};
    // Separate signed runs: adjacent rising/falling peaks must never merge through abs().
    for(int i=0;i<values.size();) {
        stop();const double value=values[i].value;const int sign=value>0 ? 1:value<0 ? -1:0;
        if(!sign){++i;continue;}
        const double peak=std::abs(value);int end=i;
        while(end+1<values.size() && sign*values[end+1].value>0 && same(values[end+1].value,value)){stop();++end;}
        const double left=i>0 ? sign*values[i-1].value:-1,right=end+1<values.size() ? sign*values[end+1].value:-1;
        if(peak>left && peak>right && !same(peak,left) && !same(peak,right)) {
            EdgeCandidate c;c.firstIndex=i;c.lastIndex=end;c.distancePx=(values[i].distancePx+values[end].distancePx)/2;
            c.strength=peak;c.signedStrength=value;c.refinement=end>i ? "plateau_midpoint":"sample_midpoint";
            const bool border=i==0 || end==values.size()-1;
            const bool polarity=o.polarity==EdgePolarity::Any || (o.polarity==EdgePolarity::Rising)==(sign>0);
            c.reason=border ? "border_support":!polarity ? "polarity":peak<o.minContrast ? "min_contrast":QString{};
            c.accepted=c.reason.isEmpty();
            if(c.accepted && o.subpixel && end==i) {
                const double a=sign*values[i-1].value,b=peak,d=sign*values[i+1].value,curvature=a-2*b+d;
                const double delta=.5*(a-d)/curvature;
                if(curvature < -1e-12*std::max(1.0,b) && std::isfinite(delta) && std::abs(delta)<=.5) {
                    c.distancePx+=delta*r.profile.actualStepPx;c.strength=b-.25*(a-d)*delta;
                    c.signedStrength=sign*c.strength;c.refined=true;c.refinement="parabolic_peak";
                } else c.refinement="unrefined_curvature";
            }
            const auto direction=(r.profile.scan.end-r.profile.scan.start)/QLineF(r.profile.scan.start,r.profile.scan.end).length();
            c.point=r.profile.scan.start+direction*c.distancePx;
            r.candidates.append(c);
            const int index=int(r.candidates.size())-1;
            if(c.accepted && (!r.selected || o.selection==EdgeSelection::Last ||
               (o.selection==EdgeSelection::Strongest && c.strength>r.candidates[*r.selected].strength && !same(c.strength,r.candidates[*r.selected].strength))))r.selected=index;
        }
        i=end+1;
    }
    stop();return r;
}
}
