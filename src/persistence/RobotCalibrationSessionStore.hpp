#pragma once

#include "domain/RobotCalibrationSession.hpp"

#include <QMutex>
#include <QString>

#include <optional>
#include <vector>

// One atomically replaced JSON file. It is a session/command authority, not a
// capture ledger; sample/frame records belong to COMM4-21. A failed QSaveFile
// commit leaves the previous complete file untouched.
class FileRobotCalibrationSessionStore final : public visionflow::CalibrationSessionStore {
public:
    explicit FileRobotCalibrationSessionStore(QString path);

    std::optional<visionflow::CalibrationCommandRecord> findCommand(
        const visionflow::CalibrationCommandKey&) const override;
    std::optional<visionflow::CalibrationSessionRecord> findSession(const QUuid&) const override;
    std::vector<visionflow::CalibrationSessionRecord> loadAll() const override;
    void commit(const visionflow::CalibrationSessionRecord&,
                const std::optional<visionflow::CalibrationCommandRecord>& = std::nullopt) override;

    const QString& path() const { return path_; }

private:
    struct State {
        std::vector<visionflow::CalibrationSessionRecord> sessions;
        std::vector<visionflow::CalibrationCommandRecord> commands;
    };
    State load() const;
    void save(const State&) const;

    QString path_;
    mutable QMutex mutex_;
};
