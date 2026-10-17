#include "application/ProjectService.hpp"
#include "application/ImageFileService.hpp"
#include "application/MatchService.hpp"
#include "app/ImageViewer.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <opencv2/imgcodecs.hpp>
#include <QJsonArray>
#include <algorithm>
#include <vector>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message) {if (!ok) throw std::runtime_error(message);}
template<class F> void rejected(F action,const char* message) {bool failed=false; try {action();} catch(const std::exception&) {failed=true;} check(failed,message);}
void encode(const QString& path,const cv::Mat& image) {
    std::vector<uchar> bytes; check(cv::imencode(".png",image,bytes),"fixture encoding");
    QFile file(path); check(file.open(QIODevice::WriteOnly),"fixture write open"); check(file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())==static_cast<qint64>(bytes.size()),"fixture write");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("editor-test-XXXXXX")); check(dir.isValid(),"test dir");
        ImageFileService acquisition;
        const auto path=dir.filePath("Türkçe_ölçüm.png");
        cv::Mat rgb(31,37,CV_8UC3,cv::Scalar(10,20,30)); encode(path,rgb);
        auto frame=acquisition.load(path);
        check(frame.size()==QSize(37,31) && frame.preview().pixelColor(0,0)==QColor(30,20,10) && frame.preview().pixelColor(36,30)==QColor(30,20,10),"odd stride and RGB channel order");
        check(frame.pixelText(0,0)=="R:30 G:20 B:10" && frame.pixelText(-1,0).isEmpty(),"raw pixel values and bounds");
        const auto gray=dir.filePath("gray.png"); encode(gray,cv::Mat(13,17,CV_8UC1,cv::Scalar(123)));
        check(acquisition.load(gray).pixelText(0,0)=="Gray: 123","gray8 raw");
        const auto gray16=dir.filePath("gray16.png"); encode(gray16,cv::Mat(13,17,CV_16UC1,cv::Scalar(65535)));
        const auto frame16=acquisition.load(gray16); check(frame16.pixelText(0,0)=="Gray: 65535" && frame16.preview().pixelColor(0,0).red()==255,"16-bit raw and display mapping");
        const auto alpha=dir.filePath("alpha.png"); encode(alpha,cv::Mat(13,17,CV_8UC4,cv::Scalar(10,20,30,77)));
        const auto frameAlpha=acquisition.load(alpha); check(frameAlpha.pixelText(0,0)=="R:30 G:20 B:10 A:77" && frameAlpha.preview().pixelColor(0,0).alpha()==77,"RGBA order and alpha");
        const auto unsupported=dir.filePath("rgb16.png"); encode(unsupported,cv::Mat(13,17,CV_16UC3,cv::Scalar(1,2,3)));
        rejected([&]{acquisition.load(unsupported);},"unsupported data rejected");
        rejected([&]{acquisition.load(dir.filePath("missing.png"));},"missing image rejected");
        const auto corrupt=dir.filePath("bad.png"); {QFile file(corrupt); check(file.open(QIODevice::WriteOnly),"bad fixture"); file.write("broken");}
        rejected([&]{acquisition.load(corrupt);},"corrupt image rejected");
        ProjectService service; service.createProject("Editor","",dir.filePath("editor.vfproject"),"ImageFile"); service.save();
        const auto recipe=service.createRecipe("Part","",ApplicationType::Measurement,"ImageFile"); service.save();
        const auto source=service.setImageFile(path); const auto match=service.addTool("template_match");
        check(service.addTool("image_file")==source && service.tools().size()==2,"single source identity");
        auto tool=service.tools()[1]; tool.name="Match_1"; tool.parameters["minScore"]=0.81; service.updateTool(tool);
        auto badTool=tool; badTool.parameters["minScore"]=1.1;
        rejected([&]{service.updateTool(badTool);},"invalid score rejected");
        check(service.tools()[1].parameters["minScore"].toDouble()==0.81,"failed tool update is non-mutating");
        service.save(); service.openProject(service.path()); service.selectRecipe(recipe);
        check(service.tools()[0].id==source && service.tools()[1].id==match && service.imageFilePath()==path && service.activeRecipe()->recipeVersion==2,"tool IDs, source path and revision roundtrip");
        service.updateTool(service.tools()[1]); check(!service.dirty(),"no-op config update stays clean");
        tool=service.tools()[0]; tool.enabled=false; service.updateTool(tool); check(service.imageFilePath().isEmpty(),"disabled source unavailable");
        service.discardChanges(); check(service.imageFilePath()==path,"discard source config");
        service.removeTool(match); service.save(); service.openProject(service.path()); service.selectRecipe(recipe);
        check(service.tools().size()==1 && service.tools()[0].id==source,"removal persisted");
        const auto empty=editorTools(QJsonObject{}); check(empty.isEmpty(),"Step 2 definition compatible");
        rejected([&]{editorTools(QJsonObject{{"editor",QJsonObject{{"schemaVersion",10},{"tools",QJsonArray{}}}}});},"future editor schema rejected");
        const auto retained=withEditorTools(QJsonObject{{"vendor",42}},service.tools()); check(retained["vendor"].toInt()==42,"unrelated definition fields preserved");
        auto legacyTool=ToolDefinition{match,"template_match","Legacy","",true,QJsonObject{{"minScore",0.81}}};
        const QJsonObject legacy{{"editor",QJsonObject{{"schemaVersion",1},{"tools",QJsonArray{QJsonObject{{"id",legacyTool.id},{"kind",legacyTool.kind},{"name",legacyTool.name},{"description",""},{"enabled",true},{"parameters",legacyTool.parameters}}}}}}};
        const auto migrated=editorTools(legacy);
        check(migrated[0].id==match && migrated[0].parameters["minScore"].toDouble()==0.81 && migrated[0].parameters["modelPng"].toString().isEmpty(),"legacy match migration preserves ID and score");
        MatchService matcher; cv::RNG rng(12345); cv::Mat noise(1944,2592,CV_8UC1); rng.fill(noise,cv::RNG::UNIFORM,0,256);
        const auto matchPath=dir.filePath("match.png"); encode(matchPath,noise); auto matchFrame=acquisition.load(matchPath);
        auto params=defaultParameters("template_match"); const QRect modelRoi(700,600,64,48);
        params["modelRoi"]=MatchService::rectangle(modelRoi); params["modelPng"]=matcher.teach(matchFrame,modelRoi);
        params["searchRoi"]=MatchService::rectangle(QRect(600,500,512,512)); params["minScore"]=0.9;
        auto outcome=matcher.run(matchFrame,params); check(outcome.accepted && outcome.rectangle==modelRoi && outcome.score>0.999,"positive full-image coordinates");
        if (app.arguments().contains("--benchmark")) {
            for (int i=0;i<10;++i) matcher.run(matchFrame,params);
            std::vector<double> times; int correct=0;
            for (int i=0;i<100;++i) {const auto r=matcher.run(matchFrame,params); times.push_back(r.milliseconds); if (r.accepted && r.rectangle==modelRoi) ++correct;}
            std::sort(times.begin(),times.end());
            QFile csv(QDir(QCoreApplication::applicationDirPath()).filePath("roi_match_service.csv")); check(csv.open(QIODevice::WriteOnly|QIODevice::Text),"benchmark csv");
            QTextStream stream(&csv); stream<<"image_width,image_height,model_width,model_height,roi_width,roi_height,iterations,correct,median_ms,p95_ms\n2592,1944,64,48,512,512,100,"<<correct<<','<<times[49]<<','<<times[94]<<'\n';
            QTextStream(stdout)<<"ROI service benchmark: median "<<times[49]<<" ms; p95 "<<times[94]<<" ms; correct "<<correct<<"/100\n";
        }
        auto moved=noise.clone(); rng.fill(moved,cv::RNG::UNIFORM,0,256); noise(cv::Rect(700,600,64,48)).copyTo(moved(cv::Rect(840,720,64,48))); encode(matchPath,moved);
        outcome=matcher.run(acquisition.load(matchPath),params); check(outcome.accepted && outcome.rectangle.topLeft()==QPoint(840,720),"translated model found");
        rng.fill(moved,cv::RNG::UNIFORM,0,256); encode(matchPath,moved); outcome=matcher.run(acquisition.load(matchPath),params); check(!outcome.accepted,"negative random image rejected");
        auto invalidParams=params; invalidParams["modelPng"]="broken"; rejected([&]{matcher.run(matchFrame,invalidParams);},"corrupt model rejected");
        invalidParams=params; invalidParams["searchRoi"]=MatchService::rectangle(QRect(2580,1930,64,48)); rejected([&]{matcher.run(matchFrame,invalidParams);},"out-of-bounds search rejected");
        invalidParams=params; invalidParams["searchRoi"]=MatchService::rectangle(QRect(0,0,2,2)); rejected([&]{matcher.run(matchFrame,invalidParams);},"small search rejected");
        invalidParams=defaultParameters("template_match"); rejected([&]{matcher.run(matchFrame,invalidParams);},"missing model rejected");
        rejected([&]{matcher.teach(frame,QRect(0,0,10,10));},"flat model rejected");
        rejected([&]{matcher.teach(matchFrame,QRect(-1,0,10,10));},"invalid model region rejected");
        cv::Mat high; noise.convertTo(high,CV_16U,257); encode(gray16,high); auto highFrame=acquisition.load(gray16);
        check(matcher.teach(highFrame,modelRoi)==params["modelPng"].toString(),"16-bit fixed-range grayscale conversion");
        const auto taughtId=service.addTool("template_match"); auto taughtTool=service.tools().last(); taughtTool.parameters=params; service.updateTool(taughtTool); service.save(); service.openProject(service.path()); service.selectRecipe(recipe);
        check(service.tools().last().id==taughtId && service.tools().last().parameters==params && matcher.run(matchFrame,service.tools().last().parameters).accepted,"embedded model roundtrip");
        ImageViewer viewer; viewer.resize(640,480); viewer.show(); app.processEvents();
        QImage big(2592,1944,QImage::Format_RGB888); big.fill(Qt::white); viewer.setImage(big); app.processEvents();
        const auto fit=viewer.zoomFactor(); check(fit>0 && fit<1,"5 MP fit");
        viewer.zoomBy(2); check(std::abs(viewer.zoomFactor()-fit*2)<1e-8,"zoom in"); viewer.actualSize(); check(viewer.zoomFactor()==1,"actual size");
        viewer.centerOn(1000,900); const auto point=viewer.mapFromScene(QPointF(1000,900));
        check(viewer.imagePixelAt(point)==QPoint(1000,900),"view transform to image pixel");
        viewer.zoomBy(2); viewer.centerOn(1000,900);
        check(viewer.imagePixelAt(viewer.mapFromScene(QPointF(1000.25,900.25)))==QPoint(1000,900),"zoomed pixel coordinates");
        const int before=viewer.horizontalScrollBar()->value(); const QPoint start=viewer.viewport()->rect().center();
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(start),QPointF(viewer.viewport()->mapToGlobal(start)),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier); QApplication::sendEvent(viewer.viewport(),&press);
        QMouseEvent move(QEvent::MouseMove,QPointF(start+QPoint(60,20)),QPointF(viewer.viewport()->mapToGlobal(start+QPoint(60,20))),Qt::NoButton,Qt::LeftButton,Qt::NoModifier); QApplication::sendEvent(viewer.viewport(),&move);
        QMouseEvent release(QEvent::MouseButtonRelease,QPointF(start+QPoint(60,20)),QPointF(viewer.viewport()->mapToGlobal(start+QPoint(60,20))),Qt::LeftButton,Qt::NoButton,Qt::NoModifier); QApplication::sendEvent(viewer.viewport(),&release);
        check(viewer.horizontalScrollBar()->value()!=before,"mouse pan scrolls scene");
        viewer.actualSize(); viewer.centerOn(1000,900); QRect drawn;
        viewer.onRegion=[&](QRect r){drawn=r;}; viewer.drawRegion(true);
        auto gesture=[&](QEvent::Type type,QPoint p,Qt::MouseButton button,Qt::MouseButtons buttons) {QMouseEvent event(type,QPointF(p),QPointF(viewer.viewport()->mapToGlobal(p)),button,buttons,Qt::NoModifier); QApplication::sendEvent(viewer.viewport(),&event);};
        const auto from=viewer.mapFromScene(QPointF(1050,950)); const auto to=viewer.mapFromScene(QPointF(1000,900));
        gesture(QEvent::MouseButtonPress,from,Qt::LeftButton,Qt::LeftButton); gesture(QEvent::MouseMove,to,Qt::NoButton,Qt::LeftButton); gesture(QEvent::MouseButtonRelease,to,Qt::LeftButton,Qt::NoButton);
        check(drawn==QRect(1000,900,51,51),"reverse ROI image coordinates");
        viewer.centerOn(2500,1850); viewer.drawRegion(true);
        const auto inside=viewer.mapFromScene(QPointF(2500,1850)); const auto outside=viewer.mapFromScene(QPointF(2700,2100));
        gesture(QEvent::MouseButtonPress,inside,Qt::LeftButton,Qt::LeftButton); gesture(QEvent::MouseButtonRelease,outside,Qt::LeftButton,Qt::NoButton);
        check(drawn==QRect(2500,1850,92,94),"ROI drag clamps image boundary");
        viewer.clearImage(); check(!viewer.hasImage() && viewer.imagePixelAt({1,1})==QPoint(-1,-1),"empty viewer has no stale pixels");
        QTextStream(stdout)<<"PASS: image decode/pixels, tools/persistence, zoom/pan and image coordinates\n"; return 0;
    } catch (const std::exception& e) {QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n'; return 1;}
}
