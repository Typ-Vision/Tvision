#include "RuntimeDiagnostics.hpp"
#include "domain/ToolDefinition.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QUuid>

Q_LOGGING_CATEGORY(runtimeDiagnosticsLog,"visionflow.runtime")
namespace visionflow {
RuntimeDiagnostics::RuntimeDiagnostics():session_(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
    // Compute once per executable, even when a UI opens multiple workspaces.
    static const QJsonObject executable=[] {
        const auto path=QCoreApplication::applicationFilePath();QFile file(path);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        const bool ok=file.open(QIODevice::ReadOnly) && hash.addData(&file);
        return QJsonObject{{"exePath",path},{"exeSha256",ok ? QString::fromLatin1(hash.result().toHex()):QString()},
            {"exeHashStatus",ok ? "ok":"unavailable"},{"buildType",VISIONFLOW_DIAGNOSTIC_BUILD_TYPE},
            {"widgets",bool(VISIONFLOW_DIAGNOSTIC_UI)},{"zxing",bool(VISIONFLOW_DIAGNOSTIC_ZXING)},
            {"qtVersion",qVersion()},{"compiler",__VERSION__}};
    }();
    manifest_=executable;manifest_["sessionId"]=session_;publish(manifest_);
}
void RuntimeDiagnostics::publish(QJsonObject event) noexcept {
    try {
        if(!event.contains("atUtc"))event["atUtc"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        if(event.contains("reason"))event["reason"]=event["reason"].toString().left(2048);
        qCInfo(runtimeDiagnosticsLog).noquote()<<QJsonDocument(event).toJson(QJsonDocument::Compact);
    }catch(...){}
}
void RuntimeDiagnostics::append(QJsonObject event,bool failure) noexcept {
    try {
        QMutexLocker lock(&mutex_);
        event["sessionId"]=session_;event["sequence"]=QString::number(++sequence_);
        event["atUtc"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);event["failure"]=failure;
        // Bound free-form errors as well as event count; never include pixels/port payloads.
        if(event.contains("reason"))event["reason"]=event["reason"].toString().left(2048);
        if(failure && firstFailure_.isEmpty())firstFailure_=event;
        if(!firstFailure_.isEmpty())event["firstFailureSequence"]=firstFailure_["sequence"];
        if(events_.size()==256)events_.removeFirst();events_.append(event);publish(event);
    }catch(...){} // Diagnostics must not alter admission, cancellation or record retry.
}
QJsonObject RuntimeDiagnostics::snapshot() const {
    QMutexLocker lock(&mutex_);return {{"manifest",manifest_},{"firstFailure",firstFailure_},{"events",events_}};
}
QJsonObject RuntimeDiagnostics::recipeContext(const Recipe& recipe) noexcept {
    try {
    QJsonArray sources;
    for(const auto& tool:editorTools(recipe.definition))if(tool.enabled && (tool.kind=="image_file" || tool.kind=="webcam" || tool.kind=="industrial_camera"))sources.append(QJsonObject{{"sourceId",tool.id},{"kind",tool.kind}});
    return {{"recipeId",recipe.id},{"recipeSchema",recipe.schemaVersion},{"recipeVersion",recipe.recipeVersion},{"applicationType",typeKey(recipe.applicationType)},{"editorSchema",recipe.definition["editor"].toObject()["schemaVersion"]},
        {"sourceCount",sources.size()},{"sources",sources}};
    }catch(...){return {};}
}
}
