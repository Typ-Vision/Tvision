#include "application/BackendCapabilities.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const bool expected=argc==2 && QString(argv[1])=="1";
        const auto c=BackendCapabilities::get("zxing-reader");
        if(c.available!=expected || BackendCapabilities::get("tesseract").available || ToolRegistry::entries().size()!=16)throw std::runtime_error("capability/legacy registry mismatch");
        bool rejected=false;
        try {BackendCapabilities::require("zxing-reader","future",QString(64,'a'));}catch(const std::exception&){rejected=true;}
        if(!rejected)throw std::runtime_error("missing/changed backend accepted");
        if(expected)BackendCapabilities::require(c.id,c.version,c.contentId);
        QTextStream(stdout)<<"reader="<<c.available<<" version="<<c.version<<" contentId="<<c.contentId<<" reason="<<c.reason<<"\n";
        return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
