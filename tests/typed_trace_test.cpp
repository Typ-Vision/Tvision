#include "application/ToolResultCodec.hpp"
#include "application/ProductionResultCodec.hpp"
#include "application/ProductionStore.hpp"
#include <QTemporaryDir>
#include <QDir>
#include <QUuid>
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
        auto invalid=values[5];invalid.valid=false;invalid.invalidReason="no geometry";values.append(invalid);values[0].diagnostic=true;
        QJsonObject outputs;for(int i=0;i<values.size();++i){values[i].port.id=QString("v%1").arg(i);outputs[values[i].port.id]=ProductionResultCodec::output(values[i]);}
        QJsonObject record{{"schemaVersion",3},{"cycleId","cycle"},{"evaluationId","evaluation"},{"recipeId","recipe"},{"tools",QJsonArray{QJsonObject{{"toolId","t"},{"name","fixture"},{"state","DONE"},{"decision","NOT_EVALUATED"},{"outputs",outputs}}}}};
        QTemporaryDir dir(QDir::currentPath()+"/typed-trace-XXXXXX");check(dir.isValid(),"build fixture");
        const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);ProductionStore(dir.path()).append("cycles",id,record);
        const auto read=ProductionStore(dir.path()).recent("cycles").first();check(read==record,"all typed results survive restart exact");
        check(ProductionResultCodec::summary(read).contains("diagnostic") && ProductionResultCodec::summary(read).contains("no geometry"),"diagnostic and invalid reason visible");
        const auto csv=ProductionResultCodec::csv({read});check(csv.contains("evaluation") && csv.contains("no geometry") && csv.contains("methodVersion"),"CSV includes typed provenance and invalid reason");
        auto wrong=record;wrong["schemaVersion"]=5;reject([&]{ProductionResultCodec::validate(wrong);},"future cycle schema rejected");
        auto bad=outputs["v5"].toObject();bad["value"]=QJsonObject{};auto broken=outputs;broken["v5"]=bad;auto tool=record["tools"].toArray().first().toObject();tool["outputs"]=broken;wrong=record;wrong["tools"]=QJsonArray{tool};reject([&]{ProductionResultCodec::validate(wrong);},"outer/typed payload tampering rejected");
        wrong=record;wrong["cycleId"]="foreign";reject([&]{ProductionResultCodec::validate(wrong);},"foreign cycle replay rejected");
        wrong=record;wrong["evaluationId"]="foreign";reject([&]{ProductionResultCodec::validate(wrong);},"foreign evaluation rejected");
        tool=record["tools"].toArray().first().toObject();tool["decision"]="PASS";wrong=record;wrong["tools"]=QJsonArray{tool};reject([&]{ProductionResultCodec::validate(wrong);},"diagnostic cannot be PASS");
        QJsonObject legacy{{"schemaVersion",2},{"cycleId","old"},{"tools",QJsonArray{QJsonObject{{"outputs",QJsonObject{{"value",QJsonObject{{"type","Number"},{"value",1.0}}}}}}}}};ProductionResultCodec::validate(legacy);check(!ProductionResultCodec::csv({legacy}).isEmpty(),"legacy scalar evidence readable");
        wrong=record;wrong["error"]=QString(ProductionResultCodec::maxRecordBytes,'x');reject([&]{ProductionResultCodec::validate(wrong);},"total result limit rejected");
        QTextStream(stdout)<<checks<<" trace checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
