#include "DeliveryDiagnosticsWidget.hpp"
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QSignalBlocker>
namespace visionflow {
DeliveryDiagnosticsWidget::DeliveryDiagnosticsWidget(std::function<QString()> archive,QWidget* parent):QWidget(parent),archive_(std::move(archive)) {
 auto* layout=new QVBoxLayout(this);
 status_=new QLabel;status_->setObjectName("deliveryStatus");status_->setTextFormat(Qt::PlainText);status_->setWordWrap(true);layout->addWidget(status_);
 auto* inspect=new QPushButton("Recovery inspect / Refresh");inspect->setObjectName("deliveryInspect");layout->addWidget(inspect);connect(inspect,&QPushButton::clicked,this,[this]{refresh();});
 table_=new QTableWidget(0,6);table_->setObjectName("deliveryTable");table_->setHorizontalHeaderLabels({"Endpoint / peer","Request ID","Cycle ID (Logs)","Delivery","Attempts","Decision"});table_->setSelectionBehavior(QAbstractItemView::SelectRows);table_->setSelectionMode(QAbstractItemView::SingleSelection);table_->setEditTriggers(QAbstractItemView::NoEditTriggers);table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table_->horizontalHeader()->setStretchLastSection(true);table_->setMinimumHeight(150);layout->addWidget(table_);
 connect(table_,&QTableWidget::itemSelectionChanged,this,[this]{selection();});
 detail_=new QPlainTextEdit;detail_->setObjectName("deliveryDetail");detail_->setReadOnly(true);detail_->setMaximumBlockCount(256);layout->addWidget(detail_,1);
 retry_=new QPushButton("Retry Delivery - same committed payload");retry_->setObjectName("deliveryRetry");layout->addWidget(retry_);connect(retry_,&QPushButton::clicked,this,[this]{retry();});
 message_=new QLabel;message_->setObjectName("deliveryMessage");message_->setWordWrap(true);message_->setTextFormat(Qt::PlainText);layout->addWidget(message_);
 auto* note=new QLabel("ACK_TIMEOUT is a delivery fault; vision NOK is a separate decision. Use Cycle ID in Runtime Logs. Recovery inspect is read-only. Requests without a terminal need operator reconciliation. Field values and free-form packet/error text are redacted.");note->setWordWrap(true);layout->addWidget(note);
 trace_=new QPlainTextEdit;trace_->setObjectName("deliveryTrace");trace_->setReadOnly(true);trace_->setMaximumBlockCount(128);trace_->setMaximumHeight(110);layout->addWidget(trace_);
 auto* timer=new QTimer(this);timer->setInterval(1000);connect(timer,&QTimer::timeout,this,[this]{if(isVisible())refresh();});timer->start();refresh();
}
void DeliveryDiagnosticsWidget::bindEndpoint(CommunicationTcpEndpoint* endpoint){endpoint_=endpoint;refresh();}
void DeliveryDiagnosticsWidget::refresh(){
 const int previous=table_->currentRow();QString selected;if(previous>=0 && previous<rows_.size())selected=rows_[previous].request.fingerprint+rows_[previous].request.requestId;
 QSignalBlocker block(table_);rows_.clear();table_->setRowCount(0);retry_->setEnabled(false);
 if(endpoint_){const auto state=endpoint_->diagnosticStatus();status_->setText(QString("Endpoint: %1 | Peer: %2\n%3 | Ready: %4 | Busy: %5").arg(state["endpoint"].toString().left(48),state["peer"].toString().left(48),state["state"].toString(),state["ready"].toBool()?"true":"false",state["busy"].toBool()?"true":"false"));status_->setToolTip(QString::fromUtf8(QJsonDocument(state).toJson(QJsonDocument::Indented)));}else{status_->setText("TCP v3: no bound production endpoint | Ready=false | Busy=false");status_->setToolTip({});}
 QStringList events;if(endpoint_)for(const auto& e:endpoint_->diagnosticEvents())events<<QString::fromUtf8(QJsonDocument(e).toJson(QJsonDocument::Compact));const auto trace=events.join('\n');if(trace_->toPlainText()!=trace)trace_->setPlainText(trace);
 try{const auto path=archive_();if(path.isEmpty() || !QFileInfo::exists(path)){detail_->clear();message_->setText("No durable delivery archive.");return;}
 rows_=DeliveryStore(path).recovery();int restore=-1;
 if(rows_.size()>128){rows_=rows_.mid(rows_.size()-128);message_->setText("128 requests shown; archive preserved.");}else message_->setText(QString("%1 durable requests | ACKED requires ACK_COMMITTED evidence").arg(rows_.size()));
 for(const auto& row:rows_){const auto o=QJsonDocument::fromJson(row.status.payload).object();const int n=table_->rowCount();table_->insertRow(n);
 const QStringList cells{row.request.endpoint+" / "+row.request.peer,row.request.requestId,row.status.cycleId,row.status.state=="ACK_COMMITTED"?QString("ACKED / ACK_COMMITTED"):row.status.state,QString::number(row.status.attempts),o["decision"].toString("NOT_EVALUATED")};
 for(int c=0;c<cells.size();++c){auto* item=new QTableWidgetItem(cells[c]);item->setToolTip(cells[c]);table_->setItem(n,c,item);}if(selected==row.request.fingerprint+row.request.requestId)restore=n;
 }if(restore>=0)table_->selectRow(restore);else if(!rows_.isEmpty())table_->selectRow(0);selection();
 }catch(...){rows_.clear();table_->setRowCount(0);detail_->clear();message_->setText("RECOVERY_REQUIRED - archive unavailable; secret/error details redacted.");}
}
void DeliveryDiagnosticsWidget::selection(){
 retry_->setEnabled(false);const int n=table_->currentRow();if(n<0 || n>=rows_.size()){detail_->clear();return;}
 const auto& row=rows_[n];const auto o=QJsonDocument::fromJson(row.status.payload).object();
 QStringList lines{"Request: "+row.request.requestId,"Cycle (Runtime Logs): "+row.status.cycleId,"Result: "+o["resultId"].toString(),"Delivery: "+row.status.state,"Execution: "+o["execution"].toString("UNKNOWN"),"Decision: "+o["decision"].toString("NOT_EVALUATED"),"Payload hash: "+row.status.payloadHash};
 for(const auto& v:o["fields"].toArray()){const auto f=v.toObject();const auto validity=f["validity"].toString("missing");lines<<QString("Field %1 | %2 | %3 | validity=%4 | value=%5").arg(f["name"].toString(),f["type"].toString(),f["unit"].toString(),validity,validity=="valid"?"[redacted]":"--");}
 if(row.status.payload.isEmpty())lines<<"RECOVERY_REQUIRED: no committed payload; no measurement restart.";
 const auto text=lines.join('\n');if(detail_->toPlainText()!=text)detail_->setPlainText(text);retry_->setEnabled(endpoint_ && endpoint_->diagnosticStatus()["canRetry"].toBool() && row.status.dispatchable() && !row.status.payload.isEmpty());
}
void DeliveryDiagnosticsWidget::retry(){const int n=table_->currentRow();if(!endpoint_ || n<0 || n>=rows_.size())return;const auto request=rows_[n].request;
 try{endpoint_->retryDelivery(request);refresh();message_->setText("Same durable payload sent; waiting for ACK.");}catch(...){refresh();message_->setText("Retry rejected: check connected peer, namespace, idle state and committed pending evidence. Secret details redacted.");}}
}
