#include "SourceStatusPresenter.hpp"
#include <QJsonDocument>
namespace visionflow {
QString acquisitionStatus(const QString& state,const QString& diagnostic){
    if(state=="SKIPPED")return "İptal edildi • sonuç yok";
    if(state=="DONE")return "Kare edinildi";
    if(state!="ERROR")return "Hazırlık • kare yok";
    const auto text=diagnostic.toLower();
    if(text.contains("unsupported") || text.contains("desteklenm"))return "Desteklenmiyor";
    if(text.contains("busy") || text.contains("lease held"))return "Meşgul";
    if(text.contains("timeout") || text.contains("deadline") || text.contains("expired"))return "Zaman aşımı";
    if(text.contains("unavailable") || text.contains("native") || text.contains("kullanılam"))return "Kullanılamıyor";
    return "Edinim hatası";
}
IndustrialSourcePresentation presentIndustrialSource(const QJsonObject& reference,const QJsonObject& admission,const QJsonObject& acquisition){
    const auto ref=admission.isEmpty() ? reference:admission["reference"].toObject();
    if(ref.isEmpty())return {"Industrial Camera • profil bağlanmadı","Kurulum / profil seçilmedi","Beyan • gerçek kare yok"};
    const auto physical=admission.isEmpty() ? ref["physical"].toObject():admission["physical"].toObject();
    IndustrialSourcePresentation out;
    out.device=physical["vendor"].toString()+" / "+physical["model"].toString()+"\nSeri: "+physical["serial"].toString();
    out.configuration=QString("%1 r%2 / %3 r%4").arg(ref["setupId"].toString()).arg(ref["setupRevision"].toInt()).arg(ref["profileId"].toString()).arg(ref["profileRevision"].toInt());
    const auto setup=admission["setup"].toObject(),profile=admission["profile"].toObject();
    out.detail=out.device+"\nKamera: "+ref["cameraId"].toString()+"\nKurulum / profil: "+out.configuration;
    if(!setup.isEmpty())out.detail+="\nKurulum: "+setup["name"].toString()+" • Profil: "+profile["name"].toString();
    if(acquisition.isEmpty())out.detail+="\nHazırlık / beyan • gerçek kare yok";
    else if(acquisition["status"]=="ACQUIRED")out.detail+="\nEdinildi • Frame: "+acquisition["execution"].toObject()["frameId"].toString()+"\nPiksel SHA256: "+acquisition["pixelHash"].toString()+"\nActual profil / geometri: "+QString::fromUtf8(QJsonDocument(acquisition["actual"].toObject()).toJson(QJsonDocument::Compact));
    else out.detail+="\nKare alınmadı • "+acquisition["reason"].toString()+" • "+acquisition["errorCode"].toString();
    if(!admission.isEmpty())out.detail+="\nBackend / SDK kanıtı: "+QString::fromUtf8(QJsonDocument(admission["backend"].toObject()).toJson(QJsonDocument::Compact))+"\nProducer / rota: "+QString::fromUtf8(QJsonDocument(admission["route"].toObject()).toJson(QJsonDocument::Compact));
    return out;
}
}
