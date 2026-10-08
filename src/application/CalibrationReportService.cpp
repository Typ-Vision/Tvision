#include "CalibrationReportService.hpp"
#include <QFileInfo>
#include <QJsonArray>
#include <QSaveFile>
#include <stdexcept>
namespace visionflow {
namespace {
QString cell(QString value){bool numeric=false;value.toDouble(&numeric);const auto trimmed=value.trimmed();if(!numeric && !trimmed.isEmpty() && QString("=+-@\t\r\n").contains(trimmed.front()))value.prepend('\'');value.replace('"',"\"\"");return '"'+value+'"';}
QString number(double value){return QString::number(value,'g',17);}
}
void CalibrationReportService::saveCsv(const QString& path,const PlanarCalibration& c) {
    if(!QFileInfo(path).isAbsolute() || !c.validated())throw std::runtime_error("Accepted independent evidence and absolute report path required");
    const auto& d=c.dependency();QString csv;
    auto row=[&](const QStringList& values){QStringList escaped;for(auto value:values)escaped.append(cell(value));csv+=escaped.join(',')+"\r\n";};
    row({"VisionFlow calibration evidence report","1"});row({"Acceptance scope","Mathematical holdout only; physical reference/accuracy and production approval not attested"});
    row({"Calibration ID",d.calibrationId,"version",QString::number(d.version),"model",d.modelType,"modelVersion",QString::number(d.modelVersion)});
    row({"contentId",d.contentId});row({"sourceId",d.cameraId,"lens/installation",d.lensId,"plane",d.workingPlaneId});row({"raw resolution",QString::number(d.resolution.width()),QString::number(d.resolution.height()),"input","raw pixel centres","output","mm"});
    row({"holdout RMS mm",number(c.validationRmsMillimetres()),"holdout maximum residual mm",number(c.maxValidationErrorMillimetres())});
    if(c.mapping()){const auto& m=*c.mapping();row({"lens policy","raw assumed negligible; distortion correction not applied"});row({"evidence origin",d.cameraId.startsWith("calibration-test:") ? "offline file test; cannot bind physical camera" : "acquisition; physical identity/reference not independently attested"});row({"fit frame",m.observation().source.frameId,"fit timestamp UTC",m.observation().source.completedAtUtc.toString(Qt::ISODateWithMs)});row({"target ID",m.observation().target.id,"inner columns",QString::number(m.observation().target.columns),"inner rows",QString::number(m.observation().target.rows),"square mm",number(m.observation().target.squareMillimetres)});row({"target origin","corner 1","+X","columns","+Y","rows","orientation","symmetric physical direction not identified"});row({"Fit RMS mm",number(m.fitRmsMillimetres()),"inverse RMS px",number(m.inverseRmsPixels()),"inverse max px",number(m.maxInversePixels())});row({"H direction","raw image -> target-local mm"});const auto h=m.matrix();for(int r=0;r<3;++r)row({number(h[r*3]),number(h[r*3+1]),number(h[r*3+2])});row({"Allowed region","inner-corner convex hull; extrapolation disabled"});for(auto p:m.regionPixels())row({"region corner px",number(p.x()),number(p.y())});}
    else if(c.manualProvenance()) {
        const auto& p=*c.manualProvenance();
        row({"manual px/mm",number(c.pixelsPerMillimetre()),"mm/px",number(c.millimetresPerPixel())});
        row({"evidence origin",p.origin,"sourceId",p.sourceId,"frameId",p.frameId});
        row({"evidence resolution",QString::number(p.resolution.width()),QString::number(p.resolution.height())});
        row({"scale policy","Fixed manual input; holdout does not refit scale"});
    } else row({"legacy mm/px",number(c.millimetresPerPixel())});
    row({"Holdout ID","X1 px","Y1 px","X2 px","Y2 px","Known mm","Measured mm","Absolute residual mm"});
    const auto samples=MeasurementService{}.encode(c)["holdout"].toArray();if(samples.size()!=c.measuredHoldoutMillimetres().size() || samples.size()!=c.residualHoldoutMillimetres().size())throw std::runtime_error("Report evidence mismatch");
    for(int i=0;i<samples.size();++i){const auto s=samples[i].toObject();row({s["id"].toString(),number(s["x1"].toDouble()),number(s["y1"].toDouble()),number(s["x2"].toDouble()),number(s["y2"].toDouble()),number(s["knownMm"].toDouble()),number(c.measuredHoldoutMillimetres()[i]),number(c.residualHoldoutMillimetres()[i])});}
    const auto bytes=csv.toUtf8();QSaveFile file(path);file.setDirectWriteFallback(false);if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())throw std::runtime_error("Cannot atomically save calibration report");
}
}
