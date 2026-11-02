#include "StationSourceSetup.hpp"
#include <QFile>
#include <QFileInfo>
#include <opencv2/imgcodecs.hpp>
#include <limits>
#include <stdexcept>
namespace visionflow {
namespace {void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}}
QSize StationSourceSetup::inspectImage(const QString& path){
    QFile f(path);need(QFileInfo(path).isAbsolute() && f.open(QIODevice::ReadOnly) && f.size()>0 && f.size()<=64*1024*1024,"Goruntu okunamadi (mutlak yol, en fazla 64 MiB).");
    const auto bytes=f.readAll();const auto image=cv::imdecode(std::vector<uchar>(bytes.begin(),bytes.end()),cv::IMREAD_UNCHANGED);
    need(!image.empty() && image.cols>0 && image.rows>0,"Dosya gecerli bir goruntu degil.");return {image.cols,image.rows};
}
StationFileSource StationSourceSetup::save(const QUuid& expected,const QUuid& station,const QString& name,const QUuid& camera,const QString& cameraName,const QString& path){
    inspectImage(path);auto c=store_.load();const auto original=c;need(c.revision==expected,"Katalog degisti; kaynak ayarlarini yeniden acin.");
    need(!station.isNull() && !camera.isNull(),"Istasyon ve kamera secin.");
    auto ensure=[&](CatalogueCategory category,const QUuid& id,const QString& title){
        for(auto& e:c.entries)if(e.identity.category==category && e.identity.entity==id){need(!e.retired,"Kimlik kullanim disi.");if(category==CatalogueCategory::Station)e.name=title;return;}
        need(c.nextKey<=std::numeric_limits<quint32>::max(),"Katalog anahtarlari tukendi.");
        c.entries.append({quint32(c.nextKey++),{category,id,{},{}},title,false});
    };
    ensure(CatalogueCategory::Station,station,name.trimmed());ensure(CatalogueCategory::Camera,camera,cameraName.trimmed());
    // A logical camera is exclusive to one station in the first product release.
    for(const auto& s:c.stationSources)need(s.station==station || s.camera!=camera,"Kamera baska bir istasyona bagli.");
    StationFileSource source{station,camera,QUuid::createUuid(),QFileInfo(path).absoluteFilePath()};bool found=false;
    for(auto& s:c.stationSources)if(s.station==station){need(s.camera==camera,"Kayitli kamera kimligi degistirilemez.");if(s.path==source.path)source.revision=s.revision;s=source;found=true;break;}
    if(!found)c.stationSources.append(source);
    if(c==original)return source;
    c.revision=QUuid::createUuid();validateIdentityCatalogue(c);store_.compareExchange(expected,c);return source;
}
}
