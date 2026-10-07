#include "application/CameraService.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <exception>
using namespace visionflow;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QCoreApplication::setApplicationName("visionflow_camera_probe");
    QCommandLineParser parser;parser.setApplicationDescription("GenTL producer keşfi ve isteğe bağlı salt okunur bağlantı kontrolü; görüntü/trigger çalıştırmaz.");parser.addHelpOption();
    parser.addOption({"producer","Açık .cti producer yolu (birden fazla kullanılabilir).","path"});
    parser.addOption({"timeout","Her producer için keşif bütçesi (ms).","ms","1000"});
    parser.addOption({"scan-wait","Her liste güncellemesi için azami tarama beklemesi (ms); kalan bütçenin yarısıyla sınırlanır.","ms","250"});
    parser.addOption({"open-check","Keşfedilen cihazları salt okunur açıp kapat."});parser.process(app);
    bool ok=false;const auto timeout=parser.value("timeout").toInt(&ok);
    if(!ok || timeout<=0 || timeout>60000){QTextStream(stderr)<<"Timeout 1–60000 ms aralığında olmalı.\n";return 2;}
    const auto scanWait=parser.value("scan-wait").toInt(&ok);
    if(!ok || scanWait<=0 || scanWait>60000){QTextStream(stderr)<<"Scan wait 1–60000 ms aralığında olmalı.\n";return 2;}
    const auto discovery=CameraService{}.discover(parser.values("producer"),timeout,{},scanWait);
    QJsonArray devices,producers,errors;for(const auto& p:discovery.producers)producers.append(p);for(const auto& e:discovery.errors)errors.append(e);
    for(const auto& d:discovery.devices) {
        QJsonObject device{{"producer",d.producerPath},{"interfaceId",d.interfaceId},{"deviceId",d.deviceId},{"vendor",d.vendor},{"model",d.model},{"serial",d.serialNumber},{"transport",d.transport}};
        if(parser.isSet("open-check"))try {CameraService{}.checkConnection(d);device["readOnlyOpenClose"]="ok";}
        catch(const std::exception& e){device["readOnlyOpenClose"]="error";errors.append(QString::fromUtf8(e.what()));}
        devices.append(device);
    }
    const QString status=producers.isEmpty() ? "NO_PRODUCER" : !errors.isEmpty() ? "ERROR" : "DONE";
    QTextStream(stdout)<<QJsonDocument(QJsonObject{{"status",status},{"producers",producers},{"devices",devices},{"errors",errors},{"discoveryBudgetMs",timeout},{"scanWaitCapMs",scanWait},{"acquisitionImplemented",false}}).toJson();
    return status=="DONE" ? 0 : 1;
}
