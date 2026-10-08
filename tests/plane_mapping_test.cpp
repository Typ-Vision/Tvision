#include "application/PlaneMappingService.hpp"
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
    std::cout<<"PASS planar homography analytic/region/replay contract\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
