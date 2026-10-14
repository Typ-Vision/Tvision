#include "EdgePairFinder.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void validate(const EdgePairOptions& o){
    if(!std::isfinite(o.minWidthPx) || !std::isfinite(o.maxWidthPx) || o.minWidthPx<0 || o.maxWidthPx<=0 || o.minWidthPx>o.maxWidthPx || o.maxWidthPx>65536 ||
       (o.polarity!=StripePolarity::Bright && o.polarity!=StripePolarity::Dark && o.polarity!=StripePolarity::Any))throw std::runtime_error("Invalid edge pair options");
}
}
EdgePairSearch EdgePairFinder::find(const ImageFrame& image,const ToolRoi& roi,const EdgePairOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    validate(o);
    EdgeOptions edge{o.profile,EdgePolarity::Any,o.selection,o.minContrast,o.subpixel};
    return select(EdgeFinder::find(image,roi,edge,cancel),o,cancel);
}
EdgePairSearch EdgePairFinder::findMapped(const ImageFrame& image,const ScanSegment& scan,const ProfileRawMap& map,const EdgePairOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    validate(o);
    EdgeOptions edge{o.profile,EdgePolarity::Any,o.selection,o.minContrast,o.subpixel};
    return select(EdgeFinder::findMapped(image,scan,map,edge,cancel),o,cancel);
}
EdgePairSearch EdgePairFinder::select(EdgeSearch search,const EdgePairOptions& o,const std::shared_ptr<std::atomic_bool>& cancel) {
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Edge pair search cancelled");};
    stop();EdgePairSearch r;r.edges=std::move(search);
    const auto& edges=r.edges.candidates;
    QVector<int> eligible;for(int i=0;i<edges.size();++i){stop();if(edges[i].accepted)eligible.append(i);}
    for(int i=0;i+1<eligible.size();++i) {
        stop();const auto& a=edges[eligible[i]];const auto& b=edges[eligible[i+1]];
        if((a.signedStrength>0)==(b.signedStrength>0))continue;
        EdgePairCandidate c;c.firstEdge=eligible[i];c.secondEdge=eligible[i+1];c.widthPx=b.distancePx-a.distancePx;c.strength=std::min(a.strength,b.strength);
        const bool polarity=o.polarity==StripePolarity::Any || (o.polarity==StripePolarity::Bright)==(a.signedStrength>0);
        c.reason=!polarity ? "polarity":c.widthPx<=0 ? "nonpositive_width":c.widthPx<o.minWidthPx ? "min_width":c.widthPx>o.maxWidthPx ? "max_width":QString{};
        c.accepted=c.reason.isEmpty();r.candidates.append(c);const int index=int(r.candidates.size())-1;
        const double previous=r.selected ? r.candidates[*r.selected].strength:0;
        const bool stronger=c.strength>previous && std::abs(c.strength-previous)>1e-9*std::max({1.0,c.strength,previous});
        if(c.accepted && (!r.selected || o.selection==EdgeSelection::Last || (o.selection==EdgeSelection::Strongest && stronger)))r.selected=index;
    }
    stop();return r;
}
}
