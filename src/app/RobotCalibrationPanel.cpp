#include "RobotCalibrationPanel.hpp"
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
namespace visionflow {
namespace {
QLabel* label(const char* name){auto* l=new QLabel;l->setObjectName(name);l->setWordWrap(true);l->setTextFormat(Qt::PlainText);l->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);return l;}
QString id(const QUuid& u){return u.isNull()?QString("yok"):u.toString(QUuid::WithoutBraces);}
}
RobotCalibrationPanel::RobotCalibrationPanel(QWidget* parent):QWidget(parent){
    setObjectName("robotCalibrationWorkspace");auto* root=new QVBoxLayout(this);
    root->addWidget(new QLabel("Robot kalibrasyonu • BEGIN → SAMPLE → SOLVE → VALIDATE → ACTIVATE"));
    auto* form=new QFormLayout;mode_=new QComboBox;mode_->setObjectName("robotCalibrationMode");mode_->addItem("Eye-in-hand • hareketli kamera / sabit plaka",int(HandEyeMode::EyeInHand));mode_->addItem("Eye-to-hand • sabit kamera / hareketli plaka",int(HandEyeMode::EyeToHand));mode_->setAccessibleName("Robot kamera mount modu");form->addRow("&Mount modu",mode_);root->addLayout(form);
    readiness_=label("robotCalibrationReadiness");models_=label("robotCalibrationModels");progress_=label("robotCalibrationProgress");reasons_=label("robotCalibrationReasons");message_=label("robotCalibrationMessage");
    root->addWidget(readiness_);root->addWidget(models_);root->addWidget(progress_);
    samples_=new QTableWidget(0,6);samples_->setObjectName("robotCalibrationSamples");samples_->setAccessibleName("Kalibrasyon sample ve exact pose-frame eşleşmeleri");samples_->setHorizontalHeaderLabels({"Sample UUID","Role","Frame UUID","Durum","Reject nedeni","Pairing / token"});samples_->setEditTriggers(QAbstractItemView::NoEditTriggers);samples_->setMinimumHeight(125);samples_->setMaximumHeight(210);samples_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);samples_->horizontalHeader()->setStretchLastSection(true);root->addWidget(samples_);
    auto* grid=new QGridLayout;using A=RobotCalibrationAction;
    const QVector<QPair<A,QString>> items{{A::Begin,"&BEGIN"},{A::SampleSolve,"SAMPLE • &solve"},{A::SampleHoldout,"SAMPLE • &holdout"},{A::Solve,"S&OLVE → candidate"},{A::Validate,"&VALIDATE"},{A::Activate,"ACTI&VATE"},{A::Abort,"A&BORT / drain"},{A::Recover,"&Recovery (açık eylem)"}};
    int n=0;for(const auto& item:items){auto* b=new QPushButton(item.second);b->setObjectName("robotCalibrationAction"+QString::number(int(item.first)));b->setAccessibleName(item.second);grid->addWidget(b,n/3,n%3);++n;actions_[item.first]=b;connect(b,&QPushButton::clicked,this,[this,a=item.first]{command(a);});}root->addLayout(grid);root->addWidget(reasons_);root->addWidget(message_);
    connect(mode_,&QComboBox::currentIndexChanged,this,[this]{refresh();});refresh();
}
void RobotCalibrationPanel::bind(RobotCalibrationWorkspace* w){workspace_=w?w:&fallback_;refresh();}
void RobotCalibrationPanel::selectStation(const QUuid& id){station_=id;refresh();}
void RobotCalibrationPanel::refresh(){
    try{
        view_=workspace_->view(station_,HandEyeMode(mode_->currentData().toInt()));
        mode_->setEnabled(view_.session.isNull() || calibrationStateTerminal(view_.state));
        readiness_->setText(QString("%1\nFrame: %2\nTCP: %3\nLens/intrinsics: %4\nActual pose: %5\nKamera: %6 • Lease sahibi: %7").arg(view_.physical,view_.frameReadiness,view_.tcpReadiness,view_.lensReadiness,view_.actualPose,view_.camera,view_.leaseOwner));
        models_->setText(QString("Candidate: %1 / %2\nValidation: %3\nActive: %4 / %5").arg(id(view_.candidate),id(view_.candidateRevision),id(view_.validation),id(view_.active),id(view_.activeRevision)));
        progress_->setText(QString("%1%2 • Session: %3\n%4%5").arg(view_.fixture?"SYNTHETIC fixture • ":"",calibrationStateName(view_.state),id(view_.session),view_.recoveryRequired?"RECOVERY_REQUIRED • ":"",view_.drain));
        samples_->setRowCount(view_.samples.size());for(int r=0;r<view_.samples.size();++r){const auto& s=view_.samples[r];const QStringList values{id(s.sample),s.role,id(s.frame),s.status,s.reason,s.pairing};for(int c=0;c<values.size();++c){auto* item=new QTableWidgetItem(values[c]);item->setToolTip(values[c]);samples_->setItem(r,c,item);}}
        QStringList reasons;for(auto i=actions_.begin();i!=actions_.end();++i){const auto reason=workspace_->disabled(view_,i.key());i.value()->setEnabled(reason.isEmpty());i.value()->setToolTip(reason);i.value()->setAccessibleDescription(reason);if(!reason.isEmpty())reasons<<i.value()->text().remove('&')+": "+reason;}
        reasons_->setText(reasons.join('\n'));
    }catch(const std::exception& e){for(auto* b:actions_)b->setEnabled(false);message_->setText("RECOVERY_REQUIRED • "+QString::fromUtf8(e.what()));}
}
void RobotCalibrationPanel::command(RobotCalibrationAction a){try{message_->setText(workspace_->command({a,view_}));}catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}refresh();}
}
