#include "domain/Coordinates.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
using namespace visionflow;
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void near(const QPointF& actual,const QPointF& expected,const char* message) {check(std::hypot(actual.x()-expected.x(),actual.y()-expected.y())<1e-9,message);}
template<class F> void rejects(F action,const char* message) {bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,message);}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const CoordinateFrame image{"camera/image"},local{"part/local"},other{"other/image"};
        const CoordinateFrame plane{"plane",LengthUnit::Millimetre,AxisConvention::XRightYUp};
        constexpr auto pi=std::numbers::pi;
        const Transform2D t{local,image,{10,20},pi/2};
        near(t.map(Point2D{{2,3},local}).position,{7,22},"rotate then translate known geometry");
        near(t.inverse().map(t.map(Point2D{{2,3},local})).position,{2,3},"inverse maps back");
        const Transform2D shift{image,other,{5,-4},0};
        near(t.then(shift).map(Point2D{{2,3},local}).position,{12,18},"then means next times this");
        near(t.then(t.inverse()).map(Point2D{{2,3},local}).position,{2,3},"composition identity");
        const Transform2D rotateAfter{image,other,{0,0},pi/2};
        near(t.then(rotateAfter).map(Point2D{{2,3},local}).position,{-22,7},"later rotation rotates previous translation");
        const auto pose=t.map(Pose2D{{2,3},pi/2,local});
        near(pose.position,{7,22},"pose position mapping");check(std::abs(std::abs(pose.angleRadians)-pi)<1e-9,"pose angle composed");
        const CoordinateFrame robot{"robot/plane",LengthUnit::Millimetre,AxisConvention::XRightYUp};
        near(Transform2D{plane,robot,{1,2},pi/2}.map(Point2D{{2,0},plane}).position,{1,4},"mm Y-up rotation geometry");
        rejects([&]{t.map(Point2D{{1,2},image});},"wrong source frame rejected");
        rejects([&]{t.then(t);},"wrong composition order rejected");
        rejects([&]{Transform2D{image,plane,{},0}.validate();},"rigid px-to-mm conversion rejected");
        auto mmImage=image;mmImage.unit=LengthUnit::Millimetre;
        rejects([&]{t.map(Point2D{{1,2},mmImage});},"same ID different units rejected");
        auto yUp=image;yUp.axes=AxisConvention::XRightYUp;
        rejects([&]{Transform2D{image,yUp,{},0}.validate();},"implicit axis flip rejected");
        rejects([&]{CoordinateFrame{}.validate();},"empty coordinate identity rejected");
        rejects([&]{Pose2D{{0,0},std::numeric_limits<double>::infinity(),image}.validate();},"nonfinite angle rejected");
        rejects([&]{t.map(Point2D{{std::numeric_limits<double>::quiet_NaN(),0},local});},"nonfinite position rejected");
        const QVector<Point2D> corners{{{12,20},image},{{16,20},image},{{16,22},image},{{12,22},image}};
        const auto roi=TrackingRoi::teach(corners,{{10,20},0,image},local);
        const auto moved=roi.resolve({{30,40},pi/2,image});
        const QVector<QPointF> expected{{30,42},{30,46},{28,46},{28,42}};
        for(int i=0;i<4;++i){near(moved[i].position,expected[i],"ROI rotates offset and corners");check(moved[i].frame==image,"ROI destination frame");}
        const auto initial=roi.resolve({{10,20},0,image});for(int i=0;i<4;++i)near(initial[i].position,corners[i].position,"teaching roundtrip");
        const auto rotatedTeaching=TrackingRoi::teach(corners,{{10,20},pi/3,image},local);
        const auto restored=rotatedTeaching.resolve({{10,20},pi/3,image});for(int i=0;i<4;++i)near(restored[i].position,corners[i].position,"nonzero teaching angle roundtrip");
        rejects([&]{roi.resolve({{30,40},0,other});},"ROI foreign destination rejected");
        rejects([&]{roi.resolve({{30,40},0,mmImage});},"ROI incompatible unit rejected");
        rejects([&]{TrackingRoi::teach({},{{10,20},0,image},local);},"empty ROI rejected");
        auto crossed=corners;std::swap(crossed[1],crossed[2]);
        rejects([&]{TrackingRoi::teach(crossed,{{10,20},0,image},local);},"bowtie ROI rejected");
        auto invalid=roi;invalid.localCorners[1]=invalid.localCorners[0];
        rejects([&]{invalid.resolve({{10,20},0,image});},"mutated degenerate record rejected");
        const QRect box(90,48,20,24);const Point2D anchor{{27.5,27.5},image};
        check(translationTrackingRoi(box,anchor,{{57.5,42.5},image},{200,150})==QRect(120,63,20,24),"legacy translation geometry");
        check(translationTrackingRoi(box,anchor,{{28,27},image},{200,150})==QRect(91,47,20,24),"half pixel rounding away from zero");
        check(translationTrackingRoi(box,anchor,{{27.99,27.01},image},{200,150})==box,"round delta once rather than origins");
        check(translationTrackingRoi(QRect(0,0,200,150),anchor,anchor,{200,150})==QRect(0,0,200,150),"last pixel inclusive bound");
        rejects([&]{translationTrackingRoi(box,anchor,{{200,27.5},image},{200,150});},"outside ROI rejects instead of clamp");
        rejects([&]{translationTrackingRoi(box,anchor,{{27.5,27.5},other},{200,150});},"tracking mixed frames rejected");
        rejects([&]{translationTrackingRoi({},anchor,anchor,{200,150});},"missing legacy ROI rejected");
        CalibrationDependency dependency{"calibration",1,"sha256-content","camera","lens",{1920,1080},"work-plane",image,plane};
        dependency.requireCurrent(dependency);
        check(CalibrationDependency::fromJson(dependency.toJson())==dependency,"calibration dependency JSON roundtrip");
        for(int i=0;i<6;++i) {
            auto malformed=dependency.toJson();
            if(i==0)malformed["schemaVersion"]=2;
            if(i==1)malformed.remove("lensId");
            if(i==2)malformed["version"]=1.5;
            if(i==3)malformed["extra"]=true;
            if(i==4){auto f=malformed["source"].toObject();f["unit"]="inch";malformed["source"]=f;}
            if(i==5)malformed["width"]="1920";
            rejects([&]{CalibrationDependency::fromJson(malformed);},"invalid/future dependency record rejected");
        }
        for(int i=0;i<10;++i) {
            auto changed=dependency;
            switch(i) {
                case 0:changed.calibrationId="other";break;case 1:++changed.version;break;
                case 2:changed.contentId="changed";break;case 3:changed.cameraId="camera2";break;
                case 4:changed.lensId="lens2";break;case 5:changed.resolution={1280,720};break;
                case 6:changed.workingPlaneId="plane2";break;case 7:changed.source=other;break;
                case 8:changed.target=robot;break;case 9:changed.source.axes=AxisConvention::XRightYUp;break;
            }
            rejects([&]{dependency.requireCurrent(changed);},"changed calibration dependency rejected");
        }
        rejects([&]{CalibrationDependency{}.validate();},"missing calibration dependency rejected");
        QTextStream(stdout)<<"Coordinate/tracking contract passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
