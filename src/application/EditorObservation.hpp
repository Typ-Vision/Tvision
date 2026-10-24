#pragma once
#include <array>
#include <chrono>

namespace visionflow::editor_observation {
// Opt-in owner-thread observations. No cache, SDK access or execution policy.
enum Point { Workspace, Editor, Calibration, Runtime, Context, CatalogRead,
    CatalogDecode, ModelDecode, PlaneReplay, ProjectSave, AtomicWrite, Count };
inline constexpr std::array<const char*,Count> names{"workspace","editor","calibration",
    "runtime","context","catalog_read","catalog_decode","model_decode","plane_replay",
    "project_save","atomic_write"};
struct Sample {
    std::array<unsigned long long,Count> calls{};
    std::array<long long,Count> nanoseconds{};
};
inline thread_local Sample* active=nullptr;
class Binding {
    Sample* previous_;
public:
    explicit Binding(Sample& sample):previous_(active){active=&sample;}
    ~Binding(){active=previous_;}
    Binding(const Binding&)=delete;
    Binding& operator=(const Binding&)=delete;
};
class Scope {
    Sample* sample_=active;
    Point point_;
    std::chrono::steady_clock::time_point start_;
public:
    explicit Scope(Point point):point_(point){if(sample_){++sample_->calls[point_];start_=std::chrono::steady_clock::now();}}
    ~Scope(){if(sample_)sample_->nanoseconds[point_]+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start_).count();}
};
}
