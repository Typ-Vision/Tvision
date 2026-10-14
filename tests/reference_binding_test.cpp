#include "domain/ReferenceBinding.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
void near(QPointF p,QPointF expected){check(QLineF(p,expected).length()<1e-9,"Independent coordinate oracle mismatch");}
template<class F> void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Expected strict rejection");}
constexpr double pi=std::numbers::pi;
const CoordinateFrame raw{"camera/raw"};
ReferenceBinding binding(bool rigid=false){
    ReferenceBinding b;b.mode=rigid ? ReferenceMode::Rigid:ReferenceMode::Translation;
    b.sourceTool="source";b.sourcePort=rigid ? "pose":"point";b.geometryId=QString(64,'a');b.imageSource="camera";
    b.nominal={raw,ScanSegment{{140,100},{150,100},2}};b.rawSize={400,400};
    if(rigid)b.taught=Pose2D{{100,100},0,raw};else b.taught=Point2D{{100,100},raw};return b;
}
QPointF start(const ToolRoi& r){return std::get<ScanSegment>(r.geometry).start;}
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        auto p=binding();p.offsetPx={5,-2};
        const Point2D now{{120,110},raw};
        near(start(p.resolve(now,1,p.rawSize)),{165,108});
        auto r=binding(true);r.offsetPx={5,-2};const Pose2D turned{{120,110},pi/2,raw};
        near(start(r.resolve(turned,1,r.rawSize)),{122,155});
        r.angleOffsetRadians=pi/2;near(start(r.resolve(turned,1,r.rawSize)),{82,115});
        near(start(r.resolve(turned,1,r.rawSize)),{82,115}); // repeat must not accumulate
        check(std::get<ScanSegment>(r.nominal.geometry).start==QPointF(140,100),"Base geometry preserved");
        r.angleOffsetRadians=0;
        near(start(r.resolve(Pose2D{{120,110},-pi/2,raw},1,r.rawSize)),{118,65});
        near(start(r.resolve(Pose2D{{120,110},5*pi/2,raw},1,r.rawSize)),{122,155});
        near(start(r.resolve(Pose2D{{120,110},0,raw},1,r.rawSize)),{165,108});
        r.taught=Pose2D{{100,100},pi/2,raw};r.offsetPx={};
        near(start(r.resolve(Pose2D{{120,110},0,raw},1,r.rawSize)),{120,70});
        near(start(r.resolve(r.taught,1,r.rawSize)),{140,100});
        p.offsetPx={.125,-.375};near(start(p.resolve(Point2D{{120.25,110.5},raw},1,p.rawSize)),{160.375,110.125});
        const QVector<RoiGeometry> geometries{
            ScanSegment{{140,100},{150,100},2},RotatedRectangle{{140,100},{10,6},15},
            Annulus{{140,100},2,5},ArcRoi{{{140,100},2,5},20,-80},
            PolygonRoi{{{140,100},{150,100},{140,110}}},PathRoi{{{140,100},{150,100},{145,110}}}};
        for(const auto& g:geometries)for(bool rigid:{false,true}){
            auto b=binding(rigid);b.nominal.geometry=g;
            const auto restored=b.resolve(b.taught,1,b.rawSize);
            check(restored.toJson()==b.nominal.toJson(),"Zero offset identity for six shapes");
            check(ReferenceBinding::fromJson(b.toJson()).toJson()==b.toJson(),"Strict codec roundtrip for six shapes");
            b.offsetPx={5,-2};
            const ReferenceAnchor current=rigid ? ReferenceAnchor(turned):ReferenceAnchor(now);
            const auto moved=b.resolve(current,1,b.rawSize);check(moved.geometry.index()==g.index(),"No AABB conversion");
            std::visit([&](const auto& shape){using T=std::decay_t<decltype(shape)>;
                if constexpr(std::is_same_v<T,ScanSegment>){near(shape.start,rigid ? QPointF(122,155):QPointF(165,108));near(shape.end,rigid ? QPointF(122,165):QPointF(175,108));check(shape.width==2,"Scan width preserved");}
                else if constexpr(std::is_same_v<T,RotatedRectangle>){near(shape.centre,rigid ? QPointF(122,155):QPointF(165,108));check(shape.size==QSizeF(10,6) && std::abs(shape.angleDegrees-(rigid ? 105:15))<1e-9,"Rectangle size/orientation");}
                else if constexpr(std::is_same_v<T,Annulus>){near(shape.centre,rigid ? QPointF(122,155):QPointF(165,108));check(shape.innerRadius==2 && shape.outerRadius==5,"Ring radii preserved");}
                else if constexpr(std::is_same_v<T,ArcRoi>){near(shape.ring.centre,rigid ? QPointF(122,155):QPointF(165,108));check(shape.sweepDegrees==-80 && std::abs(shape.startDegrees-(rigid ? 110:20))<1e-9,"Signed arc orientation preserved");}
                else {near(shape.vertices[0],rigid ? QPointF(122,155):QPointF(165,108));near(shape.vertices[1],rigid ? QPointF(122,165):QPointF(175,108));}
            },moved.geometry);
            // Re-teaching uses a new unoffset base, never last resolved geometry.
            b.taught=current;near(b.rawTransform(current,1,b.rawSize).map(Point2D{{140,100},raw}).position,rigid ? QPointF(142,105):QPointF(145,98));
        }
        auto encoded=binding().toJson();
        for(int i=0;i<20;++i){auto bad=encoded;auto d=bad["offset"].toObject();auto t=bad["taught"].toObject();
            switch(i){
                case 0:bad["schemaVersion"]=2;break;case 1:bad["extra"]=true;break;
                case 2:bad.remove("sourcePort");break;case 3:bad["mode"]="automatic";break;
                case 4:bad["width"]=400.5;break;case 5:bad["height"]="400";break;
                case 6:bad["sourceTool"]=false;break;case 7:bad["roiIdentity"]=QString(64,'b');break;
                case 8:d["angleRadians"]=.01;break;case 9:d["unit"]="mm";break;
                case 10:d["axes"]="source-local";break;case 11:d["x"]="5";break;
                case 12:d["extra"]=0;break;case 13:d.remove("y");break;
                case 14:t["angleRadians"]=0;break;case 15:t["x"]=-1;break;
                case 16:bad["geometryId"]="not-hash";break;case 17:bad["mapping"]="future";break;
                case 18:bad["calibration"]=false;break;case 19:bad["sourcePort"]=" ";break;
            }bad["offset"]=d;bad["taught"]=t;rejects([&]{ReferenceBinding::fromJson(bad);});
        }
        auto rigidJson=binding(true).toJson();auto wrongMode=rigidJson;wrongMode["mode"]="translation";rejects([&]{ReferenceBinding::fromJson(wrongMode);});
        for(auto n:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),1e300}){
            auto b=binding();b.offsetPx={n,0};rejects([&]{b.validate();});
            b=binding(true);b.angleOffsetRadians=n;
            if(std::isfinite(n))check(b.resolve(b.taught,1,b.rawSize).inside(b.rawSize),"Huge finite angle normalized without overflow");else rejects([&]{b.validate();});
        }
        p=binding();rejects([&]{p.resolve(turned,1,p.rawSize);});
        r=binding(true);rejects([&]{r.resolve(now,1,r.rawSize);});
        rejects([&]{p.resolve(now,1.001,p.rawSize);});rejects([&]{p.resolve(now,1,{401,400});});
        rejects([&]{p.resolve(Point2D{{399,399},raw},1,p.rawSize);});
        rejects([&]{p.resolve(Point2D{{120,110},{"other"}},1,p.rawSize);});
        rejects([&]{p.resolve(Point2D{{-1,110},raw},1,p.rawSize);});
        auto invalid=p;invalid.mode=ReferenceMode::Rigid;rejects([&]{invalid.validate();});
        invalid=p;invalid.rawSize={5000,5000};rejects([&]{invalid.validate();});
        invalid=p;invalid.nominal.frame.unit=LengthUnit::Millimetre;rejects([&]{invalid.validate();});
        const CoordinateFrame plane{"plane",LengthUnit::Millimetre,AxisConvention::XRightYUp};
        p.calibration=CalibrationDependency{"cal",1,"content","camera","lens",p.rawSize,"plane",raw,plane};
        check(ReferenceBinding::fromJson(p.toJson()).toJson()==p.toJson(),"Calibration dependency preserved");
        p.calibration->resolution={20,20};rejects([&]{p.validate();});
        // Legacy strict schema/rounding remain independent of the new record.
        const auto old=PoseRoi::teach(binding(true).nominal,{{100,100},0,raw},{"local"},"source",QString(64,'a'),{400,400},"camera");
        check(PoseRoi::fromJson(old.toJson()).toJson()==old.toJson(),"Legacy binding codec stable");
        check(old.toJson()["schemaVersion"]==1 && !old.toJson().contains("offset"),"No legacy record expansion");
        check(translationTrackingRoi({90,48,20,24},{{27.5,27.5},raw},{{28,27},raw},{400,400})==QRect(91,47,20,24),"Legacy integer rounding stable");
        QTextStream(stdout)<<"Reference binding contract: "<<checks<<" checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}
}
