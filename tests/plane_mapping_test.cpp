#include "application/PlaneMappingService.hpp"
#include "application/ActiveCalibrationService.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QLineF>
#include <iostream>
#include <stdexcept>
using namespace visionflow;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
QPointF imagePoint(QPointF q){double w=0.001*q.x()+0.002*q.y()+1;return {(5*q.x()+0.8*q.y()+100)/w,(0.2*q.x()+6*q.y()+70)/w};}
ChessboardObservation observation(){ChessboardObservation o{{"analytic-board",9,6,10},{"test-source","fit-frame","fit-cycle","image",QDateTime::currentDateTimeUtc()},{700,520},{},{}};for(int r=0;r<6;++r)for(int c=0;c<9;++c){QPointF q{c*10.0,r*10.0};o.worldMillimetres.append(q);o.pixels.append(imagePoint(q));}return o;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto o=observation();const auto m=PlaneMappingService::fit(o);
    const QPointF a{13.25,11.5},b{64.5,38.25};const auto d=PlaneMappingService::distance(m,imagePoint(a),imagePoint(b));
    check(QLineF(d.firstMillimetres,a).length()<1e-5 && QLineF(d.secondMillimetres,b).length()<1e-5 && std::abs(d.millimetres-QLineF(a,b).length())<1e-5,"independent known perspective coordinates/distance");
    check(m.fitRmsMillimetres()<1e-5 && m.inverseRmsPixels()<1e-4,"fit statistics separate");
    const auto json=PlaneMappingService::encode(m);const auto replay=PlaneMappingService::decode(json);check(PlaneMappingService::encode(replay)==json,"exact evidence replay");
    auto broken=json;auto h=broken["H"].toArray();h[0]=h[0].toDouble()+0.01;broken["H"]=h;rejects([&]{PlaneMappingService::decode(broken);});broken=json;broken["coordinateSpace"]="undistorted_pixels";rejects([&]{PlaneMappingService::decode(broken);});
    rejects([&]{PlaneMappingService::map(m,{0,0});});rejects([&]{PlaneMappingService::map(m,{699,519});});rejects([&]{PlaneMappingService::map(m,{NAN,20});});
    auto bad=o;std::swap(bad.pixels[1],bad.pixels[2]);rejects([&]{PlaneMappingService::fit(bad);});bad=o;for(int i=0;i<bad.pixels.size();++i)bad.pixels[i]={double(i+10),20};rejects([&]{PlaneMappingService::fit(bad);});bad=o;bad.pixels[1]=bad.pixels[0];rejects([&]{PlaneMappingService::fit(bad);});bad=o;bad.worldMillimetres[1]={99,0};rejects([&]{PlaneMappingService::fit(bad);});
    bad=o;bad.pixels[20]+=QPointF(10,0);rejects([&]{PlaneMappingService::fit(bad,0.1);});bad=o;bad.source.coordinateSystemId="normalized";rejects([&]{PlaneMappingService::fit(bad);});
    auto fg=o;fg.target={"framed-model",7,7,12,CalibrationGridPattern::FramedCircles,FramedPlateGeometry{}};fg.pixelHash=QString(64,'a');fg.pixels.clear();fg.worldMillimetres.clear();QVector<PlateCentre> centres;
    for(int r=0;r<7;++r)for(int c=0;c<7;++c){QPointF w{12.*c,12.*r};auto p=imagePoint(w);fg.worldMillimetres.append(w);fg.pixels.append(p);centres.append({r,c,p,.1});}
    const auto raw=RawPlateObservation::create(fg,centres,{PlateOrientation::TriangleCorner,0,false,QPointF(80,50)},"analytic/1");
    const auto fm=PlaneMappingService::fit(raw);const auto fj=PlaneMappingService::encode(fm);
    check(fj["schemaVersion"]==3 && fj["observationId"].toString().size()==64 && PlaneMappingService::encode(PlaneMappingService::decode(fj))==fj,"framed v3 identity and exact replay");
    rejects([&]{PlaneMappingService::fit(fg);});rejects([&]{PlaneMappingService::fit(raw,1.01);});
    for(int n=0;n<12;++n){auto j=fj;auto e=j["observation"].toObject();auto direction=e["orientation"].toObject();
        if(n==0)j["schemaVersion"]=4;if(n==1)j["extra"]=true;if(n==2)j["pixelHash"]=QString(64,'b');
        if(n==3){auto t=j["target"].toObject();t["spacingMm"]=13;j["target"]=t;}
        if(n==4){auto source=j["source"].toObject();source["sourceId"]="other";j["source"]=source;}
        if(n==5)e["coordinateSpace"]="undistorted/1";if(n==6)direction["mirrored"]=true;if(n==7)direction["triangle"]=QJsonArray{600,500};
        if(n==8){auto cs=e["centres"].toArray();auto c=cs[1].toObject();c["column"]=0;cs[1]=c;e["centres"]=cs;}
        if(n==9)e["acquisition"]=QJsonObject{{"originY",1}};if(n==10)j["observationId"]=QString(64,'f');if(n==11)e["schemaVersion"]=2;
        e["orientation"]=direction;j["observation"]=e;rejects([&]{PlaneMappingService::decode(j);});}
    MeasurementSetup setup{"bench","image_file",QString(64,'a'),"lens","plane",fg.resolution,{"image"},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
    CalibrationDependency dep{"framed",1,"pending",fg.source.sourceId,setup.lensId,setup.resolution,setup.planeId,setup.source,setup.target};
    const auto model=MeasurementService{}.teachHomography(dep,fm);CalibrationImageEvidence image{"setup_file",fg.source.sourceId,fg.source.frameId,fg.pixelHash,{}};
    const auto record=CalibrationModelRecord::create(model,setup,image);check(CalibrationModelRecord::decode(record.encode()).encode()==record.encode() && !record.controlAccepted(),"framed record replay without invented physical acceptance");
    image.pixelHash=QString(64,'b');rejects([&]{CalibrationModelRecord::create(model,setup,image);});
    auto foreign=fg;foreign.source.experimentalAcquisition={{"originY",1}};
    auto foreignRaw=RawPlateObservation::create(foreign,centres,raw.orientation(),"analytic/1");
    const auto foreignModel=MeasurementService{}.teachHomography(dep,PlaneMappingService::fit(foreignRaw));
    rejects([&]{CalibrationModelRecord::create(foreignModel,setup,record.image());});
    CalibrationControl control;control.samples.append({"fit-is-not-control",{fg.pixels[0],setup.source},{fg.pixels[1],setup.source},12});rejects([&]{CalibrationModelRecord::create(model,setup,record.image(),control);});
    std::cout<<"PASS planar homography analytic/region/replay contract\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
