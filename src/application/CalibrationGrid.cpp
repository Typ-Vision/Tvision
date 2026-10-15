#include "CalibrationGrid.hpp"
#include <QSet>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool b){if(!b)throw std::runtime_error("Invalid/unsupported calibration grid descriptor");}}
void CalibrationGridTarget::validate() const {
    require(pattern==CalibrationGridPattern::Chessboard || pattern==CalibrationGridPattern::SymmetricCircles);
    require(!id.trimmed().isEmpty() && id.size()<=128 && columns>=3 && rows>=3 && columns<=32 && rows<=32 && columns*rows<=512 &&
        std::isfinite(squareMillimetres) && squareMillimetres>0 && squareMillimetres<=1000000);
}
QJsonObject CalibrationGridTarget::toJson() const {
    validate();const bool circles=pattern==CalibrationGridPattern::SymmetricCircles;
    return {{"schemaVersion",1},{"id",id},{"kind",circles ? "symmetric_circle_grid":"chessboard"},{"columns",columns},{"rows",rows},
        {"spacingMm",squareMillimetres},{"origin",circles ? "centre_1":"corner_1"},{"axes","columns_rows"}};
}
CalibrationGridTarget CalibrationGridTarget::fromJson(const QJsonObject& o){
    const QSet<QString> fields{"schemaVersion","id","kind","columns","rows","spacingMm","origin","axes"};require(o.size()==fields.size());for(auto i=o.begin();i!=o.end();++i)require(fields.contains(i.key()));
    require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["id"].isString() && o["kind"].isString());
    const auto kind=o["kind"].toString();require(kind=="chessboard" || kind=="symmetric_circle_grid");
    auto integer=[&](const char* k){require(o[k].isDouble() && o[k].toDouble()==o[k].toInt() && o[k].toInt()>=3 && o[k].toInt()<=32);return o[k].toInt();};
    require(o["spacingMm"].isDouble());CalibrationGridTarget t{o["id"].toString(),integer("columns"),integer("rows"),o["spacingMm"].toDouble(),
        kind=="chessboard" ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles};t.validate();require(t.toJson()==o);return t;
}
}
