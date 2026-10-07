#pragma once
#include "ImageFileService.hpp"
#include <QJsonObject>
#include <QRect>
namespace visionflow {
struct MatchOutcome { QRect rectangle; double score{}; double milliseconds{}; bool accepted{}; };
struct MatchParameters {QRect modelRoi,searchRoi;QString modelPng;double minScore=-1;};
class MatchService {
public:
    QString teach(const ImageFrame& frame,const QRect& roi) const;
    MatchOutcome run(const ImageFrame& frame,const QJsonObject& parameters) const;
    MatchOutcome run(const ImageFrame& frame,const MatchParameters& parameters) const;
    static MatchParameters parameters(const QJsonObject& json);
    static QSize validateModel(const MatchParameters& parameters);
    static QRect rectangle(const QJsonObject& object);
    static QJsonObject rectangle(const QRect& roi);
private:
    cv::Mat gray(const ImageFrame& frame) const;
};
}
