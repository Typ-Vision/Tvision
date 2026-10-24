#pragma once
#include <QString>
#include <QStringList>
#include <QMap>
#include <optional>
#include <variant>

namespace visionflow {
using FeatureValue=std::variant<qint64,double,bool,QString>;
enum class FeatureType {Integer,Float,Boolean,Enumeration,String,Unsupported};
enum class FeatureAccess {Missing,Unavailable,WriteOnly,ReadOnly,ReadWrite,Unknown};
struct CameraFeature {
    QString name;
    FeatureType type=FeatureType::Unsupported;
    FeatureAccess access=FeatureAccess::Unknown;
    FeatureValue value=qint64(0);
    double minimum=0,maximum=0;
    std::optional<double> increment;
    QString unit; // empty = unreported, never inferred from the node name
    QMap<QString,quint32> entries;
    bool readable() const {return access==FeatureAccess::ReadOnly || access==FeatureAccess::ReadWrite;}
};
// Deliberately bounded SFNC feature inventory, not a vendor-node UI generator.
QStringList acquisitionFeatureNames();
bool featureEquivalent(const CameraFeature&,const FeatureValue&);
void validateFeatureWrite(const CameraFeature&,const FeatureValue&);
}
