#include "hardware/OecsSession.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QTextStream>
#include <functional>
#include <stdexcept>
#include <thread>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F action,const char* message){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,message);}
struct FakeLog {
    QStringList calls;
    int liveHandles=0;
    bool loaded=false,unloadWithLiveHandles=false,destroyed=false;
    bool loadOk=true,unloadOk=true;
    int enumerateCode=0,createCode=0,openCode=0,closeCode=0,destroyCode=0;
    QVector<OecsDeviceInfo> devices;
};
class FakeApi : public OecsCApi {
public:
    explicit FakeApi(std::shared_ptr<FakeLog> log):log_(std::move(log)) {}
    ~FakeApi() override {log_->destroyed=true;}
    bool load(const QString&,QString& reason) override {log_->calls<<"load";if(!log_->loadOk){reason="fake load failure";return false;}log_->loaded=true;return true;}
    bool unload() override {
        log_->calls<<"unload";
        if(log_->liveHandles>0){log_->unloadWithLiveHandles=true;return false;}
        if(!log_->unloadOk)return false;
        log_->loaded=false;
        return true;
    }
    quint32 sdkVersion() override {return 0x01020304;}
    int enumerateUsb(QVector<OecsDeviceInfo>& devices) override {log_->calls<<"enumerate";if(log_->enumerateCode!=0)return log_->enumerateCode;devices=log_->devices;return 0;}
    int createHandle(int,void** handle) override {
        log_->calls<<"create";
        if(log_->createCode!=0){*handle=nullptr;return log_->createCode;}
        static int next=1;
        *handle=reinterpret_cast<void*>(quintptr(0x1000+16*next++));
        ++log_->liveHandles;
        return 0;
    }
    int openExclusive(void*) override {log_->calls<<"open";return log_->openCode;}
    int closeDevice(void*) override {log_->calls<<"close";return log_->closeCode;}
    int destroyHandle(void*) override {
        log_->calls<<"destroy";
        if(log_->destroyCode!=0)return log_->destroyCode;
        --log_->liveHandles;
        return 0;
    }
private:
    std::shared_ptr<FakeLog> log_;
};
QString fakeDll(){return QDir::temp().absoluteFilePath("fake-MvCameraControl.dll");}
OecsDeviceInfo device(const QString& serial,const QString& model="VF-CAM"){OecsDeviceInfo info;info.vendor="VF";info.model=model;info.serial=serial;info.userName="u";info.deviceVersion="1.0";info.usbVendorId=0x2BDF;info.usbProductId=0x0102;info.deviceNumber=7;return info;}
OecsDeviceSelector selector(const QString& serial){return {"VF","VF-CAM",serial};}
std::shared_ptr<FakeLog> makeLog(const QString& serial){auto log=std::make_shared<FakeLog>();log->devices<<device(serial);return log;}
std::unique_ptr<OecsSession> makeSession(const std::shared_ptr<FakeLog>& log,const QString& dll=fakeDll()){return std::make_unique<OecsSession>(std::make_unique<FakeApi>(log),dll);}
// Runs action, requires an OecsSessionError of the given kind and stage, returns it.
OecsSessionError expectFailure(const std::function<void()>& action,OecsFailure failure,const char* stage,const char* message) {
    try{action();}
    catch(const OecsSessionError& error){check(error.failure()==failure&&error.stage()==stage,message);return error;}
    throw std::runtime_error(message);
}
QStringList list(std::initializer_list<const char*> names){QStringList out;for(const char* name:names)out<<name;return out;}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const QStringList args=app.arguments();
        if(args.size()==3&&args[1]=="--sdk") {
            check(oecsSdkBindingCompiled(),"sdk binding compiled");
            OecsSession session(makeOecsCApi(),args[2]);
            const OecsDeviceSelector missing{"VF","NO-SUCH-MODEL","VF-NO-SUCH-SERIAL"};
            expectFailure([&]{session.open(missing);},OecsFailure::NotFound,"select","sdk smoke not found");
            check(session.stage()==OecsSessionStage::Unloaded&&session.sdkVersion()!=0,"sdk smoke not found");
            const quint32 version=session.sdkVersion();
            expectFailure([&]{session.open(missing);},OecsFailure::NotFound,"select","sdk reload");
            check(session.stage()==OecsSessionStage::Unloaded&&session.sdkVersion()==version,"sdk reload");
            auto api=makeOecsCApi();
            QString reason;
            check(api->load(args[2],reason),"sdk smoke not found");
            QVector<OecsDeviceInfo> devices;
            const int status=api->enumerateUsb(devices);
            check(api->unload(),"sdk smoke not found");
            QTextStream(stdout)<<"OECS sdk version 0x"<<QString::number(version,16)<<" usb devices "<<devices.size()<<" enumerate status "<<status<<Qt::endl;
            return 0;
        }
        { // 1 happy path
            auto log=makeLog("S-HAPPY");
            auto session=makeSession(log);
            const OecsDeviceInfo info=session->open(selector("S-HAPPY"));
            check(log->calls==list({"load","enumerate","create","open"})&&session->stage()==OecsSessionStage::Open&&info.serial=="S-HAPPY"&&info.deviceNumber==7&&session->sdkVersion()==0x01020304,"happy open order");
            session->close();
            check(log->calls==list({"load","enumerate","create","open","close","destroy","unload"})&&session->stage()==OecsSessionStage::Unloaded&&log->liveHandles==0&&!log->loaded&&!log->unloadWithLiveHandles,"happy close order");
            session->close();
            check(log->calls.size()==7,"close idempotent");
        }
        { // 2 load failure
            auto log=makeLog("S-LOAD");log->loadOk=false;
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-LOAD"));},OecsFailure::Unavailable,"load","load failure has no cleanup");
            check(log->calls==list({"load"})&&session->stage()==OecsSessionStage::Unloaded,"load failure has no cleanup");
        }
        { // 3 enumerate failure
            auto log=makeLog("S-ENUM");log->enumerateCode=oecs::errorHandle;
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-ENUM"));},OecsFailure::ApiError,"enumerate","enumerate failure unloads");
            check(log->calls==list({"load","enumerate","unload"})&&session->stage()==OecsSessionStage::Unloaded&&!log->unloadWithLiveHandles,"enumerate failure unloads");
        }
        { // 4 serial alone does not select
            auto log=makeLog("S-MODEL");log->devices.clear();log->devices<<device("S-MODEL","OTHER-MODEL");
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-MODEL"));},OecsFailure::NotFound,"select","serial alone does not select");
            check(log->calls==list({"load","enumerate","unload"}),"serial alone does not select");
        }
        { // 5 ambiguous
            auto log=makeLog("S-AMBIG");log->devices<<device("S-AMBIG");
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-AMBIG"));},OecsFailure::Ambiguous,"select","ambiguous identity rejected");
            check(log->calls==list({"load","enumerate","unload"}),"ambiguous identity rejected");
        }
        { // 6 empty serial
            auto log=makeLog("S-EMPTY");
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector(""));},OecsFailure::InvalidSelector,"select","empty serial rejected before sdk");
            check(log->calls.isEmpty(),"empty serial rejected before sdk");
        }
        { // 7 relative dll path
            auto log=makeLog("S-REL");
            auto session=makeSession(log,"MvCameraControl.dll");
            expectFailure([&]{session->open(selector("S-REL"));},OecsFailure::Unavailable,"load","relative dll path rejected");
            check(log->calls.isEmpty(),"relative dll path rejected");
        }
        { // 8 create failure releases claim
            auto log=makeLog("S-CREATE");log->createCode=oecs::errorParameter;
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-CREATE"));},OecsFailure::ApiError,"create","create failure releases claim");
            check(log->calls==list({"load","enumerate","create","unload"}),"create failure releases claim");
            log->createCode=0;log->calls.clear();
            auto again=makeSession(log);
            again->open(selector("S-CREATE"));
            check(again->stage()==OecsSessionStage::Open,"create failure releases claim");
            again->close();
        }
        { // 9 open failure
            auto log=makeLog("S-OPEN");log->openCode=oecs::errorParameter;
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-OPEN"));},OecsFailure::ApiError,"open","open failure destroys handle");
            check(log->calls==list({"load","enumerate","create","open","destroy","unload"})&&log->liveHandles==0&&!log->unloadWithLiveHandles,"open failure destroys handle");
        }
        { // 10 busy and access denied
            const int codes[2]={oecs::errorBusy,oecs::errorAccessDenied};
            const char* messages[2]={"vendor busy diagnosed","access denied diagnosed"};
            for(int i=0;i<2;++i) {
                auto log=makeLog(i==0?"S-BUSY":"S-DENIED");log->openCode=codes[i];
                auto session=makeSession(log);
                const OecsSessionError error=expectFailure([&]{session->open(selector(i==0?"S-BUSY":"S-DENIED"));},OecsFailure::Busy,"open",messages[i]);
                check(error.code()==codes[i]&&QString(error.what()).contains("meşgul")&&log->calls.mid(4)==list({"destroy","unload"}),messages[i]);
            }
        }
        { // 11 in-process owner
            auto logA=makeLog("S-OWNER");auto logB=makeLog("S-OWNER");
            auto a=makeSession(logA);auto b=makeSession(logB);
            a->open(selector("S-OWNER"));
            expectFailure([&]{b->open(selector("S-OWNER"));},OecsFailure::Busy,"owner","in-process owner busy");
            check(logB->calls==list({"load","enumerate","unload"}),"in-process owner busy");
            a->close();
            logB->calls.clear();
            b->open(selector("S-OWNER"));
            check(b->stage()==OecsSessionStage::Open,"owner released after close");
            b->close();
        }
        { // 12 failed destroy pins library
            auto log=makeLog("S-PIN");log->openCode=oecs::errorParameter;log->destroyCode=oecs::errorHandle;
            auto session=makeSession(log);
            expectFailure([&]{session->open(selector("S-PIN"));},OecsFailure::ApiError,"open","failed destroy pins library");
            check(log->calls==list({"load","enumerate","create","open","destroy"})&&session->stage()==OecsSessionStage::Faulted&&session->libraryPinned()&&!log->unloadWithLiveHandles,"failed destroy pins library");
            const int before=log->calls.size();
            expectFailure([&]{session->open(selector("S-PIN"));},OecsFailure::InvalidState,"state","faulted session refuses open");
            check(log->calls.size()==before,"faulted session refuses open");
        }
        { // 13 close failure still destroys
            auto log=makeLog("S-CLOSE");
            auto session=makeSession(log);
            session->open(selector("S-CLOSE"));
            log->closeCode=oecs::errorHandle;log->calls.clear();
            expectFailure([&]{session->close();},OecsFailure::CleanupFailed,"close","close failure still destroys");
            check(log->calls==list({"close","destroy","unload"})&&session->stage()==OecsSessionStage::Unloaded&&log->liveHandles==0&&!session->libraryPinned(),"close failure still destroys");
        }
        { // 14 destroy failure keeps library
            auto log=makeLog("S-DESTROY");
            auto session=makeSession(log);
            session->open(selector("S-DESTROY"));
            log->destroyCode=oecs::errorHandle;log->calls.clear();
            expectFailure([&]{session->close();},OecsFailure::CleanupFailed,"destroy","destroy failure keeps library");
            check(log->calls==list({"close","destroy"})&&session->stage()==OecsSessionStage::Faulted&&session->libraryPinned()&&!log->unloadWithLiveHandles,"destroy failure keeps library");
        }
        { // 15 unload refusal pins
            auto log=makeLog("S-UNLOAD");
            auto session=makeSession(log);
            session->open(selector("S-UNLOAD"));
            log->unloadOk=false;log->calls.clear();
            expectFailure([&]{session->close();},OecsFailure::CleanupFailed,"unload","unload refusal pins");
            check(session->stage()==OecsSessionStage::Faulted&&session->libraryPinned(),"unload refusal pins");
        }
        { // 17 foreign thread
            auto log=makeLog("S-THREAD");
            auto session=makeSession(log);
            bool openRejected=false,closeRejected=false;
            std::thread([&]{
                try{session->open(selector("S-THREAD"));}catch(const OecsSessionError& error){openRejected=error.failure()==OecsFailure::WrongThread&&error.stage()=="thread";}
            }).join();
            check(openRejected&&log->calls.isEmpty(),"foreign thread open rejected");
            session->open(selector("S-THREAD"));
            const int before=log->calls.size();
            std::thread([&]{
                try{session->close();}catch(const OecsSessionError& error){closeRejected=error.failure()==OecsFailure::WrongThread;}
            }).join();
            check(closeRejected&&log->calls.size()==before&&session->stage()==OecsSessionStage::Open,"foreign thread close rejected");
            session->close();
            check(session->stage()==OecsSessionStage::Unloaded,"foreign thread close rejected");
        }
        { // 18 double open
            auto log=makeLog("S-DOUBLE");
            auto session=makeSession(log);
            session->open(selector("S-DOUBLE"));
            const int before=log->calls.size();
            expectFailure([&]{session->open(selector("S-DOUBLE"));},OecsFailure::InvalidState,"state","double open rejected");
            check(log->calls.size()==before,"double open rejected");
            session->close();
        }
        { // 19 destructor on owner
            auto log=makeLog("S-DTOR");
            {
                auto session=makeSession(log);
                session->open(selector("S-DTOR"));
                log->calls.clear();
            }
            check(log->calls==list({"close","destroy","unload"}),"destructor cleans up on owner");
        }
        { // 20 destructor on foreign thread
            auto log=makeLog("S-FDTOR");
            auto session=makeSession(log);
            session->open(selector("S-FDTOR"));
            log->calls.clear();
            std::thread([&]{session.reset();}).join();
            check(log->calls.isEmpty()&&log->destroyed,"foreign destructor makes no sdk call");
        }
        { // 21 real binding without runtime
            OecsSession session(makeOecsCApi(),QDir::temp().absoluteFilePath("vf-no-such-dir/MvCameraControl.dll"));
            const OecsSessionError error=expectFailure([&]{session.open(selector("S-REAL"));},OecsFailure::Unavailable,"load","real binding unavailable without runtime");
            check(oecsSdkBindingCompiled()||QString(error.what())=="OECS SDK başlıkları olmadan derlendi; OECS C API kullanılamaz","real binding unavailable without runtime");
        }
        QTextStream(stdout)<<"oecs_session_test passed"<<Qt::endl;
        return 0;
    } catch(const std::exception& error) {
        QTextStream(stderr)<<"FAIL: "<<error.what()<<Qt::endl;
        return 1;
    }
}
