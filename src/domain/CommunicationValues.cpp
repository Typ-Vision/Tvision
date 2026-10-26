#include "CommunicationValues.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
const QStringList types{"Boolean","Int32","UInt32","Int64","Float64","String","Enum"};
const QStringList validities{"valid","missing","stale","skipped","invalid","error"};
const QStringList units{"unitless","mm","m","rad","degree"};
QString tag(const QStringList& names,int index) {
    require(index>=0 && index<names.size(),"Unknown scalar discriminator");return names[index];
}
int discriminator(const QJsonValue& v,const QStringList& names) {
    require(v.isString(),"Expected scalar discriminator");const int i=names.indexOf(v.toString());
    require(i>=0,"Unknown scalar discriminator");return i;
}
void text(const QString& s,int limit=CommunicationValues::maxStringBytes) {
    for(qsizetype i=0;i<s.size();++i) {
        if(s[i].isHighSurrogate()){require(i+1<s.size() && s[i+1].isLowSurrogate(),"Unpaired Unicode surrogate");++i;}
        else require(!s[i].isLowSurrogate(),"Unpaired Unicode surrogate");
    }
    require(s.toUtf8().size()<=limit,"Scalar UTF-8 byte limit exceeded");
}
QString string(const QJsonValue& v) { require(v.isString(),"Expected scalar string");auto s=v.toString();text(s);return s; }
void keys(const QJsonObject& o,std::initializer_list<QString> fields) {
    const QSet<QString> allowed(fields.begin(),fields.end());require(o.size()==allowed.size(),"Scalar fields missing/extra");
    for(auto it=o.begin();it!=o.end();++it)require(allowed.contains(it.key()),"Unknown scalar field");
}
double number(const QJsonValue& v) { require(v.isDouble() && std::isfinite(v.toDouble()),"Expected finite scalar number");return v.toDouble(); }
template<class T> T integer(const QJsonValue& v) {
    const double n=number(v);
    require(std::trunc(n)==n && n>=double(std::numeric_limits<T>::min()) && n<=double(std::numeric_limits<T>::max()),"Scalar integer out of range");
    return static_cast<T>(n);
}
qint64 int64(const QJsonValue& v) {
    const auto s=string(v);bool ok=false;const auto n=s.toLongLong(&ok,10);
    require(ok && QString::number(n)==s,"Int64 requires canonical decimal string");return n;
}
QJsonValue data(const ScalarData& value) {
    return std::visit([](const auto& v)->QJsonValue {
        using T=std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T,qint64>)return QString::number(v);
        else if constexpr(std::is_same_v<T,quint32>)return QJsonValue(double(v));
        else if constexpr(std::is_same_v<T,EnumValue>)return QJsonObject{{"definitionId",v.definitionId},{"code",v.code}};
        else return QJsonValue(v);
    },value);
}
}
UnitDimension CommunicationValues::dimension(ScalarUnit unit) {
    switch(unit) {
    case ScalarUnit::Unitless:return UnitDimension::Unitless;
    case ScalarUnit::Millimetre:case ScalarUnit::Metre:return UnitDimension::Length;
    case ScalarUnit::Radian:case ScalarUnit::Degree:return UnitDimension::Angle;
    }
    throw std::runtime_error("Unknown scalar unit");
}
void CommunicationValues::validate(const ValueEnvelope& e,const EnumDefinitions& definitions) {
    tag(types,int(e.type));tag(validities,int(e.validity));tag(units,int(e.unit));
    const bool numeric=e.type==ScalarType::Int32 || e.type==ScalarType::UInt32 || e.type==ScalarType::Int64 || e.type==ScalarType::Float64;
    require(numeric || e.unit==ScalarUnit::Unitless,"Non-numeric scalar must be unitless");
    text(e.reason);text(e.producerId);text(e.producerPort);text(e.cycleId);text(e.frameId);text(e.configId);
    require(e.producerId.isEmpty()==e.producerPort.isEmpty(),"Producer ID and port must be paired");
    require(e.dependencyIds.size()<=maxDependencies,"Too many scalar dependencies");QSet<QString> seen;
    for(const auto& id:e.dependencyIds){text(id);require(!id.isEmpty() && !seen.contains(id),"Empty/duplicate scalar dependency");seen.insert(id);}
    if(e.validity!=ValueValidity::Valid) {
        require(!e.value && !e.reason.trimmed().isEmpty(),"Absent scalar requires null and reason");return;
    }
    require(e.value.has_value() && e.reason.isEmpty(),"Valid scalar requires value and no failure reason");
    require(e.value->index()==size_t(e.type),"Scalar type/value mismatch; explicit converter required");
    if(const auto p=std::get_if<double>(&*e.value))require(std::isfinite(*p),"Nonfinite scalar");
    if(const auto p=std::get_if<QString>(&*e.value))text(*p);
    if(const auto p=std::get_if<EnumValue>(&*e.value)) {
        text(p->definitionId);require(!p->definitionId.isEmpty() && definitions.contains(p->definitionId) && definitions[p->definitionId].contains(p->code),"Unknown enum definition/code");
    }
}
QJsonObject CommunicationValues::encode(const ValueEnvelope& e,const EnumDefinitions& definitions) {
    validate(e,definitions);QJsonArray deps;for(const auto& id:e.dependencyIds)deps.append(id);
    QJsonObject o{{"schemaVersion",1},{"type",tag(types,int(e.type))},{"value",e.value?data(*e.value):QJsonValue(QJsonValue::Null)},
        {"validity",tag(validities,int(e.validity))},{"reason",e.reason},{"unit",tag(units,int(e.unit))},
        {"producerId",e.producerId},{"producerPort",e.producerPort},{"cycleId",e.cycleId},{"frameId",e.frameId},
        {"configId",e.configId},{"dependencyIds",deps}};
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=maxJsonBytes,"Scalar envelope too big");return o;
}
ValueEnvelope CommunicationValues::decode(const QJsonObject& o,const EnumDefinitions& definitions) {
    require(QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=maxJsonBytes,"Scalar envelope too big");
    keys(o,{"schemaVersion","type","value","validity","reason","unit","producerId","producerPort","cycleId","frameId","configId","dependencyIds"});
    require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1,"Unsupported scalar schema");
    ValueEnvelope e;e.type=ScalarType(discriminator(o["type"],types));e.validity=ValueValidity(discriminator(o["validity"],validities));e.unit=ScalarUnit(discriminator(o["unit"],units));
    e.reason=string(o["reason"]);e.producerId=string(o["producerId"]);e.producerPort=string(o["producerPort"]);
    e.cycleId=string(o["cycleId"]);e.frameId=string(o["frameId"]);e.configId=string(o["configId"]);
    require(o["dependencyIds"].isArray() && o["dependencyIds"].toArray().size()<=maxDependencies,"Invalid scalar dependencies");
    for(const auto& id:o["dependencyIds"].toArray())e.dependencyIds.append(string(id));
    const auto v=o["value"];
    if(!v.isNull())switch(e.type) {
    case ScalarType::Boolean:require(v.isBool(),"Expected Boolean");e.value=v.toBool();break;
    case ScalarType::Int32:e.value=integer<qint32>(v);break;
    case ScalarType::UInt32:e.value=integer<quint32>(v);break;
    case ScalarType::Int64:e.value=int64(v);break;
    case ScalarType::Float64:e.value=number(v);break;
    case ScalarType::String:e.value=string(v);break;
    case ScalarType::Enum: {
        require(v.isObject(),"Expected Enum object");auto d=v.toObject();keys(d,{"definitionId","code"});
        e.value=EnumValue{string(d["definitionId"]),integer<qint32>(d["code"])};break;
    }
    }
    validate(e,definitions);return e;
}
}
