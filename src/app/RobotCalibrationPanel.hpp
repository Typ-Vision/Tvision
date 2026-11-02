#pragma once
#include "application/RobotCalibrationWorkspace.hpp"
#include <QWidget>
class QLabel;class QComboBox;class QTableWidget;class QPushButton;
namespace visionflow {
class RobotCalibrationPanel final:public QWidget {
public:
    explicit RobotCalibrationPanel(QWidget* parent=nullptr);
    void bind(RobotCalibrationWorkspace*);
    void selectStation(const QUuid&);
    void refresh();
private:
    void command(RobotCalibrationAction);
    RobotCalibrationWorkspace fallback_;
    RobotCalibrationWorkspace* workspace_=&fallback_;
    QUuid station_;
    RobotCalibrationView view_;
    QComboBox* mode_;
    QLabel *readiness_,*models_,*progress_,*reasons_,*message_;
    QTableWidget* samples_;
    QMap<RobotCalibrationAction,QPushButton*> actions_;
};
}
