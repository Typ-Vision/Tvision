#include "presentation/OutputVisibilityPolicy.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks = 0;
void check(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
ToolDefinition tool(const QString& kind) {
    return {"stable-uuid", kind, "Display name", {}, true, ToolRegistry::get(kind).defaults, {}};
}
const OutputPortPresentation& port(const OutputVisibility& view, const QString& id) {
    for (const auto& entry : view.ports) if (entry.port.id == id) return entry;
    throw std::runtime_error("missing registry output");
}
void verifyCatalog(const ToolDefinition& t, const QSet<QString>& expected) {
    const auto before = QJsonDocument(t.parameters).toJson(QJsonDocument::Compact);
    const auto descriptor = ToolRegistry::get(t.kind);
    const auto view = OutputVisibilityPolicy::resolve(t);
    check(!view.explicitOverride && view.ignoredPortIds.isEmpty(), "default has no override");
    check(view.visiblePortIds() == expected, "default table mismatch");
    check(view.ports.size() == descriptor.outputs.size(), "registry catalog cardinality");
    for (int i = 0; i < view.ports.size(); ++i) {
        const auto& p = view.ports[i].port;
        const auto& actual = descriptor.outputs[i];
        check(p.id == actual.id && p.type == actual.type && p.unit == actual.unit
            && p.frame == actual.frame && p.required == actual.required, "registry order and metadata unchanged");
        check(view.ports[i].availability.state != OutputAvailability::Available, "selection cannot imply a result");
    }
    const auto empty = OutputVisibilityPolicy::resolve(t, QSet<QString>{});
    check(empty.explicitOverride && empty.visiblePortIds().isEmpty(), "explicit empty remains empty");
    const auto all = OutputVisibilityPolicy::allPorts(t.kind);
    check(OutputVisibilityPolicy::resolve(t, all).visiblePortIds() == all, "show all only registry ports");
    const auto stale = OutputVisibilityPolicy::resolve(t, QSet<QString>{"old-deleted-port"});
    check(stale.explicitOverride && stale.visiblePortIds().isEmpty()
        && stale.ignoredPortIds == QSet<QString>{"old-deleted-port"}, "unknown-only override never falls back");
    auto mixed = expected; mixed.insert("valueMM");
    const auto normalized = OutputVisibilityPolicy::resolve(t, mixed);
    check(normalized.visiblePortIds() == expected && normalized.ignoredPortIds == QSet<QString>{"valueMM"}, "case-sensitive stale port dropped");
    check(before == QJsonDocument(t.parameters).toJson(QJsonDocument::Compact)
        && descriptor.version == ToolRegistry::get(t.kind).version, "no tool parameter/version mutation");
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        const QMap<QString, QSet<QString>> table{
            {"industrial_camera", {"image"}}, {"roi", {"region"}}, {"image_file", {"image"}}, {"webcam", {"image"}}, {"preprocess", {"image"}},
            {"template_match", {"found", "score", "point"}}, {"blob", {"found", "count", "area"}},
            {"edge", {"found", "point"}}, {"calculate", {"value"}}, {"measurement", {"value", "pass"}},
            {"edge_point", {"found", "pointMm"}}, {"code_read", {"found", "codes", "accepted"}},
            {"pattern_pose", {"found", "pose", "score"}}, {"shape_pose", {"found", "pose", "score"}},
            {"geometry_measurement", {"valueMm", "status"}}, {"edge_pair", {"found", "widthPx", "accepted"}},
            {"line_fit", {"found", "anglePx", "accepted"}}, {"circle_fit", {"found", "diameterPx", "accepted"}},
            {"set_variables", {"bundle"}}, {"result", {"decision", "validity"}}, {"publish_data", {}}
        };
        check(table.size() == ToolRegistry::entries().size(), "every builtin covered by default table");
        for (auto i = table.cbegin(); i != table.cend(); ++i) {
            verifyCatalog(tool(i.key()), i.value());
            auto partial = tool(i.key()); partial.parameters = {};
            check(OutputVisibilityPolicy::defaults(partial) == i.value(), "missing mode uses registry default");
        }

        const QStringList operations{"point_distance", "x_offset", "y_offset", "point_line_distance",
            "point_segment_distance", "line_angle", "line_intersection", "parallel_distance",
            "circle_centre_distance", "circle_radius", "circle_diameter"};
        for (const QString mode : {"pixel", "calibrated"}) {
            const QString suffix = mode == "pixel" ? "Px" : "Mm";
            for (const auto& operation : operations) {
                auto t = tool("geometry_measurement");
                t.parameters["mode"] = mode; t.parameters["operation"] = operation;
                const QString metric = operation == "line_angle" ? "angle"
                    : operation == "line_intersection" ? "intersection" : "value";
                ToolRegistry::validate(t);
                verifyCatalog(t, {metric + suffix, "status"});
                const auto explicitView = OutputVisibilityPolicy::resolve(t, QSet<QString>{"valueMm"});
                check(explicitView.visiblePortIds() == QSet<QString>{"valueMm"}, "mode/operation never replaces override");
                const auto& selected = port(explicitView, "valueMm");
                check((selected.availability.state == OutputAvailability::Unavailable)
                    == (mode == "pixel" || metric != "value"), "inactive geometry output reason");
                check(port(explicitView, "accepted").availability.reason == "TOLERANCE_DISABLED", "disabled tolerance reason");
                if (operation != "line_intersection") {
                    t.parameters["toleranceEnabled"] = true;
                    check(port(OutputVisibilityPolicy::resolve(t), "accepted").availability.reason.isEmpty(), "enabled tolerance awaits actual result");
                }
            }
            for (const QString kind : {"edge_pair", "line_fit", "circle_fit"}) {
                auto t = tool(kind); t.parameters["mode"] = mode;
                const QString metric = kind == "edge_pair" ? "width" : kind == "line_fit" ? "angle" : "diameter";
                verifyCatalog(t, {"found", metric + suffix, "accepted"});
                const auto overrideView = OutputVisibilityPolicy::resolve(t, QSet<QString>{metric + "Mm"});
                check(overrideView.visiblePortIds() == QSet<QString>{metric + "Mm"}, "metrology override preserved");
                check((port(overrideView, metric + "Mm").availability.state == OutputAvailability::Unavailable)
                    == (mode == "pixel"), "pixel mode has no physical metric");
            }
        }
        // Availability metadata represents the actual executor's invalid port;
        // policy never takes or fabricates a payload, score, pose or PASS value.
        auto edge = tool("edge_point");
        const QMap<QString, OutputAvailabilityInfo> unavailable{
            {"pointMm", {OutputAvailability::Unavailable, "NO_ACTIVE_CALIBRATION"}},
            {"found", {OutputAvailability::Available, {}}},
            {"deleted", {OutputAvailability::Unavailable, "STALE"}}
        };
        const auto edgeView = OutputVisibilityPolicy::resolve(edge, std::nullopt, unavailable);
        check(port(edgeView, "pointMm").visible && port(edgeView, "pointMm").availability.reason == "NO_ACTIVE_CALIBRATION", "selected unavailable mm retains reason");
        check(port(edgeView, "found").availability.state == OutputAvailability::Available, "actual availability preserved");
        check(!edgeView.invalidOutputReasons.contains("deleted"), "unknown availability cannot fabricate port/warning");
        const auto hidden = OutputVisibilityPolicy::resolve(edge, QSet<QString>{}, unavailable);
        check(hidden.visiblePortIds().isEmpty() && hidden.invalidOutputReasons == edgeView.invalidOutputReasons, "all hidden still retains invalid warnings");
        const auto noReason = OutputVisibilityPolicy::resolve(edge, QSet<QString>{}, {{"pointMm", {OutputAvailability::Unavailable, {}}}});
        check(noReason.invalidOutputReasons.value("pointMm") == "UNAVAILABLE", "missing reason safe fallback");
        for (const auto& reason : {QString("NOT_FOUND"), QString("source frame mismatch"), QString("calibration expired")}) {
            const auto v = OutputVisibilityPolicy::resolve(edge, QSet<QString>{}, {{"pointMm", {OutputAvailability::Unavailable, reason}}});
            check(v.invalidOutputReasons.value("pointMm") == reason, "actual first invalid reason preserved");
        }
        auto geometry = tool("geometry_measurement"); geometry.parameters["mode"] = "pixel";
        const auto actualReason = OutputVisibilityPolicy::resolve(geometry, QSet<QString>{"valueMm"},
            {{"valueMm", {OutputAvailability::Unavailable, "NOT_APPLICABLE"}}});
        check(port(actualReason, "valueMm").availability.reason == "NOT_APPLICABLE", "actual invalid reason takes precedence");
        auto circle = tool("circle_fit"); circle.parameters["mode"] = "calibrated";
        check(port(OutputVisibilityPolicy::resolve(circle, QSet<QString>{"diameterPx"}), "diameterPx").availability.reason == "INACTIVE_MODE", "calibrated circle cannot publish pixel diameter");
        auto line = tool("line_fit"); line.parameters["mode"] = "calibrated";
        check(port(OutputVisibilityPolicy::resolve(line, QSet<QString>{"anglePx"}), "anglePx").availability.state == OutputAvailability::NotEvaluated, "calibrated line may still supply actual raw angle");
        // Registered extensions use their own exact port set as safe defaults.
        ToolDescriptor extension{"visibility_extension", "Extension", 1, {}, {},
            {{"custom", PortType::Number, "ratio", "image"}}, [](const ToolDefinition&) {}};
        ToolRegistry::registerType(extension);
        verifyCatalog(tool(extension.kind), {"custom"});
        bool rejected = false;
        try { OutputVisibilityPolicy::allPorts("missing-tool-kind"); }
        catch (const std::exception&) { rejected = true; }
        check(rejected, "unknown kind follows registry rejection without fake ports");
        check(OutputVisibilityPolicy::defaultVersion == 1, "independent defaults version");
        QTextStream(stdout) << "output_visibility_policy: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        QTextStream(stderr) << "output_visibility_policy failed after " << checks << " checks: " << e.what() << '\n';
        return 1;
    }
}
