#pragma once
#include "application/StationIdentityRegistry.hpp"
namespace visionflow {
// One atomic checksummed snapshot; corruption never falls back to an empty registry.
class JsonStationIdentityStore final : public StationIdentityStore {
public:
    using WriteBoundary=std::function<void(const QString&)>;
    explicit JsonStationIdentityStore(QString path,WriteBoundary boundary={}):path_(std::move(path)),boundary_(std::move(boundary)){}
    IdentityCatalogue load() const override;
    void create(const IdentityCatalogue&) const override;
    void compareExchange(const QUuid&,const IdentityCatalogue&) const override;
    static QByteArray encode(const IdentityCatalogue&);
    static IdentityCatalogue decode(const QByteArray&);
private:
    void write(const IdentityCatalogue&) const;
    QString path_;
    WriteBoundary boundary_;
};
}
