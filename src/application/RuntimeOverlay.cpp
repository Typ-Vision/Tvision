#include "RuntimeOverlay.hpp"
#include "RuntimeService.hpp"
#include <QCryptographicHash>
#include <algorithm>
namespace visionflow {
RuntimeOverlayCollection runtimeOverlays(const CycleResult& cycle,const QVector<ToolDefinition>& tools,const GraphValidation& graph,
    const ImageFrame& raw,const std::shared_ptr<const ProductionSnapshot>& snapshot,const QString& selectedTool) {
    const auto& m=raw.metadata();
    RuntimeOverlayCollection out{snapshot ? snapshot->validation().id.toUtf8()+"|"+snapshot->validation().dependencyId:QByteArray{},m.sourceId,m.frameId,m.cycleId,m.coordinateSystemId,raw.size(),{}};
    if(!RuntimeService::presentationMatches(cycle,snapshot) || cycle.recipeId!=snapshot->recipe().id || cycle.configId!=snapshot->validation().contentId || cycle.cycleId!=m.cycleId || m.sourceId.isEmpty() || m.frameId.isEmpty() || m.coordinateSystemId.isEmpty() || !raw.processing().isEmpty())return out;
    auto matches=[&](const FrameMetadata& x){return x.sourceId==m.sourceId && x.frameId==m.frameId && x.cycleId==m.cycleId && x.coordinateSystemId==m.coordinateSystemId;};
    for(const auto& r:cycle.tools){
        if(r.state!=ExecutionState::Done || graph.toolSources.value(r.toolId)!=m.sourceId || (!selectedTool.isEmpty() && r.toolId!=selectedTool))continue;
        const auto definition=std::find_if(tools.begin(),tools.end(),[&](const auto& t){return t.id==r.toolId;});if(definition==tools.end())continue;
        bool current=false,mismatch=false;
        for(const auto& v:r.outputs)if(v.valid){current=true;if(!matches(v.origin) || v.frameId!=m.frameId || v.cycleId!=m.cycleId)mismatch=true;}
        if(!current || mismatch)continue;
        RuntimeOverlayLayer layer;layer.toolId=r.toolId;layer.name=definition->name;layer.state=executionStateName(r.state);layer.detail=r.message;
        const auto hash=QCryptographicHash::hash(r.toolId.toUtf8(),QCryptographicHash::Sha256);layer.color=QColor::fromHsv((quint8(hash[0])*256+quint8(hash[1]))%360,165,255);
        layer.regions=r.regions;layer.points=r.points;layer.rejected=r.rejectedPoints;layer.lines=r.lines;layer.mask={r.mask.pixels,r.mask.size,r.mask.origin};
        if(definition->kind=="measurement" && r.points.size()==2)layer.lines<<QLineF(r.points[0],r.points[1]);
        const auto code=r.outputs.value("codes");if(code.valid)if(const auto* codes=std::get_if<CodeResults>(&code.data))layer.codes=*codes;
        if(r.overlayGeometry && matches(r.overlayGeometry->origin) && r.overlayGeometry->rawSize==raw.size()){
            layer.scans=r.overlayGeometry->outlines;layer.guides=r.overlayGeometry->guides;layer.lines+=r.overlayGeometry->resultLines;
            if(!r.overlayGeometry->reason.isEmpty())layer.detail+="\nTarama alanı gösterilmedi: "+r.overlayGeometry->reason;
        }else if(definition->parameters.contains("referenceBinding") || definition->parameters.contains("poseTracking") || definition->parameters.contains("scanRoi"))layer.detail+="\nTarama alanı için geçerli raw dönüşüm yok.";
        if(layer.points.isEmpty() && layer.lines.isEmpty() && layer.regions.isEmpty() && layer.mask.pixels.isEmpty() && layer.codes.items.isEmpty())layer.state+=" • Tarama alanı; sonuç yok";
        if(layer.points.isEmpty() && layer.lines.isEmpty() && layer.regions.isEmpty() && layer.scans.isEmpty() && layer.mask.pixels.isEmpty() && layer.codes.items.isEmpty())continue;
        out.layers<<layer;
    }
    return out;
}
}
