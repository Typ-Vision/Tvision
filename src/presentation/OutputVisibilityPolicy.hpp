#pragma once
#include "domain/ToolGraph.hpp"
#include <QMap>
#include <QSet>
#include <optional>

namespace visionflow {
// View preferences only. nullopt means defaults; an engaged empty set means
// hide every output. Neither changes execution, dependencies or terminal data.
using OutputVisibilityOverride = std::optional<QSet<QString>>;
enum class OutputAvailability { NotEvaluated, Available, Unavailable };
struct OutputAvailabilityInfo {
    OutputAvailability state = OutputAvailability::NotEvaluated;
    QString reason;
};
struct OutputPortPresentation {
    PortDefinition port;
    bool visible = false;
    OutputAvailabilityInfo availability;
};
struct OutputVisibility {
    QVector<OutputPortPresentation> ports; // Registry order, including hidden ports.
    QSet<QString> ignoredPortIds;
    bool explicitOverride = false;
    // Actual invalid-output warnings survive even when that port is hidden.
    QMap<QString, QString> invalidOutputReasons;
    QSet<QString> visiblePortIds() const;
};
class OutputVisibilityPolicy {
public:
    static constexpr int defaultVersion = 1; // Presentation only, not engine/schema.
    static QSet<QString> defaults(const ToolDefinition& tool);
    static QSet<QString> allPorts(const QString& kind);
    // Supply availability from actual results, never infer calibration success
    // from selection. No entry means not evaluated, never a zero/PASS/value.
    // Execution status, error, part decision and frame identity are deliberately
    // outside this API and must always be presented independently by callers.
    static OutputVisibility resolve(const ToolDefinition& tool,
        const OutputVisibilityOverride& selection = std::nullopt,
        const QMap<QString, OutputAvailabilityInfo>& actualAvailability = {});
};
}
