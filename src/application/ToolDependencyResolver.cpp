#include "ToolDependencyResolver.hpp"
#include "BackendCapabilities.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <stdexcept>
namespace visionflow {
QJsonArray ToolDependencyResolver::resolve(const ToolDefinition& tool,const QString& directory) {
    const auto encoded=encodeToolDependencies(tool.dependencies);
    const auto descriptor=ToolRegistry::get(tool.kind);bool backend=descriptor.requiredBackend.isEmpty();
    for(const auto& d:tool.dependencies) {
        if(d.kind==ToolDependencyKind::Backend) {
            BackendCapabilities::require(d.id,d.version,d.contentId);
            if(d.id==descriptor.requiredBackend)backend=true;
        } else {
            const auto root=QFileInfo(directory).canonicalFilePath();
            QFile file(QDir(directory).absoluteFilePath(d.path));const auto canonical=QFileInfo(file).canonicalFilePath();
            if(root.isEmpty() || canonical.isEmpty() || !canonical.startsWith(root+"/",Qt::CaseInsensitive) || !file.open(QIODevice::ReadOnly) || file.size()>64*1024*1024)throw std::runtime_error("Model/asset missing, outside project, or over 64 MiB");
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if(!hash.addData(&file) || file.error()!=QFileDevice::NoError || QString::fromLatin1(hash.result().toHex())!=d.contentId)throw std::runtime_error("Model/asset content identity differs");
        }
    }
    if(!backend)throw std::runtime_error("Tool requires an explicit pinned backend dependency");
    return encoded;
}
}
