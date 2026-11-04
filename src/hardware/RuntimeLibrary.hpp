#pragma once
#include <QLibrary>
#include <QStringList>
namespace visionflow {
// Validate before executing a vendor binary; never searches PATH for the main file.
QString runtimeBinaryError(const QString& path);
class RuntimeLibrary {
public:
    RuntimeLibrary()=default;
    RuntimeLibrary(const RuntimeLibrary&)=delete;
    RuntimeLibrary& operator=(const RuntimeLibrary&)=delete;
    ~RuntimeLibrary();
    void unload();
    bool load(const QString& path,const QStringList& dependencies,QString& error);
private:
    void* handle_=nullptr;
};
}
