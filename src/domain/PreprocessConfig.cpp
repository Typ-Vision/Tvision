#include "PreprocessConfig.hpp"
#include <QJsonArray>
#include <QStringList>
#include <cmath>
#include <stdexcept>
namespace visionflow {
void validatePreprocess(const QJsonObject& p) {
    auto require=[](bool ok){if(!ok)throw std::runtime_error("Preprocess: kanal veya sıralı filtre parametreleri geçersiz.");};
    require(p.size()==2 && p["channel"].isString() && QStringList{"gray","red","green","blue","alpha"}.contains(p["channel"].toString()) && p["steps"].isArray());
    const auto steps=p["steps"].toArray();require(steps.size()<=16);
    for(const auto& value:steps) {
        require(value.isObject());const auto s=value.toObject();const auto op=s["op"].toString();
        require(s.size()==3 && s["op"].isString() && QStringList{"gaussian","median","threshold","adaptive_mean","adaptive_gaussian","erode","dilate","open","close"}.contains(op));
        const auto size=s["size"],v=s["value"];
        require(size.isDouble() && size.toDouble()==size.toInt() && size.toInt()>=3 && size.toInt()<=31 && size.toInt()%2==1);
        require(v.isDouble() && std::isfinite(v.toDouble()));
        if(op=="gaussian")require(v.toDouble()>=0 && v.toDouble()<=100);
        else if(op=="threshold")require(v.toDouble()>=0 && v.toDouble()<=255);
        else if(op.startsWith("adaptive"))require(v.toDouble()>=-255 && v.toDouble()<=255);
        else require(v.toDouble()==0);
    }
}
}
