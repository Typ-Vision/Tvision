// COMM4-17: shared publication source resolution, machine template manifests and
// the Editor slot-assignment candidates. Domain/evaluator behaviour stays in
// publication_template_contract and fixed_slot_publication_contract.
#include "application/PublicationAssignmentCatalog.hpp"
#include "application/PublicationService.hpp"
#include "domain/ToolGraph.hpp"
#include "domain/ToolRegistry.hpp"
#include "persistence/PublicationTemplateStore.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int assertions = 0;
void check(bool ok, const char *text) { ++assertions; if (!ok) throw std::runtime_error(text); }
template <class F> void rejects(F f) {
  bool threw = false; try { f(); } catch (const std::exception &) { threw = true; }
  check(threw, "Expected rejection");
}
QUuid uuid(int n) { return QUuid(QString("00000000-0000-4000-8000-%1").arg(n, 12, 16, QChar('0'))); }
QString id(int n) { return uuid(n).toString(QUuid::WithoutBraces); }
void writeJson(const QString &path, const QJsonObject &object) {
  QFile file(path);
  check(file.open(QIODevice::WriteOnly), "fixture write");
  file.write(QJsonDocument(object).toJson());
}
struct Fixture {
  QString sourceId = id(1), codeId = id(2), producerId = id(3), variableId = id(4);
  PublicationPoseContract pose{uuid(10), uuid(11), uuid(12), uuid(13), uuid(14)};
  PublicationTemplate t = PublicationTemplate::create(uuid(20), uuid(21), pose);
  QJsonObject definition;
  Fixture() {
    ToolDefinition image{sourceId, "image_file", "Source", {}, true, {{"path", "part.png"}}};
    ToolDefinition code{codeId, "code_read", "Code", {}, true, ToolRegistry::get("code_read").defaults};
    VariableDefinition variable;
    variable.id = variableId; variable.name = "Accepted"; variable.type = VariableType::Boolean;
    ToolDefinition producer{producerId, "set_variables", "Bundle", {},
        true, encodeAssignmentConfig({producerId, 3,
            {{variableId, ConstantAssignmentSource{
                ValueEnvelope{ScalarType::Boolean, false, ValueValidity::Valid, {}, {}, {}, {}, {}}}}}})};
    definition = withRecipeVariables({}, {variable});
    definition = withEditorTools(definition, {producer, code, image});
    definition = withEditorConnections(definition, {{sourceId, "image", codeId, "image"}});
  }
  PublicationAssignments assignment() const {
    return PublicationAssignments{t.id, t.revision, {}};
  }
  QJsonObject candidate(const PublicationAssignments &a) const {
    return withRecipePublicationAssignments(definition, a);
  }
};
PublicationSourceDefinition catalogueEntry(const QVector<PublicationSourceOption> &options,
                                           const PublicationSource &source,
                                           const PublicationSourceOption **found = nullptr) {
  for (const auto &option : options)
    if (option.source == source) {
      if (found) *found = &option;
      return {option.source, option.type, option.unit, option.pose};
    }
  throw std::runtime_error("source option missing");
}
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    Fixture f;
    QDir().mkpath(QCoreApplication::applicationDirPath());
    // A. Machine template manifest store: strict per-file, one bad file cannot
    // hide the remaining machine revisions.
    QTemporaryDir root;
    check(root.isValid(), "temporary machine directory");
    const auto templates = QDir(root.path()).filePath("publication-templates-v4");
    check(QDir().mkpath(templates), "template directory");
    writeJson(QDir(templates).filePath("good.json"), f.t.toJson());
    QFile bad(QDir(templates).filePath("bad.json"));
    check(bad.open(QIODevice::WriteOnly), "bad manifest");
    bad.write("{not json"); bad.close();
    QStringList errors;
    const auto loaded = PublicationTemplateStore(templates).load(&errors);
    check(loaded.size() == 1 && errors.size() == 1, "invalid manifest reported, valid revision kept");
    check(loaded.first().value.id == f.t.id && loaded.first().value.revision == f.t.revision,
          "manifest identity preserved");
    check(PublicationTemplateStore(QDir(root.path()).filePath("missing")).load().isEmpty(),
          "absent machine directory yields no templates");
    // B. Template revision is machine authority: metadata is immutable within a
    // revision and a change needs an explicit new revision/activation.
    auto edited = f.t; edited.numbers[0].required = true;
    rejects([&] { validatePublicationTemplateReplacement(f.t, edited); });
    auto revised = edited; revised.revision = uuid(22);
    validatePublicationTemplateReplacement(f.t, revised);
    rejects([&] { validatePublicationActivation(f.t, revised); });
    // C. Candidate list: variables and direct typed outputs, with blocked
    // entries carrying a visible reason instead of disappearing.
    const auto options = publicationSourceOptions(f.definition);
    const PublicationSourceOption *variableOption = nullptr;
    const PublicationSourceOption *flagOption = nullptr;
    const PublicationSourceOption *countOption = nullptr;
    for (const auto &option : options) {
      if (option.source.kind == PublicationSourceKind::Variable && option.source.id == f.variableId) variableOption = &option;
      if (option.source.kind == PublicationSourceKind::ToolOutput && option.source.port == "found") flagOption = &option;
      if (option.source.kind == PublicationSourceKind::ToolOutput && option.source.port == "count") countOption = &option;
    }
    check(variableOption && variableOption->type == PublicationValueType::Boolean && variableOption->blockReason.isEmpty(),
          "variable candidate resolves Boolean authority");
    check(flagOption && flagOption->type == PublicationValueType::Boolean, "direct Boolean candidate");
    check(countOption && countOption->type == PublicationValueType::Float64 &&
              countOption->unit == ScalarUnit::Unitless, "direct Number candidate keeps unitless contract");
    // D. Metadata validation is per slot and keeps the exact reason.
    auto assignment = f.assignment();
    assignment.rows.append({PublicationSlotKind::Flag, 0,
                            {PublicationSourceKind::Variable, f.variableId, {}, 0}});
    assignment.rows.append({PublicationSlotKind::Flag, 1,
                            {PublicationSourceKind::ToolOutput, f.codeId, "found",
                             ToolRegistry::get("code_read").version}});
    assignment.rows.append({PublicationSlotKind::Number, 0,
                            {PublicationSourceKind::ToolOutput, f.codeId, "count",
                             ToolRegistry::get("code_read").version}});
    auto preview = previewPublication(f.t, assignment, f.candidate(assignment));
    check(preview.errors.isEmpty(), qPrintable("compatible contract validates: " + preview.errors.join(" | ")));
    int assigned = 0, invalid = 0;
    for (const auto &row : preview.rows) {
      if (row.assigned) ++assigned;
      if (!row.valid) ++invalid;
      check(!row.valueChecked, "definition preview never claims a value");
    }
    check(assigned == 3 && invalid == 0, "three assignments satisfy the contract");
    // Required missing is an explicit per-slot failure and blocks Apply.
    auto requiredTemplate = f.t;
    requiredTemplate.flags[31].required = true;
    preview = previewPublication(requiredTemplate, assignment, f.candidate(assignment));
    bool requiredMissing = false;
    for (const auto &row : preview.rows)
      if (row.kind == PublicationSlotKind::Flag && row.index == 31)
        requiredMissing = row.required && !row.assigned && !row.valid && row.status.contains("Gerekli");
    check(requiredMissing && !preview.errors.isEmpty(),
          "missing required slot is named per field and rejected");
    // E. Incompatible port selection is an explicit, per-slot validation error.
    auto incompatible = assignment;
    incompatible.rows.append({PublicationSlotKind::Flag, 2,
                              {PublicationSourceKind::ToolOutput, f.codeId, "count",
                               ToolRegistry::get("code_read").version}});
    preview = previewPublication(f.t, incompatible, f.candidate(incompatible));
    bool flagReason = false;
    for (const auto &row : preview.rows)
      if (row.kind == PublicationSlotKind::Flag && row.index == 2)
        flagReason = !row.valid && row.status.contains("Boolean");
    check(flagReason, "Number source in Flag slot names the incompatible contract");
    // F. Schema roundtrip is byte-stable, so an unchanged draft stays a no-op.
    const auto encoded = f.candidate(assignment);
    const auto decoded = recipePublicationAssignments(encoded);
    check(decoded && decoded->toJson() == assignment.toJson(), "assignment schema roundtrip");
    check(withRecipePublicationAssignments(encoded, *decoded) == encoded, "second apply is byte no-op");
    const auto legacy = withRecipePublicationAssignments(encoded, assignment);
    check(!legacy.contains("publicationV4") ? false : legacy == encoded, "legacy fields untouched");
    // G. Wrong revision, unknown source and duplicate slot are rejected.
    auto wrongRevision = assignment; wrongRevision.templateRevision = uuid(99);
    preview = previewPublication(f.t, wrongRevision, f.candidate(wrongRevision));
    check(!preview.errors.isEmpty(), "wrong template revision rejected");
    auto unknown = assignment;
    unknown.rows.append({PublicationSlotKind::Number, 1,
                         {PublicationSourceKind::Variable, id(77), {}, 0}});
    preview = previewPublication(f.t, unknown, f.candidate(unknown));
    check(!preview.errors.isEmpty(), "unknown variable source rejected");
    // H. Shared resolution: the Runtime binding builder and Editor preview agree
    // on unit/type authority and fail closed for unpublished converters.
    const auto bindings = publicationBindings(f.candidate(assignment));
    check(bindings.size() == 3, "one binding per assigned source");
    for (const auto &binding : bindings)
      check(!binding.producerId.isEmpty() && !binding.port.isEmpty() && binding.revision > 0,
            "binding authority complete");
    {
      ToolDefinition edge{id(30), "edge_pair", "Edges", {},
          true, ToolRegistry::get("edge_pair").defaults};
      auto tools = editorTools(f.definition);
      tools.append(edge);
      auto definition = withEditorTools(f.definition, tools);
      auto links = editorConnections(f.definition);
      links.append({f.sourceId, "image", edge.id, "image"});
      definition = withEditorConnections(definition, links);
      definition = withRecipePublicationAssignments(definition,
          PublicationAssignments{f.t.id, f.t.revision,
              {{PublicationSlotKind::Number, 2,
                {PublicationSourceKind::ToolOutput, edge.id, "widthMm",
                 ToolRegistry::get("edge_pair").version}}}});
      // "mm" is a known unit, so this resolves; the pixel port must not.
      const auto resolved = publicationBindings(definition);
      check(resolved.size() == 1 && resolved.first().definition.unit == ScalarUnit::Millimetre,
            "known unit resolved explicitly");
      auto pixel = definition;
      auto a = recipePublicationAssignments(pixel);
      a->rows[0].source.port = "widthPx";
      pixel = withRecipePublicationAssignments(pixel, *a);
      rejects([&] { publicationBindings(pixel); });
    }
    // I. Whole Pose stays outside this card: no adapter, no planar promotion.
    {
      VariableDefinition poseVar; poseVar.id = id(40); poseVar.name = "Pose";
      poseVar.type = VariableType::SpatialTransform; poseVar.unit = ScalarUnit::Millimetre;
      auto definition = withRecipeVariables(f.definition, {poseVar});
      definition = withEditorTools(definition, editorTools(f.definition));
      definition = withRecipePublicationAssignments(definition,
          PublicationAssignments{f.t.id, f.t.revision,
              {{PublicationSlotKind::Pose, 0,
                {PublicationSourceKind::Variable, poseVar.id, {}, 0}}}});
      rejects([&] { publicationBindings(definition); });
      bool blocked = false;
      for (const auto &option : publicationSourceOptions(definition))
        if (option.source.id == poseVar.id) blocked = option.blockReason.contains("COMM4-19");
      check(blocked, "Pose candidate is visible with its explicit adapter gap");
      const auto posePreview = previewPublication(f.t, *recipePublicationAssignments(definition), definition);
      bool required = false;
      for (const auto &row : posePreview.rows)
        if (row.kind == PublicationSlotKind::Pose) required = !row.valid;
      check(required, "Pose slot cannot pass without the adapter contract");
    }
    QTextStream(stdout) << "PASS COMM4-17 publication assignment: manifest store, machine-template fence, "
                           "variable/direct candidates, per-slot reasons, schema no-op, converter and Pose gates\n";
    return 0;
  } catch (const std::exception &e) {
    QTextStream(stderr) << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
