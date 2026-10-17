#pragma once
#include "application/CalibrationCapture.hpp"
#include "domain/Project.hpp"
#include "application/EditorSession.hpp"
#include <QWidget>
class QLabel; class QTableWidget;
namespace visionflow {
struct CalibrationHomeContext {QString path,projectId,directory;std::optional<ToolDefinition> source;QVector<Recipe> recipes;std::optional<Recipe> currentRecipe;};
class CalibrationHome : public QWidget {
public:
    CalibrationHome(std::function<CalibrationHomeContext()> context,std::function<bool()> gate,QWidget* parent=nullptr);
    ~CalibrationHome();
    void refresh();
    ActiveCalibrationService* catalog() const {return catalog_.get();}
    std::function<void()> onSelectionChanged;
private:
    void attempt(std::function<void()>);
    void launch(bool plate);
    void activate();
    void adoptLegacy();
    std::function<CalibrationHomeContext()> context_;
    std::function<bool()> gate_;
    QString key_;
    std::unique_ptr<ActiveCalibrationService> catalog_;
    JobSession jobs_;
    std::shared_ptr<std::atomic_bool> cancel_;
    QLabel *state_,*status_;
    QTableWidget* records_;
};
}
