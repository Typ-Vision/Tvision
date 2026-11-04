#include "CameraService.hpp"
#include "CameraLocalConfiguration.hpp"
#include "WebcamAdapter.hpp"
#include "hardware/AcquisitionTiming.hpp"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <exception>
#include <stdexcept>
namespace visionflow {
CameraInventory CameraService::combine(const QVector<DeviceBinding>& webcams,const IndustrialDiscoverySnapshot& industrial){
    CameraInventory result;result.industrial=industrial;result.errors=industrial.errors;result.cancelled=industrial.cancelled;
    if(result.cancelled)return result;
    for(const auto& webcam:webcams){webcam.validate();if(webcam.adapter!="webcam")throw std::runtime_error("Expected webcam discovery entry.");result.devices.append({webcam,{}});}
    for(const auto& group:groupIndustrial(industrial))result.devices.append({{},group});
    return result;
}
IndustrialDiscoverySnapshot CameraService::industrialSnapshot(const CameraDiscovery& discovery){
    IndustrialDiscoverySnapshot result;result.errors=discovery.errors;result.cancelled=discovery.cancelled;result.producerResults=discovery.producerResults;
    if(result.cancelled)return result;
    QMap<QString,BackendIdentity> binaries;
    for(const auto& d:discovery.devices){
        try {
            const auto path=QFileInfo(d.producerPath).canonicalFilePath();
            if(!binaries.contains(path)){
                AcquisitionTimingSpan timing("discovery.backend-hash");
                QFile file(path);if(path.isEmpty() || !file.open(QIODevice::ReadOnly))throw std::runtime_error("Producer fingerprint unavailable.");
                QCryptographicHash hash(QCryptographicHash::Sha256);if(!hash.addData(&file))throw std::runtime_error("Producer fingerprint failed.");
                // File hash is the binary version identity; no SDK or G1 admission is inferred.
                binaries.insert(path,{"gentl","sha256:"+QString::fromLatin1(hash.result().toHex()),QString::fromLatin1(hash.result().toHex()),1});
            }
            PhysicalDeviceIdentity id;
            if(!d.vendor.isEmpty() && !d.model.isEmpty())id={d.vendor,d.model,d.serialNumber,
                d.serialNumber.isEmpty()?DeviceIdentityConfidence::ProviderReported:DeviceIdentityConfidence::SerialReported};
            DiscoveredDevice device{id,{"gentl",path,d.interfaceId,d.deviceId},binaries[path]};device.validate();result.devices.append(device);
        }catch(const std::exception& e){const auto error=d.producerPath+": "+QString::fromUtf8(e.what());result.errors.append(error);
            for(auto& p:result.producerResults)if(p.producer==d.producerPath){p.complete=false;p.errors.append(error);}}
    }return result;
}
CameraInventory CameraService::inventory(const QStringList& files,int timeout,const std::shared_ptr<std::atomic_bool>& cancel,bool explicitList) const {
    const auto scan=discover(files,timeout,cancel,250,explicitList);
    auto snapshot=industrialSnapshot(scan);
    auto result=combine(snapshot.cancelled?QVector<DeviceBinding>{}:WebcamAdapter::discover(),snapshot);
    if(cancel && cancel->load()){result.cancelled=true;result.devices.clear();result.industrial.devices.clear();result.industrial.cancelled=true;}
    result.producers=scan.producers;return result;
}
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
bool CameraService::routeComplete(const IndustrialDiscoverySnapshot& snapshot,const TransportLocator& route){
    if(snapshot.cancelled)return false;
    // Legacy/injected snapshots without provenance retain the conservative global gate.
    if(snapshot.producerResults.isEmpty())return snapshot.errors.isEmpty();
    for(const auto& error:snapshot.errors){bool attributed=false;for(const auto& p:snapshot.producerResults)if(p.errors.contains(error))attributed=true;if(!attributed)return false;}
    int matches=0;
    for(const auto& p:snapshot.producerResults)if(p.producer==route.producer){
        if(!p.complete || !p.errors.isEmpty())return false;
        ++matches;
    }
    return matches==1;
}
DiscoveredDevice CameraService::resolveIndustrial(const MeasurementCamera& camera,const IndustrialDiscoverySnapshot& snapshot,
    const std::optional<TransportLocator>& localRoute){
    camera.validate();
    if(!camera.physical || !camera.physical->usable())throw std::runtime_error("Exact industrial camera identity required.");
    if(snapshot.cancelled)throw std::runtime_error("Industrial discovery cancelled; resolve rejected.");
    // Validate the complete snapshot, including unrelated malformed records.
    groupIndustrial(snapshot);
    const auto routes=resolveRoutes(*camera.physical,snapshot.devices);
    if(localRoute){
        localRoute->validate();
        for(auto index:routes)if(snapshot.devices[index].route==*localRoute){
            if(!routeComplete(snapshot,*localRoute))throw std::runtime_error("Selected producer discovery incomplete/unavailable.");
            return snapshot.devices[index];}
        throw std::runtime_error("Selected industrial route offline; explicit route selection required.");
    }
    if(routes.size()!=1)throw std::runtime_error("Multiple industrial routes; explicit local route required.");
    if(!routeComplete(snapshot,snapshot.devices[routes.front()].route))throw std::runtime_error("Selected producer discovery incomplete/unavailable.");
    return snapshot.devices[routes.front()];
}
CameraDiscovery CameraService::discover(const QStringList& files,int timeoutMs,const std::shared_ptr<std::atomic_bool>& cancel,int scanWaitMs,bool explicitList) const {
    CameraDiscovery result;const auto config=qEnvironmentVariable("VISIONFLOW_CAMERA_CONFIGURATION");
    result.producers=explicitList || !files.isEmpty()?files:config.isEmpty()?genTLProducerPaths():CameraLocalConfiguration(config).automaticProducers();
    for(auto& file:result.producers){const auto canonical=QFileInfo(file).canonicalFilePath();file=canonical.isEmpty()?QFileInfo(file).absoluteFilePath():canonical;}
    QStringList unique;for(const auto& file:result.producers)if(!unique.contains(file,Qt::CaseInsensitive))unique.append(file);result.producers=unique;
    if(timeoutMs<=0){result.errors<<"Keşif timeout değeri pozitif olmalı.";return result;}
    if(scanWaitMs<=0){result.errors<<"Tarama bekleme değeri pozitif olmalı.";return result;}
    for(const auto& file:result.producers) {
        if(cancel && cancel->load()){result.cancelled=true;break;}
        ProducerDiscovery producer{file,false,{}};
        try {GenTLTransport transport(file);result.devices.append(transport.discover(timeoutMs,cancel,scanWaitMs));producer.complete=true;}
        catch(const std::exception& e){producer.errors<<file+": "+QString::fromUtf8(e.what());result.errors.append(producer.errors);}
        result.producerResults.append(producer);
    }
    if(cancel && cancel->load()){result.cancelled=true;result.devices.clear();}
    return result;
}
void CameraService::checkConnection(const CameraDevice& device) const {
    GenTLTransport transport(device.producerPath);transport.openReadOnly(device);transport.closeDevice();
}
}
