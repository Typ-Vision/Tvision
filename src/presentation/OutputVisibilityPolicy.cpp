#include "OutputVisibilityPolicy.hpp"
#include "domain/ToolRegistry.hpp"

namespace visionflow {
namespace {
QString mode(const ToolDefinition& tool) {
    return tool.parameters.value("mode").toString(
        ToolRegistry::get(tool.kind).defaults.value("mode").toString());
}
QString geometryMetric(const ToolDefinition& tool) {
    const auto operation = tool.parameters.value("operation").toString("point_distance");
    if (operation == "line_angle") return "angle";
    if (operation == "line_intersection") return "intersection";
    return "value";
}
QString inapplicableReason(const ToolDefinition& tool, const QString& port) {
    if (tool.kind != "geometry_measurement" && tool.kind != "edge_pair"
        && tool.kind != "line_fit" && tool.kind != "circle_fit") return {};
    const bool pixel = mode(tool) == "pixel";
    if ((pixel && port.endsWith("Mm")) || (!pixel && port.endsWith("Px"))) {
        // Calibrated Line/Pair also publish raw geometry; Circle and Geometry
        // publish metrics only for the selected plane.
        if (pixel || tool.kind == "geometry_measurement" || tool.kind == "circle_fit")
            return "INACTIVE_MODE";
    }
    if (tool.kind == "geometry_measurement") {
        if ((port.startsWith("value") || port.startsWith("angle") || port.startsWith("intersection"))
            && !port.startsWith(geometryMetric(tool))) return "INACTIVE_OPERATION";
        if (port == "accepted" && !tool.parameters.value("toleranceEnabled").toBool())
            return "TOLERANCE_DISABLED";
    }
    return {};
}
}
QSet<QString> OutputVisibility::visiblePortIds() const {
    QSet<QString> result;
    for (const auto& port : ports) if (port.visible) result.insert(port.port.id);
    return result;
}
QSet<QString> OutputVisibilityPolicy::allPorts(const QString& kind) {
    QSet<QString> result;
    for (const auto& port : outputPorts(kind)) result.insert(port.id);
    return result;
}
QSet<QString> OutputVisibilityPolicy::defaults(const ToolDefinition& tool) {
    QSet<QString> selected;
    const auto& kind = tool.kind;
    const QString suffix = mode(tool) == "pixel" ? "Px" : "Mm";
    if (kind == "edge_point") selected = {"found", "pointMm"};
    else if (kind == "geometry_measurement") selected = {geometryMetric(tool) + suffix, "status"};
    else if (kind == "edge_pair") selected = {"found", "width" + suffix, "accepted"};
    else if (kind == "line_fit") selected = {"found", "angle" + suffix, "accepted"};
    else if (kind == "circle_fit") selected = {"found", "diameter" + suffix, "accepted"};
    else if (kind == "measurement") selected = {"value", "pass"};
    else if (kind == "template_match") selected = {"found", "score", "point"};
    else if (kind == "pattern_pose" || kind == "shape_pose") selected = {"found", "pose", "score"};
    else if (kind == "blob") selected = {"found", "count", "area"};
    else if (kind == "code_read") selected = {"found", "codes", "accepted"};
    else if (kind == "edge") selected = {"found", "point"};
    else if (kind == "calculate") selected = {"value"};
    else selected = allPorts(kind); // Compiled extension: real ports, no guesses.
    selected.intersect(allPorts(kind));
    return selected;
}
OutputVisibility OutputVisibilityPolicy::resolve(const ToolDefinition& tool,
    const OutputVisibilityOverride& selection,
    const QMap<QString, OutputAvailabilityInfo>& actualAvailability) {
    OutputVisibility result;
    result.explicitOverride = selection.has_value();
    const auto selected = selection ? *selection : defaults(tool);
    const auto known = allPorts(tool.kind);
    result.ignoredPortIds = selected - known;
    for (const auto& port : outputPorts(tool.kind)) {
        auto availability = actualAvailability.value(port.id);
        if (availability.state == OutputAvailability::Unavailable) {
            if (availability.reason.isEmpty()) availability.reason = "UNAVAILABLE";
            result.invalidOutputReasons.insert(port.id, availability.reason);
        } else {
            const auto reason = inapplicableReason(tool, port.id);
            if (!reason.isEmpty()) availability = {OutputAvailability::Unavailable, reason};
        }
        result.ports.append({port, selected.contains(port.id), availability});
    }
    return result;
}
}
