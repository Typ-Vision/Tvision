#include "vision/ImageFrame.hpp"
#include "vision/ImageAccess.hpp"
#include "application/EditorSession.hpp"
#include "app/ImageViewer.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QTextStream>
#include <QDir>
#include <stdexcept>
using namespace visionflow;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    const bool baseline=app.arguments().contains("--baseline");
    const auto evidence=qEnvironmentVariable("VISIONFLOW_CCUX22A_UI_DIR");
    if(!evidence.isEmpty())check(QDir().mkpath(evidence),"UI timing evidence directory");
    for(const auto size:{QSize(640,480),QSize(2448,2048)}){
        AcquisitionTimingBinding timingBinding(AcquisitionTiming::request("image-refresh-fixture"));
        QByteArray pixels(size.width()*size.height(),char(123));
        QElapsedTimer timer;timer.start();
        auto frame=ImageFrame::fromBuffer(pixels.constData(),pixels.size(),size.width(),size.height(),size.width(),PixelFormat::Gray8);
        const auto creation=timer.nsecsElapsed();
        QCryptographicHash expected(QCryptographicHash::Sha256);
        expected.addData(QByteArray::number(size.width())+":"+QByteArray::number(size.height())+":0");expected.addData(pixels);
        check(frame.contentId()==expected.result(),"legacy raster identity preserved");
        timer.restart();for(int i=0;i<100;++i)check(frame.withMetadata({}).contentId()==expected.result(),"shared metadata copy identity");
        const auto hashing=timer.nsecsElapsed();
        ImageViewer viewer;viewer.resize(850,600);viewer.setImage(frame.preview());
        if(!evidence.isEmpty())for(const auto screen:{QSize(1400,900),QSize(850,600)}){viewer.resize(screen);viewer.show();QApplication::processEvents();check(viewer.grab().save(QDir(evidence).absoluteFilePath(QString("viewer-%1-%2.png").arg(size.width()).arg(screen.width()))),"timing viewer screenshot");}
        QGraphicsPixmapItem* item=nullptr;for(auto* entry:viewer.scene()->items())if(auto* p=dynamic_cast<QGraphicsPixmapItem*>(entry))item=p;
        check(item,"raster item exists");const auto key=item->pixmap().cacheKey();
        timer.restart();for(int i=0;i<100;++i)viewer.setPreviewPixels(frame.preview());const auto refresh=timer.nsecsElapsed();
        if(!baseline)check(item->pixmap().cacheKey()==key,"unchanged raster must not recreate pixmap");
        auto changed=frame.preview();changed.setPixelColor(0,0,Qt::black);viewer.setPreviewPixels(changed);
        check(item->pixmap().toImage().pixelColor(0,0)==Qt::black,"same-size changed raster refreshes");
        check(frame.preview().pixelColor(0,0).red()==123,"preview detach does not mutate raw");
        pixels[0]=char(124);auto other=ImageFrame::fromBuffer(pixels.constData(),pixels.size(),size.width(),size.height(),size.width(),PixelFormat::Gray8,{},false);
        check(other.contentId()!=frame.contentId(),"new pixels new identity without preview");
        EditorSession session;EditorContext context{"project","","recipe",{}};session.frame=frame;
        const auto scope=session.evaluationScope(context);
        auto metadata=frame.metadata();metadata.frameId="new-acquisition";session.frame=frame.withMetadata(metadata);
        check(session.evaluationScope(context)!=scope,"same pixels new acquisition still invalidates scope");
        session.frame=other;check(session.evaluationScope(context)!=scope,"changed pixels invalidate scope");
        viewer.clearImage();viewer.setPreviewPixels(frame.preview());check(!viewer.scene()->items().isEmpty(),"clear resets display cache");
        QTextStream(stdout)<<size.width()<<"x"<<size.height()<<" create_ms="<<creation/1e6<<" hash100_ms="<<hashing/1e6<<" refresh100_ms="<<refresh/1e6<<"\n";
    }return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
