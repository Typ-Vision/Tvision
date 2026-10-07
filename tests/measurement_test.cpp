#include "application/MeasurementService.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <QTimeZone>
#include <QUuid>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void reject(F f,const char* message){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw,message);}
ToolResult edge(QPointF p,const CalibrationDependency& d) {
    FrameMetadata m{d.cameraId,"frame","cycle",d.source.id,QDateTime::fromMSecsSinceEpoch(1000,QTimeZone::UTC)};
    ToolResult r;r.state=ExecutionState::Done;r.outputs["point"]={{"point",PortType::Point,"px","image"},p,true,m.frameId,m.cycleId,m};return r;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    const MeasurementService service;
    CalibrationDependency setup{"scale",1,"ignored","camera-source","lens",{1000,800},"plane",{"camera/image",LengthUnit::Pixel,AxisConvention::XRightYDown},{"plane/mm",LengthUnit::Millimetre,AxisConvention::XRightYUp}};
    auto sample=[&](QString id,QPointF a,QPointF b,double mm){return CalibrationSample{id,{a,setup.source},{b,setup.source},mm};};
    const auto teaching=sample("teach",{0,0},{400,0},10);
    const QVector<CalibrationSample> holdout={sample("holdout-x",{10,30},{810,30},20),sample("holdout-y",{40,10},{40,610},15)};
    auto candidate=service.teach(setup,teaching);check(!candidate.validated() && candidate.millimetresPerPixel()==0.025,"known scale teaching is candidate only");
    auto accepted=service.validate(candidate,holdout,0.001);const auto current=accepted.dependency();
    check(accepted.validated() && accepted.validationRmsMillimetres()<1e-12 && current.contentId.size()==64 && current.contentId!=candidate.dependency().contentId,"validation bound into dependency hash");
    check(!candidate.validated(),"candidate immutable");
    auto a=edge({100,100},current),b=edge({500,400},current);
    auto run=[&](const PlanarScaleCalibration& c,const CalibrationDependency& d){return service.distance(a,"point",b,"point",c,d,setup.resolution,12.5,12.5);};
    auto result=run(accepted,current);check(result.state==ExecutionState::Done && result.pixels==500 && result.millimetres==12.5 && result.decision==PartDecision::Pass,"3-4-5 calibrated distance and inclusive tolerance");
    result=run(candidate,candidate.dependency());check(result.state==ExecutionState::Error && result.decision==PartDecision::NotEvaluated && !result.millimetres,"unvalidated no value or decision");
    result=service.distance(a,"point",b,"point",accepted,current,setup.resolution,0,12.4999);check(result.state==ExecutionState::Done && result.decision==PartDecision::Fail,"raw value tolerance not rounded UI text");
    const auto originalB=b;
    for(auto state:{ExecutionState::Error,ExecutionState::Skipped,ExecutionState::Ready,ExecutionState::Busy}){b.state=state;result=run(accepted,current);check(result.state==ExecutionState::Error && !result.pixels && !result.millimetres && result.decision==PartDecision::NotEvaluated,"upstream execution distinct from part result");}b=originalB;
    for(int i=0;i<8;++i) {
        b=originalB;auto& v=b.outputs["point"];
        if(i==0)v.valid=false;if(i==1)v.origin.frameId="foreign";if(i==2)v.origin.cycleId="foreign";if(i==3)v.origin.sourceId="foreign";if(i==4)v.origin.coordinateSystemId="foreign";if(i==5)v.port.unit="mm";if(i==6)v.data=true;if(i==7)v.origin.completedAtUtc={};
        result=run(accepted,current);check(result.state==ExecutionState::Error && result.errorCode=="MEASUREMENT_INPUT_INVALID" && !result.millimetres && result.decision==PartDecision::NotEvaluated,"input mismatch rejects values");
    }b=originalB;
    for(int i=0;i<8;++i) {
        auto changed=current;if(i==0)++changed.version;if(i==1)changed.contentId="changed";if(i==2)changed.cameraId="moved";if(i==3)changed.lensId="lens-2";if(i==4)changed.resolution={800,600};if(i==5)changed.workingPlaneId="plane-2";if(i==6)changed.source.id="other/image";if(i==7)changed.target.axes=AxisConvention::XRightYDown;
        result=run(accepted,changed);check(result.state==ExecutionState::Error && result.errorCode=="CALIBRATION_INVALID" && !result.millimetres,"changed setup invalidates calibration");
    }
    result=service.distance(a,"point",b,"point",accepted,current,{640,480},0,20);check(result.errorCode=="CALIBRATION_INVALID" && !result.millimetres,"actual raw resolution independently checked");
    b.outputs["point"].data=QPointF(1000,400);check(run(accepted,current).state==ExecutionState::Error,"outside pixel centers rejected");b=originalB;
    b.outputs["point"].data=QPointF(std::numeric_limits<double>::quiet_NaN(),400);check(run(accepted,current).state==ExecutionState::Error,"nonfinite point rejected");b=originalB;
    result=service.distance(a,"point",b,"point",accepted,current,setup.resolution,13,12);check(result.errorCode=="TOLERANCE_INVALID" && !result.millimetres,"invalid tolerance no result");
    auto wrong=teaching;wrong.second.position=wrong.first.position;reject([&]{service.teach(setup,wrong);},"zero teaching rejected");
    wrong=teaching;wrong.knownMillimetres=std::numeric_limits<double>::infinity();reject([&]{service.teach(setup,wrong);},"nonfinite known length");
    reject([&]{service.validate(candidate,{holdout[0]},0.001);},"insufficient holdout");
    reject([&]{service.validate(candidate,{teaching,holdout[0]},0.001);},"teaching not holdout");
    wrong=teaching;wrong.id="renamed";reject([&]{service.validate(candidate,{wrong,holdout[0]},0.001);},"renamed teaching geometry not holdout");
    wrong=holdout[0];wrong.id="duplicate";reject([&]{service.validate(candidate,{wrong,holdout[0]},0.001);},"duplicate holdout geometry");
    wrong=holdout[0];wrong.knownMillimetres=21;reject([&]{service.validate(candidate,{wrong,holdout[1]},0.001);},"bad residual rejected");
    auto revised=service.validate(candidate,holdout,0.002);check(revised.dependency().contentId!=current.contentId,"changed acceptance bound changes identity");
    auto revisedSetup=setup;++revisedSetup.version;revisedSetup.lensId="new lens";auto revisedCandidate=service.teach(revisedSetup,teaching);check(!revisedCandidate.validated(),"reteaching requires revalidation");
    // Real engine Edge outputs, not constructed result doubles, feed measurement.
    const auto sourceId=QUuid::createUuid().toString();auto dagSetup=setup;dagSetup.cameraId=sourceId;
    const auto dagCalibration=service.validate(service.teach(dagSetup,teaching),holdout,0.001);
    ToolDefinition source{sourceId,"webcam","Camera","",true,defaultParameters("webcam")};
    ToolDefinition left{QUuid::createUuid().toString(),"edge","Left","",true,defaultParameters("edge")},right{QUuid::createUuid().toString(),"edge","Right","",true,defaultParameters("edge")};right.parameters["direction"]="right_to_left";
    auto definition=withEditorConnections(withEditorTools({}, {source,left,right}),{{sourceId,"image",left.id,"image"},{sourceId,"image",right.id,"image"}});
    cv::Mat pixels(800,1000,CV_8UC1,cv::Scalar(0));pixels(cv::Rect(100,0,400,800)).setTo(255);
    FrameMetadata origin;origin.sourceId=sourceId;origin.coordinateSystemId=setup.source.id;
    const auto frame=ImageFrame::fromBuffer(pixels.data,pixels.total(),1000,800,pixels.step,PixelFormat::Gray8,origin,false);
    const auto cycle=ExecutionService{}.run(definition,{},"measurement","signature",{}, {},frame);
    check(cycle.state==ExecutionState::Done && cycle.tools.size()==3,"real Edge DAG done");
    result=service.distance(cycle.tools[1],"point",cycle.tools[2],"point",dagCalibration,dagCalibration.dependency(),frame.size(),9.97,9.98);
    check(result.state==ExecutionState::Done && result.decision==PartDecision::Pass && std::abs(*result.millimetres-9.975)<1e-12 && result.origin.cycleId==cycle.cycleId,"real Edge to calibrated measurement provenance");
    QTextStream(stdout)<<"PASS: scale teaching, independent evidence, dependency invalidation, provenance, real DAG and raw tolerance\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
