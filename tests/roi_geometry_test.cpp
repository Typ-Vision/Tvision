#include "domain/ToolRoi.hpp"
#include "vision/ImageAccess.hpp"
#include "application/EditorSession.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTextStream>
#include <QJsonArray>
#include <limits>
#include <stdexcept>
using namespace visionflow;
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CoordinateFrame raw{"camera/raw"};
    QVector<ToolRoi> rois{{raw,ScanSegment{{10.25,20.5},{30.25,20.5},4}},
        {raw,RotatedRectangle{{50,50},{20,10},90}},{raw,Annulus{{50,50},10,20}},
        {raw,ArcRoi{{{50,50},10,20},0,90}},{raw,PolygonRoi{{{0,0},{99,0},{99,99},{0,99}}}},
        {raw,PathRoi{{{0.25,1.5},{50,30},{99,99}}}}};
    for(const auto& r:rois){check(r.inside({100,100}),"valid ROI fits raw pixel domain");check(ToolRoi::fromJson(r.toJson())==r,"exact subpixel geometry roundtrip");auto o=r.toJson();o["schemaVersion"]=2;reject([&]{ToolRoi::fromJson(o);},"future ROI schema");o=r.toJson();o["extra"]=1;reject([&]{ToolRoi::fromJson(o);},"unknown ROI field");}
    check(rois[0].bounds()==QRectF(10.25,18.5,20,4),"scan footprint width and direction");
    const auto rb=rois[1].bounds();check(std::abs(rb.left()-45)<1e-9 && std::abs(rb.top()-40)<1e-9 && std::abs(rb.width()-10)<1e-9 && std::abs(rb.height()-20)<1e-9,"90 degree rectangle oracle");
    check(rois[2].bounds()==QRectF(30,30,40,40),"full ring extrema");
    const auto arc=rois[3].bounds();check(std::abs(arc.left()-50)<1e-9 && std::abs(arc.top()-50)<1e-9 && std::abs(arc.right()-70)<1e-9 && std::abs(arc.bottom()-70)<1e-9,"quarter arc excludes unused quadrants");
    check(!rois[4].inside({99,100}),"pixel centre right boundary rejects instead of clips");
    ToolRoi reverse{raw,ArcRoi{{{50,50},10,20},90,-90}};check(reverse.bounds()==arc,"signed arc sweep bounds");
    reject([&]{ToolRoi{raw,ScanSegment{{1,1},{1,1},1}}.validate();},"zero segment");
    reject([&]{ToolRoi{raw,Annulus{{1,1},2,1}}.validate();},"inverted radii");
    reject([&]{ToolRoi{raw,ArcRoi{{{1,1},1,2},0,0}}.validate();},"zero sweep");
    reject([&]{ToolRoi{raw,RotatedRectangle{{1,1},{0,2},0}}.validate();},"zero rectangle");
    reject([&]{ToolRoi{raw,PolygonRoi{{{0,0},{3,3},{0,3},{3,0}}}}.validate();},"self intersecting polygon");
    reject([&]{ToolRoi{raw,PathRoi{{{0,0},{0,0}}}}.validate();},"duplicate path points");
    reject([&]{ToolRoi{raw,PathRoi{{{0,0},{std::numeric_limits<double>::quiet_NaN(),1}}}}.validate();},"nonfinite ROI");
    for(int i=0;i<4;++i){const ToolRoi world{{"world",LengthUnit::Millimetre},rois[i].geometry};check(ToolRoi::fromJson(world.toJson())==world,"world scan/rectangle/ring/arc exact schema2");reject([&]{world.inside({100,100});},"world ROI not silently raw");}
    // Padded source rows, all six formats, lifetime and ownership.
    FrameMetadata m{"camera","frame","cycle","camera/raw"};
    const auto parameterId=QCryptographicHash::hash("params",QCryptographicHash::Sha256);
    for(auto format:{PixelFormat::Gray8,PixelFormat::Gray16,PixelFormat::Bgr8,PixelFormat::Bgra8,PixelFormat::Rgb8,PixelFormat::Rgba8}) {
        const int bpp=format==PixelFormat::Gray8 ? 1:format==PixelFormat::Gray16 ? 2:(format==PixelFormat::Bgr8 || format==PixelFormat::Rgb8) ? 3:4;
        const int stride=3*bpp+6;QByteArray bytes(stride*2,char(7));
        auto input=ImageFrame::fromBuffer(bytes.data(),bytes.size(),3,2,stride,format,m,false);const auto hash=input.contentId();bytes.fill(0);
        const ImageAccess access(input);check(access.row(0).size()==std::size_t(3*bpp) && access.row(1)[0]==std::byte{7},"owned normalized row excludes padding");
        auto copy=access.ownedCopy();copy.setTo(cv::Scalar::all(42));check(input.contentId()==hash,"mutable copy cannot alter raw input");
        auto processed=ImageAccess::processed(input,copy,input.format(),"test-transform",1,parameterId,false);copy.setTo(cv::Scalar::all(99));
        check(processed.metadata().frameId==m.frameId && processed.metadata().coordinateSystemId==m.coordinateSystemId && processed.processing().size()==1 && processed.processing().first().inputContentId==hash,"processing preserves raw origin and records input identity");
        check(ImageAccess(processed).row(0)[0]==std::byte{42} && input.contentId()==hash,"processed output owns separate bytes");
        check(processed.geometry()==input.geometry(),"same-size processing preserves sealed geometry");
        auto noChange=ImageAccess::processed(input,access.ownedCopy(),input.format(),"identity",1,parameterId,false);
        check(noChange.contentId()==hash && !noChange.processingId().isEmpty() && input.processingId().isEmpty(),"provenance independent of identical pixel hash");
        reject([&]{access.row(2);},"row boundary");
        reject([&]{ImageAccess::processed(input,cv::Mat(1,1,copy.type()),input.format(),"crop",1,parameterId);},"hidden crop/resize rejected");
        reject([&]{ImageAccess::processed(input,access.ownedCopy(),input.format(),"x",1,{});},"missing parameters identity rejected");
        EditorSession session;EditorContext context{"project","path","recipe",{}, {}};session.frame=input;const auto scope=session.evaluationScope(context);session.frame=noChange;check(session.evaluationScope(context)!=scope,"same pixels different processing invalidates cache scope");
        for(int i=1;i<32;++i)noChange=ImageAccess::processed(noChange,access.ownedCopy(),input.format(),"identity",1,parameterId,false);
        reject([&]{ImageAccess::processed(noChange,access.ownedCopy(),input.format(),"identity",1,parameterId,false);},"bounded processing chain");
    }
    QTextStream(stdout)<<checks<<" ROI/image checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<"\n";return 1;}}
