#include "CalibrationWizardSession.hpp"
#include "ChessboardService.hpp"
#include "CircleGridService.hpp"
#include "domain/EdgePointConfig.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
double length(QPointF p){return std::hypot(p.x(),p.y());}
}
QString CalibrationWizardSession::configurationId(const QJsonObject& config){
    require(!config.isEmpty(),"Acquisition configuration is required");
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(config).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
}
void CalibrationWizardSession::cancel(){if(cancel_)cancel_->store(true);ticket_.clear();cancel_.reset();}
void CalibrationWizardSession::invalidateSelection(){first_.clear();second_.clear();model_.reset();}
void CalibrationWizardSession::clear(){cancel();frame_.reset();points_.clear();scans_.clear();invalidateSelection();knownMm_=0;modelId_.clear();plate_.reset();plateConfirmed_=false;}
void CalibrationWizardSession::useFrame(ImageFrame f,MeasurementSetup s,QString origin,std::optional<ExperimentalCalibrationEvidence> experimental){
    // Failed replacement must not leave the previous draft available as current.
    clear();s.validate();const auto& m=f.metadata();
    require(f.size()==s.resolution && f.processing().isEmpty() && m.coordinateSystemId==s.source.id &&
        !m.sourceId.isEmpty() && !m.frameId.isEmpty() && !m.cycleId.isEmpty() && m.completedAtUtc.isValid(),"Raw frame/setup provenance mismatch");
    require(origin=="acquired" || origin=="setup_file" || origin=="trial_file","Invalid image origin");
    require(origin!="acquired" || !m.sourceId.startsWith("calibration-test:"),"Imported test cannot become acquired");
    require(experimental.has_value()==!m.experimentalAcquisition.isEmpty(),"Experimental frame evidence cannot be removed");
    if(experimental){experimental->verify(f);require((origin=="acquired" && experimental->origin==SourceOrigin::CameraAcquired) || (origin=="setup_file" && experimental->origin==SourceOrigin::SetupFile),"Experimental replay/live origin mismatch");origin="experimental_"+origin;}
    frame_=f;setup_=s;image_={origin,m.sourceId,m.frameId,QString::fromLatin1(f.contentId().toHex()),experimental};modelId_=id();
}
void CalibrationWizardSession::importFile(QString path,MeasurementSetup s,bool declared){
    clear();auto image=ChessboardService::importImage(path);useFrame(image.frame,s,declared ? "setup_file":"trial_file");
}
WizardScanJob CalibrationWizardSession::begin(QString scanId,ToolRoi roi,QJsonObject p,std::optional<EdgePairOptions> pair){
    require(frame_.has_value(),"Choose a raw frame first");
    require(!scanId.trimmed().isEmpty() && scanId.size()<=128,"Invalid scan identity");
    // Retire the old scan before validation/worker errors; never keep stale endpoints.
    cancel();removeScan(scanId);model_.reset();
    require(scans_.size()<128,"Wizard scan limit exceeded");roi.validate();
    require(roi.frame==setup_.source && std::holds_alternative<ScanSegment>(roi.geometry) && roi.inside(frame_->size()),"Raw scan strip outside frame/setup");
    if(!pair){p["scanRoi"]=roi.toJson();validateEdgePoint(p);}
    ticket_=id();cancel_=std::make_shared<std::atomic_bool>(false);
    WizardScanJob j;j.frame_=*frame_;j.roi_=roi;j.parameters_=p;j.pair_=pair;j.scanId_=scanId;j.ticket_=ticket_;j.cancel_=cancel_;return j;
}
WizardScanJob CalibrationWizardSession::scanEdge(QString s,ToolRoi r,QJsonObject p){return begin(s,r,std::move(p),{});}
WizardScanJob CalibrationWizardSession::scanPair(QString s,ToolRoi r,EdgePairOptions o){
    QJsonObject p{{"method","caliper/1"},{"stepPx",o.profile.stepPx},{"widthStepPx",o.profile.widthStepPx},{"sigmaPx",o.profile.sigmaPx},
        {"polarity",int(o.polarity)},{"selection",int(o.selection)},{"minContrast",o.minContrast},{"minWidthPx",o.minWidthPx},{"maxWidthPx",o.maxWidthPx},{"subpixel",o.subpixel}};
    return begin(s,r,p,o);
}
WizardScanResult CalibrationWizardSession::run(const WizardScanJob& j){
    WizardScanResult r;r.ticket_=j.ticket_;r.scanId_=j.scanId_;r.roi_=j.roi_;r.parameters_=j.parameters_;
    try{
        require(!j.cancel_->load(),"Scan cancelled");
        if(j.pair_){
            const auto found=EdgePairService::measure(j.frame_,j.roi_,*j.pair_,EdgePairMode::PixelDebug,nullptr,j.cancel_);
            require(found.state==ExecutionState::Done,found.error.toUtf8().constData());
            // Full candidate diagnostics belong to the draft, not recipe/terminal data.
            const auto all=EdgePairFinder::find(j.frame_,j.roi_,*j.pair_,j.cancel_);
            for(const auto& c:all.edges.candidates)r.candidates_.append({c.point,c.refined});
            for(int i=0;i<all.candidates.size();++i){const auto& p=all.candidates[i];if(!p.accepted)continue;
                if(all.selected==i)r.selected_=r.pairs_.size();r.pairs_.append({p.firstEdge,p.secondEdge});}
            require(bool(r.selected_)==found.found,"Caliper diagnostic mismatch");
        }else{
            ToolDefinition tool;tool.id=j.scanId_;tool.kind="edge_point";tool.name="Calibration edge";tool.parameters=j.parameters_;
            const auto& m=j.frame_.metadata();const auto result=EdgePointService::execute(tool,{{"image",{{"image",PortType::Image,"",""},j.frame_,true,m.frameId,m.cycleId,m}}},{},m.frameId,m.cycleId,j.cancel_);
            require(result.state==ExecutionState::Done && result.edgeDiagnostics.has_value(),"Edge detection failed");
            for(const auto& c:result.edgeDiagnostics->candidates)if(c.accepted){if(c.selected)r.selected_=r.candidates_.size();r.candidates_.append({c.point,c.refinement=="parabolic_peak"});}
            // The public refined flag is the actual algorithm result for the selected point.
            if(r.selected_)r.candidates_[*r.selected_].second=std::get<bool>(result.outputs["refined"].data);
        }
        require(!j.cancel_->load(),"Scan cancelled");
    }catch(const std::exception& e){r.error_=QString::fromUtf8(e.what());r.candidates_.clear();r.pairs_.clear();r.selected_.reset();}
    return r;
}
bool CalibrationWizardSession::accept(const WizardScanResult& r){
    if(ticket_.isEmpty() || r.ticket_!=ticket_ || !cancel_ || cancel_->load())return false;
    cancel();scans_.insert(r.scanId_,r);if(r.selected_)populate(r,*r.selected_);return true;
}
void CalibrationWizardSession::removeScan(QString s){
    cancel();for(int i=points_.size()-1;i>=0;--i)if(points_[i].scanId==s){if(points_[i].id==first_ || points_[i].id==second_)invalidateSelection();points_.removeAt(i);}
    scans_.remove(s);model_.reset();
}
void CalibrationWizardSession::populate(const WizardScanResult& r,int selected){
    const bool pair=!r.pairs_.isEmpty();require(selected>=0 && selected<(pair ? r.pairs_.size():r.candidates_.size()),"Missing actual candidate");
    for(int i=points_.size()-1;i>=0;--i)if(points_[i].scanId==r.scanId_){if(first_==points_[i].id || second_==points_[i].id)invalidateSelection();points_.removeAt(i);}
    const auto pairId=pair ? id():QString{};
    QVector<int> indices=pair ? QVector<int>{r.pairs_[selected].first,r.pairs_[selected].second}:QVector<int>{selected};
    QStringList added;for(int i:indices){const auto& p=r.candidates_[i];const auto pid=id();added.append(pid);
        points_.append({pid,QString("P%1").arg(points_.size()+1),r.scanId_,r.ticket_,pairId,p.first,image_,r.roi_,r.parameters_,p.second});}
    model_.reset();if(pair)selectPair(added[0],added[1]);
}
void CalibrationWizardSession::chooseCandidate(const QString& s,int i){cancel();const auto it=scans_.constFind(s);require(it!=scans_.cend(),"Unknown scan");populate(it.value(),i);}
const WizardPoint& CalibrationWizardSession::point(const QString& s) const{for(const auto& p:points_)if(p.id==s)return p;throw std::runtime_error("Missing detected point");}
void CalibrationWizardSession::renamePoint(const QString& s,const QString& name){require(!name.trimmed().isEmpty() && name.size()<=128,"Invalid point name");point(s);for(auto& p:points_)if(p.id==s)p.name=name;}
void CalibrationWizardSession::selectPair(const QString& a,const QString& b){
    cancel();invalidateSelection();const auto& p=point(a);const auto& q=point(b);
    require(a!=b && p.image==image_ && q.image==image_ && length(p.pixel-q.pixel)>1e-6,"Select two distinct current detected points");first_=a;second_=b;
}
void CalibrationWizardSession::setKnownMillimetres(double mm){cancel();model_.reset();knownMm_=0;require(std::isfinite(mm) && mm>0 && mm<=1000000,"Positive finite real distance required");knownMm_=mm;}
std::optional<double> CalibrationWizardSession::distancePixels() const{if(first_.isEmpty() || second_.isEmpty())return {};return length(point(first_).pixel-point(second_).pixel);}
CalibrationModelRecord CalibrationWizardSession::calculate(const CalibrationControl& control){
    model_.reset();require(!busy() && frame_ && knownMm_>0 && distancePixels().has_value(),"Current pair and real millimetres required");
    CalibrationDependency d{modelId_,1,"pending",image_.sourceToolId,setup_.lensId,setup_.resolution,setup_.planeId,setup_.source,setup_.target};
    CalibrationSample sample{"selected-pair",{point(first_).pixel,setup_.source},{point(second_).pixel,setup_.source},knownMm_};
    model_=CalibrationModelRecord::create(MeasurementService{}.teach(d,sample),setup_,image_,control);return *model_;
}
WizardPlateJob CalibrationWizardSession::scanPlate(CalibrationGridTarget t){
    cancel();plate_.reset();plateConfirmed_=false;model_.reset();t.validate();require(frame_.has_value(),"Choose a raw frame first");
    ticket_=id();cancel_=std::make_shared<std::atomic_bool>(false);WizardPlateJob j;j.frame_=*frame_;j.target_=t;j.ticket_=ticket_;j.cancel_=cancel_;return j;
}
WizardPlateResult CalibrationWizardSession::run(const WizardPlateJob& j){
    WizardPlateResult r;r.ticket_=j.ticket_;
    try{
        require(!j.cancel_->load(),"Plate detection cancelled");
        r.observation_=j.target_.pattern==CalibrationGridPattern::Chessboard ? ChessboardService::detect(j.frame_,j.target_):CircleGridService::detect(j.frame_,j.target_,j.cancel_);
        require(!j.cancel_->load(),"Plate detection cancelled");
        // Count/order/convexity/fit gates before offering observations to confirm.
        PlaneMappingService::fit(*r.observation_);
    }catch(const std::exception& e){r.error_=QString::fromUtf8(e.what());r.observation_.reset();}return r;
}
bool CalibrationWizardSession::accept(const WizardPlateResult& r){
    if(ticket_.isEmpty() || r.ticket_!=ticket_ || !cancel_ || cancel_->load())return false;
    cancel();plate_=r.observation_;plateConfirmed_=false;model_.reset();return true;
}
void CalibrationWizardSession::confirmPlate(){require(!busy() && plate_.has_value(),"Current complete grid required");plateConfirmed_=true;}
CalibrationModelRecord CalibrationWizardSession::calculatePlate(const CalibrationControl& control){
    model_.reset();require(!busy() && frame_ && plate_ && plateConfirmed_,"Confirm current grid points before calculating");
    require(plate_->source.frameId==image_.frameId && plate_->pixelHash==image_.pixelHash,"Plate/frame raster mismatch");
    require(setup_.target.axes==AxisConvention::XRightYDown,"Plate target-local axes are columns-right/rows-down");
    CalibrationDependency d{modelId_,1,"pending",image_.sourceToolId,setup_.lensId,setup_.resolution,setup_.planeId,setup_.source,setup_.target};
    model_=CalibrationModelRecord::create(MeasurementService{}.teachHomography(d,PlaneMappingService::fit(*plate_)),setup_,image_,control);return *model_;
}
}
