#include "CameraService.hpp"
#include <algorithm>
#include <exception>
#include <stdexcept>
namespace visionflow {
QVector<IndustrialDeviceGroup> CameraService::groupIndustrial(const IndustrialDiscoverySnapshot& snapshot){
    QVector<IndustrialDeviceGroup> groups;
    for(const auto& device:snapshot.devices){
        device.validate();
        auto group=groups.end();
        if(device.identity.usable())
            group=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.identity==device.identity;});
        if(group==groups.end())groups.append({device.identity,{device},false});
        else group->routes.append(device);
    }
    for(auto& group:groups){
        if(!group.identity.usable())continue;
        try {resolveRoutes(group.identity,group.routes);}
        catch(const std::exception&){group.ambiguous=true;}
    }
    return groups;
}
DiscoveredDevice CameraService::resolveIndustrial(const MeasurementCamera& camera,const IndustrialDiscoverySnapshot& snapshot,
    const std::optional<TransportLocator>& localRoute){
    camera.validate();
    if(!camera.physical || !camera.physical->usable())throw std::runtime_error("Exact industrial camera identity required.");
    if(snapshot.cancelled || !snapshot.errors.isEmpty())throw std::runtime_error("Industrial discovery incomplete/unavailable; resolve rejected.");
    // Validate the complete snapshot, including unrelated malformed records.
    groupIndustrial(snapshot);
    const auto routes=resolveRoutes(*camera.physical,snapshot.devices);
    if(localRoute){
        localRoute->validate();
        for(auto index:routes)if(snapshot.devices[index].route==*localRoute)return snapshot.devices[index];
        throw std::runtime_error("Selected industrial route offline; explicit route selection required.");
    }
    if(routes.size()!=1)throw std::runtime_error("Multiple industrial routes; explicit local route required.");
    return snapshot.devices[routes.front()];
}
CameraDiscovery CameraService::discover(const QStringList& files,int timeoutMs,const std::shared_ptr<std::atomic_bool>& cancel,int scanWaitMs) const {
    CameraDiscovery result;result.producers=files.isEmpty() ? genTLProducerPaths() : files;
    result.producers.removeDuplicates();
    if(timeoutMs<=0){result.errors<<"Keşif timeout değeri pozitif olmalı.";return result;}
    if(scanWaitMs<=0){result.errors<<"Tarama bekleme değeri pozitif olmalı.";return result;}
    for(const auto& file:result.producers) {
        if(cancel && cancel->load()){result.cancelled=true;break;}
        try {GenTLTransport transport(file);result.devices.append(transport.discover(timeoutMs,cancel,scanWaitMs));}
        catch(const std::exception& e){result.errors<<file+": "+QString::fromUtf8(e.what());}
    }
    if(cancel && cancel->load()){result.cancelled=true;result.devices.clear();}
    return result;
}
void CameraService::checkConnection(const CameraDevice& device) const {
    GenTLTransport transport(device.producerPath);transport.openReadOnly(device);transport.closeDevice();
}
}
