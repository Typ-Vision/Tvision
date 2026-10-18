#pragma once
#include "application/MeasurementWorkspace.hpp"
#include <QWidget>
class QGridLayout;class QLabel;class QComboBox;class QTableWidget;
namespace visionflow {
class CalibrationOverview:public QWidget {
public:
    explicit CalibrationOverview(std::function<MeasurementWorkspace*()> context,QWidget* parent=nullptr);
    void refresh();
    void transferFile(const QString& action,const QString& path); // same command used by file chooser/tests
    std::function<void()> onChanged;
    std::function<void()> onCameraManager;
    void setRecipes(std::function<QVector<Recipe>()> f){recipesProvider_=std::move(f);}
    void setCaptureBackendForTests(IndependentCaptureBackend b){backend_=std::move(b);}
    void setLocalSettings(QString path){localSettings_=std::move(path);}
    QString selectedCamera() const{return selected_;}
protected:
    void resizeEvent(QResizeEvent*) override;
private:
    void attempt(std::function<void()>);
    void activate();
    void configureCapture(IndependentCaptureRequest&) const;
    QString localSettings_;
    void migrate();
    void transfer(const QString& action);
    std::function<QVector<Recipe>()> recipesProvider_;
    IndependentCaptureBackend backend_;
    void startTrial();
    void wizard(bool plate,bool trial);
    void associateTrial();
    void edit(bool addCamera,bool addSetup=false);
    void select(const QString&);
    void reflow();
    std::function<MeasurementWorkspace*()> context_;
    QString selected_;
    QLabel *status_,*summary_,*detail_;
    QWidget* cards_;
    QGridLayout* grid_;
    QComboBox *setups_,*profiles_;
    QTableWidget* records_;
    QVector<QWidget*> cardWidgets_;
    bool refreshing_=false;
};
}
