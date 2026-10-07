#pragma once
#include "domain/Project.hpp"

namespace visionflow {
class ProjectRepository {
public:
    Project load(const QString& path) const;
    void save(const QString& path, const Project& project) const;
    static QByteArray encode(const Project& project);
};
}
