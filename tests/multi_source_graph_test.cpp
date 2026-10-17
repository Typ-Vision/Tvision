#include "application/SourceFrameIndex.hpp"
#include "application/ProductionRecipeService.hpp"
#include "persistence/ProjectRepository.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>
using namespace visionflow;
namespace {
int checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F> void reject(F f,const char* why){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,why);}
ToolDefinition tool(QString kind,QString name){return {QUuid::createUuid().toString(QUuid::WithoutBraces),kind,name,"",true,defaultParameters(kind)};}
QJsonObject definition(const QVector<ToolDefinition>& tools,const QVector<ToolConnection>& connections,int schema=9){
    QJsonArray array;for(const auto& t:tools)array.append(encodeEditorTool(t,schema));
    return withEditorConnections({{"editor",QJsonObject{{"schemaVersion",schema},{"tools",array},{"connections",QJsonArray{}}}}},connections);
}
bool cross(const GraphValidation& graph){return graph.errors.join("\n").contains("cross-source");}
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir dir(QDir::current().filePath("kmc10a-XXXXXX"));check(dir.isValid(),"test directory under build");
        for(int n:{1,2,8,10}) {
            QVector<ToolDefinition> tools;QVector<ToolConnection> connections;QVector<FrameMetadata> frames;
            QStringList sources;
            for(int i=0;i<n;++i) {
                auto source=tool(i%2 ? "webcam":"image_file",QString("Camera %1").arg(i));
                auto pre=tool("preprocess",QString("Pre %1").arg(i));auto edge=tool("edge",QString("Edge %1").arg(i));
                tools<<source<<pre<<edge;connections<<ToolConnection{source.id,"image",pre.id,"image"}<<ToolConnection{pre.id,"image",edge.id,"image"};
                sources<<source.id;frames.append({source.id,QString("frame-%1").arg(i),"cycle",source.id+"/image"});
            }
            const auto d=definition(tools,connections);validateEditorDefinition(d);
            const auto graph=validateSourceGraph(editorTools(d),editorConnections(d));check(graph.valid(),"independent N-source DAG");
            check(graph.requiredSources.size()==n,"required source count");
            SourceFrameIndex index("cycle",sources,frames);check((n==1)==!index.legacyFrameId().isEmpty(),"legacy ID only for one frame");
            for(int i=0;i<n;++i) {
                const auto& edge=tools[i*3+2];check(graph.toolSources[edge.id]==sources[i],"preprocess retains branch identity");
                for(const auto& p:outputPorts(edge.kind))check(graph.portSources[edge.id][p.id]==sources[i],"per-port origin");
                PortValue value{{"point",PortType::Point,"px","image"},QPointF(2,3),true,frames[i].frameId,"cycle",frames[i]};
                index.validatePort(sources[i],value,value.port);check(true,"own port accepted");
                if(n>1)reject([&]{index.validatePort(sources[(i+1)%n],value,value.port);},"other camera port rejected");
                auto bad=value;bad.origin.coordinateSystemId="other/image";reject([&]{index.validatePort(sources[i],bad,bad.port);},"coordinate swap");
                bad=value;bad.origin.frameId="foreign";reject([&]{index.validatePort(sources[i],bad,bad.port);},"frame swap");
                bad=value;bad.origin.cycleId="foreign";reject([&]{index.validatePort(sources[i],bad,bad.port);},"cycle swap");
            }
            // Byte-identical rasters never establish shared identity.
            const QByteArray pixels(16,'x');
            auto image=ImageFrame::fromBuffer(pixels.constData(),pixels.size(),4,4,4,PixelFormat::Gray8,frames[0]);
            PortValue raster{{"image",PortType::Image,"px","image"},image,true,frames[0].frameId,"cycle",frames[0]};
            index.validatePort(sources[0],raster,raster.port);
            if(n>1){raster.origin=frames[1];raster.frameId=frames[1].frameId;reject([&]{index.validatePort(sources[1],raster,raster.port);},"same pixels inner image origin swap");}
            auto badFrames=frames;badFrames[0].sourceId="foreign";reject([&]{SourceFrameIndex bad("cycle",sources,badFrames);},"foreign source");
            badFrames=frames;badFrames[0].cycleId="foreign";reject([&]{SourceFrameIndex bad("cycle",sources,badFrames);},"foreign cycle");
            badFrames=frames;badFrames[0].deviceClockId=QString(257,'x');reject([&]{SourceFrameIndex bad("cycle",sources,badFrames);},"bounded optional device clock identity");
            badFrames=frames;badFrames.removeLast();reject([&]{SourceFrameIndex bad("cycle",sources,badFrames);},"missing frame");
            if(n>1){badFrames=frames;badFrames[1]=badFrames[0];reject([&]{SourceFrameIndex bad("cycle",sources,badFrames);},"duplicate source");badFrames=frames;badFrames[1].frameId=badFrames[0].frameId;reject([&]{SourceFrameIndex bad("cycle",sources,badFrames);},"duplicate frame");}
            auto badTools=tools;badTools[0].enabled=false;check(!validateSourceGraph(badTools,connections).valid(),"disabled required source");
            badTools=tools;for(int j=0;j<3;++j)badTools[j].enabled=false;
            check(validateSourceGraph(badTools,connections).valid() && validateSourceGraph(badTools,connections).requiredSources.size()==n-1,"disabled complete branch is not required");
            badTools=tools;badTools.removeFirst();check(!validateSourceGraph(badTools,connections).valid(),"deleted required source");
            badTools=tools;badTools[1].id=badTools[0].id;check(!validateSourceGraph(badTools,connections).valid(),"graph duplicate IDs");
            reject([&]{editorTools(definition(badTools,connections));},"schema duplicate IDs");
            badTools=tools;badTools[1].name=badTools[0].name.toUpper();reject([&]{editorTools(definition(badTools,connections));},"case-folded duplicate names");
            auto renamed=tools;renamed[0].name="Renamed camera";check(validateSourceGraph(renamed,connections).toolSources==graph.toolSources,"name is not origin");
            const auto execution=ExecutionService{}.run(d,dir.path(),"recipe","config",std::make_shared<std::atomic_bool>(true));
            check(execution.state==ExecutionState::Skipped && execution.acquisitions==0 && execution.frames.isEmpty() && (n==1 || execution.frameId.isEmpty()),"schema9 pre-cancel never opens file/webcam or invents acquired frames");
            for(auto type:{ApplicationType::RobotGuidance,ApplicationType::DefectInspection,ApplicationType::Measurement}) {
                Recipe r;r.id=QUuid::createUuid().toString();r.name="Recipe";r.cameraReference="file";r.definition=d;r.applicationType=type;r.createdAt=r.modifiedAt=QDateTime::currentDateTimeUtc();
                Project p;p.id=QUuid::createUuid().toString();p.name="Project";p.cameraReference="file";p.createdAt=p.modifiedAt=r.createdAt;p.recipes={r};
                ProjectRepository repository;const auto path=dir.filePath("roundtrip.vfproject");repository.save(path,p);
                check(ProjectRepository::encode(repository.load(path))==ProjectRepository::encode(p),"all application types project roundtrip");
                ProductionRecipeService production([](const QString&,ProductionAction){return true;});
                try{production.validate("engineer",r,dir.path());check(false,"production must reject");}catch(const ProductionException& e){check(e.code()==ProductionError::InvalidConfiguration && QString::fromUtf8(e.what()).contains("whole source-set"),"direct legacy production requires whole-set Runtime");}
            }
        }
        auto a=tool("image_file","A"),b=tool("image_file","B"),ea=tool("edge","EA"),eb=tool("edge","EB"),calc=tool("calculate","Distance"),match=tool("template_match","Match");
        QVector<ToolDefinition> tools{a,b,ea,eb,calc,match};
        QVector<ToolConnection> base{{a.id,"image",ea.id,"image"},{b.id,"image",eb.id,"image"},{a.id,"image",match.id,"image"},{ea.id,"point",calc.id,"p1"},{eb.id,"point",calc.id,"p2"}};
        check(cross(validateSourceGraph(tools,base)),"cross-camera point geometry");
        auto rebound=base;rebound.last().sourceTool=ea.id;check(validateSourceGraph(tools,rebound).valid(),"explicit same-source rebind");
        auto region=rebound;region.append({match.id,"region",eb.id,"region"});check(cross(validateSourceGraph(tools,region)),"cross-camera region");
        auto reference=rebound;reference.append({ea.id,"point",eb.id,"reference"});check(cross(validateSourceGraph(tools,reference)),"cross-camera reference");
        auto images=rebound;images.append({b.id,"image",ea.id,"image"});check(cross(validateSourceGraph(tools,images)),"cross-camera image merge");
        auto lineA=tool("line_fit","LineA"),lineB=tool("line_fit","LineB"),geometry=tool("geometry_measurement","Geometry");
        auto geometryTools=QVector<ToolDefinition>{a,b,lineA,lineB,geometry};
        check(cross(validateSourceGraph(geometryTools,{{a.id,"image",lineA.id,"image"},{b.id,"image",lineB.id,"image"},{lineA.id,"linePx",geometry.id,"line1"},{lineB.id,"linePx",geometry.id,"line2"}})),"cross-camera typed geometry");
        auto legacy=definition({a,ea},{{a.id,"image",ea.id,"image"}},8);const auto bytes=QJsonDocument(legacy).toJson(QJsonDocument::Compact);
        validateEditorDefinition(legacy);check(QJsonDocument(withEditorTools(legacy,editorTools(legacy))).toJson(QJsonDocument::Compact)==bytes,"legacy no-op bytes");
        reject([&]{editorTools(definition({a,b},{},8));},"legacy second source cannot silently migrate");
        auto migrated=withMultiSourceSchema(legacy);check(migrated["editor"].toObject()["schemaVersion"]==9 && editorConnections(migrated)==editorConnections(legacy),"explicit schema migration retains links");
        check(withMultiSourceSchema(migrated)==migrated,"migration idempotent");
        check(editorTools(withEditorTools(migrated,{a,b,ea})).size()==3,"explicit opt-in admits second source");
        auto future=migrated;auto editor=future["editor"].toObject();editor["schemaVersion"]=10;future["editor"]=editor;reject([&]{validateEditorDefinition(future);},"future schema rejected");
        editor["schemaVersion"]=9.5;future["editor"]=editor;reject([&]{validateEditorDefinition(future);},"fractional schema rejected");
        editor["schemaVersion"]=9;editor["unknown"]=true;future["editor"]=editor;reject([&]{validateEditorDefinition(future);},"unknown schema field rejected");
        auto referenceTool=ea;referenceTool.parameters["referenceBinding"]=QJsonObject{};
        check(requiredEditorSchema({referenceTool},9)==9,"reference schema cannot downgrade explicit multi-source opt-in");
        reject([&]{SourceFrameIndex bad("cycle",{},{});},"empty frame index");
        QStringList excess;QVector<FrameMetadata> excessFrames;for(int i=0;i<257;++i){const auto s=QString("source-%1").arg(i);excess<<s;excessFrames.append({s,QString("frame-%1").arg(i),"cycle",s+"/image"});}
        reject([&]{SourceFrameIndex bad("cycle",excess,excessFrames);},"bounded frame index");
        excess.removeLast();excessFrames.removeLast();SourceFrameIndex maximum("cycle",excess,excessFrames);check(maximum.legacyFrameId().isEmpty(),"256-source frame index admitted");
        QVector<ToolDefinition> maximumTools;for(int i=0;i<256;++i)maximumTools.append(tool("image_file",QString("Source %1").arg(i)));
        check(editorTools(definition(maximumTools,{})).size()==256,"existing 256-tool budget includes 256 sources");
        maximumTools.append(tool("image_file","Overflow"));reject([&]{editorTools(definition(maximumTools,{}));},"257 tools rejected");
        QTextStream(stdout)<<"multi source graph: "<<checks<<" checks passed\n";return 0;
    }catch(const std::exception& e){QTextStream(stderr)<<e.what()<<" after "<<checks<<" checks\n";return 1;}
}
