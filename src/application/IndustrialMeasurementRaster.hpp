#pragma once
#include "domain/MeasurementSetupContract.hpp"
#include "vision/ImageFrame.hpp"

namespace visionflow {
// Explicit conversion of the known full-sensor COE Mono8 raster. This is a
// measurement geometry, never a request to write camera Height/OffsetY.
struct IndustrialMeasurementRasterResult {
    ImageFrame frame;
    GeometryReadback acquisitionGeometry;
    GeometryDescriptor measurementGeometry;
    QRect regionInAcquisition;
    QByteArray inputContentId;
    QString policyId;
};
class IndustrialMeasurementRaster {
public:
    static GeometryDescriptor geometry(const PhysicalDeviceIdentity&,const GeometryReadback&);
    // Raw owned input only; the result carries the original hardware geometry.
    // No trigger admission, freshness, calibration activation or project write.
    static IndustrialMeasurementRasterResult excludeFirstSensorRow(
        const ImageFrame&,const PhysicalDeviceIdentity&,const GeometryReadback&);
    // Explicit candidate edit: uses the existing revision/invalidation rules.
    static MeasurementSetupDescriptor revisedSetup(const MeasurementSetupDescriptor&,
        const PhysicalDeviceIdentity&,const GeometryReadback&);
};
}
