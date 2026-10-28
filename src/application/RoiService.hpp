#pragma once
#include "ExecutionService.hpp"
#include "MatchService.hpp"
#include "vision/ImageAccess.hpp"
#include "domain/ToolRegistry.hpp"
#include <stdexcept>

namespace visionflow {
// Region stays in the input raster; Image optionally owns a child raster.
struct RoiService {
    static bool contains(const ImageFrame& image,const QRect& r) {
        return r.x()>=0 && r.y()>=0 && r.width()>0 && r.height()>0 &&
            qint64(r.x())+qint64(r.width())<=image.size().width() &&
            qint64(r.y())+qint64(r.height())<=image.size().height();
    }
    static QJsonObject imageBinding(const ImageFrame& image) {
        return {{"source",image.metadata().sourceId},{"coordinates",image.metadata().coordinateSystemId},
                {"width",image.size().width()},{"height",image.size().height()}};
    }
    static QJsonObject teach(const ImageFrame& image,const QRect& region,bool cropImage=false) {
        if(!contains(image,region) ||
           image.metadata().sourceId.isEmpty() || image.metadata().coordinateSystemId.isEmpty())
            throw std::runtime_error("ROI görüntü sınırı veya kaynak kimliği geçersiz.");
        return {{"searchRoi",MatchService::rectangle(region)},{"imageBinding",imageBinding(image)},{"cropImage",cropImage}};
    }
    static ToolResult execute(const ToolDefinition& tool,const QMap<QString,PortValue>& inputs,
                              const QString& frame,const QString& cycle) {
        ToolRegistry::validate(tool);
        const auto image=std::get<ImageFrame>(inputs.value("image").data);
        const auto region=MatchService::rectangle(tool.parameters["searchRoi"].toObject());
        if(tool.parameters["imageBinding"].toObject()!=imageBinding(image))
            throw std::runtime_error("ROI kaynağı / geometrisi değişti. ROI'yi yeniden çizin.");
        if(!contains(image,region))
            throw std::runtime_error("ROI çizilmemiş veya görüntü sınırı dışında.");
        ToolResult r;r.toolId=tool.id;r.state=ExecutionState::Done;r.searchRegion=region;
        r.outputs["region"]={outputPorts(tool.kind).first(),region,true,frame,cycle,image.metadata()};
        r.outputs["region"].imageGeometry=image.geometry();
        const auto output=tool.parameters["cropImage"].toBool() ? ImageAccess::cropped(image,region):image;
        r.outputs["image"]={outputPorts(tool.kind).at(1),output,true,frame,cycle,output.metadata()};
        r.outputs["image"].imageGeometry=output.geometry();
        r.message="ROI işlem alanı hazır";return r;
    }
};
}
