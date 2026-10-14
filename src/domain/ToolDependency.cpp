#include "ToolDependency.hpp"
#include <QJsonObject>
#include <QSet>
#include <QDir>
#include <algorithm>
#include <stdexcept>
namespace visionflow {
QVector<ToolDependency> decodeToolDependencies(const QJsonArray& a) {
    if(a.size()>32)throw std::runtime_error("At most 32 dependencies per tool");
    QVector<ToolDependency> result;QSet<QString> ids;
    for(auto item:a) {
        if(!item.isObject())throw std::runtime_error("Dependency must be object");const auto o=item.toObject();
        const QSet<QString> fields{"kind","id","version","contentId","path"};
        if(o.size()!=fields.size())throw std::runtime_error("Dependency fields missing/extra");
        for(auto it=o.begin();it!=o.end();++it)if(!fields.contains(it.key()) || !it->isString())throw std::runtime_error("Invalid dependency field");
        const auto kind=o["kind"].toString(),id=o["id"].toString(),version=o["version"].toString(),sha=o["contentId"].toString(),path=o["path"].toString();
        if((kind!="backend" && kind!="model" && kind!="asset") || id.isEmpty() || id.size()>128 || version.isEmpty() || version.size()>128 || ids.contains(kind+":"+id) || sha.size()!=64 || !std::all_of(sha.begin(),sha.end(),[](QChar c){return (c>='0' && c<='9') || (c>='a' && c<='f');}))throw std::runtime_error("Invalid/duplicate dependency identity");
        if(kind=="backend" ? !path.isEmpty() : (path.isEmpty() || path.size()>4096 || QDir::isAbsolutePath(path) || path.contains('\\') || path.contains(':') || path.split('/').contains("..") || path!=QDir::cleanPath(path)))throw std::runtime_error("Dependency path must be canonical project-relative (backend has no path)");
        ids.insert(kind+":"+id);result.append({kind=="backend" ? ToolDependencyKind::Backend:kind=="model" ? ToolDependencyKind::Model:ToolDependencyKind::Asset,id,version,sha,path});
    }return result;
}
QJsonArray encodeToolDependencies(const QVector<ToolDependency>& deps) {
    if(deps.size()>32)throw std::runtime_error("At most 32 dependencies per tool");QJsonArray a;
    for(const auto& d:deps) {
        if(d.kind!=ToolDependencyKind::Backend && d.kind!=ToolDependencyKind::Model && d.kind!=ToolDependencyKind::Asset)throw std::runtime_error("Unknown dependency kind");
        a.append(QJsonObject{{"kind",d.kind==ToolDependencyKind::Backend ? "backend":d.kind==ToolDependencyKind::Model ? "model":"asset"},{"id",d.id},{"version",d.version},{"contentId",d.contentId},{"path",d.path}});
    }decodeToolDependencies(a);return a;
}
}
