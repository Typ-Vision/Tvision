#include "vision/codes/CodeReader.hpp"
#include "application/BackendCapabilities.hpp"
#include "domain/CodeReadConfig.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
using namespace visionflow;
namespace {
int checks=0;void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
template<class F>void reject(F f){bool failed=false;try{f();}catch(...){failed=true;}check(failed,"expected code reader rejection");}
ImageFrame frame(const cv::Mat& m){return ImageFrame::fromBuffer(m.data,(m.rows-1)*m.step+m.cols,m.cols,m.rows,m.step,PixelFormat::Gray8,{"source","frame","cycle","raw"});}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==4,"reader fixture arguments");CodeReaderOptions options;options.formats=codeFormats();cv::Mat blank(100,100,CV_8U,cv::Scalar(255));
    if(!BackendCapabilities::get("zxing-reader").available){reject([&]{CodeReader::read(frame(blank),options);});QTextStream(stdout)<<"OFF backend explicitly rejected\n";return 0;}
    QFile definitions(argv[1]);check(definitions.open(QIODevice::ReadOnly),"fixture definitions");int fixtures=0;
    for(const auto& row:definitions.readAll().split('\n')){if(row.trimmed().isEmpty())continue;const auto fields=row.trimmed().split('\t');check(fields.size()==4,"fixture row");const auto image=cv::imread(QDir(argv[2]).filePath(QString::fromUtf8(fields[0])+".pgm").toStdString(),cv::IMREAD_GRAYSCALE);check(!image.empty(),"independent fixture exists");const auto original=frame(image);const auto hash=original.contentId();
        const auto format=codeFormats()[fixtures++];options.formats={format};
        for(int variant=0;variant<4;++variant){cv::Mat m=image;if(variant==1)cv::rotate(image,m,cv::ROTATE_90_CLOCKWISE);if(variant==2)cv::bitwise_not(image,m);if(variant==3){cv::Mat padded(image.rows+60,image.cols+90,CV_8U,cv::Scalar(255));image.copyTo(padded(cv::Rect(37,23,image.cols,image.rows)));m=padded;options.roi={37,23,image.cols,image.rows};}
            const auto codes=CodeReader::read(frame(m),options);if(codes.items.size()!=1)QTextStream(stderr)<<fields[0]<<" variant "<<variant<<" count "<<codes.items.size()<<'\n';check(codes.items.size()==1,"one physical code");check(codes.items.first().text==QString::fromUtf8(fields[3]) && codes.items.first().bytes==fields[3],"exact decoded text and raw bytes");check(codes.items.first().symbology==(format=="QRCode" ? QString("QR"):QString(format).remove('-')),"exact symbology");for(const auto& p:codes.items.first().corners)check(p.x()>=0 && p.y()>=0 && p.x()<m.cols && p.y()<m.rows && (variant!=3 || (p.x()>=37 && p.y()>=23)),"raw corners after ROI mapping");options.roi={};}
        options.formats={format=="QRCode" ? "Code39":"QRCode"};check(CodeReader::read(original,options).items.isEmpty(),"allow-list exclusion");check(original.contentId()==hash,"immutable input");
    }check(fixtures==8,"eight format fixtures");options.formats=codeFormats();check(CodeReader::read(frame(blank),options).items.isEmpty(),"valid no-read empty list");
    auto qr=cv::imread(QDir(argv[2]).filePath("qr.pgm").toStdString(),cv::IMREAD_GRAYSCALE);cv::Mat pair(qr.rows,qr.cols*2+70,CV_8U,cv::Scalar(255));qr.copyTo(pair(cv::Rect(0,0,qr.cols,qr.rows)));qr.copyTo(pair(cv::Rect(qr.cols+70,0,qr.cols,qr.rows)));options.formats={"QRCode"};auto duplicates=CodeReader::read(frame(pair),options);check(duplicates.items.size()==2 && duplicates.items[0].text==duplicates.items[1].text && duplicates.items[0].corners!=duplicates.items[1].corners,"same text physical duplicates retained");
    options.maxCount=1;reject([&]{CodeReader::read(frame(pair),options);});options.maxCount=32;reject([&]{CodeReader::read(frame(qr),options,std::make_shared<std::atomic_bool>(true));});options.roi={1,1,qr.cols,qr.rows};reject([&]{CodeReader::read(frame(qr),options);});options.roi={5,5,0,0};reject([&]{CodeReader::read(frame(qr),options);});options.roi={};options.formats={"unknown"};reject([&]{CodeReader::read(frame(qr),options);});options.formats={"QRCode"};options.budgetMs=0;reject([&]{CodeReader::read(frame(qr),options);});
    // A large image with a one-millisecond cooperative budget must discard late work.
    options.budgetMs=1;cv::Mat large(3000,3000,CV_8U,cv::Scalar(255));qr.copyTo(large(cv::Rect(10,10,qr.cols,qr.rows)));reject([&]{CodeReader::read(frame(large),options);});
    options.budgetMs=1000;
    const QStringList ids{"qr_unicode","datamatrix_gs","code128_gs"};
    const QStringList payloads{QString::fromUtf8("İşlem-Ölçüm-2048"),QString("0101234567890128")+QChar(0x1d)+"10LOT42",QString("ABC")+QChar(0x1d)+"2048"};
    for(int i=0;i<ids.size();++i){const auto m=cv::imread(QDir(argv[3]).filePath(ids[i]+".pgm").toStdString(),cv::IMREAD_GRAYSCALE);check(!m.empty(),"native Unicode/GS fixture");options.formats={codeFormats()[i]};const auto read=CodeReader::read(frame(m),options);check(read.items.size()==1 && read.items[0].text==payloads[i] && read.items[0].bytes==payloads[i].toUtf8(),"native UTF-8 ECI and GS bytes preserved");}
    QTextStream(stdout)<<checks<<" reader checks passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
