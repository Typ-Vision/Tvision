#include "app/PrecisionInput.hpp"
#include "app/MetrologyEditor.hpp"
#include "app/EdgePointEditor.hpp"
#include "app/PoseMatchEditor.hpp"
#include "app/RoiEditor.hpp"
#include "app/ImageViewer.hpp"
#include "app/CalibrationWizard.hpp"
#include "domain/ToolRegistry.hpp"
#include <QApplication>
#include <QFontDatabase>
#include <QScrollArea>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QDir>
#include <QJsonDocument>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>
#include <stdexcept>
#include <cstdio>
using namespace visionflow;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
QByteArray hash(const QJsonObject& p){return QCryptographicHash::hash(QJsonDocument(p).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);}
void type(QDoubleSpinBox* spin,const QString& text){spin->setFocus();spin->selectAll();QTest::keyClicks(spin,text);QTest::keyClick(spin,Qt::Key_Return);}
}
int main(int argc,char** argv){QApplication app(argc,argv);QFontDatabase::addApplicationFont("C:/Windows/Fonts/segoeui.ttf");try{
    const double exact=1.23456789;
    auto p=defaultParameters("circle_fit");p["diameterLower"]=exact;p["maxRms"]=.000123456789;p["unitsPerSample"]=.0000123456789;
    MetrologyEditor editor("circle_fit",p,"raw",true);editor.show();
    check(hash(editor.parameters())==hash(p),"load no-op compact JSON/hash");
    auto* length=editor.findChild<QDoubleSpinBox*>("mt_diameterLower");
    auto* tiny=editor.findChild<QDoubleSpinBox*>("mt_maxRms");
    check(length->decimals()==3 && length->singleStep()==.001 && tiny->value()==0,"length presentation");
    check(tiny->toolTip().contains("0.000123456789"),"legacy tooltip");
    editor.findChild<QCheckBox*>("mt_ransac")->toggle();
    auto changed=editor.parameters();check(changed["diameterLower"]==exact && changed["maxRms"]==p["maxRms"] && changed["unitsPerSample"]==p["unitsPerSample"],"unrelated edit preserves original doubles");
    // Typing the already visible value is an explicit edit, even without valueChanged.
    type(length,"1.235");check(editor.parameters()["diameterLower"]==1.235,"explicit same-display commit");
    type(tiny,"0.000");check(editor.parameters()["maxRms"]==.001,"new positive minimum");
    loadPrecisionInput(length,exact,true);length->setFocus();length->selectAll();QTest::keyClicks(length,"2.345");QTest::keyClick(length,Qt::Key_Escape);
    check(editor.parameters()["diameterLower"]==exact,"Escape restores original precision");
    loadPrecisionInput(length,exact,true);type(length,"2.345");type(length,"1.235");check(editor.parameters()["diameterLower"]==1.235,"typed return commits displayed value");
    loadPrecisionInput(length,exact,true);length->setFocus();length->selectAll();QTest::keyClicks(length,"3.456");tiny->setFocus();QApplication::processEvents();check(editor.parameters()["diameterLower"]==3.456,"focus loss commits edit");
    loadPrecisionInput(length,exact,true);length->stepUp();length->stepDown();check(editor.parameters()["diameterLower"]==1.235,"step round trip is an explicit edit");
    const auto encoded=QJsonDocument(editor.parameters()).toJson();const auto reopened=QJsonDocument::fromJson(encoded).object();MetrologyEditor reopen("circle_fit",reopened,"raw",true);check(hash(reopen.parameters())==hash(reopened),"encode reopen no-op");
    auto edge=defaultParameters("edge_point");edge["sigmaPx"]=exact;EdgePointEditor ep(edge);ep.findChild<QCheckBox*>("ep_subpixel")->toggle();check(ep.parameters()["sigmaPx"]==exact,"edge unrelated commit");type(ep.findChild<QDoubleSpinBox*>("ep_sigmaPx"),"1.235");check(ep.parameters()["sigmaPx"]==1.235,"edge explicit commit");
    auto pose=defaultParameters("pattern_pose");pose["nmsDistance"]=12.3456789;pose["anchor"]=QJsonObject{{"x",exact},{"y",2.34567891}};PoseMatchEditor pe("pattern_pose",pose);check(hash(pe.parameters())==hash(pose),"pose load no-op");pe.findChild<QComboBox*>("pose_selection")->setCurrentText("best");check(pe.parameters()["anchor"]==pose["anchor"] && pe.parameters()["nmsDistance"]==pose["nmsDistance"],"pose unrelated commit");
    QDoubleSpinBox locale;locale.setRange(-1e8,1e12);locale.setLocale(QLocale(QLocale::Turkish));loadPrecisionInput(&locale,-exact,true);type(&locale,"-2,345");check(precisionValue(&locale)==-2.345,"Turkish negative coordinate");loadPrecisionInput(&locale,99999999.123456,true);check(precisionValue(&locale)==99999999.123456,"large untouched coordinate");
    QWidget parent;ImageViewer viewer;viewer.setImage(QImage(100,100,QImage::Format_Grayscale8));ToolRoi roi{{"raw"},ScanSegment{{exact,20.123456789},{50.123456789,20.123456789},.000123456789}};viewer.setRoiFrame("raw");viewer.setRoi(roi);showRoiEditor(&viewer,&parent,"raw");auto* dialog=parent.findChild<QDialog*>("roiGeometryDialog");dialog->findChild<QPushButton*>("roiApply")->click();check(viewer.roi()->toJson()==roi.toJson(),"ROI apply no-op preserves all coordinates");dialog->close();
    CalibrationWizard wizard(false,std::nullopt,QDir::currentPath(),[](const auto&){});check(wizard.findChild<QDoubleSpinBox*>("wizardKnownMm")->decimals()==3 && wizard.findChild<QDoubleSpinBox*>("wizardContrast")->decimals()==6,"wizard units exclude contrast");wizard.close();
    const auto evidence=qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS",QDir::currentPath()+"/../ccux/ccux-24d");QDir().mkpath(evidence);
    editor.hide();QScrollArea preview;preview.setWidgetResizable(true);preview.setWidget(&editor);preview.resize(500,900);preview.show();QApplication::processEvents();preview.grab().save(evidence+"/precision-normal.png");preview.resize(330,600);QApplication::processEvents();preview.grab().save(evidence+"/precision-small.png");preview.takeWidget();
    qInfo("precision input: load/unrelated commit/explicit edit/encode/hash/reopen/locale/minimum/Escape/ROI/wizard passed");return 0;
}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
