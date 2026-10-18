#pragma once
#include "domain/ToolRoi.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace visionflow {
// Pure nominal geometry, shared by acquisition and application guide presentation.
struct ScanLayout {
    static QVector<ScanSegment> radial(const ArcRoi& arc,int count,double width,bool inward) {
        ToolRoi{{"nominal"},arc}.validate();
        if(count<3 || count>4096 || !std::isfinite(width) || width<=0)throw std::runtime_error("Invalid radial layout");
        QVector<ScanSegment> scans;
        const double step=arc.sweepDegrees/(std::abs(arc.sweepDegrees)==360 ? count:count-1);
        for(int i=0;i<count;++i){const double a=(arc.startDegrees+i*step)*std::numbers::pi/180;const QPointF u(std::cos(a),std::sin(a));
            auto start=arc.ring.centre+u*arc.ring.innerRadius,end=arc.ring.centre+u*arc.ring.outerRadius;
            if(inward)std::swap(start,end);scans.append({start,end,width});}
        return scans;
    }
    static QVector<ScanSegment> line(const RotatedRectangle& box,int count,double width) {
        ToolRoi{{"nominal"},box}.validate();
        if(count<3 || count>4096 || !std::isfinite(width) || width<=0 || width>=box.size.width())throw std::runtime_error("Invalid line layout");
        const double a=box.angleDegrees*std::numbers::pi/180;const QPointF u(std::cos(a),std::sin(a)),n(-u.y(),u.x());
        QVector<ScanSegment> scans;
        for(int i=0;i<count;++i){const double t=(box.size.width()-width)*(double(i)/(count-1)-.5);const auto c=box.centre+u*t;
            scans.append({c-n*(box.size.height()/2),c+n*(box.size.height()/2),width});}
        return scans;
    }
};
}
