#pragma once
#include "vision/ImageFrame.hpp"
#include "domain/ToolValues.hpp"
#include "domain/ToolRoi.hpp"
#include <QRect>
#include <QStringList>
#include <atomic>
#include <memory>
namespace visionflow {
struct CodeReaderOptions {QRect roi;QStringList formats;int maxCount=32,budgetMs=1000;std::optional<ToolRoi> trackingRoi;};
class CodeReader {
public:
    // Cooperative cancellation/deadline: a completed native read is discarded when late.
    static CodeResults read(const ImageFrame&,const CodeReaderOptions&,const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
