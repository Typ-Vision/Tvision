#include "application/MeasurementService.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimeZone>
#include <iostream>
#include <limits>
#include <cmath>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void reject(F action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
QByteArray digest(const QJsonObject& o){return QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex();}
ToolResult edge(QPointF p,const CalibrationDependency& d){FrameMetadata m{d.cameraId,"frame","cycle",d.source.id,QDateTime::fromMSecsSinceEpoch(1000,QTimeZone::UTC)};ToolResult r;r.state=ExecutionState::Done;r.outputs["point"]={{"point",PortType::Point,"px","image"},p,true,m.frameId,m.cycleId,m};return r;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    MeasurementService math;
    CalibrationDependency setup{"scale",1,"ignored","camera-source","lens",{1000,800},"plane",{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
    ManualScaleProvenance provenance{"acquired",setup.cameraId,"calibration-frame",setup.resolution};
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,setup.source},{b,setup.source},mm};};
    const QVector<CalibrationSample> holdout{sample("horizontal",{10,30},{210,30},10),sample("vertical",{40,10},{40,110},5)};
    const auto candidate=math.manualScale(setup,20,provenance);
    check(candidate.pixelsPerMillimetre()==20 && candidate.millimetresPerPixel()==.05 && !candidate.mapping() && !candidate.validated(),"manual candidate and reciprocal");
    check(math.trialDistance(candidate,{{0,0},setup.source},{{200,0},setup.source})==10,"200 pixels is 10 mm candidate preview");
    check(math.trialDistance(candidate,{{.25,.5},setup.source},{{60.25,80.5},setup.source})==5,"subpixel diagonal preview");
    check(math.trialDistance(math.manualScale(setup,40,provenance),{{0,0},setup.source},{{200,0},setup.source})==5,"different coefficient");
    reject([&]{math.trialDistance(candidate,{{-1,0},setup.source},{{20,0},setup.source});});
    auto wrongFrame=setup.source;wrongFrame.unit=LengthUnit::Millimetre;reject([&]{math.trialDistance(candidate,{{0,0},wrongFrame},{{20,0},setup.source});});
    for(double value:{0.,-1.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),std::numeric_limits<double>::denorm_min()})reject([&]{math.manualScale(setup,value,provenance);});
    const auto huge=math.manualScale(setup,std::numeric_limits<double>::max(),provenance);check(huge.millimetresPerPixel()>0 && math.encode(math.decode(math.encode(huge)))==math.encode(huge),"finite reciprocal lower boundary replay");
    reject([&]{math.trialDistance(huge,{{0,0},setup.source},{{std::numeric_limits<double>::denorm_min(),0},setup.source});});
    for(int i=0;i<5;++i){auto p=provenance;if(i==0)p.origin="acquisition";if(i==1)p.sourceId="foreign";if(i==2)p.frameId=" ";if(i==3)p.resolution={640,480};if(i==4)p.origin="file_test";reject([&]{math.manualScale(setup,20,p);});}
    auto invalidSetup=setup;invalidSetup.modelVersion=2;reject([&]{math.manualScale(invalidSetup,20,provenance);});
    invalidSetup=setup;invalidSetup.target.unit=LengthUnit::Pixel;reject([&]{math.manualScale(invalidSetup,20,provenance);});
    reject([&]{math.validate(candidate,{},0);});reject([&]{math.validate(candidate,{holdout[0]},0);});
    for(int i=0;i<7;++i){auto h=holdout;if(i==0)h[1].id=h[0].id;if(i==1){h[1].first=h[0].second;h[1].second=h[0].first;}if(i==2)h[0].knownMillimetres=0;if(i==3)h[0].second=h[0].first;if(i==4)h[0].first.frame=wrongFrame;if(i==5)h[0].second.position={1000,0};if(i==6)h[0].knownMillimetres=11;reject([&]{math.validate(candidate,h,0);});}
    auto exact=holdout;exact[0].knownMillimetres=10.5;const auto boundary=math.validate(candidate,exact,.5);check(boundary.maxValidationErrorMillimetres()==.5,"inclusive residual boundary");reject([&]{math.validate(candidate,exact,std::nextafter(.5,0.));});
    QVector<CalibrationSample> many;for(int i=0;i<256;++i)many.append(sample(QString::number(i),{.25,double(i)},{20.25,double(i)},1));
    check(math.validate(candidate,many,0).validated(),"256 holdouts accepted");many.append(sample("257",{.25,300},{20.25,300},1));reject([&]{math.validate(candidate,many,0);});
    const auto accepted=math.validate(candidate,holdout,0);check(accepted.validated() && accepted.validationRmsMillimetres()==0 && accepted.measuredHoldoutMillimetres()==QVector<double>{10,5},"actual independent evidence");
    check(!candidate.validated() && accepted.pixelsPerMillimetre()==20 && accepted.dependency()!=candidate.dependency(),"validation immutable and never refits");
    reject([&]{math.validate(accepted,exact,.49);});check(accepted.validated() && accepted.validationRmsMillimetres()==0,"failed validation never mutates accepted value");
    auto changedProvenance=provenance;changedProvenance.frameId="new-frame";check(math.manualScale(setup,20,changedProvenance).dependency()!=candidate.dependency(),"provenance participates in identity");
    check(math.validate(candidate,holdout,.001).dependency()!=accepted.dependency(),"validation policy participates in identity");
    check(!math.manualScale(setup,21,provenance).validated(),"changed coefficient requires new validation");
    for(const auto& c:{candidate,accepted,boundary}){auto json=math.encode(c);const auto disk=QJsonDocument::fromJson(QJsonDocument(json).toJson()).object();check(json["schemaVersion"].toInt()==3 && !json.contains("teaching") && math.encode(math.decode(disk))==json,"manual record JSON replay including candidate");check(CalibrationDependency::fromJson(c.dependency().toJson())==c.dependency() && c.dependency().toJson()["schemaVersion"].toInt()==3,"dependency schema 3 replay");}
    for(int i=0;i<5;++i){auto d=accepted.dependency().toJson();if(i==0)d["schemaVersion"]=4;if(i==1)d["modelVersion"]=1.5;if(i==2)d["modelVersion"]="1";if(i==3)d.remove("modelType");if(i==4)d["extra"]=true;reject([&]{CalibrationDependency::fromJson(d);});}
    const auto good=math.encode(accepted);
    for(int i=0;i<21;++i){auto j=good;auto d=j["dependency"].toObject();auto p=j["provenance"].toObject();
        if(i==0)j["schemaVersion"]=4;if(i==1)j["schemaVersion"]=2;if(i==2)j["model"]="manual_planar_scale/2";if(i==3)j["pixelsPerMillimetre"]=21;if(i==4)j["millimetresPerPixel"]=.051;if(i==5)j["validated"]=false;if(i==6)j["rmsMm"]=1;if(i==7)j["maxResidualMm"]=1;if(i==8)j["maxErrorMm"]=1;if(i==9)j["extra"]=true;if(i==10)j.remove("holdout");if(i==11)j["pixelsPerMillimetre"]="20";if(i==12)d["contentId"]="forged";if(i==13)d["modelVersion"]=2;if(i==14)d["schemaVersion"]=2;if(i==15)d["modelType"]="raw_planar_homography";if(i==16)p["origin"]="file_test";if(i==17)p["frameId"]="changed";if(i==18)p["width"]=1000.5;if(i==19)p["extra"]=true;if(i==20){auto h=j["holdout"].toArray();auto s=h[0].toObject();s["knownMm"]=11;h[0]=s;j["holdout"]=h;}
        j["dependency"]=d;j["provenance"]=p;reject([&]{math.decode(j);});
    }
    auto forged=math.encode(candidate);forged["validated"]=true;reject([&]{math.decode(forged);});forged=math.encode(candidate);forged["maxResidualMm"]=1;reject([&]{math.decode(forged);});
    auto a=edge({0,0},accepted.dependency()),b=edge({200,0},accepted.dependency());
    auto result=math.distance(a,"point",b,"point",candidate,candidate.dependency(),setup.resolution,10,10);check(!result.millimetres && result.decision==PartDecision::NotEvaluated,"candidate cannot produce production result");
    result=math.distance(a,"point",b,"point",accepted,accepted.dependency(),setup.resolution,10,10);check(result.millimetres==10 && result.decision==PartDecision::Pass,"accepted exact raw tolerance");
    // Numerical overflow must not publish a trial or acceptance.
    const auto tiny=math.manualScale(setup,std::numeric_limits<double>::min(),provenance);
    reject([&]{math.trialDistance(tiny,{{0,0},setup.source},{{999,799},setup.source});});reject([&]{math.validate(tiny,holdout,std::numeric_limits<double>::max());});
    auto testSetup=setup;testSetup.cameraId="calibration-test:fixture";auto testProvenance=provenance;testProvenance.origin="file_test";testProvenance.sourceId=testSetup.cameraId;
    const auto test=math.validate(math.manualScale(testSetup,20,testProvenance),holdout,0);check(math.encode(math.decode(math.encode(test)))==math.encode(test),"file test origin replay");
    result=math.distance(edge({0,0},test.dependency()),"point",edge({200,0},test.dependency()),"point",test,test.dependency(),setup.resolution,10,10);check(!result.millimetres && result.decision==PartDecision::NotEvaluated,"file test cannot produce production result");
    // Fixed legacy inputs pin complete serialized records as well as content identity.
    const auto legacy=math.validate(math.teach(setup,sample("teach",{0,0},{400,0},20)),holdout,0);
    ChessboardObservation o{{"board",3,3,10},{setup.cameraId,"board-frame","board-cycle",setup.source.id,QDateTime::fromMSecsSinceEpoch(1000,QTimeZone::UTC)},setup.resolution,{}, {}};
    for(int y=0;y<3;++y)for(int x=0;x<3;++x){o.pixels.append({100.+x*100,100.+y*100});o.worldMillimetres.append({x*10.,y*10.});}
    const auto homography=math.validate(math.teachHomography(setup,PlaneMappingService::fit(o)),{sample("x",{125,125},{225,125},10),sample("y",{150,150},{150,250},10)},.0001);
    check(legacy.dependency().contentId=="2bd1ee1397d8756bab5a4da25a4de5bf0ef9bd97e6b7de9edcd8488dc02494a1" && digest(math.encode(legacy))=="5ddbd6575eb1aec50a3cf8f90d5e0a9e4cb9a72b21188e81d4ede175bcce6c87","legacy schema 1 hash and complete encoding frozen");
    check(homography.dependency().contentId=="9579e72f77c78447164d4b68d04a61ec793fdef75985a7cf0d9a0d7e1dbe73bd" && digest(math.encode(homography))=="74c522aed628cab00d6132319620205b2a204958380904fcf1a328348c5279b4","homography schema 2 hash and complete encoding frozen");
    for(const auto& c:{legacy,homography})check(math.encode(math.decode(math.encode(c)))==math.encode(c),"legacy codec exact replay");
    std::cout<<"PASS manual scale math / evidence / strict codec / legacy records\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
