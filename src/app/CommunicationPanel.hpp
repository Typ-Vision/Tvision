#pragma once
#include "application/CommunicationWorkspace.hpp"
#include <QWidget>
class QLabel;class QTableWidget;class QPlainTextEdit;class QPushButton;class QLineEdit;class QSpinBox;class QCheckBox;class QComboBox;
namespace visionflow {
class CommunicationPanel final:public QWidget {
public:
    explicit CommunicationPanel(QWidget* parent=nullptr);
    void bind(CommunicationWorkspace*);
    void refresh();
    std::function<QString()> legacyHistory;
private:
    void selection();
    void command(const QString&);
    void migration(const QString&);
    CommunicationWorkspace* workspace_=nullptr;
    QVector<CommunicationStationView> rows_;
    QLabel *listener_,*message_,*detail_;
    QTableWidget* stations_;
    QPlainTextEdit *history_,*migrationReport_;
    QLineEdit *address_,*migrationRequest_;
    QSpinBox* port_;
    QCheckBox* enabled_;
    QCheckBox* remote_;
    QComboBox* recipes_;
    QPushButton *run_,*automatic_,*manual_,*stop_;
    std::optional<CommunicationMigrationInput> reviewedInput_;
    QString reviewedRequest_,workspacePath_,packagePath_;
    QByteArray planDigest_,manifestDigest_,pointerDigest_,ledgerDigest_;
};
}
