#pragma once
#include "CommunicationValues.hpp"
#include <QVector>

namespace visionflow {
// Separate from legacy tool PartDecision (PASS/FAIL) and execution status.
enum class ResultDecision { NotEvaluated, Ok, Nok };
enum class ResultOperator { All, Any };
struct ResultBooleanBinding {
    QString toolId, port;
    int revision=0;
};
struct ResultConfig {
    static constexpr int maxRequired=256;
    ResultOperator operation=ResultOperator::All;
    // Only explicit tool outputs: no constant, variable-name or system writes.
    QVector<ResultBooleanBinding> required;
};
QString resultDecisionKey(ResultDecision);
void validateResultConfig(const ResultConfig&);
QJsonObject encodeResultConfig(const ResultConfig&);
ResultConfig decodeResultConfig(const QJsonObject&);
}
