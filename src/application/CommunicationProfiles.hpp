#pragma once
#include <QJsonObject>
#include <QMap>
#include <QString>
namespace visionflow {
// Machine-local data only. credentialRef is an opaque lookup key, never a secret.
struct EndpointProfile {
    QString id;int revision=1,contractRevision=1;
    QString protocol="tcp",role="client",address="127.0.0.1";int port=5000;
    bool enabled=false;int timeoutMs=1000,retries=0;
    QString credentialRef;
    QMap<QString,QString> fieldMapping;
};
class CommunicationProfileStore {
public:
    virtual ~CommunicationProfileStore()=default;
    virtual QMap<QString,EndpointProfile> load() const=0;
    virtual void save(const QMap<QString,EndpointProfile>&) const=0;
};
class CommunicationProfiles final:public CommunicationProfileStore {
public:
    explicit CommunicationProfiles(QString path):path_(std::move(path)){}
    QMap<QString,EndpointProfile> load() const override;
    void save(const QMap<QString,EndpointProfile>&) const override;
private:QString path_;
};
void validateEndpointProfile(const EndpointProfile&);
// Opaque identity only: no addresses, physical mappings or credential references.
QByteArray endpointDependencyId(const EndpointProfile&);
}
