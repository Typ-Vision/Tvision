#pragma once
#include "domain/ToolRoi.hpp"
#include "vision/ImageAccess.hpp"
#include <atomic>
#include <memory>
#include <optional>
#include <functional>

namespace visionflow {
struct ProfileOptions {
    double stepPx=1, widthStepPx=1, sigmaPx=0;
};
struct IntensityProfileSample {
    double distancePx=0;
    QPointF point;
    double intensity=0; // 0..255, double; Gray16 is scaled without 8-bit quantization.
    std::optional<double> smoothed; // Missing where the full Gaussian support is unavailable.
};
struct ProfileDerivative {
    double distancePx=0;
    QPointF point;
    double value=0; // Signed intensity/px at the midpoint of adjacent smoothed samples.
};
struct SampledProfile {
    ScanSegment scan;
    ProfileOptions options;
    FrameMetadata metadata;
    QByteArray inputContentId,processingId;
    double actualStepPx=0,actualWidthStepPx=0;
    int widthSamples=0,smoothingRadius=0;
    QVector<IntensityProfileSample> samples;
    QVector<ProfileDerivative> derivatives;
};
// Maps a synthetic pixel-centre sampling plane to raw pixels. Caller owns domain checks.
using ProfileRawMap=std::function<QPointF(QPointF)>;
class ProfileSampler {
public:
    // Profile coordinates/options remain in the synthetic sampling plane;
    // every intensity tap is mapped separately (including transverse width).
    static SampledProfile sampleMapped(const ImageFrame&,const ScanSegment&,const ProfileRawMap&,
                                      const ProfileOptions& = {},const std::shared_ptr<std::atomic_bool>& cancel={});
    // Raw pixel-centre footprint only. No clipping, border padding or acquisition.
    static SampledProfile sample(const ImageFrame&,const ToolRoi&,const ProfileOptions& = {},
                                 const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
