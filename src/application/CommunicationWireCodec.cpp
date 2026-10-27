#include "CommunicationWireCodec.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>

namespace visionflow {
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
void keys(const QJsonObject& o,const QStringList& names){
    need(o.size()==names.size(),"V3 fields missing/extra");
    for(auto i=o.begin();i!=o.end();++i)need(names.contains(i.key()),"Unknown V3 field");
}
void unicode(const QString& s){
    for(qsizetype i=0;i<s.size();++i){
        if(s[i].isHighSurrogate()){need(i+1<s.size() && s[i+1].isLowSurrogate(),"Unpaired Unicode surrogate");++i;}
        else need(!s[i].isLowSurrogate(),"Unpaired Unicode surrogate");
    }
}
void tree(const QJsonValue& v,int depth=0){
    need(depth<=32,"V3 nesting limit exceeded");
    if(v.isString())unicode(v.toString());
    else if(v.isDouble())need(std::isfinite(v.toDouble()),"Nonfinite V3 number");
    else if(v.isObject()){const auto o=v.toObject();for(auto i=o.begin();i!=o.end();++i){unicode(i.key());tree(i.value(),depth+1);}}
    else if(v.isArray())for(const auto& child:v.toArray())tree(child,depth+1);
}
QString text(const QJsonObject& o,const char* key,bool empty=false,int max=128){
    need(o[key].isString(),"V3 string required");const auto s=o[key].toString();unicode(s);
    need((empty || !s.isEmpty()) && s.toUtf8().size()<=max,"Invalid V3 string length");return s;
}
void identity(const QJsonObject& o,const char* key,bool empty=false){
    const auto s=text(o,key,empty);static const QRegularExpression re("^[A-Za-z0-9_.:-]+$");
    need((empty && s.isEmpty()) || re.match(s).hasMatch(),"Invalid V3 identity");
}
void decimal(const QJsonObject& o,const char* key){
    auto s=text(o,key);bool ok=false;auto n=s.toULongLong(&ok);
    need(ok && QString::number(n)==s,"V3 UInt64 requires canonical decimal string");
}
void hash(const QJsonObject& o){static const QRegularExpression re("^[0-9a-f]{64}$");need(re.match(text(o,"payloadHash")).hasMatch(),"Invalid V3 SHA-256 hash");}
const QStringList fieldKeys{"name","type","unit","validity","value","reason","producerId","producerPort","cycleId","frameId","configId","dependencyIds"};
void fields(const QJsonObject& o){
    need(o["fields"].isArray(),"V3 fields array required");const auto a=o["fields"].toArray();need(a.size()<=64,"V3 field limit exceeded");QSet<QString> names;
    for(const auto& v:a){need(v.isObject(),"V3 field object required");const auto f=v.toObject();CommunicationWireCodec::decodeField(f);const auto n=f["name"].toString();need(!names.contains(n),"Duplicate V3 field name");names.insert(n);}
}
QByteArray compact(const QJsonObject& o){return QJsonDocument(o).toJson(QJsonDocument::Compact);}
QByteArray payload(QJsonObject o){for(const auto& k:{"v","type","sessionId","payloadHash"})o.remove(k);return compact(o);}
// Qt's JSON parser accepts duplicate members by retaining the last. Reject those
// after syntax parsing, including escaped-equivalent names, before dispatch/hash.
class UniqueKeys {
    QByteArrayView b;qsizetype p=0;
    void space(){while(p<b.size() && (b[p]==' ' || b[p]=='\t' || b[p]=='\r' || b[p]=='\n'))++p;}
    QByteArray string(){auto start=p++;while(p<b.size()){auto c=b[p++];if(c=='\\')++p;else if(c=='"')break;}return QByteArray(b.data()+start,p-start);}
    void value(int depth){
        need(depth<=32,"V3 nesting limit exceeded");space();auto c=b[p];
        if(c=='"'){string();return;}
        if(c=='{' || c=='['){++p;space();QSet<QString> seen;auto end=c=='{'?'}':']';
            if(b[p]==end){++p;return;}
            for(;;){space();if(c=='{'){auto key=QJsonDocument::fromJson('['+string()+']').array()[0].toString();need(!seen.contains(key),"Duplicate V3 JSON key");seen.insert(key);space();++p;}
                value(depth+1);space();if(b[p++]==end)break;
            }return;
        }
        while(p<b.size() && b[p]!=',' && b[p]!=']' && b[p]!='}' && b[p]!=' ' && b[p]!='\r' && b[p]!='\n' && b[p]!='\t')++p;
    }
public:
    explicit UniqueKeys(QByteArrayView bytes):b(bytes){}
    void check(){space();need(p<b.size() && b[p]=='{',"V3 JSON object required without BOM");value(0);}
};
}
QJsonObject CommunicationWireCodec::encodeField(const QString& name,const AssignmentValue& v){
    QJsonObject f;
    if(auto scalar=std::get_if<ValueEnvelope>(&v))f=encodeAssignmentValue(*scalar)["envelope"].toObject();
    else {const auto s=encodeAssignmentValue(v);f=s["metadata"].toObject();f["type"]="Pose";f["unit"]="mm";f["value"]=s["transform"];}
    f.remove("schemaVersion");f["name"]=name;decodeField(f);return f;
}
AssignmentValue CommunicationWireCodec::decodeField(const QJsonObject& f){
    keys(f,fieldKeys);tree(f);static const QRegularExpression name("^[A-Za-z][A-Za-z0-9_.-]{0,63}$");
    need(name.match(text(f,"name")).hasMatch(),"Invalid V3 logical name");
    auto e=f;e.remove("name");e["schemaVersion"]=1;
    if(f["type"]!="Pose")return decodeAssignmentValue({{"kind","scalar"},{"envelope",e}});
    need(f["unit"]=="mm","V3 Pose requires mm");const auto transform=e.take("value");
    e["type"]="Boolean";e["unit"]="unitless";e["value"]=transform.isNull()?QJsonValue(QJsonValue::Null):QJsonValue(true);
    return decodeAssignmentValue({{"kind","spatial"},{"metadata",e},{"transform",transform}});
}
void CommunicationWireCodec::validate(const QJsonObject& o){
    tree(o);need(compact(o).size()<=maxLineBytes,"V3 line exceeds 64 KiB");
    need(o["v"].isDouble() && o["v"].toDouble()==3,"Unsupported V3 version");
    identity(o,"sessionId");auto t=text(o,"type");QStringList k{"v","type","sessionId"};
    if(t=="STATUS" && o.size()==3){keys(o,k);return;}
    if(t=="HELLO" || t=="STATUS"){
        k.append({"state","ready","busy","recipeId","configId"});keys(o,k);identity(o,"state");identity(o,"recipeId",true);identity(o,"configId",true);
        need(o["ready"].isBool() && o["busy"].isBool() && !(o["ready"].toBool() && o["busy"].toBool()),"Invalid V3 readiness");return;
    }
    if(t=="TRIGGER"){
        k.append({"requestId","sequence","recipeId","configId","inputSnapshotId","inputRevision"});keys(o,k);
        for(auto key:{"requestId","recipeId","configId","inputSnapshotId"})identity(o,key);decimal(o,"sequence");decimal(o,"inputRevision");return;
    }
    if(t=="INPUT"){
        k.append({"snapshotId","revision","recipeId","configId","fields","sourceTime","clockId"});keys(o,k);
        for(auto key:{"snapshotId","recipeId","configId","clockId"})identity(o,key);decimal(o,"revision");text(o,"sourceTime",false,256);fields(o);return;
    }
    if(t=="ACK" || t=="ACKED"){
        k.append({"requestId","resultId","payloadHash"});keys(o,k);identity(o,"requestId");identity(o,"resultId");hash(o);return;
    }
    if(t=="ACCEPTED" || t=="BUSY"){
        k.append({"requestId","sequence","cycleId"});keys(o,k);identity(o,"requestId");decimal(o,"sequence");identity(o,"cycleId");return;
    }
    if(t=="INVALID" || t=="ERROR"){
        k.append({"requestId",t=="INVALID"?"reason":"code"});keys(o,k);identity(o,"requestId",true);identity(o,t=="INVALID"?"reason":"code");return;
    }
    need(t=="RESULT","Unknown V3 message type");
    k.append({"requestId","sequence","recipeId","configId","inputSnapshotId","inputRevision","resultId","cycleId","execution","decision","resultValid","fields","error","payloadHash"});keys(o,k);
    for(auto key:{"requestId","recipeId","configId","inputSnapshotId","resultId","cycleId"})identity(o,key);
    decimal(o,"sequence");decimal(o,"inputRevision");hash(o);fields(o);text(o,"error",true,4096);
    need(o["execution"]=="DONE" || o["execution"]=="ERROR" || o["execution"]=="SKIPPED","Invalid V3 execution");
    need(o["decision"]=="OK" || o["decision"]=="NOK" || o["decision"]=="NOT_EVALUATED","Invalid V3 decision");
    need(o["resultValid"].isBool(),"V3 resultValid Boolean required");
    need(o["resultValid"].toBool()==(o["decision"]!="NOT_EVALUATED"),"V3 decision validity mismatch");
    need(o["execution"]=="DONE" || !o["resultValid"].toBool(),"Failed execution cannot carry part decision");
    need(o["execution"]!="DONE" || o["error"]=="","Successful execution cannot carry terminal error");
    need(o["payloadHash"].toString()==payloadHash(payload(o)),"V3 payload hash mismatch");
}
QJsonObject CommunicationWireCodec::decode(QByteArrayView line){
    need(!line.empty() && line.size()<=maxLineBytes,"Invalid V3 line length");
    const auto bytes=QByteArray(line.data(),line.size());need(!bytes.contains('\n') && !bytes.contains('\r'),"V3 decoder expects one LF-stripped line");
    need(QString::fromUtf8(bytes).toUtf8()==bytes,"Invalid V3 UTF-8");QJsonParseError error;
    const auto doc=QJsonDocument::fromJson(bytes,&error);need(error.error==QJsonParseError::NoError && doc.isObject(),"Invalid V3 JSON object");
    UniqueKeys(line).check();validate(doc.object());return doc.object();
}
QByteArray CommunicationWireCodec::encode(const QJsonObject& o){validate(o);return compact(o)+'\n';}
QByteArray CommunicationWireCodec::resultPayload(const QJsonObject& o){validate(o);need(o["type"]=="RESULT","RESULT payload required");return payload(o);}
QString CommunicationWireCodec::payloadHash(QByteArrayView bytes){return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());}
QJsonObject CommunicationWireCodec::sealResult(QJsonObject o){o["payloadHash"]=payloadHash(payload(o));validate(o);need(o["type"]=="RESULT","RESULT required");return o;}

void CommunicationLineFramer::fail(const char* m){failed_=true;partial_.clear();throw std::runtime_error(m);}
void CommunicationLineFramer::tick(qint64 now){
    if(failed_)throw std::runtime_error("V3 framer failed; close/reset peer");
    if(now<0 || now<last_)fail("V3 framer requires monotonic time");last_=now;
    if(started_>=0 && now-started_>=partialDeadlineMs)fail("V3 partial frame deadline exceeded");
}
QVector<QByteArray> CommunicationLineFramer::feed(QByteArrayView bytes,qint64 now){
    tick(now);if(bytes.size()>CommunicationWireCodec::maxLineBytes+1)fail("V3 read chunk limit exceeded");QVector<QByteArray> lines;
    for(auto c:bytes){
        if(c=='\n'){
            if(partial_.isEmpty())fail("Empty V3 frame");lines.append(std::move(partial_));partial_.clear();started_=-1;
        }else{
            if(partial_.size()>=CommunicationWireCodec::maxLineBytes)fail("V3 line exceeds 64 KiB");
            if(partial_.isEmpty())started_=now;partial_.append(c);
        }
    }return lines;
}
void CommunicationLineFramer::reset(){partial_.clear();started_=-1;last_=-1;failed_=false;}
}
