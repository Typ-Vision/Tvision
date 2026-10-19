#include "MetrologyService.hpp"
#include "vision/metrology/ScanLayout.hpp"
#include "EdgePairService.hpp"
#include "domain/ToolRegistry.hpp"
#include "vision/metrology/LineFitter.hpp"
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
namespace visionflow {
QStringList MetrologyService::configurationWarnings(const QString& kind,const QJsonObject& p) {
    QStringList warnings;
    if(kind!="line_fit" && kind!="circle_fit")return warnings;
    const int count=p["scanCount"].toInt(),minimum=std::max(p["minPoints"].toInt(),p["minFoundPoints"].toInt());
    if(count<minimum)warnings<<QString("MUI_COUNT: %1 tarama, gerekli en az %2 nokta için yetersiz.").arg(count).arg(minimum);
    if(p["scanRoi"].toObject().isEmpty())return warnings;
    try {
        const auto roi=ToolRoi::fromJson(p["scanRoi"].toObject());
        const bool tracked=p.contains("poseTracking") || p["referenceBinding"].toObject()["mapping"]=="world-reference-v1";
        double length=0,unit=1;
        if(kind=="circle_fit") {
            const auto arc=std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);
            length=arc.ring.outerRadius-arc.ring.innerRadius;unit=p["unitsPerSample"].toDouble();
            const double sweep=std::abs(arc.sweepDegrees),spacing=count>1 ? sweep/(sweep==360 ? count:count-1):sweep;
            // Current world angles can differ from nominal ROI angles. Do not
            // claim a contradiction in the transformed metric plane.
            if(!tracked && spacing>p["maxGapDegrees"].toDouble())warnings<<QString("MUI_GAP: Nominal tarama aralığı %1°, kontur boşluğu sınırı %2°. Tarama sayısı veya kalite sınırını inceleyin.").arg(spacing,0,'g',6).arg(p["maxGapDegrees"].toDouble());
            const double span=sweep==360 ? sweep-spacing:sweep;
            if(!tracked && span<p["minAngularSpan"].toDouble())warnings<<QString("MUI_SPAN: Nominal yay desteği en çok %1°, fit için %2° isteniyor. Daha geniş yay seçin.").arg(span,0,'g',6).arg(p["minAngularSpan"].toDouble());
        } else {
            const auto box=std::get<RotatedRectangle>(roi.geometry);length=box.size.height();
            if(tracked){const auto binding=p.contains("referenceBinding") ? p["referenceBinding"].toObject():p["poseTracking"].toObject();unit=binding["mmPerSample"].toDouble();}
            const double width=p["scanWidthPx"].toDouble()*unit;
            if(width>=box.size.width())warnings<<"MUI_STRIP: Ortalama şerit kalınlığı ROI destek genişliğinden küçük olmalı.";
            if(!tracked && box.size.width()-width<p["minSpanPx"].toDouble())warnings<<QString("MUI_SPAN: Nominal çizgi desteği %1 px; minimum %2 px. ROI genişliğini ve destek kalitesini inceleyin.").arg(box.size.width()-width,0,'g',6).arg(p["minSpanPx"].toDouble());
        }
        const double step=p["stepPx"].toDouble(),sigma=p["sigmaPx"].toDouble();
        if(unit>0 && step>0 && length>0) {
            const double samples=std::ceil(length/unit/step)+1,actual=length/unit/(samples-1),radius=sigma==0 ? 0:std::ceil(3*sigma/actual);
            if(samples<2*radius+4)warnings<<QString("MUI_SUPPORT: Arama uzunluğu %1 %2; %3 örnek var, filtre/tepe desteği için %4 gerekli. ROI'yi kenarın iki tarafında pay bırakarak genişletin.").arg(length,0,'g',6).arg(roi.frame.unit==LengthUnit::Millimetre ? "mm":"px").arg(samples).arg(2*radius+4);
        }
    }catch(const std::exception&){warnings<<"MUI_ROI: Geçerli ROI öğretin.";}
    return warnings;
}
QVector<QLineF> MetrologyService::scanGuides(const QString& kind,const QJsonObject& p,const ToolRoi& roi) {
    roi.validate();QVector<ScanSegment> scans;
    if(kind=="circle_fit") {
        const auto arc=std::holds_alternative<Annulus>(roi.geometry) ? ArcRoi{std::get<Annulus>(roi.geometry),0,360}:std::get<ArcRoi>(roi.geometry);
        scans=ScanLayout::radial(arc,p["scanCount"].toInt(),p["scanWidth"].toDouble(),p["direction"]=="inward");
    } else if(kind=="line_fit") {
        double unit=1;
        if(roi.frame.unit==LengthUnit::Millimetre){const auto binding=p.contains("referenceBinding") ? p["referenceBinding"].toObject():p["poseTracking"].toObject();unit=binding["mmPerSample"].toDouble();
            if(!std::isfinite(unit) || unit<=0)throw std::runtime_error("World guide sampling unit unavailable");}
        scans=ScanLayout::line(std::get<RotatedRectangle>(roi.geometry),p["scanCount"].toInt(),p["scanWidthPx"].toDouble()*unit);
    }
    else if(kind=="edge_point" || kind=="edge_pair")scans.append(std::get<ScanSegment>(roi.geometry));
    QVector<QLineF> lines;for(const auto& s:scans)lines.append(QLineF(s.start,s.end));return lines;
}
ToolResult MetrologyService::execute(const ToolDefinition& t,const QMap<QString,PortValue>& inputs,const QString&,const QString& frameId,const QString& cycleId,const std::shared_ptr<std::atomic_bool>& cancel,const MeasurementExecutionContext& context){
    ToolRegistry::validate(t);const auto p=t.parameters;const bool world=p["mode"]=="calibrated";
    if(p.contains("poseTracking"))throw std::runtime_error("Pose tracking requires DAG resolution");
    auto stop=[&]{if(cancel && cancel->load())throw std::runtime_error("Metrology cancelled");};stop();
    const auto image=std::get<ImageFrame>(inputs.value("image").data);const auto& m=image.metadata();
    if(p["scanRoi"].toObject().isEmpty())throw std::runtime_error("Metrology requires a taught ROI");
    if(world && (!context.calibration || !context.current))throw std::runtime_error(context.error.isEmpty() ? "Metrology requires current calibration":context.error.toStdString());
    const auto roi=ToolRoi::fromJson(p["scanRoi"].toObject());
    ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;
    auto put=[&](const QString& id,PortData data,bool valid=true,const QString& reason=QString{}){for(const auto& port:outputPorts(t.kind))if(port.id==id){PortValue v{port,std::move(data),valid,frameId,cycleId,m};v.method=t.kind=="edge_pair" ? "profile-pair":"robust-line";v.methodVersion=1;v.diagnostic=!world;if(!valid)v.invalidReason=reason.isEmpty() ? "NOT_FOUND":reason;r.outputs[id]=std::move(v);}};
    const ProfileOptions profile{p["stepPx"].toDouble(),p["widthStepPx"].toDouble(),p["sigmaPx"].toDouble()};
    const auto selection=p["selection"]=="first" ? EdgeSelection::First:p["selection"]=="last" ? EdgeSelection::Last:EdgeSelection::Strongest;
    if(t.kind=="edge_pair"){
        EdgePairOptions o;o.profile=profile;o.selection=selection;o.minContrast=p["minContrast"].toDouble();o.subpixel=p["subpixel"].toBool();o.minWidthPx=p["minWidthPx"].toDouble();o.maxWidthPx=p["maxWidthPx"].toDouble();o.polarity=p["polarity"]=="bright" ? StripePolarity::Bright:p["polarity"]=="dark" ? StripePolarity::Dark:StripePolarity::Any;
        std::optional<EdgePairCalibration> calibration;if(world)calibration=EdgePairCalibration{*context.calibration,*context.current};
        const auto pair=EdgePairService::measure(image,roi,o,world ? EdgePairMode::Calibrated:EdgePairMode::PixelDebug,calibration ? &*calibration:nullptr,cancel);
        if(pair.state!=ExecutionState::Done)throw std::runtime_error(pair.error.toStdString());
        put("found",pair.found);put("first",pair.first.value_or(QPointF{}),pair.found);put("second",pair.second.value_or(QPointF{}),pair.found);put("centre",pair.centre.value_or(QPointF{}),pair.found);put("widthPx",pair.widthPx.value_or(0),pair.found);
        put("widthMm",pair.widthMm.value_or(0),pair.widthMm.has_value(),world ? "NOT_FOUND":"PIXEL_MODE");
        if(pair.found){const double width=world ? *pair.widthMm:*pair.widthPx;put("accepted",width>=p["lower"].toDouble() && width<=p["upper"].toDouble());r.points={*pair.first,*pair.second,*pair.centre};r.lines={QLineF(*pair.first,*pair.second)};r.message=QString("Width %1 %2 • accepted=%3").arg(width,0,'f',6).arg(world ? "mm":"px").arg(std::get<bool>(r.outputs["accepted"].data) ? "true":"false");r.calibration=pair.calibration;}
        else {put("accepted",false,false);r.message="NOT_FOUND";}
    }else {
        LineScanOptions o;o.scanCount=p["scanCount"].toInt();o.scanWidthPx=p["scanWidthPx"].toDouble();o.edge.profile=profile;o.edge.selection=selection;o.edge.minContrast=p["minContrast"].toDouble();o.edge.subpixel=p["subpixel"].toBool();o.edge.polarity=p["polarity"]=="rising" ? EdgePolarity::Rising:p["polarity"]=="falling" ? EdgePolarity::Falling:EdgePolarity::Any;
        o.fit.minPoints=p["minPoints"].toInt();o.fit.minCoverage=p["minCoverage"].toDouble();o.fit.minSpanCoverage=p["minSpanCoverage"].toDouble();o.fit.ransac=p["ransac"].toBool();o.fit.minSpan=p["minSpanPx"].toDouble();o.fit.inlierDistance=p["inlierDistancePx"].toDouble();o.fit.maxRms=p["maxRmsPx"].toDouble();o.fit.maxResidual=p["maxResidualPx"].toDouble();
        const auto raw=LineFitter::find(image,roi,o,cancel).line;auto fitted=raw;
        const CoordinateFrame pixel{m.coordinateSystemId,LengthUnit::Pixel,AxisConvention::XRightYDown};
        PointSet observations{{},pixel};for(const auto& s:raw.scans)if(s.point)observations.points.append(*s.point);put("observationsPx",observations);
        put("linePx",Line2D{raw.supportStart,raw.supportEnd,pixel});put("originPx",raw.origin);put("tangentPx",raw.tangent);put("anglePx",raw.angleDegrees);put("spanPx",raw.span);put("rmsPx",raw.rms);put("maxResidualPx",raw.maxResidual);
        if(world){
            // Reuse the full validated current/source/frame/hull guard for every
            // observed point before mapping. No nominal-scale angle conversion.
            auto map=[&](QPointF point){ToolResult v;v.state=ExecutionState::Done;v.outputs["p"]={{"p",PortType::Point,"px","image"},point,true,frameId,cycleId,m};const auto check=MeasurementService{}.distance(v,"p",v,"p",*context.calibration,*context.current,image.size(),0,std::numeric_limits<double>::max());if(check.state!=ExecutionState::Done)throw std::runtime_error(check.error.toStdString());const auto& c=*context.calibration;if(c.mapping())return PlaneMappingService::map(*c.mapping(),point);auto q=point*c.millimetresPerPixel();if(c.dependency().target.axes==AxisConvention::XRightYUp)q.setY(-q.y());return q;};
            QVector<LineScanPoint> scans=raw.scans;for(auto& s:scans){stop();if(s.point)s.point=map(*s.point);}
            const auto a=map(raw.supportStart),b=map(raw.supportEnd);auto fit=o.fit;fit.minSpan=p["minSpanMm"].toDouble();fit.inlierDistance=p["inlierDistanceMm"].toDouble();fit.maxRms=p["maxRmsMm"].toDouble();fit.maxResidual=p["maxResidualMm"].toDouble();fitted=LineFitter::fit(scans,b-a,fit,cancel);
            put("lineMm",Line2D{fitted.supportStart,fitted.supportEnd,context.calibration->dependency().target});put("originMm",fitted.origin);put("tangentMm",fitted.tangent);put("angleMm",fitted.angleDegrees);put("spanMm",fitted.span);put("rmsMm",fitted.rms);put("maxResidualMm",fitted.maxResidual);r.calibration=context.calibration->dependency();
        }else {put("lineMm",Line2D{},false,"PIXEL_MODE");for(auto k:{"originMm","tangentMm"})put(k,QPointF{},false,"PIXEL_MODE");for(auto k:{"angleMm","spanMm","rmsMm","maxResidualMm"})put(k,0.,false,"PIXEL_MODE");}
        put("found",true);put("accepted",fitted.span>=p["lower"].toDouble() && fitted.span<=p["upper"].toDouble() && fitted.angleDegrees>=p["angleLower"].toDouble() && fitted.angleDegrees<=p["angleUpper"].toDouble());
        put("coverage",fitted.coverage);put("spanCoverage",fitted.spanCoverage);put("usedCount",double(fitted.usedCount));put("outlierCount",double(fitted.rejectedCount));put("missingCount",double(fitted.missingCount));
        // Persist scan membership and residuals, including missing slots, with a
        // bounded typed explanation. Raw overlay uses raw observations only.
        InspectionResults diagnostics;for(int i=0;i<fitted.scans.size();++i){const auto& s=fitted.scans[i];diagnostics.items.append({QString("scan %1").arg(i),s.point ? QString("%1,%2; residual=%3 %4; %5").arg(s.point->x(),0,'g',17).arg(s.point->y(),0,'g',17).arg(*s.signedResidual,0,'g',17).arg(world ? "mm":"px",s.used ? "used":"outlier"):"not_found",s.used});}
        put("scans",diagnostics);r.lines={QLineF(raw.supportStart,raw.supportEnd)};for(int i=0;i<raw.scans.size();++i)if(raw.scans[i].point){if(fitted.scans[i].used)r.points.append(*raw.scans[i].point);else r.rejectedPoints.append(*raw.scans[i].point);}
        r.message=QString("Support %1 %2 • angle %3° • RMS %4 • used/outlier/missing %5/%6/%7 • accepted=%8").arg(fitted.span,0,'f',6).arg(world ? "mm":"px").arg(fitted.angleDegrees,0,'f',6).arg(fitted.rms,0,'f',6).arg(fitted.usedCount).arg(fitted.rejectedCount).arg(fitted.missingCount).arg(std::get<bool>(r.outputs["accepted"].data) ? "true":"false");
    }
    stop();return r;
}
}
