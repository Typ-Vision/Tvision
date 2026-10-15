#include "CalibrationCapture.hpp"
#include "AcquisitionService.hpp"
#include "WebcamAdapter.hpp"
#include "domain/ToolRegistry.hpp"
#include <QDir>
#include <QUuid>
#include <stdexcept>
namespace visionflow {
ImageFrame CalibrationCapture::acquire(const ToolDefinition& tool,const QString& directory,const std::shared_ptr<std::atomic_bool>& cancel){
    ToolRegistry::validate(tool);
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    AcquisitionRequest request{tool.id,id,id,TriggerMode::Software,5000,cancel};
    std::unique_ptr<AcquisitionAdapter> adapter;
    if(tool.kind=="image_file")adapter=std::make_unique<ImageFileAdapter>(QDir(directory).absoluteFilePath(tool.parameters["path"].toString()));
    else if(tool.kind=="webcam")adapter=std::make_unique<WebcamAdapter>(WebcamSettings{tool.parameters["deviceIndex"].toInt(),tool.parameters["backend"].toString()});
    else throw std::runtime_error("Yalnız çalışan Image File veya Webcam kaynağı kullanılabilir.");
    auto result=AcquisitionService{}.acquire(*adapter,request);
    if(result.state!=AcquisitionState::Done || !result.frame)throw std::runtime_error(result.error.toUtf8().constData());
    auto image=result.frame->image;auto metadata=image.metadata();metadata.coordinateSystemId="image";return image.withMetadata(metadata);
}
QString CalibrationCapture::configuration(const ToolDefinition& t,const QString& directory){
    ToolRegistry::validate(t);
    QJsonObject config;
    if(t.kind=="image_file")config={{"path",QDir::cleanPath(QDir(directory).absoluteFilePath(t.parameters["path"].toString()))}};
    else if(t.kind=="webcam")config={{"deviceIndex",t.parameters["deviceIndex"]},{"backend",t.parameters["backend"]}};
    else throw std::runtime_error("Ölçüm kurulumu için Image File veya Webcam kaynağı seçin.");
    return CalibrationWizardSession::configurationId(config);
}
MeasurementSetup CalibrationCapture::setup(const ToolDefinition& t,const QString& directory,const ImageFrame& f,const QString& installation,const QString& lens,const QString& plane){
    MeasurementSetup s{installation.trimmed(),t.kind,configuration(t,directory),lens.trimmed(),plane.trimmed(),f.size(),
        {f.metadata().coordinateSystemId},{plane.trimmed()+"/mm",LengthUnit::Millimetre,AxisConvention::XRightYDown}};
    s.validate();return s;
}
}
