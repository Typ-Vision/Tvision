#include "application/RuntimeService.hpp"
#include "application/CalibrationStudyStore.hpp"
#include "application/CalibrationReportService.hpp"
#include "chessboard_fixture.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QJsonArray>
#include <QLockFile>
#include <QFile>
#include <QLineF>
#include <iostream>
using namespace visionflow;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");}
QPointF imagePoint(QPointF q){const auto w=.002*q.x()+.001*q.y()+1;return {(10*q.x()+1.5*q.y()+100)/w,(.8*q.x()+12*q.y()+100)/w};}
QPointF worldPoint(QPointF p){const double a=10-.002*p.x(),b=1.5-.001*p.x(),c=.8-.002*p.y(),d=12-.001*p.y(),x=p.x()-100,y=p.y()-100;const double det=a*d-b*c;return {(x*d-b*y)/det,(a*y-x*c)/det};}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir(QDir(QCoreApplication::applicationDirPath()).filePath("homography-runtime-XXXXXX"));check(dir.isValid(),"build fixture");
    const auto imagePath=dir.filePath("part.png");QImage image(1000,800,QImage::Format_Grayscale8);image.fill(0);{QPainter p(&image);p.fillRect(250,250,300,100,Qt::white);}check(image.save(imagePath),"part fixture");
    ProjectService project;project.createProject("H test","",dir.filePath("test.vfproject"),"file");const auto recipeId=project.createRecipe("Part","",ApplicationType::Measurement,"file");const auto source=project.addTool("image_file"),left=project.addTool("edge"),right=project.addTool("edge"),measurement=project.addTool("measurement");
    const double expected=QLineF(worldPoint({250,399.5}),worldPoint({549,399.5})).length();
    for(auto t:project.tools()){if(t.id==source)t.parameters["path"]="part.png";if(t.id==right)t.parameters["direction"]="right_to_left";if(t.id==measurement){t.parameters["lowerMm"]=expected-0.001;t.parameters["upperMm"]=expected+0.001;}project.updateTool(t);}project.setConnections({{source,"image",left,"image"},{source,"image",right,"image"},{left,"point",measurement,"p1"},{right,"point",measurement,"p2"}});project.setCalibrationSetup("lens","plane");
    const auto frame=RuntimeService::probe(*project.activeRecipe(),dir.path(),{});const auto install=CalibrationWorkflow::installation(*project.activeRecipe(),frame,"raw_planar_homography");
    ChessboardObservation observation{{"physical-declared-test-board",9,6,10},frame.metadata(),frame.size(),{}, {}};for(int r=0;r<6;++r)for(int c=0;c<9;++c){QPointF q{10.0*c,10.0*r};observation.worldMillimetres.append(q);observation.pixels.append(imagePoint(q));}
    const auto mapping=PlaneMappingService::fit(observation);CalibrationDependency setup{"plane-H",1,"pending",source,"lens",frame.size(),"plane",install.source,install.target};MeasurementService math;const auto candidate=math.teachHomography(setup,mapping);
    auto sample=[&](QString id,QPointF a,QPointF b){return CalibrationSample{id,{imagePoint(a),install.source},{imagePoint(b),install.source},QLineF(a,b).length()};};
    const QVector<CalibrationSample> holdout{sample("independent-x",{10.25,15.25},{60.25,15.25}),sample("independent-y",{20.25,5.25},{20.25,40.25})};
    auto accepted=math.validate(candidate,holdout,0.0001);check(accepted.mapping() && accepted.validated() && accepted.dependency().modelType=="raw_planar_homography","typed accepted model");check(accepted.validationRmsMillimetres()<1e-5,"independent holdout");rejects([&]{candidate.millimetresPerPixel();});
    const auto reportPath=dir.filePath("evidence.csv");rejects([&]{CalibrationReportService::saveCsv(reportPath,candidate);});CalibrationReportService::saveCsv(reportPath,accepted);QFile report(reportPath);check(report.open(QIODevice::ReadOnly),"report exists");const auto reportBytes=report.readAll();check(reportBytes.contains("raw_planar_homography") && reportBytes.contains("Measured mm") && reportBytes.contains("independent-x") && reportBytes.contains("physical reference/accuracy"),"report actual model/values and acceptance scope");
    auto fitEvidence=holdout;fitEvidence[0]={"reused-fit",{observation.pixels[0],install.source},{observation.pixels[8],install.source},80};rejects([&]{math.validate(candidate,fitEvidence,1);});auto wrong=holdout;wrong[0].knownMillimetres+=.01;rejects([&]{math.validate(candidate,wrong,0.001);});
    auto json=math.encode(accepted);check(math.decode(json).dependency()==accepted.dependency(),"typed model codec replay");auto broken=json;broken["validated"]=false;rejects([&]{math.decode(broken);});broken=json;auto dep=broken["dependency"].toObject();dep["modelType"]="uniform_planar_scale";broken["dependency"]=dep;rejects([&]{math.decode(broken);});broken=json;broken["rmsMm"]=.1;rejects([&]{math.decode(broken);});
    const auto catalogPath=project.path()+".calibrations.json";CalibrationService catalog(catalogPath,[install](const QString&){return install;});rejects([&]{catalog.save(candidate);});catalog.save(accepted);CalibrationWorkflow::bind(project,frame,catalogPath,"plane-H","raw_planar_homography");project.save();ProjectService reopened;reopened.openProject(project.path());reopened.selectRecipe(recipeId);check(reopened.activeRecipe()->calibrationBinding==accepted.dependency(),"schema2 typed binding reopened");
    auto run=[&]{return ExecutionService{}.run(project.activeRecipe()->definition,dir.path(),recipeId,"test",{}, {},{},[&](const ImageFrame& f){return CalibrationWorkflow::resolve(*project.activeRecipe(),f,catalogPath);});};
    auto result=run();auto actual=result.tools.last();check(actual.state==ExecutionState::Done && actual.decision==PartDecision::Pass && std::abs(std::get<double>(actual.outputs["value"].data)-expected)<1e-5,"PNG Edge -> H points -> actual mm pass");check(std::get<double>(actual.outputs["pixels"].data)==299,"Calculate/pixel distance retained");
    RuntimeService runtime(project.path()+".production",catalogPath,RuntimeIdentity::localEngineer());runtime.validate(*project.activeRecipe(),dir.path(),frame);runtime.activate(*project.activeRecipe(),dir.path());auto lease=runtime.begin();
    for(auto t:project.tools())if(t.id==measurement){t.parameters["lowerMm"]=expected+.1;t.parameters["upperMm"]=expected+.2;project.updateTool(t);}result=RuntimeService::execute(lease,catalogPath,{});runtime.finish(lease.ticket,result);check(result.tools.last().decision==PartDecision::Pass && runtime.records().size()==1,"immutable H Runtime snapshot and persisted actual decision");
    result=run();check(result.tools.last().decision==PartDecision::Fail && result.tools.last().state==ExecutionState::Done,"fresh draft raw tolerance fail distinct from error");
    auto setup2=setup;setup2.version=2;const auto next=math.validate(math.teachHomography(setup2,mapping),holdout,.0001);catalog.save(next);rejects([&]{runtime.begin();});result=run();check(result.tools.last().state==ExecutionState::Error && result.tools.last().outputs.isEmpty(),"new current invalidates old binding without old mm");
    const auto studyImagePath=dir.filePath("board.png");check(chessboardFixture(true).save(studyImagePath),"study image");const auto test=ChessboardService::importImage(studyImagePath);const auto testMapping=PlaneMappingService::fit(ChessboardService::detect(test.frame,{"test-board",9,6,10}));const auto testCandidate=math.teachHomography({"offline-test",1,"pending",test.frame.metadata().sourceId,"test-only",test.frame.size(),"test-plane",{"image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"test-plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}},testMapping);
    const auto studyPath=dir.filePath("board.vfcalstudy");CalibrationStudyStore::save(studyPath,test,testCandidate);auto study=CalibrationStudyStore::load(studyPath);check(study.calibration.dependency()==testCandidate.dependency() && study.image.contentSha256==test.contentSha256,"offline candidate study exact image/replay reopening");
    std::vector<cv::Point2f> from{{50,50},{650,50},{650,470},{50,470}},to{{85,75},{590,40},{650,455},{40,480}},original{{150,150},{450,150},{210,130},{210,370}},projected;cv::perspectiveTransform(original,projected,cv::getPerspectiveTransform(from,to));
    const CoordinateFrame testCoordinates{"image",LengthUnit::Pixel,AxisConvention::XRightYDown};auto testSample=[&](QString id,int i,double mm){return CalibrationSample{id,{{projected[i].x,projected[i].y},testCoordinates},{{projected[i+1].x,projected[i+1].y},testCoordinates},mm};};const auto acceptedTest=math.validate(testCandidate,{testSample("known-x",0,50),testSample("known-y",2,40)},.1);
    CalibrationStudyStore::save(studyPath,test,acceptedTest);check(CalibrationStudyStore::load(studyPath).calibration.validated(),"offline accepted holdout replay");catalog.save(acceptedTest);rejects([&]{catalog.bind(project,"offline-test");});rejects([&]{CalibrationWorkflow::bind(project,frame,catalogPath,"offline-test","raw_planar_homography");});
    {QLockFile lock(studyPath+".lock");check(lock.tryLock(),"lock fixture");rejects([&]{CalibrationStudyStore::save(studyPath,test,testCandidate);});}
    check(image.save(studyImagePath),"change asset");rejects([&]{CalibrationStudyStore::load(studyPath);});
    std::cout<<"PASS homography typed persistence / Measurement / Runtime / offline study\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
