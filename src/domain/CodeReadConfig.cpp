#include "CodeReadConfig.hpp"
#include <QJsonArray>
#include <QSet>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>
namespace visionflow {
QStringList codeFormats(){return {"QRCode","DataMatrix","Code128","Code39","EAN-8","EAN-13","UPC-A","UPC-E"};}
QJsonObject codeReadDefaults(){QJsonArray formats;for(const auto& f:codeFormats())formats.append(f);return {{"formats",formats},{"searchRoi",QJsonObject{}},{"maxCount",32},{"budgetMs",1000},{"rule","none"},{"expected",""},{"minCount",1},{"maxExpected",32},{"selection","all"},{"selectedIndex",0},{"minLength",0},{"maxLength",4096}};}
void validateCodeRead(const QJsonObject& p) {
    auto require=[](bool ok){if(!ok)throw std::runtime_error("Code Read configuration invalid (regex: no groups/alternation/backreferences, at most one quantifier)");};
    const auto defaults=codeReadDefaults();require(p.size()==defaults.size());for(auto it=p.begin();it!=p.end();++it)require(defaults.contains(it.key()));
    auto integer=[&](const char* key,int low,int high){const auto v=p[key];require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>=low && v.toInt()<=high);return v.toInt();};
    integer("maxCount",1,128);integer("budgetMs",1,60000);const int low=integer("minCount",0,128);require(integer("maxExpected",0,128)>=low);integer("selectedIndex",0,127);
    const int min=integer("minLength",0,4096);require(integer("maxLength",0,4096)>=min);
    require(p["formats"].isArray());QSet<QString> seen;for(const auto& f:p["formats"].toArray()){require(f.isString() && codeFormats().contains(f.toString()) && !seen.contains(f.toString()));seen.insert(f.toString());}require(!seen.isEmpty());
    require(p["searchRoi"].isObject());const auto roi=p["searchRoi"].toObject();require(roi.isEmpty() || roi.size()==4);
    if(!roi.isEmpty())for(const auto key:{"x","y","width","height"}){const auto v=roi[key];require(v.isDouble() && v.toDouble()==v.toInt() && v.toInt()>=(QString(key)=="x" || QString(key)=="y" ? 0:1) && v.toInt()<=100000);}
    require(p["rule"].isString() && QStringList{"none","exact","prefix","length","regex"}.contains(p["rule"].toString()));
    require(p["selection"].isString() && QStringList{"all","any","selected"}.contains(p["selection"].toString()));
    require(p["expected"].isString() && p["expected"].toString().size()<=256);
    if(p["rule"]=="regex") {
        const auto pattern=p["expected"].toString();require(!pattern.isEmpty() && QRegularExpression(pattern).isValid());
        bool escaped=false,inClass=false;int quantifiers=0;
        for(const QChar c:pattern) {
            if(escaped){require(!c.isDigit() && c!='g' && c!='k' && c!='Q' && c!='E' && c!='p' && c!='P');escaped=false;continue;}
            if(c=='\\'){escaped=true;continue;}if(c=='[')inClass=true;if(c==']')inClass=false;if(inClass)continue;
            require(c!='(' && c!=')' && c!='|');if(c=='*' || c=='+' || c=='?' || c=='{')require(++quantifiers<=1);
        }
    }
}
}
