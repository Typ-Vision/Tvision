#include "application/ChessboardService.hpp"
#include "chessboard_fixture.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <iostream>
#include <stdexcept>
using namespace visionflow;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("chessboard-XXXXXX"));check(dir.isValid(),"build fixture directory");const auto path=dir.filePath("board.png");ChessboardTarget target{"test-board",9,6,10};
    for(bool perspective:{false,true}) {
        check(chessboardFixture(perspective).save(path),"save fixture");const auto test=ChessboardService::importImage(path);
        const auto observation=ChessboardService::detect(test.frame,target);check(observation.pixels.size()==54,"all corners");
        check(observation.resolution==QSize(700,520) && observation.source.sourceId.startsWith("calibration-test:"),"full resolution / test origin");
        check(observation.worldMillimetres[0]==QPointF(0,0) && observation.worldMillimetres[53]==QPointF(80,50),"target world grid");
        if(!perspective)for(int i=0;i<54;++i){auto p=observation.pixels[i];double x=110+(i%9)*60,y=110+(i/9)*60;double direct=QLineF(p,{x-0.5,y-0.5}).length(),reverse=QLineF(p,{700-x-0.5,520-y-0.5}).length();check(std::min(direct,reverse)<0.6,"independent known pixel geometry");}
        const auto again=ChessboardService::importImage(path);check(again.contentSha256==test.contentSha256 && again.frame.metadata().frameId!=test.frame.metadata().frameId,"stable content / fresh frame");
        auto wrong=target;wrong.columns=8;rejects([&]{ChessboardService::detect(test.frame,wrong);});wrong=target;wrong.squareMillimetres=0;rejects([&]{ChessboardService::detect(test.frame,wrong);});
        wrong=target;wrong.squareMillimetres=20;const auto changed=ChessboardService::detect(test.frame,wrong);check(changed.worldMillimetres[53]==QPointF(160,100),"physical spacing cannot be inferred from image");
    }
    QImage blank(700,520,QImage::Format_RGB32);blank.fill(Qt::white);blank.save(path);auto test=ChessboardService::importImage(path);rejects([&]{ChessboardService::detect(test.frame,target);});
    QFile corrupt(path);check(corrupt.open(QIODevice::WriteOnly|QIODevice::Truncate),"corrupt open");corrupt.write("not an image");corrupt.close();rejects([&]{ChessboardService::importImage(path);});
    std::cout<<"PASS chessboard raw/test observation contract\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
