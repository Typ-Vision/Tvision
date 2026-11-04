#include "hardware/PylonSession.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "application/CameraLeaseArbiter.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QFile>
#include "application/Deadline.hpp"
using namespace visionflow;
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);QCommandLineParser parser;
    parser.setApplicationDescription("pylon C exact USB probe; optional untrusted acquisition with settings restore.");parser.addHelpOption();
    for(const auto& key:QStringList{"vendor","model","serial","runtime"})parser.addOption({key,key,"value"});
    parser.addOption({"frames","Diagnostic frame count, 0..100","count","0"});
    parser.addOption({"preview","Use free-running preview instead of software triggers"});
    parser.addOption({"image","Save last diagnostic frame as new PGM","path"});
    parser.process(app);
    QJsonObject report{{"abi",PylonSession::abi()}};int exit=1;
    try{
        const PylonSelector selector{parser.value("vendor"),parser.value("model"),parser.value("serial")};
        const PhysicalDeviceIdentity physical{selector.vendor,selector.model,selector.serial,DeviceIdentityConfidence::SerialReported};
        auto leases=CameraLeaseArbiter::reserve("pylon-g0-probe",CameraLeaseUse::Preview,{{{},physical}});
        if(leases.reason)throw std::runtime_error("Physical camera lease busy");
        const auto config=CameraLocalConfiguration(qEnvironmentVariable("VISIONFLOW_CAMERA_CONFIGURATION"));
        const auto runtime=config.runtime("basler",parser.value("runtime"));
        report["runtime"]=runtime.path;if(!runtime.error.isEmpty())throw std::runtime_error(runtime.error.toStdString());
        // Reservation outlives the session, including a failed open/cleanup.
        PylonSession session;report["readback"]=session.open(runtime.path,runtime.dependencies,selector);
        bool ok=false;const int count=parser.value("frames").toInt(&ok);if(!ok || count<0 || count>100)throw std::runtime_error("frames must be 0..100");
        if(count){
            QJsonArray frames;Deadline deadline(60000);session.start(!parser.isSet("preview"),[&]{return deadline.remainingMs();});
            PylonFrame last;
            for(int i=0;i<count;++i){Deadline capture(5000);last=session.grab([&]{return std::min(deadline.remainingMs(),capture.remainingMs());});
                frames.append(QJsonObject{{"index",i},{"blockId",QString::number(last.blockId)},{"timestamp",QString::number(last.timestamp)},
                    {"width",last.size.width()},{"height",last.size.height()},{"offsetX",last.offset.x()},{"offsetY",last.offset.y()},
                    {"stride",double(last.stride)},{"bytes",last.pixels.size()},{"sha256",QString::fromLatin1(QCryptographicHash::hash(last.pixels,QCryptographicHash::Sha256).toHex())}});
            }
            session.stop();report["frames"]=frames;report["triggerAssociationVerified"]=false;report["productionAdmission"]=false;
            if(parser.isSet("image")){QFile file(parser.value("image"));if(!file.open(QIODevice::WriteOnly|QIODevice::NewOnly))throw std::runtime_error("New diagnostic PGM path required");
                const auto header=QString("P5\n# Basler diagnostic - trigger association unverified\n%1 %2\n255\n").arg(last.size.width()).arg(last.size.height()).toLatin1();
                if(file.write(header)!=header.size())throw std::runtime_error("PGM header write failed");
                for(int y=0;y<last.size.height();++y)if(file.write(last.pixels.constData()+size_t(y)*last.stride,last.size.width())!=last.size.width())throw std::runtime_error("PGM write failed");
            }
        }
        session.close();report["close"]="ok";report["status"]="DONE";exit=0;
    }catch(const std::exception& e){report["status"]="ERROR";report["error"]=QString::fromUtf8(e.what());}
    QTextStream(stdout)<<QJsonDocument(report).toJson();return exit;
}
