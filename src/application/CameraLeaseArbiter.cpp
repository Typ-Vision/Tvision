#include "CameraLeaseArbiter.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace visionflow {
namespace {
std::mutex mutex;
QMap<QString,std::weak_ptr<CameraLeaseArbiter::Lease>> owners;
QVector<CameraLeaseArbiter::Ticket> quarantine;
quint64 generation=0;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
QString CameraLeaseArbiter::physicalKey(const PhysicalDeviceIdentity& p){
    p.validate();require(p.usable(),"Exact physical camera required");
    return QString::fromLatin1(QJsonDocument(QJsonArray{p.vendor,p.model,p.serial}).toJson(QJsonDocument::Compact).toHex());
}
CameraLeaseArbiter::Reservation CameraLeaseArbiter::reserve(QString owner,CameraLeaseUse use,
    const QVector<CameraLeaseTarget>& targets,const QVector<Ticket>& reusable){
    require(!owner.trimmed().isEmpty(),"Camera lease owner required");
    require(use==CameraLeaseUse::Runtime || use==CameraLeaseUse::Editor || use==CameraLeaseUse::Calibration || use==CameraLeaseUse::Preview,"Invalid lease use");
    auto ordered=targets;std::sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.cameraId<b.cameraId;});
    QMap<QString,CameraLeaseTarget> unique;QMap<QString,QString> ids;
    for(const auto& t:ordered){auto key=physicalKey(t.physical);
        require(!unique.contains(key) || unique[key].cameraId==t.cameraId,"Conflicting UUID aliases for physical camera");
        require(t.cameraId.isEmpty() || !ids.contains(t.cameraId) || ids[t.cameraId]==key,"Camera UUID maps to multiple physical cameras");
        unique[key]=t;if(!t.cameraId.isEmpty())ids[t.cameraId]=key;
    }
    std::lock_guard lock(mutex);Reservation out;
    for(auto it=owners.begin();it!=owners.end();)if(it.value().expired())it=owners.erase(it);else ++it;
    QMap<QString,Ticket> reuse;
    for(const auto& ticket:reusable)if(ticket && !ticket->diagnostic_.quarantined && ticket->diagnostic_.owner==owner && ticket->diagnostic_.use==use)
        reuse[ticket->diagnostic_.physicalKey]=ticket;
    // Check every key before publishing any reservation: no hold-and-wait/queue.
    for(const auto& t:ordered){const auto key=physicalKey(t.physical);auto expected=reuse.value(key);
        for(const auto& k:QStringList{"p:"+key,t.cameraId.isEmpty()?QString{}:"u:"+t.cameraId}){
            if(k.isEmpty())continue;auto held=owners.value(k).lock();
            if(held && (held!=expected || held->diagnostic_.cameraId!=t.cameraId))return {3,{}};
        }
    }
    for(auto it=unique.cbegin();it!=unique.cend();++it){auto ticket=reuse.value(it.key());
        if(!ticket){ticket=std::make_shared<Lease>();ticket->diagnostic_={owner,it->cameraId,it.key(),use,++generation,false,false};}
        owners["p:"+it.key()]=ticket;if(!it->cameraId.isEmpty())owners["u:"+it->cameraId]=ticket;
        out.leases.append(ticket);
    }
    return out;
}
void CameraLeaseArbiter::claim(const Ticket& ticket,const PhysicalDeviceIdentity& physical,CameraLeaseUse use,const QString& id){
    const auto key=physicalKey(physical);std::lock_guard lock(mutex);
    require(ticket && ticket->diagnostic_.physicalKey==key && ticket->diagnostic_.use==use && ticket->diagnostic_.cameraId==id
        && !ticket->diagnostic_.claimed && !ticket->diagnostic_.quarantined && owners.value("p:"+key).lock()==ticket,"Invalid/already claimed camera lease");
    ticket->diagnostic_.claimed=true;
}
void CameraLeaseArbiter::drained(const Ticket& ticket,bool clean){
    std::lock_guard lock(mutex);if(!ticket)return;
    ticket->diagnostic_.claimed=false;
    if(!clean && !ticket->diagnostic_.quarantined){ticket->diagnostic_.quarantined=true;quarantine.append(ticket);}
}
QVector<CameraLeaseDiagnostic> CameraLeaseArbiter::diagnostics(){
    std::lock_guard lock(mutex);QVector<CameraLeaseDiagnostic> out;QSet<quint64> seen;
    for(auto it=owners.begin();it!=owners.end();){auto ticket=it.value().lock();if(!ticket){it=owners.erase(it);continue;}
        if(!seen.contains(ticket->generation())){seen.insert(ticket->generation());out.append(ticket->diagnostic_);}++it;
    }return out;
}
}
