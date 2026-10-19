#pragma once
#include <QString>
#include <QVector>
#include <stdexcept>

namespace visionflow {
// Fixed-size evidence only: no profiles or unbounded candidate arrays.
struct ScanRejections {
    int polarity=0,contrast=0,border=0,noCandidate=0;
    ScanRejections& operator+=(const ScanRejections& b) {
        polarity+=b.polarity;contrast+=b.contrast;border+=b.border;noCandidate+=b.noCandidate;return *this;
    }
};
struct FitDiagnostic {
    QString code,stage;
    int requested=0,found=0,required=0,used=0;
    ScanRejections rejections;
};
class FitDiagnosticError:public std::runtime_error {
public:
    FitDiagnostic evidence;
    FitDiagnosticError(const char* message,FitDiagnostic d):std::runtime_error(message),evidence(std::move(d)){}
};
template<class Scan> void requireFitSupport(bool ok,const char* message,const char* code,const char* stage,
                                          const QVector<Scan>& scans,int required,int used=0) {
    if(ok)return;
    FitDiagnostic d;d.code=code;d.stage=stage;d.requested=scans.size();d.required=required;d.used=used;
    for(const auto& s:scans){if(s.point)++d.found;d.rejections+=s.rejections;}
    throw FitDiagnosticError(message,std::move(d));
}
}
