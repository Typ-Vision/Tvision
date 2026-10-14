#pragma once
#include <QWidget>
#include "application/ExecutionService.hpp"
class QTableWidget;
class QLabel;
namespace visionflow {
class EdgeProfileView : public QWidget {
public:
    explicit EdgeProfileView(QWidget* parent=nullptr);
    void setDiagnostics(const std::optional<EdgeDiagnostics>&);
    bool hasDiagnostics() const {return diagnostics_.has_value();}
private:
    std::optional<EdgeDiagnostics> diagnostics_;
    QWidget* plot_;
    QLabel* info_;
    QTableWidget* candidates_;
};
}
