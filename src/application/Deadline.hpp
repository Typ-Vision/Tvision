#pragma once
#include <chrono>
#include <algorithm>

namespace visionflow {
// Monotonic, process-local budget. UTC/device clocks are never used for deadlines.
class Deadline {
public:
    using Clock=std::chrono::steady_clock;
    explicit Deadline(int milliseconds):end_(Clock::now()+std::chrono::milliseconds(milliseconds)) {}
    explicit Deadline(Clock::time_point end):end_(end) {}
    bool expired() const {return Clock::now()>=end_;}
    int remainingMs() const {
        if(expired())return 0;
        // Round up so a sub-ms remainder does not become an infinite/zero SDK wait.
        return int(std::clamp<long long>(std::chrono::ceil<std::chrono::milliseconds>(end_-Clock::now()).count(),0,2147483647));
    }
    Deadline limitedTo(int milliseconds) const {return Deadline(std::min(end_,Clock::now()+std::chrono::milliseconds(milliseconds)));}
private:
    Clock::time_point end_;
};
}
