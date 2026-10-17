#include "application/IndustrialMeasurementRaster.hpp"
#include "vision/ImageAccess.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>
#include <cstring>

using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F action){bool rejected=false;try{action();}catch(const std::runtime_error&){rejected=true;}check(rejected,"expected rejection");}
GeometryReadback fullGeometry(){
    GeometryReadback g;
    g.resolution={2448,2048};g.offset={0,0};
    g.offsetEvidence=g.binningEvidence=g.decimationEvidence=g.reverseEvidence=GeometryEvidence::Reported;
    return g;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const PhysicalDeviceIdentity device{"Opto Engineering Srl","COE-050-M-USB-050-IR-C","TEST-SERIAL",DeviceIdentityConfidence::SerialReported};
    const auto actual=fullGeometry();
    const auto hardwareBytes=QJsonDocument(actual.toJson()).toJson(QJsonDocument::Compact);
    constexpr int width=2448,height=2048,stride=2456;
    QByteArray pixels(stride*height,char(0xee));
    for(int y=0;y<height;++y)for(int x=0;x<width;++x)pixels[y*stride+x]=char((y*7+x*3)%251);
    // Simulate embedded metadata in row 0, which must never enter the output.
    std::memcpy(pixels.data()+4,"METADATA",8);
    FrameMetadata metadata;metadata.deviceTimestampTicks=12345;
    metadata.completedAtUtc=QDateTime::currentDateTimeUtc();metadata.coordinateSystemId="old-full-sensor";
    auto input=ImageFrame::fromBuffer(pixels.constData(),pixels.size(),width,height,stride,PixelFormat::Gray8,metadata,false);
    const auto originalHash=input.contentId();
    auto converted=IndustrialMeasurementRaster::excludeFirstSensorRow(input,device,actual);
    check(converted.frame.size()==QSize(width,height-1)&&converted.regionInAcquisition==QRect(0,1,width,height-1),"derived raster shape and origin");
    check(converted.acquisitionGeometry==actual&&QJsonDocument(actual.toJson()).toJson(QJsonDocument::Compact)==hardwareBytes,"hardware geometry remains unchanged");
    check(converted.measurementGeometry.sensorRoi==QRect(0,1,width,height-1)&&converted.measurementGeometry.complete,"sensor coordinate evidence");
    check(converted.inputContentId==originalHash&&converted.frame.contentId()!=originalHash,"input/output identity separated");
    check(!converted.policyId.isEmpty()&&converted.frame.metadata().coordinateSystemId.isEmpty(),"explicit policy and no stale coordinate ID");
    check(converted.frame.metadata().deviceTimestampTicks==metadata.deviceTimestampTicks
        &&converted.frame.metadata().completedAtUtc==metadata.completedAtUtc
        &&converted.frame.metadata().frameId.isEmpty()&&converted.frame.metadata().cycleId.isEmpty()
        &&!converted.frame.metadata().exposureAtUtc,"preserved actual metadata without invented admission");
    ImageAccess output(converted.frame);
    for(int y=0;y<height-1;++y){const auto row=output.row(y);
        check(row.size()==width&&std::memcmp(row.data(),pixels.constData()+(y+1)*stride,width)==0,"every retained sensor pixel is exact");}
    check(input.contentId()==originalHash,"conversion does not mutate input");
    const auto croppedHash=converted.frame.contentId();
    input={};pixels.fill(0);
    check(converted.frame.contentId()==croppedHash,"output owns storage after input destruction");

    MeasurementSetupDescriptor setup{{"setup"},{"camera"},"Setup","lens","plane",3,1,actual.descriptor()};
    const auto beforeBytes=QJsonDocument(setup.toJson()).toJson(QJsonDocument::Compact);
    const auto candidate=IndustrialMeasurementRaster::revisedSetup(setup,device,actual);
    const auto change=acquisitionChange({setup},{candidate});
    check(candidate.revision==4&&candidate.deviceRevision==setup.deviceRevision&&candidate.id==setup.id,"explicit geometry edit advances setup only");
    check(candidate.geometryId()!=setup.geometryId()&&change.geometry&&!change.calibrationCompatible()&&!change.cacheCompatible(),"old calibration and cache invalidated");
    check(IndustrialMeasurementRaster::revisedSetup(candidate,device,actual)==candidate,"repeated edit is a no-op");
    check(QJsonDocument(setup.toJson()).toJson(QJsonDocument::Compact)==beforeBytes,"original setup remains byte identical");
    check(MeasurementSetupDescriptor::fromJson(candidate.toJson())==candidate,"existing geometry schema round trip");
    AcquisitionProfile profile{{"profile"},{"camera"},{"setup"},"Manual",1,
        {"Mono8",{false,{},{}},AutoMode::Off,5000.,AutoMode::Off,0.,{}}};
    AcquisitionReadback readback{actual,profile.requested};
    check(!verifyApplied(setup,profile,readback).isEmpty(),"original hardware profile remains valid");
    rejects([&]{verifyApplied(candidate,profile,readback);});
    check(!change.preparationCompatible(),"measurement conversion requires fresh preparation");
    auto unrelated=setup;unrelated.geometry.sensorRoi=QRect(1,0,width,height);
    rejects([&]{IndustrialMeasurementRaster::revisedSetup(unrelated,device,actual);});
    auto unknown=actual;unknown.offsetEvidence=GeometryEvidence::Unknown;
    rejects([&]{IndustrialMeasurementRaster::geometry(device,unknown);});
    for(int scenario=0;scenario<6;++scenario){auto g=actual;
        switch(scenario){case 0:g.offset={0,1};break;case 1:g.reverse.y=true;break;case 2:g.reverse.x=true;break;
        case 3:g.binning={2,1};break;case 4:g.decimation={1,2};break;case 5:g.resolution={2448,2047};break;}
        rejects([&]{IndustrialMeasurementRaster::geometry(device,g);});}
    for(int scenario=0;scenario<3;++scenario){auto d=device;
        if(scenario==0)d.model="other";else if(scenario==1)d.vendor="other";else {d.confidence=DeviceIdentityConfidence::ProviderReported;d.serial.clear();}
        rejects([&]{IndustrialMeasurementRaster::geometry(d,actual);});}
    rejects([&]{IndustrialMeasurementRaster::excludeFirstSensorRow(converted.frame,device,actual);});
    QByteArray gray16(width*height*2,0);
    const auto wrongFormat=ImageFrame::fromBuffer(gray16.constData(),gray16.size(),width,height,width*2,PixelFormat::Gray16,{},false);
    rejects([&]{IndustrialMeasurementRaster::excludeFirstSensorRow(wrongFormat,device,actual);});
    const auto raw=ImageFrame::fromBuffer(gray16.constData(),width*height,width,height,width,PixelFormat::Gray8,{},false);
    auto processed=ImageAccess::processed(raw,ImageAccess(raw).ownedCopy(),PixelFormat::Gray8,"test",1,QByteArray(32,'a'),false);
    rejects([&]{IndustrialMeasurementRaster::excludeFirstSensorRow(processed,device,actual);});
    QTextStream(stdout)<<"SYNTHETIC measurement raster: exact retained pixels, origin, ownership, geometry invalidation and negative cases passed"<<Qt::endl;
    return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<Qt::endl;return 1;}}
