#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QUuid>

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QCommandLineParser parser;parser.setApplicationDescription("VisionFlow vf.socket/2 PLC simulator; triggers a real active Runtime cycle.");parser.addHelpOption();
    parser.addOptions({{"host","VisionFlow server address","host","127.0.0.1"},{"port","TCP port","port","5000"},
        {"request","Request ID; default fresh UUID","id"},{"duplicate","Send the same trigger twice"},
        {"reconnect","Disconnect after BUSY, reconnect and repeat the same request"},
        {"drop-ack","Observe results for 10 seconds without ACK; leaves server pending"},
        {"ack-delay","Delay ACK in milliseconds","ms","0"},{"resume","ACK an outstanding result without triggering a cycle"},
        {"status-only","Read HELLO without acquisition"}});
    parser.process(app);bool portOk=false,delayOk=false;const auto port=parser.value("port").toUInt(&portOk);const auto delay=parser.value("ack-delay").toInt(&delayOk);
    if(!portOk || !port || port>65535 || !delayOk || delay<0 || delay>15000){QTextStream(stderr)<<"Invalid port/ACK delay\n";return 2;}
    QTcpSocket socket;QByteArray input;QString session;QJsonObject request;bool reconnectDone=false,ackScheduled=false,dropScheduled=false;
    const auto requestId=parser.isSet("request") ? parser.value("request") : QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto send=[&](QJsonObject o){o["v"]=2;o["sessionId"]=session;const auto data=QJsonDocument(o).toJson(QJsonDocument::Compact);QTextStream(stdout)<<"TX "<<data<<'\n';socket.write(data+'\n');};
    QObject::connect(&socket,&QTcpSocket::readyRead,&app,[&]{
        input+=socket.readAll();if(input.size()>65536){app.exit(2);return;}
        for(;;){const auto end=input.indexOf('\n');if(end<0)break;const auto line=input.left(end);input.remove(0,end+1);
            const auto doc=QJsonDocument::fromJson(line);if(!doc.isObject()){app.exit(2);return;}const auto o=doc.object();QTextStream(stdout)<<"RX "<<line<<'\n';
            const auto type=o["type"].toString();
            if(o["v"]!=2){app.exit(2);return;}
            if(type=="HELLO") {
                if(!session.isEmpty() && session!=o["sessionId"].toString()){QTextStream(stderr)<<"Server session changed; automatic retrigger refused.\n";app.exit(3);return;}
                session=o["sessionId"].toString();if(parser.isSet("status-only")){app.exit(0);return;}
                if(parser.isSet("resume")){if(o["requestId"].toString().isEmpty()){QTextStream(stderr)<<"No outstanding request\n";app.exit(3);}continue;}
                if(request.isEmpty()) {
                    request={{"type","TRIGGER"},{"requestId",requestId},{"recipeId",o["recipeId"]},{"configId",o["configId"]}};
                }send(request);if(parser.isSet("duplicate"))send(request);
            }else if(type=="BUSY" && parser.isSet("reconnect") && !reconnectDone) {
                reconnectDone=true;socket.abort();input.clear();QTimer::singleShot(100,&app,[&]{socket.connectToHost(parser.value("host"),quint16(port));});return;
            }else if(type=="DONE") {
                if(!parser.isSet("resume") && o["requestId"]!=requestId){QTextStream(stderr)<<"Unexpected request result\n";app.exit(3);return;}
                if(parser.isSet("drop-ack")){if(!dropScheduled){dropScheduled=true;QTimer::singleShot(10000,&app,[&]{app.exit(0);});}continue;}
                if(!ackScheduled){ackScheduled=true;const auto id=o["requestId"].toString();QTimer::singleShot(delay,&app,[&,id]{send({{"type","ACK"},{"requestId",id}});});}
            }else if(type=="ACKED"){app.exit(0);return;}
            else if(type=="INVALID" || type=="ERROR"){app.exit(3);return;}
        }
    });
    QObject::connect(&socket,&QTcpSocket::errorOccurred,&app,[&](QAbstractSocket::SocketError){QTextStream(stderr)<<socket.errorString()<<'\n';app.exit(4);});
    QTimer::singleShot(20000,&app,[&]{QTextStream(stderr)<<"Simulator deadline exceeded\n";app.exit(5);});
    socket.connectToHost(parser.value("host"),quint16(port));return app.exec();
}
