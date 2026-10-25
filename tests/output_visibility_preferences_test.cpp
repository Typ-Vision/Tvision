#include "presentation/OutputVisibilityPreferences.hpp"
#include "application/EditorSession.hpp"
#include "application/ProjectService.hpp"
#include "application/ToolResultCodec.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QProcess>
#include <QLockFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks = 0;
void check(bool ok, const char* message) { ++checks; if (!ok) throw std::runtime_error(message); }
QByteArray read(const QString& path) {
    QFile file(path); check(file.open(QIODevice::ReadOnly), "fixture read"); return file.readAll();
}
QString entry(const OutputVisibilityIdentity& id) {
    return "outputVisibility/v1/" + QString::fromLatin1(id.projectId.toUtf8().toHex()) + '/'
        + QString::fromLatin1(id.recipeId.toUtf8().toHex()) + '/' + QString::fromLatin1(id.toolId.toUtf8().toHex());
}
void inject(const QString& file, const OutputVisibilityIdentity& id, const QString& json) {
    QSettings ini(file, QSettings::IniFormat); ini.setValue(entry(id), json); ini.sync();
    check(ini.status() == QSettings::NoError, "corrupt record fixture write");
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        if (app.arguments().size() == 6 && app.arguments()[1] == "--read-empty") {
            const auto result = OutputVisibilityPreferences(app.arguments()[2]).load(
                {app.arguments()[3], app.arguments()[4], app.arguments()[5]});
            check(result.error.isEmpty() && result.selection && result.selection->isEmpty(), "fresh process explicit empty");
            return 0;
        }
        QTemporaryDir dir(QDir(app.applicationDirPath()).filePath("d2-preferences-XXXXXX"));
        check(dir.isValid(), "build fixture directory"); dir.setAutoRemove(false);
        qputenv("TMP", dir.path().toUtf8()); qputenv("TEMP", dir.path().toUtf8());
        check(QDir::tempPath() == dir.path(), "temporary INI staging remains under build");
        const auto file = dir.filePath("local.ui.ini");
        OutputVisibilityPreferences preferences(file);
        const OutputVisibilityIdentity id{"project", "recipe", "tool"};
        check(!QFile::exists(file), "construction performs no I/O");
        check(!preferences.load(id).selection && preferences.load(id).error.isEmpty(), "missing preferences use defaults");
        check(!QFile::exists(file), "missing read does not create INI");
        { QSettings ini(file, QSettings::IniFormat); ini.setValue("layout/horizontal", "original"); ini.setValue("session/project", "saved.vfproject"); }
        check(preferences.save(id, QSet<QString>{"found", "pointMm"}).ok(), "save explicit selection");
        const auto saved = OutputVisibilityPreferences(file).load(id);
        check(saved.error.isEmpty() && saved.selection == OutputVisibilityOverride(QSet<QString>{"found", "pointMm"})
            && saved.storedDefaultVersion == OutputVisibilityPolicy::defaultVersion, "reopen selection/version");
        for (const auto& other : QVector<OutputVisibilityIdentity>{{"other", "recipe", "tool"},
            {"project", "other", "tool"}, {"project", "recipe", "new-tool"}, {"project/recipe", "tool", ""}})
            check(!preferences.load(other).selection, "project recipe and tool isolation");
        const OutputVisibilityIdentity secondProject{"other", "recipe", "tool"}, secondRecipe{"project", "other", "tool"};
        check(preferences.save(secondProject, QSet<QString>{"point"}).ok()
            && preferences.save(secondRecipe, QSet<QString>{"found"}).ok(), "similar tools in two projects/recipes saved independently");
        check(preferences.load(secondProject).selection == OutputVisibilityOverride(QSet<QString>{"point"})
            && preferences.load(secondRecipe).selection == OutputVisibilityOverride(QSet<QString>{"found"})
            && preferences.load(id).selection == saved.selection, "parallel identity selections do not leak");
        const OutputVisibilityIdentity slash{"a/b", "c", "t"}, split{"a", "b/c", "t"};
        check(preferences.save(slash, QSet<QString>{"found"}).ok() && !preferences.load(split).selection, "separator identity isolation");
        const OutputVisibilityIdentity unicode{QString::fromUtf8("proje-ş"), "r%/", "工具"};
        check(preferences.save(unicode, QSet<QString>{}).ok() && preferences.load(unicode).selection.has_value(), "unicode identities");
        check(preferences.save(id, QSet<QString>{}).ok(), "explicit empty save");
        QProcess restart;
        restart.start(app.applicationFilePath(), {"--read-empty", file, id.projectId, id.recipeId, id.toolId});
        check(restart.waitForFinished(10000) && restart.exitStatus() == QProcess::NormalExit && restart.exitCode() == 0,
            "restart preserves explicit empty without process cache");
        check(preferences.showAll(id, "edge_point").ok() && preferences.load(id).selection ==
            OutputVisibilityOverride(OutputVisibilityPolicy::allPorts("edge_point")), "show all stores explicit real ports");
        ToolDefinition tool{"tool", "geometry_measurement", "Original", {}, true, defaultParameters("geometry_measurement")};
        check(preferences.save(id, QSet<QString>{"valueMm"}).ok(), "mode override save");
        tool.name = "Renamed"; tool.parameters["mode"] = "pixel";
        check(OutputVisibilityPolicy::resolve(tool, preferences.load(id).selection).visiblePortIds() == QSet<QString>{"valueMm"}, "rename/mode cannot overwrite explicit selection");
        check(preferences.reset(id).ok() && !preferences.load(id).selection, "reset removes override");
        check(preferences.load(secondProject).selection == OutputVisibilityOverride(QSet<QString>{"point"}), "reset scoped to one identity");
        check(OutputVisibilityPolicy::resolve(tool, preferences.load(id).selection).visiblePortIds() == OutputVisibilityPolicy::defaults(tool), "reset follows current mode defaults");
        check(preferences.save(id, QSet<QString>{"obsolete"}).ok(), "unknown key is persistable");
        auto view = OutputVisibilityPolicy::resolve(tool, preferences.load(id).selection);
        check(view.explicitOverride && view.visiblePortIds().isEmpty() && view.ignoredPortIds.contains("obsolete"), "unknown only does not become defaults");
        for (const auto& json : QStringList{"garbage", "[]", "{}", "{\"schemaVersion\":2,\"defaultVersion\":1,\"ports\":[]}",
            "{\"schemaVersion\":1,\"defaultVersion\":999,\"ports\":[]}", "{\"schemaVersion\":1,\"defaultVersion\":1,\"ports\":[17]}",
            "{\"schemaVersion\":1,\"defaultVersion\":1,\"ports\":[\"\"]}", QString(65537, 'x')}) {
            inject(file, id, json); const auto bad = preferences.load(id);
            check(!bad.selection && !bad.error.isEmpty(), "malformed preference safely reports fallback");
        }
        check(preferences.reset(id).ok() && preferences.load(id).error.isEmpty(), "explicit reset repairs bad entry");
        check(!preferences.save({}, QSet<QString>{}).ok() && !preferences.load({}).error.isEmpty(), "empty identity rejected");
        check(!OutputVisibilityPreferences("").reset(id).ok(), "empty settings path rejected");
        check(!preferences.save(id, QSet<QString>{""}).ok(), "empty port rejected");
        check(!preferences.save(id, QSet<QString>{QString(65537, 'x')}).ok(), "oversized write rejected");
        QSet<QString> tooMany; for (int n = 0; n < 257; ++n) tooMany.insert(QString::number(n));
        check(!preferences.save(id, tooMany).ok(), "port limit write rejected");
        const auto blocked = dir.filePath("blocker");
        { QFile blocker(blocked); check(blocker.open(QIODevice::WriteOnly), "blocker fixture"); blocker.write("keep"); }
        check(!OutputVisibilityPreferences(blocked + "/local.ui.ini").save(id, QSet<QString>{}).ok(), "save failure is explicit");
        check(read(blocked) == "keep", "failed save preserves blocking file");
        check(!OutputVisibilityPreferences(dir.path()).reset(id).ok(), "directory target failure");
        const auto beforeLock = read(file);
        { QLockFile lock(file + ".lock"); check(lock.tryLock(0), "lock fixture");
          check(!preferences.save(id, QSet<QString>{"found"}).ok() && read(file) == beforeLock, "locked save fails without mutation"); }
        check(preferences.showAll(id, "edge_point").ok(), "save recovers after lock release");
        const auto corruptFile = dir.filePath("corrupt.ini");
        { QFile corrupt(corruptFile); check(corrupt.open(QIODevice::WriteOnly), "malformed INI fixture"); corrupt.write("[broken\nentry=value\n"); }
        const auto corruptBefore = read(corruptFile);
        check(!OutputVisibilityPreferences(corruptFile).load(id).error.isEmpty(), "malformed INI read reported");
        check(!OutputVisibilityPreferences(corruptFile).save(id, QSet<QString>{"found"}).ok()
            && read(corruptFile) == corruptBefore, "malformed INI write refused without clobbering");
        { QSettings ini(file, QSettings::IniFormat); check(ini.value("layout/horizontal") == "original" && ini.value("session/project") == "saved.vfproject", "unrelated local UI settings preserved"); }

        // Real file acquisition/blob + cache: visibility edits must be presentation only.
        const auto imagePath = dir.filePath("image.png"); QImage image(64, 48, QImage::Format_Grayscale8); image.fill(0);
        for (int y = 10; y < 20; ++y) for (int x = 10; x < 30; ++x) image.setPixel(x, y, qRgb(255,255,255));
        check(image.save(imagePath), "real raster fixture");
        ProjectService project; project.createProject("Project", {}, dir.filePath("project.vfproject"), "file");
        const auto recipe = project.createRecipe("Recipe", {}, ApplicationType::Measurement, "file");
        const auto source = project.setImageFile(imagePath), blob = project.addTool("blob");
        project.setConnections({{source, "image", blob, "image"}}); project.save();
        const auto definition = project.activeRecipe()->definition;
        const EditorContext context{project.project().id, project.path(), recipe, definition};
        EditorSession session; const auto signature = session.signature(context);
        auto cycle = ExecutionService{}.run(definition, dir.path(), recipe, signature, {});
        check(cycle.state == ExecutionState::Done && session.acceptCycle(cycle, signature, context), "real cycle accepted/cache populated");
        const auto cache = session.evaluation(context, blob);
        check(cache.cache.contains(blob) && cache.cache.contains(source), "upstream and output cache retained");
        const auto payload = ToolResultCodec::encode(cache.cache[blob].result.outputs["area"]);
        check(payload["valid"].toBool(), "real scalar output valid");
        const auto encodedProject = ProjectRepository::encode(project.project());
        const auto projectDisk = read(project.path());
        const auto graph = validateGraph(project.tools(), editorConnections(definition));
        const OutputVisibilityIdentity realId{context.projectId, recipe, blob};
        for (int n = 0; n < 4; ++n) {
            const auto result = n == 0 ? preferences.save(realId, QSet<QString>{}) : n == 1 ? preferences.showAll(realId, "blob")
                : n == 2 ? preferences.save(realId, QSet<QString>{"count"}) : preferences.reset(realId);
            check(result.ok(), "preference action persisted");
            check(!project.dirty() && !project.recipeDirty(recipe) && ProjectRepository::encode(project.project()) == encodedProject
                && read(project.path()) == projectDisk, "recipe bytes/dirty/revision unchanged");
            check(session.signature(context) == signature && session.accepts(signature, context), "configuration hash unchanged");
            session.reconcile(context); const auto after = session.evaluation(context, blob);
            check(after.cache.size() == cache.cache.size() && ToolResultCodec::encode(after.cache[blob].result.outputs["area"]) == payload,
                "cached typed execution payload unchanged");
            check(validateGraph(project.tools(), editorConnections(definition)).valid() == graph.valid(), "validation unaffected");
        }
        check(preferences.save(realId, QSet<QString>{"area"}).ok(), "rename selection");
        auto renamed = project.tools().last(); renamed.name = "Renamed blob"; project.updateTool(renamed); project.save();
        project.openProject(project.path()); project.selectRecipe(recipe);
        check(preferences.load({project.project().id, recipe, blob}).selection == OutputVisibilityOverride(QSet<QString>{"area"}), "rename/reopen stable UUID");
        const auto copyPath = dir.filePath("copy.vfproject");
        ProjectRepository{}.save(copyPath, project.project()); ProjectService copied; copied.openProject(copyPath); copied.selectRecipe(recipe);
        check(preferences.load({copied.project().id, copied.activeRecipe()->id, blob}).selection == preferences.load(realId).selection,
            "same identity copy shares machine-local selection");
        ProjectService otherProject; otherProject.createProject("Project", {}, dir.filePath("other.vfproject"), "file");
        const auto otherRecipe = otherProject.createRecipe("Recipe", {}, ApplicationType::Measurement, "file");
        const auto otherBlob = otherProject.addTool("blob");
        check(!preferences.load({otherProject.project().id, otherRecipe, otherBlob}).selection, "new project with same labels gets defaults");
        project.removeTool(blob); const auto replacement = project.addTool("blob");
        check(replacement != blob && !preferences.load({project.project().id, recipe, replacement}).selection, "delete/new UUID starts default");
        QTextStream(stdout) << "Output visibility preferences: " << checks << " checks passed; artifacts " << dir.path() << '\n';
        return 0;
    } catch (const std::exception& error) { QTextStream(stderr) << error.what() << '\n'; return 1; }
}
