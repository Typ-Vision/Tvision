// Manual opt-in diagnostic; never registered as an automatic hardware CTest.
#include "application/IndustrialFrameProbe.hpp"
#include "vision/ImageAccess.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);
    QJsonObject out{{"productionAdmission",false},{"softwareTriggerAttempted",false}};int rc=0;
    try{
        if(argc!=6)throw std::runtime_error("usage: oecs_capture_probe absolute-dll exact-vendor exact-model exact-serial output.pgm");
        OecsSession session(makeOecsCApi(),QString::fromLocal8Bit(argv[1]));
        const auto info=session.open({QString::fromLocal8Bit(argv[2]),QString::fromLocal8Bit(argv[3]),QString::fromLocal8Bit(argv[4])});
        out["vendor"]=info.vendor;out["model"]=info.model;out["serial"]=info.serial;
        QJsonArray nodes;
        for(const QString& name: {"TriggerSelector","TriggerMode","TriggerSource","AcquisitionStatusSelector","AcquisitionStatus","TriggerCounter","FrameCounter","ChunkModeActive","ChunkSelector","EventSelector"}){
            QJsonObject node{{"name",name}};
            try{const auto f=session.feature(name);node["access"]=int(f.access);
                if(f.readable())node["value"]=std::visit([](const auto& v)->QJsonValue{return QVariant::fromValue(v).toJsonValue();},f.value);
                QJsonArray entries;for(const auto& entry:f.entries.keys())entries.append(entry);node["entries"]=entries;
            }catch(const OecsSessionError& e){node["error"]=e.what();node["stage"]=e.stage();node["code"]=QString::number(quint32(e.code()),16);}
            nodes.append(node);
        }out["nodes"]=nodes;
        // Avoid a misleading timeout in a pre-existing triggered session. No setting writes.
        const auto mode=session.feature("TriggerMode");
        if(!std::holds_alternative<QString>(mode.value)||std::get<QString>(mode.value)!="Off")throw std::runtime_error("Diagnostic requires actual TriggerMode Off; no settings changed");
        auto result=IndustrialFrameProbe::capture(session);
        session.close(); // evidence raster must survive SDK teardown
        QFile file(QString::fromLocal8Bit(argv[5]));if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot save raster");
        const auto header=QString("P5\n%1 %2\n255\n").arg(result.actual.width).arg(result.actual.height).toLatin1();
        if(file.write(header)!=header.size())throw std::runtime_error("Raster header write failed");
        ImageAccess access(result.frame);
        for(int y=0;y<int(result.actual.height);++y){const auto row=access.row(y);if(file.write(reinterpret_cast<const char*>(row.data()),qint64(row.size()))!=qint64(row.size()))throw std::runtime_error("Raster row write failed");}
        file.close();
        out["width"]=int(result.actual.width);out["height"]=int(result.actual.height);
        out["bytes"]=QString::number(result.actual.receivedBytes);out["stride"]=QString::number(result.actual.stride);
        out["hardwareFrameId"]=QString::number(result.actual.hardwareFrameId);out["deviceTicks"]=QString::number(result.actual.deviceTicks);
        out["contentId"]=QString::fromLatin1(result.frame.contentId().toHex());out["raster"]=file.fileName();out["status"]="OWNED_MONO8_DIAGNOSTIC";
    }catch(const OecsSessionError& e){out["error"]=e.what();out["stage"]=e.stage();out["code"]=QString::number(quint32(e.code()),16);out["status"]="ERROR";rc=1;
    }catch(const std::exception& e){out["error"]=e.what();out["status"]="ERROR";rc=1;}
    QTextStream(stdout)<<QJsonDocument(out).toJson();return rc;
}
