#include "CameraService.hpp"
#include <exception>
namespace visionflow {
CameraDiscovery CameraService::discover(const QStringList& files,int timeoutMs,const std::shared_ptr<std::atomic_bool>& cancel) const {
    CameraDiscovery result;result.producers=files.isEmpty() ? genTLProducerPaths() : files;
    result.producers.removeDuplicates();
    if(timeoutMs<=0){result.errors<<"Keşif timeout değeri pozitif olmalı.";return result;}
    for(const auto& file:result.producers) {
        if(cancel && cancel->load()){result.cancelled=true;break;}
        try {GenTLTransport transport(file);result.devices.append(transport.discover(timeoutMs,cancel));}
        catch(const std::exception& e){result.errors<<file+": "+QString::fromUtf8(e.what());}
    }
    if(cancel && cancel->load()){result.cancelled=true;result.devices.clear();}
    return result;
}
void CameraService::checkConnection(const CameraDevice& device) const {
    GenTLTransport transport(device.producerPath);transport.openReadOnly(device);transport.closeDevice();
}
}
