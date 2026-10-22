#include "app/CalibrationOverview.hpp"
#include "app/CalibrationWizard.hpp"
#include <QApplication>
#include <QGroupBox>
#include <QTemporaryDir>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTimer>
#include <QTest>
#include <QTextStream>
#include <QFontDatabase>
#include <QFile>
using namespace visionflow;
namespace {int checks=0;void check(bool b,const char* why){++checks;if(!b)throw std::runtime_error(why);}
MeasurementSetupContract fixture(int n){MeasurementSetupContract c;for(int i=0;i<n;++i){auto id=QString::number(i);c.cameras.append({{"c"+id},i==0 ? QString(110,'W'):"Kamera "+id,{"webcam","provider-"+id,DeviceIdentityConfidence::ProviderReported,1,{true,true,false,false}}});c.setups.append({{"s"+id},c.cameras.last().id,"Ölçüm kurulumu "+id,"lens","plane",1,1,{{640,480},{},{},true}});}return c;}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");app.setStyle("Fusion");app.setFont(QFont("Segoe UI",10));QFile theme("../../resources/theme.qss");if(theme.open(QIODevice::ReadOnly))app.setStyleSheet(QString::fromUtf8(theme.readAll()));
QTemporaryDir dir(QDir::currentPath()+"/overview-XXXXXX");check(dir.isValid(),"build fixtures");int discoveries=0;bool gate=true;QVector<DeviceBinding> devices;for(const auto& c:fixture(10).cameras)devices.append(c.device);
for(const auto& c:fixture(12).cameras)if(!devices.contains(c.device))devices.append(c.device);
MeasurementWorkspace workspace(dir.filePath("empty.vfproject"),"project",[&]{return gate;},[&]{++discoveries;return devices;});MeasurementWorkspace* context=nullptr;
CalibrationOverview view([&]{return context;});view.resize(850,600);view.show();app.processEvents();check(view.findChild<QLabel*>("overviewSummary")->text().contains("Reçete"),"no project explains prerequisite");context=&workspace;view.refresh();check(workspace.environment().current.cameras.isEmpty() && discoveries==0,"zero recipe zero camera no discovery");
view.findChild<QGroupBox*>("overviewDetails")->setChecked(true);
for(int method:{0,1}){QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(d && d->objectName()=="calibrationWizard","Zero-camera method opens recipe-free trial");check(!d->findChild<QPushButton*>("wizardAcquire")->isEnabled(),"Zero camera cannot invent capture");d->reject();});view.findChild<QPushButton*>("overviewMethod"+QString::number(method))->click();}
QTimer::singleShot(0,[&]{auto* choice=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(choice && choice->objectName()=="trialMethodChoice","Trial method chooser");QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(d && d->windowTitle().contains("plakası"),"Zero-target plate trial available");d->reject();});choice->findChild<QPushButton*>("trialPlate")->click();});view.findChild<QPushButton*>("overviewMethod2")->click();
view.findChild<QGroupBox*>("overviewDetails")->setChecked(false);
for(int n:{0,1,5,10}){auto contract=fixture(n);if(n)workspace.replace(contract);view.refresh();app.processEvents();auto cards=view.findChildren<QPushButton*>(QRegularExpression("overviewCamera_.*"));check(cards.size()==n,"all N cards retained");check(discoveries==0,"refresh never discovers or acquires");
if(n==1)for(int method=0;method<2;++method){
 QTimer::singleShot(0,[&,method]{auto* choice=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(choice && choice->objectName()=="calibrationMethodChoice","Normal method chooser");check(!choice->findChild<QPushButton*>("calibrateRobot")->isEnabled(),"Robot explicitly unsupported");QTimer::singleShot(0,[&,method]{auto* wizard=qobject_cast<QDialog*>(QApplication::activeModalWidget());check(wizard && wizard->objectName()=="calibrationWizard","Both normal methods open independently");check(wizard->findChild<QLabel*>("wizardSourceNote")->text().contains(fixture(1).cameras[0].name),"Selected camera copied into immutable target");check(!wizard->findChild<QLineEdit*>("wizardLens")->isVisible(),"No normal setup/profile question");wizard->resize(1400,900);app.processEvents();wizard->grab().save(QString("../ccux/ccux-09/start-%1-1400.png").arg(method));wizard->resize(850,600);app.processEvents();wizard->grab().save(QString("../ccux/ccux-09/start-%1-850.png").arg(method));wizard->reject();});choice->findChild<QPushButton*>(method ? "calibratePlate":"calibrateKnown")->click();});view.findChild<QPushButton*>("overviewCalibrate")->setFocus();QTest::keyClick(view.findChild<QPushButton*>("overviewCalibrate"),Qt::Key_Space);
}
for(const auto size:{QSize(1400,900),QSize(850,600)}){view.resize(size);app.processEvents();view.refresh();app.processEvents();auto* scroll=view.findChild<QScrollArea*>();check(scroll->horizontalScrollBar()->maximum()==0,"no horizontal overflow");for(auto* card:view.findChildren<QPushButton*>(QRegularExpression("overviewCamera_.*"))){if(n>1){const auto cameraId=card->objectName().mid(15);scroll->ensureWidgetVisible(card);app.processEvents();check(card->isVisible(),"every card reachable");QTest::mouseClick(card,Qt::LeftButton);check(view.selectedCamera()==cameraId,"card selection by stable ID");break;}}
app.processEvents();QTest::qWait(30);view.findChild<QScrollArea*>()->verticalScrollBar()->setValue(0);app.processEvents();check(view.grab().save(QDir::currentPath()+QString("/../ccux/ccux-09/overview-%1-%2.png").arg(n).arg(size.width())),"save visual evidence");}
}
view.findChild<QGroupBox*>("overviewDetails")->setChecked(true);app.processEvents();
// Cards are recreated on refresh; reach all ten by fresh stable ID lookup.
auto* scroll=view.findChild<QScrollArea*>();for(int i=0;i<10;++i){auto* b=view.findChild<QPushButton*>("overviewCamera_c"+QString::number(i));scroll->ensureWidgetVisible(b);QTest::mouseClick(b,Qt::LeftButton);check(view.selectedCamera()=="c"+QString::number(i),"ten cards independently accessible");}
auto* last=view.findChild<QPushButton*>("overviewCamera_c9");last->setFocus();QTest::keyClick(last,Qt::Key_Space);check(view.selectedCamera()=="c9","keyboard card action");
// A real setup-file calculation appears only under its exact selected setup.
QImage stripe(640,480,QImage::Format_Grayscale8);stripe.fill(20);for(int y=0;y<480;++y)for(int x=120;x<520;++x)stripe.scanLine(y)[x]=220;const auto imagePath=dir.filePath("stripe.png");check(stripe.save(imagePath),"actual setup file fixture");
for(int change=0;change<2;++change){const auto before=workspace.environment().selections;QTimer::singleShot(0,[&,change]{auto* wizard=dynamic_cast<CalibrationWizard*>(QApplication::activeModalWidget());check(wizard,"Targeted wizard before cancellation");wizard->openFile(imagePath);if(change==0)view.findChild<QPushButton*>("overviewCamera_c0")->click();else context=nullptr;});view.findChild<QPushButton*>("overviewMethod0")->click();context=&workspace;view.refresh();check(workspace.environment().selections==before,"Camera/project change cancels old pending work without saving");view.findChild<QPushButton*>("overviewCamera_c9")->click();}
IndependentCalibrationSession session;IndependentCaptureRequest request;request.origin=SourceOrigin::SetupFile;request.filePath=imagePath;request.camera=fixture(10).cameras.last();request.setup=fixture(10).setups.last();check(session.accept(IndependentCalibrationSession::run(session.begin(request))),"recipe-free file admission");
auto& math=session.math();EdgePairOptions options;options.profile={.5,1,1};check(math.accept(CalibrationWizardSession::run(math.scanPair("pair",{{"image"},ScanSegment{{20,100},{620,100},4}},options))),"actual pair calculation");math.setKnownMillimetres(10);auto record=math.calculate();workspace.calibrations().save({request.camera->id,request.setup->id,1},record);view.refresh();
auto* records=view.findChild<QTableWidget*>("overviewRecords");check(records->rowCount()==1 && records->item(0,2)->text()=="Yapılmadı","new real record and absent control visible");app.processEvents();scroll->ensureWidgetVisible(records);app.processEvents();view.grab().save(QDir::currentPath()+"/kmc5-overview-records-850.png");auto unselected=workspace.environment();records->selectRow(0);check(workspace.environment().revision==unselected.revision && unselected.selections.isEmpty(),"row selection and candidate save do not activate");
view.findChild<QPushButton*>("overviewDiscover")->click();check(discoveries==1 && workspace.availability(fixture(10).cameras[0]).contains("Keşfedildi"),"explicit metadata discovery only");
auto before=workspace.environment();view.findChild<QPushButton*>("overviewOperational")->click();auto after=workspace.environment();check(after.operational.value("c9")=="s9","explicit operational persistence");view.findChild<QPushButton*>("overviewOperational")->click();check(workspace.environment().revision==after.revision,"operational no-op");
// Dialog no-op Save keeps revision; rename preserves setup revision.
auto edit=[&](bool rename){QTimer::singleShot(0,[&,rename]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!d)return;auto* name=d->findChild<QLineEdit*>("overviewCameraName");if(rename)name->setText("Renamed camera");d->grab().save(QDir::currentPath()+"/kmc5-setup-form.png");d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();});view.findChild<QPushButton*>("overviewEdit")->click();};
edit(false);check(workspace.environment().revision==after.revision,"form no-op preserves document");edit(true);check(workspace.environment().current.cameras.last().name=="Renamed camera" && workspace.environment().current.setups.last().revision==1,"rename preserves geometry");
QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!d)return;d->findChild<QSpinBox*>("overviewWidth")->setValue(800);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();if(d->result()!=QDialog::Accepted)d->reject();});view.findChild<QPushButton*>("overviewEdit")->click();check(workspace.environment().current.setups.last().revision==2,"geometry edit advances setup revision");check(records->rowCount()==1 && records->item(0,1)->text().contains("Eski kurulum"),"historical record remains visible and unusable after geometry change");
gate=false;view.findChild<QPushButton*>("overviewArchive")->click();check(workspace.environment().archivedCameras.isEmpty() && !view.findChild<QLabel*>("overviewStatus")->text().isEmpty(),"busy mutation rejected visibly");gate=true;
view.findChild<QPushButton*>("overviewArchive")->click();check(workspace.environment().archivedCameras.contains("c9"),"archive persists history");
bool accepted=false;QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!d)return;
d->findChild<QLineEdit*>("overviewCameraName")->setText("New camera");d->findChild<QComboBox*>("overviewDevice")->setCurrentIndex(10);d->findChild<QLineEdit*>("overviewSetupName")->setText("New setup");d->findChild<QLineEdit*>("overviewLens")->setText("Lens");d->findChild<QLineEdit*>("overviewPlane")->setText("Plane");d->findChild<QCheckBox*>("overviewGeometryKnown")->setChecked(true);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();accepted=d->result()==QDialog::Accepted;if(!accepted)d->reject();});
view.findChild<QPushButton*>("overviewAdd")->click();check(accepted && workspace.environment().current.cameras.size()==11,"real camera/setup add command");
const auto newCamera=view.selectedCamera();auto newSetup=workspace.environment().current.setups.last();
QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!d)return;d->findChild<QLineEdit*>("overviewSetupName")->setText("Second plane");d->findChild<QLineEdit*>("overviewLens")->setText("Lens");d->findChild<QLineEdit*>("overviewPlane")->setText("Plane 2");d->findChild<QCheckBox*>("overviewGeometryKnown")->setChecked(true);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();accepted=d->result()==QDialog::Accepted;if(!accepted)d->reject();});
view.findChild<QPushButton*>("overviewAddSetup")->click();check(accepted && workspace.environment().current.setups.size()==12,"multiple setups for one camera");
QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!d)return;d->findChild<QCheckBox*>("overviewRemap")->setChecked(true);d->findChild<QComboBox*>("overviewDevice")->setCurrentIndex(11);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();accepted=d->result()==QDialog::Accepted;if(!accepted)d->reject();});
view.findChild<QPushButton*>("overviewEdit")->click();check(accepted,"explicit discovered device remap");for(const auto& s:workspace.environment().current.setups)if(s.camera.value==newCamera)check(s.deviceRevision==2 && s.revision==2,"remap revisions all camera setups");
view.findChild<QPushButton*>("overviewArchiveSetup")->click();check(workspace.environment().archivedSetups.size()==1,"setup archive command");
MeasurementWorkspace restart(dir.filePath("empty.vfproject"),"project",[]{return true;});check(restart.environment().current.cameras.size()==11 && restart.environment().archivedCameras.size()==1,"restart restores definitions only");
check(discoveries==1,"UI selection editing never captures or discovers implicitly");
auto stale=workspace.environment();MeasurementEnvironmentRepository external(dir.filePath("empty.vfproject"),"project");auto changed=stale;changed.current.cameras[0].name="External rename";external.commit(stale,changed);bool conflict=false;try{workspace.replace(stale.current,stale.revision);}catch(const std::exception&){conflict=true;}check(conflict && external.load().current.cameras[0].name=="External rename","stale form cannot overwrite external registry edits");
QTextStream(stdout)<<checks<<" overview checks passed; zero recipes; discovery fixture, no camera opens\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
