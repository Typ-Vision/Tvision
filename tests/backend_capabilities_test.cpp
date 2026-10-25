#include "application/BackendCapabilities.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <QSet>
#include <stdexcept>
using namespace visionflow;
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const bool expected=argc==2 && QString(argv[1])=="1";
        const auto c=BackendCapabilities::get("zxing-reader");
        if(c.available!=expected || BackendCapabilities::get("tesseract").available)throw std::runtime_error("backend capability mismatch");
        const QSet<QString> legacyKinds{"image_file","template_match","blob","edge","calculate","webcam","measurement",
            "preprocess","code_read","edge_point","geometry_measurement","edge_pair","line_fit","circle_fit","pattern_pose","shape_pose"};
        QSet<QString> actualKinds;
        for(const auto& entry:ToolRegistry::entries()) {
            if(actualKinds.contains(entry.kind))throw std::runtime_error("duplicate registry kind");
            actualKinds.insert(entry.kind);
        }
        auto expectedKinds=legacyKinds;expectedKinds.insert("industrial_camera");expectedKinds.insert("roi");
        if(actualKinds!=expectedKinds)throw std::runtime_error("legacy and industrial registry mismatch");
        const auto industrial=ToolRegistry::get("industrial_camera");
        if(!industrial.acquisitionSource || industrial.minimumEditorSchema!=9 || !industrial.requiredBackend.isEmpty())
            throw std::runtime_error("industrial source schema/backend boundary mismatch");
        bool rejected=false;
        try {BackendCapabilities::require("zxing-reader","future",QString(64,'a'));}catch(const std::exception&){rejected=true;}
        if(!rejected)throw std::runtime_error("missing/changed backend accepted");
        if(expected)BackendCapabilities::require(c.id,c.version,c.contentId);
        QTextStream(stdout)<<"reader="<<c.available<<" version="<<c.version<<" contentId="<<c.contentId<<" reason="<<c.reason<<"\n";
        return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
