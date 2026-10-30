#include "application/FixedSlotPublication.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <cfenv>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace visionflow;
namespace {
int assertions = 0;
void check(bool b, const char *s) { ++assertions; if (!b) throw std::runtime_error(s); }
template<class F> void rejects(F f) {
  bool threw = false; try { f(); } catch (const std::exception &) { threw = true; }
  check(threw, "Expected capture rejection");
}
QUuid uuid(int n) { return QUuid(QString("00000000-0000-4000-8000-%1").arg(n,12,16,QChar('0'))); }
QString id(int n) { return uuid(n).toString(QUuid::WithoutBraces); }
struct Fixture {
  PublicationPoseContract pose{uuid(1),uuid(2),uuid(3),uuid(4),uuid(5)};
  PublicationTemplate t = PublicationTemplate::create(uuid(6),uuid(7),pose);
  PublicationAssignments a{t.id,t.revision,{}};
  QVector<PublicationBinding> bindings;
  PublicationCycleView cycle;
  bool variable;
  explicit Fixture(bool useVariable = false):variable(useVariable) {
    cycle.inputs.cycleId = "cycle-1"; cycle.inputs.configId = "config-1";
    cycle.execution = PublicationExecution::Done; cycle.decision = PublicationDecision::Nok;
  }
  PublicationSource add(PublicationSlotKind kind, int index, PublicationValueType type, ScalarData data) {
    const int n = 20 + bindings.size();
    PublicationSource source{variable ? PublicationSourceKind::Variable : PublicationSourceKind::ToolOutput,
                             id(n), variable ? QString{} : QString("out"), variable ? 0 : 3};
    PublicationBinding b; b.definition = {source,type,ScalarUnit::Unitless,{}};
    b.producerId = variable ? id(90+n) : source.id;
    b.port = variable ? source.id : source.port; b.revision = 3;
    bindings.append(b); a.rows.append({kind,index,source});
    ValueEnvelope e; e.type = ScalarType(int(type)); e.value = std::move(data);
    e.validity = ValueValidity::Valid; e.cycleId = cycle.inputs.cycleId;
    e.configId = cycle.inputs.configId; e.producerId = b.producerId; e.producerPort = b.port;
    if (variable) cycle.inputs.bundles[b.producerId] = {b.producerId,e.cycleId,e.configId,3,true,{{b.port,e}}};
    else cycle.inputs.toolOutputs[b.producerId][b.port] = {3,e};
    return source;
  }
  AssignmentValue &value(int i) {
    const auto &b = bindings[i];
    return variable ? cycle.inputs.bundles[b.producerId].fields[b.port]
                    : cycle.inputs.toolOutputs[b.producerId][b.port].value;
  }
  ValueEnvelope &scalar(int i) { return std::get<ValueEnvelope>(value(i)); }
  std::shared_ptr<const FixedSlotPublication> capture() const {
    return FixedSlotPublication::capture(t,a,bindings,"config-1");
  }
  FixedSlotResult eval() const { return capture()->evaluate(cycle); }
};
void equivalentSources() {
  std::array<FixedSlotResult,2> results;
  for (int v = 0; v < 2; ++v) {
    Fixture f(v); f.t.numbers[15].required = f.t.flags[31].required = f.t.texts[3].required = true;
    f.add(PublicationSlotKind::Number,15,PublicationValueType::Int32,qint32(0));
    f.add(PublicationSlotKind::Flag,31,PublicationValueType::Boolean,false);
    f.add(PublicationSlotKind::Text,3,PublicationValueType::String,QString{});
    results[v] = f.eval(); const auto &r = results[v];
    check(r.resultValid && r.numberValid == 0x8000 && r.flagValid == 0x80000000u &&
          r.flagValue == 0 && r.textValid == 8, "Zero/false/empty are valid at custom slots");
    check(r.execution == PublicationExecution::Done && r.decision == PublicationDecision::Nok,
          "Publication validity independent from execution and decision");
    check(!r.pose && r.numbers[0] == 0 && r.texts[0].isEmpty(), "Absent optional slots canonical zero");
    auto snapshot = f.capture(); f.t.numbers[15].scaleNumerator = 99;
    f.bindings[0].revision = 55;
    check(snapshot->evaluate(f.cycle).resultValid, "Snapshot independent of mutable caller metadata");
    f.cycle.inputs.cycleId = "cycle-2";
    const auto stale = snapshot->evaluate(f.cycle);
    check(!stale.resultValid && stale.numberValid == 0 && stale.flagValid == 0 && stale.textValid == 0 &&
          stale.reason == "RESULT_INVALID", "New cycle cannot reuse old payload");
  }
  check(results[0].numbers == results[1].numbers && results[0].texts == results[1].texts &&
        results[0].flagValid == results[1].flagValid, "Both sources yield identical slots");
}
void realAssignmentBundle() {
  Fixture f; f.t.numbers[0].required = f.t.numbers[1].required = true;
  const auto direct = f.add(PublicationSlotKind::Number,0,PublicationValueType::Int32,qint32(123));
  const QString variableId = id(150), producer = id(151);
  f.cycle.inputs.variables.append({variableId,"PublishedNumber",VariableType::Int32});
  const AssignmentConfig assignment{producer,4,{{variableId,
      ToolOutputAssignmentSource{direct.id,direct.port,direct.revision}}}};
  const auto assigned = AssignmentEvaluator::evaluate(assignment,f.cycle.inputs);
  check(assigned.bundle.has_value(),"Real Set Variables evaluator commits current direct output");
  f.cycle.inputs.bundles[producer] = *assigned.bundle;
  const PublicationSource variable{PublicationSourceKind::Variable,variableId,{},0};
  f.bindings.append({{variable,PublicationValueType::Int32,ScalarUnit::Unitless,{}},
                     producer,variableId,4,{direct},{},{}});
  f.a.rows.append({PublicationSlotKind::Number,1,variable});
  const auto snapshot = f.capture(); const auto r = snapshot->evaluate(f.cycle);
  check(r.resultValid && r.numbers[0] == 123 && r.numbers[1] == 123 && r.numberValid == 3,
        "Direct and real committed variable share one immutable cycle and DAG");
  f.scalar(0).cycleId = "previous-cycle";
  check(!snapshot->evaluate(f.cycle).resultValid && snapshot->evaluate(f.cycle).numberValid == 0,
        "Valid-looking committed variable cannot bypass stale original dependency");
}
void numeric() {
  Fixture f; f.t.numbers[0].required = true;
  f.add(PublicationSlotKind::Number,0,PublicationValueType::Int64,qint64(0));
  auto integer = [&](qint64 input, qint32 expected) {
    f.scalar(0).value = input; const auto r = f.eval();
    check(r.resultValid && r.numbers[0] == expected, "Exact signed integer conversion");
  };
  integer(INT32_MIN,INT32_MIN); integer(INT32_MAX,INT32_MAX);
  for (auto value : {qint64(INT32_MIN)-1,qint64(INT32_MAX)+1,INT64_MIN,INT64_MAX}) {
    f.scalar(0).value = value; check(!f.eval().resultValid, "Signed overflow invalid");
  }
  f.t.numbers[0].scaleDenominator = INT32_MAX;
  integer(qint64(INT32_MAX)*INT32_MAX,INT32_MAX);
  integer(-qint64(INT32_MAX)*INT32_MAX,-INT32_MAX);
  f.t.numbers[0].scaleDenominator = 2;
  integer(1,0); integer(3,2); integer(5,2); integer(-1,0); integer(-3,-2); integer(-5,-2);
  f.t.numbers[0].scaleNumerator = -1; integer(3,-2);
  f.t.numbers[0].scaleNumerator = 1; f.t.numbers[0].offsetNumerator = 1;
  f.t.numbers[0].offsetDenominator = 2; integer(2,2); integer(-2,0);
  // Cancellation beyond double's integer precision must retain exact residual.
  f.t.numbers[0].scaleDenominator = INT32_MAX;
  f.t.numbers[0].offsetNumerator = -INT32_MAX; f.t.numbers[0].offsetDenominator = 1;
  integer(qint64(INT32_MAX)*INT32_MAX+1,0);
  // Independent bounded integer rational oracle over deterministic signed inputs.
  quint32 seed = 12345;
  for (int n=0;n<400;++n) {
    seed = seed*1664525u+1013904223u;
    const qint64 value = qint64(seed % 200001) - 100000;
    auto &s = f.t.numbers[0];
    s.scaleNumerator = n%19-9; if (!s.scaleNumerator) s.scaleNumerator = 1;
    s.scaleDenominator = n%17+1; s.offsetNumerator = n%13-6; s.offsetDenominator = n%7+1;
    qint64 numerator = value*s.scaleNumerator*s.offsetDenominator + qint64(s.offsetNumerator)*s.scaleDenominator;
    const qint64 denominator = qint64(s.scaleDenominator)*s.offsetDenominator;
    const bool negative = numerator < 0; if (negative) numerator = -numerator;
    qint64 rounded = numerator/denominator, remainder = numerator%denominator;
    if (2*remainder > denominator || (2*remainder == denominator && (rounded&1))) ++rounded;
    integer(value,qint32(negative ? -rounded : rounded));
  }

  Fixture d; d.t.numbers[0].required = true;
  d.add(PublicationSlotKind::Number,0,PublicationValueType::Float64,0.0);
  auto real = [&](double input, qint32 expected) {
    d.scalar(0).value = input; const auto r = d.eval();
    check(r.resultValid && r.numbers[0] == expected, "Exact binary64 conversion");
  };
  for (int mode : {FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
    std::fesetround(mode);
    real(0.5,0); real(1.5,2); real(2.5,2); real(-1.5,-2); real(-2.5,-2);
    real(std::nextafter(0.5,0.0),0); real(std::nextafter(0.5,1.0),1);
  }
  std::fesetround(FE_TONEAREST);
  real(std::numeric_limits<double>::denorm_min(),0); real(-0.0,0);
  real(INT32_MIN,INT32_MIN); real(INT32_MAX,INT32_MAX);
  for (double x : {std::numeric_limits<double>::max(),std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::quiet_NaN(),2147483647.5,-2147483648.75}) {
    d.scalar(0).value = x; check(!d.eval().resultValid, "Nonfinite/overflow cannot wrap or saturate");
  }
  d.t.numbers[0].scaleNumerator = 3; d.t.numbers[0].scaleDenominator = 10;
  d.t.numbers[0].offsetNumerator = 1; d.t.numbers[0].offsetDenominator = 5;
  real(1.0,0); real(11.0,4); real(-9.0,-2);
  d.t.numbers[0].minimum = 0; real(-1.0,0);
  d.scalar(0).value = -10.0; check(!d.eval().resultValid, "Declared slot range enforced");
  Fixture u; u.t.numbers[0].required = true;
  u.add(PublicationSlotKind::Number,0,PublicationValueType::UInt32,quint32(UINT32_MAX));
  check(!u.eval().resultValid, "Unsigned-to-signed overflow invalid");
}
void provenance() {
  for (bool variable : {false,true}) {
    Fixture f(variable); f.t.numbers[0].required = true;
    f.add(PublicationSlotKind::Number,0,PublicationValueType::Int32,qint32(42));
    const auto original = f.cycle;
    for (int n = 0; n < 12; ++n) {
      f.cycle = original; auto &e = f.scalar(0);
      switch(n) {
      case 0: e.cycleId = "other"; break;
      case 1: e.configId = "other"; break;
      case 2: e.producerId = "other"; break;
      case 3: e.producerPort = "other"; break;
      case 4: e.frameId = "old-frame"; break;
      case 5: e.unit = ScalarUnit::Metre; break;
      case 6: e.type = ScalarType::UInt32; break;
      case 7: e.validity = ValueValidity::Stale; break;
      case 8: e.validity = ValueValidity::Skipped; break;
      case 9: e.value.reset(); break;
      case 10: e.dependencyIds.append("undeclared:port"); break;
      case 11: e.reason = "failed"; break;
      }
      const auto r = f.eval();
      check(!r.resultValid && r.numberValid == 0 && r.numbers[0] == 0,
            "Exact value provenance required for both source forms");
    }
    f.cycle = original;
    if (variable) f.cycle.inputs.bundles[f.bindings[0].producerId].committed = false;
    else f.cycle.inputs.toolOutputs[f.bindings[0].producerId][f.bindings[0].port].revision++;
    check(!f.eval().resultValid, "Uncommitted bundle/wrong direct revision rejected");
    f.cycle = original;
    if (variable) f.cycle.inputs.bundles.clear(); else f.cycle.inputs.toolOutputs.clear();
    check(!f.eval().resultValid, "Missing source has no previous-cycle fallback");
    f.cycle = original; f.cycle.inputs.configId = "other";
    check(f.eval().reason == "snapshot_mismatch", "Snapshot config fence");
    f.cycle = original; f.t.numbers[0].required = false; f.scalar(0).value.reset();
    check(f.eval().resultValid && f.eval().numberValid == 0, "Optional invalid does not invalidate result");
    f.cycle = original;
    for (auto execution : {PublicationExecution::Unknown,PublicationExecution::Error,PublicationExecution::Canceled}) {
      f.cycle.execution = execution; f.cycle.decision = PublicationDecision::Ok;
      const auto r = f.eval();
      check(!r.resultValid && r.numberValid == 0 && r.decision == PublicationDecision::NotEvaluated &&
            r.execution == execution, "Failed execution preserves status without stale decision/slots");
    }
  }
  Fixture f; f.t.numbers[0].required = true;
  auto first = f.add(PublicationSlotKind::Number,0,PublicationValueType::Int32,qint32(7));
  auto second = f.add(PublicationSlotKind::Number,1,PublicationValueType::Int32,qint32(8));
  f.bindings[0].dependencies.append(second);
  f.scalar(0).dependencyIds.append(f.bindings[1].producerId+":"+f.bindings[1].port);
  check(f.eval().resultValid, "Declared acyclic dependency resolves current upstream");
  f.scalar(1).cycleId = "old";
  check(!f.eval().resultValid && f.eval().numberValid == 0, "Stale upstream invalidates dependent slot");
  f.bindings[1].dependencies.append(first);
  rejects([&]{f.capture();});
  f.bindings[1].dependencies.clear(); f.bindings[0].dependencies.append(second);
  rejects([&]{f.capture();});
  f.bindings[0].dependencies = {{PublicationSourceKind::ToolOutput,id(99),"x",1}};
  rejects([&]{f.capture();});
  Fixture frame; frame.t.numbers[0].required = true;
  frame.add(PublicationSlotKind::Number,0,PublicationValueType::Int32,qint32(9));
  frame.cycle.producerFrames[frame.bindings[0].producerId] = "acquired-frame-1";
  frame.scalar(0).frameId = "acquired-frame-1";
  check(frame.eval().resultValid,"Exact source-specific acquired frame accepted");
  frame.cycle.producerFrames[frame.bindings[0].producerId] = "acquired-frame-2";
  check(!frame.eval().resultValid,"Frame from previous acquisition rejected");
  frame.t.revision = uuid(200); rejects([&]{frame.capture();});
  Fixture partial; partial.t.numbers[0].required = true;
  partial.add(PublicationSlotKind::Number,0,PublicationValueType::Int32,qint32(8));
  partial.add(PublicationSlotKind::Flag,0,PublicationValueType::Boolean,true);
  partial.scalar(0).value.reset(); const auto diagnostic = partial.eval();
  check(!diagnostic.resultValid && diagnostic.flagValid == 1 && diagnostic.flagValue == 1 &&
        diagnostic.execution == PublicationExecution::Done && diagnostic.decision == PublicationDecision::Nok,
        "Required invalid retains valid diagnostics and separate execution/decision");
}
void textAndPose() {
  Fixture f; f.t.texts[0].required = true;
  f.add(PublicationSlotKind::Text,0,PublicationValueType::String,QString(32,QChar(0x00e9)));
  check(f.eval().resultValid && f.eval().texts[0].size() == 64, "UTF8 byte boundary");
  f.scalar(0).value = QString(65,'a'); check(!f.eval().resultValid, "No text truncation");
  f.scalar(0).value = QString(QChar(0xd800)); check(!f.eval().resultValid, "Malformed Unicode rejected");
  f.scalar(0).value = QString{}; check(f.eval().resultValid, "Empty text remains valid");
  Fixture p; p.t.poses[0].required = true;
  FrameRef target{id(70),{},{},id(71),4}, source{{},id(72),{},id(73),5};
  PublicationSource s{PublicationSourceKind::ToolOutput,id(74),"transform",2};
  PublicationBinding b{{s,PublicationValueType::SpatialTransform,ScalarUnit::Millimetre,p.pose},
                        s.id,s.port,2,{},target,source};
  p.bindings.append(b); p.a.rows.append({PublicationSlotKind::Pose,0,s});
  SpatialTransform transform{source,target,{1.25,-2.5,7.0},{0.5,0.5,0.5,0.5}};
  SpatialAssignmentValue v; v.value = transform; v.validity = ValueValidity::Valid;
  v.producerId = s.id; v.producerPort = s.port; v.cycleId = "cycle-1"; v.configId = "config-1";
  p.cycle.inputs.toolOutputs[s.id][s.port] = {2,v};
  const auto r = p.eval();
  check(r.resultValid && r.pose && r.pose->toJson() == transform.toJson() && r.poseContract == p.pose,
        "Whole pose preserves translation wxyz and explicit UUID revision catalogue");
  auto &actual = std::get<SpatialAssignmentValue>(p.value(0));
  actual.value->source.revision++;
  check(!p.eval().resultValid && !p.eval().pose, "Wrong geometric frame revision invalid");
  actual.value = transform; actual.value->rotation.w = 10;
  check(!p.eval().resultValid, "Invalid quaternion rejected");
  actual.value.reset(); actual.validity = ValueValidity::Missing;
  check(!p.eval().resultValid && !p.eval().pose, "Missing pose cannot fabricate identity");
  p.bindings[0].definition.type = PublicationValueType::Pose2D;
  rejects([&]{p.capture();});
  p.bindings[0] = b; p.bindings[0].definition.pose.adapterContract = uuid(100);
  rejects([&]{p.capture();});
}
} // namespace
int main(int argc,char **argv) {
  QCoreApplication app(argc,argv);
  try {
    equivalentSources(); realAssignmentBundle(); numeric(); provenance(); textAndPose();
    // Repeated Editor previews have only an in-memory return value. The API has
    // no socket, ledger, callback, persistence or terminal-commit capability.
    Fixture preview; preview.add(PublicationSlotKind::Flag,0,PublicationValueType::Boolean,true);
    const auto frozen = preview.capture(); const auto input = encodeAssignmentValue(preview.value(0));
    for (int i=0;i<100;++i) check(frozen->evaluate(preview.cycle).flagValue == 1,"Pure repeated preview");
    check(encodeAssignmentValue(preview.value(0)) == input,"Preview leaves immutable inputs unchanged");
    QTextStream(stdout) << assertions << " assertions passed; fixture comm4-12-fixed-slots-v1\n";
    return 0;
  } catch (const std::exception &e) { QTextStream(stderr) << e.what() << '\n'; return 1; }
}
