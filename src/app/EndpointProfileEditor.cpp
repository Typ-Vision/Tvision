#include "EndpointProfileEditor.hpp"
#include <QVBoxLayout>
#include <stdexcept>
#include <QFormLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUuid>
#include <QHostAddress>
namespace visionflow {
EndpointProfileEditor::EndpointProfileEditor(QString path,std::function<bool()> active,QWidget* parent):QWidget(parent),store_(std::move(path)),active_(std::move(active)){
    auto* layout=new QVBoxLayout(this);
    auto* note=new QLabel("TCP v3 • Makineye yerel bağlantı profilleri\nConnect/Test yalnız bağlantı denemesidir; tetik ve üretim kapalı. Değişiklik sonrası Stop → Load gerekir. Credential ref bir anahtar adıdır; parola girmeyin.");
    note->setWordWrap(true);layout->addWidget(note);
    auto* form=new QFormLayout;
    list_=new QComboBox;list_->setObjectName("endpointList");form->addRow("Endpoint UUID / revision",list_);
    enabled_=new QCheckBox("Enabled");enabled_->setObjectName("endpointEnabled");form->addRow(enabled_);
    role_=new QComboBox;role_->setObjectName("endpointRole");role_->addItems({"server","client"});form->addRow("TCP v3 rol",role_);
    address_=new QLineEdit;address_->setObjectName("endpointAddress");form->addRow("IPv4 / IPv6 adres",address_);
    auto spin=[&](const char* name,int lo,int hi,const QString& label){auto* s=new QSpinBox;s->setObjectName(name);s->setRange(lo,hi);form->addRow(label,s);return s;};
    port_=spin("endpointPort",1,65535,"Port");timeout_=spin("endpointTimeout",100,60000,"Timeout (ms)");retries_=spin("endpointRetries",0,10,"Retry");
    credential_=new QLineEdit;credential_->setObjectName("endpointCredential");credential_->setEchoMode(QLineEdit::Password);credential_->setMaxLength(256);form->addRow("Credential ref (gizli gösterim)",credential_);layout->addLayout(form);
    auto* grid=new QGridLayout;int i=0;
    for(auto pair:QVector<QPair<QString,QString>>{{"add","&Ekle"},{"save","&Kaydet"},{"cancel","&Vazgeç"},{"connect","&Connect"},{"disconnect","&Disconnect"},{"test","&Test Connection"}}){
        auto* b=new QPushButton(pair.second);b->setObjectName("endpoint_"+pair.first);grid->addWidget(b,i/3,i%3);++i;
        connect(b,&QPushButton::clicked,this,[this,a=pair.first]{action(a);});
    }
    layout->addLayout(grid);
    status_=new QLabel("DISCONNECTED");status_->setObjectName("endpointStatus");status_->setTextFormat(Qt::PlainText);status_->setWordWrap(true);layout->addWidget(status_);layout->addStretch();
    probe_.changed=[this]{status_->setText(probe_.state());};connect(list_,&QComboBox::currentIndexChanged,this,[this]{loadDraft();});
    try{profiles_=store_.load();refreshList();}catch(...){status_->setText("Profil deposu okunamadı; dosyayı düzeltmeden kayıt yapılmaz.");}
}
void EndpointProfileEditor::refreshList(const QString& id){
    QSignalBlocker block(list_);list_->clear();
    for(const auto& p:profiles_)list_->addItem(p.id+" / r"+QString::number(p.revision),p.id);
    list_->setCurrentIndex(id.isEmpty()?0:list_->findData(id));loadDraft();
}
void EndpointProfileEditor::loadDraft(){
    draft_=profiles_.value(list_->currentData().toString());
    if(!profiles_.contains(draft_.id)){draft_.id=QUuid::createUuid().toString(QUuid::WithoutBraces);draft_.role="server";draft_.timeoutMs=2000;draft_.retries=3;}
    enabled_->setChecked(draft_.enabled);role_->setCurrentText(draft_.role);address_->setText(draft_.address);port_->setValue(draft_.port);timeout_->setValue(draft_.timeoutMs);retries_->setValue(draft_.retries);credential_->setText(draft_.credentialRef);
}
void EndpointProfileEditor::action(const QString& a){try{
    if(a=="disconnect"){probe_.disconnectProfile();return;}
    if(a=="cancel"){refreshList(list_->currentData().toString());status_->setText("Taslak iptal edildi.");return;}
    if(a=="add"){list_->setCurrentIndex(-1);loadDraft();status_->setText("Yeni taslak; Kaydet ile uygulanır.");return;}
    auto p=draft_;p.enabled=enabled_->isChecked();p.role=role_->currentText();p.address=address_->text().trimmed();p.port=port_->value();p.timeoutMs=timeout_->value();p.retries=retries_->value();p.credentialRef=credential_->text();
    QHostAddress address;if(!address.setAddress(p.address))throw std::runtime_error("Geçerli IPv4/IPv6 adresi girin.");validateEndpointProfile(p);
    if(a=="save"){
        const auto current=store_.load();
        if(current.contains(p.id) && endpointDependencyId(current[p.id])==endpointDependencyId(p)){status_->setText("Değişiklik yok.");return;}
        if(current.contains(p.id) && endpointDependencyId(current[p.id])!=endpointDependencyId(draft_))throw std::runtime_error("Profil dışarıdan değişti; ekranı yeniden açın.");
        if(active_() || probe_.active())throw std::runtime_error("Aktif bağlantı/Runtime: önce Disconnect ve Stop, sonra Kaydet → Load.");
        auto any=[](const QHostAddress& ip){return ip==QHostAddress::Any || ip==QHostAddress::AnyIPv4 || ip==QHostAddress::AnyIPv6;};
        for(const auto& other:current)if(other.id!=p.id && p.enabled && other.enabled && p.role=="server" && other.role=="server" && p.port==other.port){
            QHostAddress otherAddress(other.address);
            if(address==otherAddress || any(address) || any(otherAddress))throw std::runtime_error("Enabled server profilleri arasında adres/port çakışması.");
        }
        auto next=current;if(current.contains(p.id))p.revision=current[p.id].revision+1;
        next[p.id]=p;store_.save(next);profiles_=next;refreshList(p.id);
        status_->setText("Profil kaydedildi. Loaded snapshot değişmedi; kullanmak için Stop → Load.");
    }else{
        if(active_())throw std::runtime_error("Önce Runtime Stop; bağlantı denemesi üretim sırasında kullanılamaz.");
        if(!profiles_.contains(p.id) || endpointDependencyId(profiles_[p.id])!=endpointDependencyId(p))throw std::runtime_error("Önce taslağı Kaydet veya Vazgeç.");
        probe_.connectProfile(p,a=="test");
    }
}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));}}
}
