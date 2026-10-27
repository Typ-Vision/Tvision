#include "ResultDecision.hpp"
#include <QJsonArray>
#include <stdexcept>

namespace visionflow {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void keys(const QJsonObject& o,std::initializer_list<QString> fields) {
    QSet<QString> expected(fields.begin(),fields.end());
    require(o.size()==expected.size(),"Result fields missing/extra");
    for(auto i=o.begin();i!=o.end();++i)require(expected.contains(i.key()),"Unknown Result field");
}
}
QString resultDecisionKey(ResultDecision d) {
    switch(d) {
    case ResultDecision::NotEvaluated:return "NOT_EVALUATED";
    case ResultDecision::Ok:return "OK";
    case ResultDecision::Nok:return "NOK";
    }
    throw std::runtime_error("Unknown Result decision");
}
void validateResultConfig(const ResultConfig& c) {
    require(c.operation==ResultOperator::All || c.operation==ResultOperator::Any,"Invalid Result operator");
    require(!c.required.isEmpty() && c.required.size()<=ResultConfig::maxRequired,"Result requires Boolean bindings");
    QMap<QString,QSet<QString>> seen;
    for(const auto& b:c.required) {
        require(!b.toolId.trimmed().isEmpty() && !b.port.trimmed().isEmpty() &&
            b.toolId.toUtf8().size()<=128 && b.port.toUtf8().size()<=128 &&
            b.revision>0 && b.revision<=1000000000,"Invalid Result binding");
        require(!seen[b.toolId].contains(b.port),"Duplicate Result binding");
        seen[b.toolId].insert(b.port);
    }
}
QJsonObject encodeResultConfig(const ResultConfig& c) {
    validateResultConfig(c);QJsonArray rows;
    for(const auto& b:c.required)rows.append(QJsonObject{{"toolId",b.toolId},{"port",b.port},{"revision",b.revision}});
    return {{"operator",c.operation==ResultOperator::All?"ALL":"ANY"},{"required",rows}};
}
ResultConfig decodeResultConfig(const QJsonObject& o) {
    keys(o,{"operator","required"});
    require(o["operator"]=="ALL" || o["operator"]=="ANY","Invalid Result operator");
    require(o["required"].isArray() && o["required"].toArray().size()<=ResultConfig::maxRequired,"Invalid Result bindings");
    ResultConfig c;c.operation=o["operator"]=="ALL"?ResultOperator::All:ResultOperator::Any;
    for(const auto& row:o["required"].toArray()) {
        require(row.isObject(),"Result binding must be an object");auto b=row.toObject();keys(b,{"toolId","port","revision"});
        require(b["toolId"].isString() && b["port"].isString() && b["revision"].isDouble() &&
            b["revision"].toDouble()==b["revision"].toInt(),"Invalid Result binding types");
        c.required.append({b["toolId"].toString(),b["port"].toString(),b["revision"].toInt()});
    }
    validateResultConfig(c);return c;
}
}
