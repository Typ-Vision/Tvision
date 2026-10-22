#pragma once
#include <QJsonObject>
#include <QString>
#include <chrono>

namespace visionflow {
// Optional JSONL diagnostics, never persisted in recipes or frame provenance.
// Spans are inclusive intervals: aggregate by stage, never sum nested stages.
struct AcquisitionTimingContext {QString request,session,flow;std::chrono::steady_clock::time_point commandedAt;};
class AcquisitionTiming {
public:
    using Clock=std::chrono::steady_clock;
    static AcquisitionTimingContext current();
    static AcquisitionTimingContext request(const QString& flow);
    static QString nextId();
    static bool enabled();
    static void record(const AcquisitionTimingContext&,const QString& stage,Clock::time_point start,
                       const QString& outcome="success",const QJsonObject& detail={}) noexcept;
};
class AcquisitionTimingBinding {
    AcquisitionTimingContext value_;
    const AcquisitionTimingContext* previous_;
public:
    explicit AcquisitionTimingBinding(AcquisitionTimingContext);
    ~AcquisitionTimingBinding();
    AcquisitionTimingBinding(const AcquisitionTimingBinding&)=delete;
};
class AcquisitionTimingSpan {
    AcquisitionTimingContext context_;
    QString stage_;
    AcquisitionTiming::Clock::time_point start_;
    int exceptions_;
    bool active_;
public:
    explicit AcquisitionTimingSpan(QString stage,AcquisitionTiming::Clock::time_point start=AcquisitionTiming::Clock::now());
    ~AcquisitionTimingSpan();
    void finish(const QString& outcome="success",const QJsonObject& detail={}) noexcept;
    AcquisitionTimingSpan(const AcquisitionTimingSpan&)=delete;
};
}
