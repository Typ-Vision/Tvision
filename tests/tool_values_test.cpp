#include "application/ToolResultCodec.hpp"
#include "application/ProductionRecipeService.hpp"
#include "application/ToolExecutors.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
PortValue port(PortType type,PortData data,QString unit="px",QString frame="image") {
    PortValue v{{"out",type,unit,frame},std::move(data),true,"frame","cycle",{"source","frame","cycle","source/image"}};
    v.evaluationId="evaluation";v.method="fixture";v.methodVersion=1;return v;
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CoordinateFrame pixels{"source/image"},world{"part/plane",LengthUnit::Millimetre,AxisConvention::XRightYUp};
        const QString unicode=QString::fromUtf8("Çığ-工場")+QChar(0x1d)+QString::fromUcs4(U"😀");
        const CodeResult code{unicode,"QR",QByteArray::fromHex("00ff1d80"),{{1,2},{11,2},{11,12},{1,12}}};
        QVector<PortValue> values{
            port(PortType::Number,5.0,"mm","plane"),port(PortType::Boolean,true,""),port(PortType::Point,QPointF(3,4)),port(PortType::Region,QRect(2,3,4,5)),
            port(PortType::Text,unicode,""),port(PortType::CodeResults,CodeResults{{code,code}}),port(PortType::CodeResults,CodeResults{}),
            port(PortType::PointSet,PointSet{{{0,0},{3,4}},pixels}),port(PortType::PointSet,PointSet{{},world},"mm","plane"),
            port(PortType::Profile,Profile{{{{1,2},200,-5},{{2,2},100,-100}}},"gray8"),
            port(PortType::Line2D,Line2D{{0,0},{3,4},pixels}),port(PortType::Circle2D,Circle2D{{2,3},5,world},"mm","plane"),
            port(PortType::Arc2D,Arc2D{{{2,3},5,pixels},30,-90}),port(PortType::Pose2D,Pose2D{{4,5},0.5,pixels}),
            port(PortType::InspectionResults,InspectionResults{{{"content","match",true},{"count","two",false}}},"")
        };
        for(const auto& v:values) {
            const auto json=ToolResultCodec::encode(v);const auto decoded=ToolResultCodec::decode(json);
            check(ToolResultCodec::encode(decoded)==json,"bounded typed roundtrip");check(!ToolResultCodec::display(decoded).isEmpty(),"all variants display");
            auto wrong=v;wrong.cycleId="foreign";reject([&]{ToolResultCodec::validate(wrong,v.port,"frame","cycle");},"foreign cycle rejected");
            wrong=v;wrong.frameId="foreign";reject([&]{ToolResultCodec::validate(wrong,v.port,"frame","cycle");},"foreign frame rejected");
            wrong=v;wrong.port.unit="invented";reject([&]{ToolResultCodec::validate(wrong,v.port,"frame","cycle");},"fixed unit mismatch");
            wrong=v;wrong.data=false;if(v.port.type!=PortType::Boolean)reject([&]{ToolResultCodec::validate(wrong,v.port,"frame","cycle");},"wrong variant rejected");
            auto altered=json;altered["extra"]=true;reject([&]{ToolResultCodec::decode(altered);},"unknown envelope rejected");
            altered=json;altered["schemaVersion"]=2;reject([&]{ToolResultCodec::decode(altered);},"future envelope rejected");
        }
        const auto c=std::get<CodeResults>(ToolResultCodec::decode(ToolResultCodec::encode(values[5])).data);
        check(c.items.size()==2 && c.items[0].text==unicode && c.items[0].bytes==code.bytes,"same text physical codes retained; binary/Unicode/GS retained");
        auto missing=values[6];missing.valid=false;missing.invalidReason="backend unavailable";
        const auto decodedMissing=ToolResultCodec::decode(ToolResultCodec::encode(missing));
        check(!decodedMissing.valid && decodedMissing.invalidReason==missing.invalidReason,"empty valid list differs from invalid collection");
        missing.invalidReason.clear();reject([&]{ToolResultCodec::encode(missing);},"invalid typed reason required");
        auto bad=values[10];bad.port.unit="mm";reject([&]{ToolResultCodec::encode(bad);},"geometry frame units checked");
        bad=values[10];std::get<Line2D>(bad.data).frame.id="other/image";reject([&]{ToolResultCodec::encode(bad);},"geometry coordinate identity checked");
        bad=values[10];std::get<Line2D>(bad.data).frame.axes=AxisConvention::XRightYUp;reject([&]{ToolResultCodec::encode(bad);},"raw geometry axes checked");
        bad=values[5];bad.method.clear();reject([&]{ToolResultCodec::encode(bad);},"method provenance required");
        reject([&]{ToolValues::encode(Line2D{{1,1},{1,1},pixels});},"degenerate line");
        reject([&]{ToolValues::encode(Circle2D{{1,1},-1,pixels});},"negative radius");
        reject([&]{ToolValues::encode(Arc2D{{{1,1},1,pixels},0,361});},"oversized arc");
        reject([&]{ToolValues::encode(PointSet{{{std::numeric_limits<double>::infinity(),0}},pixels});},"nonfinite geometry");
        reject([&]{ToolValues::encode(QString(ToolValues::maxText+1,'x'));},"oversized text");
        reject([&]{ToolValues::encode(QString(QChar(0xd800)));},"invalid Unicode");
        reject([&]{ToolValues::encode(PointSet{QVector<QPointF>(ToolValues::maxItems+1),pixels});},"oversized point set");
        reject([&]{ToolValues::encode(CodeResults{QVector<CodeResult>(ToolValues::maxCodes+1,code)});},"oversized codes");
        reject([&]{ToolValues::encode(InspectionResults{{{"x","",true},{"x","",false}}});},"duplicate inspection IDs");
        auto json=ToolValues::encode(CodeResults{{code}});auto data=json["data"].toObject();auto items=data["items"].toArray();auto item=items[0].toObject();
        item["bytes"]="%%%";items[0]=item;data["items"]=items;json["data"]=data;reject([&]{ToolValues::decode(json);},"malformed base64");
        json=ToolValues::encode(QString("x"));json["type"]="Unknown";reject([&]{ToolValues::decode(json);},"unknown union");
        check(ToolRegistry::get("measurement").usesCalibration && !ToolRegistry::get("edge").usesCalibration,"calibration capability registry");
        ToolDescriptor descriptor{"typed_fixture","Typed",1,{}, {},{{"out",PortType::Line2D,"px","image"}},[](const ToolDefinition&){}};
        reject([&]{ToolRegistry::registerType(descriptor);},"typed descriptor must opt into schema 7");
        descriptor.minimumEditorSchema=7;ToolRegistry::registerType(descriptor);
        auto a=ToolDefinition{"a","typed_fixture","A","",true,{}};auto b=ToolDefinition{"b","typed_fixture","B","",true,{}};
        ToolDescriptor consumer{"typed_consumer","Consumer",1,{},{{"in",PortType::Line2D,"mm","plane"}},{},[](const ToolDefinition&){}};consumer.minimumEditorSchema=7;ToolRegistry::registerType(consumer);b.kind="typed_consumer";
        check(!validateGraph({a,b},{{"a","out","b","in"}}).valid(),"px/image cannot bind to mm/plane");
        ToolExecutors::registerExecutor("typed_fixture",[](const ToolDefinition& t,const QMap<QString,PortValue>&,const QString&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>&){
            ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;
            PortValue v{{"out",PortType::Line2D,"px","image"},Line2D{{0,0},{3,4},{"fixture/image"}},true,frameId,cycleId,{"fixture",frameId,cycleId,"fixture/image"}};
            v.method="analytical-fixture";v.methodVersion=1;r.outputs["out"]=v;return r;
        });
        a.id="{492dcf8d-a7be-4ff0-9277-904c6f64c729}";
        const auto cycle=ExecutionService{}.run(withEditorTools({}, {a}),{},"recipe","config",{});
        check(cycle.state==ExecutionState::Done && cycle.tools[0].outputs["out"].valid,"typed executor uses same engine");
        check(cycle.tools[0].outputs["out"].evaluationId==cycle.evaluationId,"engine stamps actual evaluation identity");
        ToolDescriptor cachedConsumer{"cached_typed","Cached",1,{},{{"in",PortType::Line2D,"px","image"}},{},[](const ToolDefinition&){}};cachedConsumer.minimumEditorSchema=7;ToolRegistry::registerType(cachedConsumer);
        ToolExecutors::registerExecutor("cached_typed",[](const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&){
            if(!inputs.value("in").diagnostic || std::get<Line2D>(inputs.value("in").data).end!=QPointF(3,4))throw std::runtime_error("typed cache lost payload or diagnostic");ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;return r;
        });
        ToolDefinition consumerTool{"{72903505-38a1-416f-926a-e53905127bd7}","cached_typed","Cached","",true,{}};
        const auto cachedDef=withEditorConnections(withEditorTools({}, {a,consumerTool}),{{a.id,"out",consumerTool.id,"in"}});
        const uchar gray=0;FrameMetadata m{"fixture",cycle.frameId,cycle.cycleId,"fixture/image"};
        TargetEvaluation evaluation{ImageFrame::fromBuffer(&gray,1,1,1,1,PixelFormat::Gray8,m),"scope",{}};
        auto cachedResult=cycle.tools.first();cachedResult.outputs["out"].diagnostic=true;
        evaluation.cache[a.id]={ExecutionService::toolSignature(cachedDef,{},a.id,evaluation.scope),cachedResult};
        const auto target=ExecutionService{}.runTool(cachedDef,{},"recipe","config",{},consumerTool.id,evaluation);
        check(target.state==ExecutionState::Done && target.partial && target.executedTools==QStringList{consumerTool.id} && target.acquisitions==0,"typed diagnostic cache preserves payload without acquisition/upstream run");
        auto changed=editorTools(cachedDef);changed[0].enabled=false;const auto changedDef=withEditorTools(cachedDef,changed);
        check(ExecutionService{}.runTool(changedDef,{},"recipe","config",{},consumerTool.id,evaluation).tools.first().state==ExecutionState::Skipped,"typed cache signature invalidates changed upstream");
        ToolDescriptor worldSource{"world_fixture","World",1,{{"plane","plane-a"}}, {},{{"out",PortType::Line2D,"mm","plane"}},[](const ToolDefinition&){}};worldSource.minimumEditorSchema=7;ToolRegistry::registerType(worldSource);
        ToolDescriptor worldConsumer{"world_consumer","World consumer",1,{},{{"a",PortType::Line2D,"mm","plane"},{"b",PortType::Line2D,"mm","plane"}},{},[](const ToolDefinition&){}};worldConsumer.minimumEditorSchema=7;ToolRegistry::registerType(worldConsumer);
        ToolExecutors::registerExecutor("world_fixture",[](const ToolDefinition& t,const QMap<QString,PortValue>&,const QString&,const QString& f,const QString& c,const std::shared_ptr<std::atomic_bool>&){
            ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;PortValue v{{"out",PortType::Line2D,"mm","plane"},Line2D{{0,0},{3,4},{t.parameters["plane"].toString(),LengthUnit::Millimetre}},true,f,c,{"fixture",f,c,"fixture/image"}};v.method="fixture";v.methodVersion=1;r.outputs["out"]=v;return r;
        });
        ToolExecutors::registerExecutor("world_consumer",[](const ToolDefinition& t,const QMap<QString,PortValue>&,const QString&,const QString&,const QString&,const std::shared_ptr<std::atomic_bool>&){ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;return r;});
        ToolDefinition wa{"{bddffec7-ad2d-4625-9fac-43ba9fcb9704}","world_fixture","WorldA","",true,{{"plane","plane-a"}}};
        ToolDefinition wb{"{d73634c4-c48f-4dbd-b9c5-7f59361ec6df}","world_fixture","WorldB","",true,{{"plane","plane-b"}}};
        ToolDefinition wc{"{71903505-38a1-416f-926a-e53905127bd7}","world_consumer","Consumer","",true,{}};
        auto worldDef=withEditorConnections(withEditorTools({}, {wa,wb,wc}),{{wa.id,"out",wc.id,"a"},{wb.id,"out",wc.id,"b"}});
        auto worldCycle=ExecutionService{}.run(worldDef,{},"recipe","config",{});
        check(worldCycle.state==ExecutionState::Error && worldCycle.tools.last().outputs.isEmpty(),"same raw frame cannot mix two world planes");
        wb.parameters["plane"]="plane-a";worldDef=withEditorTools(worldDef,{wa,wb,wc});
        check(ExecutionService{}.run(worldDef,{},"recipe","config",{}).state==ExecutionState::Done,"same world plane inputs execute");
        ToolDescriptor gated=worldSource;gated.kind="backend_fixture";gated.requiredBackend="missing-reader";ToolRegistry::registerType(gated);wa.kind=gated.kind;
        check(ExecutionService{}.run(withEditorTools({}, {wa}),{},"recipe","config",{}).state==ExecutionState::Error,"required reader gates execution before executor lookup");
        auto big=code;big.text=QString(ToolValues::maxText,'x');big.bytes=QByteArray(ToolValues::maxText,'x');
        reject([&]{ToolValues::encode(CodeResults{QVector<CodeResult>(32,big)});},"total JSON payload bound enforced");
        QTextStream(stdout)<<checks<<" checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}
}
