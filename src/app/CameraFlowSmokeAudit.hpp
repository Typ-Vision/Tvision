#pragma once
// Acceptance-only checks called by fixture/smoke screenshot paths.
#include <QAbstractButton>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QWidget>
#include <stdexcept>
namespace visionflow {
struct CameraFlowSmokeAudit {
    static void text(const QString& value,const QString& object) {
        for(const auto& removed:{QStringLiteral("Deneysel OECS"),QStringLiteral("Experimental OECS"),
                QStringLiteral("Tetik–kare"),QStringLiteral("Tetik-kare"),
                QStringLiteral("eşleşmesi doğrulan"),QStringLiteral("Önceki akış"),
                QStringLiteral("eski kanıt ekran")})
            if(value.contains(removed,Qt::CaseInsensitive))
                throw std::runtime_error(("Removed camera UI text in "+object+": "+value).toStdString());
    }
    static bool save(QWidget& root,const QString& path) {
        for(const auto name:{"cameraExperimental","cameraExperimentalNotice","overviewExperimental",
                "experimentalEditor","experimentalRuntime","calibrationLegacy"})
            if(root.findChild<QWidget*>(name))throw std::runtime_error("Removed camera UI widget exists");
        auto widgets=root.findChildren<QWidget*>();widgets.prepend(&root);
        for(auto* widget:widgets)if(widget==&root || widget->isVisibleTo(&root)){
            const auto id=widget->objectName();text(widget->toolTip(),id);
            if(auto* label=qobject_cast<QLabel*>(widget))text(label->text(),id);
            if(auto* button=qobject_cast<QAbstractButton*>(widget))text(button->text(),id);
            if(auto* combo=qobject_cast<QComboBox*>(widget))for(int i=0;i<combo->count();++i){
                text(combo->itemText(i),id);text(combo->itemData(i,Qt::ToolTipRole).toString(),id);
            }
            if(auto* table=qobject_cast<QTableWidget*>(widget))
                for(int r=0;r<table->rowCount();++r)for(int c=0;c<table->columnCount();++c)
                    if(auto* item=table->item(r,c)){text(item->text(),id);text(item->toolTip(),id);}
        }
        return root.grab().save(path);
    }
};
}
