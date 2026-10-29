#include "CommunicationPeerSessionService.hpp"
#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>
#include <algorithm>
#include <stdexcept>

namespace visionflow {
namespace {
using C=CommunicationV4Codec;
QUuid uuid(QByteArrayView b,qsizetype o){return QUuid::fromRfc4122(b.sliced(o,16));}
void putUuid(QByteArray& b,qsizetype o,const QUuid& id){b.replace(o,16,id.toRfc4122());}
}
CommunicationPeerSessionService::CommunicationPeerSessionService(StationIdentityStore& store,Metadata metadata):store_(store),metadata_(std::move(metadata)){}
QUuid CommunicationPeerSessionService::contractRevision(){return QUuid("00434634-0002-4000-8000-202610290001");}
void CommunicationPeerSessionService::owner() const {if(QThread::currentThread()!=owner_)throw std::runtime_error("Session owner thread required");}
bool CommunicationPeerSessionService::open(const QUuid& connection){owner();if(connection.isNull() || !connection_.isNull())return false;connection_=connection;return true;}
void CommunicationPeerSessionService::close(const QUuid& connection){owner();if(connection_!=connection)return;connection_=QUuid{};session_=QUuid{};peer_=QUuid{};epoch_=QUuid{};installation_=QUuid{};exact_=false;}
quint32 CommunicationPeerSessionService::identityReason(const IdentityCatalogue& c,const CommunicationV4Header& h) const {
    for(const auto& e:c.epochs)if(e.peer==h.peer && e.epoch==h.epoch && e.retired)return 19;
    // The v4 policy has one locally enrolled controlling identity. Ambiguous
    // active enrollment never selects the first network caller as controller.
    if(std::count_if(c.epochs.begin(),c.epochs.end(),[](const auto& e){return !e.retired;})!=1)return 24;
    for(const auto& e:c.epochs)if(e.peer==h.peer && e.epoch==h.epoch)return 0;
    return 24; // Explicit local enrollment is required; never create a namespace.
}
quint32 CommunicationPeerSessionService::authorize(const QUuid& connection,const CommunicationV4Frame& f) const {
    owner();if(connection!=connection_)return 18;
    try{C::encode(f,V4Direction::Request);}catch(...){return 13;}
    if(session_.isNull() || f.header.session!=session_ || f.header.peer!=peer_ || f.header.epoch!=epoch_)return 5;
    try{const auto c=store_.load();validateIdentityCatalogue(c);if(c.installation!=installation_)return 24;if(auto reason=identityReason(c,f.header))return reason;}
    catch(...){return 8;}
    if(f.header.kind!=2 && f.header.kind!=3 && !exact_)return 24;
    return 0;
}
CommunicationV4Frame CommunicationPeerSessionService::response(const CommunicationV4Frame& f,quint32 kind,quint32 reason) const {
    CommunicationV4Frame r{f.header,QByteArray(896,0)};r.header.kind=kind;r.header.session=session_;
    C::putUInt32(r.body,76,reason);C::putUInt32(r.body,824,f.header.kind);return r;
}
CommunicationV4Frame CommunicationPeerSessionService::handle(const QUuid& connection,const CommunicationV4Frame& f){
    owner(); // Keep the state port safe for non-socket callers too.
    C::encode(f,V4Direction::Request);
    if(f.header.kind==1){
        auto r=response(f,101,0);r.body.fill(0);r.header.session=QUuid{};
        putUuid(r.body,16,contractRevision());
        for(auto [o,v]:{std::pair{48,2}, {52,32},{56,64},{60,65536},{64,2000},{68,512},{72,1024}})C::putUInt32(r.body,o,v);
        quint32 reason=connection==connection_?0:18;
        // Every HELLO on the owner connection fences its previous session,
        // including failed negotiation (no stale exact authority survives).
        if(connection==connection_){session_=QUuid{};peer_=QUuid{};epoch_=QUuid{};installation_=QUuid{};exact_=false;}
        if(!reason)try{
            const auto c=store_.load();validateIdentityCatalogue(c);putUuid(r.body,0,c.installation);putUuid(r.body,32,c.revision);
            reason=identityReason(c,f.header);const auto contract=uuid(f.body,0);
            if(!reason && !contract.isNull() && contract!=contractRevision())reason=24;
            if(!reason){session_=QUuid::createUuid();peer_=f.header.peer;epoch_=f.header.epoch;installation_=c.installation;exact_=!contract.isNull();r.header.session=session_;}
        }catch(...){reason=8;}
        C::putUInt32(r.body,76,reason);return r;
    }
    if(auto reason=authorize(connection,f))return response(f,190,reason);
    if(f.header.kind==2){
        // Global status is truthful before station composition. Station status
        // belongs to the station application port introduced by COMM4-08.
        return response(f,f.header.stationKey?190:102,f.header.stationKey?20:0);
    }
    if(f.header.kind==3){
        try{
            const auto c=store_.load();validateIdentityCatalogue(c);
            const auto expected=uuid(f.body,8);if(!expected.isNull() && expected!=c.revision)return response(f,190,4);
            const auto category=C::uint32(f.body,0),key=C::uint32(f.body,4);
            const CatalogueEntry* found=nullptr;quint32 next=0;
            for(const auto& e:c.entries)if(!e.retired && quint32(e.identity.category)==category && (!key || e.key==key))if(!found || e.key<found->key)found=&e;
            auto r=response(f,103,0);r.body.fill(0);putUuid(r.body,0,c.revision);C::putUInt32(r.body,16,category);C::putUInt32(r.body,20,key);
            if(!found){C::putUInt32(r.body,788,10);return r;}
            if(!metadata_)return response(f,190,20);
            const auto metadata=metadata_(c,*found);if(metadata.size()!=256)return response(f,190,20);
            for(const auto& e:c.entries)if(!e.retired && quint32(e.identity.category)==category && e.key>found->key && (!next || e.key<next))next=e.key;
            C::putUInt32(r.body,20,found->key);putUuid(r.body,24,found->identity.entity);putUuid(r.body,40,found->identity.revision);putUuid(r.body,56,found->identity.parent);
            const auto name=found->name.toUtf8();C::putUInt32(r.body,72,name.size());r.body.replace(76,name.size(),name);
            C::putUInt32(r.body,140,256);r.body.replace(144,256,metadata);C::putUInt32(r.body,784,next);
            C::encode(r,V4Direction::Response);return r;
        }catch(...){return response(f,190,8);}
    }
    if((f.header.kind==4 || f.header.kind==5 || f.header.kind==6 || f.header.kind==11) && control_){
        std::optional<V4RequestIdentity> key;
        try{key=requestIdentity(connection,f);}catch(...){return response(f,190,10);}
        try{if(auto result=control_(*key,f))return *result;}catch(...){return response(f,190,8);}
    }
    if(f.header.kind==10 && trigger_){
        std::optional<V4RequestIdentity> key;
        try{key=requestIdentity(connection,f);}catch(...){return response(f,190,10);}
        try{return trigger_(*key,f);}catch(...){return response(f,190,8);}
    }
    // Missing station composition never grants production permission.
    return response(f,190,20);
}
V4RequestIdentity CommunicationPeerSessionService::requestIdentity(const QUuid& connection,const CommunicationV4Frame& f) const {
    if(authorize(connection,f))throw std::runtime_error("V4 request has no session authority");
    const auto c=store_.load();validateIdentityCatalogue(c);
    for(const auto& e:c.entries)if(e.key==f.header.stationKey && !e.retired && e.identity.category==CatalogueCategory::Station)return {c.installation,f.header.peer,f.header.epoch,e.identity.entity,f.header.requestId};
    throw std::runtime_error("V4 station identity is unknown");
}
namespace {
class PeerConnection final : public QObject {
public:
    PeerConnection(QTcpSocket& socket,CommunicationPeerSessionService& service):QObject(&socket),socket_(socket),service_(service){
        accepted_=service_.open(id_);socket_.setReadBufferSize(4096);clock_.start();timer_.setInterval(25);
        connect(&socket_,&QTcpSocket::readyRead,this,[this]{pump();});
        connect(&socket_,&QTcpSocket::bytesWritten,this,[this]{write();});
        connect(&socket_,&QTcpSocket::disconnected,this,[this]{service_.close(id_);timer_.stop();});
        connect(&timer_,&QTimer::timeout,this,[this]{pump();});timer_.start();
    }
    ~PeerConnection(){service_.close(id_);}
private:
    void write(){try{while(true){const auto bytes=queue_.pendingBytes(socket_.bytesToWrite());if(bytes.isEmpty())break;const auto n=socket_.write(bytes.data(),bytes.size());if(n<0)throw std::runtime_error("V4 socket write");if(!n)break;queue_.consumeWritten(n);}if(closing_ && !queue_.queuedBytes())socket_.disconnectFromHost();}catch(...){socket_.abort();}}
    void pump(){
        if(socket_.state()!=QAbstractSocket::ConnectedState)return;
        try{
            if(!accepted_ && clock_.elapsed()>=2000){socket_.abort();return;}
            framer_.tick(clock_.elapsed());if(closing_)return;
            const auto bytes=socket_.read(qMin<qint64>(socket_.bytesAvailable(),2048));
            const auto frames=framer_.feed(bytes,clock_.elapsed());
            for(const auto& frame:frames){if(!queue_.enqueue(service_.handle(id_,frame)))throw std::runtime_error("V4 response queue full");if(!accepted_){closing_=true;break;}}
            write();
        }catch(...){socket_.abort();}
    }
    QTcpSocket& socket_;CommunicationPeerSessionService& service_;QUuid id_=QUuid::createUuid();
    CommunicationV4Framer framer_;CommunicationV4WriteQueue queue_;QElapsedTimer clock_;QTimer timer_;bool accepted_=false,closing_=false;
};
}
void attachCommunicationPeer(QTcpSocket& socket,CommunicationPeerSessionService& service){new PeerConnection(socket,service);}
}
