#pragma once
#include "domain/IndustrialSourceReference.hpp"
#include "domain/ToolGraph.hpp"
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <stdexcept>
namespace visionflow {
// A backend choice, never an admission or permission. Opening is still explicit.
struct AcquisitionPolicy {
    static bool oecs(const PhysicalDeviceIdentity& id) {
        return id.usable() && id.vendor=="Opto Engineering Srl" && id.model=="COE-050-M-USB-050-IR-C";
    }
    static bool camera(const MeasurementCamera& c) {return c.physical && oecs(*c.physical);}
    static void profile(const AcquisitionProfile& p) {
        if(p.pending || p.requested.trigger!=TriggerSettings{true,"FrameBurstStart","Software"})
            throw std::runtime_error("Camera requires an explicit FrameBurstStart / Software profile; update the profile before acquisition.");
    }
    static bool replay(const QString& path) {
        QFile f(path);if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Source image unavailable");
        const bool marked=f.read(64).startsWith("P5\n# VisionFlow-OECS-Experimental-Unverified\n");
        QFile sidecar(path+".json");bool evidence=false;
        if(sidecar.open(QIODevice::ReadOnly) && sidecar.size()<=1024*1024)
            evidence=QJsonDocument::fromJson(sidecar.readAll()).object()["schema"]=="visionflow.oecs-experimental1";
        // Detection only. The existing bounded image loader verifies sidecar,
        // readback and raster hash before any frame is accepted.
        return marked || evidence;
    }
    // Call with only the graph's required source tools. Mixed native/OECS camera
    // admission is not supported; ordinary files may accompany OECS raw sources.
    static bool sources(const QVector<ToolDefinition>& tools,const QString& directory) {
        bool selected=false,native=false,webcam=false;
        for(const auto& t:tools)if(t.enabled){
            if(t.kind=="industrial_camera"){
                const auto r=IndustrialSourceReference::fromJson(t.parameters["sourceReference"].toObject());
                if(oecs(r.physical))selected=true;else native=true;
            }else if(t.kind=="image_file")selected=replay(QDir(directory).absoluteFilePath(t.parameters["path"].toString())) || selected;
            else if(t.kind=="webcam")webcam=true;
        }
        if(selected && (native || webcam))throw std::runtime_error("OECS acquisition cannot be combined with native camera or webcam sources in one Runtime recipe.");
        return selected;
    }
    static bool recipe(const QJsonObject& definition,const QString& directory) {
        auto tools=editorTools(definition);
        if(definition["editor"].toObject()["schemaVersion"]==9){
            const auto graph=validateSourceGraph(tools,editorConnections(definition));
            if(!graph.valid())throw std::runtime_error("Invalid acquisition source graph");
            tools.erase(std::remove_if(tools.begin(),tools.end(),[&](const auto& t){return !graph.requiredSources.contains(t.id);}),tools.end());
        }
        return sources(tools,directory);
    }
};
}
