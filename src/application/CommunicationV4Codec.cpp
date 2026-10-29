#include "application/CommunicationV4Codec.hpp"
#include <QCryptographicHash>
#include <QStringDecoder>
#include <QtEndian>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void range(QByteArrayView b,qsizetype o,qsizetype n){need(o>=0 && n>=0 && o<=b.size() && n<=b.size()-o,"V4 field bounds");}
bool zero(QByteArrayView b,qsizetype o,qsizetype n){range(b,o,n);for(qsizetype i=o;i<o+n;++i)if(b[i]!=0)return false;return true;}
void reserved(QByteArrayView b,qsizetype o,qsizetype n){need(zero(b,o,n),"V4 reserved/padding bytes");}
quint32 u(QByteArrayView b,qsizetype o){return C::uint32(b,o);}
void eq(QByteArrayView b,qsizetype o,quint32 v){need(u(b,o)==v,"V4 fixed field value");}
void atMost(QByteArrayView b,qsizetype o,quint32 v){need(u(b,o)<=v,"V4 enum/bitmap range");}
void between(QByteArrayView b,qsizetype o,quint32 lo,quint32 hi){need(u(b,o)>=lo && u(b,o)<=hi,"V4 enum range");}
bool requestKind(quint32 k){switch(k){case 1:case 2:case 3:case 4:case 5:case 6:case 10:case 11:case 12:case 20:case 21:case 22:case 23:case 24:case 25:case 30:return true;default:return false;}}
bool responseKind(quint32 k){switch(k){case 101:case 102:case 103:case 110:case 111:case 112:case 113:case 190:case 191:return true;default:return false;}}
void text(QByteArrayView b,qsizetype o,quint32 cap){
    const auto n=u(b,o);need(n<=cap,"V4 UTF-8 byte limit");range(b,o+4,cap);
    QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
    const QString decoded=decoder(b.sliced(o+4,n));Q_UNUSED(decoded);
    need(!decoder.hasError(),"V4 invalid UTF-8");reserved(b,o+4+n,cap-n);
}
void pose(QByteArrayView b,qsizetype o,bool contract){
    reserved(b,o+124,4);for(int i=56;i<=104;i+=16)need(!zero(b,o+i,16),"V4 pose frame identity");
    eq(b,o+120,contract?2:1);
    double values[7];for(int i=0;i<7;++i){values[i]=C::float64(b,o+i*8);need(std::isfinite(values[i]),"V4 nonfinite pose");}
    if(contract){reserved(b,o,56);return;}
    // Keep the frozen evaluation order; do not normalize incoming sealed bytes.
    const double a=values[3]*values[3]+values[4]*values[4];
    const double c=a+values[5]*values[5];const double n=std::sqrt(c+values[6]*values[6]);
    need(std::isfinite(n) && std::abs(n-1)<=1e-6,"V4 quaternion norm");
}
void metadata(QByteArrayView b){
    const auto category=u(b,144);between(b,144,1,13);eq(b,268,256);eq(b,272,1);eq(b,276,256);
    // Per-category final assigned byte; all gaps/tails in schemaVersion1 are zero.
    static constexpr int ends[]={0,124,84,104,92,96,88,84,92,88,84,84,84,100};
    const auto o=272;reserved(b,o+ends[category],640-ends[category]);
    switch(category){
    case 1:atMost(b,o+72,6);atMost(b,o+84,32);atMost(b,o+120,1);break;
    case 2:between(b,o+72,1,3);atMost(b,o+76,32);break;
    case 3:for(auto [off,v]:{std::pair{72,16}, {76,32},{80,4},{84,1},{88,64},{92,1},{96,1},{100,1}})eq(b,o+off,v);break;
    case 4:atMost(b,o+72,3);atMost(b,o+88,7);break;
    case 5:between(b,o+84,1,2);atMost(b,o+88,2);atMost(b,o+92,1);break;
    case 6:between(b,o+72,1,2);eq(b,o+80,1);atMost(b,o+84,1);break;
    case 7:eq(b,o+76,1);between(b,o+80,1,6);break;
    case 8:eq(b,o+72,1);between(b,o+76,1,2);eq(b,o+80,1);atMost(b,o+88,1);break;
    case 9:eq(b,o+76,1);between(b,o+80,1,2);atMost(b,o+84,1);break;
    case 10:atMost(b,o+72,1);atMost(b,o+76,3);atMost(b,o+80,1);break;
    case 11:between(b,o+72,1,2);eq(b,o+76,1);atMost(b,o+80,1);break;
    case 12:eq(b,o+72,1);eq(b,o+76,2);atMost(b,o+80,1);break;
    case 13:eq(b,o+72,3);atMost(b,o+76,1);atMost(b,o+96,1);break;
    }
}
void body(QByteArrayView b,V4Direction d){
    const auto k=u(b,8);
    if(d==V4Direction::Request){
        qsizetype end=128;
        switch(k){
        case 1:end=148;eq(b,144,0);break;
        case 2:case 6:break;
        case 3:end=152;between(b,128,1,13);break;
        case 4:end=136;break;
        case 5:end=132;atMost(b,128,1);break;
        case 10:case 11:end=176;break;
        case 12:end=144;break;
        case 20:end=312;between(b,144,1,2);pose(b,184,true);break;
        case 21:end=344;eq(b,164,1);between(b,204,1,2);between(b,340,1,2);pose(b,208,false);break;
        case 22:end=180;break;
        case 23:end=216;break;
        case 24:end=240;break;
        case 25:end=148;atMost(b,144,25);break;
        case 30:end=160;break;
        }
        reserved(b,end,512-end);return;
    }
    if(k==101){atMost(b,176,3);eq(b,180,32);eq(b,184,64);eq(b,188,65536);eq(b,192,2000);eq(b,196,512);eq(b,200,1024);atMost(b,204,25);reserved(b,208,816);return;}
    if(k==103){
        text(b,200,64);atMost(b,916,25);reserved(b,920,104);
        if(u(b,916)==10 || u(b,916)==11){reserved(b,152,764);return;}
        metadata(b);return;
    }
    reserved(b,956,68);atMost(b,192,3);atMost(b,196,2);atMost(b,200,1);atMost(b,204,25);
    atMost(b,208,65535);atMost(b,220,15);atMost(b,224,1);atMost(b,228,12);
    atMost(b,884,1);atMost(b,888,1);
    need((u(b,216)&~u(b,212))==0,"V4 invalid flag bit value");
    for(int i=0;i<16;++i)if(!(u(b,208)&(quint32(1)<<i)))reserved(b,280+4*i,4);
    for(int i=0;i<4;++i){text(b,344+68*i,64);if(!(u(b,220)&(quint32(1)<<i)))reserved(b,344+68*i,68);}
    if(u(b,224))pose(b,616,false);else reserved(b,616,128);
    text(b,744,128);need(u(b,952)==0 || requestKind(u(b,952)),"V4 unknown originating kind");
    if(k!=111){reserved(b,144,16);reserved(b,192,12);reserved(b,208,20);reserved(b,280,464);}
    if(k!=111 && k!=112){reserved(b,128,16);reserved(b,160,32);}
    if(k==111){
        need(u(b,192)==1 || (u(b,196)==0 && u(b,200)==0),"V4 failed terminal decision/validity");
        need(C::terminalDigest(b)==b.sliced(160,32),"V4 terminal digest mismatch");
    }
}
QUuid uuid(QByteArrayView b,qsizetype o){return QUuid::fromRfc4122(b.sliced(o,16));}
}
qsizetype CommunicationV4Codec::frameBytes(V4Direction d){return d==V4Direction::Request?requestBytes:responseBytes;}
quint32 CommunicationV4Codec::uint32(QByteArrayView b,qsizetype o){range(b,o,4);return qFromBigEndian<quint32>(b.data()+o);}
qint32 CommunicationV4Codec::int32(QByteArrayView b,qsizetype o){return std::bit_cast<qint32>(uint32(b,o));}
quint64 CommunicationV4Codec::uint64(QByteArrayView b,qsizetype o){range(b,o,8);return qFromBigEndian<quint64>(b.data()+o);}
double CommunicationV4Codec::float64(QByteArrayView b,qsizetype o){return std::bit_cast<double>(uint64(b,o));}
void CommunicationV4Codec::putUInt32(QByteArray& b,qsizetype o,quint32 v){range(b,o,4);qToBigEndian(v,b.data()+o);}
void CommunicationV4Codec::putInt32(QByteArray& b,qsizetype o,qint32 v){putUInt32(b,o,std::bit_cast<quint32>(v));}
void CommunicationV4Codec::putUInt64(QByteArray& b,qsizetype o,quint64 v){range(b,o,8);qToBigEndian(v,b.data()+o);}
void CommunicationV4Codec::putFloat64(QByteArray& b,qsizetype o,double v){need(std::isfinite(v),"V4 nonfinite float");putUInt64(b,o,std::bit_cast<quint64>(v==0?0.:v));}
void CommunicationV4Codec::validateHeader(QByteArrayView b,V4Direction d){
    need(b.size()>=128,"V4 incomplete header");need(b.first(4)==QByteArrayView("VF04",4),"V4 bad magic");
    eq(b,4,4);eq(b,12,quint32(frameBytes(d)));eq(b,16,0);reserved(b,104,24);
    need(d==V4Direction::Request?requestKind(u(b,8)):responseKind(u(b,8)),"V4 unknown/direction kind");
    need(uint64(b,24)!=0,"V4 request ID zero");
    if(d==V4Direction::Request){
        const auto k=u(b,8);
        need(!zero(b,48,16) && !zero(b,64,16),"V4 peer/epoch required");
        if(k==1){reserved(b,20,4);reserved(b,32,16);reserved(b,80,24);}
        else need(!zero(b,32,16),"V4 session required");
        if(k==2)reserved(b,80,24);
        if(k==3 || k==30){reserved(b,20,4);reserved(b,80,24);}
    }
}
CommunicationV4Frame CommunicationV4Codec::decode(QByteArrayView b,V4Direction d){
    need(b.size()==frameBytes(d),"V4 exact frame size");validateHeader(b,d);body(b,d);
    CommunicationV4Header h{u(b,8),u(b,20),uint64(b,24),uuid(b,32),uuid(b,48),uuid(b,64),u(b,80),u(b,84),uuid(b,88)};
    return {h,QByteArray(b.data()+128,b.size()-128)};
}
QByteArray CommunicationV4Codec::encode(const CommunicationV4Frame& f,V4Direction d){
    need(f.body.size()==frameBytes(d)-128,"V4 fixed body size");QByteArray b(frameBytes(d),0);b.replace(qsizetype(0),4,"VF04",4);
    const auto& h=f.header;putUInt32(b,4,4);putUInt32(b,8,h.kind);putUInt32(b,12,quint32(b.size()));putUInt32(b,20,h.stationKey);putUInt64(b,24,h.requestId);
    b.replace(32,16,h.session.toRfc4122());b.replace(48,16,h.peer.toRfc4122());b.replace(64,16,h.epoch.toRfc4122());
    putUInt32(b,80,h.recipeRevisionKey);putUInt32(b,84,h.templateRevisionKey);b.replace(88,16,h.activation.toRfc4122());b.replace(128,f.body.size(),f.body);
    validateHeader(b,d);body(b,d);return b;
}
QByteArray CommunicationV4Codec::requestFingerprint(QByteArrayView b){
    need(b.size()==512,"V4 fingerprint size");validateHeader(b,V4Direction::Request);
    QByteArray p("vf.socket4.request/1",21);p+=b.sliced(8,4);p+=b.sliced(20,12);p+=b.sliced(48,56);p+=b.sliced(128,384);
    return QCryptographicHash::hash(p,QCryptographicHash::Sha256);
}
QByteArray CommunicationV4Codec::terminalDigest(QByteArrayView b){
    need(b.size()==1024,"V4 terminal digest size");
    QByteArray p("vf.socket4.terminal/1",22);p+=b.sliced(20,12);p+=b.sliced(48,56);p+=b.sliced(128,32);p+=QByteArray(32,0);p+=b.sliced(192);
    return QCryptographicHash::hash(p,QCryptographicHash::Sha256);
}

[[noreturn]] void CommunicationV4Framer::fail(const char* why){failed_=true;buffer_.clear();arrival_.clear();throw std::runtime_error(why);}
void CommunicationV4Framer::tick(qint64 now){
    if(failed_)throw std::runtime_error("V4 failed framer; close/reset peer");
    if(now<0 || now<last_)fail("V4 monotonic clock required");last_=now;
    const auto size=C::frameBytes(direction_);
    for(qsizetype o=0;o<buffer_.size();o+=size){
        const auto n=qMin(size,buffer_.size()-o);
        if(n>=128){try{C::validateHeader(QByteArrayView(buffer_).sliced(o,n),direction_);}catch(...){fail("V4 invalid stream header");}}
        const auto end=n==size?arrival_[o+size-1]:now;
        if(end-arrival_[o]>=partialDeadlineMs)fail("V4 partial deadline");
    }
}
QVector<CommunicationV4Frame> CommunicationV4Framer::feed(QByteArrayView bytes,qint64 now){
    tick(now);
    if(bytes.size()>maxReadBytes || bytes.size()>maxBufferBytes-buffer_.size())fail("V4 receive cap");
    buffer_.append(bytes);for(qsizetype i=0;i<bytes.size();++i)arrival_.append(now);tick(now);
    QVector<CommunicationV4Frame> out;const auto size=C::frameBytes(direction_);qsizetype consumed=0;
    try{while(buffer_.size()-consumed>=size && out.size()<maxBatch){out.append(C::decode(QByteArrayView(buffer_).sliced(consumed,size),direction_));consumed+=size;}}
    catch(...){fail("V4 invalid stream frame");}
    buffer_.remove(0,consumed);arrival_.remove(0,consumed);return out;
}
void CommunicationV4Framer::reset(){buffer_.clear();arrival_.clear();last_=-1;failed_=false;}
bool CommunicationV4WriteQueue::enqueue(const CommunicationV4Frame& f){
    if(queuedFrames()>=maxFrames || queuedBytes_>highWaterBytes-C::responseBytes)return false;
    auto bytes=C::encode(f,V4Direction::Response);queuedBytes_+=bytes.size();frames_.push_back(std::move(bytes));return true;
}
bool CommunicationV4WriteQueue::backpressured(qsizetype socketBytes) const {
    need(socketBytes>=0,"V4 socket byte count");return queuedFrames()>=maxFrames || socketBytes>=highWaterBytes || queuedBytes_>=highWaterBytes-socketBytes;
}
QByteArrayView CommunicationV4WriteQueue::pendingBytes(qsizetype socketBytes) const {
    need(socketBytes>=0,"V4 socket byte count");if(frames_.empty() || socketBytes>=highWaterBytes)return {};
    return QByteArrayView(frames_.front()).sliced(offset_).first(qMin(qsizetype(frames_.front().size()-offset_),highWaterBytes-socketBytes));
}
void CommunicationV4WriteQueue::consumeWritten(qsizetype n){
    need(n>=0 && !frames_.empty() && n<=frames_.front().size()-offset_,"V4 partial write count");
    offset_+=n;queuedBytes_-=n;if(offset_==frames_.front().size()){frames_.pop_front();offset_=0;}
}
void CommunicationV4WriteQueue::reset(){frames_.clear();offset_=0;queuedBytes_=0;}
}
