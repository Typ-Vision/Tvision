#include "EndpointConnectionProbe.hpp"
#include <stdexcept>
namespace visionflow {
EndpointConnectionProbe::EndpointConnectionProbe(QObject* parent):QObject(parent){
    timer_.setSingleShot(true);
    connect(&timer_,&QTimer::timeout,this,[this]{disconnectProfile();report("TIMEOUT — bağlantı kurulamadı");});
    connect(&socket_,&QTcpSocket::connected,this,[this]{timer_.stop();if(testOnly_){socket_.abort();report("TEST OK — yalnız TCP bağlantısı");}else report("CONNECTED — yalnız bağlantı denemesi");});
    connect(&socket_,&QTcpSocket::errorOccurred,this,[this]{timer_.stop();report("CONNECTION ERROR — adres/port erişilemiyor");});
    connect(&socket_,&QTcpSocket::disconnected,this,[this]{if(!testOnly_)report("DISCONNECTED");});
    connect(&socket_,&QTcpSocket::readyRead,this,[this]{socket_.readAll();});
    connect(&server_,&QTcpServer::newConnection,this,[this]{while(server_.hasPendingConnections()){auto* peer=server_.nextPendingConnection();peer->abort();peer->deleteLater();}});
}
void EndpointConnectionProbe::report(const QString& s){state_=s;if(changed)changed();}
bool EndpointConnectionProbe::active() const{return server_.isListening() || socket_.state()!=QAbstractSocket::UnconnectedState;}
void EndpointConnectionProbe::disconnectProfile(){timer_.stop();socket_.abort();server_.close();report("DISCONNECTED");}
void EndpointConnectionProbe::connectProfile(const EndpointProfile& p,bool testOnly){
    validateEndpointProfile(p);if(!p.enabled)throw std::runtime_error("Profil Enabled olmalı.");
    QHostAddress address;if(!address.setAddress(p.address))throw std::runtime_error("Geçerli IPv4/IPv6 adresi girin.");
    disconnectProfile();testOnly_=testOnly;
    if(p.role=="server"){
        if(!server_.listen(address,quint16(p.port)))throw std::runtime_error("Adres/port kullanılamıyor veya başka listener ile çakışıyor.");
        if(testOnly){server_.close();report("TEST OK — yerel adres/port kullanılabilir");}else report("LISTENING — yalnız bağlantı denemesi; üretim kapalı");
    }else{report("CONNECTING");timer_.start(p.timeoutMs);socket_.connectToHost(address,p.port);}
}
}
