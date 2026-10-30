#include "PublicationTemplate.hpp"
#include <QJsonArray>
#include <QRegularExpression>
#include <stdexcept>

namespace visionflow {
namespace {
void need(bool b, const char *message) {
  if (!b)
    throw std::runtime_error(message);
}
void keys(const QJsonObject &o, const QSet<QString> &expected) {
  need(o.size() == expected.size(), "Publication schema fields missing/extra");
  for (auto i = o.begin(); i != o.end(); ++i)
    need(expected.contains(i.key()), "Unknown publication field");
}
qint32 integer(const QJsonObject &o, const char *key) {
  const auto v = o[key];
  const double n = v.toDouble();
  need(v.isDouble() && n >= std::numeric_limits<qint32>::min() &&
           n <= std::numeric_limits<qint32>::max() && n == qint32(n),
       "Publication integer required");
  return qint32(n);
}
QString string(const QJsonObject &o, const char *key) {
  need(o[key].isString(), "Publication string required");
  return o[key].toString();
}
QUuid uuid(const QJsonObject &o, const char *key) {
  auto s = string(o, key);
  QUuid id(s);
  need(!id.isNull() && id.toString(QUuid::WithoutBraces) == s,
       "Canonical publication UUID required");
  return id;
}
QString uid(const QUuid &id) {
  need(!id.isNull(), "Publication identity required");
  return id.toString(QUuid::WithoutBraces);
}
void poseValid(const PublicationPoseContract &p) {
  uid(p.targetFrame);
  uid(p.targetRevision);
  uid(p.sourceFrame);
  uid(p.sourceRevision);
  uid(p.adapterContract);
}
QJsonObject poseJson(const PublicationPoseContract &p) {
  poseValid(p);
  return {{"targetFrame", uid(p.targetFrame)},
          {"targetRevision", uid(p.targetRevision)},
          {"sourceFrame", uid(p.sourceFrame)},
          {"sourceRevision", uid(p.sourceRevision)},
          {"adapterContract", uid(p.adapterContract)}};
}
PublicationPoseContract poseRead(const QJsonObject &o) {
  keys(o, {"targetFrame", "targetRevision", "sourceFrame", "sourceRevision",
           "adapterContract"});
  return {uuid(o, "targetFrame"), uuid(o, "targetRevision"),
          uuid(o, "sourceFrame"), uuid(o, "sourceRevision"),
          uuid(o, "adapterContract")};
}
int count(PublicationSlotKind kind) {
  switch (kind) {
  case PublicationSlotKind::Number:
    return 16;
  case PublicationSlotKind::Flag:
    return 32;
  case PublicationSlotKind::Text:
    return 4;
  case PublicationSlotKind::Pose:
    return 1;
  }
  throw std::runtime_error("Unknown slot kind");
}
const PublicationSlot &slot(const PublicationTemplate &t, PublicationSlotKind k,
                            int index) {
  need(index >= 0 && index < count(k), "Slot index out of range");
  switch (k) {
  case PublicationSlotKind::Number:
    return t.numbers[index];
  case PublicationSlotKind::Flag:
    return t.flags[index];
  case PublicationSlotKind::Text:
    return t.texts[index];
  case PublicationSlotKind::Pose:
    return t.poses[index];
  }
  throw std::runtime_error("Unknown slot");
}
void sourceValid(const PublicationSource &s) {
  need(!QUuid(s.id).isNull() &&
           QUuid(s.id).toString(QUuid::WithoutBraces) == s.id,
       "Canonical publication source UUID required");
  if (s.kind == PublicationSourceKind::Variable)
    need(s.port.isEmpty() && s.revision == 0,
         "Variable source has no output/revision");
  else {
    need(s.kind == PublicationSourceKind::ToolOutput && s.revision > 0,
         "Tool revision required");
    static const QRegularExpression port("^[A-Za-z][A-Za-z0-9_.-]{0,63}\\z");
    need(port.match(s.port).hasMatch(), "Invalid typed output port");
  }
}
void shape(const PublicationAssignments &a) {
  uid(a.templateId);
  uid(a.templateRevision);
  need(a.rows.size() <= 53, "Too many slot assignments");
  QSet<int> used;
  for (const auto &r : a.rows) {
    need(r.index >= 0 && r.index < count(r.kind), "Slot index out of range");
    int key = int(r.kind) * 32 + r.index;
    need(!used.contains(key), "Duplicate publication slot");
    used.insert(key);
    sourceValid(r.source);
  }
}
QJsonObject slotJson(const PublicationSlot &s, PublicationSlotKind kind) {
  QJsonObject o{{"name", s.name}, {"required", s.required}};
  if (kind == PublicationSlotKind::Number) {
    o["unit"] = int(s.unit);
    o["scaleNumerator"] = s.scaleNumerator;
    o["scaleDenominator"] = s.scaleDenominator;
    o["offsetNumerator"] = s.offsetNumerator;
    o["offsetDenominator"] = s.offsetDenominator;
    o["minimum"] = s.minimum;
    o["maximum"] = s.maximum;
    o["encoding"] = "Int32";
    o["rounding"] = "tiesToEven";
  }
  if (kind == PublicationSlotKind::Text)
    o["maxBytes"] = s.textMaxBytes;
  if (kind == PublicationSlotKind::Pose)
    o["contract"] = poseJson(s.pose);
  return o;
}
PublicationSlot slotRead(const QJsonObject &o, PublicationSlotKind kind) {
  QSet<QString> fields{"name", "required"};
  if (kind == PublicationSlotKind::Number)
    fields.unite({"unit", "scaleNumerator", "scaleDenominator",
                  "offsetNumerator", "offsetDenominator", "minimum", "maximum",
                  "encoding", "rounding"});
  if (kind == PublicationSlotKind::Text)
    fields.insert("maxBytes");
  if (kind == PublicationSlotKind::Pose)
    fields.insert("contract");
  keys(o, fields);
  PublicationSlot s;
  s.name = string(o, "name");
  need(o["required"].isBool(), "Slot required must be Boolean");
  s.required = o["required"].toBool();
  if (kind == PublicationSlotKind::Number) {
    s.unit = ScalarUnit(integer(o, "unit"));
    s.scaleNumerator = integer(o, "scaleNumerator");
    s.scaleDenominator = integer(o, "scaleDenominator");
    s.offsetNumerator = integer(o, "offsetNumerator");
    s.offsetDenominator = integer(o, "offsetDenominator");
    s.minimum = integer(o, "minimum");
    s.maximum = integer(o, "maximum");
    need(o["encoding"] == "Int32" && o["rounding"] == "tiesToEven",
         "Unsupported number encoding/rounding");
  }
  if (kind == PublicationSlotKind::Text)
    s.textMaxBytes = integer(o, "maxBytes");
  if (kind == PublicationSlotKind::Pose) {
    need(o["contract"].isObject(), "Pose contract required");
    s.pose = poseRead(o["contract"].toObject());
    s.unit = ScalarUnit::Millimetre;
  }
  return s;
}
} // namespace
void validatePublicationText(const QString &text, int maxBytes) {
  need(maxBytes > 0 && maxBytes <= 64, "Text byte limit must be 1..64");
  need(text.toUtf8().size() <= maxBytes,
       "Publication text too long; truncation forbidden");
  for (qsizetype i = 0; i < text.size(); ++i) {
    if (text[i].isHighSurrogate()) {
      need(i + 1 < text.size() && text[i + 1].isLowSurrogate(),
           "Malformed Unicode publication text");
      ++i;
    } else
      need(!text[i].isLowSurrogate(), "Malformed Unicode publication text");
  }
}
PublicationTemplate
PublicationTemplate::create(const QUuid &id, const QUuid &revision,
                            const PublicationPoseContract &pose) {
  PublicationTemplate t;
  t.id = id;
  t.revision = revision;
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < count(PublicationSlotKind(k)); ++i) {
      auto &s =
          const_cast<PublicationSlot &>(slot(t, PublicationSlotKind(k), i));
      s.name = QStringList{"Number", "Flag", "Text", "Pose"}[k] +
               (k == 3 ? QString{} : QString::number(i));
    }
  t.poses[0].unit = ScalarUnit::Millimetre;
  t.poses[0].pose = pose;
  t.validate();
  return t;
}
void PublicationTemplate::validate() const {
  uid(id);
  uid(revision);
  QSet<QString> names;
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < count(PublicationSlotKind(k)); ++i) {
      const auto &s = slot(*this, PublicationSlotKind(k), i);
      static const QRegularExpression name("^[A-Za-z][A-Za-z0-9_.-]{0,63}\\z");
      need(name.match(s.name).hasMatch() && !names.contains(s.name),
           "Invalid/duplicate slot name");
      names.insert(s.name);
      PublicationSlot expected;
      expected.name = s.name;
      expected.required = s.required;
      if (k == 0) {
        need(int(s.unit) >= 0 && int(s.unit) <= int(ScalarUnit::Degree),
             "Unknown number unit");
        need(s.scaleNumerator != 0 && s.scaleDenominator > 0 &&
                 s.offsetDenominator > 0 && s.minimum <= s.maximum,
             "Invalid number conversion/range");
        expected = s;
        need(s.pose == PublicationPoseContract{} && s.textMaxBytes == 64,
             "Unexpected numeric metadata");
      } else if (k == 2) {
        need(s.textMaxBytes > 0 && s.textMaxBytes <= 64,
             "Text limit exceeds wire capacity");
        expected.textMaxBytes = s.textMaxBytes;
      } else if (k == 3) {
        poseValid(s.pose);
        expected.pose = s.pose;
        expected.unit = ScalarUnit::Millimetre;
      }
      need(s == expected, "Metadata incompatible with slot kind");
    }
}
QJsonObject PublicationTemplate::toJson() const {
  validate();
  QJsonObject o{
      {"schemaVersion", 1}, {"id", uid(id)}, {"revision", uid(revision)}};
  const QStringList names{"numbers", "flags", "texts", "poses"};
  for (int k = 0; k < 4; ++k) {
    QJsonArray a;
    for (int i = 0; i < count(PublicationSlotKind(k)); ++i)
      a.append(slotJson(slot(*this, PublicationSlotKind(k), i),
                        PublicationSlotKind(k)));
    o[names[k]] = a;
  }
  return o;
}
PublicationTemplate PublicationTemplate::fromJson(const QJsonObject &o) {
  keys(o, {"schemaVersion", "id", "revision", "numbers", "flags", "texts",
           "poses"});
  need(integer(o, "schemaVersion") == 1, "Unknown template schema");
  PublicationTemplate t;
  t.id = uuid(o, "id");
  t.revision = uuid(o, "revision");
  const QStringList names{"numbers", "flags", "texts", "poses"};
  for (int k = 0; k < 4; ++k) {
    need(o[names[k]].isArray(), "Slot array required");
    auto a = o[names[k]].toArray();
    need(a.size() == count(PublicationSlotKind(k)),
         "Fixed template slot count required");
    for (int i = 0; i < a.size(); ++i) {
      need(a[i].isObject(), "Slot object required");
      const_cast<PublicationSlot &>(slot(t, PublicationSlotKind(k), i)) =
          slotRead(a[i].toObject(), PublicationSlotKind(k));
    }
  }
  t.validate();
  return t;
}
void PublicationAssignments::validate(
    const PublicationTemplate &t,
    const QVector<PublicationSourceDefinition> &catalogue) const {
  t.validate();
  shape(*this);
  need(templateId == t.id && templateRevision == t.revision,
       "Exact template revision required");
  for (int i = 0; i < catalogue.size(); ++i) {
    sourceValid(catalogue[i].source);
    for (int j = 0; j < i; ++j)
      need(catalogue[i].source != catalogue[j].source,
           "Ambiguous publication source");
  }
  QSet<int> assigned;
  for (const auto &r : rows) {
    const auto &target = slot(t, r.kind, r.index);
    const PublicationSourceDefinition *found = nullptr;
    for (const auto &d : catalogue)
      if (d.source == r.source)
        found = &d;
    need(found, "Unresolved publication source/revision");
    const auto &d = *found;
    bool numeric = d.type >= PublicationValueType::Int32 &&
                   d.type <= PublicationValueType::Float64;
    switch (r.kind) {
    case PublicationSlotKind::Number:
      need(numeric, "Number requires numeric typed source");
      need(d.unit == target.unit,
           "Number unit mismatch; explicit typed adapter required");
      break;
    case PublicationSlotKind::Flag:
      need(d.type == PublicationValueType::Boolean &&
               d.unit == ScalarUnit::Unitless,
           "Flag requires Boolean unitless source");
      break;
    case PublicationSlotKind::Text:
      need(d.type == PublicationValueType::String &&
               d.unit == ScalarUnit::Unitless,
           "Text requires String unitless source");
      break;
    case PublicationSlotKind::Pose:
      need(d.type == PublicationValueType::SpatialTransform &&
               d.unit == ScalarUnit::Millimetre,
           "Whole pose requires SpatialTransform mm");
      need(d.pose == target.pose, "Whole pose frame/revision/adapter mismatch");
      break;
    }
    assigned.insert(int(r.kind) * 32 + r.index);
  }
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < count(PublicationSlotKind(k)); ++i)
      need(!slot(t, PublicationSlotKind(k), i).required ||
               assigned.contains(k * 32 + i),
           "Required slot unassigned");
}
QJsonObject PublicationAssignments::toJson() const {
  shape(*this);
  QJsonArray a;
  for (const auto &r : rows)
    a.append(
        QJsonObject{{"kind", int(r.kind)},
                    {"index", r.index},
                    {"source", QJsonObject{{"kind", int(r.source.kind)},
                                           {"id", r.source.id},
                                           {"port", r.source.port},
                                           {"revision", r.source.revision}}}});
  return {{"schemaVersion", 1},
          {"templateId", uid(templateId)},
          {"templateRevision", uid(templateRevision)},
          {"rows", a}};
}
PublicationAssignments PublicationAssignments::fromJson(const QJsonObject &o) {
  keys(o, {"schemaVersion", "templateId", "templateRevision", "rows"});
  need(integer(o, "schemaVersion") == 1 && o["rows"].isArray(),
       "Invalid assignment schema");
  PublicationAssignments a;
  a.templateId = uuid(o, "templateId");
  a.templateRevision = uuid(o, "templateRevision");
  need(o["rows"].toArray().size() <= 53, "Too many assignments");
  for (auto v : o["rows"].toArray()) {
    need(v.isObject(), "Slot assignment object required");
    auto r = v.toObject();
    keys(r, {"kind", "index", "source"});
    need(r["source"].isObject(), "Source object required");
    auto s = r["source"].toObject();
    keys(s, {"kind", "id", "port", "revision"});
    a.rows.append({PublicationSlotKind(integer(r, "kind")),
                    integer(r, "index"),
                    {PublicationSourceKind(integer(s, "kind")), string(s, "id"),
                     string(s, "port"), integer(s, "revision")}});
  }
  shape(a);
  return a;
}
void validatePublicationTemplateReplacement(const PublicationTemplate &before,
                                            const PublicationTemplate &after) {
  before.validate();
  after.validate();
  need(before.id == after.id, "Template replacement identity mismatch");
  need(before.revision != after.revision || before.toJson() == after.toJson(),
       "Immutable template revision cannot change metadata");
}
void validatePublicationActivation(const PublicationTemplate &active,
                                   const PublicationTemplate &requested) {
  active.validate();
  requested.validate();
  need(active.toJson() == requested.toJson(),
       "Template change requires explicit new activation");
}
std::optional<PublicationAssignments>
recipePublicationAssignments(const QJsonObject &definition) {
  if (!definition.contains("publicationV4"))
    return {};
  need(definition["publicationV4"].isObject(), "publicationV4 object required");
  return PublicationAssignments::fromJson(
      definition["publicationV4"].toObject());
}
QJsonObject
withRecipePublicationAssignments(QJsonObject definition,
                                 const PublicationAssignments &assignments) {
  definition["publicationV4"] = assignments.toJson();
  return definition;
}
} // namespace visionflow
