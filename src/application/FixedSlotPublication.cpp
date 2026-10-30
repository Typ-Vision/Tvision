#include "FixedSlotPublication.hpp"
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

namespace visionflow {
namespace {
void need(bool b, const char *s) { if (!b) throw std::runtime_error(s); }
QString token(const PublicationBinding &b) { return b.producerId + ":" + b.port; }
// Bounded exact unsigned arithmetic: binary64 rational needs at most 1138 bits
// after int32 scale/offset cross multiplication. No FP rounding mode, lossy
// Int64->double conversion, external bigint dependency or unbounded allocation.
struct Wide {
  std::array<quint32, 40> words{};
  explicit Wide(quint64 v = 0) { words[0] = quint32(v); words[1] = quint32(v >> 32); }
  Wide times(quint64 v) const {
    Wide r;
    // Multipliers here are at most 2^33+1.
    quint64 carry = 0;
    for (size_t i = 0; i < words.size(); ++i) {
      // Split the multiplier so each limb product fits uint64.
      const quint64 lo = quint64(words[i]) * quint32(v) + carry;
      r.words[i] = quint32(lo);
      carry = (lo >> 32) + quint64(words[i]) * (v >> 32);
    }
    need(carry == 0, "Numeric capacity exceeded");
    return r;
  }
  Wide shifted(int bits) const {
    Wide r;
    for (int i = 0; i < int(words.size()); ++i) if (words[i]) {
      const int j = i + bits / 32, n = bits % 32;
      need(j < int(words.size()), "Numeric capacity exceeded");
      r.words[j] |= words[i] << n;
      if (n && (words[i] >> (32 - n))) {
        need(j + 1 < int(words.size()), "Numeric capacity exceeded");
        r.words[j + 1] |= words[i] >> (32 - n);
      }
    }
    return r;
  }
  int compare(const Wide &b) const {
    for (int i = int(words.size()) - 1; i >= 0; --i)
      if (words[i] != b.words[i]) return words[i] < b.words[i] ? -1 : 1;
    return 0;
  }
  Wide plus(const Wide &b) const {
    Wide r; quint64 carry = 0;
    for (size_t i = 0; i < words.size(); ++i) {
      const quint64 sum = quint64(words[i]) + b.words[i] + carry;
      r.words[i] = quint32(sum); carry = sum >> 32;
    }
    need(!carry, "Numeric capacity exceeded"); return r;
  }
  Wide minus(const Wide &b) const { // caller guarantees this >= b
    Wide r; quint64 borrow = 0;
    for (size_t i = 0; i < words.size(); ++i) {
      const quint64 sub = quint64(b.words[i]) + borrow;
      r.words[i] = quint32(quint64(words[i]) - sub);
      borrow = quint64(words[i]) < sub;
    }
    return r;
  }
};
quint64 magnitude(qint64 v) { return v < 0 ? quint64(-(v + 1)) + 1 : quint64(v); }
qint32 convert(const ValueEnvelope &e, const PublicationSlot &s) {
  quint64 mantissa = 0; int exponent = 0; bool negative = false;
  std::visit([&](const auto &v) {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, double>) {
      need(std::isfinite(v), "Nonfinite number");
      const auto bits = std::bit_cast<quint64>(v);
      negative = bits >> 63; const int exp = int((bits >> 52) & 2047);
      mantissa = bits & ((quint64(1) << 52) - 1);
      if (exp) mantissa |= quint64(1) << 52;
      exponent = exp ? exp - 1023 - 52 : -1074;
    } else if constexpr (std::is_same_v<T, qint32> || std::is_same_v<T, qint64>) {
      negative = v < 0; mantissa = magnitude(v);
    } else if constexpr (std::is_same_v<T, quint32>) mantissa = v;
    else throw std::runtime_error("Number requires numeric data");
  }, *e.value);
  negative = negative != (s.scaleNumerator < 0);
  Wide a = Wide(mantissa).times(magnitude(s.scaleNumerator))
              .times(s.offsetDenominator).shifted(std::max(exponent, 0));
  Wide b = Wide(magnitude(s.offsetNumerator)).times(s.scaleDenominator)
              .shifted(std::max(-exponent, 0));
  const bool offsetNegative = s.offsetNumerator < 0;
  Wide n;
  if (negative == offsetNegative) n = a.plus(b);
  else if (a.compare(b) >= 0) n = a.minus(b);
  else { n = b.minus(a); negative = offsetNegative; }
  const Wide d = Wide(s.scaleDenominator).times(s.offsetDenominator)
                     .shifted(std::max(-exponent, 0));
  // Find the magnitude floor, bounded just past the signed32 endpoints.
  quint64 lo = 0, hi = quint64(1) << 32;
  need(n.compare(d.times(hi)) < 0, "Number overflow");
  while (lo + 1 < hi) {
    const quint64 mid = (lo + hi) / 2;
    if (n.compare(d.times(mid)) >= 0) lo = mid; else hi = mid;
  }
  const int tie = n.shifted(1).compare(d.times(2 * lo + 1));
  if (tie > 0 || (tie == 0 && (lo & 1))) ++lo;
  const qint64 value = negative ? -qint64(lo) : qint64(lo);
  need(value >= s.minimum && value <= s.maximum, "Number overflow/range");
  return qint32(value);
}
const PublicationSlot &slot(const PublicationTemplate &t, const SlotAssignment &r) {
  switch (r.kind) {
  case PublicationSlotKind::Number: return t.numbers[r.index];
  case PublicationSlotKind::Flag: return t.flags[r.index];
  case PublicationSlotKind::Text: return t.texts[r.index];
  case PublicationSlotKind::Pose: return t.poses[r.index];
  }
  throw std::runtime_error("Invalid slot");
}
int find(const QVector<PublicationBinding> &bs, const PublicationSource &s) {
  for (int i = 0; i < bs.size(); ++i) if (bs[i].definition.source == s) return i;
  throw std::runtime_error("Unresolved DAG source");
}
} // namespace

std::shared_ptr<const FixedSlotPublication> FixedSlotPublication::capture(
    PublicationTemplate t, PublicationAssignments a,
    QVector<PublicationBinding> bs, QString configId) {
  need(!configId.isEmpty() && bs.size() <= 256, "Invalid publication snapshot");
  QVector<PublicationSourceDefinition> catalogue;
  for (const auto &b : bs) {
    catalogue.append(b.definition);
    need(!b.producerId.isEmpty() && !b.port.isEmpty() && b.revision > 0,
         "Publication producer identity/revision required");
    const auto &s = b.definition.source;
    if (s.kind == PublicationSourceKind::ToolOutput)
      need(s.id == b.producerId && s.port == b.port && s.revision == b.revision,
           "Direct source identity mismatch");
    else need(b.port == s.id, "Variable binding must resolve its exact field UUID");
    const bool spatial = b.definition.type == PublicationValueType::SpatialTransform;
    need(spatial == (b.targetFrame.has_value() && b.sourceFrame.has_value()),
         "Explicit spatial frame catalogue required");
    need(spatial || (!b.targetFrame && !b.sourceFrame), "Unexpected spatial metadata");
    if (spatial) { b.targetFrame->validate(); b.sourceFrame->validate(); }
    need(b.dependencies.size() <= 256, "Too many publication dependencies");
  }
  a.validate(t, catalogue);
  QVector<int> marks(bs.size(), 0);
  std::function<void(int)> visit = [&](int i) {
    need(marks[i] != 1, "Publication dependency cycle");
    if (marks[i] == 2) return;
    marks[i] = 1; QSet<QString> seen;
    for (const auto &d : bs[i].dependencies) {
      const int j = find(bs, d); const auto key = token(bs[j]);
      need(!seen.contains(key), "Duplicate publication dependency");
      seen.insert(key); visit(j);
    }
    marks[i] = 2;
  };
  for (int i = 0; i < bs.size(); ++i) visit(i);
  auto result = std::shared_ptr<FixedSlotPublication>(new FixedSlotPublication);
  result->template_ = std::move(t); result->assignments_ = std::move(a);
  result->bindings_ = std::move(bs); result->configId_ = std::move(configId);
  return result;
}

FixedSlotResult FixedSlotPublication::evaluate(const PublicationCycleView &cycle) const {
  FixedSlotResult result; result.execution = cycle.execution;
  // Error/canceled must not carry a previous OK/NOK or authorize valid output.
  result.decision = cycle.execution == PublicationExecution::Done
                        ? cycle.decision : PublicationDecision::NotEvaluated;
  if (cycle.inputs.cycleId.isEmpty() || cycle.inputs.configId != configId_) {
    result.reason = "snapshot_mismatch"; return result;
  }
  if (cycle.execution != PublicationExecution::Done) {
    result.reason = "execution_not_done"; return result;
  }
  QVector<int> state(bindings_.size(), 0);
  QVector<std::optional<AssignmentValue>> values(bindings_.size());
  QVector<QString> errors(bindings_.size());
  std::function<const AssignmentValue &(int)> resolve = [&](int i) -> const AssignmentValue & {
    if (state[i] == 2) { need(values[i].has_value(), qPrintable(errors[i])); return *values[i]; }
    need(state[i] == 0, "Publication cycle"); state[i] = 1;
    const auto &b = bindings_[i];
    try {
      QStringList dependencies;
      for (const auto &d : b.dependencies) {
        const int j = find(bindings_, d); resolve(j); dependencies.append(token(bindings_[j]));
      }
      AssignmentValue value;
      if (b.definition.source.kind == PublicationSourceKind::Variable) {
        const auto bundle = cycle.inputs.bundles.constFind(b.producerId);
        need(bundle != cycle.inputs.bundles.cend(), "Missing variable bundle");
        need(bundle->committed && bundle->producerId == b.producerId &&
             bundle->revision == b.revision && bundle->cycleId == cycle.inputs.cycleId &&
             bundle->configId == configId_, "Variable bundle provenance mismatch");
        const auto field = bundle->fields.constFind(b.port);
        need(field != bundle->fields.cend(), "Missing variable field"); value = *field;
      } else {
        const auto producer = cycle.inputs.toolOutputs.constFind(b.producerId);
        need(producer != cycle.inputs.toolOutputs.cend(), "Missing direct producer");
        const auto output = producer->constFind(b.port);
        need(output != producer->cend(), "Missing direct port");
        need(output->revision == b.revision, "Direct revision mismatch"); value = output->value;
      }
      std::visit([&](const auto &e) {
        need(e.validity == ValueValidity::Valid && e.value && e.reason.isEmpty(), "Invalid/missing source");
        need(e.cycleId == cycle.inputs.cycleId && e.configId == configId_ &&
             e.producerId == b.producerId && e.producerPort == b.port,
             "Value provenance mismatch");
        need(e.frameId == cycle.producerFrames.value(b.producerId), "Acquired frame mismatch");
        auto actual = e.dependencyIds; actual.sort(); dependencies.sort();
        need(actual == dependencies, "DAG provenance mismatch");
        need(e.unit == b.definition.unit, "Value unit mismatch");
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, ValueEnvelope>) {
          need(int(e.type) == int(b.definition.type) && int(e.type) <= int(ScalarType::String),
               "Value type mismatch");
          CommunicationValues::validate(e);
        } else {
          need(b.definition.type == PublicationValueType::SpatialTransform &&
               b.targetFrame && b.sourceFrame, "Whole spatial source required");
          e.value->validate();
          need(e.value->target == *b.targetFrame && e.value->source == *b.sourceFrame,
               "Spatial catalogue frame/revision mismatch");
        }
      }, value);
      values[i] = std::move(value); state[i] = 2; return *values[i];
    } catch (const std::exception &e) {
      errors[i] = QString::fromUtf8(e.what()); state[i] = 2; throw;
    }
  };
  bool requiredInvalid = false;
  for (const auto &r : assignments_.rows) {
    const auto &target = slot(template_, r);
    try {
      const auto &value = resolve(find(bindings_, r.source));
      const quint32 bit = quint32(1) << r.index;
      switch (r.kind) {
      case PublicationSlotKind::Number:
        result.numbers[r.index] = convert(std::get<ValueEnvelope>(value), target);
        result.numberValid |= bit; break;
      case PublicationSlotKind::Flag:
        if (std::get<bool>(*std::get<ValueEnvelope>(value).value)) result.flagValue |= bit;
        result.flagValid |= bit; break;
      case PublicationSlotKind::Text: {
        const auto text = std::get<QString>(*std::get<ValueEnvelope>(value).value);
        validatePublicationText(text, target.textMaxBytes);
        result.texts[r.index] = text.toUtf8(); result.textValid |= bit; break;
      }
      case PublicationSlotKind::Pose:
        result.pose = std::get<SpatialAssignmentValue>(value).value;
        result.poseContract = target.pose; break;
      }
    } catch (const std::exception &e) {
      result.diagnostics.insert(QString::number(int(r.kind)) + ":" + QString::number(r.index),
                                QString::fromUtf8(e.what()));
      requiredInvalid |= target.required;
    }
  }
  result.resultValid = !requiredInvalid;
  if (requiredInvalid) result.reason = "RESULT_INVALID";
  return result;
}
} // namespace visionflow
