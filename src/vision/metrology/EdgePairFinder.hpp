#pragma once
#include "EdgeFinder.hpp"
namespace visionflow {
enum class StripePolarity { Bright, Dark, Any };
struct EdgePairOptions {
    ProfileOptions profile;
    StripePolarity polarity=StripePolarity::Bright;
    EdgeSelection selection=EdgeSelection::Strongest;
    double minContrast=20, minWidthPx=0, maxWidthPx=65536;
    bool subpixel=true;
};
struct EdgePairCandidate {
    int firstEdge=0,secondEdge=0;
    double widthPx=0,strength=0; // Strength is the weaker of the two edges.
    bool accepted=false;
    QString reason;
};
struct EdgePairSearch {
    EdgeSearch edges;
    QVector<EdgePairCandidate> candidates;
    std::optional<int> selected;
};
class EdgePairFinder {
public:
    // Only neighbouring accepted peaks may form a stripe. Rejected noise peaks
    // remain diagnostic; accepted intermediate edges may never be skipped.
    static EdgePairSearch find(const ImageFrame&,const ToolRoi&,const EdgePairOptions& = {},
                              const std::shared_ptr<std::atomic_bool>& cancel={});
    static EdgePairSearch findMapped(const ImageFrame&,const ScanSegment&,const ProfileRawMap&,const EdgePairOptions& = {},
                                    const std::shared_ptr<std::atomic_bool>& cancel={});
private:
    static EdgePairSearch select(EdgeSearch,const EdgePairOptions&,const std::shared_ptr<std::atomic_bool>&);
};
}
