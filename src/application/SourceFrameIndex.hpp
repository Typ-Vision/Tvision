#pragma once
#include "ToolResultCodec.hpp"
namespace visionflow {
// Contract for future multi-source execution. One acquired raw frame per required
// source and cycle; processed images retain that origin. No cross-camera mapping.
class SourceFrameIndex {
public:
    static constexpr int maxSources=256;
    SourceFrameIndex(const QString& cycleId,const QStringList& requiredSources,const QVector<FrameMetadata>& frames);
    const FrameMetadata& frame(const QString& sourceId) const;
    void validatePort(const QString& expectedSource,const PortValue&,const PortDefinition&) const;
    // Compatibility field is defined only for a single acquired source.
    QString legacyFrameId() const;
private:
    QMap<QString,FrameMetadata> frames_;
};
}
