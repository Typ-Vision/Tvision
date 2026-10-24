#pragma once
#include "ProfileSampler.hpp"
#include "FitDiagnostic.hpp"
namespace visionflow {
enum class EdgePolarity { Rising,Falling,Any };
enum class EdgeSelection { First,Last,Strongest };
struct EdgeOptions {
    ProfileOptions profile;
    EdgePolarity polarity=EdgePolarity::Any;
    EdgeSelection selection=EdgeSelection::Strongest;
    double minContrast=20; // Sampled peak magnitude in intensity/px; inclusive.
    bool subpixel=true;
};
struct EdgeCandidate {
    QPointF point;
    double distancePx=0,strength=0,signedStrength=0;
    int firstIndex=0,lastIndex=0;
    bool accepted=false,refined=false;
    QString reason,refinement;
};
struct EdgeSearch {
    SampledProfile profile;
    QVector<EdgeCandidate> candidates;
    std::optional<int> selected;
};
inline ScanRejections scanRejections(const EdgeSearch& edge) {
    ScanRejections r;r.noCandidate=edge.candidates.isEmpty() ? 1:0;
    for(const auto& c:edge.candidates){if(c.reason=="polarity")++r.polarity;else if(c.reason=="min_contrast")++r.contrast;else if(c.reason=="border_support")++r.border;}
    return r;
}
class EdgeFinder {
public:
    static EdgeSearch find(const ImageFrame&,const ToolRoi&,const EdgeOptions& = {},
                           const std::shared_ptr<std::atomic_bool>& cancel={});
    static EdgeSearch findMapped(const ImageFrame&,const ScanSegment&,const ProfileRawMap&,const EdgeOptions& = {},
                                 const std::shared_ptr<std::atomic_bool>& cancel={});
private:
    static EdgeSearch select(SampledProfile,const EdgeOptions&,const std::shared_ptr<std::atomic_bool>&);
};
}
