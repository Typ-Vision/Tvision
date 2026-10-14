#pragma once
#include "ExecutionService.hpp"
namespace visionflow {
class ToolResultCodec {
public:
    static void validate(const PortValue&,const PortDefinition&,const QString& frameId,const QString& cycleId);
    static QJsonValue payload(const PortValue&);
    static QString display(const PortValue&,int precision=6);
    static std::optional<CoordinateFrame> coordinates(const PortValue&);
    // Non-image ports roundtrip; images stay in the owned frame store.
    static QJsonObject encode(const PortValue&);
    static PortValue decode(const QJsonObject&);
};
}
