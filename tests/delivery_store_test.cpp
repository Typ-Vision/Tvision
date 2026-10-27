#include "application/DeliveryStore.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QUuid>
#include <QTextStream>
using namespace visionflow;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"expected rejection");}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
QJsonObject terminal(){const auto cycle=id();QJsonObject b{{"schemaVersion",1},{"cycleId",cycle},{"configHash","config"},{"dependencyId","dependency"},{"decision","NOT_EVALUATED"},{"groups",QJsonArray{}}};b["bundleId"]=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(b).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());QJsonObject t{{"schemaVersion",10},{"baseSchemaVersion",3},{"recordId",id()},{"cycleId",cycle},{"configId","config"},{"dependencyId","dependency"},{"state","ERROR"},{"tools",QJsonArray{}},{"resultBundle",b}};ProductionResultCodec::validate(t);return t;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir root(QCoreApplication::applicationDirPath()+"/delivery-XXXXXX");check(root.isValid(),"fixture");
    const QByteArray payload("{\"status\":\"INVALID\"}\n\0bytes",28);int sends=0;
    for(const QString boundary:{"REQUEST_INTENT","CYCLE_BEGIN","TERMINAL","DELIVERY_PENDING","ATTEMPT","ACK_RECEIVED","ACK_COMMITTED"}){
        const auto path=root.filePath(boundary);QString fail=boundary;
        DeliveryStore store(path,[&](const QString& p){if(p==fail)throw std::runtime_error("injected disk boundary");});
        DeliveryRequest r{"cell","project","endpoint","peer","request","fingerprint"};const auto t=terminal();
        if(boundary=="REQUEST_INTENT"){rejects([&]{store.request(r);});check(store.inspect(r).state=="ABSENT","failed intent absent");fail.clear();}
        check(store.request(r).state=="RECOVERY_REQUIRED","intent is recovery required");
        if(boundary=="CYCLE_BEGIN"){rejects([&]{store.begin(r,t["cycleId"].toString());});check(!store.dispatch(r,[&](auto){++sends;}),"no uncommitted dispatch");fail.clear();}
        store.begin(r,t["cycleId"].toString());
        if(boundary=="TERMINAL"||boundary=="DELIVERY_PENDING"){rejects([&]{store.commit(r,t,payload);});check(store.inspect(r).state=="RECOVERY_REQUIRED","no early pending");check(!store.dispatch(r,[&](auto){++sends;}),"orphan terminal not dispatchable");fail.clear();}
        store.commit(r,t,payload);store.commit(r,t,payload);check(store.inspect(r).state=="DELIVERY_PENDING","committed manifest");rejects([&]{store.commit(r,t,payload+"x");});
        auto conflict=r;conflict.fingerprint="other";rejects([&]{store.request(conflict);});
        if(boundary=="ATTEMPT"){rejects([&]{store.dispatch(r,[&](auto){++sends;});});check(store.inspect(r).attempts==0,"failed attempt no dispatch");fail.clear();}
        check(store.dispatch(r,[&](const auto& bytes){check(bytes==payload,"exact bytes");++sends;}),"committed dispatch");
        DeliveryStore reopened(path);check(reopened.inspect(r).payload==payload,"reopen immutable payload");check(reopened.request(r).attempts==1,"duplicate does not begin cycle");check(reopened.dispatch(r,[&](const QByteArray& bytes){check(bytes==payload,"retry exact persisted bytes");++sends;}),"reopen replay");check(store.inspect(r).attempts==2,"attempt ledger reopened");
        rejects([&]{store.acknowledge(r,"wrong");});const auto digest=store.inspect(r).payloadHash;
        if(boundary=="ACK_RECEIVED"){rejects([&]{store.acknowledge(r,digest);});check(store.inspect(r).state=="ATTEMPT","failed ACK receipt pending");fail.clear();}
        store.acknowledge(r,digest);
        if(boundary=="ACK_COMMITTED"){rejects([&]{store.commitAcknowledgement(r);});check(store.inspect(r).state=="ACK_RECEIVED","no early ACKED");fail.clear();}
        store.commitAcknowledgement(r);store.commitAcknowledgement(r);check(reopened.inspect(r).state=="ACK_COMMITTED","durable ACK");check(!store.dispatch(r,[&](auto){++sends;}),"ACK suppresses replay");
        // Removing the first journal record cannot silently recreate a new request.
        const auto files=QDir(path+"/delivery").entryInfoList({"*.json"},QDir::Files);for(const auto& f:files){QFile file(f.absoluteFilePath());check(file.open(QIODevice::ReadOnly),"journal open");const auto o=QJsonDocument::fromJson(file.readAll()).object();file.close();if(o["payload"].toObject()["sequence"]==0){check(file.remove(),"remove intent");break;}}
        rejects([&]{reopened.inspect(r);});auto unrelated=r;unrelated.requestId="another-request";rejects([&]{reopened.request(unrelated);});
    }
    check(sends==14,"only committed attempts dispatched");
    const auto absent=root.filePath("absent");DeliveryStore missing(absent);DeliveryRequest noRequest{"c","p","endpoint","peer","id","fp"};auto t=terminal();rejects([&]{missing.begin(noRequest,t["cycleId"].toString());});rejects([&]{missing.commit(noRequest,t,payload);});rejects([&]{missing.acknowledge(noRequest,"none");});
    const auto orphan=root.filePath("orphan");DeliveryStore broken(orphan);broken.request(noRequest);broken.begin(noRequest,t["cycleId"].toString());broken.commit(noRequest,t,payload);check(QFile::remove(orphan+"/cycles/"+t["recordId"].toString()+".json"),"remove terminal fixture");rejects([&]{broken.inspect(noRequest);});rejects([&]{broken.dispatch(noRequest,[&](auto){++sends;});});
    // Exercise a real QSaveFile/open failure as well as named boundary injection.
    const auto disk=root.filePath("disk");QDir().mkpath(disk);QFile obstruction(disk+"/delivery");check(obstruction.open(QIODevice::WriteOnly),"disk obstruction");obstruction.close();DeliveryStore unavailable(disk);rejects([&]{unavailable.request(noRequest);});check(unavailable.inspect(noRequest).state=="ABSENT","failed physical write does not create intent");
    const auto corrupt=root.filePath("corrupt");DeliveryStore c(corrupt);DeliveryRequest r{"c","p","endpoint","e","r","f"};c.request(r);const auto f=QDir(corrupt+"/delivery").entryInfoList({"*.json"},QDir::Files).first();QFile file(f.absoluteFilePath());check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"partial fixture");file.write("{");file.close();rejects([&]{c.inspect(r);});rejects([&]{c.request(r);});
    const auto legacy=root.filePath("legacy");ProductionStore(legacy).append("events",id(),{{"event","PLC_REQUEST"}});check(DeliveryStore(legacy).inspect(r).state=="ABSENT","legacy events not v3");
    QTextStream(stdout)<<"delivery store contract passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
