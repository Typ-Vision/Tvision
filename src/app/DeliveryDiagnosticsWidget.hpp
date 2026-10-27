#pragma once
#include <QWidget>
#include <QPointer>
#include "application/CommunicationTcpEndpoint.hpp"
class QLabel;class QTableWidget;class QPlainTextEdit;class QPushButton;
namespace visionflow {
class DeliveryDiagnosticsWidget final:public QWidget {
public:
    explicit DeliveryDiagnosticsWidget(std::function<QString()> archive,QWidget* parent=nullptr);
    void bindEndpoint(CommunicationTcpEndpoint* endpoint);
    void refresh();
private:
    std::function<QString()> archive_;QPointer<CommunicationTcpEndpoint> endpoint_;
    QVector<DeliveryRecovery> rows_;QLabel *status_,*message_;QTableWidget* table_;
    QPlainTextEdit *detail_,*trace_;QPushButton* retry_;
    void selection();void retry();
};
}
