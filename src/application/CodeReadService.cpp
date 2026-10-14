#include "CodeReadService.hpp"
#include "domain/CodeReadConfig.hpp"
#include "domain/ToolRegistry.hpp"
#include "ToolDependencyResolver.hpp"
#include "vision/codes/CodeReader.hpp"
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>
namespace visionflow {
InspectionResults CodeReadService::inspect(const CodeResults& codes,const QJsonObject& p) {
    validateCodeRead(p);ToolValues::encode(codes);const int count=codes.items.size();
    const bool countOk=count>=p["minCount"].toInt() && count<=p["maxExpected"].toInt();
    const auto rule=p["rule"].toString(),expected=p["expected"].toString();
    auto accepts=[&](const CodeResult& code){const auto text=code.text;
        if(rule=="none")return true;if(rule=="exact")return text==expected;if(rule=="prefix")return text.startsWith(expected);
        if(rule=="length")return text.size()>=p["minLength"].toInt() && text.size()<=p["maxLength"].toInt();
        if(text.size()>4096)throw std::runtime_error("Regex input exceeds 4096 UTF-16 units");
        return QRegularExpression(QRegularExpression::anchoredPattern(expected)).match(text).hasMatch();};
    bool content=false;const auto selection=p["selection"].toString();
    if(selection=="selected"){const int i=p["selectedIndex"].toInt();content=i<count && accepts(codes.items[i]);}
    else if(count)content=selection=="any" ? std::any_of(codes.items.begin(),codes.items.end(),accepts) : std::all_of(codes.items.begin(),codes.items.end(),accepts);
    else content=rule=="none" && selection=="all";
    return {{{"count",countOk ? QString{}:QString("Unexpected count: %1").arg(count),countOk},{"content",content ? QString{}:QString("Content/selection mismatch"),content}}};
}
ToolResult CodeReadService::execute(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString& directory,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel,const std::optional<ToolRoi>& resolvedRoi) {
    if(t.parameters.contains("poseTracking"))throw std::runtime_error("Pose tracking must be resolved by execution engine");
    ToolRegistry::validate(t);ToolDependencyResolver::resolve(t,directory);
    const auto image=std::get<ImageFrame>(inputs.value("image").data);const auto p=t.parameters;const auto roi=p["searchRoi"].toObject();
    CodeReaderOptions options;options.maxCount=p["maxCount"].toInt();options.budgetMs=p["budgetMs"].toInt();
    if(!roi.isEmpty())options.roi={roi["x"].toInt(),roi["y"].toInt(),roi["width"].toInt(),roi["height"].toInt()};
    if(resolvedRoi){options.roi={};options.trackingRoi=resolvedRoi;}
    for(const auto& f:p["formats"].toArray())options.formats.append(f.toString());
    const auto codes=CodeReader::read(image,options,cancel);const auto checks=inspect(codes,p);
    const bool accepted=std::all_of(checks.items.begin(),checks.items.end(),[](const auto& c){return c.accepted;});
    ToolResult result;result.toolId=t.id;result.state=ExecutionState::Done;result.message=codes.items.isEmpty() ? "NO_READ" : "READ";
    auto put=[&](const QString& id,PortData data){for(const auto& port:outputPorts(t.kind))if(port.id==id){PortValue v{port,std::move(data),true,frameId,cycleId,image.metadata()};v.method="zxing-reader";v.methodVersion=1;result.outputs[id]=std::move(v);}};
    put("codes",codes);put("count",double(codes.items.size()));put("found",!codes.items.isEmpty());put("accepted",accepted);put("checks",checks);
    return result;
}
}
