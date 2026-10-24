#include "application/IndustrialMeasurementRaster.hpp"
#include "vision/ImageAccess.hpp"
#include <stdexcept>

namespace visionflow {
GeometryDescriptor IndustrialMeasurementRaster::geometry(const PhysicalDeviceIdentity& device,const GeometryReadback& actual){
    device.validate();
    if(!device.usable()||device.vendor!="Opto Engineering Srl"||device.model!="COE-050-M-USB-050-IR-C")
        throw std::runtime_error("First sensor row policy requires the supported exact camera identity");
    // descriptor() rejects every Unknown critical feature. A reversed/offset or
    // sampled raster would require a different sensor-to-image transform.
    auto result=actual.descriptor();
    if(actual.resolution!=QSize(2448,2048)||actual.offset!=QPoint(0,0)||actual.binning!=QSize(1,1)
       ||actual.decimation!=QSize(1,1)||actual.reverse!=GeometryMirror{})
        throw std::runtime_error("First sensor row policy requires an unmodified full sensor raster");
    result.resolution={2448,2047};
    result.sensorRoi=QRect(0,1,2448,2047);
    result.validate();
    return result;
}
IndustrialMeasurementRasterResult IndustrialMeasurementRaster::excludeFirstSensorRow(
    const ImageFrame& input,const PhysicalDeviceIdentity& device,const GeometryReadback& actual){
    const auto measurement=geometry(device,actual);
    if(input.size()!=actual.resolution||input.format()!=PixelFormat::Gray8||!input.processing().isEmpty())
        throw std::runtime_error("First sensor row policy requires a raw full sensor Mono8 frame");
    // ImageAccess owns a shared reference throughout the copy. fromBuffer creates
    // independent storage and checks the entire remaining byte/stride envelope.
    const ImageAccess access(input);
    const auto first=access.row(1);
    const auto stride=input.stride();
    const auto bytes=std::size_t(measurement.resolution.height()-1)*stride+first.size();
    auto metadata=input.metadata();
    // Existing coordinate IDs describe the incoming raster; do not carry one
    // across a geometry change. The typed descriptor is the coordinate evidence.
    metadata.coordinateSystemId.clear();
    auto frame=ImageFrame::fromBuffer(first.data(),bytes,2448,2047,stride,PixelFormat::Gray8,metadata,false);
    return {std::move(frame),actual,measurement,QRect(0,1,2448,2047),input.contentId(),
        "coe050-mono8-exclude-sensor-row0-v1"};
}
MeasurementSetupDescriptor IndustrialMeasurementRaster::revisedSetup(const MeasurementSetupDescriptor& setup,
    const PhysicalDeviceIdentity& device,const GeometryReadback& actual){
    setup.validate();
    const auto measurement=geometry(device,actual);
    if(setup.geometry!=actual.descriptor()&&setup.geometry!=measurement)
        throw std::runtime_error("Measurement setup does not match acquisition or derived geometry");
    auto candidate=setup;
    candidate.geometry=measurement;
    return setup.revised(candidate);
}
}
