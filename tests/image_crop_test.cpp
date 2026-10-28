#include "vision/ImageAccess.hpp"
#include "vision/Preprocessing.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QTextStream>
#include <QTimeZone>
#include <cstring>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
FrameMetadata metadata() {
    FrameMetadata m{"source","frame","cycle","source/raw"};
    m.completedAtUtc=QDateTime::fromMSecsSinceEpoch(1234567,QTimeZone::UTC);
    m.exposureAtUtc=QDateTime::fromMSecsSinceEpoch(1234500,QTimeZone::UTC);
    m.deviceTimestampTicks=987654321;m.deviceClockId="device";
    m.experimentalAcquisition={{"evidence","unverified"}};return m;
}
ImageFrame gray(QSize size,uchar value=7) {
    cv::Mat pixels(size.height(),size.width(),CV_8UC1,cv::Scalar(value));
    return ImageFrame::fromBuffer(pixels.data,pixels.total(),pixels.cols,pixels.rows,pixels.step,PixelFormat::Gray8,metadata(),false);
}
void sameOrigin(const FrameMetadata& child,const FrameMetadata& parent) {
    check(child.sourceId==parent.sourceId && child.frameId==parent.frameId && child.cycleId==parent.cycleId &&
        child.completedAtUtc==parent.completedAtUtc && child.exposureAtUtc==parent.exposureAtUtc &&
        child.deviceTimestampTicks==parent.deviceTimestampTicks && child.deviceClockId==parent.deviceClockId &&
        child.experimentalAcquisition==parent.experimentalAcquisition,"crop preserves complete acquisition metadata");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    // Independent byte oracle, including RGB normalization exactly once and padded rows.
    for(auto format:{PixelFormat::Gray8,PixelFormat::Gray16,PixelFormat::Bgr8,PixelFormat::Bgra8,PixelFormat::Rgb8,PixelFormat::Rgba8}) {
        const bool rgb=format==PixelFormat::Rgb8 || format==PixelFormat::Rgba8;
        const int bpp=format==PixelFormat::Gray8 ? 1:format==PixelFormat::Gray16 ? 2:(format==PixelFormat::Bgr8 || format==PixelFormat::Rgb8) ? 3:4;
        const int stride=5*bpp+10;QByteArray bytes(stride*4,char(0xee)),expected(3*2*bpp,0);
        for(int y=0;y<4;++y)for(int x=0;x<5;++x)for(int c=0;c<bpp;++c)bytes[y*stride+x*bpp+c]=char(10+y*40+x*5+c);
        for(int y=0;y<2;++y)for(int x=0;x<3;++x)for(int c=0;c<bpp;++c)
            expected[(y*3+x)*bpp+c]=bytes[(y+1)*stride+(x+1)*bpp+(rgb && c<3 ? 2-c:c)];
        ImageFrame child;QImage expectedPreview;QByteArray parentId;
        {
            const auto parent=ImageFrame::fromBuffer(bytes.constData(),bytes.size(),5,4,stride,format,metadata());
            parentId=parent.contentId();expectedPreview=parent.preview().copy(1,1,3,2);
            child=ImageAccess::cropped(parent,{1,1,3,2});bytes.fill(0);
            check(ImageAccess(child).row(0).data()!=ImageAccess(parent).row(1).data()+bpp,"crop owns separate allocation rather than a parent view");
            check(child.size()==QSize(3,2) && child.format()==parent.format(),"crop size and normalized format");
            sameOrigin(child.metadata(),parent.metadata());
            check(parent.contentId()==parentId && parent.geometry().cropDepth()==0,"parent unchanged");
        }
        const ImageAccess retained(child);
        for(int y=0;y<2;++y){const auto row=retained.row(y);check(row.size()==std::size_t(3*bpp) &&
            std::memcmp(row.data(),expected.constData()+y*3*bpp,row.size())==0,"owned child exact valid bytes after parent destruction");}
        check(child.preview()==expectedPreview,"preview built from cropped raster");
        auto writable=retained.ownedCopy();writable.setTo(cv::Scalar::all(0));
        child={};check(std::memcmp(retained.row(0).data(),expected.constData(),3*bpp)==0,"access owns lifetime; writable copy isolated");
    }
    const auto root=gray({500,400});const auto crop=ImageAccess::cropped(root,{300,200,100,80},false);
    check(root.geometry().rootSize()==root.size() && root.geometry().rasterSize()==root.size() && root.geometry().rootOffset()==QPoint(0,0) &&
        root.geometry().rootCoordinateSystemId()==root.metadata().coordinateSystemId && root.geometry().coordinateSystemId()==root.metadata().coordinateSystemId,"root identity and geometry");
    check(crop.geometry().toRoot({10,20})==QPointF(310,220) && crop.geometry().fromRoot({310.25,220.75})==QPointF(10.25,20.75),"forward/inverse subpixel translation");
    const auto nested=ImageAccess::cropped(crop,{10,20,30,40},false);
    check(nested.geometry().rootOffset()==QPoint(310,220) && nested.geometry().toRoot({29,39})==QPointF(339,259) &&
        nested.geometry().fromRoot({310,220})==QPointF(0,0) && nested.geometry().rootSize()==root.size() && nested.geometry().cropDepth()==2,"nested root composition and last centre");
    check(nested.metadata().coordinateSystemId==nested.geometry().coordinateSystemId() && nested.metadata().coordinateSystemId.size()<=256 &&
        nested.geometry().rootCoordinateSystemId()=="source/raw","child metadata uses bounded local ID; raw root retained");
    sameOrigin(nested.metadata(),root.metadata());
    nested.geometry().requireFootprint({{0,0},{29,39},{0.25,1.75}});
    for(QPointF p:{QPointF(-0.01,0),QPointF(30,0),QPointF(0,40),QPointF(std::numeric_limits<double>::infinity(),0),QPointF(0,std::numeric_limits<double>::quiet_NaN())})
        reject([&]{nested.geometry().toRoot(p);},"local/nonfinite centre rejected");
    for(QPointF p:{QPointF(309,220),QPointF(340,220),QPointF(500,220),QPointF(310,-1),QPointF(310,std::numeric_limits<double>::infinity())})
        reject([&]{nested.geometry().fromRoot(p);},"root or child footprint rejected in inverse");
    reject([&]{nested.geometry().requireFootprint({});},"empty footprint rejected");
    reject([&]{nested.geometry().requireFootprint({{0,0},{30,39}});},"every footprint point checked");
    reject([&]{ImageFrame{}.geometry().toRoot({0,0});},"empty frame geometry rejected");
    for(const auto& rect:QVector<QRect>{{0,0,0,1},{0,0,1,0},{0,0,-1,1},{-1,0,1,1},{0,-1,1,1},{499,399,2,1},{500,0,1,1},{0,400,1,1},
            QRect(QPoint(std::numeric_limits<int>::max()-1,0),QPoint(std::numeric_limits<int>::max(),0)),
            QRect(QPoint(0,std::numeric_limits<int>::max()-1),QPoint(0,std::numeric_limits<int>::max())),
            QRect(QPoint(std::numeric_limits<int>::min(),0),QPoint(std::numeric_limits<int>::max(),0))})
        reject([&]{ImageAccess::cropped(root,rect);},"invalid extents checked without clamp or int overflow");
    reject([&]{ImageAccess::cropped({},QRect(0,0,1,1));},"empty crop input");
    const auto single=ImageAccess::cropped(root,{499,399,1,1},false),full=ImageAccess::cropped(root,{0,0,500,400},false);
    check(single.geometry().toRoot({0,0})==QPointF(499,399) && single.pixelText(0,0)=="Gray: 7" && single.preview().isNull(),"one-pixel edge crop and preview-free");
    check(full.contentId()==root.contentId() && full.geometry().identity()!=root.geometry().identity() && full.geometry().cropDepth()==1,"full crop explicitly creates child");
    const auto other=ImageAccess::cropped(root,{299,200,100,80},false);
    check(crop.contentId()==other.contentId() && crop.processingId()!=other.processingId() && crop.geometry().identity()!=other.geometry().identity(),"identical pixels different rect differ in provenance and geometry");
    auto freshMetadata=root.metadata();freshMetadata.frameId="next-frame";freshMetadata.cycleId="next-cycle";freshMetadata.completedAtUtc=freshMetadata.completedAtUtc.addSecs(3);
    const auto fresh=ImageAccess::cropped(gray(root.size(),99).withMetadata(freshMetadata),{300,200,100,80},false);
    check(fresh.contentId()!=crop.contentId() && fresh.geometry()==crop.geometry() && fresh.metadata().frameId=="next-frame","geometry excludes pixels/frame/cycle/time");
    auto restamp=crop.metadata();restamp.frameId="restamped";restamp.cycleId="restamped-cycle";restamp.completedAtUtc=restamp.completedAtUtc.addSecs(10);
    restamp.exposureAtUtc=restamp.exposureAtUtc->addMSecs(5);restamp.deviceTimestampTicks=123;restamp.deviceClockId="another-clock";
    check(crop.withMetadata(restamp).geometry()==crop.geometry() && crop.withMetadata(restamp).processingId()==crop.processingId(),"restamp preserves sealed geometry and processing");
    restamp.sourceId="foreign";reject([&]{crop.withMetadata(restamp);},"child source cannot be rebound");
    restamp=crop.metadata();restamp.coordinateSystemId="source/raw";reject([&]{crop.withMetadata(restamp);},"child cannot be relabelled as raw");
    auto rebind=root.metadata();rebind.sourceId="foreign";check(root.withMetadata(rebind).geometry().identity()!=root.geometry().identity(),"root identity includes source");
    rebind=root.metadata();rebind.coordinateSystemId="foreign/raw";check(root.withMetadata(rebind).geometry().identity()!=root.geometry().identity(),"root identity includes raw coordinate");
    check(gray({501,400}).geometry().identity()!=root.geometry().identity(),"root identity includes raw size");
    const auto sameNested=ImageAccess::cropped(root,{310,220,30,40},false);
    check(sameNested.contentId()==nested.contentId() && sameNested.geometry().rootOffset()==nested.geometry().rootOffset() && sameNested.geometry().identity()!=nested.geometry().identity(),"parent chain differentiates equal final footprint");
    check(crop.processing().first().method=="roi-crop" && crop.processing().first().version==1 && crop.processing().first().inputContentId==root.contentId() &&
        crop.processing().first().parametersId==crop.geometry().identity() && root.processingId().isEmpty(),"canonical crop processing and legacy root no-op");
    const QJsonObject preprocess{{"channel","gray"},{"steps",QJsonArray{QJsonObject{{"op","threshold"},{"size",3},{"value",5}}}}};
    const auto before=Preprocessing::run(root,preprocess),after=Preprocessing::run(crop,preprocess);
    check(before.geometry()==root.geometry() && after.geometry()==crop.geometry() && after.metadata().coordinateSystemId==crop.metadata().coordinateSystemId,"real preprocess preserves root/child geometry");
    const auto recrop=ImageAccess::cropped(after,{10,20,30,40},false);
    check(recrop.geometry()==nested.geometry() && recrop.processing().size()==3 && recrop.contentId()!=nested.contentId(),"crop/preprocess/crop preserves geometry with real pixels and shared history");
    check(ImageAccess::cropped(before,{300,200,100,80},false).geometry()==crop.geometry(),"preprocess before crop keeps geometry identity");
    reject([&]{ImageAccess::processed(crop,cv::Mat(1,1,CV_8UC1),PixelFormat::Gray8,"resize",1,QCryptographicHash::hash("x",QCryptographicHash::Sha256));},"processed size check remains");
    auto chain=single;
    for(int i=1;i<32;++i)chain=ImageAccess::cropped(chain,{0,0,1,1},false);
    check(chain.geometry().cropDepth()==32 && chain.processing().size()==32,"32 nested crops accepted");
    reject([&]{ImageAccess::cropped(chain,{0,0,1,1});},"33rd crop rejected");
    reject([&]{Preprocessing::run(chain,preprocess);},"preprocess shares crop chain budget");
    chain=gray({1,1});for(int i=0;i<31;++i)chain=Preprocessing::run(chain,preprocess);
    chain=ImageAccess::cropped(chain,{0,0,1,1},false);
    check(chain.processing().size()==32 && chain.geometry().cropDepth()==1,"crop shares preprocessing budget rather than depth only");
    reject([&]{ImageAccess::cropped(chain,{0,0,1,1});},"mixed chain overflow rejected");
    // Buffer extent overflow remains rejected before any allocation/copy or crop.
    unsigned char byte=7;
    reject([&]{ImageFrame::fromBuffer(&byte,std::numeric_limits<std::size_t>::max(),1,3,std::numeric_limits<std::size_t>::max(),PixelFormat::Gray8);},"buffer extent overflow");
    reject([&]{ImageFrame::fromBuffer(&byte,1,std::numeric_limits<int>::max(),std::numeric_limits<int>::max(),1,PixelFormat::Gray8);},"pixel count limit without int overflow");
    QTextStream(stdout)<<checks<<" image crop checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<"\n";return 1;}}
