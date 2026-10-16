#pragma once
#include <QString>
namespace visionflow {
class MeasurementProjectTransfer {
public:
    // Saved source only. Project commits last; identical interrupted copies resume.
    // Existing destination assets are verified; images are never silently omitted.
    static void copy(const QString& source,const QString& destination,const QString& projectId,int failAfter=-1);
};
}
