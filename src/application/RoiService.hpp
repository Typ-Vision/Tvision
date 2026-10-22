#pragma once
#include "ExecutionService.hpp"
#include "MatchService.hpp"
#include "domain/ToolRegistry.hpp"
#include <stdexcept>

namespace visionflow {
// A Region is an operation domain in the original image coordinates, never a crop.
struct RoiService {
    static QJsonObject imageBinding(const ImageFrame& image) {
        return {{"source",image.metadata().sourceId},{"coordinates",image.metadata().coordinateSystemId},
                {"width",image.size().width()},{"height",image.size().height()}};
    }
    static QJsonObject teach(const ImageFrame& image,const QRect& region) {
        if(region.isEmpty() || !QRect(QPoint{},image.size()).contains(region) ||
           image.metadata().sourceId.isEmpty() || image.metadata().coordinateSystemId.isEmpty())
            throw std::runtime_error("ROI görüntü sınırı veya kaynak kimliği geçersiz.");
        return {{"searchRoi",MatchService::rectangle(region)},{"imageBinding",imageBinding(image)}};
    }
    static ToolResult execute(const ToolDefinition& tool,const QMap<QString,PortValue>& inputs,
                              const QString& frame,const QString& cycle) {
        ToolRegistry::validate(tool);
        const auto image=std::get<ImageFrame>(inputs.value("image").data);
        const auto region=MatchService::rectangle(tool.parameters["searchRoi"].toObject());
        if(tool.parameters["imageBinding"].toObject()!=imageBinding(image))
            throw std::runtime_error("ROI kaynağı / geometrisi değişti. ROI'yi yeniden çizin.");
        if(region.isEmpty() || !QRect(QPoint{},image.size()).contains(region))
            throw std::runtime_error("ROI çizilmemiş veya görüntü sınırı dışında.");
        ToolResult r;r.toolId=tool.id;r.state=ExecutionState::Done;r.searchRegion=region;
        r.outputs["region"]={outputPorts(tool.kind).first(),region,true,frame,cycle,image.metadata()};
        r.message="ROI işlem alanı hazır";return r;
    }
};
}
