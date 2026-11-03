#include "application/CommunicationStationComposition.hpp"
#include "application/MachineSetup.hpp"
#include "application/ProjectService.hpp"
#include "application/PublicationAssignmentCatalog.hpp"
#include "application/PublicationPublisher.hpp"
#include "domain/ToolRegistry.hpp"
#include "persistence/ProjectRepository.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>

using namespace visionflow;
namespace {
int checks = 0;
void check(bool ok, const char *message) {
  ++checks;
  if (!ok) throw std::runtime_error(message);
}
QString stage = "startup";
template <class F> void rejects(F action, const QString &fragment) {
  bool rejected = false;
  QString text;
  try {
    action();
  } catch (const std::exception &e) {
    rejected = true;
    text = QString::fromUtf8(e.what());
  }
  check(rejected, qPrintable("Expected publication rejection at " + stage));
  check(fragment.isEmpty() || text.contains(fragment),
        qPrintable("Expected reason at " + stage + ": " + text));
}
QByteArray bytes(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}
void write(const QString &path, const QByteArray &value) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write(value) != value.size())
    throw std::runtime_error("Fixture write");
}
VariableDefinition variable(const QString &name, VariableType type,
                            ScalarUnit unit = ScalarUnit::Unitless) {
  VariableDefinition value;
  value.name = name;
  value.type = type;
  value.unit = unit;
  return value;
}
struct Fixture {
  QString projectPath, imagePath;
  QString valueId, flagId, producer;
  Recipe recipe;
};
// Builds one saved project exactly as the editor would: an image source, a Set
// Variables producer with one committed row per variable, and the v4 slot
// assignment selected from the machine template.
Fixture build(ProjectService &project, const QString &path,
              const QImage &image, const PublicationTemplate &machineTemplate,
              VariableType valueType, ScalarUnit valueUnit,
              const QVector<SlotAssignment> &rows) {
  Fixture fixture;
  fixture.projectPath = path;
  fixture.imagePath = QDir(QFileInfo(path).absolutePath()).filePath("source.png");
  check(image.save(fixture.imagePath), "Source image");
  project.createProject("VF03", "", path, "file");
  project.createRecipe("Ölçüm A", "", ApplicationType::Measurement, "file");
  project.setImageFile(fixture.imagePath);
  fixture.valueId =
      project.addVariable(variable("Value", valueType, valueUnit));
  fixture.flagId =
      project.addVariable(variable("Flag", VariableType::Boolean));
  ValueEnvelope number;
  number.type = valueType == VariableType::Float64 ? ScalarType::Float64
                                                   : ScalarType::Int32;
  number.unit = valueUnit;
  number.validity = ValueValidity::Valid;
  if (valueType == VariableType::Float64)
    number.value = 42.0;
  else
    number.value = qint32(42);
  ValueEnvelope flag;
  flag.type = ScalarType::Boolean;
  flag.validity = ValueValidity::Valid;
  flag.value = false;
  fixture.producer = project.addTool("set_variables");
  for (auto tool : project.tools())
    if (tool.id == fixture.producer)
      tool.parameters = encodeAssignmentConfig(
          {fixture.producer,
           1,
           {{fixture.valueId, ConstantAssignmentSource{number}},
            {fixture.flagId, ConstantAssignmentSource{flag}}}}),
      project.updateTool(tool);
  project.setPublicationAssignments(
      PublicationAssignments{machineTemplate.id, machineTemplate.revision, rows});
  project.save();
  fixture.recipe = *project.activeRecipe();
  return fixture;
}
QVector<SlotAssignment> measurementRows(const Fixture &fixture) {
  return {{PublicationSlotKind::Number,
           0,
           {PublicationSourceKind::Variable, fixture.valueId, {}, 0}},
          {PublicationSlotKind::Flag,
           0,
           {PublicationSourceKind::Variable, fixture.flagId, {}, 0}}};
}
} // namespace

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    QTemporaryDir root(QCoreApplication::applicationDirPath() +
                       "/publication-publish-XXXXXX");
    check(root.isValid(), "Fixture root");
    const auto machine = root.path();
    const auto cataloguePath =
        QDir(machine).filePath("publication-catalogue-v4.json");
    const auto stationsPath =
        QDir(machine).filePath("communication-stations-v4.json");
    const auto identityPath =
        QDir(machine).filePath("communication-identities-v4.json");

    // VF-01 prerequisite through the normal machine-setup service.
    const auto catalogue = MachineSetup(machine).start();
    const auto defaults = MachineSetup::defaults(catalogue.installation);
    const auto surface = defaults[0];
    const auto measurement = defaults[1];
    check(measurement.numbers[0].required &&
              measurement.numbers[0].unit == ScalarUnit::Unitless,
          "Measurement Number0 required and unitless");

    // Empty activation catalogue exists before publishing; the publish action
    // must not touch it.
    {
      CommunicationServerProfiles profiles(
          QDir(machine).filePath("communication-server-v4.json"));
      JsonStationIdentityStore identityStore(identityPath);
      CommunicationPeerSessionService sessions(identityStore);
      CommunicationStationComposition composition(machine, {}, identityStore,
                                                   sessions);
      CommunicationServerLifecycle server(
          profiles, QDir(machine).filePath("shutdown.json"), &sessions);
      CommunicationWorkspace workspace(server, profiles, identityStore);
      composition.attach(server, workspace);
      check(workspace.recipes().isEmpty(), "No publication before action");
      check(workspace.publicationStatus().contains("Yayımlanmış reçete yok"),
            "Initial publication status is explicit");
      check(workspace.stations().isEmpty(), "Publishing fixture has no station");
      server.beginShutdown(0);
      check(server.pollShutdown(), "Clean fixture shutdown");
    }
    const auto stationsBefore = bytes(stationsPath);
    const auto identitiesBefore = bytes(identityPath);
    check(!stationsBefore.isEmpty() && !identitiesBefore.isEmpty(),
          "Machine fixtures exist");

    QImage image(32, 32, QImage::Format_Grayscale8);
    image.fill(255);

    // ---- accepted measurement publication ----
    ProjectService project;
    auto fixture = build(project, QDir(machine).filePath("project.vfproject"),
                         image, measurement, VariableType::Int32,
                         ScalarUnit::Unitless, {});
    QVector<SlotAssignment> rows{
        {PublicationSlotKind::Number,
         0,
         {PublicationSourceKind::Variable, fixture.valueId, {}, 0}},
        {PublicationSlotKind::Flag,
         0,
         {PublicationSourceKind::Variable, fixture.flagId, {}, 0}}};
    project.setPublicationAssignments(
        PublicationAssignments{measurement.id, measurement.revision, rows});
    project.save();
    fixture.recipe = *project.activeRecipe();
    check(recipePublicationAssignments(fixture.recipe.definition).has_value(),
          "Slot assignment saved in recipe");

    // Unit and scale are visible in the editor contract preview; the required
    // slot is not shown as a valid zero when unassigned.
    {
      const auto preview = previewPublication(
          measurement,
          PublicationAssignments{measurement.id, measurement.revision, {}},
          fixture.recipe.definition, nullptr);
      bool sawRequiredMissing = false;
      for (const auto &row : preview.rows)
        if (row.kind == PublicationSlotKind::Number && row.index == 0) {
          check(!row.assigned && !row.valid, "Missing Measurement is not valid");
          check(row.contract.contains("scale") &&
                    row.contract.contains("unitless"),
                "Measurement unit and scale visible");
          sawRequiredMissing = row.status.contains("Gerekli slot atanmadı");
        }
      check(sawRequiredMissing, "Required Measurement slot named");
      check(preview.errors.join(" ").contains("Required slot"),
            "Missing required slot is a contract error");
    }

    const auto templateDir = QDir(machine).filePath("publication-templates-v4");
    const auto templateKey = [&] {
      for (const auto &entry : catalogue.entries)
        if (entry.identity.category == CatalogueCategory::TemplateRevision &&
            entry.identity.entity == measurement.id &&
            entry.identity.revision == measurement.revision)
          return entry.key;
      throw std::runtime_error("Measurement template key missing");
    }();
    PublicationPublisher publisher(machine);
    const auto first = publisher.publish(fixture.projectPath,
                                         fixture.recipe.id, templateDir,
                                         &fixture.recipe);
    check(first.created, "First publish creates an immutable revision");
    check(first.publication.templateKey == templateKey,
          "Published template key matches machine catalogue");
    check(first.publication.recipeKey > 0 && !first.publication.revision.isNull(),
          "Published key and revision");
    {
      const auto after = JsonStationIdentityStore(identityPath).load();
      for (const auto &entry : catalogue.entries) {
        bool present = false;
        for (const auto &current : after.entries)
          present |= current == entry;
        check(present, "Publishing preserves every prior identity entry");
      }
      int published = 0;
      for (const auto &entry : after.entries)
        published += entry.identity.category == CatalogueCategory::RecipeRevision &&
                     entry.identity.entity == QUuid(fixture.recipe.id) &&
                     entry.identity.revision == first.publication.revision;
      check(published == 1, "One immutable recipe revision key bound");
    }
    check(bytes(stationsPath) == stationsBefore,
          "Publishing does not touch station activation catalogue");
    check(bytes(cataloguePath).size() > 0, "Catalogue written");

    // Same content is a byte-identical no-op.
    const auto catalogueBytes = bytes(cataloguePath);
    const auto identitiesAfterFirst = bytes(identityPath);
    const auto repeat = publisher.publish(fixture.projectPath,
                                          fixture.recipe.id, templateDir,
                                          &fixture.recipe);
    check(!repeat.created && repeat.publication.revision == first.publication.revision &&
              repeat.publication.recipeKey == first.publication.recipeKey,
          "Identical publish is the same immutable revision");
    check(bytes(cataloguePath) == catalogueBytes, "No-op leaves catalogue bytes");
    check(bytes(identityPath) == identitiesAfterFirst, "No-op leaves identity bytes");

    // Composition discovers the published revision without migration/fixture.
    {
      CommunicationServerProfiles profiles(
          QDir(machine).filePath("communication-server-v4.json"));
      JsonStationIdentityStore identities(identityPath);
      CommunicationPeerSessionService sessions(identities);
      CommunicationStationComposition composition(machine, {}, identities,
                                                   sessions);
      CommunicationServerLifecycle server(
          profiles, QDir(machine).filePath("shutdown.json"), &sessions);
      CommunicationWorkspace workspace(server, profiles, identities);
      composition.attach(server, workspace);
      const auto recipes = workspace.recipes();
      check(recipes.size() == 1 && recipes.first().publicationOnly,
            "Published revision offered without migration package");
      check(recipes.first().selection.recipeKey == first.publication.recipeKey &&
                recipes.first().selection.revision == first.publication.revision &&
                recipes.first().templateKey == templateKey,
            "Exact published revision/template in recipe list");
      check(recipes.first().selection.cameras.isEmpty(),
            "Station camera resolution is not fabricated by publishing");
      check(workspace.publicationStatus().contains("Yayımlanmış reçete: 1"),
            "Publication status visible");
      check(workspace.stations().isEmpty(), "Discovery creates no station");
      server.beginShutdown(0);
      check(server.pollShutdown(), "Clean shutdown");
    }

    // ---- changed content creates a new revision and preserves the old ----
    ValueEnvelope changed;
    changed.type = ScalarType::Int32;
    changed.validity = ValueValidity::Valid;
    changed.value = qint32(7);
    for (auto tool : project.tools())
      if (tool.id == fixture.producer) {
        auto config = decodeAssignmentConfig(tool.parameters, tool.id);
        config.rows[0].source = ConstantAssignmentSource{changed};
        tool.parameters = encodeAssignmentConfig(config);
        project.updateTool(tool);
      }
    project.save();
    fixture.recipe = *project.activeRecipe();
    const auto second = publisher.publish(fixture.projectPath,
                                          fixture.recipe.id, templateDir,
                                          &fixture.recipe);
    check(second.created && second.publication.revision != first.publication.revision &&
              second.publication.recipeKey != first.publication.recipeKey,
          "Changed content gets a new revision and key");
    {
      QStringList errors;
      const auto all = PublicationCatalogueStore(machine).load(&errors);
      check(errors.isEmpty() && all.size() == 2, "Previous publication retained");
      check(all[0].revision == first.publication.revision &&
                all[0].contentDigest == first.publication.contentDigest,
            "Old revision bytes never rewritten");
    }
    check(bytes(stationsPath) == stationsBefore,
          "Second publish still leaves activations untouched");

    // ---- negative publication cases: catalogue stays byte-identical ----
    const auto accepted = bytes(cataloguePath);
    auto rejectedPublish = [&](ProjectService &candidate, const QString &reason) {
      candidate.save();
      const auto before = bytes(cataloguePath);
      stage = "rejectedPublish: " + reason;
      rejects(
          [&] {
            PublicationPublisher(machine).publish(
                candidate.path(), candidate.activeRecipe()->id, templateDir,
                candidate.activeRecipe());
          },
          reason);
      check(bytes(cataloguePath) == before, "Rejected publish writes nothing");
    };

    ProjectService wrongType;
    auto wrongFixture =
        build(wrongType, QDir(machine).filePath("wrong-type.vfproject"), image,
              measurement, VariableType::Boolean, ScalarUnit::Unitless, {});
    QVector<SlotAssignment> wrongRows{
        {PublicationSlotKind::Number,
         0,
         {PublicationSourceKind::Variable, wrongFixture.valueId, {}, 0}}};
    wrongType.setPublicationAssignments(
        PublicationAssignments{measurement.id, measurement.revision, wrongRows});
    rejectedPublish(wrongType, "numeric");

    ProjectService wrongUnit;
    auto unitFixture = build(wrongUnit, QDir(machine).filePath("wrong-unit.vfproject"),
                             image, measurement, VariableType::Float64,
                             ScalarUnit::Millimetre, {});
    QVector<SlotAssignment> unitRows{
        {PublicationSlotKind::Number,
         0,
         {PublicationSourceKind::Variable, unitFixture.valueId, {}, 0}}};
    wrongUnit.setPublicationAssignments(
        PublicationAssignments{measurement.id, measurement.revision, unitRows});
    rejectedPublish(wrongUnit, "unit mismatch");

    ProjectService missing;
    build(missing, QDir(machine).filePath("missing-slot.vfproject"), image,
          measurement, VariableType::Int32, ScalarUnit::Unitless, {});
    // Empty assignment with a required Measurement slot must fail publish, not save.
    missing.setPublicationAssignments(
        PublicationAssignments{measurement.id, measurement.revision, {}});
    rejectedPublish(missing, "Required slot");

    ProjectService unknownTemplate;
    auto unknownFixture = build(unknownTemplate,
                                QDir(machine).filePath("unknown-template.vfproject"),
                                image, measurement, VariableType::Int32,
                                ScalarUnit::Unitless, {});
    auto brokenAssignments = PublicationAssignments{
        measurement.id, QUuid::createUuid(), {}};
    unknownTemplate.setPublicationAssignments(brokenAssignments);
    rejectedPublish(unknownTemplate, "revision");

    // Unsaved editor drafts cannot be published through a stale candidate.
    auto draft = *project.activeRecipe();
    draft.name += " (taslak)";
    const auto diskProject = ProjectRepository{}.load(project.path());
    check(diskProject.recipes.size() == 1 &&
              PublicationCatalogueStore::recipeBytes(diskProject.recipes.first()) !=
                  PublicationCatalogueStore::recipeBytes(draft),
          "Unsaved candidate differs from saved project bytes");
    check(PublicationCatalogueStore::recipeBytes(draft) !=
              PublicationCatalogueStore::recipeBytes(*project.activeRecipe()),
          "Unsaved candidate actually differs");
    stage = "unsaved candidate";
    rejects(
        [&] {
          PublicationPublisher(machine).publish(project.path(),
                                                project.activeRecipe()->id,
                                                templateDir, &draft);
        },
        "Kaydedilmemiş");
    check(bytes(cataloguePath) == accepted, "Unsaved candidate writes nothing");

    // Surface template needs only the standard decision/validity contract.
    ProjectService surfaceProject;
    const auto surfaceFixture = build(
        surfaceProject, QDir(machine).filePath("surface.vfproject"), image,
        surface, VariableType::Int32, ScalarUnit::Unitless, {});
    const auto surfacePublished = publisher.publish(
        surfaceFixture.projectPath, surfaceFixture.recipe.id, templateDir,
        &surfaceFixture.recipe);
    check(surfacePublished.created &&
              surfacePublished.publication.recipeKey > second.publication.recipeKey,
          "Surface template publishes without extra slots; keys are monotonic");

    // ---- interrupted writes ----
    // A fresh first publication interrupted at every boundary is either absent
    // or fully valid; retry converges to exactly one entry and one key.
    for (const auto &stop :
         {QString("PREPARED"), QString("HALF_WRITTEN"), QString("WRITTEN")}) {
      QTemporaryDir partial(QCoreApplication::applicationDirPath() +
                            "/publication-partial-XXXXXX");
      check(partial.isValid(), "Partial root");
      const auto setup = MachineSetup(partial.path()).start();
      const auto templates = MachineSetup::defaults(setup.installation);
      ProjectService partialProject;
      const auto partialFixture = build(
          partialProject, QDir(partial.path()).filePath("p.vfproject"), image,
          templates[1], VariableType::Int32, ScalarUnit::Unitless, {});
      QVector<SlotAssignment> partialRows{
          {PublicationSlotKind::Number,
           0,
           {PublicationSourceKind::Variable, partialFixture.valueId, {}, 0}}};
      partialProject.setPublicationAssignments(
          PublicationAssignments{templates[1].id, templates[1].revision,
                                 partialRows});
      partialProject.save();
      bool fired = false;
      stage = "partial " + stop;
      rejects(
          [&] {
            PublicationPublisher(
                partial.path(), [&](const QString &step) {
                  if (step == stop && !fired) {
                    fired = true;
                    throw std::runtime_error("interrupted");
                  }
                })
                .publish(partialProject.path(),
                         partialProject.activeRecipe()->id,
                         QDir(partial.path())
                             .filePath("publication-templates-v4"),
                         partialProject.activeRecipe());
          },
          "interrupted");
      check(fired, "Injected boundary reached");
      const auto partialCatalogue =
          QDir(partial.path()).filePath("publication-catalogue-v4.json");
      QStringList errors;
      const auto present =
          PublicationCatalogueStore(partial.path()).load(&errors);
      check(errors.isEmpty(), "Partial file never corrupts the catalogue");
      check(present.isEmpty(), "Interrupted publish is not loadable");
      const auto retried = PublicationPublisher(partial.path()).publish(
          partialProject.path(), partialProject.activeRecipe()->id,
          QDir(partial.path()).filePath("publication-templates-v4"),
          partialProject.activeRecipe());
      check(retried.created, "Retry after interruption succeeds");
      const auto after = PublicationCatalogueStore(partial.path()).load(&errors);
      check(errors.isEmpty() && after.size() == 1 &&
                after.first().recipeKey == retried.publication.recipeKey &&
                after.first().revision == retried.publication.revision,
            "Retry resumes the same revision without duplicates");
    }
    // Interrupting a second publication leaves the existing catalogue exactly.
    {
      ValueEnvelope newer;
      newer.type = ScalarType::Int32;
      newer.validity = ValueValidity::Valid;
      newer.value = qint32(99);
      for (auto tool : surfaceProject.tools())
        if (tool.kind == "set_variables") {
          auto config = decodeAssignmentConfig(tool.parameters, tool.id);
          config.rows[0].source = ConstantAssignmentSource{newer};
          tool.parameters = encodeAssignmentConfig(config);
          surfaceProject.updateTool(tool);
        }
      surfaceProject.save();
      const auto before = bytes(cataloguePath);
      bool fired = false;
      stage = "second partial";
      rejects(
          [&] {
            PublicationPublisher(machine, [&](const QString &step) {
              if (step == "HALF_WRITTEN" && !fired) {
                fired = true;
                throw std::runtime_error("interrupted");
              }
            }).publish(surfaceProject.path(), surfaceProject.activeRecipe()->id,
                       templateDir, surfaceProject.activeRecipe());
          },
          "interrupted");
      check(fired && bytes(cataloguePath) == before,
            "Interrupted second publish keeps previous catalogue bytes");
    }

    // ---- corrupt catalogue is preserved and blocks a misleading publish ----
    const auto valid = bytes(cataloguePath);
    write(cataloguePath, "broken");
    stage = "corrupt catalogue";
    rejects(
        [&] {
          publisher.publish(surfaceProject.path(),
                            surfaceProject.activeRecipe()->id, templateDir,
                            surfaceProject.activeRecipe());
        },
        "bozuk");
    check(bytes(cataloguePath) == "broken", "Corrupt catalogue not overwritten");
    {
      QStringList errors;
      check(PublicationCatalogueStore(machine).load(&errors).isEmpty() &&
                errors.size() == 1,
            "Corrupt catalogue reported");
    }
    write(cataloguePath, valid);

    // A station activation recorded before a later publish stays byte-identical.
    check(bytes(stationsPath) == stationsBefore,
          "All publications leave activation history untouched");
    QTextStream(stdout) << "PASS " << checks
                        << " publication publish checks; file fixtures only\n";
    return 0;
  } catch (const std::exception &e) {
    QTextStream(stderr) << e.what() << "\n";
    return 1;
  }
}
