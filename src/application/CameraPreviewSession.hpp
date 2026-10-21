#pragma once
#include "CameraWorkerSession.hpp"

namespace visionflow {
enum class PreviewState { Idle, Starting, Streaming, Stopping, Error };
struct PreviewFrame {
    AcquiredFrame frame;
    quint64 generation=0;
    PreviewAcquisitionKind kind=PreviewAcquisitionKind::SequentialSingleFrame;
};
struct PreviewStats {
    quint64 completed=0,replaced=0;
    double fps=0;
    QDateTime completedAtUtc;
};
// Owner-thread application controller. Call poll from the host event loop.
// Opener must return a fresh PreviewLatest worker with capacity 1 and exact lease.
// No trusted completion/result is produced. Destruction joins, potentially blocking
// on an uninterruptible SDK; requestShutdown + poll before destroying the owner.
class CameraPreviewSession {
public:
    using Opener=std::function<std::unique_ptr<CameraWorkerSession>()>;
    void start(Opener,bool live,int operationTimeoutMs=5000);
    void poll();
    void stop(int budgetMs=1000);
    void transportLost(const QString&);
    void requestShutdown(int budgetMs=1000) {stop(budgetMs);}
    bool drained() const {owner();return !worker_;}
    PreviewState state() const {owner();return state_;}
    QString error() const {owner();return error_;}
    quint64 generation() const {owner();return generation_;}
    PreviewStats stats() const {owner();return stats_;}
    std::optional<PreviewFrame> takeLatest();
private:
    void owner() const;
    void dispatch(CameraOperation);
    void fail(const QString&);
    void close(int budgetMs,bool discard);
    const std::thread::id owner_=std::this_thread::get_id();
    std::unique_ptr<CameraWorkerSession> worker_;
    PreviewState state_=PreviewState::Idle;
    QString error_;
    quint64 generation_=0;
    bool live_=false;
    int timeoutMs_=5000;
    CameraOperation operation_=CameraOperation::Connect;
    std::optional<Deadline> deadline_;
    std::optional<PreviewFrame> latest_;
    PreviewStats stats_;
    std::optional<std::chrono::steady_clock::time_point> firstCompletion_;
};
}
