#include "application/AcquisitionService.hpp"
#include "application/ImageFileService.hpp"
#include "application/ExecutionService.hpp"
#include "application/ToolExecutors.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QUuid>
#include <QTextStream>
#include <QTimeZone>
#include <QTemporaryDir>
#include <QFile>
#include <QCryptographicHash>
#include "vision/ImageAccess.hpp"
#include <stdexcept>
#include <array>
using namespace visionflow;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct ReusingAdapter final:AcquisitionAdapter {
    mutable std::array<unsigned char,12> buffer{10,20,99,99,30,40,99,99,50,60,99,99};
    AcquisitionCapabilities capabilities() const override {return {"fake_buffer",true};}
    ImageFrame acquire(const AcquisitionRequest&) const override {
        FrameMetadata metadata;metadata.exposureAtUtc=QDateTime::fromMSecsSinceEpoch(1000,QTimeZone::UTC);metadata.deviceTimestampTicks=123;metadata.deviceClockId="device-clock";
        auto owned=ImageFrame::fromBuffer(buffer.data(),buffer.size(),2,3,4,PixelFormat::Gray8,metadata,false);
        buffer.fill(255);return owned;
    }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        ReusingAdapter adapter;auto acquired=AcquisitionService{}.acquire(adapter,{"source","cycle","frame",TriggerMode::Software,5000,{}});
        check(acquired.state==AcquisitionState::Done,"preview-free acquired frame valid");const auto& f=acquired.frame->image;
        check(f.preview().isNull() && f.size()==QSize(2,3) && f.pixelText(1,2)=="Gray: 60","owned buffer survived adapter reuse; raw size independent");
        check(f.metadata().sourceId=="source" && f.metadata().cycleId=="cycle" && f.metadata().frameId=="frame" && f.metadata().coordinateSystemId=="source/image","acquisition identity");
        check(f.geometry().rootSize()==f.size() && f.geometry().rasterSize()==f.size() && f.geometry().rootOffset()==QPoint(0,0) &&
            f.geometry().coordinateSystemId()==f.metadata().coordinateSystemId && f.geometry().rootCoordinateSystemId()==f.metadata().coordinateSystemId && f.geometry().cropDepth()==0,"acquisition binds owned root geometry");
        check(f.metadata().completedAtUtc.timeSpec()==Qt::UTC && f.metadata().exposureAtUtc->toMSecsSinceEpoch()==1000 && *f.metadata().deviceTimestampTicks==123 && f.metadata().deviceClockId=="device-clock","completion exposure device clock separate");
        std::array<unsigned short,6> gray16{1000,65535,0,42,64,0};auto g=ImageFrame::fromBuffer(gray16.data(),sizeof(gray16),2,2,6,PixelFormat::Gray16);gray16.fill(0);
        check(g.pixelText(1,0)=="Gray: 65535" && g.pixelText(0,1)=="Gray: 42" && g.preview().pixelColor(1,0).red()==255,"Gray16 padded ownership and preview");
        std::array<unsigned char,8> rgb{1,2,3,99,4,5,6,99};auto color=ImageFrame::fromBuffer(rgb.data(),rgb.size(),1,2,4,PixelFormat::Rgb8);rgb.fill(0);
        check(color.pixelText(0,1)=="R:4 G:5 B:6" && color.preview().pixelColor(0,0).red()==1,"RGB row padding/color order");
        std::array<unsigned char,4> rgba{7,8,9,10};auto alpha=ImageFrame::fromBuffer(rgba.data(),rgba.size(),1,1,4,PixelFormat::Rgba8);
        check(alpha.pixelText(0,0)=="R:7 G:8 B:9 A:10" && alpha.preview().pixelColor(0,0).alpha()==10,"RGBA alpha order");
        auto copied=g;auto metadata=g.metadata();metadata.sourceId="other";auto rebound=g.withMetadata(metadata);
        check(copied.pixelText(1,0)=="Gray: 65535" && g.metadata().sourceId.isEmpty() && rebound.metadata().sourceId=="other","immutable shared pixels independent metadata");
        check(copied.geometry()==g.geometry() && rebound.geometry().identity()!=g.geometry().identity() && rebound.contentId()==g.contentId(),"root metadata binding updates geometry independently of content");
        for(const auto& sample:{f,g,color,alpha,rebound}){
            QCryptographicHash hash(QCryptographicHash::Sha256);
            hash.addData(QByteArray::number(sample.size().width())+":"+QByteArray::number(sample.size().height())+":"+QByteArray::number(int(sample.format())));
            const ImageAccess access(sample);
            for(int y=0;y<sample.size().height();++y){const auto row=access.row(y);hash.addData(QByteArrayView(reinterpret_cast<const char*>(row.data()),row.size()));}
            check(sample.contentId()==hash.result(),"cached identity preserves legacy native pixels/format and excludes padding/metadata");
        }
        for(int mode=0;mode<4;++mode){bool rejected=false;try{ImageFrame::fromBuffer(rgba.data(),mode==0 ? 1 : rgba.size(),mode==1 ? 0 : 1,mode==2 ? 2 : 1,mode==3 ? 0 : 4,PixelFormat::Rgba8);}catch(const std::exception&){rejected=true;}check(rejected,"bad buffer bounds rejected");}
        QTemporaryDir directory(QCoreApplication::applicationDirPath()+"/owned-frame-XXXXXX");check(directory.isValid(),"frame fixture directory");
        const auto fixturePath=directory.filePath("owned.pgm");QFile fixture(fixturePath);
        check(fixture.open(QIODevice::WriteOnly),"frame source fixture");
        fixture.write("P5\n2 3\n255\n");for(int i=1;i<=6;++i)fixture.putChar(char(10*i));fixture.close();
        const auto loaded=ImageFileService{}.load(fixturePath);
        check(loaded.geometry().rootSize()==QSize(2,3) && loaded.geometry().rasterSize()==loaded.size() && loaded.geometry().cropDepth()==0 &&
            loaded.geometry().rootOffset()==QPoint(0,0) && loaded.geometry().toRoot({1,2})==QPointF(1,2),"file decode creates owned root geometry");
        auto sourceId=QUuid::createUuid().toString();ToolDefinition source{sourceId,"image_file","Source","",true,{{"path",fixturePath}}};
        ToolDefinition blob{QUuid::createUuid().toString(),"blob","Blob","",true,defaultParameters("blob")};
        auto def=withEditorConnections(withEditorTools({}, {source,blob}),{{source.id,"image",blob.id,"image"}});
        auto snapshot=f.metadata();snapshot.sourceId=source.id;snapshot.coordinateSystemId=source.id+"/image";
        auto cycle=ExecutionService{}.run(def,{},"recipe","config",{}, {},f.withMetadata(snapshot));
        if(cycle.state!=ExecutionState::Done){QTextStream(stderr)<<cycle.error<<"\n";for(const auto& result:cycle.tools)QTextStream(stderr)<<result.message<<"\n";}
        check(cycle.state==ExecutionState::Done && cycle.frames.size()==1,"owned raw frame executes without preview");
        check(cycle.frames[0].sourceId==source.id && cycle.frames[0].exposureAtUtc==f.metadata().exposureAtUtc && cycle.frames[0].completedAtUtc==f.metadata().completedAtUtc,"metadata survives execution snapshot");
        for(const auto& tool:cycle.tools)for(const auto& v:tool.outputs)check(v.origin.frameId==cycle.frameId && v.origin.cycleId==cycle.cycleId && v.origin.sourceId==source.id,"derived output provenance");
        auto wrong=ExecutionService{}.run(def,{},"recipe","config",{}, {},f);check(wrong.state==ExecutionState::Error,"foreign source snapshot rejected");
        ToolRegistry::registerType({"test_point","Point",1,{{"coordinates","camera-a"}}, {}, {{"point",PortType::Point,"px","image"}},[](const ToolDefinition& t){if(!t.parameters["coordinates"].isString())throw std::runtime_error("coordinates");}});
        ToolExecutors::registerExecutor("test_point",[](const ToolDefinition& t,const QMap<QString,PortValue>&,const QString&,const QString& frame,const QString& cycle,const std::shared_ptr<std::atomic_bool>&) {
            ToolResult r;r.toolId=t.id;r.state=ExecutionState::Done;FrameMetadata m;m.frameId=frame;m.cycleId=cycle;m.sourceId="source";m.coordinateSystemId=t.parameters["coordinates"].toString();if(m.coordinateSystemId=="foreign-frame"){m.frameId="old-frame";m.coordinateSystemId="camera-a";}
            auto port=outputPorts(t.kind).first();
            const auto mode=m.coordinateSystemId;
            if(mode.startsWith("wrong-")) {
                m.coordinateSystemId="camera-a";
                if(mode=="wrong-unit")port.unit="mm";
                if(mode=="wrong-type")port.type=PortType::Region;
                if(mode=="wrong-port-frame")port.frame="robot";
            }
            r.outputs["point"]={port,QPointF(1,2),true,frame,cycle,m};return r;
        });
        ToolDefinition a{QUuid::createUuid().toString(),"test_point","A","",true,{{"coordinates","camera-a"}}},b{QUuid::createUuid().toString(),"test_point","B","",true,{{"coordinates","camera-b"}}},distance{QUuid::createUuid().toString(),"calculate","Distance","",true,defaultParameters("calculate")};
        auto mismatched=withEditorConnections(withEditorTools({}, {a,b,distance}),{{a.id,"point",distance.id,"p1"},{b.id,"point",distance.id,"p2"}});
        auto rejected=ExecutionService{}.run(mismatched,{},"recipe","config",{});
        check(rejected.state==ExecutionState::Error && rejected.tools.last().outputs.isEmpty() && rejected.tools.last().message.contains("koordinat"),"same cycle cannot mix camera coordinate systems");
        b.parameters["coordinates"]="camera-a";
        auto compatible=withEditorConnections(withEditorTools({}, {a,b,distance}),editorConnections(mismatched));
        check(ExecutionService{}.run(compatible,{},"recipe","config",{}).state==ExecutionState::Done,"compatible point provenance accepted");
        b.parameters["coordinates"]="foreign-frame";
        auto oldFrame=withEditorConnections(withEditorTools({}, {a,b,distance}),editorConnections(mismatched));
        check(ExecutionService{}.run(oldFrame,{},"recipe","config",{}).state==ExecutionState::Error,"mismatched origin frame rejected");
        for(const auto mode:{"wrong-unit","wrong-type","wrong-port-frame"}) {
            b.parameters["coordinates"]=mode;
            const auto invalidPort=withEditorConnections(withEditorTools({}, {a,b,distance}),editorConnections(mismatched));
            const auto invalid=ExecutionService{}.run(invalidPort,{},"recipe","config",{});
            check(invalid.state==ExecutionState::Error && invalid.tools[1].state==ExecutionState::Error && invalid.tools[1].outputs.isEmpty() && invalid.tools.last().state==ExecutionState::Skipped && invalid.tools.last().outputs.isEmpty(),"invalid producer port is rejected before downstream measurement");
        }
        QTextStream(stdout)<<"Owned frame contract passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}
}
