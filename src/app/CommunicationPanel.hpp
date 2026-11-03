#pragma once
#include "application/CommunicationWorkspace.hpp"
#include <QWidget>
class QLabel;class QTableWidget;class QPlainTextEdit;class QPushButton;class QLineEdit;class QSpinBox;class QCheckBox;class QComboBox;
namespace visionflow {
class RobotCalibrationPanel;
class CommunicationPanel final:public QWidget {
public:
    explicit CommunicationPanel(QWidget* parent=nullptr);
    void bind(CommunicationWorkspace*);
    void refresh();
    std::function<QString()> legacyHistory;
    // Testable file pickers; the defaults are QFileDialog. Empty result cancels.
    std::function<QString(const QString&,const QString&,const QString&)> saveFile;
    std::function<QString(const QString&,const QString&)> openFile;
private:
    void selection();
    void refreshRecipeList();
    void refreshClients();
    void sourceSettings(bool create);
    void command(const QString&);
    void migration(const QString&);
    RobotCalibrationPanel* calibration_;
    CommunicationWorkspace* workspace_=nullptr;
    QVector<CommunicationStationView> rows_;
    QLabel *listener_,*message_,*detail_,*setupStatus_,*publicationStatus_;
    QTableWidget* stations_;
    QPlainTextEdit *history_,*migrationReport_;
    QLineEdit *address_,*migrationRequest_;
    QSpinBox* port_;
    QCheckBox* enabled_;
    QCheckBox* remote_;
    QComboBox* recipes_;
    QString recipeSignature_;
    QTableWidget* clients_;
    QLabel* clientStatus_;
    QLineEdit* clientLabel_;
    QString clientSignature_{"<unset>"};
    QPushButton *run_,*automatic_,*manual_,*stop_;
    std::optional<CommunicationMigrationInput> reviewedInput_;
    QString reviewedRequest_,workspacePath_,packagePath_;
    QByteArray planDigest_,manifestDigest_,pointerDigest_,ledgerDigest_;
};
}
