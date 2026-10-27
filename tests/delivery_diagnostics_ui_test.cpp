#include "app/DeliveryDiagnosticsWidget.hpp"
#include "application/ProductionResultCodec.hpp"
#include <QApplication>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QDir>
#include <QUuid>
#include <QTextStream>
using namespace visionflow;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
QJsonObject terminal(){auto cycle=uuid();QJsonObject b{{"schemaVersion",1},{"cycleId",cycle},{"configHash","config"},{"dependencyId","dependency"},{"decision","NOT_EVALUATED"},{"groups",QJsonArray{}}};b["bundleId"]=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(b).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());QJsonObject t{{"schemaVersion",10},{"baseSchemaVersion",3},{"recordId",uuid()},{"cycleId",cycle},{"configId","config"},{"dependencyId","dependency"},{"state","DONE"},{"tools",QJsonArray{}},{"resultBundle",b}};ProductionResultCodec::validate(t);return t;}
int main(int argc,char** argv){QApplication app(argc,argv);app.setStyle("Fusion");QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");try{
 QTemporaryDir dir(QCoreApplication::applicationDirPath()+"/diagnostics-XXXXXX");check(dir.isValid(),"fixture");DeliveryStore store(dir.path());
 DeliveryRequest r{"cell","project",QString(128,'e'),"peer",QString(128,'r'),"fingerprint"};store.request(r);auto t=terminal();store.begin(r,t["cycleId"].toString());
 QJsonObject payload{{"requestId",r.requestId},{"cycleId",t["cycleId"]},{"resultId","result"},{"execution","DONE"},{"decision","NOK"},{"fields",QJsonArray{QJsonObject{{"name","count"},{"type","Float64"},{"unit","mm"},{"validity","missing"},{"value",777},{"reason",QString(8000,'x')+"secret-password"}}}},{"password","secret-password"}};
 store.commit(r,t,QJsonDocument(payload).toJson(QJsonDocument::Compact));store.attempt(r);
 DeliveryRequest unknown{"cell","project","endpoint","peer","unknown","fp"};store.request(unknown);
 DeliveryDiagnosticsWidget widget([&]{return dir.path();});widget.show();auto* table=widget.findChild<QTableWidget*>("deliveryTable");auto* detail=widget.findChild<QPlainTextEdit*>("deliveryDetail");auto* retry=widget.findChild<QPushButton*>("deliveryRetry");
 check(table->rowCount()==2,"pending and recovery rows");int row=-1;for(int n=0;n<table->rowCount();++n)if(table->item(n,1)->text()==r.requestId)row=n;check(row>=0,"request row");table->selectRow(row);
 check(detail->toPlainText().contains("NOK") && detail->toPlainText().contains("missing") && !detail->toPlainText().contains("777") && !detail->toPlainText().contains("secret-password"),"missing does not show stale value; secret redaction");check(!retry->isEnabled(),"unbound retry disabled");
 RuntimeService runtime(dir.filePath("runtime"),dir.filePath("catalog.json"),RuntimeIdentity::localEngineer());RuntimeCoordinator coordinator(runtime);InputSnapshotStore inputs;TriggerRouter router(coordinator,runtime,inputs,store,[]{return QString{};});CommunicationTcpEndpoint endpoint(runtime,coordinator,router,inputs,store,dir.path(),dir.filePath("catalog.json"));widget.bindEndpoint(&endpoint);check(!retry->isEnabled(),"stopped endpoint retry disabled");
 auto* inspect=widget.findChild<QPushButton*>("deliveryInspect");inspect->setFocus();QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(inspect,&tab);check(widget.focusWidget()!=nullptr,"keyboard focus accessible");
 const auto artifacts=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS");QDir().mkpath(artifacts);for(const auto size:{QSize(1400,900),QSize(850,600)}){widget.resize(size);QApplication::processEvents();check(widget.size()==size,"no minimum width overflow");check(widget.grab().save(QDir(artifacts).filePath(QString("diagnostics-%1x%2.png").arg(size.width()).arg(size.height()))),"visual");}
 detail->verticalScrollBar()->setValue(detail->verticalScrollBar()->maximum());const auto scroll=detail->verticalScrollBar()->value();widget.refresh();check(detail->verticalScrollBar()->value()==scroll,"poll preserves detail scroll");
 store.acknowledge(r,store.inspect(r).payloadHash);widget.refresh();check(table->item(table->currentRow(),3)->text()=="ACK_RECEIVED","receipt not ACKED");store.commitAcknowledgement(r);widget.refresh();check(table->item(table->currentRow(),3)->text().contains("ACKED / ACK_COMMITTED"),"durable ACKED");
 table->selectRow(row==0?1:0);check(detail->toPlainText().contains("RECOVERY_REQUIRED") && !retry->isEnabled(),"unknown recovery inspect never ACKs or executes");
 // Corrupt journal errors stay bounded and do not copy filesystem/secret strings.
 auto files=QDir(dir.path()+"/delivery").entryList({"*.json"},QDir::Files);QFile bad(dir.path()+"/delivery/"+files.first());check(bad.open(QIODevice::WriteOnly|QIODevice::Truncate),"corrupt fixture");bad.write("secret-password{");bad.close();widget.refresh();check(table->rowCount()==0 && !retry->isEnabled(),"corrupt archive clears old data");check(widget.findChild<QLabel*>("deliveryMessage")->text().contains("RECOVERY_REQUIRED"),"bounded archive error");widget.grab().save(QDir(artifacts).filePath("diagnostics-850x600-error.png"));
 QTextStream(stdout)<<"PASS diagnostics: pending/recovery/ACK evidence, redaction, missing, bounded error, two sizes\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
