#pragma once
#include <QJsonObject>
#include <QString>
#include <QVector>
namespace visionflow {
// Archive data is evidence, never a restored authorization token.
class ProductionStore {
public:
    explicit ProductionStore(QString directory);
    void append(const QString& kind,const QString& id,const QJsonObject& payload) const;
    QVector<QJsonObject> recent(const QString& kind,int limit=128) const;
    QJsonObject get(const QString& kind,const QString& id) const;
    QVector<QJsonObject> all(const QString& kind) const;
private:
    QString directory_;
};
}
