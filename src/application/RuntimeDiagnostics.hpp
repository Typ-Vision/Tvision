#pragma once
#include "domain/Project.hpp"
#include <QJsonArray>
#include <QMutex>
#include <memory>

namespace visionflow {
// Operational evidence only: never persisted as production authority or images.
// A shared instance follows a Runtime lease onto its worker thread.
class RuntimeDiagnostics {
public:
    RuntimeDiagnostics();
    void append(QJsonObject event, bool failure=false) noexcept;
    QJsonObject snapshot() const;
    static QJsonObject recipeContext(const Recipe&) noexcept;
    static void publish(QJsonObject event) noexcept;
private:
    mutable QMutex mutex_;
    QJsonObject manifest_,firstFailure_;
    QJsonArray events_;
    QString session_;
    quint64 sequence_=0;
};
}
