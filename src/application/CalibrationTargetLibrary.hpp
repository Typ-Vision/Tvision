#pragma once
#include "CalibrationGrid.hpp"
#include <QMap>
namespace visionflow {
class CalibrationTargetLibrary {
public:
    explicit CalibrationTargetLibrary(QString path):path_(std::move(path)){}
    QMap<QString,CalibrationGridTarget> read() const;
    void save(QString name,CalibrationGridTarget) const;
private:
    QString path_;
};
}
