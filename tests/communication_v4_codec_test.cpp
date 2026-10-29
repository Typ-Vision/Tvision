#include "application/CommunicationV4Codec.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
using namespace visionflow;
using C=CommunicationV4Codec;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
QByteArray read(const QString& name){QFile file(QString(VISIONFLOW_V4_FIXTURES)+"/"+name);check(file.open(QIODevice::ReadOnly),"fixture opens");return file.readAll();}
QByteArray golden(const QString& name){return read("golden/"+name+".bin");}
void seal(QByteArray& b){b.replace(160,32,C::terminalDigest(b));}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto manifest=QJsonDocument::fromJson(read("manifest.json")).object();int valid=0,negative=0,semantic=0;
    for(const auto& v:manifest["cases"].toArray()){
        const auto c=v.toObject();const auto name=c["name"].toString();const auto b=golden(name);
        const auto direction=b.size()==512?V4Direction::Request:V4Direction::Response;
        const auto outcome=c["outcome"].toString();
        // These mutations are well-formed bytes whose rejection requires the
        // future session/catalogue/ledger authority, not a hardcoded fixture ID.
        const bool authority=outcome=="STALE_SESSION" || outcome=="STALE_CONFIG" || outcome=="ACK_MISMATCH" || outcome=="unknown peer, not canonical UUID" || outcome=="decoded 32768, not INT_MIN";
        if(outcome=="valid" || authority){
            try{
                const auto f=C::decode(b,direction);check(C::encode(f,direction)==b,"golden encode/decode byte exact");
                if(authority){if(name=="bad_signed_word_order")check(C::int32(b,280)==32768,"word reversal is decoded as 32768, never INT_MIN");++semantic;continue;}++valid;
                if(direction==V4Direction::Request)check(C::requestFingerprint(b).toHex()==c["requestFingerprint"].toString().toLatin1(),"frozen fingerprint");
                if(f.header.kind==111)check(C::terminalDigest(b).toHex()==c["terminalDigest"].toString().toLatin1(),"frozen terminal digest");
                CommunicationV4Framer stream(direction);QVector<CommunicationV4Frame> frames;
                for(qsizetype i=0;i<b.size();++i)frames+=stream.feed(QByteArrayView(b).sliced(i,1),0);
                check(frames.size()==1 && C::encode(frames[0],direction)==b && stream.bufferedBytes()==0,"one byte stream every golden");
            }catch(const std::exception& e){throw std::runtime_error((name+": "+e.what()).toStdString());}
        }else{++negative;reject([&]{C::decode(b,direction);},qPrintable(name));}
    }
    const auto trigger=golden("trigger"),terminal=golden("terminal");
    const auto t=C::decode(trigger,V4Direction::Request);
    check(t.header.requestId==9007199254740993ULL,"uint64 above 2^53");
    check(C::decode(golden("request_18446744073709551615"),V4Direction::Request).header.requestId==std::numeric_limits<quint64>::max(),"uint64 max");
    check(t.header.peer.toString(QUuid::WithoutBraces)=="00112233-4455-4677-8899-aabb00000002","UUID RFC byte order");
    check(C::int32(terminal,280)==std::numeric_limits<qint32>::min() && C::int32(terminal,284)==std::numeric_limits<qint32>::max(),"signed32 min/max");
    QByteArray numbers(24,0);C::putInt32(numbers,0,std::numeric_limits<qint32>::min());C::putUInt64(numbers,4,std::numeric_limits<quint64>::max());C::putFloat64(numbers,12,1.0);
    check(numbers.first(4).toHex()=="80000000" && numbers.mid(4,8).toHex()=="ffffffffffffffff" && numbers.mid(12,8).toHex()=="3ff0000000000000","independent BE encoder anchors");
    C::putFloat64(numbers,12,-0.);check(numbers.mid(12,8)==QByteArray(8,0),"canonical negative zero encoder");
    reject([&]{C::putFloat64(numbers,0,std::numeric_limits<double>::infinity());},"infinite writer");
    reject([&]{C::uint64(numbers,-1);},"negative field offset");reject([&]{C::putUInt64(numbers,20,1);},"out of bounds writer");
    for(int split=1;split<512;++split){CommunicationV4Framer stream;check(stream.feed(QByteArrayView(trigger).first(split),0).isEmpty(),"split partial");const auto frames=stream.feed(QByteArrayView(trigger).sliced(split),1999);check(frames.size()==1 && C::encode(frames[0],V4Direction::Request)==trigger,"all split positions");}
    QByteArray four;for(const auto& name:{"trigger_station1","trigger_station2","trigger_station3","status"})four+=golden(name);
    CommunicationV4Framer stream;auto frames=stream.feed(four,10);check(frames.size()==4,"coalesced four batch");
    check(frames[0].header.stationKey==1 && frames[1].header.stationKey==2 && frames[2].header.stationKey==3 && frames[3].header.kind==2,"coalesced ordering");
    stream.reset();stream.feed(trigger.first(1),0);stream.feed(trigger.mid(1,1),1999);
    reject([&]{stream.feed(trigger.mid(2),2000);},"trickle cannot renew deadline including final byte");check(stream.bufferedBytes()==0,"fatal releases receive bytes");
    reject([&]{stream.feed(trigger,2001);},"fatal until reset");stream.reset();stream.feed(trigger.first(1),3);reject([&]{stream.tick(2003);},"idle tick deadline");
    stream.reset();stream.feed(trigger.first(1),3);reject([&]{stream.tick(2);},"backward clock");
    stream.reset();reject([&]{stream.feed(QByteArray(2049,0),0);},"read cap before append");check(stream.bufferedBytes()==0,"oversized read does not grow receive");
    for(auto name:{"bad_size","bad_version","bad_magic","bad_flags","bad_kind"}){stream.reset();reject([&]{stream.feed(golden(name).first(128),0);},"header rejected before body allocation");check(stream.bufferedBytes()==0,"invalid header allocation bounded");}
    stream.reset();QByteArray noResync=golden("bad_magic")+trigger;reject([&]{stream.feed(noResync,0);},"no magic resync");
    // Partial tail has its own original arrival, even after completed frames.
    stream.reset();stream.feed(trigger+trigger.first(10),100);reject([&]{stream.feed(trigger.mid(10),2100);},"tail deadline");
    // Recompute digest so scalar/text/pose checks stand independently of hashing.
    for(const auto& v:QVector<std::pair<qsizetype,quint32>>{{192,4},{196,3},{200,2},{204,26},{208,65536},{220,16},{224,2},{228,13},{884,2},{888,2},{952,999}}){auto bad=terminal;C::putUInt32(bad,v.first,v.second);seal(bad);reject([&]{C::decode(bad,V4Direction::Response);},"closed operational scalar");}
    for(auto name:{"bad_utf8","bad_text65","bad_text_padding","bad_number_mask"}){auto bad=golden(name);seal(bad);reject([&]{C::decode(bad,V4Direction::Response);},"text/mask independent of digest");}
    for(auto bits:{quint64(0x7ff0000000000000ULL),quint64(0xfff0000000000000ULL),quint64(0x7ff8000000000000ULL)}){auto bad=terminal;C::putUInt64(bad,616,bits);seal(bad);reject([&]{C::decode(bad,V4Direction::Response);},"NaN/Inf sealed pose");}
    auto bad=trigger;for(int i=4;i<8;++i)bad[i]=trigger[11-i];reject([&]{C::decode(bad,V4Direction::Request);},"endian reversed version");
    bad=terminal;bad.replace(280,4,QByteArray::fromHex("00008000"));reject([&]{C::decode(bad,V4Direction::Response);},"signed word mutation invalidates exact digest");
    check(C::requestFingerprint(trigger)==C::requestFingerprint(golden("trigger_reconnect_session")),"session excluded fingerprint");
    check(C::terminalDigest(terminal)==C::terminalDigest(golden("terminal_reconnect_session")),"session excluded terminal digest");
    CommunicationV4WriteQueue queue;const auto tf=C::decode(terminal,V4Direction::Response);const auto status=C::decode(golden("status_reply"),V4Direction::Response);
    check(queue.enqueue(tf) && queue.enqueue(status),"write enqueue");QByteArray sent;
    while(queue.queuedFrames()){const auto bytes=queue.pendingBytes(65530);check(bytes.size()<=6,"socket high water credit");const auto n=qMin(qsizetype(3),bytes.size());sent+=bytes.first(n);queue.consumeWritten(n);}
    check(sent==terminal+golden("status_reply"),"partial writes never interleave frames");
    for(int i=0;i<64;++i)check(queue.enqueue(tf),"64 response cap fits");check(queue.queuedBytes()==65536 && queue.backpressured() && !queue.enqueue(tf),"queue exact cap explicit saturation");
    check(queue.pendingBytes(65536).isEmpty(),"socket saturated no write");queue.consumeWritten(1);check(!queue.enqueue(tf),"partial head still counts as frame");
    reject([&]{queue.consumeWritten(1024);},"cannot cross partial frame boundary");queue.consumeWritten(1023);check(queue.enqueue(tf),"drained frame returns queue credit");
    queue.reset();check(queue.queuedBytes()==0 && queue.pendingBytes().isEmpty(),"write reset");
    // Deterministic random length and structured mutations; no exception escapes
    // besides documented rejection, and state never exceeds receive bounds.
    std::mt19937 random(0x434f4d34);int fuzzAccepted=0;
    for(int i=0;i<20000;++i){
        auto input=i%2?trigger:terminal;const auto d=i%2?V4Direction::Request:V4Direction::Response;
        for(int j=0;j<1+int(random()%8);++j)input[random()%input.size()]=char(random());
        if(i%7==0)input.resize(random()%1500);
        std::optional<CommunicationV4Frame> parsed;
        try{parsed=C::decode(input,d);}catch(const std::runtime_error&){}
        if(parsed){check(C::encode(*parsed,d)==input,"fuzz accepted exact roundtrip");++fuzzAccepted;}
        CommunicationV4Framer fuzz(d);try{fuzz.feed(input,0);fuzz.tick(2000);}catch(const std::runtime_error&){}
        check(fuzz.bufferedBytes()<=CommunicationV4Framer::maxBufferBytes,"fuzz receive bounded");
    }
    QTextStream(stdout)<<"PASS checks="<<checks<<" golden="<<valid<<" shapeNegative="<<negative<<" authorityDeferred="<<semantic<<" fuzz=20000 accepted="<<fuzzAccepted<<" skipped=0\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
