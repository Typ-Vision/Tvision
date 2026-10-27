#include "application/DecisionEvaluator.hpp"
#include "application/AssignmentEvaluator.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
ValueEnvelope boolean(const QString& producer,bool value) {
    ValueEnvelope e;e.value=value;e.validity=ValueValidity::Valid;
    e.producerId=producer;e.producerPort="accepted";e.cycleId="cycle";e.configId="config";return e;
}
void rejected(const ResultConfig& c,const DecisionInputView& input,DecisionReason reason) {
    const auto r=DecisionEvaluator::evaluate(c,input);
    check(r.decision==ResultDecision::NotEvaluated && r.reason==reason,"Unavailable decision keeps named reason");
    check(!decisionReasonKey(r.reason).isEmpty(),"Diagnostic key exists");
}
template<class F> void throws(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Invalid rule rejected");}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        ResultConfig c{ResultOperator::All,{{"a","accepted",1},{"b","accepted",2}}};
        DecisionInputView input;input.cycleId="cycle";input.configId="config";input.execution=ResultExecutionState::Done;
        input.toolOutputs["a"]["accepted"]={1,boolean("a",false)};
        input.toolOutputs["b"]["accepted"]={2,boolean("b",false)};
        for(auto op:{ResultOperator::All,ResultOperator::Any})for(bool a:{false,true})for(bool b:{false,true}) {
            c.operation=op;input.toolOutputs["a"]["accepted"].value=boolean("a",a);
            input.toolOutputs["b"]["accepted"].value=boolean("b",b);
            const auto r=DecisionEvaluator::evaluate(c,input);
            bool expected=op==ResultOperator::All?(a && b):(a || b);
            check(r.reason==DecisionReason::None && r.decision==(expected?ResultDecision::Ok:ResultDecision::Nok),"ALL/ANY truth table including valid false NOK");
        }
        // Missing-first in both orders: neither ANY(true,missing) nor ALL(false,missing) decides.
        for(auto op:{ResultOperator::All,ResultOperator::Any})for(int absentIndex:{0,1}) {
            c.operation=op;auto view=input;
            for(auto id:{QString("a"),QString("b")})view.toolOutputs[id]["accepted"].value=boolean(id,op==ResultOperator::Any);
            const QString id=absentIndex==0?"a":"b";
            for(auto validity:{ValueValidity::Missing,ValueValidity::Stale,ValueValidity::Skipped,ValueValidity::Invalid,ValueValidity::Error}) {
                auto missing=view;auto& e=missing.toolOutputs[id]["accepted"].value;
                e.value.reset();e.validity=validity;e.reason="Not available";
                const auto r=DecisionEvaluator::evaluate(c,missing);
                check(r.decision==ResultDecision::NotEvaluated && r.row==absentIndex && r.reason!=DecisionReason::None,"All required validity states win over short circuit");
            }
            view.toolOutputs.remove(id);rejected(c,view,DecisionReason::MissingSource);
        }
        c.operation=ResultOperator::All;
        auto wrong=input;wrong.toolOutputs["b"]["accepted"].value.cycleId="old";rejected(c,wrong,DecisionReason::CycleMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.configId="old";rejected(c,wrong,DecisionReason::ConfigMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].revision=3;rejected(c,wrong,DecisionReason::RevisionMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.producerId="a";rejected(c,wrong,DecisionReason::ProducerMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.producerPort="found";rejected(c,wrong,DecisionReason::ProducerMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.type=ScalarType::Float64;wrong.toolOutputs["b"]["accepted"].value.value=1.0;rejected(c,wrong,DecisionReason::TypeMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.unit=ScalarUnit::Millimetre;rejected(c,wrong,DecisionReason::TypeMismatch);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.value=1.0;rejected(c,wrong,DecisionReason::InvalidSource);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.value.reset();rejected(c,wrong,DecisionReason::InvalidSource);
        wrong=input;wrong.toolOutputs["b"]["accepted"].value.reason="contradiction";rejected(c,wrong,DecisionReason::InvalidSource);
        auto empty=c;empty.required.clear();rejected(empty,input,DecisionReason::InvalidConfig);
        auto bad=c;bad.required.append(bad.required.first());rejected(bad,input,DecisionReason::InvalidConfig);
        bad=c;bad.operation=static_cast<ResultOperator>(99);rejected(bad,input,DecisionReason::InvalidConfig);
        bad=c;bad.required[0].revision=0;rejected(bad,input,DecisionReason::InvalidConfig);
        bad=c;while(bad.required.size()<=ResultConfig::maxRequired)bad.required.append({QString::number(bad.required.size()),"accepted",1});rejected(bad,input,DecisionReason::InvalidConfig);
        wrong=input;wrong.cycleId.clear();rejected(c,wrong,DecisionReason::InvalidConfig);
        const auto ok=DecisionEvaluator::evaluate(c,input);
        check(ok.decision==ResultDecision::Ok,"Baseline restored independently after failed evaluations");
        for(auto state:{ResultExecutionState::Incomplete,ResultExecutionState::Error,ResultExecutionState::Canceled}) {
            wrong=input;wrong.execution=state;rejected(c,wrong,DecisionReason::ExecutionUnavailable);
            const auto exported=DecisionEvaluator::forExport(ok,state,true);
            check(exported.decision==ResultDecision::NotEvaluated && !exported.resultValid,"Execution failure/cancel suppresses earlier OK and validity");
        }
        const auto measurement=DecisionEvaluator::forExport(std::nullopt,ResultExecutionState::Done,true);
        check(measurement.decision==ResultDecision::NotEvaluated && measurement.resultValid,"Valid scalar publication needs no decision");
        check(!DecisionEvaluator::forExport(ok,ResultExecutionState::Done,false).resultValid,"Publication required fields determine validity independently");
        wrong=input;wrong.toolOutputs.clear();const auto absent=DecisionEvaluator::evaluate(c,wrong);
        const auto independent=DecisionEvaluator::forExport(absent,ResultExecutionState::Done,true);
        check(independent.decision==ResultDecision::NotEvaluated && independent.resultValid,"Missing rule source does not invalidate independent measurement group");
        const auto encoded=encodeResultConfig(c);
        check(encodeResultConfig(decodeResultConfig(encoded))==encoded,"Strict rule codec roundtrip");
        auto json=encoded;json["operator"]="OK";throws([&]{decodeResultConfig(json);});
        json=encoded;json["constant"]=true;throws([&]{decodeResultConfig(json);});
        json=encoded;json["required"]=QJsonArray{QJsonObject{{"kind","constant"},{"value",true}}};throws([&]{decodeResultConfig(json);});
        json=encoded;json["required"]=QJsonArray{};throws([&]{decodeResultConfig(json);});
        json=encoded;auto row=encoded["required"].toArray()[0].toObject();row["revision"]=1.5;json["required"]=QJsonArray{row};throws([&]{decodeResultConfig(json);});
        // A constant cannot fake the provenance of the selected output.
        wrong=input;auto constant=boolean("a",true);constant.producerId.clear();constant.producerPort.clear();constant.cycleId.clear();constant.configId.clear();wrong.toolOutputs["a"]["accepted"].value=constant;
        check(DecisionEvaluator::evaluate(c,wrong).decision==ResultDecision::NotEvaluated,"Constant OK has no output authority");
        // Existing assignment protection remains the only writable-variable boundary.
        AssignmentInputView assignments;assignments.cycleId="cycle";assignments.configId="config";
        VariableDefinition v;v.id=QUuid::createUuid().toString(QUuid::WithoutBraces);v.name="ordinary";v.type=VariableType::Boolean;
        assignments.variables={v};assignments.systemTargetIds.insert(v.id);
        AssignmentConfig set{"set",1,{{v.id,ConstantAssignmentSource{constant}}}};
        check(AssignmentEvaluator::evaluate(set,assignments).reason==AssignmentReason::ReadOnlyTarget,"System output cannot be overwritten by constant assignment");
        set.rows[0].targetId="system.decision";
        check(!AssignmentEvaluator::evaluate(set,assignments).bundle,"System decision is not a recipe variable");
        check(resultDecisionKey(ResultDecision::Ok)=="OK" && resultDecisionKey(ResultDecision::Nok)=="NOK" && resultDecisionKey(ResultDecision::NotEvaluated)=="NOT_EVALUATED","New decision names preserve v2 separation");
        QTextStream(stdout)<<checks<<" decision checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}
}
