#pragma once
#include "ExecutionService.hpp"
#include "RuntimeOverlayTypes.hpp"
namespace visionflow {
RuntimeOverlayCollection runtimeOverlays(const CycleResult&,const QVector<ToolDefinition>&,const GraphValidation&,
    const ImageFrame&,const std::shared_ptr<const ProductionSnapshot>& snapshot,const QString& selectedTool={});
}
