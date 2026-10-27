#include "application/DeliveryStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
using namespace visionflow;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool b=false;try{f();}catch(...){b=true;}check(b,"expected rejection");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 QTemporaryDir root(QCoreApplication::applicationDirPath()+"/retry-XXXXXX");check(root.isValid(),"fixture");
 DeliveryStore store(root.filePath("ledger"));DeliveryRequest r{"cell","project","endpoint","peer-generation-1","request","fp"};
 check(store.admission(r,true,false)=="ENDPOINT_OFFLINE","required offline closes admission");
 check(store.admission(r,false,false).isEmpty(),"optional offline can retain bundle");
 store.request(r);check(store.admission(r)=="DELIVERY_PENDING","required unresolved request blocks");
 auto next=r;next.peer="peer-generation-2";rejects([&]{store.rolloverNamespace(r,next,"peer confirmed");});
 for(int n=1;n<128;++n){auto q=r;q.requestId=QString::number(n);store.request(q);}
 check(store.admission(r,false)=="QUEUE_FULL","optional 128 explicit queue full, no eviction");auto overflow=r;overflow.requestId="overflow";rejects([&]{store.request(overflow);});check(store.recovery().size()==128,"all unresolved identities retained");
 DeliveryStore generations(root.filePath("generations"));generations.rolloverNamespace(r,next,"operator/peer drained and agreed generation 2");
 rejects([&]{generations.inspect(r);});rejects([&]{generations.request(r);});rejects([&]{generations.rolloverNamespace(next,r,"try old generation");});
 check(DeliveryStore(root.filePath("generations")).admission(r)=="NAMESPACE_RETIRED","retirement persists on restart");check(generations.admission(next).isEmpty(),"explicit new namespace armed");
 // Current shared disk record budget, including other archive kinds, closes admission.
 const auto full=root.filePath("full");QDir().mkpath(full+"/events");for(int n=0;n<9970;++n){QFile f(full+"/events/"+QString::number(n)+".json");check(f.open(QIODevice::WriteOnly),"budget fixture");f.write("{}");}
 DeliveryStore budget(full);check(budget.admission(r)=="QUEUE_FULL","shared archive budget rejects before intent");rejects([&]{budget.request(r);});check(budget.inspect(r).state=="ABSENT","budget refusal creates no request");
 QTextStream(stdout)<<"delivery retry admission/namespace/budget contract passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
