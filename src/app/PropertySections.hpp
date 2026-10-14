#pragma once
#include <QFormLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QJsonObject>
#include <functional>

namespace visionflow {
// Presentation state belongs to the local UI settings, never to a recipe.
inline QWidget* propertySection(QFormLayout* host,const QString& title,const QString& key,const QString& file) {
    auto* body=new QWidget;body->setObjectName(key);
    auto* rows=new QFormLayout(body);rows->setContentsMargins(0,0,0,0);rows->setRowWrapPolicy(QFormLayout::WrapAllRows);
    auto* toggle=new QPushButton(title);toggle->setObjectName(key+"Toggle");toggle->setCheckable(true);
    const bool open=!file.isEmpty() && QSettings(file,QSettings::IniFormat).value("properties/"+key,false).toBool();
    toggle->setChecked(open);body->setVisible(open);host->addRow(toggle);host->addRow(body);
    QObject::connect(toggle,&QPushButton::toggled,body,[body,key,file](bool value){body->setVisible(value);if(!file.isEmpty())QSettings(file,QSettings::IniFormat).setValue("properties/"+key,value);});
    return body;
}
inline void movePropertyRow(QFormLayout* from,QWidget* field,QFormLayout* to) {
    const auto row=from->takeRow(field);
    auto* label=row.labelItem ? row.labelItem->widget():nullptr;
    if(label)to->addRow(label,field);else to->addRow(field);
    delete row.labelItem;delete row.fieldItem;
}
inline void propertyDetailMode(QFormLayout* host,const QString& kind,const QString& file,
                               const QList<QWidget*>& advanced,std::function<bool()> customized,QFormLayout* presentation=nullptr) {
    if(!presentation)presentation=host;
    auto* mode=new QComboBox;mode->setObjectName("propertyDetailMode");mode->addItems({"Basic","Detailed"});
    mode->setCurrentIndex(!file.isEmpty() && QSettings(file,QSettings::IniFormat).value("properties/"+kind+"/detailed",false).toBool() ? 1:0);
    presentation->insertRow(0,"Görünüm",mode);
    auto* hint=new QLabel("Gelişmiş ayarlar özelleştirilmiş");hint->setObjectName("advancedCustomized");hint->setWordWrap(true);presentation->insertRow(1,hint);
    auto update=[host,advanced,hint,customized](int index){for(auto* field:advanced)host->setRowVisible(field,index==1);hint->setVisible(customized() && index==0);};
    update(mode->currentIndex());
    QObject::connect(mode,&QComboBox::currentIndexChanged,mode,[update,file,kind](int index){update(index);if(!file.isEmpty())QSettings(file,QSettings::IniFormat).setValue("properties/"+kind+"/detailed",index==1);});
}
}
