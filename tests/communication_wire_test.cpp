#include "application/CommunicationWireCodec.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* m){++checks;if(!b)throw std::runtime_error(m);}
template<class F> void reject(F f,const char* m){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,m);}
QJsonObject fixture(const QString& file){QFile f(QString(VISIONFLOW_WIRE_FIXTURES)+"/"+file);check(f.open(QIODevice::ReadOnly),"fixture opens");return QJsonDocument::fromJson(f.readAll()).object();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto fixtureSet=fixture("vf.socket-3.json");check(fixtureSet["fixtureVersion"]==1,"versioned fixture");
    QJsonObject result,input,trigger;
    for(const auto& value:fixtureSet["messages"].toArray()){
        const auto o=value.toObject();const auto encoded=CommunicationWireCodec::encode(o);
        check(encoded.endsWith('\n') && CommunicationWireCodec::decode(encoded.chopped(1))==o,"all command/response fixtures roundtrip");
        if(o["type"]=="RESULT")result=o;if(o["type"]=="INPUT")input=o;if(o["type"]=="TRIGGER")trigger=o;
        for(auto i=o.begin();i!=o.end();++i){auto bad=o;bad.remove(i.key());reject([&]{CommunicationWireCodec::encode(bad);},"required top level fields");}
        auto bad=o;bad["unknown"]=true;reject([&]{CommunicationWireCodec::encode(bad);},"unknown top level fields");
    }
    // Session is transport metadata. Persist once, then retry these exact bytes.
    auto r=result;r["sessionId"]="session-next";
    check(CommunicationWireCodec::resultPayload(r)==CommunicationWireCodec::resultPayload(result),"session-independent persisted hash");
    auto compact=CommunicationWireCodec::encode(result).chopped(1);
    // Different sender field order and whitespace must produce identical payload/hash.
    QByteArray reordered="{";auto ks=result.keys();for(auto i=ks.crbegin();i!=ks.crend();++i){if(reordered.size()>1)reordered+=", ";reordered+=QJsonDocument(QJsonArray{*i}).toJson(QJsonDocument::Compact).mid(1).chopped(1)+":"+QJsonDocument(QJsonArray{result[*i]}).toJson(QJsonDocument::Compact).mid(1).chopped(1);}reordered+='}';
    check(CommunicationWireCodec::resultPayload(CommunicationWireCodec::decode(reordered))==CommunicationWireCodec::resultPayload(result),"field order canonical");
    r=result;r["error"]="tamper";reject([&]{CommunicationWireCodec::encode(r);},"hash/terminal error tamper rejected");
    r=result;r["payloadHash"]=QString(64,'0');reject([&]{CommunicationWireCodec::encode(r);},"wrong hash rejected");
    r=result;r["decision"]="NOT_EVALUATED";r["resultValid"]=true;
    check(CommunicationWireCodec::sealResult(r)["resultValid"].toBool(),"valid data group does not require a part decision");
    r=result;r["resultValid"]=false;
    check(!CommunicationWireCodec::sealResult(r)["resultValid"].toBool(),"group validity is independent of evaluated decision");
    r=result;r["execution"]="ERROR";reject([&]{CommunicationWireCodec::sealResult(r);},"ERROR cannot manufacture OK");
    r["decision"]="NOT_EVALUATED";r["resultValid"]=false;r["error"]="source missing";
    check(CommunicationWireCodec::decode(CommunicationWireCodec::encode(CommunicationWireCodec::sealResult(r)).chopped(1))["decision"]=="NOT_EVALUATED","terminal error without fabricated decision");
    for(const auto& v:input["fields"].toArray()){
        auto f=v.toObject();auto domain=CommunicationWireCodec::decodeField(f);
        check(CommunicationWireCodec::encodeField(f["name"].toString(),domain)==f,"scalar/null/Pose domain roundtrip");
        for(auto i=f.begin();i!=f.end();++i){auto bad=f;bad.remove(i.key());reject([&]{CommunicationWireCodec::decodeField(bad);},"required field metadata");}
        f["unknown"]=0;reject([&]{CommunicationWireCodec::decodeField(f);},"unknown field metadata");
    }
    auto scalar=input["fields"].toArray()[0].toObject();
    for(const QJsonValue& value:QJsonArray{42,"9223372036854775808","-9223372036854775809","01","+1","-0","1e2"}){auto f=scalar;f["value"]=value;reject([&]{CommunicationWireCodec::decodeField(f);},"strict Int64");}
    for(const auto& s:{"18446744073709551616","-1","+1","01"," 1","1.0"}){auto o=trigger;o["sequence"]=s;reject([&]{CommunicationWireCodec::encode(o);},"strict UInt64");}
    auto o=trigger;o["sequence"]=1;reject([&]{CommunicationWireCodec::encode(o);},"numeric sequence rejected");
    o=trigger;o["sequence"]="18446744073709551615";check(!CommunicationWireCodec::encode(o).isEmpty(),"UInt64 max lossless");
    o=trigger;o["v"]=4;reject([&]{CommunicationWireCodec::encode(o);},"future version");
    o=trigger;o["type"]="UNKNOWN";reject([&]{CommunicationWireCodec::encode(o);},"future type");
    o=trigger;o["sessionId"]="";reject([&]{CommunicationWireCodec::encode(o);},"missing identity");
    auto f=scalar;f["type"]="Float64";f["value"]=std::numeric_limits<double>::infinity();reject([&]{CommunicationWireCodec::decodeField(f);},"nonfinite Float64 rejected");
    f=scalar;f["value"]=QJsonValue::Null;reject([&]{CommunicationWireCodec::decodeField(f);},"valid field cannot hide null");
    o=input;auto a=o["fields"].toArray();a.append(a[0]);o["fields"]=a;reject([&]{CommunicationWireCodec::encode(o);},"duplicate logical name");
    auto pose=input["fields"].toArray()[2].toObject();pose["unit"]="m";reject([&]{CommunicationWireCodec::decodeField(pose);},"Pose mm only");
    pose=input["fields"].toArray()[2].toObject();auto transform=pose["value"].toObject();transform["rotation"]=QJsonObject{{"format","quaternion-wxyz"},{"value",QJsonArray{0,0,0,0}}};pose["value"]=transform;reject([&]{CommunicationWireCodec::decodeField(pose);},"zero quaternion");
    QByteArray status="{\"v\":3,\"type\":\"STATUS\",\"sessionId\":\"session-1\"}";
    check(CommunicationWireCodec::decode(status)["type"]=="STATUS","STATUS command");
    for(auto bad:{QByteArray("[]"),QByteArray("{}"),QByteArray("{\"v\":NaN}"),status+'\n',QByteArray("{\"v\":3,\"v\":3,\"type\":\"STATUS\",\"sessionId\":\"session-1\"}"),QByteArray("{\"v\":3,\"type\":\"STATUS\",\"sessionId\":\"x\",\"session\\u0049d\":\"x\"}")})reject([&]{CommunicationWireCodec::decode(bad);},"invalid JSON/duplicate keys");
    auto invalid=status;invalid.replace("session-1",QByteArray("\xc0\xaf",2));reject([&]{CommunicationWireCodec::decode(invalid);},"invalid UTF-8");
    reject([&]{CommunicationWireCodec::decode(QByteArray("\xef\xbb\xbf",3)+status);},"BOM cannot bypass duplicate-key scan");
    invalid=status;invalid.replace("session-1","\\ud800");reject([&]{CommunicationWireCodec::decode(invalid);},"invalid Unicode surrogate");
    const auto boundary=status+QByteArray(CommunicationWireCodec::maxLineBytes-status.size(),' ');
    check(CommunicationWireCodec::decode(boundary)["v"]==3,"exact 64 KiB JSON accepted");reject([&]{CommunicationWireCodec::decode(boundary+' ');},"64 KiB plus one rejected");
    CommunicationLineFramer framer;
    check(framer.feed(status.first(7),100).isEmpty(),"partial frame held");
    check(framer.feed(status.mid(7)+'\n'+status+'\n',2099).size()==2,"fragmented/coalesced frames");
    check(framer.bufferedBytes()==0,"no partial after LF");
    check(framer.feed(boundary,3000).isEmpty() && framer.bufferedBytes()==65536,"bounded exact max partial");
    check(framer.feed("\n",4999).first()==boundary,"LF can terminate exact max");
    framer.feed("{",5000);framer.feed(" ",6999);reject([&]{framer.tick(7000);},"partial deadline from first byte, trickle cannot refresh");
    check(framer.bufferedBytes()==0,"failed buffer cleared");reject([&]{framer.feed(status+'\n',7001);},"failed stream cannot resume");
    framer.reset();framer.feed("{",0);reject([&]{framer.feed("}\n",2000);},"late LF cannot bypass timeout");
    framer.reset();framer.feed(boundary,0);reject([&]{framer.feed("x",1);},"partial overflow");
    framer.reset();reject([&]{framer.feed(QByteArray(65538,'x'),0);},"bounded socket read chunk");
    framer.reset();reject([&]{framer.feed("\n",0);},"empty frame");
    framer.reset();framer.tick(100);reject([&]{framer.tick(99);},"monotonic clock required");
    const auto legacy=fixture("vf.socket-2.json");const auto bytes=QJsonDocument(legacy["status"].toObject()).toJson(QJsonDocument::Compact)+'\n';
    check(bytes==legacy["statusBytes"].toString().toUtf8(),"v2 known bytes retained");
    reject([&]{CommunicationWireCodec::decode(bytes.chopped(1));},"no silent v2 downgrade");
    QTextStream(stdout)<<checks<<" checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
