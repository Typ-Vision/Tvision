#include "application/EditorSession.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        EditorSession session;EditorContext context{"project","path","recipe",{}};
        auto captured=session.signature(context);check(session.accepts(captured,context),"snapshot accepted");
        auto changed=context;changed.recipeId="other";check(!session.accepts(captured,changed),"recipe switch stale");
        changed=context;changed.projectId="other";check(!session.accepts(captured,changed),"project switch stale");
        changed=context;changed.definition={{"changed",true}};check(!session.accepts(captured,changed),"configuration stale");
        ++session.generation;check(!session.accepts(captured,context),"new acquisition stale");
        QObject owner;auto* watcher=session.watch<int>(&owner,SessionJob::Execution);
        check(session.busy() && session.state()==SessionState::Working,"busy close guard");
        bool rejected=false;try{session.begin(SessionJob::Teaching);}catch(const std::exception&){rejected=true;}check(rejected,"conflicting job denied");
        auto token=session.token(SessionJob::Execution);session.stop(SessionJob::Execution);check(token->load() && session.state()==SessionState::Cancelling,"stop transition");
        QEventLoop loop;bool callbackIdle=false;
        QObject::connect(watcher,&QFutureWatcher<int>::finished,&owner,[&]{callbackIdle=!session.busy();check(watcher->result()==7,"worker result");loop.quit();});
        session.launch(watcher,[]{return 7;});QTimer::singleShot(5000,&loop,&QEventLoop::quit);loop.exec();check(callbackIdle && session.state()==SessionState::Idle,"finish before presentation");
        auto first=session.begin(SessionJob::Image),second=session.begin(SessionJob::Image);session.finish(first);check(session.busy(SessionJob::Image),"older image completion cannot clear newer worker");session.finish(second);check(!session.busy(),"all image jobs complete");
        CycleResult cycle;cycle.state=ExecutionState::Done;ToolResult r;r.regions={QRect(1,1,2,2)};r.points={QPointF(1,1)};r.searchRegion=QRect(1,1,2,2);r.outputs["value"].valid=true;cycle.tools={r};EditorSession::suppressCancelled(cycle);
        check(cycle.state==ExecutionState::Skipped && cycle.tools[0].outputs.isEmpty() && cycle.tools[0].regions.isEmpty() && cycle.tools[0].searchRegion.isEmpty(),"cancel suppression");
        std::atomic_bool completed=false;
        {JobSession pending;QObject scope;auto* work=pending.watch<int>(&scope,SessionJob::Discovery);auto cancel=pending.token(SessionJob::Discovery);pending.launch(work,[cancel,&completed]{while(!cancel->load())QThread::msleep(1);completed.store(true);return 0;});}
        check(completed.load(),"destructor cancels and waits owned worker");
        QTextStream(stdout)<<"Session contract passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
