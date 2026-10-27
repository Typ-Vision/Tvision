#pragma once
#include <QWidget>
#include "application/CommunicationProfiles.hpp"
#include "application/EndpointConnectionProbe.hpp"
class QComboBox; class QLineEdit; class QCheckBox; class QSpinBox; class QLabel;
namespace visionflow {
class EndpointProfileEditor final:public QWidget {
public:
    EndpointProfileEditor(QString path,std::function<bool()> active,QWidget* parent=nullptr);
private:
    CommunicationProfiles store_;EndpointConnectionProbe probe_;std::function<bool()> active_;
    QMap<QString,EndpointProfile> profiles_;EndpointProfile draft_;
    QComboBox *list_,*role_;QLineEdit *address_,*credential_;QCheckBox* enabled_;QSpinBox *port_,*timeout_,*retries_;QLabel* status_;
    void loadDraft();void refreshList(const QString& id={});void action(const QString&);
};
}
