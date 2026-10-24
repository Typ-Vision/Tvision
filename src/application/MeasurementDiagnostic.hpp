#pragma once
#include "ExecutionService.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QStringList>
#include <cmath>

namespace visionflow {
// Optional, versioned, bounded explanatory evidence. Never a result port or
// completion authority. The enclosing tool/cycle and acquired frame bind it.
inline QJsonObject encodeMeasurementDiagnostic(const FitDiagnostic& d) {
    return {{"toolId",d.toolId},{"sourceId",d.sourceId},{"frameId",d.frameId},{"cycleId",d.cycleId},{"evaluationId",d.evaluationId},{"schemaVersion",1},{"code",d.code},{"stage",d.stage},
        {"requested",d.requested},{"found",d.found},{"required",d.required},{"used",d.used},
        {"polarity",d.rejections.polarity},{"contrast",d.rejections.contrast},
        {"border",d.rejections.border},{"noCandidate",d.rejections.noCandidate}};
}
inline FitDiagnostic decodeMeasurementDiagnostic(const QJsonObject& o) {
    auto require=[](bool ok){if(!ok)throw std::runtime_error("Invalid measurement diagnostic");};
    const QStringList fields{"toolId","sourceId","frameId","cycleId","evaluationId","schemaVersion","code","stage","requested","found","required","used","polarity","contrast","border","noCandidate"};
    require(o.size()==fields.size());for(auto i=o.begin();i!=o.end();++i)require(fields.contains(i.key()));
    require(o["schemaVersion"]==1 && o["code"].isString() && !o["code"].toString().isEmpty() && o["code"].toString().size()<=64);
    require(QStringList{"edge_search","pair_search","fit_consensus","fit_support","fit_quality","geometry_refit","contour_quality","sampling_support","work_budget"}.contains(o["stage"].toString()));
    auto number=[&](const char* key){auto v=o[key];require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble()>=0 && v.toDouble()<=16777216 && std::floor(v.toDouble())==v.toDouble());return v.toInt();};
    FitDiagnostic d;
    for(const auto& key:QStringList{"toolId","sourceId","frameId","cycleId","evaluationId"})require(o[key].isString() && o[key].toString().size()<=256);
    d.toolId=o["toolId"].toString();d.sourceId=o["sourceId"].toString();d.frameId=o["frameId"].toString();d.cycleId=o["cycleId"].toString();d.evaluationId=o["evaluationId"].toString();d.code=o["code"].toString();d.stage=o["stage"].toString();d.requested=number("requested");d.found=number("found");d.required=number("required");d.used=number("used");
    d.rejections={number("polarity"),number("contrast"),number("border"),number("noCandidate")};
    require(d.requested<=4096 && d.required<=4096 && d.found<=d.requested && d.used<=d.found && d.rejections.noCandidate<=d.requested);return d;
}
inline QString presentMeasurementDiagnostic(const FitDiagnostic& d) {
    if(d.stage=="sampling_support")return d.code+" • Tarama yapılmadı. Filtre/tepe desteği için ROI arama uzunluğu yetersiz; ROI'yi kenarın iki tarafında pay bırakarak genişletin.";
    if(d.stage=="work_budget")return d.code+QString(" • Tarama yapılmadı. %1 tarama ile görüntü/örnekleme iş bütçesi aşılıyor. Çözünürlüğü, tarama sayısını ve şerit kalınlığını inceleyin.").arg(d.requested);
    QString text=QString("%1 • %2 taramadan %3 bulundu; %4 kullanıldı; gerekli %5.").arg(d.code).arg(d.requested).arg(d.found).arg(d.used).arg(d.required);
    if(d.stage=="geometry_refit")text.replace("taramadan","girdi gözleminden");
    if(d.stage=="contour_quality")text+=" Daire bulundu; kontur değerlendirmesi yapılamadı. Eksik bölgeleri, gerçek boşluk açısını ve tarama sayısını inceleyin.";
    else if(d.stage=="edge_search") {
        // Rejections are candidate counts, not mutually exclusive scan counts.
        if(d.rejections.border)text+=" Kenar arama sınırında olabilir. ROI'yi kenarın iki tarafında pay bırakarak genişletin.";
        if(d.rejections.polarity)text+=" Ok yönüne göre kenar geçişini kontrol edin.";
        if(d.rejections.contrast)text+=" Eşik altında adaylar var. Aday gücünü inceleyip kenar eşiğini kontrol edin.";
        if(!d.rejections.border && !d.rejections.polarity && !d.rejections.contrast)text+=" ROI'nin kenarı kestiğini ve ok yönünü kontrol edin.";
    }else if(d.stage=="pair_search")text+=" Komşu kenarları, açık/koyu şerit seçimini ve aranan genişlik aralığını kontrol edin.";
    else if(d.stage=="fit_consensus")text+=" Kenarlar bulundu; aynı geometriye uymuyor. Noktaları inceleyin; doğru konturdalarsa fit'e katılma mesafesini kontrol edin.";
    else if(d.stage=="fit_support")text+=" Geometrik destek yetersiz. ROI, yay açıklığı, tarama dağılımı ve yönünü kontrol edin.";
    else if(d.stage=="fit_quality")text+=" Fit kalite sınırı aşıldı. Yanlış kenar, gürültü ve şekli inceleyin; fit kalite sınırını kontrol edin.";
    else text+=" Girdi geometrisi bulundu; downstream refit başarısız. Girdi noktalarını ve refit kalitesini kontrol edin.";
    return text+QString(" Aday retleri: geçiş=%1, eşik=%2, sınır desteği=%3; adaysız tarama=%4.").arg(d.rejections.polarity).arg(d.rejections.contrast).arg(d.rejections.border).arg(d.rejections.noCandidate);
}
inline void validateMeasurementDiagnostic(const QJsonObject& tool,const QJsonObject& record) {
    if(!tool.contains("measurementDiagnostic"))return; // Legacy records remain readable.
    if(!tool["measurementDiagnostic"].isObject() || record["state"]=="SKIPPED" || tool["state"]=="SKIPPED" || tool["decision"]!="NOT_EVALUATED")throw std::runtime_error("Diagnostic cannot establish a result or survive Stop");
    const auto d=decodeMeasurementDiagnostic(tool["measurementDiagnostic"].toObject());
    if(d.toolId!=tool["toolId"].toString() || d.cycleId!=record["cycleId"].toString() || d.evaluationId!=record["evaluationId"].toString() || d.frameId.isEmpty() || d.sourceId.isEmpty())throw std::runtime_error("Diagnostic identity differs");
    bool frame=false;for(const auto& v:record["frames"].toArray()){const auto f=v.toObject();frame=frame || (f["frameId"]==d.frameId && f["sourceId"]==d.sourceId && f["cycleId"]==d.cycleId);}
    if(!frame)throw std::runtime_error("Diagnostic acquired frame missing");
}
}
