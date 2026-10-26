#pragma once
#include "OutputVisibilityPolicy.hpp"

namespace visionflow {
struct OutputVisibilityIdentity {
    QString projectId;
    QString recipeId;
    QString toolId;
};
struct OutputVisibilityPreferenceRead {
    OutputVisibilityOverride selection;
    QString error; // Missing is normal; corrupt/unreadable falls back with a reason.
    int storedDefaultVersion = 0;
};
struct OutputVisibilityPreferenceWrite {
    QString error;
    bool ok() const { return error.isEmpty(); }
};
// Owner-thread, local presentation storage. Construction performs no I/O.
// Never owns or mutates a ProjectService, session, payload or validation state.
class OutputVisibilityPreferences {
public:
    explicit OutputVisibilityPreferences(QString settingsFile);
    OutputVisibilityPreferenceRead load(const OutputVisibilityIdentity&) const;
    OutputVisibilityPreferenceWrite save(const OutputVisibilityIdentity&,
        const OutputVisibilityOverride&) const;
    OutputVisibilityPreferenceWrite reset(const OutputVisibilityIdentity& id) const;
    OutputVisibilityPreferenceWrite showAll(const OutputVisibilityIdentity& id,
        const QString& toolKind) const;
private:
    QString settingsFile_;
};
}
