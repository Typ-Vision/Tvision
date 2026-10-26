#pragma once
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <optional>
#include <variant>

namespace visionflow {
enum class ScalarType { Boolean, Int32, UInt32, Int64, Float64, String, Enum };
enum class ValueValidity { Valid, Missing, Stale, Skipped, Invalid, Error };
enum class ScalarUnit { Unitless, Millimetre, Metre, Radian, Degree };
enum class UnitDimension { Unitless, Length, Angle };
struct EnumValue { QString definitionId; qint32 code=0; };
using ScalarData=std::variant<bool,qint32,quint32,qint64,double,QString,EnumValue>;
// Definitions come from the caller's catalog, never from an untrusted value.
using EnumDefinitions=QMap<QString,QSet<qint32>>;

// A constant may omit producer/cycle/frame IDs. Runtime binding validates their
// identity against the active snapshot; this codec validates shape, not authority.
struct ValueEnvelope {
    ScalarType type=ScalarType::Boolean;
    std::optional<ScalarData> value;
    ValueValidity validity=ValueValidity::Missing;
    QString reason;
    ScalarUnit unit=ScalarUnit::Unitless;
    QString producerId,producerPort,cycleId,frameId,configId;
    QStringList dependencyIds;
};
class CommunicationValues {
public:
    static constexpr int maxStringBytes=4096,maxDependencies=256,maxJsonBytes=128*1024;
    static UnitDimension dimension(ScalarUnit);
    static void validate(const ValueEnvelope&,const EnumDefinitions& = {});
    static QJsonObject encode(const ValueEnvelope&,const EnumDefinitions& = {});
    static ValueEnvelope decode(const QJsonObject&,const EnumDefinitions& = {});
};
}
