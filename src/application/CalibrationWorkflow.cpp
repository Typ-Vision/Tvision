#include "CalibrationWorkflow.hpp"
#include <stdexcept>
namespace visionflow {
namespace {void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}}
CalibrationInstallation CalibrationWorkflow::installation(const Recipe& recipe,const ImageFrame& frame,const QString& modelType) {
    const auto model=modelType.isEmpty() ? (recipe.calibrationBinding ? recipe.calibrationBinding->modelType : QString("uniform_planar_scale")) : modelType;
    require(model=="uniform_planar_scale" || model=="raw_planar_homography" || model=="manual_planar_scale","Unsupported planar model");
    const auto setup=recipe.definition.value("planarSetup").toObject();
    require(setup.size()==2 && setup["lensId"].isString() && setup["workingPlaneId"].isString() && !setup["lensId"].toString().trimmed().isEmpty() && !setup["workingPlaneId"].toString().trimmed().isEmpty(),"Reçetenin kayıtlı lens / çalışma düzlemi bilgisi eksik; kamera ve kalibrasyon bağını denetleyin.");
    const auto& m=frame.metadata();require(!frame.size().isEmpty() && !m.sourceId.isEmpty() && !m.frameId.isEmpty() && !m.cycleId.isEmpty() && !m.coordinateSystemId.isEmpty() && m.completedAtUtc.isValid(),"Önce reçetenin kaynağından yeni bir kare alın.");
    int sources=0;for(const auto& t:editorTools(recipe.definition))if(t.enabled && (t.kind=="image_file" || t.kind=="webcam")) {++sources;require(t.id==m.sourceId,"Kare başka reçete/kaynaktan geliyor.");}
    require(sources==1,"Tek enabled edinim kaynağı gerekli.");
    return {m.sourceId,setup["lensId"].toString(),setup["workingPlaneId"].toString(),frame.geometry().rootSize(),{frame.geometry().rootCoordinateSystemId(),LengthUnit::Pixel,AxisConvention::XRightYDown},{setup["workingPlaneId"].toString()+"/mm",LengthUnit::Millimetre,model=="raw_planar_homography" ? AxisConvention::XRightYDown : AxisConvention::XRightYUp}};
}
PlanarCalibration CalibrationWorkflow::teach(const Recipe& recipe,const ImageFrame& frame,const QString& id,int version,const CalibrationSample& sample) {
    require(!id.trimmed().isEmpty() && id==id.trimmed() && id.size()<=128 && version>0 && version<=1000000000,"Kalibrasyon kimliği/sürümü geçersiz.");
    const auto s=installation(recipe,frame,"uniform_planar_scale");
    return MeasurementService{}.teach({id,version,"pending",s.cameraId,s.lensId,s.resolution,s.workingPlaneId,s.source,s.target},sample);
}
MeasurementExecutionContext CalibrationWorkflow::resolve(const Recipe& recipe,const ImageFrame& frame,const QString& path) {
    MeasurementExecutionContext result;result.rawSize=frame.geometry().rootSize();
    try {
        const auto setup=installation(recipe,frame);const CalibrationService catalog(path,[setup](const QString&){return setup;});
        const auto c=catalog.resolve(recipe);result.calibration=std::make_shared<const PlanarCalibration>(c);
        // resolve already compared independently constructed current installation.
        result.current=c.dependency();
    }catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;
}
PlanarCalibration CalibrationWorkflow::manualScale(const Recipe& recipe,const ImageFrame& frame,const QString& id,int version,double pixelsPerMillimetre) {
    require(!id.trimmed().isEmpty() && id==id.trimmed() && id.size()<=128 && version>0 && version<=1000000000,"Kalibrasyon kimliği/sürümü geçersiz.");
    const auto s=installation(recipe,frame,"manual_planar_scale");
    return MeasurementService{}.manualScale({id,version,"pending",s.cameraId,s.lensId,s.resolution,s.workingPlaneId,s.source,s.target},pixelsPerMillimetre,
        {"acquired",frame.metadata().sourceId,frame.metadata().frameId,frame.size()});
}
void CalibrationWorkflow::bind(ProjectService& project,const ImageFrame& frame,const QString& path,const QString& id,const QString& modelType) {
    require(project.activeRecipe()!=nullptr,"Önce reçete açın.");const auto setup=installation(*project.activeRecipe(),frame,modelType);
    CalibrationService(path,[setup](const QString&){return setup;}).bind(project,id);
}
}
