#include "app/EndpointProfileEditor.hpp"
#include <QApplication>
#include <stdexcept>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QTextStream>
#include <QElapsedTimer>
#include <QThread>
#include <QFontDatabase>
using namespace visionflow;
int main(int argc,char** argv){QApplication app(argc,argv);app.setStyle("Fusion");QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");try{
    auto check=[](bool x,const char* message){if(!x)throw std::runtime_error(message);};
    const auto artifacts=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS");QDir().mkpath(artifacts);QTemporaryDir dir(QDir(artifacts).filePath("profile-test-XXXXXX"));const auto path=dir.filePath("profiles.json");bool active=false;EndpointProfileEditor panel(path,[&]{return active;});panel.show();
    auto button=[&](const char* n){panel.findChild<QPushButton*>(QString("endpoint_")+n)->click();};auto status=[&]{return panel.findChild<QLabel*>("endpointStatus")->text();};auto* address=panel.findChild<QLineEdit*>("endpointAddress");auto* port=panel.findChild<QSpinBox*>("endpointPort");auto* enabled=panel.findChild<QCheckBox*>("endpointEnabled");auto* secret=panel.findChild<QLineEdit*>("endpointCredential");
    button("cancel");check(!QFile::exists(path),"cancel creates no file");enabled->setChecked(true);secret->setText("vault:private-key");button("save");CommunicationProfiles store(path);check(store.load().size()==1,"atomic add");const auto first=store.load().first();const auto original=endpointDependencyId(first);button("save");check(endpointDependencyId(store.load().first())==original && status().contains("yok"),"no-op revision unchanged");
    address->setText("bad address");button("save");check(status().contains("IPv4") && endpointDependencyId(store.load().first())==original,"invalid address leaves store unchanged");button("cancel");
    active=true;port->setValue(51234);button("save");check(status().contains("Stop") && endpointDependencyId(store.load().first())==original,"active snapshot guard");active=false;button("cancel");
    QTcpServer occupied;check(occupied.listen(QHostAddress::LocalHost,0),"localhost available");port->setValue(occupied.serverPort());button("save");button("test");check(status().contains("çakış"),"real occupied port visible");occupied.close();button("test");check(status().contains("TEST OK"),"real bind test");button("connect");check(status().contains("LISTENING"),"connection-only listener");port->setValue(port->value()==65535?65534:port->value()+1);button("save");check(status().contains("Disconnect"),"probe blocks edits");button("disconnect");button("cancel");
    button("add");enabled->setChecked(true);port->setValue(store.load().first().port);button("save");check(status().contains("çakış") && store.load().size()==1,"duplicate enabled bind rejected");button("cancel");
    EndpointConnectionProbe clientProbe;EndpointProfile client=store.load().first();client.role="client";
    check(occupied.listen(QHostAddress::LocalHost,0),"client simulator listen");client.port=occupied.serverPort();clientProbe.connectProfile(client,true);QElapsedTimer clock;clock.start();while(clientProbe.state()=="CONNECTING" && clock.elapsed()<3000){QApplication::processEvents();QThread::msleep(1);}check(clientProbe.state().contains("TEST OK") && !clientProbe.active(),"async client test closes socket");occupied.close();
    check(secret->echoMode()==QLineEdit::Password && !status().contains("private-key"),"credential redaction");address->setFocus();QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(address,&tab);check(QApplication::focusWidget()!=address,"keyboard traversal");
    for(auto size:{QSize(1400,900),QSize(850,600)}){panel.resize(size);QApplication::processEvents();check(panel.size()==size,"two viewport sizes fit");check(panel.grab().save(QDir(artifacts).filePath(QString("endpoint-%1x%2.png").arg(size.width()).arg(size.height()))),"screenshot saved");}
    QTextStream(stdout)<<"PASS endpoint transactions/cancel/no-op/active guard/real bind/conflict/redaction/keyboard/two sizes\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
