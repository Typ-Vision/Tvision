#include "StationActivationStore.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QSaveFile>
#include <stdexcept>

namespace visionflow {
namespace {void need(bool ok){if(!ok)throw std::runtime_error("Station store unavailable, corrupt, locked or stale");}}
StationActivationCatalogue FileStationActivationStore::load() const {
    QFile file(path_);need(file.open(QIODevice::ReadOnly) && file.size()<=16*1024*1024+32);
    auto bytes=file.readAll();need(file.error()==QFileDevice::NoError && bytes.size()>=32);auto payload=bytes.sliced(32);
    need(bytes.first(32)==QCryptographicHash::hash(payload,QCryptographicHash::Sha256));return decodeStationCatalogue(payload);
}
void FileStationActivationStore::write(const StationActivationCatalogue& c) const {
    auto payload=encodeStationCatalogue(c);auto bytes=QCryptographicHash::hash(payload,QCryptographicHash::Sha256)+payload;
    QSaveFile file(path_);file.setDirectWriteFallback(false);need(file.open(QIODevice::WriteOnly));need(file.write(bytes)==bytes.size());need(file.commit());
}
void FileStationActivationStore::create(const StationActivationCatalogue& c) const {
    validateStationCatalogue(c);need(c.history.isEmpty());need(QDir().mkpath(QFileInfo(path_).absolutePath()));
    QLockFile lock(path_+".lock");lock.setStaleLockTime(0);need(lock.tryLock(0));need(!QFileInfo::exists(path_));write(c);
}
void FileStationActivationStore::compareExchange(const QUuid& expected,const StationActivationCatalogue& c) const {
    QLockFile lock(path_+".lock");lock.setStaleLockTime(0);need(lock.tryLock(0));const auto old=load();need(old.revision==expected);validateStationTransition(old,c);write(c);
}
}
