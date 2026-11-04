#include "hardware/PylonSession.hpp"
#include "application/CameraLocalConfiguration.hpp"
#include "application/CameraLeaseArbiter.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>
using namespace visionflow;
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);QCommandLineParser parser;
    parser.setApplicationDescription("pylon C exact USB open/readback/close; no writes or images.");parser.addHelpOption();
    for(const auto& key:QStringList{"vendor","model","serial","runtime"})parser.addOption({key,key,"value"});
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
        session.close();report["close"]="ok";report["status"]="DONE";exit=0;
    }catch(const std::exception& e){report["status"]="ERROR";report["error"]=QString::fromUtf8(e.what());}
    QTextStream(stdout)<<QJsonDocument(report).toJson();return exit;
}
