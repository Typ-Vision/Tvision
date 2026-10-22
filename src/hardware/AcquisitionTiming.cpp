#include "AcquisitionTiming.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <atomic>
#include <exception>
#include <mutex>

namespace visionflow {
// Only a trivial pointer has TLS lifetime. QString storage belongs to the scoped
// binding, avoiding runtime TLS destruction after the Qt/MinGW thread teardown.
namespace {thread_local const AcquisitionTimingContext* context=nullptr;std::atomic<quint64> sequence{0};std::mutex outputMutex;}
AcquisitionTimingContext AcquisitionTiming::current(){return context?*context:AcquisitionTimingContext{};}
QString AcquisitionTiming::nextId(){return QString::number(QCoreApplication::applicationPid())+"-"+QString::number(++sequence);}
bool AcquisitionTiming::enabled(){return !qEnvironmentVariableIsEmpty("VISIONFLOW_ACQUISITION_TIMING");}
AcquisitionTimingContext AcquisitionTiming::request(const QString& flow){return enabled()?AcquisitionTimingContext{nextId(),{},flow,Clock::now()}:AcquisitionTimingContext{};}
void AcquisitionTiming::record(const AcquisitionTimingContext& c,const QString& stage,Clock::time_point start,const QString& outcome,const QJsonObject& detail) noexcept {
    if(c.request.isEmpty())return;
    try {
        const auto end=Clock::now();
        const auto ns=[](auto t){return QString::number(std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count());};
        const QString path=qEnvironmentVariable("VISIONFLOW_ACQUISITION_TIMING");
        if(path.isEmpty() || !QFileInfo(path).isAbsolute())return;
        const QJsonObject row{{"schema","visionflow.acquisition-timing/1"},{"requestId",c.request},{"sessionId",c.session},
            {"flow",c.flow},{"stage",stage},{"startMonotonicNs",ns(start)},{"endMonotonicNs",ns(end)},
            {"durationMs",std::chrono::duration<double,std::milli>(end-start).count()},{"outcome",outcome},{"inclusive",true},{"detail",detail}};
        const auto bytes=QJsonDocument(row).toJson(QJsonDocument::Compact)+'\n';
        std::lock_guard lock(outputMutex);QFile file(path);if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write(bytes);
    }catch(...){/* Diagnostics must never change acquisition/error/cleanup behavior. */}
}
AcquisitionTimingBinding::AcquisitionTimingBinding(AcquisitionTimingContext c):value_(std::move(c)),previous_(context){context=&value_;}
AcquisitionTimingBinding::~AcquisitionTimingBinding(){context=previous_;}
AcquisitionTimingSpan::AcquisitionTimingSpan(QString stage,AcquisitionTiming::Clock::time_point start)
    :context_(AcquisitionTiming::current()),stage_(std::move(stage)),start_(start),exceptions_(std::uncaught_exceptions()),active_(!context_.request.isEmpty()){}
AcquisitionTimingSpan::~AcquisitionTimingSpan(){finish(std::uncaught_exceptions()>exceptions_?"error":"success");}
void AcquisitionTimingSpan::finish(const QString& outcome,const QJsonObject& detail) noexcept {if(active_){active_=false;AcquisitionTiming::record(context_,stage_,start_,outcome,detail);}}
}
