#pragma once
#include "hardware/OecsCApi.hpp"
#include <QString>
#include <memory>
#include <stdexcept>
#include <thread>

namespace visionflow {
enum class OecsFailure {Unavailable,WrongThread,InvalidState,InvalidSelector,NotFound,Ambiguous,Busy,ApiError,CleanupFailed};
class OecsSessionError : public std::runtime_error {
public:
    OecsSessionError(OecsFailure failure,QString stage,int code,const QString& message);
    OecsFailure failure() const;
    QString stage() const;
    int code() const;
private:
    OecsFailure failure_;
    QString stage_;
    int code_;
};
// Exact vendor+model+serial; no index or serial-only fallback.
struct OecsDeviceSelector {QString vendor,model,serial;};
enum class OecsSessionStage {Unloaded,Open,Faulted};
// Owner worker = constructing thread. Every SDK call happens there.
class OecsSession {
public:
    OecsSession(std::unique_ptr<OecsCApi> api,QString dllPath);
    ~OecsSession();
    OecsSession(const OecsSession&)=delete;
    OecsSession& operator=(const OecsSession&)=delete;
    OecsDeviceInfo open(const OecsDeviceSelector& selector);
    void close();
    OecsSessionStage stage() const;
    // A handle or library cleanup failed: the DLL is never unloaded by this session again.
    bool libraryPinned() const;
    quint32 sdkVersion() const; // read after successful load, 0 otherwise
private:
    std::unique_ptr<OecsCApi> api_;
    QString dllPath_;
    std::thread::id owner_;
    OecsSessionStage stage_=OecsSessionStage::Unloaded;
    bool pinned_=false;
    quint32 sdkVersion_=0;
    void* handle_=nullptr;
    QString claimKey_;
};
}
