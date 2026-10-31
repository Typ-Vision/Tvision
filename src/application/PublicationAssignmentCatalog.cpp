#include "PublicationAssignmentCatalog.hpp"
#include "domain/ToolRegistry.hpp"
#include <QSet>
#include <algorithm>
#include <stdexcept>

namespace visionflow {
namespace {
QString typeName(PublicationValueType type) {
  switch (type) {
  case PublicationValueType::Boolean: return "Boolean";
  case PublicationValueType::Int32: return "Int32";
  case PublicationValueType::UInt32: return "UInt32";
  case PublicationValueType::Int64: return "Int64";
  case PublicationValueType::Float64: return "Float64";
  case PublicationValueType::String: return "String";
  case PublicationValueType::SpatialTransform: return "SpatialTransform";
  case PublicationValueType::Point2D: return "Point2D";
  case PublicationValueType::Pose2D: return "Pose2D";
  }
  return "?";
}
QString unitName(ScalarUnit unit) {
  switch (unit) {
  case ScalarUnit::Unitless: return "unitless";
  case ScalarUnit::Millimetre: return "mm";
  case ScalarUnit::Metre: return "m";
  case ScalarUnit::Radian: return "rad";
  case ScalarUnit::Degree: return "degree";
  }
  return "?";
}
bool numeric(PublicationValueType type) {
  return type >= PublicationValueType::Int32 &&
         type <= PublicationValueType::Float64;
}
const PublicationSlot &slotAt(const PublicationTemplate &t,
                             PublicationSlotKind kind, int index) {
  switch (kind) {
  case PublicationSlotKind::Number: return t.numbers[index];
  case PublicationSlotKind::Flag: return t.flags[index];
  case PublicationSlotKind::Text: return t.texts[index];
  case PublicationSlotKind::Pose: return t.poses[index];
  }
  throw std::runtime_error("Invalid slot kind");
}
int slotCount(PublicationSlotKind kind) {
  switch (kind) {
  case PublicationSlotKind::Number: return 16;
  case PublicationSlotKind::Flag: return 32;
  case PublicationSlotKind::Text: return 4;
  case PublicationSlotKind::Pose: return 1;
  }
  return 0;
}
QString shortUuid(const QUuid &id) {
  const auto text = id.toString(QUuid::WithoutBraces);
  return text.left(8) + "…";
}
// Metadata comparison mirrors the domain authority so each table field can show
// its own reason while final Apply still uses PublicationAssignments::validate.
QString metadataReason(PublicationSlotKind kind, const PublicationSlot &slot,
                       const PublicationSourceOption &option) {
  if (!option.blockReason.isEmpty()) return option.blockReason;
  switch (kind) {
  case PublicationSlotKind::Number:
    if (!numeric(option.type)) return "Number için sayısal kaynak gerekir (" + typeName(option.type) + ")";
    if (option.unit != slot.unit) return "Birim uyuşmaz: " + unitName(option.unit) + " → " + unitName(slot.unit);
    return {};
  case PublicationSlotKind::Flag:
    if (option.type != PublicationValueType::Boolean || option.unit != ScalarUnit::Unitless)
      return "Flag Boolean/unitless kaynak ister";
    return {};
  case PublicationSlotKind::Text:
    if (option.type != PublicationValueType::String || option.unit != ScalarUnit::Unitless)
      return "Text String/unitless kaynak ister";
    return {};
  case PublicationSlotKind::Pose:
    if (option.type != PublicationValueType::SpatialTransform || option.unit != ScalarUnit::Millimetre)
      return "Pose SpatialTransform mm kaynak ister";
    if (option.pose != slot.pose)
      return "Pose frame/revision/adapter sözleşmesi uyuşmaz";
    return {};
  }
  return {};
}
PublicationCycleView buildCycleView(const CycleResult &result,
                                    const QVector<PublicationBinding> &bs,
                                    const QJsonObject &definition,
                                    PublicationDecision decision) {
  PublicationCycleView view;
  view.inputs.cycleId = result.cycleId;
  view.inputs.configId = QString::fromLatin1(result.configId.toHex());
  view.inputs.variables = recipeVariables(definition);
  view.execution = result.state == ExecutionState::Done
                       ? PublicationExecution::Done
                       : result.state == ExecutionState::Skipped
                             ? PublicationExecution::Canceled
                             : PublicationExecution::Error;
  view.decision = decision;
  for (const auto &tool : result.tools)
    if (tool.state == ExecutionState::Done)
      for (const auto &port : tool.outputs)
        if (port.valid)
          if (const auto *bundle = std::get_if<AssignmentBundle>(&port.data))
            view.inputs.bundles[tool.toolId] = *bundle;
  for (const auto &b : bs) {
    if (b.definition.source.kind == PublicationSourceKind::Variable) {
      const auto bundle = view.inputs.bundles.value(b.producerId);
      if (bundle.fields.contains(b.port))
        std::visit([&](const auto &value) { view.producerFrames[b.producerId] = value.frameId; },
                   bundle.fields[b.port]);
      continue;
    }
    for (const auto &tool : result.tools)
      if (tool.toolId == b.producerId && tool.state == ExecutionState::Done) {
        const auto port = tool.outputs.value(b.port);
        if (!port.valid) continue;
        ValueEnvelope e;
        e.validity = ValueValidity::Valid;
        e.unit = b.definition.unit;
        e.cycleId = port.cycleId;
        e.configId = view.inputs.configId;
        e.frameId = port.frameId;
        e.producerId = b.producerId;
        e.producerPort = b.port;
        if (const auto *v = std::get_if<double>(&port.data)) {
          e.type = ScalarType::Float64; e.value = *v;
        } else if (const auto *v = std::get_if<bool>(&port.data)) {
          e.type = ScalarType::Boolean; e.value = *v;
        } else if (const auto *v = std::get_if<QString>(&port.data)) {
          e.type = ScalarType::String; e.value = *v;
        } else continue;
        view.inputs.toolOutputs[b.producerId][b.port] = {b.revision, e};
        view.producerFrames[b.producerId] = port.frameId;
      }
  }
  return view;
}
} // namespace

QString publicationSlotKindName(PublicationSlotKind kind) {
  switch (kind) {
  case PublicationSlotKind::Number: return "Number";
  case PublicationSlotKind::Flag: return "Flag";
  case PublicationSlotKind::Text: return "Text";
  case PublicationSlotKind::Pose: return "Pose";
  }
  return "?";
}
QString publicationSlotDisplayName(PublicationSlotKind kind, int index,
                                   const PublicationSlot &slot) {
  const auto base = publicationSlotKindName(kind);
  const auto reserved = kind == PublicationSlotKind::Pose ? base : base + QString::number(index);
  // Default names already carry the slot code; only custom names need it added.
  return slot.name == reserved ? slot.name : slot.name + " • " + reserved;
}
QString publicationSlotContractText(PublicationSlotKind kind,
                                    const PublicationSlot &slot) {
  switch (kind) {
  case PublicationSlotKind::Number:
    return QString("Int32 • %1 • scale %2/%3 • offset %4/%5 • [%6, %7] • tiesToEven")
        .arg(unitName(slot.unit))
        .arg(slot.scaleNumerator)
        .arg(slot.scaleDenominator)
        .arg(slot.offsetNumerator)
        .arg(slot.offsetDenominator)
        .arg(slot.minimum)
        .arg(slot.maximum);
  case PublicationSlotKind::Flag: return "Boolean • unitless";
  case PublicationSlotKind::Text:
    return QString("UTF-8 • max %1 byte").arg(slot.textMaxBytes);
  case PublicationSlotKind::Pose:
    return QString("SpatialTransform mm • hedef %1 r%2 ← kaynak %3 r%4 • adapter %5")
        .arg(shortUuid(slot.pose.targetFrame))
        .arg(shortUuid(slot.pose.targetRevision))
        .arg(shortUuid(slot.pose.sourceFrame))
        .arg(shortUuid(slot.pose.sourceRevision))
        .arg(shortUuid(slot.pose.adapterContract));
  }
  return {};
}

QString publicationSlotContractSummary(PublicationSlotKind kind,
                                       const PublicationSlot &slot) {
  switch (kind) {
  case PublicationSlotKind::Number:
    return QString("Int32 %1 [%2…%3]").arg(unitName(slot.unit)).arg(slot.minimum).arg(slot.maximum);
  case PublicationSlotKind::Flag: return "Boolean";
  case PublicationSlotKind::Text: return QString("UTF-8 ≤%1B").arg(slot.textMaxBytes);
  case PublicationSlotKind::Pose: return "Pose mm";
  }
  return {};
}

QVector<PublicationSourceOption>
publicationSourceOptions(const QJsonObject &definition) {
  QVector<PublicationSourceOption> out;
  for (const auto &variable : recipeVariables(definition)) {
    PublicationSourceOption option;
    option.source = {PublicationSourceKind::Variable, variable.id, {}, 0};
    option.scope = "Variable";
    option.unit = variable.unit;
    switch (variable.type) {
    case VariableType::Boolean: option.type = PublicationValueType::Boolean; break;
    case VariableType::Int32: option.type = PublicationValueType::Int32; break;
    case VariableType::UInt32: option.type = PublicationValueType::UInt32; break;
    case VariableType::Int64: option.type = PublicationValueType::Int64; break;
    case VariableType::Float64: option.type = PublicationValueType::Float64; break;
    case VariableType::String: option.type = PublicationValueType::String; break;
    case VariableType::Enum:
      option.type = PublicationValueType::Int32;
      option.blockReason = "Enum için açık Int32 eşlemesi yok";
      break;
    case VariableType::SpatialTransform:
      option.type = PublicationValueType::SpatialTransform;
      option.unit = ScalarUnit::Millimetre;
      option.blockReason = "Whole Pose adapter/kontrat eşlemesi COMM4-19";
      break;
    }
    option.label = variable.name + " • Variable • " + typeName(option.type) +
                   " / " + unitName(option.unit);
    option.detail = variable.id;
    out.append(option);
  }
  for (const auto &tool : editorTools(definition)) {
    if (!tool.enabled || tool.kind == "set_variables" || tool.kind == "publish_data")
      continue;
    const auto descriptor = ToolRegistry::get(tool.kind);
    for (const auto &port : descriptor.outputs) {
      if (port.type != PortType::Number && port.type != PortType::Boolean &&
          port.type != PortType::Text)
        continue;
      PublicationSourceOption option;
      option.source = {PublicationSourceKind::ToolOutput, tool.id, port.id,
                       descriptor.version};
      option.scope = "Tool";
      option.type = port.type == PortType::Number
                        ? PublicationValueType::Float64
                        : port.type == PortType::Boolean
                              ? PublicationValueType::Boolean
                              : PublicationValueType::String;
      try {
        option.unit = publicationUnit(port.unit);
      } catch (const std::exception &e) {
        option.unit = ScalarUnit::Unitless;
        option.blockReason = QString::fromUtf8(e.what());
      }
      option.label = tool.name + " / " + port.id + " • Tool • " +
                     typeName(option.type) + " / " + unitName(option.unit);
      option.detail = tool.id + " / " + port.id;
      out.append(option);
    }
  }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.scope != b.scope) return a.scope < b.scope;
    return a.label < b.label;
  });
  return out;
}

PublicationPreview previewPublication(const PublicationTemplate &t,
                                      const PublicationAssignments &a,
                                      const QJsonObject &definition,
                                      const CycleResult *cycle,
                                      PublicationDecision decision) {
  PublicationPreview preview;
  const auto options = publicationSourceOptions(definition);
  auto optionFor = [&](const PublicationSource &source) -> const PublicationSourceOption * {
    for (const auto &option : options)
      if (option.source == source) return &option;
    return nullptr;
  };
  QVector<PublicationSourceDefinition> catalogue;
  for (const auto &option : options)
    if (option.blockReason.isEmpty())
      catalogue.append({option.source, option.type, option.unit, option.pose});
  bool metadataValid = true;
  std::optional<FixedSlotResult> fixed;
  try {
    a.validate(t, catalogue);
  } catch (const std::exception &e) {
    metadataValid = false;
    preview.errors.append(QString::fromUtf8(e.what()));
  }
  const auto bs = [&] {
    try { return publicationBindings(definition); }
    catch (const std::exception &e) {
      preview.errors.append(QString::fromUtf8(e.what()));
      return QVector<PublicationBinding>{};
    }
  }();
  if (cycle && metadataValid && !bs.isEmpty()) {
    try {
      const auto view = buildCycleView(*cycle, bs, definition, decision);
      fixed = FixedSlotPublication::capture(t, a, bs, view.inputs.configId)
                  ->evaluate(view);
    } catch (const std::exception &e) {
      preview.errors.append(QString::fromUtf8(e.what()));
    }
  }
  for (int k = 0; k < 4; ++k) {
    const auto kind = PublicationSlotKind(k);
    for (int index = 0; index < slotCount(kind); ++index) {
      const auto &slot = slotAt(t, kind, index);
      PublicationSlotRow row;
      row.kind = kind; row.index = index; row.name = slot.name;
      row.required = slot.required;
      row.contract = publicationSlotContractText(kind, slot);
      const auto key = QString::number(k) + ":" + QString::number(index);
      const SlotAssignment *match = nullptr;
      for (const auto &assigned : a.rows)
        if (assigned.kind == kind && assigned.index == index) match = &assigned;
      if (!match) {
        row.assigned = false;
        row.valid = !slot.required;
        row.status = slot.required ? "✗ Gerekli slot atanmadı" : "Opsiyonel • atanmadı";
        row.badge = slot.required ? "✗ Zorunlu" : "—";
        preview.rows.append(row);
        continue;
      }
      row.assigned = true;
      const auto *option = optionFor(match->source);
      if (!option) {
        row.valid = false;
        row.source = "Eksik: " + match->source.id +
                     (match->source.port.isEmpty() ? QString{} : " / " + match->source.port);
        row.status = "✗ Kaynak tanımda bulunamadı";
        row.badge = "✗ Eksik kaynak";
        preview.rows.append(row);
        continue;
      }
      row.source = option->label;
      const auto reason = metadataReason(kind, slot, *option);
      if (!reason.isEmpty()) {
        row.valid = false;
        row.status = "✗ " + reason;
        row.badge = "✗ Uyumsuz";
        preview.rows.append(row);
        continue;
      }
      if (!fixed) {
        row.valid = true;
        row.valueChecked = false;
        row.status = "Kontrat uyumlu • değer önizlemesi yok";
        row.badge = "✓ Kontrat";
        preview.rows.append(row);
        continue;
      }
      const quint32 bit = quint32(1) << index;
      bool ok = false;
      switch (kind) {
      case PublicationSlotKind::Number:
        ok = fixed->numberValid & bit;
        if (ok) { row.status = QString("✓ Geçerli • wire %1").arg(fixed->numbers[index]); row.badge = QString("✓ %1").arg(fixed->numbers[index]); }
        break;
      case PublicationSlotKind::Flag:
        ok = fixed->flagValid & bit;
        if (ok) { row.status = QString("✓ Geçerli • %1").arg(fixed->flagValue & bit ? "true" : "false"); row.badge = QString("✓ %1").arg(fixed->flagValue & bit ? "true" : "false"); }
        break;
      case PublicationSlotKind::Text:
        ok = fixed->textValid & bit;
        if (ok) { row.status = QString("✓ Geçerli • %1 byte").arg(fixed->texts[index].size()); row.badge = QString("✓ %1B").arg(fixed->texts[index].size()); }
        break;
      case PublicationSlotKind::Pose:
        ok = fixed->pose.has_value();
        if (ok) { row.status = "✓ Geçerli • whole pose"; row.badge = "✓ Pose"; }
        break;
      }
      row.valid = ok;
      row.valueChecked = true;
      if (!ok) {
        const auto diagnostic = fixed->diagnostics.value(key);
        row.status = "✗ " + (diagnostic.isEmpty() ? QString("Değer üretilmedi")
                                                   : diagnostic);
        row.badge = "✗ Geçersiz";
      }
      preview.rows.append(row);
    }
  }
  return preview;
}
} // namespace visionflow
