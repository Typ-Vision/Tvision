#include "EdgeProfileView.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <algorithm>
namespace visionflow {
namespace {
class Plot : public QWidget {
public:
    std::optional<EdgeDiagnostics> data;
    explicit Plot(QWidget* parent):QWidget(parent){setMinimumHeight(210);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),QColor("#101923"));if(!data)return;const auto& d=*data;
        const QRectF upper(44,24,std::max(1,width()-56),66),lower(44,121,std::max(1,width()-56),66);
        double peak=1;for(auto v:d.derivative)peak=std::max(peak,std::abs(v.y()));for(auto c:d.candidates)peak=std::max(peak,std::abs(c.strength));
        auto xy=[&](double x,double y,bool derivative){const auto box=derivative ? lower:upper;return QPointF(box.left()+x/d.length*box.width(),derivative ? box.center().y()-y/peak*box.height()/2:box.bottom()-y/255*box.height());};
        p.setPen(QColor("#8193a8"));p.drawText(4,15,"gray8");p.drawText(4,112,"gray8/px");p.drawText(44,205,QString("0 → %1 px (A → B)").arg(d.length,0,'f',2));p.drawRect(upper);p.drawRect(lower);p.drawLine(lower.left(),lower.center().y(),lower.right(),lower.center().y());
        auto curve=[&](QColor color,auto points,bool derivative){QPainterPath path;bool begun=false;for(auto v:points){if(!v){begun=false;continue;}auto q=xy(v->x(),v->y(),derivative);if(begun)path.lineTo(q);else path.moveTo(q);begun=true;}p.setPen(QPen(color,1.5));p.drawPath(path);};
        QVector<std::optional<QPointF>> raw,smooth,derivative;for(auto s:d.profile){raw.append(QPointF(s.distance,s.raw));smooth.append(s.smooth ? std::optional<QPointF>{{s.distance,*s.smooth}}:std::nullopt);}for(auto s:d.derivative)derivative.append(s);
        curve(QColor("#56b4ff"),raw,false);curve(QColor("#ffd166"),smooth,false);curve(QColor("#66e3c4"),derivative,true);
        for(auto c:d.candidates){p.setPen(QPen(QColor(c.selected ? "#ffd166":c.accepted ? "#66e3c4":"#ff8585"),2));const auto q=xy(c.distance,c.strength,true);p.drawEllipse(q,c.selected ? 4:2,c.selected ? 4:2);}
    }
};
}
EdgeProfileView::EdgeProfileView(QWidget* parent):QWidget(parent){auto* l=new QVBoxLayout(this);l->setContentsMargins(0,0,0,0);info_=new QLabel;info_->setWordWrap(true);l->addWidget(info_);plot_=new Plot(this);l->addWidget(plot_);candidates_=new QTableWidget; candidates_->setObjectName("edgeCandidates");candidates_->setColumnCount(4);candidates_->setHorizontalHeaderLabels({"Mesafe px","Güç ±","Durum / ret","İnceltme"});candidates_->setEditTriggers(QAbstractItemView::NoEditTriggers);candidates_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);candidates_->setMaximumHeight(160);l->addWidget(candidates_);setDiagnostics({});}
void EdgeProfileView::setDiagnostics(const std::optional<EdgeDiagnostics>& d){diagnostics_=d;static_cast<Plot*>(plot_)->data=d;plot_->update();candidates_->setRowCount(0);setVisible(d.has_value());if(!d)return;
    info_->setText(QString("Gerçek profil: mavi ham, sarı Gaussian; yeşil signed türev. Aday: sarı seçili / yeşil kabul / kırmızı ret. Gösterilen örnek %1/%2, türev %3/%4, aday %5/%6. Büyük profiller yalnız gösterimde seyreltilir. Canlı tanı; arşive kaydedilmez.").arg(d->profile.size()).arg(d->sampleCount).arg(d->derivative.size()).arg(d->derivativeCount).arg(d->candidates.size()).arg(d->candidateCount));
    for(const auto& c:d->candidates){int r=candidates_->rowCount();candidates_->insertRow(r);QStringList values{QString::number(c.distance,'f',4),QString::number(c.strength,'f',4),c.selected ? "Seçili":c.accepted ? "Kabul":c.reason,c.refinement};for(int i=0;i<4;++i)candidates_->setItem(r,i,new QTableWidgetItem(values[i]));}
}
}
