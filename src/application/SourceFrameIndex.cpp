#include "SourceFrameIndex.hpp"
#include <QSet>
#include <algorithm>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
bool bounded(const QString& id){return !id.isEmpty() && id==id.trimmed() && id.size()<=256 && std::all_of(id.cbegin(),id.cend(),[](QChar c){return c.isPrint();});}
}
SourceFrameIndex::SourceFrameIndex(const QString& cycle,const QStringList& sources,const QVector<FrameMetadata>& frames) {
    require(bounded(cycle),"Invalid cycle identity");
    require(!sources.isEmpty() && sources.size()<=maxSources && frames.size()==sources.size(),"Missing/oversized source-frame set");
    QSet<QString> required,ids,coordinates;
    for(const auto& source:sources){require(bounded(source) && !required.contains(source),"Invalid/duplicate required source");required.insert(source);}
    for(const auto& f:frames) {
        require(required.contains(f.sourceId) && !frames_.contains(f.sourceId),"Foreign/duplicate acquired source");
        require(bounded(f.frameId) && !ids.contains(f.frameId) && f.cycleId==cycle,"Invalid/duplicate/foreign frame identity");
        require(f.coordinateSystemId==f.sourceId+"/image" && !coordinates.contains(f.coordinateSystemId),"Source coordinate identity mismatch");
        require(f.deviceClockId.isEmpty() || bounded(f.deviceClockId),"Invalid/oversized device clock identity");
        frames_.insert(f.sourceId,f);ids.insert(f.frameId);coordinates.insert(f.coordinateSystemId);
    }
}
const FrameMetadata& SourceFrameIndex::frame(const QString& source) const {
    const auto it=frames_.constFind(source);require(it!=frames_.cend(),"Source not acquired in this cycle");return it.value();
}
void SourceFrameIndex::validatePort(const QString& source,const PortValue& value,const PortDefinition& port) const {
    const auto& acquired=frame(source);
    require(value.origin.sourceId==source && (value.imageGeometry ? value.imageGeometry->rootCoordinateSystemId():value.origin.coordinateSystemId)==acquired.coordinateSystemId,"Port source/coordinate mismatch");
    require(value.frameId==acquired.frameId && value.cycleId==acquired.cycleId && value.origin.frameId==acquired.frameId && value.origin.cycleId==acquired.cycleId,"Port acquired frame/cycle mismatch");
    ToolResultCodec::validate(value,port,acquired.frameId,acquired.cycleId);
}
QString SourceFrameIndex::legacyFrameId() const {return frames_.size()==1 ? frames_.cbegin()->frameId:QString{};}
}
