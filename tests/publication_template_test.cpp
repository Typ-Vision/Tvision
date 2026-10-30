#include "domain/PublicationTemplate.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int assertions = 0;
void check(bool ok, const char *message) {
  ++assertions;
  if (!ok)
    throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
  bool rejected = false;
  try {
    action();
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected, "Expected validation rejection");
}
// Deterministic in-memory fixture: comm4-11-publication-v1, no external input.
QUuid id() {
  static quint64 sequence = 0;
  return QUuid(QString("00000000-0000-4000-8000-%1")
                   .arg(++sequence, 12, 16, QChar('0')));
}
QString sourceId() { return id().toString(QUuid::WithoutBraces); }
QByteArray bytes(const QJsonObject &object) {
  return QJsonDocument(object).toJson(QJsonDocument::Compact);
}
} // namespace
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    const PublicationPoseContract pose{id(), id(), id(), id(), id()};
    auto t = PublicationTemplate::create(id(), id(), pose);
    t.numbers[15].name = "Diameter";
    t.numbers[15].required = true;
    t.numbers[15].unit = ScalarUnit::Millimetre;
    t.numbers[15].scaleNumerator = 1000;
    t.numbers[15].offsetNumerator = -3;
    t.numbers[15].offsetDenominator = 2;
    t.flags[31].name = "Accepted";
    t.flags[31].required = true;
    t.texts[3].name = "LotCode";
    t.texts[3].required = true;
    t.poses[0].required = true;
    t.validate();
    auto encoded = t.toJson();
    const auto frozen = bytes(encoded);
    check(bytes(PublicationTemplate::fromJson(encoded).toJson()) == frozen,
          "Canonical template schema roundtrip");
    check(encoded["numbers"].toArray().size() == 16 &&
              encoded["flags"].toArray().size() == 32 &&
              encoded["texts"].toArray().size() == 4 &&
              encoded["poses"].toArray().size() == 1,
          "Fixed capacities independent of recipe");
    PublicationAssignments a{
        t.id,
        t.revision,
        {{PublicationSlotKind::Number,
          15,
          {PublicationSourceKind::Variable, sourceId(), {}, 0}},
         {PublicationSlotKind::Flag,
          31,
          {PublicationSourceKind::Variable, sourceId(), {}, 0}},
         {PublicationSlotKind::Text,
          3,
          {PublicationSourceKind::ToolOutput, sourceId(), "decodedText", 7}},
         {PublicationSlotKind::Pose,
          0,
          {PublicationSourceKind::ToolOutput, sourceId(), "transform", 4}}}};
    QVector<PublicationSourceDefinition> sources{
        {a.rows[0].source,
         PublicationValueType::Float64,
         ScalarUnit::Millimetre,
         {}},
        {a.rows[1].source,
         PublicationValueType::Boolean,
         ScalarUnit::Unitless,
         {}},
        {a.rows[2].source,
         PublicationValueType::String,
         ScalarUnit::Unitless,
         {}},
        {a.rows[3].source, PublicationValueType::SpatialTransform,
         ScalarUnit::Millimetre, pose}};
    a.validate(t, sources);
    check(true, "Highest indices variable/direct typed sources accepted");
    const auto assignmentBytes = bytes(a.toJson());
    check(bytes(PublicationAssignments::fromJson(a.toJson()).toJson()) ==
              assignmentBytes,
          "Assignment schema canonical roundtrip");
    auto b = a;
    auto bSources = sources;
    for (int i = 0; i < 4; ++i) {
      b.rows[i].source.id = sourceId();
      bSources[i].source = b.rows[i].source;
    }
    b.validate(t, bSources);
    validatePublicationActivation(t, t);
    check(bytes(t.toJson()) == frozen,
          "A/B sources cannot mutate machine layout");
    // W5 positions depend solely on index. No name/order metadata changes the
    // existing wire codec; COMM4-12 consumes these fixed positions.
    check(280 + 4 * a.rows[0].index == 340 &&
              344 + 68 * a.rows[2].index == 548 && a.rows[1].index == 31,
          "W5 highest slot offsets");
    auto changed = t;
    changed.numbers[15].name = "Width";
    rejects([&] { validatePublicationTemplateReplacement(t, changed); });
    changed.revision = id();
    validatePublicationTemplateReplacement(t, changed);
    rejects([&] { validatePublicationActivation(t, changed); });
    rejects([&] { a.validate(changed, sources); });
    auto rebound = a;
    rebound.templateRevision = changed.revision;
    rebound.validate(changed, sources);
    validatePublicationActivation(changed, changed);
    check(true, "Explicit new activation snapshot accepts revised template");
    check(bytes(t.toJson()) == frozen,
          "Old snapshot retained after replacement");
    {
      auto bad = a;
      bad.rows.append(a.rows[0]);
      rejects([&] { bad.validate(t, sources); });
      rejects([&] { bad.toJson(); });
    }
    for (int i = 0; i < 4; ++i) {
      auto bad = a;
      bad.rows[i].index = -1;
      rejects([&] { bad.validate(t, sources); });
      bad.rows[i].index = std::array<int, 4>{16, 32, 4, 1}[i];
      rejects([&] { bad.validate(t, sources); });
    }
    for (int i = 0; i < 4; ++i) {
      auto bad = a;
      bad.rows.removeAt(i);
      rejects([&] { bad.validate(t, sources); });
    }
    {
      auto optional = t;
      optional.numbers[15].required = false;
      auto partial = a;
      partial.rows.removeFirst();
      partial.validate(optional, sources);
      check(true, "Missing optional assignment allowed");
    }
    {
      auto bad = a;
      bad.templateId = id();
      rejects([&] { bad.validate(t, sources); });
      bad = a;
      bad.rows[0].source.id = sourceId();
      rejects([&] { bad.validate(t, sources); });
    }
    for (int i = 0; i < 4; ++i) {
      auto bad = sources;
      bad[i].type = PublicationValueType::Point2D;
      rejects([&] { a.validate(t, bad); });
      bad = sources;
      bad[i].unit = ScalarUnit::Degree;
      rejects([&] { a.validate(t, bad); });
    }
    for (auto type :
         {PublicationValueType::Point2D, PublicationValueType::Pose2D,
          PublicationValueType::Float64}) {
      auto bad = sources;
      bad[3].type = type;
      rejects([&] { a.validate(t, bad); });
    }
    for (int i = 0; i < 5; ++i) {
      auto bad = sources;
      auto &p = bad[3].pose;
      switch (i) {
      case 0:
        p.targetFrame = id();
        break;
      case 1:
        p.targetRevision = id();
        break;
      case 2:
        p.sourceFrame = id();
        break;
      case 3:
        p.sourceRevision = id();
        break;
      case 4:
        p.adapterContract = id();
        break;
      }
      rejects([&] { a.validate(t, bad); });
    }
    {
      auto bad = a;
      bad.rows[2].source.revision++;
      rejects([&] { bad.validate(t, sources); });
      bad = a;
      bad.rows[2].source.port = "other";
      rejects([&] { bad.validate(t, sources); });
    }
    {
      auto bad = sources;
      bad.append(sources[0]);
      rejects([&] { a.validate(t, bad); });
    }
    for (auto type :
         {PublicationValueType::Int32, PublicationValueType::UInt32,
          PublicationValueType::Int64, PublicationValueType::Float64}) {
      auto valid = sources;
      valid[0].type = type;
      a.validate(t, valid);
      check(true, "Numeric source kept typed for checked Int32 evaluation");
    }
    {
      auto bad = t;
      bad.flags[31].name = bad.numbers[15].name;
      rejects([&] { bad.validate(); });
      bad = t;
      bad.texts[3].name = "";
      rejects([&] { bad.validate(); });
      bad.texts[3].name = "LotCode\n";
      rejects([&] { bad.validate(); });
      auto invalidPort = a;
      invalidPort.rows[2].source.port += "\n";
      rejects([&] { invalidPort.toJson(); });
    }
    {
      auto bad = t;
      bad.numbers[15].scaleDenominator = 0;
      rejects([&] { bad.validate(); });
      bad = t;
      bad.numbers[15].offsetDenominator = -1;
      rejects([&] { bad.validate(); });
      bad = t;
      bad.numbers[15].minimum = 10;
      bad.numbers[15].maximum = 9;
      rejects([&] { bad.validate(); });
      bad = t;
      bad.numbers[15].unit = ScalarUnit(999);
      rejects([&] { bad.validate(); });
      bad = t;
      bad.poses[0].pose.sourceRevision = QUuid{};
      rejects([&] { bad.validate(); });
      bad = t;
      bad.flags[0].unit = ScalarUnit::Millimetre;
      rejects([&] { bad.toJson(); });
    }
    validatePublicationText("");
    validatePublicationText(QString(64, 'a'));
    validatePublicationText(QString(32, QChar(0x00e9)));
    check(true, "Empty and exact 64-byte Unicode text valid");
    rejects([&] { validatePublicationText(QString(65, 'a')); });
    rejects([&] { validatePublicationText(QString(33, QChar(0x00e9))); });
    rejects([&] { validatePublicationText(QString(QChar(0xd800))); });
    rejects([&] { validatePublicationText(QString(QChar(0xdc00))); });
    rejects([&] { validatePublicationText("x", 65); });
    {
      auto bad = t;
      bad.texts[3].textMaxBytes = 65;
      rejects([&] { bad.validate(); });
      bad.texts[3].textMaxBytes = 0;
      rejects([&] { bad.validate(); });
    }
    // Strict schemas: no arbitrary expressions, values, unknown fields,
    // bool-as-int, future schemas or noncanonical UUIDs can enter the DTOs.
    {
      auto bad = encoded;
      bad["schemaVersion"] = 2;
      rejects([&] { PublicationTemplate::fromJson(bad); });
      bad = encoded;
      bad["revision"] = t.revision.toString();
      rejects([&] { PublicationTemplate::fromJson(bad); });
      bad = encoded;
      bad["extra"] = true;
      rejects([&] { PublicationTemplate::fromJson(bad); });
    }
    for (const auto &key : {"numbers", "flags", "texts", "poses"}) {
      auto bad = encoded;
      auto values = bad[key].toArray();
      values.removeLast();
      bad[key] = values;
      rejects([&] { PublicationTemplate::fromJson(bad); });
    }
    {
      auto bad = encoded;
      auto numbers = bad["numbers"].toArray();
      auto n = numbers[0].toObject();
      n["scaleNumerator"] = 1.5;
      numbers[0] = n;
      bad["numbers"] = numbers;
      rejects([&] { PublicationTemplate::fromJson(bad); });
      n["scaleNumerator"] = true;
      numbers[0] = n;
      bad["numbers"] = numbers;
      rejects([&] { PublicationTemplate::fromJson(bad); });
      n["scaleNumerator"] = 2147483648.0;
      numbers[0] = n;
      bad["numbers"] = numbers;
      rejects([&] { PublicationTemplate::fromJson(bad); });
    }
    {
      auto bad = a.toJson();
      bad["value"] = 0;
      rejects([&] { PublicationAssignments::fromJson(bad); });
      bad = a.toJson();
      auto rows = bad["rows"].toArray();
      auto r = rows[0].toObject();
      r["kind"] = 99;
      rows[0] = r;
      bad["rows"] = rows;
      rejects([&] { PublicationAssignments::fromJson(bad); });
    }
    const QJsonObject legacy{{"editor", QJsonObject{{"schemaVersion", 6}}},
                             {"publication", QJsonObject{{"legacy", true}}}};
    const auto legacyBytes = bytes(legacy);
    check(!recipePublicationAssignments(legacy),
          "Legacy absent extension stays absent");
    check(bytes(legacy) == legacyBytes, "Legacy read is byte-identical no-op");
    const auto recipe = withRecipePublicationAssignments(legacy, a);
    check(bytes(withRecipePublicationAssignments(recipe, a)) == bytes(recipe),
          "Repeated assignment save is no-op");
    auto restored = recipePublicationAssignments(recipe);
    check(restored.has_value() && bytes(restored->toJson()) == assignmentBytes,
          "Recipe stores reference and sources only");
    check(recipe["publication"] == legacy["publication"] &&
              recipe["editor"] == legacy["editor"],
          "V3/editor definition untouched");
    QTextStream(stdout) << "publication_template_contract: " << assertions
                        << " assertions passed\n";
    return 0;
  } catch (const std::exception &e) {
    QTextStream(stderr) << e.what() << "\n";
    return 1;
  }
}
