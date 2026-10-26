#include "domain/CommunicationValues.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
ValueEnvelope valid(ScalarType type,ScalarData data) {
    ValueEnvelope e;e.type=type;e.value=std::move(data);e.validity=ValueValidity::Valid;
    e.producerId="tool";e.producerPort="out";e.cycleId="cycle";e.frameId="frame";e.configId="config";e.dependencyIds={"dep"};return e;
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const EnumDefinitions enums{{"decision",{-1,0,2}}};
        const auto i32min=std::numeric_limits<qint32>::min(),i32max=std::numeric_limits<qint32>::max();
        const auto u32max=std::numeric_limits<quint32>::max();
        const auto i64min=std::numeric_limits<qint64>::min(),i64max=std::numeric_limits<qint64>::max();
        const QVector<ValueEnvelope> values{
            valid(ScalarType::Boolean,false),valid(ScalarType::Boolean,true),
            valid(ScalarType::Int32,qint32(0)),valid(ScalarType::Int32,i32min),valid(ScalarType::Int32,i32max),
            valid(ScalarType::UInt32,quint32(0)),valid(ScalarType::UInt32,u32max),
            valid(ScalarType::Int64,i64min),valid(ScalarType::Int64,i64max),valid(ScalarType::Int64,qint64(9007199254740993LL)),
            valid(ScalarType::Float64,0.0),valid(ScalarType::Float64,-12.5),
            valid(ScalarType::String,QString::fromUtf8("Çığ 工場 😀")),valid(ScalarType::Enum,EnumValue{"decision",0})};
        for(const auto& e:values) {
            const auto json=CommunicationValues::encode(e,enums);
            const auto wire=QJsonDocument(json).toJson(QJsonDocument::Compact);
            const auto restored=CommunicationValues::decode(QJsonDocument::fromJson(wire).object(),enums);
            check(CommunicationValues::encode(restored,enums)==json,"wire roundtrip retains type/identity/value");
            check(restored.value.has_value() && restored.validity==ValueValidity::Valid,"zero/false is present and valid");
        }
        check(CommunicationValues::encode(values[9])["value"]=="9007199254740993","Int64 beyond JSON precision is decimal string");
        for(auto state:{ValueValidity::Missing,ValueValidity::Stale,ValueValidity::Skipped,ValueValidity::Invalid,ValueValidity::Error}) {
            ValueEnvelope e;e.type=ScalarType::Int64;e.validity=state;e.reason="source unavailable";
            auto json=CommunicationValues::encode(e);auto restored=CommunicationValues::decode(json);
            check(json["value"].isNull() && !restored.value && restored.validity==state && restored.reason==e.reason,"absence states remain distinct and null");
            e.value=qint64(0);reject([&]{CommunicationValues::encode(e);},"invalid state cannot hide a value");
            e.value.reset();e.reason="  ";reject([&]{CommunicationValues::encode(e);},"absence requires reason");
        }
        auto base=CommunicationValues::encode(values[2]);
        auto badValue=[&](const QJsonValue& v,const char* why){auto o=base;o["value"]=v;reject([&]{CommunicationValues::decode(o);},why);};
        for(double n:{double(i32min)-1,double(i32max)+1,0.5,1e30})badValue(n,"Int32 overflow/fraction rejected");
        badValue(true,"Boolean cannot become integer");badValue("0","string cannot become Int32");badValue(QJsonValue::Null,"valid null rejected");
        base=CommunicationValues::encode(values[5]);
        for(double n:{-1.0,double(u32max)+1,0.1})badValue(n,"UInt32 signedness/range checked");
        base=CommunicationValues::encode(values[7]);
        for(const auto& s:{"9223372036854775808","-9223372036854775809","+1","01","-0"," 1","1 ","1.0","1e2",""})badValue(s,"Int64 overflow/noncanonical rejected");
        badValue(42,"Int64 JSON number rejected");
        for(double n:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
            reject([&]{CommunicationValues::encode(valid(ScalarType::Float64,n));},"nonfinite typed Float64 rejected");
            base=CommunicationValues::encode(values[10]);badValue(QJsonValue(n),"nonfinite JSON value rejected");
        }
        const QString exact=QString::fromUtf8("😀").repeated(1024);
        check(CommunicationValues::decode(CommunicationValues::encode(valid(ScalarType::String,exact))).value.has_value(),"4096 UTF-8 bytes accepted");
        reject([&]{CommunicationValues::encode(valid(ScalarType::String,exact+"x"));},"UTF-8 byte bound instead of UTF-16 count");
        for(const QString s:{QString(QChar(0xd800)),QString(QChar(0xdc00))})reject([&]{CommunicationValues::encode(valid(ScalarType::String,s));},"unpaired surrogate rejected");
        auto textJson=CommunicationValues::encode(valid(ScalarType::String,QString("x")));textJson["value"]=exact+"x";
        reject([&]{CommunicationValues::decode(textJson);},"decode byte limit");
        auto enumValue=values.last();reject([&]{CommunicationValues::encode(enumValue);},"enum definition required");
        enumValue.value=EnumValue{"decision",1};reject([&]{CommunicationValues::encode(enumValue,enums);},"unknown enum code");
        enumValue.value=EnumValue{"other",0};reject([&]{CommunicationValues::encode(enumValue,enums);},"unknown enum ID");
        auto enumJson=CommunicationValues::encode(values.last(),enums);enumJson["value"]=QJsonObject{{"definitionId","decision"},{"code",1}};
        reject([&]{CommunicationValues::decode(enumJson,enums);},"unknown enum code on decode");
        base=CommunicationValues::encode(values[0]);
        for(const QString key:{"type","validity","unit"}) {auto o=base;o[key]="unknown";reject([&]{CommunicationValues::decode(o);},"unknown tag rejected");}
        auto o=base;o["unit"]="px";reject([&]{CommunicationValues::decode(o);},"pixel cannot masquerade as length");
        o=base;o["extra"]=1;reject([&]{CommunicationValues::decode(o);},"extra key rejected");
        for(auto it=base.begin();it!=base.end();++it){o=base;o.remove(it.key());reject([&]{CommunicationValues::decode(o);},"missing field rejected");}
        o=base;o["schemaVersion"]=2;reject([&]{CommunicationValues::decode(o);},"future schema rejected");
        auto e=values[0];e.value=qint32(0);reject([&]{CommunicationValues::encode(e);},"type narrowing requires converter");
        e=values[0];e.unit=ScalarUnit::Millimetre;reject([&]{CommunicationValues::encode(e);},"Boolean length unit rejected");
        for(auto unit:{ScalarUnit::Unitless,ScalarUnit::Millimetre,ScalarUnit::Metre,ScalarUnit::Radian,ScalarUnit::Degree}) {
            e=values[11];e.unit=unit;check(CommunicationValues::decode(CommunicationValues::encode(e)).unit==unit,"declared constant unit retained");
        }
        check(CommunicationValues::dimension(ScalarUnit::Millimetre)==CommunicationValues::dimension(ScalarUnit::Metre) && CommunicationValues::dimension(ScalarUnit::Radian)!=CommunicationValues::dimension(ScalarUnit::Metre),"unit dimensions distinct");
        e=values[0];e.dependencyIds={"same","same"};reject([&]{CommunicationValues::encode(e);},"duplicate dependency");
        e.dependencyIds={""};reject([&]{CommunicationValues::encode(e);},"empty dependency");
        e.dependencyIds.clear();for(int i=0;i<=CommunicationValues::maxDependencies;++i)e.dependencyIds.append(QString::number(i));
        reject([&]{CommunicationValues::encode(e);},"bounded dependencies");
        e=values[0];e.producerPort.clear();reject([&]{CommunicationValues::encode(e);},"partial producer identity");
        e=values[0];e.reason="error";reject([&]{CommunicationValues::encode(e);},"valid cannot retain failure reason");
        e=values[0];e.type=static_cast<ScalarType>(99);reject([&]{CommunicationValues::encode(e);},"unknown domain tag");
        e=values[0];e.dependencyIds.clear();for(int i=0;i<40;++i)e.dependencyIds.append(QString(4090,'a')+QString::number(i));
        reject([&]{CommunicationValues::encode(e);},"aggregate byte budget");
        QTextStream(stdout)<<checks<<" checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}
}
