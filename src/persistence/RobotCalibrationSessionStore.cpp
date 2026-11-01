#include "RobotCalibrationSessionStore.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <stdexcept>

using namespace visionflow;

namespace {
QString text(const QUuid& id) {
    return id.isNull() ? QString() : id.toString(QUuid::WithoutBraces);
}

QUuid uuid(const QString& value) {
    return value.isEmpty() ? QUuid{} : QUuid(value);
}

QJsonObject commandKeyToJson(const CalibrationCommandKey& key) {
    return QJsonObject{
        {"installation", text(key.installation)},
        {"peer", text(key.peer)},
        {"epoch", text(key.epoch)},
        {"station", text(key.station)},
        {"request", QString::number(key.request)}};
}

CalibrationCommandKey commandKeyFromJson(const QJsonObject& object) {
    return CalibrationCommandKey{uuid(object.value("installation").toString()),
                                 uuid(object.value("peer").toString()),
                                 uuid(object.value("epoch").toString()),
                                 uuid(object.value("station").toString()),
                                 object.value("request").toString().toULongLong()};
}

QJsonObject physicalToJson(const PhysicalDeviceIdentity& physical) {
    return physical.toJson();
}

PhysicalDeviceIdentity physicalFromJson(const QJsonObject& object) {
    return PhysicalDeviceIdentity::fromJson(object);
}

QJsonObject dependenciesToJson(const CalibrationDependencySnapshot& dependencies) {
    const auto& pose = dependencies.pose;
    return QJsonObject{
        {"mode", int(dependencies.mode)},
        {"cameraKey", int(dependencies.cameraKey)},
        {"cameraRevision", text(dependencies.cameraRevision)},
        {"intrinsicKey", int(dependencies.intrinsicRevisionKey)},
        {"intrinsicRevision", text(dependencies.intrinsicRevision)},
        {"plateKey", int(dependencies.plateRevisionKey)},
        {"plateRevision", text(dependencies.plateRevision)},
        {"flangeTcpKey", int(dependencies.flangeTcpRevisionKey)},
        {"flangeTcpRevision", text(dependencies.flangeTcpRevision)},
        {"adapterKey", int(dependencies.robotAdapterRevisionKey)},
        {"adapterRevision", text(dependencies.robotAdapterRevision)},
        {"acquisitionKey", int(dependencies.acquisitionProfileRevisionKey)},
        {"acquisitionRevision", text(dependencies.acquisitionProfileRevision)},
        {"modelKey", int(dependencies.modelRevisionKey)},
        {"modelRevision", text(dependencies.modelRevision)},
        {"pose", QJsonObject{{"targetFrame", text(pose.targetFrame)},
                             {"targetRevision", text(pose.targetFrameRevision)},
                             {"sourceFrame", text(pose.sourceFrame)},
                             {"sourceRevision", text(pose.sourceFrameRevision)},
                             {"convention", int(pose.convention)}}},
        {"cameraId", dependencies.cameraId},
        {"physical", physicalToJson(dependencies.physical)}};
}

CalibrationDependencySnapshot dependenciesFromJson(const QJsonObject& object) {
    CalibrationDependencySnapshot dependencies;
    dependencies.mode = HandEyeMode(object.value("mode").toInt());
    dependencies.cameraKey = quint32(object.value("cameraKey").toInt());
    dependencies.cameraRevision = uuid(object.value("cameraRevision").toString());
    dependencies.intrinsicRevisionKey = quint32(object.value("intrinsicKey").toInt());
    dependencies.intrinsicRevision = uuid(object.value("intrinsicRevision").toString());
    dependencies.plateRevisionKey = quint32(object.value("plateKey").toInt());
    dependencies.plateRevision = uuid(object.value("plateRevision").toString());
    dependencies.flangeTcpRevisionKey = quint32(object.value("flangeTcpKey").toInt());
    dependencies.flangeTcpRevision = uuid(object.value("flangeTcpRevision").toString());
    dependencies.robotAdapterRevisionKey = quint32(object.value("adapterKey").toInt());
    dependencies.robotAdapterRevision = uuid(object.value("adapterRevision").toString());
    dependencies.acquisitionProfileRevisionKey = quint32(object.value("acquisitionKey").toInt());
    dependencies.acquisitionProfileRevision = uuid(object.value("acquisitionRevision").toString());
    dependencies.modelRevisionKey = quint32(object.value("modelKey").toInt());
    dependencies.modelRevision = uuid(object.value("modelRevision").toString());
    const auto pose = object.value("pose").toObject();
    dependencies.pose.targetFrame = uuid(pose.value("targetFrame").toString());
    dependencies.pose.targetFrameRevision = uuid(pose.value("targetRevision").toString());
    dependencies.pose.sourceFrame = uuid(pose.value("sourceFrame").toString());
    dependencies.pose.sourceFrameRevision = uuid(pose.value("sourceRevision").toString());
    dependencies.pose.convention = quint32(pose.value("convention").toInt());
    dependencies.cameraId = object.value("cameraId").toString();
    dependencies.physical = physicalFromJson(object.value("physical").toObject());
    return dependencies;
}

QJsonObject sessionToJson(const CalibrationSessionRecord& record) {
    return QJsonObject{
        {"session", text(record.session)},
        {"begin", commandKeyToJson(record.beginCommand)},
        {"fingerprint", QString::fromLatin1(record.fingerprint.toBase64())},
        {"state", int(record.state)},
        {"recoveryRequired", record.recoveryRequired},
        {"abortRequested", record.abortRequested},
        {"generation", QString::number(record.generation)},
        {"dependencies", dependenciesToJson(record.dependencies)}};
}

CalibrationSessionRecord sessionFromJson(const QJsonObject& object) {
    CalibrationSessionRecord record;
    record.session = uuid(object.value("session").toString());
    record.beginCommand = commandKeyFromJson(object.value("begin").toObject());
    record.fingerprint = QByteArray::fromBase64(object.value("fingerprint").toString().toLatin1());
    record.state = CalibrationState(object.value("state").toInt());
    record.recoveryRequired = object.value("recoveryRequired").toBool();
    record.abortRequested = object.value("abortRequested").toBool();
    record.generation = object.value("generation").toString().toULongLong();
    record.dependencies = dependenciesFromJson(object.value("dependencies").toObject());
    return record;
}

QJsonObject commandToJson(const CalibrationCommandRecord& record) {
    return QJsonObject{
        {"key", commandKeyToJson(record.key)},
        {"fingerprint", QString::fromLatin1(record.fingerprint.toBase64())},
        {"session", text(record.session)},
        {"kind", int(record.kind)},
        {"resultState", int(record.resultState)},
        {"accepted", record.accepted}};
}

CalibrationCommandRecord commandFromJson(const QJsonObject& object) {
    CalibrationCommandRecord record;
    record.key = commandKeyFromJson(object.value("key").toObject());
    record.fingerprint = QByteArray::fromBase64(object.value("fingerprint").toString().toLatin1());
    record.session = uuid(object.value("session").toString());
    record.kind = CalibrationCommandKind(object.value("kind").toInt());
    record.resultState = CalibrationState(object.value("resultState").toInt());
    record.accepted = object.value("accepted").toBool();
    return record;
}

bool sameKey(const CalibrationCommandKey& a, const CalibrationCommandKey& b) {
    return a == b;
}
} // namespace

FileRobotCalibrationSessionStore::FileRobotCalibrationSessionStore(QString path)
    : path_(std::move(path)) {
    if (path_.isEmpty()) throw std::runtime_error("Calibration session store path required");
}

FileRobotCalibrationSessionStore::State FileRobotCalibrationSessionStore::load() const {
    State state;
    QFile file(path_);
    if (!file.exists()) return state;
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Calibration session store is unreadable");
    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) throw std::runtime_error("Calibration session store is not a JSON object");
    const auto root = document.object();
    if (root.value("schema").toInt() != 1)
        throw std::runtime_error("Unsupported calibration session store schema");
    for (const auto& value : root.value("sessions").toArray())
        state.sessions.push_back(sessionFromJson(value.toObject()));
    for (const auto& value : root.value("commands").toArray())
        state.commands.push_back(commandFromJson(value.toObject()));
    return state;
}

void FileRobotCalibrationSessionStore::save(const State& state) const {
    QJsonArray sessions;
    for (const auto& record : state.sessions) sessions.append(sessionToJson(record));
    QJsonArray commands;
    for (const auto& record : state.commands) commands.append(commandToJson(record));
    const QJsonObject root{
        {"schema", 1},
        {"sessions", sessions},
        {"commands", commands}};
    const QFileInfo info(path_);
    if (!QDir().mkpath(info.absolutePath()))
        throw std::runtime_error("Calibration session store directory could not be created");
    QSaveFile file(path_);
    if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Calibration session store is not writable");
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("Calibration session store commit failed");
}

std::optional<CalibrationCommandRecord> FileRobotCalibrationSessionStore::findCommand(
    const CalibrationCommandKey& key) const {
    QMutexLocker lock(&mutex_);
    for (const auto& record : load().commands)
        if (sameKey(record.key, key)) return record;
    return std::nullopt;
}

std::optional<CalibrationSessionRecord> FileRobotCalibrationSessionStore::findSession(
    const QUuid& id) const {
    QMutexLocker lock(&mutex_);
    for (const auto& record : load().sessions)
        if (record.session == id) return record;
    return std::nullopt;
}

std::vector<CalibrationSessionRecord> FileRobotCalibrationSessionStore::loadAll() const {
    QMutexLocker lock(&mutex_);
    return load().sessions;
}

void FileRobotCalibrationSessionStore::commit(
    const CalibrationSessionRecord& record,
    const std::optional<CalibrationCommandRecord>& command) {
    QMutexLocker lock(&mutex_);
    State state = load();
    bool replaced = false;
    for (auto& existing : state.sessions) {
        if (existing.session == record.session) {
            existing = record;
            replaced = true;
            break;
        }
    }
    if (!replaced) state.sessions.push_back(record);
    if (command) {
        bool found = false;
        for (auto& existing : state.commands) {
            if (sameKey(existing.key, command->key)) {
                existing = *command;
                found = true;
                break;
            }
        }
        if (!found) state.commands.push_back(*command);
    }
    save(state);
}
