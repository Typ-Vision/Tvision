#include "hardware/CameraFeatures.hpp"
#include <cmath>
#include <stdexcept>
namespace visionflow {
QStringList acquisitionFeatureNames(){return {"DeviceVendorName","DeviceModelName","DeviceSerialNumber","SensorWidth","SensorHeight","Width","Height","OffsetX","OffsetY","BinningHorizontal","BinningVertical","DecimationHorizontal","DecimationVertical","ReverseX","ReverseY","PixelFormat","PayloadSize","ExposureAuto","ExposureTime","GainAuto","Gain","AcquisitionMode","TriggerSelector","TriggerSource","TriggerMode"};}
bool featureEquivalent(const CameraFeature& f,const FeatureValue& requested){
    if(!f.readable() || f.value.index()!=requested.index())return false;
    if(f.type!=FeatureType::Float)return f.value==requested;
    const double a=std::get<double>(f.value),b=std::get<double>(requested);
    // The OECS ABI represents floats as IEEE float, allow only its rounding error.
    return std::isfinite(a)&&std::isfinite(b)&&(a==b || a==double(float(b)));
}
void validateFeatureWrite(const CameraFeature& f,const FeatureValue& value){
    auto fail=[&]{throw std::runtime_error(("Feature write rejected: "+f.name).toStdString());};
    if(!f.readable() || f.value.index()!=value.index())fail();
    if(f.type==FeatureType::Integer || f.type==FeatureType::Float){
        double n=f.type==FeatureType::Integer?double(std::get<qint64>(value)):std::get<double>(value);
        if(!std::isfinite(n)||!std::isfinite(f.minimum)||!std::isfinite(f.maximum)||n<f.minimum||n>f.maximum)fail();
        if(f.increment){const double inc=*f.increment;if(!std::isfinite(inc)||inc<=0)fail();const double steps=(n-f.minimum)/inc;if(std::abs(steps-std::round(steps))>1e-7)fail();}
    }
    if(f.type==FeatureType::Enumeration && !f.entries.contains(std::get<QString>(value)))fail();
    if(f.type==FeatureType::Unsupported)fail();
    if(!featureEquivalent(f,value)&&f.access!=FeatureAccess::ReadWrite)fail();
}
}
