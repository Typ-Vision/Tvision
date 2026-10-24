#include "application/CircleToolService.hpp"
#include "application/MetrologyService.hpp"
#include "application/MeasurementDiagnostic.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <cmath>
#include <numbers>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
ImageFrame frame(const cv::Mat& m){return ImageFrame::fromBuffer(m.data,m.rows*m.step,m.cols,m.rows,m.step,m.depth()==CV_16U ? PixelFormat::Gray16:PixelFormat::Gray8,{"source","frame","cycle","raw"},false);}
ToolDefinition tool(const QString& kind,const ToolRoi& roi){ToolDefinition t;t.id=QUuid::createUuid().toString(QUuid::WithoutBraces);t.name=kind;t.kind=kind;t.parameters=defaultParameters(kind);t.parameters["mode"]="pixel";t.parameters["scanRoi"]=roi.toJson();return t;}
ToolResult run(const ToolDefinition& t,const cv::Mat& pixels){auto f=frame(pixels);QMap<QString,PortValue> inputs{{"image",{{"image",PortType::Image,"px","image"},f,true,"frame","cycle",f.metadata()}}};return t.kind=="circle_fit" ? CircleToolService::execute(t,inputs,"frame","cycle",{},{}):MetrologyService::execute(t,inputs,{},"frame","cycle",{},{});}
double number(const ToolResult& r,const char* key){const auto v=r.outputs.value(key);check(v.valid && std::holds_alternative<double>(v.data),"real numeric port required");return std::get<double>(v.data);}
template<class F>FitDiagnostic failure(F f){try{f();}catch(const FitDiagnosticError& e){return e.evidence;}throw std::runtime_error("Expected structured rejection");}
cv::Mat ring(QPointF centre,bool dark,bool damaged=false){cv::Mat pixels(192,192,CV_8UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){const auto d=QPointF(x,y)-centre;double angle=std::atan2(d.y(),d.x())*180/std::numbers::pi;const double radius=damaged && angle>20 && angle<40 ? 43:40;bool white=std::hypot(d.x(),d.y())>radius;if(dark)white=!white;pixels.at<uchar>(y,x)=white ? 220:20;}return pixels;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    int circleCases=0,lineCases=0;double radiusMax=0,lineMax=0;
    for(double phase:{0.,.37,.73})for(bool dark:{false,true})for(bool inward:{false,true}){
        const QPointF centre(95+phase,96-phase);auto pixels=ring(centre,dark);
        auto t=tool("circle_fit",{{"raw"},Annulus{centre,28,52}});t.parameters["direction"]=inward ? "inward":"outward";t.parameters["polarity"]=(dark!=inward) ? "falling":"rising";
        const auto r=run(t,pixels);check(r.state==ExecutionState::Done && r.decision==PartDecision::NotEvaluated,"sharp circle has diagnostic geometry only");const double radius=number(r,"radiusPx");radiusMax=std::max(radiusMax,std::abs(radius-40));check(std::abs(radius-40)<.6,"independent sharp circle radius oracle");
        cv::Mat gray16;pixels.convertTo(gray16,CV_16UC1,257);const auto r16=run(t,gray16);check(std::abs(number(r16,"radiusPx")-radius)<1e-9,"Gray8 Gray16 normalized parity");
        t.parameters["polarity"]=(dark!=inward) ? "rising":"falling";auto rejected=failure([&]{run(t,pixels);});check(rejected.stage=="edge_search" && rejected.rejections.polarity>0,"wrong polarity has measured rejection");++circleCases;
    }
    for(double angle:{0.,17.,63.})for(double phase:{0.,.37,.73})for(bool dark:{false,true}){
        const double a=angle*std::numbers::pi/180;QPointF centre(96+phase,96-phase);cv::Mat pixels(192,192,CV_8UC1);
        for(int y=0;y<192;++y)for(int x=0;x<192;++x){double d=-(x-centre.x())*std::sin(a)+(y-centre.y())*std::cos(a);pixels.at<uchar>(y,x)=((d>=0)!=dark) ? 220:20;}
        auto t=tool("line_fit",{{"raw"},RotatedRectangle{centre,{100,30},angle}});t.parameters["polarity"]=dark ? "falling":"rising";const auto r=run(t,pixels);const double measured=number(r,"anglePx");lineMax=std::max(lineMax,std::abs(measured-angle));check(std::abs(measured-angle)<.8 && r.decision==PartDecision::NotEvaluated,"sharp line independent angle oracle");
        cv::Mat gray16;pixels.convertTo(gray16,CV_16UC1,257);check(std::abs(number(run(t,gray16),"anglePx")-measured)<1e-9,"line normalized Gray16 parity");
        t.parameters["scanRoi"]=ToolRoi{{"raw"},RotatedRectangle{centre,{100,30},angle+180}}.toJson();t.parameters["polarity"]=dark ? "rising":"falling";check(std::abs(number(run(t,pixels),"anglePx")-(angle-180))<.8,"same raster reverse arrow and opposite polarity");++lineCases;
    }
    auto pixels=ring({95.37,95.63},false);auto t=tool("circle_fit",{{"raw"},Annulus{{95.37,95.63},28,52}});
    t.parameters["polarity"]="rising";t.parameters["scanCount"]=12;const auto sparse=run(t,pixels);check(sparse.state==ExecutionState::Done && sparse.fitDiagnostic && sparse.fitDiagnostic->stage=="contour_quality" && !sparse.outputs["accepted"].valid,"12 scans 15 degree gap cannot fabricate acceptance");
    t.parameters["scanCount"]=72;t.parameters["scanRoi"]=ToolRoi{{"raw"},Annulus{{95.37,95.63},37,43}}.toJson();check(failure([&]{run(t,pixels);}).stage=="sampling_support","narrow ring suggests support instead of threshold");
    t.parameters["scanRoi"]=ToolRoi{{"raw"},ArcRoi{{{95.37,95.63},28,52},0,45}}.toJson();auto narrow=failure([&]{run(t,pixels);});check(narrow.stage=="fit_support","narrow arc retains conditioning support rejection");
    t.parameters["scanRoi"]=ToolRoi{{"raw"},Annulus{{95.37,95.63},28,52}}.toJson();cv::Mat low16;pixels.convertTo(low16,CV_16UC1,16);auto low=failure([&]{run(t,low16);});check(low.stage=="edge_search" && low.rejections.contrast>0,"low effective Gray16 range has contrast explanation");
    t.parameters["deviationUpper"]=1.;const auto damaged=run(t,ring({95.37,95.63},false,true));check(number(damaged,"observedDeviationPx")>2 && damaged.outputs["deviationAccepted"].valid && !std::get<bool>(damaged.outputs["deviationAccepted"].data),"damaged contour outliers stay in real deviation rejection");
    cv::Mat shortLine(192,192,CV_8UC1,cv::Scalar(20));for(int y=96;y<192;++y)for(int x=0;x<192;++x)shortLine.at<uchar>(y,x)=220;
    auto line=tool("line_fit",{{"raw"},RotatedRectangle{{96,96},{8,30},0}});line.parameters["polarity"]="rising";auto shortSupport=failure([&]{run(line,shortLine);});check(shortSupport.code=="LINE_SPAN" && shortSupport.stage=="fit_support","short line support is distinct from missing edges");
    cv::Mat stripe(192,192,CV_8UC1);for(int y=0;y<192;++y)for(int x=0;x<192;++x){double r=std::hypot(x-96.,y-96.);stripe.at<uchar>(y,x)=r>=30 && r<45 ? 220:20;}
    t=tool("circle_fit",{{"raw"},Annulus{{96,96},20,55}});t.parameters["polarity"]="any";t.parameters["selection"]="first";const double first=number(run(t,stripe),"radiusPx");t.parameters["selection"]="last";const double last=number(run(t,stripe),"radiusPx");check(std::abs(first-30)<.6 && std::abs(last-45)<.6,"wrong-edge fixture proves first last selection, no automatic correction");
    QString report=QString("Synthetic hard-edge corpus only; user raster acceptance pending.\nCircle cases=%1 (+Gray16 parity +wrong polarity), max radius error=%2 px\nLine cases=%3 (+Gray16 parity +reversed arrows), max angle error=%4 deg\nSparse/narrow/low16/damaged/multiple-edge gates included.\n%5 checks passed\n").arg(circleCases).arg(radiusMax,0,'g',12).arg(lineCases).arg(lineMax,0,'g',12).arg(checks);
    if(argc==2){QFile f(QString::fromLocal8Bit(argv[1]));check(f.open(QIODevice::WriteOnly),"report output");f.write(report.toUtf8());}QTextStream(stdout)<<report;return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
