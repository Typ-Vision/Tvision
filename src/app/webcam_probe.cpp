#include "application/WebcamAdapter.hpp"
#include "application/ExecutionService.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>
#include <QUuid>
#include <QFutureWatcher>
#include <QEventLoop>
#include <QtConcurrent/QtConcurrentRun>
using namespace visionflow;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QCommandLineParser p;p.addHelpOption();
    p.addOption({"index","Windows camera index","index","0"});p.addOption({"backend","Qt Multimedia (qt)","backend","qt"});p.addOption({"timeout","Cooperative acceptance deadline (ms)","ms","5000"});p.process(app);
    QJsonObject report;
    try {
        bool okIndex=false,okTime=false;int index=p.value("index").toInt(&okIndex),timeout=p.value("timeout").toInt(&okTime);
        if(!okIndex || !okTime || timeout<1 || timeout>60000)throw std::runtime_error("Invalid index/timeout");
        WebcamAdapter adapter({index,p.value("backend")});
        const auto source=QUuid::createUuid().toString();
        QFutureWatcher<AcquisitionResult> watcher;QEventLoop loop;
        QObject::connect(&watcher,&QFutureWatcher<AcquisitionResult>::finished,&loop,&QEventLoop::quit);
        watcher.setFuture(QtConcurrent::run([&]{return AcquisitionService{}.acquire(adapter,{source,"probe-cycle","probe-frame",TriggerMode::Software,timeout,{}});}));
        loop.exec();const auto r=watcher.result();
        report={{"state",r.state==AcquisitionState::Done ? "DONE" : r.state==AcquisitionState::Timeout ? "TIMEOUT" : "ERROR"},{"error",r.error},{"milliseconds",r.milliseconds},{"backend",p.value("backend")},{"deviceIndex",index},{"hardInterruption",false}};
        if(r.frame) {
            ToolDefinition camera{source,"webcam","Webcam_1","",true,defaultParameters("webcam")};
            ToolDefinition blob{QUuid::createUuid().toString(),"blob","Blob_1","",true,defaultParameters("blob")};
            auto definition=withEditorConnections(withEditorTools({}, {camera,blob}),{{source,"image",blob.id,"image"}});
            const auto cycle=ExecutionService{}.run(definition,{},"webcam-probe","probe",{}, {},r.frame->image);
            report["width"]=r.frame->image.size().width();report["height"]=r.frame->image.size().height();report["format"]=r.frame->image.pixelFormat();
            report["cycleId"]=cycle.cycleId;report["frameId"]=cycle.frameId;report["dagState"]=executionStateName(cycle.state);report["completedAtUtc"]=r.frame->acquiredAtUtc.toString(Qt::ISODateWithMs);
            if(cycle.state!=ExecutionState::Done)report["state"]="ERROR";
        }
    }catch(const std::exception& e){report={{"state","ERROR"},{"error",QString::fromUtf8(e.what())}};}
    QTextStream(stdout)<<QJsonDocument(report).toJson();return report["state"]=="DONE" ? 0 : 1;
}
