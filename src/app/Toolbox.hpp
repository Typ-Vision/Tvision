#pragma once
#include "application/ToolCatalog.hpp"
#include "application/BackendCapabilities.hpp"
#include <QListWidget>
#include <QSettings>
#include <QKeyEvent>
#include <QMap>
#include <QStyle>
#include <QPainter>
#include <functional>

namespace visionflow {
class Toolbox : public QListWidget {
public:
    enum {GroupRole=Qt::UserRole+1,HeaderRole=Qt::UserRole+2};
    explicit Toolbox(const QString& file,QWidget* parent=nullptr):QListWidget(parent),file_(file) {
        setObjectName("toolbox");setAccessibleName("Toolbox");
        const QVector<QPair<QString,QString>> groups{{"acquisition","Edinim"},{"processing","Ön işleme"},{"detection","Tespit"},{"matching","Eşleştirme"},{"measurement","Ölçüm"},{"calculation","Hesaplama"},{"other","Diğer"}};
        for(const auto& [id,label]:groups) {
            QVector<ToolDescriptor> entries;for(const auto& entry:ToolCatalog::entries())if(ToolCatalog::groupForKind(entry.kind)==id)entries.append(entry);
            if(entries.isEmpty())continue;
            auto* header=new QListWidgetItem(this);header->setData(GroupRole,id);header->setData(HeaderRole,true);header->setData(Qt::AccessibleDescriptionRole,"Araç grubu; Enter veya sağ/sol ok ile aç/kapat");
            auto font=header->font();font.setBold(true);header->setFont(font);header->setForeground(QColor("#9bb3c3"));header->setSizeHint(QSize(0,28));
            labels_[id]=label;headers_[id]=header;expanded_[id]=QSettings(file_,QSettings::IniFormat).value("editor/toolboxGroups/"+id,true).toBool();
            for(const auto& entry:entries) {
                auto* item=new QListWidgetItem(entry.label,this);item->setData(Qt::UserRole,entry.kind);item->setData(GroupRole,id);
                if(!entry.requiredBackend.isEmpty() && !BackendCapabilities::get(entry.requiredBackend).available){item->setFlags(item->flags() & ~Qt::ItemIsEnabled);item->setToolTip("Backend bu derlemede yok; araç eklenemez.");}
                else item->setToolTip(entry.kind=="industrial_camera" ? "Kayıtlı kamera, kurulum ve profil seçin; native edinim kabul kapılarına bağlıdır.":"Gerçek araç yürütmesi; girişleri Properties üzerinden bağlayın.");
            }
        }
        connect(this,&QListWidget::itemClicked,this,[this](QListWidgetItem* item){if(item->data(HeaderRole).toBool())toggle(item);});
        connect(this,&QListWidget::itemDoubleClicked,this,[this](QListWidgetItem* item){if(!item->data(HeaderRole).toBool() && onAddRequested)onAddRequested();});
        filter({});
        for(int i=0;i<count();++i)if(!this->item(i)->data(Qt::UserRole).toString().isEmpty()){setCurrentRow(i);break;}
    }
    std::function<void()> onAddRequested,onAvailabilityChanged;
    void filter(const QString& query) {
        query_=query.trimmed();
        for(auto it=headers_.begin();it!=headers_.end();++it){const auto id=it.key();bool matches=false;
            for(int i=0;i<count();++i){auto* entry=item(i);if(entry->data(HeaderRole).toBool() || entry->data(GroupRole)!=id)continue;
                const bool match=query_.isEmpty() || entry->text().contains(query_,Qt::CaseInsensitive) || entry->data(Qt::UserRole).toString().contains(query_,Qt::CaseInsensitive);
                matches|=match;entry->setHidden(!match || (query_.isEmpty() && !expanded_[id]));}
            it.value()->setHidden(!matches);it.value()->setText(labels_[id]);
            auto pixmap=style()->standardIcon(!query_.isEmpty() || expanded_[id] ? QStyle::SP_ArrowDown:QStyle::SP_ArrowRight).pixmap(12,12);
            {QPainter painter(&pixmap);painter.setCompositionMode(QPainter::CompositionMode_SourceIn);painter.fillRect(pixmap.rect(),QColor("#9bb3c3"));}
            it.value()->setIcon(QIcon(pixmap));
            it.value()->setToolTip(id+" / "+(id=="acquisition" ? QString("Görüntü edinimi"):labels_[id]));
        }
        if(onAvailabilityChanged)onAvailabilityChanged();
    }
protected:
    void keyPressEvent(QKeyEvent* event) override {
        auto* current=currentItem();
        if(current && current->data(HeaderRole).toBool() && (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter || event->key()==Qt::Key_Space || event->key()==Qt::Key_Left || event->key()==Qt::Key_Right)) {
            const auto id=current->data(GroupRole).toString();
            if((event->key()!=Qt::Key_Left || expanded_[id]) && (event->key()!=Qt::Key_Right || !expanded_[id]))toggle(current);
            event->accept();return;
        }
        if(current && !current->data(HeaderRole).toBool()) {
            if(event->key()==Qt::Key_Left){setCurrentItem(headers_.value(current->data(GroupRole).toString()));event->accept();return;}
            if(event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter){if(onAddRequested)onAddRequested();event->accept();return;}
        }
        QListWidget::keyPressEvent(event);
    }
private:
    void toggle(QListWidgetItem* header) {
        if(!query_.isEmpty())return;
        const auto id=header->data(GroupRole).toString();expanded_[id]=!expanded_[id];
        QSettings(file_,QSettings::IniFormat).setValue("editor/toolboxGroups/"+id,expanded_[id]);filter({});
    }
    QString file_,query_;
    QMap<QString,bool> expanded_;
    QMap<QString,QString> labels_;
    QMap<QString,QListWidgetItem*> headers_;
};
}
