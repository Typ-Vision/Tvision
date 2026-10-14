#include <QString>
#include "RoiEditor.hpp"
#include "ImageViewer.hpp"
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QPointer>
#include <QSignalBlocker>
namespace visionflow {
namespace {
class RoiEditor : public QDialog {
    QPointer<ImageViewer> viewer_;
    QComboBox* shape_;
    QTableWidget* table_;
    QLabel* status_;
    QJsonObject geometry_;
    bool imageAvailable_=true;
    QString frame_;
    void populate(QJsonObject g){
        geometry_=g;table_->setRowCount(0);
        auto add=[&](QString key,double value){int row=table_->rowCount();table_->insertRow(row);auto* label=new QTableWidgetItem(key);label->setFlags(label->flags() & ~Qt::ItemIsEditable);table_->setItem(row,0,label);auto* spin=new QDoubleSpinBox;spin->setRange(-1e8,1e8);spin->setDecimals(9);spin->setValue(value);table_->setCellWidget(row,1,spin);};
        for(auto it=g.begin();it!=g.end();++it){if(it->isDouble())add(it.key(),it->toDouble());else if(it.key()=="vertices"){const auto points=it->toArray();for(int i=0;i<points.size();++i){const auto p=points[i].toArray();add(QString("P%1.x").arg(i+1),p[0].toDouble());add(QString("P%1.y").arg(i+1),p[1].toDouble());}}else if(it->isArray()){const auto p=it->toArray();add(it.key()+".x",p[0].toDouble());add(it.key()+".y",p[1].toDouble());}}
    }
    QJsonObject read() const {
        auto g=geometry_;auto points=g.value("vertices").toArray();
        for(int row=0;row<table_->rowCount();++row){const auto key=table_->item(row,0)->text();const auto value=static_cast<QDoubleSpinBox*>(table_->cellWidget(row,1))->value();const auto dot=key.indexOf('.');if(dot<0)g[key]=value;else {const auto name=key.left(dot);const int axis=key.endsWith('x') ? 0:1;if(name.startsWith('P')){const int i=name.mid(1).toInt()-1;auto p=points[i].toArray();p[axis]=value;points[i]=p;}else{auto p=g[name].toArray();p[axis]=value;g[name]=p;}}}
        if(g.contains("vertices"))g["vertices"]=points;return g;
    }
    void defaults(){
        if(!viewer_)return;const auto size=viewer_->sceneRect().size();const QPointF c(size.width()/2,size.height()/2);const double r=std::max(1.0,std::min(size.width(),size.height())/5);
        ToolRoi roi{{frame_},ScanSegment{c-QPointF(r,0),c+QPointF(r,0),r/4}};
        switch(shape_->currentIndex()){
        case 1:roi.geometry=RotatedRectangle{c,{r*2,r},30};break;
        case 2:roi.geometry=Annulus{c,r/2,r};break;
        case 3:roi.geometry=ArcRoi{{c,r/2,r},0,90};break;
        case 4:roi.geometry=PolygonRoi{{c+QPointF(-r,-r),c+QPointF(r,-r),c+QPointF(0,r)}};break;
        case 5:roi.geometry=PathRoi{{c-QPointF(r,0),c+QPointF(r,0)}};break;
        }populate(roi.toJson()["geometry"].toObject());
    }
    void changed(){
        if(!viewer_)return;imageAvailable_=viewer_->hasImage();for(const auto& id:{"roiDraw","roiApply","roiUndo","roiClear"})if(auto* b=findChild<QPushButton*>(id))b->setEnabled(imageAvailable_);if(viewer_->roi()){const auto& roi=*viewer_->roi();const QSignalBlocker block(shape_);shape_->setCurrentIndex(int(roi.geometry.index()));populate(roi.toJson()["geometry"].toObject());}else defaults();status_->setText(viewer_->roiStatus());
    }
public:
    RoiEditor(ImageViewer* viewer,QWidget* parent,QString frame):QDialog(parent),viewer_(viewer),frame_(std::move(frame)){
        setObjectName("roiGeometryDialog");setWindowTitle("ROI geometrisi — geçici");setAttribute(Qt::WA_DeleteOnClose);resize(380,540);
        auto* layout=new QVBoxLayout(this);auto* help=new QLabel("Geometri çalışma alanı; reçeteye kaydedilmez. Algoritma veya ölçüm çalıştırmaz.\nOk/kutu/halka/yay: sürükle. Polygon/path: tıkla, Enter ile bitir. Tutamaçları sürükle; Escape iptal, Ctrl+Z geri al.");help->setWordWrap(true);layout->addWidget(help);
        shape_=new QComboBox;shape_->setObjectName("roiShape");shape_->addItems({"Yönlü ok / şerit","Dönmüş kutu","Halka","Yay","Polygon","Path"});layout->addWidget(shape_);
        table_=new QTableWidget;table_->setObjectName("roiNumbers");table_->setColumnCount(2);table_->setHorizontalHeaderLabels({"Parametre (px / derece)","Değer"});table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);layout->addWidget(table_,1);
        status_=new QLabel;status_->setObjectName("roiStatus");status_->setWordWrap(true);layout->addWidget(status_);
        auto* actions=new QHBoxLayout;layout->addLayout(actions);
        auto add=[&](QString text,QString id,auto action){auto* b=new QPushButton(text);b->setObjectName(id);actions->addWidget(b);connect(b,&QPushButton::clicked,this,action);};
        add("Çiz","roiDraw",[this]{if(viewer_)viewer_->beginRoi(shape_->currentIndex());});
        add("Uygula","roiApply",[this]{if(!viewer_)return;try{viewer_->setRoi(ToolRoi::fromJson({{"schemaVersion",1},{"frameId",frame_},{"geometry",read()}}));}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));}});
        add("Geri al","roiUndo",[this]{if(viewer_)viewer_->undoRoi();});
        add("Temizle","roiClear",[this]{if(viewer_)viewer_->setRoi({});});
        auto* vertices=new QHBoxLayout;layout->addLayout(vertices);
        auto* append=new QPushButton("Nokta ekle");auto* remove=new QPushButton("Son noktayı sil");vertices->addWidget(append);vertices->addWidget(remove);
        connect(append,&QPushButton::clicked,this,[this]{auto g=read();if(!g.contains("vertices"))return;auto points=g.value("vertices").toArray();if(points.size()>=1024)return;points.append(QJsonArray{0.0,0.0});g["vertices"]=points;populate(g);});
        connect(remove,&QPushButton::clicked,this,[this]{auto g=read();auto p=g["vertices"].toArray();if(p.isEmpty())return;p.removeLast();g["vertices"]=p;populate(g);});
        connect(shape_,&QComboBox::currentIndexChanged,this,[this]{defaults();});
        const QPointer<RoiEditor> self(this);viewer->onRoiChanged=[self]{if(self)self->changed();};viewer->onRoiStatus=[self]{if(self && self->viewer_)self->status_->setText(self->viewer_->roiStatus());};
        connect(this,&QDialog::finished,this,[this]{if(viewer_)viewer_->cancelRoi();});changed();
    }
};
}
void showRoiEditor(ImageViewer* viewer,QWidget* parent,const QString& frameId){
    if(!viewer->hasImage())return;
    if(auto* existing=parent->findChild<QDialog*>("roiGeometryDialog")){existing->raise();existing->activateWindow();return;}
    viewer->onRoiEdited={};viewer->onRoiChanged={};viewer->resetRoiHistory();viewer->setRoiRecipeBound(false);viewer->setRoiFrame(frameId);auto* editor=new RoiEditor(viewer,parent,frameId);editor->show();
}
}
