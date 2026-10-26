#include "application/AssignmentEvaluator.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <QUuid>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
VariableDefinition variable(const QString& name,VariableType type=VariableType::Int64,ScalarUnit unit=ScalarUnit::Unitless) {
    VariableDefinition v;v.id=uuid();v.name=name;v.type=type;v.unit=unit;return v;
}
ValueEnvelope number(qint64 n) {
    ValueEnvelope e;e.type=ScalarType::Int64;e.value=n;e.validity=ValueValidity::Valid;return e;
}
ConstantAssignmentSource constant(qint64 n){return {number(n)};}
void rejected(const AssignmentConfig& c,const AssignmentInputView& i,AssignmentReason expected,int row=-2) {
    const auto result=AssignmentEvaluator::evaluate(c,i);
    check(result.reason==expected,"named rejection reason");
    check(!result.bundle,"failure exposes no partial or previous bundle");
    check(!result.detail.isEmpty(),"failure has diagnostic");
    if(row!=-2)check(result.row==row,"failing row is identified");
}
qint64 read(const AssignmentBundle& b,const QString& id) {
    return std::get<qint64>(*std::get<ValueEnvelope>(b.fields[id]).value);
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        auto a=variable("A"),b=variable("B"),c=variable("C");
        AssignmentInputView input;input.cycleId="cycle-1";input.configId="config-1";input.variables={a,b,c};
        AssignmentConfig config{"set",1,{{a.id,constant(11)},{b.id,constant(22)},{c.id,constant(9007199254740993LL)}}};
        auto result=AssignmentEvaluator::evaluate(config,input);
        check(result.bundle && result.bundle->committed && result.bundle->fields.size()==3,"three assignments commit together");
        check(read(*result.bundle,c.id)==9007199254740993LL,"Int64 retained beyond double precision");
        check(result.bundle->producerId=="set" && result.bundle->revision==1 && result.bundle->cycleId==input.cycleId && result.bundle->configId==input.configId,"bundle identity");
        auto two=config;two.rows.removeLast();check(AssignmentEvaluator::evaluate(two,input).bundle->fields.size()==2,"two assignments commit");
        auto bad=config;bad.rows.last().source=ToolOutputAssignmentSource{"missing","out",1};
        rejected(bad,input,AssignmentReason::MissingSource,2);
        check(read(*result.bundle,a.id)==11,"later failure does not mutate earlier returned snapshot");
        bad=config;bad.rows.last().targetId=a.id;rejected(bad,input,AssignmentReason::DuplicateTarget,2);
        auto system=input;system.systemTargetIds.insert(a.id);rejected(config,system,AssignmentReason::ReadOnlyTarget,0);
        bad=config;bad.rows[0].targetId="system.decision";rejected(bad,input,AssignmentReason::UnknownTarget);
        // The same producer may already have an older revision in the immutable view.
        AssignmentBundle old{"set",input.cycleId,input.configId,1,true,{}};
        auto oldA=number(7);oldA.producerId="set";oldA.producerPort=a.id;oldA.cycleId=input.cycleId;oldA.configId=input.configId;
        old.fields.insert(a.id,oldA);input.bundles.insert("set",old);
        auto sequential=config;sequential.revision=2;sequential.rows={{a.id,constant(99)},{b.id,BundleAssignmentSource{"set",a.id,1}}};
        auto simultaneous=AssignmentEvaluator::evaluate(sequential,input);
        check(simultaneous.bundle && read(*simultaneous.bundle,a.id)==99 && read(*simultaneous.bundle,b.id)==7,"B reads old A from immutable upstream bundle");
        check(read(input.bundles["set"],a.id)==7,"input bundle untouched");
        auto upstream=input;upstream.bundles["set"].revision=3;rejected(sequential,upstream,AssignmentReason::RevisionMismatch);
        upstream=input;upstream.bundles["set"].cycleId="old-cycle";rejected(sequential,upstream,AssignmentReason::CycleMismatch);
        upstream=input;upstream.bundles["set"].configId="old-config";rejected(sequential,upstream,AssignmentReason::ConfigMismatch);
        upstream=input;upstream.bundles["set"].committed=false;rejected(sequential,upstream,AssignmentReason::InvalidUpstream);
        upstream=input;upstream.bundles["set"].producerId="other";rejected(sequential,upstream,AssignmentReason::ProducerMismatch);
        upstream=input;upstream.bundles["set"].fields.clear();rejected(sequential,upstream,AssignmentReason::MissingSource);
        auto output=number(42);output.producerId="measure";output.producerPort="out";output.cycleId=input.cycleId;output.configId=input.configId;
        input.toolOutputs["measure"]["out"]={4,output};
        AssignmentConfig tool{"tool-set",1,{{a.id,ToolOutputAssignmentSource{"measure","out",4}}}};
        auto toolResult=AssignmentEvaluator::evaluate(tool,input);
        check(toolResult.bundle && read(*toolResult.bundle,a.id)==42,"explicit tool output accepted");
        check(std::get<ValueEnvelope>(toolResult.bundle->fields[a.id]).dependencyIds.contains("measure:out"),"explicit source retained");
        auto chained=input;chained.bundles.insert("tool-set",*toolResult.bundle);
        AssignmentConfig next{"next",1,{{b.id,BundleAssignmentSource{"tool-set",a.id,1}}}};
        check(read(*AssignmentEvaluator::evaluate(next,chained).bundle,b.id)==42,"returned bundle feeds an explicit downstream assignment");
        auto invalidField=std::get<ValueEnvelope>(chained.bundles["tool-set"].fields[a.id]);
        invalidField.validity=ValueValidity::Invalid;invalidField.value.reset();invalidField.reason="invalid upstream";
        chained.bundles["tool-set"].fields[a.id]=invalidField;rejected(next,chained,AssignmentReason::InvalidUpstream);
        for(auto state:{ValueValidity::Missing,ValueValidity::Stale,ValueValidity::Skipped,ValueValidity::Invalid,ValueValidity::Error}) {
            auto view=input;auto absent=output;absent.value.reset();absent.validity=state;absent.reason="unavailable";
            view.toolOutputs["measure"]["out"].value=absent;
            rejected(tool,view,state==ValueValidity::Missing?AssignmentReason::MissingSource:state==ValueValidity::Stale?AssignmentReason::StaleSource:AssignmentReason::InvalidUpstream);
        }
        upstream=input;upstream.toolOutputs["measure"]["out"].revision=5;rejected(tool,upstream,AssignmentReason::RevisionMismatch);
        for(int identity=0;identity<3;++identity) {
            upstream=input;auto e=output;
            if(identity==0)e.cycleId="old";if(identity==1)e.configId="old";if(identity==2)e.producerId="wrong";
            upstream.toolOutputs["measure"]["out"].value=e;
            rejected(tool,upstream,identity==0?AssignmentReason::CycleMismatch:identity==1?AssignmentReason::ConfigMismatch:AssignmentReason::ProducerMismatch);
        }
        bad=config;auto wrong=number(1);wrong.type=ScalarType::Float64;wrong.value=1.0;bad.rows[2].source=ConstantAssignmentSource{wrong};rejected(bad,input,AssignmentReason::TypeMismatch,2);
        wrong=number(1);wrong.unit=ScalarUnit::Metre;bad.rows[2].source=ConstantAssignmentSource{wrong};rejected(bad,input,AssignmentReason::UnitMismatch,2);
        wrong=number(1);wrong.value=true;bad.rows[2].source=ConstantAssignmentSource{wrong};rejected(bad,input,AssignmentReason::InvalidUpstream);
        wrong=number(1);wrong.cycleId="old";bad.rows[2].source=ConstantAssignmentSource{wrong};rejected(bad,input,AssignmentReason::InvalidConfig);
        bad=config;bad.rows.clear();rejected(bad,input,AssignmentReason::InvalidConfig);
        bad=config;bad.revision=0;rejected(bad,input,AssignmentReason::InvalidConfig);
        upstream=input;upstream.cycleId.clear();rejected(config,upstream,AssignmentReason::InvalidConfig);
        upstream=input;upstream.variables.append(a);rejected(config,upstream,AssignmentReason::InvalidConfig);
        auto malformed=tool;malformed.rows[0].source=ToolOutputAssignmentSource{"measure","out",0};rejected(malformed,input,AssignmentReason::InvalidConfig);
        malformed=tool;malformed.rows[0].source=ToolOutputAssignmentSource{"measure","missing",4};rejected(malformed,input,AssignmentReason::MissingSource);
        const QVector<ScalarData> scalarData{qint32(-4),quint32(5),3.25,QString::fromUtf8("Çığ 工場")};
        const QVector<VariableType> scalarTypes{VariableType::Int32,VariableType::UInt32,VariableType::Float64,VariableType::String};
        for(int n=0;n<scalarData.size();++n) {
            auto v=variable("scalar",scalarTypes[n]);AssignmentInputView view=input;view.variables={v};
            ValueEnvelope e;e.type=ScalarType(int(v.type));e.value=scalarData[n];e.validity=ValueValidity::Valid;
            AssignmentConfig scalar{"scalar",1,{{v.id,ConstantAssignmentSource{e}}}};
            auto accepted=AssignmentEvaluator::evaluate(scalar,view);
            check(accepted.bundle && std::get<ValueEnvelope>(accepted.bundle->fields[v.id]).value->index()==scalarData[n].index(),"scalar type retained without coercion");
        }
        auto length=variable("length",VariableType::Float64,ScalarUnit::Millimetre);auto lengthView=input;lengthView.variables={length};
        ValueEnvelope metres;metres.type=ScalarType::Float64;metres.value=1.0;metres.validity=ValueValidity::Valid;metres.unit=ScalarUnit::Metre;
        AssignmentConfig lengthConfig{"length",1,{{length.id,ConstantAssignmentSource{metres}}}};
        rejected(lengthConfig,lengthView,AssignmentReason::UnitMismatch);
        auto many=input;AssignmentConfig capacity{"many",1,{}};many.variables.clear();
        for(int n=0;n<64;++n){auto v=variable(QString::number(n));many.variables.append(v);capacity.rows.append({v.id,constant(n)});}
        check(AssignmentEvaluator::evaluate(capacity,many).bundle->fields.size()==64,"64 row boundary");
        auto extra=variable("extra");many.variables.append(extra);capacity.rows.append({extra.id,constant(0)});rejected(capacity,many,AssignmentReason::InvalidConfig);
        auto boolean=variable("boolean",VariableType::Boolean);ValueEnvelope falseValue;falseValue.value=false;falseValue.validity=ValueValidity::Valid;
        input.variables.append(boolean);AssignmentConfig falseConfig{"bool",1,{{boolean.id,ConstantAssignmentSource{falseValue}}}};
        check(!std::get<bool>(*std::get<ValueEnvelope>(AssignmentEvaluator::evaluate(falseConfig,input).bundle->fields[boolean.id]).value),"false remains valid and present");
        auto enumeration=variable("enum",VariableType::Enum);enumeration.enumDefinitionId="kind";enumeration.enumCodes={1,2};input.variables.append(enumeration);
        ValueEnvelope enumValue;enumValue.type=ScalarType::Enum;enumValue.value=EnumValue{"kind",2};enumValue.validity=ValueValidity::Valid;
        AssignmentConfig enumConfig{"enum",1,{{enumeration.id,ConstantAssignmentSource{enumValue}}}};
        check(AssignmentEvaluator::evaluate(enumConfig,input).bundle.has_value(),"declared enum accepted");
        enumValue.value=EnumValue{"kind",3};enumConfig.rows[0].source=ConstantAssignmentSource{enumValue};rejected(enumConfig,input,AssignmentReason::InvalidUpstream);
        enumValue.value=EnumValue{"other",2};enumConfig.rows[0].source=ConstantAssignmentSource{enumValue};rejected(enumConfig,input,AssignmentReason::TypeMismatch);
        FrameRef source{"camera",{},{},uuid(),1},target{{},"robot",{},uuid(),1};
        auto pose=variable("pose",VariableType::SpatialTransform,ScalarUnit::Millimetre);pose.frameConstraint=VariableFrameConstraint{source,target};input.variables.append(pose);
        SpatialAssignmentValue spatial;spatial.value=SpatialTransform{source,target,{1,2,3},{1,0,0,0}};spatial.validity=ValueValidity::Valid;
        AssignmentConfig poseConfig{"pose",1,{{pose.id,ConstantAssignmentSource{spatial}}}};
        check(AssignmentEvaluator::evaluate(poseConfig,input).bundle.has_value(),"frame constrained spatial assignment accepted");
        auto spatialBad=spatial;spatialBad.value->source.revision=2;poseConfig.rows[0].source=ConstantAssignmentSource{spatialBad};rejected(poseConfig,input,AssignmentReason::FrameMismatch);
        spatialBad=spatial;spatialBad.value->rotation.w=0;poseConfig.rows[0].source=ConstantAssignmentSource{spatialBad};rejected(poseConfig,input,AssignmentReason::InvalidUpstream);
        spatialBad=spatial;spatialBad.value->translationMm[0]=std::numeric_limits<double>::infinity();poseConfig.rows[0].source=ConstantAssignmentSource{spatialBad};rejected(poseConfig,input,AssignmentReason::InvalidUpstream);
        for(int r=0;r<=int(AssignmentReason::ProducerMismatch);++r)check(!assignmentReasonKey(AssignmentReason(r)).isEmpty(),"stable named reasons");
        QTextStream(stdout)<<checks<<" assignment checks passed\n";
        return 0;
    } catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
